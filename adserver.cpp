#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <cstdlib>
#include <signal.h>

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

class AdServer {
private:
    asio::io_context& io_context_;
    tcp::acceptor acceptor_;
    std::string pod_name_;
    std::string pod_namespace_;
    
    // HTTP client for database communication
    asio::io_context client_io_context_;
    
public:
    AdServer(asio::io_context& io_context, unsigned short port)
        : io_context_(io_context)
        , acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
    {
        // Get environment variables
        pod_name_ = std::getenv("POD_NAME") ? std::getenv("POD_NAME") : "unknown-pod";
        pod_namespace_ = std::getenv("POD_NAMESPACE") ? std::getenv("POD_NAMESPACE") : "unknown-namespace";
        
        std::cout << "AdServer starting on port " << port << std::endl;
        std::cout << "Pod: " << pod_name_ << ", Namespace: " << pod_namespace_ << std::endl;
        
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<Session>(std::move(socket), *this)->start();
                }
                do_accept();
            });
    }

    std::string read_file(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return "Error: Could not open file " + path;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    std::string make_http_request(const std::string& host, const std::string& port, 
                                 const std::string& path, const std::string& query = "") {
        try {
            asio::io_context io_ctx;
            tcp::resolver resolver(io_ctx);
            
            auto const results = resolver.resolve(host, port);
            tcp::socket socket(io_ctx);
            asio::connect(socket, results);
            
            http::request<http::string_body> req{http::verb::get, path + (query.empty() ? "" : "?" + query), 11};
            req.set(http::field::host, host);
            req.set(http::field::user_agent, "AdServer/1.0");
            
            http::write(socket, req);
            
            beast::flat_buffer buffer;
            http::response<http::string_body> res;
            http::read(socket, buffer, res);
            
            return res.body();
        } catch (const std::exception& e) {
            return "Error: " + std::string(e.what());
        }
    }

public:
    class Session : public std::enable_shared_from_this<Session> {
    private:
        tcp::socket socket_;
        AdServer& server_;
        beast::flat_buffer buffer_;
        http::request<http::string_body> req_;
        http::response<http::string_body> res_;

    public:
        Session(tcp::socket socket, AdServer& server)
            : socket_(std::move(socket))
            , server_(server) {
        }

        void start() {
            read_request();
        }

    private:
        void read_request() {
            auto self = shared_from_this();

            http::async_read(socket_, buffer_, req_,
                [self](beast::error_code ec, std::size_t) {
                    if (!ec) {
                        self->handle_request();
                    }
                });
        }

        void handle_request() {
            res_.version(req_.version());
            res_.keep_alive(false);

            std::string target = std::string(req_.target());
            
            if (target == "/") {
                handle_root();
            } else if (target.find("/banner") == 0) {
                handle_banner();
            } else if (target == "/geo") {
                handle_geo();
            } else if (target == "/plz") {
                handle_plz();
            } else if (target == "/cfg") {
                handle_cfg();
            } else {
                res_.result(http::status::not_found);
                res_.set(http::field::content_type, "text/plain");
                res_.body() = "Not Found";
            }

            write_response();
        }

        void handle_root() {
            res_.result(http::status::ok);
            res_.set(http::field::content_type, "application/json");
            res_.body() = "{\"name\":\"" + server_.pod_name_ + "\",\"namespace\":\"" + server_.pod_namespace_ + "\"}";
        }

        void handle_banner() {
            // Parse query parameters
            std::string query = std::string(req_.target());
            size_t pos = query.find('?');
            if (pos == std::string::npos) {
                res_.result(http::status::bad_request);
                res_.set(http::field::content_type, "text/plain");
                res_.body() = "Missing query parameters";
                return;
            }

            std::string params = query.substr(pos + 1);
            std::map<std::string, std::string> query_params;
            
            // Simple query parameter parsing
            size_t start = 0;
            size_t end = 0;
            while ((end = params.find('&', start)) != std::string::npos) {
                std::string pair = params.substr(start, end - start);
                size_t eq_pos = pair.find('=');
                if (eq_pos != std::string::npos) {
                    std::string key = pair.substr(0, eq_pos);
                    std::string value = pair.substr(eq_pos + 1);
                    query_params[key] = value;
                }
                start = end + 1;
            }
            
            // Handle last parameter
            if (start < params.length()) {
                std::string pair = params.substr(start);
                size_t eq_pos = pair.find('=');
                if (eq_pos != std::string::npos) {
                    std::string key = pair.substr(0, eq_pos);
                    std::string value = pair.substr(eq_pos + 1);
                    query_params[key] = value;
                }
            }

            auto name_it = query_params.find("name");
            if (name_it == query_params.end()) {
                res_.result(http::status::bad_request);
                res_.set(http::field::content_type, "text/plain");
                res_.body() = "Missing 'name' parameter";
                return;
            }

            // Make request to database
            std::string banner_data = server_.make_http_request("db-svc", "80", "/get", "k=banner-" + name_it->second);
            
            if (banner_data.find("Error:") == 0) {
                res_.result(http::status::internal_server_error);
                res_.set(http::field::content_type, "application/json");
                res_.body() = "{\"error\":\"Couldn't get the banner\"}";
            } else {
                res_.result(http::status::ok);
                res_.set(http::field::content_type, "application/json");
                res_.body() = "{\"banner\":\"" + banner_data + "\"}";
            }
        }

        void handle_geo() {
            res_.result(http::status::ok);
            res_.set(http::field::content_type, "text/plain");
            res_.body() = server_.read_file("/mnt/adserver_geodata/geodata.txt");
        }

        void handle_plz() {
            res_.result(http::status::ok);
            res_.set(http::field::content_type, "text/plain");
            res_.body() = server_.read_file("/mnt/adserver_geodata/plz.txt");
        }

        void handle_cfg() {
            res_.result(http::status::ok);
            res_.set(http::field::content_type, "text/plain");
            res_.body() = server_.read_file("/mnt/adserver_config/adserver-config.conf");
        }

        void write_response() {
            auto self = shared_from_this();

            res_.prepare_payload();

            http::async_write(socket_, res_,
                [self](beast::error_code ec, std::size_t) {
                    self->socket_.shutdown(tcp::socket::shutdown_both, ec);
                });
        }
    };
};

void signal_handler(int) {
    std::cout << "Terminating" << std::endl;
    exit(0);
}

int main() {
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    try {
        asio::io_context io_context;
        AdServer server(io_context, 80);
        
        std::cout << "AdServer running on port 80" << std::endl;
        io_context.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} 
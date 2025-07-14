from flask import Flask, request, jsonify
import os
import requests
import signal

app = Flask(__name__)

@app.route("/new-banner", methods=["GET"])
def new_banner():
    response = requests.get(
        url=f"http://db-svc/set",
        params={
            "k": "banner-" + request.args.get("name"),
            "v": request.args.get("data")
        },
        timeout=5
    )
    if response.status_code != 200:
        return jsonify({
            "error": "Couldn't save the banner",
            "dbcode": response.status_code
        }), 500
    return ""

def terminate(signal, frame):
    print("Terminating")
    sys.exit(0)

if __name__ == "__main__":
    signal.signal(signal.SIGTERM, terminate)
    app.run(host="0.0.0.0", port=80)

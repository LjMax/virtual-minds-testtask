##########################################################################################
#                                         README
#
# Reference implementation which implements the required logic correctly, but in Python,
# not efficiently at all, and not very customizable nor extendible. It must be replaced
# with your C++ implementation.
#
##########################################################################################

from flask import Flask, request, jsonify
from pathlib import Path
import os
import requests
import signal

app = Flask(__name__)
pod_name = os.getenv("POD_NAME", "unknown-pod")
pod_namespace = os.getenv("POD_NAMESPACE", "unknown-namespace")

# Path: "/". Return some basic info about the node. Specifically, we are interested in the
# Kubernetes Pod and Namespace names.
@app.route("/", methods=["GET"])
def index():
    return jsonify({
        "name": pod_name,
        "namespace": pod_namespace
    })

# Path: "/banner". Return the banner data if found.
@app.route("/banner", methods=["GET"])
def get_banner():
    # This code fetches the banner from a database on each request. Caching is of course
    # more preferable, but then need to find out how to reload the banner when it was
    # changed in the DB.
    response = requests.get(
        url=f"http://db-svc/get",
        params={"k": "banner-" + request.args.get("name")},
        timeout=5
    )
    if response.status_code != 200:
        return jsonify({"error": "Couldn't get the banner"}), response.status_code
    return jsonify({
        "banner": response.text
    }), 200

# Path: "/geo". Geographical data, like IP -> Country matching. We might use it for
# targeting and filters.
@app.route("/geo", methods=["GET"])
def get_geodata():
    return Path("/mnt/adserver_geodata/geodata.txt").read_text()

# Path: "/plz". Postcodes information, also for targeting and filters.
@app.route("/plz", methods=["GET"])
def get_postcodes():
    return Path("/mnt/adserver_geodata/plz.txt").read_text()

# Path: "/cfg". Configuration of this specific adserver.
@app.route("/cfg", methods=["GET"])
def get_config():
    return Path("/mnt/adserver_config/adserver-config.conf").read_text()

def terminate(signal, frame):
    print("Terminating")
    sys.exit(0)

if __name__ == "__main__":
    signal.signal(signal.SIGTERM, terminate)
    app.run(host="0.0.0.0", port=80)

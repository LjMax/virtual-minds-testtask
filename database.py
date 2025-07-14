from flask import Flask, request, jsonify
import os
import signal

app = Flask(__name__)

data = dict()
version = 0

@app.route("/get", methods=["GET"])
def data_get():
    global data
    key = request.args.get("k")
    val = data.get(key)
    if not val:
        return "", 404
    return val

@app.route("/set", methods=["GET"])
def data_set():
    global data
    global version
    version += 1
    key = request.args.get("k")
    val = request.args.get("v")
    data[key] = val
    return ""

@app.route("/version", methods=["GET"])
def version_get():
    global data
    global version
    return jsonify({
        "version": version,
        "data": data
    })

def terminate(signal, frame):
    print("Terminating")
    sys.exit(0)

if __name__ == "__main__":
    signal.signal(signal.SIGTERM, terminate)
    app.run(host="0.0.0.0", port=80)

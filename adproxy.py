from flask import Flask, request, jsonify
import os
import requests
import signal
import random

KUBE_API_SERVER = \
    f"https://{os.environ['KUBERNETES_SERVICE_HOST']}:"\
        f"{os.environ['KUBERNETES_SERVICE_PORT']}"

TOKEN_FILE = "/var/run/secrets/kubernetes.io/serviceaccount/token"
CA_CERT_FILE = "/var/run/secrets/kubernetes.io/serviceaccount/ca.crt"

# Read token for authentication
with open(TOKEN_FILE, 'r') as f:
    TOKEN = f.read().strip()

headers = {
    "Authorization": f"Bearer {TOKEN}"
}

namespace = os.environ.get('POD_NAMESPACE', 'default')

def getPodList():
    response = requests.get(
        f"{KUBE_API_SERVER}/api/v1/namespaces/{namespace}/pods?"\
            "labelSelector=app%3Dadserver",
        headers=headers,
        verify=CA_CERT_FILE
    )
    if response.status_code == 200:
        pods = response.json()
        pod_names = [pod["metadata"]["name"] for pod in pods.get("items", [])]
        print("Response: ", pods)
        print("Pods in namespace:", namespace)
        print(pod_names)
        if not pod_names:
            print("No adserver pods are found")
            return None
        return pod_names
    else:
        print("Failed to get pods. Status code:", response.status_code)
        print("Response:", response.text)
        return None

app = Flask(__name__)

@app.route("/", defaults={"path": ""})
@app.route("/<path:path>", methods=["GET", "POST"])
def to_adserver(path):
    user_id = request.args.get("userID")
    host = request.args.get("host")

    replicas = getPodList()
    if not replicas:
        return jsonify({"error": "Couldn't get the pod list"}), 500
    
    # Special routing logic. It is necessary for requests consistency. For
    # example, multiple requests from the same user we want to send to the same
    # server, to avoid showing duplicate banners without doing any sync via
    # KV-stores.
    if host:
        target = host
    elif user_id and user_id.isdigit():
        target_index = int(user_id) % len(replicas)
        target = replicas[target_index]
    else:
        target = random.choice(replicas)
    target += '.ad-svc'

    try:
        print("Forwarding to {}".format(target))
        response = requests.request(
            method=request.method,
            url=f"http://{target}/{path}",
            headers=request.headers,
            params=request.args,
            timeout=5
        )
        return response.content, response.status_code, response.headers.items()
    except Exception as e:
        print("Error during forward: {}".format(e))
        return jsonify({"error": str(e)}), 500

def terminate(signal, frame):
    print("Terminating")
    sys.exit(0)

if __name__ == "__main__":
    signal.signal(signal.SIGTERM, terminate)
    app.run(host="0.0.0.0", port=80)

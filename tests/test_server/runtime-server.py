from flask import Flask, jsonify, request
import uuid
import time

app = Flask(__name__)

# Simulated queue of events
event_queue = [{"payload": "Event 1"}, {"payload": "Event 2"}, {"payload": "Event 3"}]


@app.route("/2018-06-01/runtime/invocation/next", methods=["GET"])
def next_invocation():
    if not event_queue:
        # If no more events, wait for a bit and return 204 No Content
        time.sleep(2)
        return "", 204

    event = event_queue.pop(0)
    request_id = str(uuid.uuid4())

    headers = {
        "Lambda-Runtime-Aws-Request-Id": request_id,
        "Lambda-Runtime-Deadline-Ms": str(
            int(time.time() * 1000) + 300000
        ),  # 5 minutes from now
        "Lambda-Runtime-Invoked-Function-Arn": "arn:aws:lambda:us-east-1:123456789012:function:test-function",
        "Lambda-Runtime-Trace-Id": f"Root=1-{uuid.uuid4().hex[:8]}-{uuid.uuid4().hex[:24]}",
    }

    return jsonify(event), 200, headers


@app.route(
    "/2018-06-01/runtime/invocation/<string:aws_request_id>/response", methods=["POST"]
)
def invocation_response(aws_request_id):
    response_data = request.get_json()
    print(f"Received response for request {aws_request_id}:")
    print(response_data)
    return "", 202


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=9001)

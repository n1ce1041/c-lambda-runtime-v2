#!/usr/bin/env python3

import json
import os
import subprocess
import sys
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

EVENTS = [b'{"message":"first"}', b'{"message":"second"}']
RESPONSES: list[bytes] = []


class RuntimeApiHandler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"
    pending_events = list(EVENTS)
    request_number = 0

    def do_GET(self) -> None:
        if self.path != "/2018-06-01/runtime/invocation/next":
            self.send_error(404)
            return

        if not self.pending_events:
            self.send_error(410)
            return

        payload = self.pending_events.pop(0)
        type(self).request_number += 1
        request_id = f"request-{self.request_number}"

        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(payload)))
        self.send_header("Lambda-Runtime-Aws-Request-Id", request_id)
        self.send_header("Lambda-Runtime-Deadline-Ms", "4102444800000")
        self.send_header(
            "Lambda-Runtime-Invoked-Function-Arn",
            "arn:aws:lambda:us-east-1:111122223333:function:test",
        )
        self.send_header(
            "Lambda-Runtime-Trace-Id",
            "Root=1-00000000-000000000000000000000000",
        )
        self.end_headers()
        self.wfile.write(payload)

    def do_POST(self) -> None:
        expected_prefix = "/2018-06-01/runtime/invocation/"
        if not self.path.startswith(expected_prefix) or not self.path.endswith(
            "/response"
        ):
            self.send_error(404)
            return

        content_length = int(self.headers.get("Content-Length", "0"))
        RESPONSES.append(self.rfile.read(content_length))
        self.send_response(202)
        self.send_header("Content-Length", "0")
        self.end_headers()

    def log_message(self, format: str, *args: object) -> None:
        return


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: integration_test.py PATH_TO_BOOTSTRAP", file=sys.stderr)
        return 2

    runtime_path = Path(sys.argv[1]).resolve()
    server = ThreadingHTTPServer(("127.0.0.1", 0), RuntimeApiHandler)
    server_thread = threading.Thread(target=server.serve_forever, daemon=True)
    server_thread.start()

    env = os.environ.copy()
    env["AWS_LAMBDA_RUNTIME_API"] = f"127.0.0.1:{server.server_port}"

    try:
        completed = subprocess.run(
            [str(runtime_path), "--max-invocations", str(len(EVENTS))],
            env=env,
            capture_output=True,
            text=True,
            timeout=10,
            check=False,
        )
    finally:
        server.shutdown()
        server.server_close()
        server_thread.join(timeout=2)

    if completed.returncode != 0:
        print(completed.stdout, file=sys.stderr)
        print(completed.stderr, file=sys.stderr)
        return 1

    if RESPONSES != EVENTS:
        print(
            f"unexpected responses: {json.dumps([r.decode() for r in RESPONSES])}",
            file=sys.stderr,
        )
        return 1

    print(f"integration test passed: {len(RESPONSES)} invocations")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

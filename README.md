# C Lambda Runtime

A small AWS Lambda custom runtime written in C11. The project implements the
Lambda Runtime API lifecycle directly with libcurl to explore systems
programming, HTTP, process lifecycle, memory ownership, and failure handling.

This is a learning project, not an AWS-supported runtime.

## Runtime lifecycle

For each invocation, the runtime:

1. Long-polls `GET /2018-06-01/runtime/invocation/next`.
2. Parses the request ID, deadline, function ARN, and trace ID from bounded HTTP
   header buffers.
3. Accumulates the event body across any number of libcurl callback chunks.
4. Calls the C handler with an owned invocation context.
5. Publishes the handler result or a function error to the Runtime API.
6. Frees invocation state and waits for the next request.

`SIGTERM` and `SIGINT` stop the loop cleanly after the current libcurl operation
returns. The demonstration handler echoes the invocation payload.

## Safety properties

- Heap-owned fields are zero-initialized and released between invocations.
- HTTP callbacks use the explicit buffer lengths supplied by libcurl.
- Connect and result-publication operations have bounded timeouts. The
  next-invocation request intentionally has no overall timeout because it is a
  long poll.
- libcurl failures and non-success HTTP responses stop the runtime rather than
  creating an unbounded retry loop.
- The runtime does not collect or print environment variables or event payloads.
- The local test server binds only to `127.0.0.1`.

The runtime does not configure Lambda invocation permissions, function URLs, or
API Gateway. Keep deployed functions private unless their intended audience and
authorization mechanism require otherwise.

## Dependencies

- A C11 compiler, such as GCC or Clang
- GNU Make
- libcurl development headers and library
- Python 3 for the local integration test
- Docker for the optional Lambda container image

On Debian or Ubuntu:

```sh
sudo apt-get install build-essential libcurl4-openssl-dev python3
```

## Build and test

```sh
make build             # build build/bootstrap
make test              # compile and run unit tests
make integration-test  # exercise two complete Runtime API invocations
make sanitize          # run tests with ASan and UBSan
make clean
```

The integration test starts a standard-library HTTP server on an ephemeral
loopback port. It verifies the complete `next -> handler -> response` path for
two invocations without requiring AWS credentials.

## Run against a local Runtime API

Set the same environment variable that Lambda supplies and start the runtime:

```sh
AWS_LAMBDA_RUNTIME_API=127.0.0.1:9001 ./build/bootstrap
```

For bounded local experiments, the executable accepts:

```sh
AWS_LAMBDA_RUNTIME_API=127.0.0.1:9001 \
  ./build/bootstrap --max-invocations 2
```

Do not use `--max-invocations` for a deployed Lambda runtime.

## Lambda container image

The multi-stage Dockerfile builds on Amazon Linux 2023 and places the executable
at `/var/runtime/bootstrap` in the Lambda provided-runtime base image:

```sh
make container
```

The image intentionally publishes no network ports. Push and deploy it through
your normal AWS tooling, with least-privilege Lambda invocation permissions.

## Project structure

```text
include/
  handler.h       Handler contract and invocation data
  runtime.h       Runtime API client contract
src/
  handler.c       Demonstration echo handler
  runtime.c       Runtime loop, HTTP client, parsing, and cleanup
tests/
  test_main.c          Unit tests
  integration_test.py Local Runtime API integration test
```

## Current scope

The project implements the core invocation lifecycle, response publication, and
error publication. It deliberately fails fast on Runtime API transport errors;
a production runtime would need a documented retry policy, telemetry, packaging
release process, and broader compatibility testing.

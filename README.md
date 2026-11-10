## C Lambda Runtime

A barebones implementation of an AWS Lambda Custom Runtime written in C.

This project explores the underlying mechanics of the AWS Lambda execution environment by implementing a runtime that communicates directly with the Lambda Runtime API.

Rather than relying on an existing language runtime, the goal is to understand the interfaces, execution lifecycle and communication mechanisms that allow Lambda functions to execute within their managed environment.

## Project Objectives

The project explores several systems-level concepts:

* Systems programming: Implementing runtime functionality in C.
* Linux: Understanding the execution environment and application lifecycle.
* Networking: HTTP communication with the Lambda Runtime API.
* Runtime internals: Understanding Lambda invocation handling and execution flow.
* Memory management: Working with C data structures and resource lifecycle.
* Software engineering: Modular implementation, compilation and unit testing.

AWS Lambda Custom Runtimes communicate with the Lambda Runtime API through an HTTP interface exposed inside the execution environment.

The intended runtime lifecycle follows this general sequence:

1. Initialise the runtime inside the Lambda execution environment.
2. Retrieve the next invocation from the Runtime API.
3. Process the invocation event.
4. Submit the execution result.
5. Continue polling for subsequent invocations.

Component	Technology
Language	C
C Standard	C11
Compiler	GCC
HTTP Client	libcurl
Build System	GNU Make
Testing	Unity

Repository Structure

.
├── build/       # Build artefacts
├── include/     # Header files
├── src/         # Runtime implementation
├── tests/       # Unit tests
├── Makefile     # Build configuration
└── README.md

## Building

The project uses GNU Make.

Compile the project:

````make build```

Build with debugging enabled:

```make DEBUG=1 build```

Build the test executable:

```make TEST=1 build```

Execute the resulting binary:

```make execute```

Clean build artefacts:

```make clean```

## Dependencies

* GCC
* GNU Make
* libcurl development libraries
* Unity testing framework (included in the repository)


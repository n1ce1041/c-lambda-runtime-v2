#define _POSIX_C_SOURCE 200809L

#include "handler.h"
#include "runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition)) {                                                        \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);   \
      failures++;                                                              \
    }                                                                          \
  } while (0)

static void test_build_url(void) {
  char *url = build_url("127.0.0.1:9001",
                        "2018-06-01/runtime/invocation/next");
  CHECK(url != NULL);
  if (url != NULL) {
    CHECK(strcmp(url,
                 "http://127.0.0.1:9001/2018-06-01/runtime/invocation/next") ==
          0);
  }
  free(url);
}

static void test_runtime_urls(void) {
  struct lambda_urls urls = {0};

  CHECK(init_runtime_urls(&urls, "runtime.internal:9001") == 0);
  CHECK(strcmp(urls.next_invocation,
               "http://runtime.internal:9001/2018-06-01/runtime/invocation/next") ==
        0);
  CHECK(strcmp(urls.init_error,
               "http://runtime.internal:9001/2018-06-01/runtime/init/error") ==
        0);
  CHECK(strcmp(urls.function_error_template,
               "http://runtime.internal:9001/2018-06-01/runtime/invocation/%s/error") ==
        0);
  CHECK(strcmp(urls.response_template,
               "http://runtime.internal:9001/2018-06-01/runtime/invocation/%s/response") ==
        0);

  free_runtime_urls(&urls);
  CHECK(urls.next_invocation == NULL);
  CHECK(urls.init_error == NULL);
  CHECK(urls.function_error_template == NULL);
  CHECK(urls.response_template == NULL);
}

static void test_payload_accumulates_chunks(void) {
  struct lambda_context context = {0};

  CHECK(append_event_payload(&context, "first", 5) == 0);
  CHECK(append_event_payload(&context, "-second", 7) == 0);
  CHECK(context.event_payload_length == 12);
  CHECK(memcmp(context.event_payload, "first-second", 12) == 0);
  CHECK(context.event_payload[12] == '\0');

  reset_lambda_context(&context);
  CHECK(context.event_payload == NULL);
  CHECK(context.event_payload_length == 0);
}

static void test_runtime_headers(void) {
  struct lambda_context context = {0};
  const char request_id[] = "Lambda-Runtime-Aws-Request-Id: request-123\r\n";
  const char deadline[] = "Lambda-Runtime-Deadline-Ms: 4102444800000\r\n";
  const char trace_id[] = "Lambda-Runtime-Trace-Id: Root=1-test\r\n";
  const char arn[] = "Lambda-Runtime-Invoked-Function-Arn: arn:test\r\n";

  CHECK(parse_runtime_header(&context, request_id, sizeof(request_id) - 1) == 0);
  CHECK(parse_runtime_header(&context, deadline, sizeof(deadline) - 1) == 0);
  CHECK(parse_runtime_header(&context, trace_id, sizeof(trace_id) - 1) == 0);
  CHECK(parse_runtime_header(&context, arn, sizeof(arn) - 1) == 0);

  CHECK(strcmp(context.request_id, "request-123") == 0);
  CHECK(context.deadline_ms == 4102444800000LL);
  CHECK(strcmp(context.xray_trace_id, "Root=1-test") == 0);
  CHECK(strcmp(context.invoked_function_arn, "arn:test") == 0);

  reset_lambda_context(&context);
}

static void test_handler_echoes_event(void) {
  struct lambda_context context = {0};
  struct lambda_result result = {0};
  const char event[] = "{\"hello\":\"runtime\"}";

  CHECK(append_event_payload(&context, event, sizeof(event) - 1) == 0);
  CHECK(lambda_handler(&context, &result) == 0);
  CHECK(result.payload_length == sizeof(event) - 1);
  CHECK(memcmp(result.payload, event, sizeof(event) - 1) == 0);

  free_lambda_result(&result);
  reset_lambda_context(&context);
  CHECK(result.payload == NULL);
}

int main(void) {
  test_build_url();
  test_runtime_urls();
  test_payload_accumulates_chunks();
  test_runtime_headers();
  test_handler_echoes_event();

  if (failures != 0) {
    fprintf(stderr, "%d test assertion(s) failed\n", failures);
    return EXIT_FAILURE;
  }

  puts("unit tests passed");
  return EXIT_SUCCESS;
}

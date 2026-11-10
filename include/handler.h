#ifndef HANDLER_H
#define HANDLER_H

#include <stddef.h>

struct lambda_context {
  char *request_id;
  long long deadline_ms;
  char *invoked_function_arn;
  char *xray_trace_id;
  char *event_payload;
  size_t event_payload_length;
};

struct lambda_result {
  char *payload;
  size_t payload_length;
};

int lambda_handler(const struct lambda_context *context,
                   struct lambda_result *result);
void free_lambda_result(struct lambda_result *result);

#endif

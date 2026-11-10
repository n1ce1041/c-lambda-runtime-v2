#include "handler.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int lambda_handler(const struct lambda_context *context,
                   struct lambda_result *result) {
  if (context == NULL || result == NULL || context->event_payload == NULL) {
    return -1;
  }

  result->payload = NULL;
  result->payload_length = 0;

  if (context->event_payload_length == SIZE_MAX) {
    return -1;
  }

  result->payload = malloc(context->event_payload_length + 1);
  if (result->payload == NULL) {
    return -1;
  }

  memcpy(result->payload, context->event_payload,
         context->event_payload_length);
  result->payload[context->event_payload_length] = '\0';
  result->payload_length = context->event_payload_length;
  return 0;
}

void free_lambda_result(struct lambda_result *result) {
  if (result == NULL) {
    return;
  }

  free(result->payload);
  result->payload = NULL;
  result->payload_length = 0;
}

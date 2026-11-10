#ifndef RUNTIME_H
#define RUNTIME_H

#include "handler.h"
#include <curl/curl.h>
#include <stddef.h>

struct lambda_urls {
  char *next_invocation;
  char *init_error;
  char *function_error_template;
  char *response_template;
};

struct runtime_client {
  CURL *curl;
};

char *build_url(const char *base_url, const char *path);
int init_runtime_urls(struct lambda_urls *urls, const char *runtime_api);
void free_runtime_urls(struct lambda_urls *urls);

int append_event_payload(struct lambda_context *context, const void *data,
                         size_t length);
int parse_runtime_header(struct lambda_context *context, const char *line,
                         size_t length);
void reset_lambda_context(struct lambda_context *context);

int runtime_get_next(struct runtime_client *client,
                     const struct lambda_urls *urls,
                     struct lambda_context *context);
int runtime_send_response(struct runtime_client *client,
                          const struct lambda_urls *urls,
                          const struct lambda_context *context,
                          const struct lambda_result *result);
int runtime_send_function_error(struct runtime_client *client,
                                const struct lambda_urls *urls,
                                const struct lambda_context *context,
                                const char *error_type,
                                const char *error_payload);
int runtime_send_init_error(struct runtime_client *client,
                            const struct lambda_urls *urls,
                            const char *error_type,
                            const char *error_payload);

#endif

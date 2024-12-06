#ifndef HANDLER_H
#define HANDLER_H
#include <curl/curl.h>

#define SAFE_FREE(ptr)                                                         \
  do {                                                                         \
    if ((ptr) != NULL) {                                                       \
      free(ptr);                                                               \
      (ptr) = NULL;                                                            \
    }                                                                          \
  } while (0)

enum interface_methods {
  INIT_ERROR,
  FUNCTION_ERROR,
  INVOKE_NEXT,
  INVOKE_RESPONSE
};

struct lambda_context {
  char *request_id;
  long long deadline;
  char *env_variables;
  char *x_ray_trace_id;
  char *event_payload;
};

struct lambda_urls {
  char *next_invocation;
  char *init_error;
  char *function_error;
  char *response_template;
};

struct runtime_client {
  CURL *curl_client;
  enum interface_methods method;
};

struct lambda_event {
  char *event;
  struct lambda_context *context;
};

int lambda_handler();
char *build_url(const char *base_url, const char *path);
void init_runtime_urls(struct lambda_urls *urls);
void free_runtime_urls(struct lambda_urls *urls);

/* DEBUG */

void print_lambda_context(struct lambda_context *ctx);

#endif // HANDLER_H

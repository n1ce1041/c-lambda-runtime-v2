#define _POSIX_C_SOURCE 200809L

#include "runtime.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define RUNTIME_API_VERSION "2018-06-01"
#define CONNECT_TIMEOUT_MS 1000L
#define POST_TIMEOUT_MS 5000L

static char *duplicate_range(const char *value, size_t length) {
  char *copy;

  if (value == NULL || length == SIZE_MAX) {
    return NULL;
  }

  copy = malloc(length + 1);
  if (copy == NULL) {
    return NULL;
  }

  memcpy(copy, value, length);
  copy[length] = '\0';
  return copy;
}

char *build_url(const char *base_url, const char *path) {
  static const char prefix[] = "http://";
  size_t prefix_length = sizeof(prefix) - 1;
  size_t base_length;
  size_t path_length;
  size_t total_length;
  char *url;

  if (base_url == NULL || path == NULL || base_url[0] == '\0' ||
      path[0] == '\0') {
    return NULL;
  }

  base_length = strlen(base_url);
  path_length = strlen(path);
  if (base_length > SIZE_MAX - prefix_length - 2 ||
      path_length > SIZE_MAX - prefix_length - base_length - 2) {
    return NULL;
  }

  total_length = prefix_length + base_length + 1 + path_length + 1;
  url = malloc(total_length);
  if (url == NULL) {
    return NULL;
  }

  if (snprintf(url, total_length, "%s%s/%s", prefix, base_url, path) < 0) {
    free(url);
    return NULL;
  }

  return url;
}

int init_runtime_urls(struct lambda_urls *urls, const char *runtime_api) {
  if (urls == NULL || runtime_api == NULL || runtime_api[0] == '\0') {
    return -1;
  }

  *urls = (struct lambda_urls){0};
  urls->next_invocation =
      build_url(runtime_api, RUNTIME_API_VERSION "/runtime/invocation/next");
  urls->init_error =
      build_url(runtime_api, RUNTIME_API_VERSION "/runtime/init/error");
  urls->function_error_template = build_url(
      runtime_api, RUNTIME_API_VERSION "/runtime/invocation/%s/error");
  urls->response_template = build_url(
      runtime_api, RUNTIME_API_VERSION "/runtime/invocation/%s/response");

  if (urls->next_invocation == NULL || urls->init_error == NULL ||
      urls->function_error_template == NULL || urls->response_template == NULL) {
    free_runtime_urls(urls);
    return -1;
  }

  return 0;
}

void free_runtime_urls(struct lambda_urls *urls) {
  if (urls == NULL) {
    return;
  }

  free(urls->next_invocation);
  free(urls->init_error);
  free(urls->function_error_template);
  free(urls->response_template);
  *urls = (struct lambda_urls){0};
}

void reset_lambda_context(struct lambda_context *context) {
  if (context == NULL) {
    return;
  }

  free(context->request_id);
  free(context->invoked_function_arn);
  free(context->xray_trace_id);
  free(context->event_payload);
  *context = (struct lambda_context){0};
}

int append_event_payload(struct lambda_context *context, const void *data,
                         size_t length) {
  char *resized;

  if (context == NULL || (data == NULL && length != 0)) {
    return -1;
  }

  if (context->event_payload_length > SIZE_MAX - length - 1) {
    return -1;
  }

  resized = realloc(context->event_payload,
                    context->event_payload_length + length + 1);
  if (resized == NULL) {
    return -1;
  }

  context->event_payload = resized;
  if (length != 0) {
    memcpy(context->event_payload + context->event_payload_length, data, length);
  }
  context->event_payload_length += length;
  context->event_payload[context->event_payload_length] = '\0';
  return 0;
}

static int find_header_value(const char *line, size_t length, const char *name,
                             const char **value, size_t *value_length) {
  size_t name_length = strlen(name);
  size_t start;
  size_t end;

  if (length <= name_length || strncasecmp(line, name, name_length) != 0 ||
      line[name_length] != ':') {
    return 0;
  }

  start = name_length + 1;
  while (start < length && (line[start] == ' ' || line[start] == '\t')) {
    start++;
  }

  end = length;
  while (end > start &&
         (line[end - 1] == '\r' || line[end - 1] == '\n' ||
          line[end - 1] == ' ' || line[end - 1] == '\t')) {
    end--;
  }

  *value = line + start;
  *value_length = end - start;
  return 1;
}

static int assign_string_header(char **destination, const char *line,
                                size_t length, const char *name, int *matched) {
  const char *value;
  size_t value_length;
  char *copy;

  *matched = find_header_value(line, length, name, &value, &value_length);
  if (!*matched) {
    return 0;
  }

  copy = duplicate_range(value, value_length);
  if (copy == NULL) {
    return -1;
  }

  free(*destination);
  *destination = copy;
  return 0;
}

int parse_runtime_header(struct lambda_context *context, const char *line,
                         size_t length) {
  static const char request_id_header[] = "Lambda-Runtime-Aws-Request-Id";
  static const char deadline_header[] = "Lambda-Runtime-Deadline-Ms";
  static const char trace_header[] = "Lambda-Runtime-Trace-Id";
  static const char function_arn_header[] =
      "Lambda-Runtime-Invoked-Function-Arn";
  const char *value;
  size_t value_length;
  char *deadline_text;
  char *end;
  long long deadline;
  int matched;

  if (context == NULL || line == NULL) {
    return -1;
  }

  if (assign_string_header(&context->request_id, line, length,
                           request_id_header, &matched) != 0) {
    return -1;
  }
  if (matched) {
    return 0;
  }

  if (assign_string_header(&context->xray_trace_id, line, length, trace_header,
                           &matched) != 0) {
    return -1;
  }
  if (matched) {
    return 0;
  }

  if (assign_string_header(&context->invoked_function_arn, line, length,
                           function_arn_header, &matched) != 0) {
    return -1;
  }
  if (matched) {
    return 0;
  }

  matched = find_header_value(line, length, deadline_header, &value,
                              &value_length);
  if (!matched) {
    return 0;
  }

  deadline_text = duplicate_range(value, value_length);
  if (deadline_text == NULL) {
    return -1;
  }

  errno = 0;
  end = NULL;
  deadline = strtoll(deadline_text, &end, 10);
  if (errno != 0 || end == deadline_text || *end != '\0' || deadline < 0) {
    free(deadline_text);
    return -1;
  }

  free(deadline_text);
  context->deadline_ms = deadline;
  return 0;
}

static size_t header_callback(char *data, size_t size, size_t count,
                              void *user_data) {
  struct lambda_context *context = user_data;
  size_t length;

  if (count != 0 && size > SIZE_MAX / count) {
    return 0;
  }
  length = size * count;

  if (parse_runtime_header(context, data, length) != 0) {
    return 0;
  }
  return length;
}

static size_t body_callback(char *data, size_t size, size_t count,
                            void *user_data) {
  struct lambda_context *context = user_data;
  size_t length;

  if (count != 0 && size > SIZE_MAX / count) {
    return 0;
  }
  length = size * count;

  if (append_event_payload(context, data, length) != 0) {
    return 0;
  }
  return length;
}

static size_t discard_callback(char *data, size_t size, size_t count,
                               void *user_data) {
  (void)data;
  (void)user_data;
  if (count != 0 && size > SIZE_MAX / count) {
    return 0;
  }
  return size * count;
}

static int configure_common_options(CURL *curl) {
  if (curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L) != CURLE_OK ||
      curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, CONNECT_TIMEOUT_MS) !=
          CURLE_OK ||
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_callback) !=
          CURLE_OK) {
    return -1;
  }
  return 0;
}

int runtime_get_next(struct runtime_client *client,
                     const struct lambda_urls *urls,
                     struct lambda_context *context) {
  CURLcode result;
  long status_code = 0;

  if (client == NULL || client->curl == NULL || urls == NULL ||
      urls->next_invocation == NULL || context == NULL) {
    return -1;
  }

  reset_lambda_context(context);
  curl_easy_reset(client->curl);

  if (configure_common_options(client->curl) != 0 ||
      curl_easy_setopt(client->curl, CURLOPT_URL, urls->next_invocation) !=
          CURLE_OK ||
      curl_easy_setopt(client->curl, CURLOPT_HTTPGET, 1L) != CURLE_OK ||
      curl_easy_setopt(client->curl, CURLOPT_HEADERFUNCTION, header_callback) !=
          CURLE_OK ||
      curl_easy_setopt(client->curl, CURLOPT_HEADERDATA, context) != CURLE_OK ||
      curl_easy_setopt(client->curl, CURLOPT_WRITEFUNCTION, body_callback) !=
          CURLE_OK ||
      curl_easy_setopt(client->curl, CURLOPT_WRITEDATA, context) != CURLE_OK) {
    return -1;
  }

  /* The next-invocation request is a long poll, so it intentionally has no
   * overall request timeout. The connection itself remains bounded. */
  result = curl_easy_perform(client->curl);
  if (result != CURLE_OK) {
    fprintf(stderr, "failed to retrieve invocation: %s\n",
            curl_easy_strerror(result));
    return -1;
  }

  if (curl_easy_getinfo(client->curl, CURLINFO_RESPONSE_CODE, &status_code) !=
          CURLE_OK ||
      status_code != 200 || context->request_id == NULL) {
    fprintf(stderr, "Runtime API returned an invalid invocation response\n");
    return -1;
  }

  if (context->event_payload == NULL &&
      append_event_payload(context, NULL, 0) != 0) {
    return -1;
  }

  if (context->xray_trace_id != NULL) {
    if (setenv("_X_AMZN_TRACE_ID", context->xray_trace_id, 1) != 0) {
      return -1;
    }
  } else if (unsetenv("_X_AMZN_TRACE_ID") != 0) {
    return -1;
  }

  return 0;
}

static char *format_invocation_url(CURL *curl, const char *template,
                                   const char *request_id) {
  char *escaped_request_id;
  char *url;
  int required;
  size_t url_length;

  if (curl == NULL || template == NULL || request_id == NULL) {
    return NULL;
  }

  escaped_request_id = curl_easy_escape(curl, request_id, 0);
  if (escaped_request_id == NULL) {
    return NULL;
  }

  required = snprintf(NULL, 0, template, escaped_request_id);
  if (required < 0) {
    curl_free(escaped_request_id);
    return NULL;
  }

  url_length = (size_t)required + 1;
  url = malloc(url_length);
  if (url != NULL &&
      snprintf(url, url_length, template, escaped_request_id) != required) {
    free(url);
    url = NULL;
  }

  curl_free(escaped_request_id);
  return url;
}

static int append_header(struct curl_slist **headers, const char *value) {
  struct curl_slist *updated = curl_slist_append(*headers, value);
  if (updated == NULL) {
    return -1;
  }
  *headers = updated;
  return 0;
}

static int contains_line_break(const char *value) {
  return value != NULL &&
         (strchr(value, '\r') != NULL || strchr(value, '\n') != NULL);
}

static int post_payload(struct runtime_client *client, const char *url,
                        const char *payload, size_t payload_length,
                        const char *error_type) {
  CURLcode result;
  struct curl_slist *headers = NULL;
  char error_header[256];
  long status_code = 0;
  int outcome = -1;

  if (client == NULL || client->curl == NULL || url == NULL ||
      (payload == NULL && payload_length != 0) ||
      payload_length > (size_t)LLONG_MAX || contains_line_break(error_type)) {
    return -1;
  }

  if (append_header(&headers, "Content-Type: application/json") != 0) {
    return -1;
  }

  if (error_type != NULL) {
    int written = snprintf(error_header, sizeof(error_header),
                           "Lambda-Runtime-Function-Error-Type: %s", error_type);
    if (written < 0 || (size_t)written >= sizeof(error_header) ||
        append_header(&headers, error_header) != 0) {
      curl_slist_free_all(headers);
      return -1;
    }
  }

  curl_easy_reset(client->curl);
  if (configure_common_options(client->curl) != 0 ||
      curl_easy_setopt(client->curl, CURLOPT_URL, url) != CURLE_OK ||
      curl_easy_setopt(client->curl, CURLOPT_POST, 1L) != CURLE_OK ||
      curl_easy_setopt(client->curl, CURLOPT_HTTPHEADER, headers) != CURLE_OK ||
      curl_easy_setopt(client->curl, CURLOPT_POSTFIELDS,
                       payload == NULL ? "" : payload) != CURLE_OK ||
      curl_easy_setopt(client->curl, CURLOPT_POSTFIELDSIZE_LARGE,
                       (curl_off_t)payload_length) != CURLE_OK ||
      curl_easy_setopt(client->curl, CURLOPT_TIMEOUT_MS, POST_TIMEOUT_MS) !=
          CURLE_OK) {
    goto cleanup;
  }

  result = curl_easy_perform(client->curl);
  if (result != CURLE_OK) {
    fprintf(stderr, "failed to publish invocation result: %s\n",
            curl_easy_strerror(result));
    goto cleanup;
  }

  if (curl_easy_getinfo(client->curl, CURLINFO_RESPONSE_CODE, &status_code) !=
          CURLE_OK ||
      status_code < 200 || status_code >= 300) {
    fprintf(stderr, "Runtime API rejected invocation result with HTTP %ld\n",
            status_code);
    goto cleanup;
  }

  outcome = 0;

cleanup:
  curl_slist_free_all(headers);
  return outcome;
}

int runtime_send_response(struct runtime_client *client,
                          const struct lambda_urls *urls,
                          const struct lambda_context *context,
                          const struct lambda_result *result) {
  char *url;
  int outcome;

  if (client == NULL || client->curl == NULL || urls == NULL ||
      context == NULL || result == NULL || context->request_id == NULL ||
      urls->response_template == NULL) {
    return -1;
  }

  url = format_invocation_url(client->curl, urls->response_template,
                              context->request_id);
  if (url == NULL) {
    return -1;
  }

  outcome =
      post_payload(client, url, result->payload, result->payload_length, NULL);
  free(url);
  return outcome;
}

int runtime_send_function_error(struct runtime_client *client,
                                const struct lambda_urls *urls,
                                const struct lambda_context *context,
                                const char *error_type,
                                const char *error_payload) {
  char *url;
  int outcome;

  if (client == NULL || urls == NULL || context == NULL || error_type == NULL ||
      error_payload == NULL || context->request_id == NULL ||
      urls->function_error_template == NULL) {
    return -1;
  }

  url = format_invocation_url(client->curl, urls->function_error_template,
                              context->request_id);
  if (url == NULL) {
    return -1;
  }

  outcome = post_payload(client, url, error_payload, strlen(error_payload),
                         error_type);
  free(url);
  return outcome;
}

int runtime_send_init_error(struct runtime_client *client,
                            const struct lambda_urls *urls,
                            const char *error_type,
                            const char *error_payload) {
  if (client == NULL || urls == NULL || urls->init_error == NULL ||
      error_type == NULL || error_payload == NULL) {
    return -1;
  }

  return post_payload(client, urls->init_error, error_payload,
                      strlen(error_payload), error_type);
}

#ifndef TEST
static volatile sig_atomic_t keep_running = 1;

static void stop_runtime(int signal_number) {
  (void)signal_number;
  keep_running = 0;
}

static int install_signal_handlers(void) {
  struct sigaction action = {0};
  action.sa_handler = stop_runtime;
  sigemptyset(&action.sa_mask);

  if (sigaction(SIGTERM, &action, NULL) != 0 ||
      sigaction(SIGINT, &action, NULL) != 0) {
    return -1;
  }
  return 0;
}

static int parse_max_invocations(int argc, char **argv, size_t *maximum) {
  char *end;
  unsigned long long parsed;

  *maximum = 0;
  if (argc == 1) {
    return 0;
  }
  if (argc != 3 || strcmp(argv[1], "--max-invocations") != 0) {
    return -1;
  }

  errno = 0;
  end = NULL;
  parsed = strtoull(argv[2], &end, 10);
  if (errno != 0 || end == argv[2] || *end != '\0' || parsed == 0 ||
      parsed > SIZE_MAX) {
    return -1;
  }

  *maximum = (size_t)parsed;
  return 0;
}

int main(int argc, char **argv) {
  static const char handler_error[] =
      "{\"errorMessage\":\"handler failed\",\"errorType\":\"Runtime.HandlerError\"}";
  const char *runtime_api = getenv("AWS_LAMBDA_RUNTIME_API");
  struct lambda_urls urls = {0};
  struct lambda_context context = {0};
  struct runtime_client client = {0};
  size_t maximum_invocations;
  size_t completed_invocations = 0;
  int exit_status = EXIT_FAILURE;

  if (parse_max_invocations(argc, argv, &maximum_invocations) != 0) {
    fprintf(stderr, "usage: %s [--max-invocations COUNT]\n", argv[0]);
    return EXIT_FAILURE;
  }

  if (runtime_api == NULL || runtime_api[0] == '\0') {
    fputs("AWS_LAMBDA_RUNTIME_API is not set\n", stderr);
    return EXIT_FAILURE;
  }

  if (init_runtime_urls(&urls, runtime_api) != 0) {
    fputs("failed to initialize Runtime API URLs\n", stderr);
    return EXIT_FAILURE;
  }

  if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
    fputs("failed to initialize libcurl\n", stderr);
    free_runtime_urls(&urls);
    return EXIT_FAILURE;
  }

  client.curl = curl_easy_init();
  if (client.curl == NULL) {
    fputs("failed to create Runtime API client\n", stderr);
    goto cleanup;
  }

  if (install_signal_handlers() != 0) {
    fputs("failed to install signal handlers\n", stderr);
    (void)runtime_send_init_error(
        &client, &urls, "Runtime.InitializationError",
        "{\"errorMessage\":\"failed to install signal handlers\"}");
    goto cleanup;
  }

  while (keep_running &&
         (maximum_invocations == 0 ||
          completed_invocations < maximum_invocations)) {
    struct lambda_result result = {0};
    int handler_status;
    int publish_status;

    if (runtime_get_next(&client, &urls, &context) != 0) {
      if (!keep_running) {
        exit_status = EXIT_SUCCESS;
      }
      goto cleanup;
    }

    handler_status = lambda_handler(&context, &result);
    if (handler_status == 0) {
      publish_status = runtime_send_response(&client, &urls, &context, &result);
    } else {
      publish_status = runtime_send_function_error(
          &client, &urls, &context, "Runtime.HandlerError", handler_error);
    }

    free_lambda_result(&result);
    reset_lambda_context(&context);
    if (publish_status != 0) {
      goto cleanup;
    }

    completed_invocations++;
  }

  exit_status = EXIT_SUCCESS;

cleanup:
  reset_lambda_context(&context);
  if (client.curl != NULL) {
    curl_easy_cleanup(client.curl);
  }
  curl_global_cleanup();
  free_runtime_urls(&urls);
  (void)unsetenv("_X_AMZN_TRACE_ID");
  return exit_status;
}
#endif

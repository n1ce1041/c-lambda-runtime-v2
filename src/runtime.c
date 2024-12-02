#include "handler.h"
#include <ctype.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_RUNTIME_API "localhost:9001"

/* global to store lambda URLs for runtime usage.*/

/*************************/
/******URL BUILDERS*******/
/*************************/

char *build_url(const char *base_url, const char *path) {
  size_t url_length =
      7 + strlen(base_url) + strlen(path) + 2; // "http://" + base + path + '\0'
  char *full_url = malloc(url_length);
  if (!full_url) {
    perror("Failed to allocate memory for URL");
    exit(EXIT_FAILURE);
  }
  snprintf(full_url, url_length, "http://%s/%s", base_url, path);
  return full_url;
}

void init_runtime_urls(struct lambda_urls *urls) {
  /* Build all URLs to be available in our struct */
  const char *runtime_api = getenv("AWS_LAMDA_RUNTIME_API");
  if (!runtime_api)
    runtime_api = DEFAULT_RUNTIME_API;

  urls->next_invocation =
      build_url(runtime_api, "2018-06-01/runtime/invocation/next");
  urls->init_error = build_url(runtime_api, "2018-06-01/runtime/init/error");
  urls->function_error =
      build_url(runtime_api, "2018-06-01/runtime/invocation/error");
  urls->response_template =
      build_url(runtime_api, "2018-06-01/runtime/invocation/%s/response");
}

char *trim_whitespace(char *str) {
  char *end;

  // Trim leading space
  while (isspace((unsigned char)*str))
    str++;

  // If all spaces
  if (*str == 0)
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while (end > str && isspace((unsigned char)*end))
    end--;

  // Null-terminate
  *(end + 1) = 0;

  return str;
}

/*********** END OF URL BUILDERS **************/

/***************************/
/******CURL CALLBACKS*******/
/***************************/

// My own strdup as c11 doesnt define this
char *my_strdup(const char *s) {
  size_t len = strlen(s) + 1;      // +1 for the null terminator
  char *dup = (char *)malloc(len); // Allocate memory for the duplicate
  if (dup) {
    memcpy(dup, s, len); // Copy the string
  }
  return dup; // Return the pointer to the newly allocated string
}
// Header callback function to print headers
static size_t header_callback(void *ptr, size_t size, size_t nmemb,
                              void *userdata) {
  size_t total_size = size * nmemb;

  // The ptr is already pointing to the received data
  // Assuming userdata is a pointer to a struct lambda_context
  struct lambda_context *context = (struct lambda_context *)userdata;

  // Parse the header and match the key
  if (strncmp(ptr, "Lambda-Runtime-Aws-Request-Id:", 29) == 0) {
    // Extract and trim the value
    context->request_id =
        my_strdup(trim_whitespace(ptr + 29)); // Store the request ID
    if (context->request_id) {
      printf("Request ID: %s\n", context->request_id);
    }
  } else if (strncmp(ptr, "Lambda-Runtime-Trace-Id:", 24) == 0) {
    context->x_ray_trace_id =
        my_strdup(trim_whitespace(ptr + 24)); // Store the trace ID
    if (context->x_ray_trace_id) {
      printf("X-Ray Trace ID: %s\n", context->x_ray_trace_id);
    }
  } else if (strncmp(ptr, "Lambda-Runtime-Deadline-Ms:", 27) == 0) {
    // Convert the deadline value to a long long (int64)
    context->deadline = strtoll(
        ptr + 28, NULL, 10); // Assuming 'deadline' is a long long or int64
    printf("Deadline: %lld\n", context->deadline);
  }

  return total_size; // Return the number of bytes processed
}
// Write callback function to print the body data
static size_t write_callback(void *ptr, size_t size, size_t nmemb,
                             void *userdata) {
  size_t total_size = size * nmemb;
  printf("Data: %.*s\n", (int)total_size, (char *)ptr);
  return total_size; // Return the number of bytes processed
}
/********************************************************* */
/********************************************************* */
/***************CURL CLIENT WRAPPA************************ */
/********************************************************* */

int invoke_next(struct runtime_client *client, struct lambda_urls *urls) {

  curl_easy_reset(client->curl_client);

  return 0;
}
int invoke_response(struct runtime_client *client, struct lambda_urls *urls) {
  curl_easy_reset(client->curl_client);

  return 0;
}
int function_error(struct runtime_client *client, struct lambda_urls *urls) {
  curl_easy_reset(client->curl_client);

  return 0;
}
int init_error(struct runtime_client *client, struct lambda_urls *urls) {
  curl_easy_reset(client->curl_client);

  return 0;
}

int request_client(struct runtime_client *client, struct lambda_urls *urls) {

  curl_easy_reset(client->curl_client);

  switch (client->method) {
  case INVOKE_NEXT:
    invoke_next(client, urls);
    break;
  case INVOKE_RESPONSE:
    invoke_response(client, urls);
    break;
  case FUNCTION_ERROR:
    function_error(client, urls);
    break;
  case INIT_ERROR:
    init_error(client, urls);
    break;
  default:
    printf("This is the output");
    break;
  }

  /* Function 1: Invoke next */
  /* Function 2: Invoke response */
  /* Function 3: Init error */
  /* Function 4: Function Error*/

  return 0;
}

/********************************************************* */
/********************************************************* */
/***************CLEANUP FUNCTIONS************************* */
/********************************************************* */
/********************************************************* */

void free_lambda_context(struct lambda_context *context) {
  free(context->request_id);
  free(context->env_variables);
  free(context->x_ray_trace_id);
}

void free_runtime_urls(struct lambda_urls *urls) {
  free(urls->next_invocation);
  free(urls->init_error);
  free(urls->function_error);
  free(urls->response_template);
}

#ifndef TEST
int main() {
  printf("Lambda Runtime Starting...\n");

  struct runtime_client client;
  struct lambda_urls runtime_urls;
  struct lambda_context context;
  init_runtime_urls(&runtime_urls);

  /* Setup , move out afterwards */
  curl_global_init(CURL_GLOBAL_DEFAULT);

  client.curl_client = curl_easy_init();
  if (client.curl_client == NULL) {
    /* OOPS */
    perror("Curl client failed to initialise");
  }

  curl_easy_setopt(client.curl_client, CURLOPT_URL,
                   runtime_urls.next_invocation);
  curl_easy_setopt(client.curl_client, CURLOPT_HEADERFUNCTION, header_callback);
  curl_easy_setopt(client.curl_client, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(client.curl_client, CURLOPT_WRITEDATA, &context);
  curl_easy_setopt(client.curl_client, CURLOPT_HEADERDATA, &context);

  CURLcode res = curl_easy_perform(client.curl_client);
  printf("Result code %d \n", res);
  printf("Request ID: %s\nDeadline: %lld\nX-Ray Trace ID: %s\n",
         context.request_id, context.deadline, context.x_ray_trace_id);

  /* Cleanup (move to another function)*/
  curl_global_cleanup();
  free_runtime_urls(&runtime_urls);
  free_lambda_context(&context);

  printf("Lambda Runtime Finished\n");
  return 0;
}
#endif

#ifdef TEST
/* Exclude when linking for test */
int prog_main() {
  printf("TESTING is ");
  printf("Lambda Runtime Starting...\n");

  // Initialize and run the Lambda function handler
  if (lambda_handler() != 0) {
    fprintf(stderr, "Handler execution failed\n");
    return 1;
  }

  printf("Lambda Runtime Finished\n");
  return 0;
}

#endif

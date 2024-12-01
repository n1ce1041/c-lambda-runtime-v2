#include "handler.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_RUNTIME_API "localhost:9001"

/* global to store lambda URLs for runtime usage.*/
struct lambda_urls {
  char *next_invocation;
  char *init_error;
  char *function_error;
  char *response_template;
} runtime_urls;

struct lambda_urls runtime_urls;

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

void init_runtime_urls(void) {
  /* Build all URLs to be available in our struct */
  const char *runtime_api = getenv("AWS_LAMDA_RUNTIME_API");
  if (!runtime_api)
    runtime_api = DEFAULT_RUNTIME_API;

  runtime_urls.next_invocation =
      build_url(runtime_api, "2018-06-01/runtime/invocation/next");
  runtime_urls.init_error =
      build_url(runtime_api, "2018-06-01/runtime/init/error");
  runtime_urls.function_error =
      build_url(runtime_api, "2018-06-01/runtime/invocation/error");
  runtime_urls.response_template =
      build_url(runtime_api, "2018-06-01/runtime/invocation/%s/response");
}

void free_runtime_urls(void) {
  free(runtime_urls.next_invocation);
  free(runtime_urls.init_error);
  free(runtime_urls.function_error);
  free(runtime_urls.response_template);
}

// Regular main function for the normal program
#ifndef TEST
int main() {
  printf("Lambda Runtime Starting...\n");

  /* Setup , move out afterwards */
  curl_global_init(CURL_GLOBAL_DEFAULT);
  CURL *curl_client = curl_easy_init();
  init_runtime_urls();

  if (curl_client) {
    CURLcode res;
    curl_easy_setopt(curl_client, CURLOPT_URL, runtime_urls.next_invocation);
    res = curl_easy_perform(curl_client);
    printf("Result code %d \n", res);
  }

  /* Cleanup (move to another function)*/
  curl_global_cleanup();
  free_runtime_urls();

  printf("Lambda Runtime Finished\n");
  return 0;
}
#endif

#ifdef TEST
/* This is basically just a copy of main with a different name for testing to
 * allow the test runner to have the main function*/
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

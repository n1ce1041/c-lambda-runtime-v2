#define _POSIX_C_SOURCE                                                        \
  200809L // For POSIX.1-2008 (you can also try other versions like 199309L)

#include "handler.h"
#include "stdlib.h"
#include "string.h"
#include "testFunctions.h"
#include "unity.h"

void test_build_url(void) {
  const char *base_url = "localhost:8080";
  const char *path = "2018-06-01/runtime/invocation/next";

  char *url = build_url(base_url, path);

  TEST_ASSERT_NOT_NULL(url);
  TEST_ASSERT_EQUAL_STRING(
      "http://localhost:8080/2018-06-01/runtime/invocation/next", url);

  free(url); // Don't forget to free the allocated memory
}

void test_init_runtime_urls(void) {
  struct lambda_urls urls;

  // Mock the environment variable
  setenv("AWS_LAMDA_RUNTIME_API", "mocked_runtime_api", 1);

  // Initialize the URLs
  init_runtime_urls(&urls);

  // Assert expected values for each URL
  TEST_ASSERT_EQUAL_STRING(
      "http://mocked_runtime_api/2018-06-01/runtime/invocation/next",
      urls.next_invocation);
  TEST_ASSERT_EQUAL_STRING(
      "http://mocked_runtime_api/2018-06-01/runtime/init/error",
      urls.init_error);
  TEST_ASSERT_EQUAL_STRING(
      "http://mocked_runtime_api/2018-06-01/runtime/invocation/error",
      urls.function_error);
  TEST_ASSERT_EQUAL_STRING(
      "http://mocked_runtime_api/2018-06-01/runtime/invocation/%s/response",
      urls.response_template);

  // Free the allocated memory
  free_runtime_urls(&urls);
}

// Test the free_runtime_urls function
void test_free_runtime_urls(void) {
  struct lambda_urls urls;

  // Allocate mock URLs
  urls.next_invocation =
      strdup("http://mocked_runtime_api/2018-06-01/runtime/invocation/next");
  urls.init_error =
      strdup("http://mocked_runtime_api/2018-06-01/runtime/init/error");
  urls.function_error =
      strdup("http://mocked_runtime_api/2018-06-01/runtime/invocation/error");
  urls.response_template = strdup(
      "http://mocked_runtime_api/2018-06-01/runtime/invocation/%s/response");

  // Ensure the strings are allocated properly
  TEST_ASSERT_NOT_NULL(urls.next_invocation);
  TEST_ASSERT_NOT_NULL(urls.init_error);
  TEST_ASSERT_NOT_NULL(urls.function_error);
  TEST_ASSERT_NOT_NULL(urls.response_template);

  // Free the allocated memory
  free_runtime_urls(&urls);

  // Verify that the pointers are NULL after freeing
  // (Assuming the function nullifies pointers; otherwise, this check may not be
  // applicable)
  TEST_ASSERT_NULL(urls.next_invocation);
  TEST_ASSERT_NULL(urls.init_error);
  TEST_ASSERT_NULL(urls.function_error);
  TEST_ASSERT_NULL(urls.response_template);
}

void run_runtime_url_tests(void) {
  RUN_TEST(test_build_url);
  RUN_TEST(test_init_runtime_urls);
  // Add more RUN_TEST calls here as you add more test functions
}
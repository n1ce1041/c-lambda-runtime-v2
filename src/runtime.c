#include "handler.h"
#include <curl/curl.h>
#include <stdio.h>

#ifndef TESTING
// Regular main function for the normal program
int main() {
  printf("Lambda Runtime Starting...\n");

  // Initialize and run the Lambda function handler
  if (lambda_handler() != 0) {
    fprintf(stderr, "Handler execution failed\n");
    return 1;
  }

  printf("Lambda Runtime Finished\n");
  return 0;
}
#else
// Test main function for the test runner
int test_main() {
  printf("Running Lambda Runtime Tests...\n");

  // Initialize and run the Lambda function handler
  if (lambda_handler() != 0) {
    fprintf(stderr, "Handler execution failed\n");
    return 1;
  }

  printf("Lambda Runtime Tests Finished\n");
  return 0;
}
#endif
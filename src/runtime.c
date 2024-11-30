#include "handler.h"
#include <curl/curl.h>
#include <stdio.h>

// Regular main function for the normal program
#ifndef TEST
int main() {
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

#include "handler.h"
#include <curl/curl.h>
#include <stdio.h>

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
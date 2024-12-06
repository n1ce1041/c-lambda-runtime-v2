#include "handler.h"
#include <stdio.h>

int lambda_handler(struct lambda_context *context) {
  // Example Lambda function logic
  printf("Executing Lambda Handler...\n");
  printf("#######################################");
  print_lambda_context(context);
  printf("#######################################");

  // Simulate some processing
  return 0; // Return 0 for success
}

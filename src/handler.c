#include "handler.h"
#include <stdio.h>

int lambda_handler(struct lambda_event *payload) {
  // Example Lambda function logic
  printf("Executing Lambda Handler...\n");

  // Simulate some processing
  return 0; // Return 0 for success
}
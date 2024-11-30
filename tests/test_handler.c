#include "handler.h"
#include "unity.h"

void test_lambda_handler_should_return_zero_when_success(void) {
  int result = lambda_handler();
  TEST_ASSERT_EQUAL(0, result); // Check that lambda_handler returns 0 (success)
}

void run_handler_tests(void) {
  RUN_TEST(test_lambda_handler_should_return_zero_when_success);
  // Add more RUN_TEST calls here as you add more test functions
}

// Remove the main function from here
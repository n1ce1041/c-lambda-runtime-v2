#include "testFunctions.h"
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

int main(void) {
  UNITY_BEGIN();

  // Run all test suites
  run_handler_tests();
  run_runtime_url_tests();

  return UNITY_END();
}
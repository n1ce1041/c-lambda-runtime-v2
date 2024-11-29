#include "testFunctions.h"
#include "unity/unity.h"

void setUp(void) {}
void tearDown(void) {}

int main(void) {
  UNITY_BEGIN();

  // Run all test suites
  run_handler_tests();

  return UNITY_END();
}
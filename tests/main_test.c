/**
 * @file main_test.c
 * @brief Main Entry Point for Host Test Suite.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Aggregates and executes all registered test suites.
 */

#include "test_framework.h"

// Test Suites
void run_vna_sanity_tests(void);
void run_math_tests(void);
void run_fast_math_tests(void); // New
void run_fft_tests(void);
void run_event_tests(void);
void run_vna_pipeline_tests(void);

int main(void) {
  printf("======================================\n");
  printf("   MeasLib Host Test Suite\n");
  printf("======================================\n");

  run_vna_sanity_tests();
  run_math_tests();
  run_fast_math_tests(); // New
  run_fft_tests();
  run_event_tests();
  run_vna_pipeline_tests();

  printf("\nAll Tests Passed Successfully.\n");
  return 0;
}

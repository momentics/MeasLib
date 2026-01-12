/**
 * @file test_framework.h
 * @brief Minimal Unit Testing Framework.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 *
 * Provides basic assertion macros (TEST_ASSERT, TEST_ASSERT_EQUAL) and
 * a test runner helper. Designed to have zero dependencies for easy
 * portability.
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include "measlib/types.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_ASSERT(cond)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      printf("FAIL: %s:%d: Assertion '%s' failed.\n", __FILE__, __LINE__,      \
             #cond);                                                           \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_EQUAL(a, b)                                                \
  do {                                                                         \
    if ((a) != (b)) {                                                          \
      printf("FAIL: %s:%d: Expected %d, got %d.\n", __FILE__, __LINE__,        \
             (int)(a), (int)(b));                                              \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual)                      \
  do {                                                                         \
    double diff = (double)(expected) - (double)(actual);                       \
    if (diff < 0)                                                              \
      diff = -diff;                                                            \
    if (diff > (delta)) {                                                      \
      printf("FAIL: %s:%d: Expected %.6f, got %.6f (delta %.6f > tol %.6f)\n", \
             __FILE__, __LINE__, (double)(expected), (double)(actual),         \
             (double)(diff), (double)(delta));                                 \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define RUN_TEST(func)                                                         \
  do {                                                                         \
    printf("Running %s... ", #func);                                           \
    func();                                                                    \
    printf("PASS\n");                                                          \
  } while (0)

#define TEST_ASSERT_EQUAL_STRING(expected, actual)                             \
  do {                                                                         \
    if (strcmp((expected), (actual)) != 0) {                                   \
      printf("FAIL: %s:%d: Expected '%s', got '%s'.\n", __FILE__, __LINE__,    \
             (expected), (actual));                                            \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

#define TEST_FAIL_MESSAGE(msg)                                                 \
  do {                                                                         \
    printf("FAIL: %s:%d: %s\n", __FILE__, __LINE__, msg);                      \
    exit(1);                                                                   \
  } while (0)

#endif // TEST_FRAMEWORK_H

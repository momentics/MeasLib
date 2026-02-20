/**
 * @file test_scpi.c
 * @brief SCPI Service API Tests.
 *
 * @author Architected by momentics <momentics@gmail.com>
 * @copyright (c) 2026 momentics
 */

#include "measlib/sys/scpi/scpi_core.h"
#include "test_framework.h"
#include <stdio.h>
#include <string.h>

static scpi_context_t ctx;
static char line_buf[128];

static char output_buf[256];
static size_t output_pos = 0;

static size_t mock_write(scpi_context_t *ctx, const char *data, size_t len) {
  (void)ctx;
  if (output_pos + len < sizeof(output_buf)) {
    memcpy(output_buf + output_pos, data, len);
    output_pos += len;
    output_buf[output_pos] = '\0';
    return len;
  }
  return 0;
}

// Extern declaration for command tree initialization
void scpi_def_init(void);

void test_scpi_idn(void) {
  memset(&ctx, 0, sizeof(ctx));
  memset(line_buf, 0, sizeof(line_buf));
  output_pos = 0;
  output_buf[0] = '\0';

  scpi_init(&ctx, line_buf, sizeof(line_buf), NULL, mock_write);
  scpi_def_init();

  const char *input = "*IDN?\n";
  scpi_status_t res = scpi_process(&ctx, input, strlen(input));
  TEST_ASSERT_EQUAL(SCPI_RES_OK, res);

  // Verify output
  TEST_ASSERT_EQUAL_STRING("MOMENTICS,MeasLib,0,0.1\r\n", output_buf);
}

void test_scpi_rst(void) {
  memset(&ctx, 0, sizeof(ctx));
  memset(line_buf, 0, sizeof(line_buf));
  scpi_init(&ctx, line_buf, sizeof(line_buf), NULL, NULL);
  scpi_def_init();

  const char *input = "*RST\r\n";
  scpi_status_t res = scpi_process(&ctx, input, strlen(input));
  TEST_ASSERT_EQUAL(SCPI_RES_OK, res);
}

void test_scpi_unknown(void) {
  memset(&ctx, 0, sizeof(ctx));
  memset(line_buf, 0, sizeof(line_buf));
  scpi_init(&ctx, line_buf, sizeof(line_buf), NULL, NULL);
  scpi_def_init();

  const char *input = "UNKNOWN:CMD\n";
  scpi_status_t res = scpi_process(&ctx, input, strlen(input));
  TEST_ASSERT_EQUAL(SCPI_RES_ERR_INVALID_HEADER, res);
}

void test_scpi_fragmented(void) {
  memset(&ctx, 0, sizeof(ctx));
  memset(line_buf, 0, sizeof(line_buf));
  scpi_init(&ctx, line_buf, sizeof(line_buf), NULL, NULL);
  scpi_def_init();

  const char *frag1 = "*I";
  const char *frag2 = "DN?\n";

  // This test mimics receiving split packets
  // scpi_process handles appending to internal buffer until newline
  // Note: we need to persist context between calls

  scpi_process(&ctx, frag1, strlen(frag1));
  scpi_status_t res = scpi_process(&ctx, frag2, strlen(frag2));

  TEST_ASSERT_EQUAL(SCPI_RES_OK, res);
}

void test_scpi_case_insensitive(void) {
  memset(&ctx, 0, sizeof(ctx));
  memset(line_buf, 0, sizeof(line_buf));
  scpi_init(&ctx, line_buf, sizeof(line_buf), NULL, NULL);
  scpi_def_init();

  const char *input = "*idn?\n";
  scpi_status_t res = scpi_process(&ctx, input, strlen(input));
  TEST_ASSERT_EQUAL(SCPI_RES_OK, res);
}

static scpi_status_t test_cmd_param(scpi_context_t *ctx) {
  int32_t val_i;
  float val_f;
  scpi_status_t res;

  res = scpi_param_int(ctx, &val_i);
  TEST_ASSERT_EQUAL(SCPI_RES_OK, res);
  TEST_ASSERT_EQUAL(42, val_i);

  res = scpi_param_float(ctx, &val_f);
  TEST_ASSERT_EQUAL(SCPI_RES_OK, res);
  // Simple check
  if (val_f < 3.14 || val_f > 3.15) {
    printf("Float fail: %f\n", val_f);
    exit(1);
  }

  return SCPI_RES_OK;
}

void test_scpi_params(void) {
  memset(&ctx, 0, sizeof(ctx));
  memset(line_buf, 0, sizeof(line_buf));
  scpi_init(&ctx, line_buf, sizeof(line_buf), NULL, NULL);

  // Register a temporary tree for this test
  static const scpi_command_t param_cmds[] = {
      {.pattern = "TEST", .callback = test_cmd_param, .children = NULL},
      SCPI_CMD_LIST_END};
  scpi_register_tree(param_cmds);

  const char *input = "TEST 42, 3.14159\n";
  scpi_status_t res = scpi_process(&ctx, input, strlen(input));
  TEST_ASSERT_EQUAL(SCPI_RES_OK, res);
}

void run_scpi_tests(void) {
  printf("--- Running SCPI Tests ---\n");
  RUN_TEST(test_scpi_idn);
  RUN_TEST(test_scpi_rst);
  RUN_TEST(test_scpi_unknown);
  RUN_TEST(test_scpi_case_insensitive);
  RUN_TEST(test_scpi_fragmented);
  RUN_TEST(test_scpi_params);
}

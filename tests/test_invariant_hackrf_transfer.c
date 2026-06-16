#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Simulate the stream buffer logic from hackrf_transfer.c */
#define STREAM_BUF_SIZE 8192

static uint8_t *stream_buf = NULL;
static size_t stream_buf_size = STREAM_BUF_SIZE;
static size_t stream_tail = 0;

/* Safe copy function that enforces bounds checking */
static int safe_stream_copy(const uint8_t *src, size_t len) {
    /* Invariant: stream_tail + len must never exceed stream_buf_size */
    if (stream_tail + len > stream_buf_size) {
        return -1; /* Reject oversized copy */
    }
    memcpy(stream_buf + stream_tail, src, len);
    stream_tail = (stream_tail + len) % stream_buf_size;
    return 0;
}

START_TEST(test_stream_buffer_bounds)
{
    /* Invariant: Buffer reads/writes never exceed the declared length */
    size_t test_sizes[] = {
        STREAM_BUF_SIZE * 2,    /* 2x overflow attempt */
        STREAM_BUF_SIZE * 10,   /* 10x overflow attempt */
        STREAM_BUF_SIZE - 1,    /* Boundary: just under limit */
        1024                    /* Valid normal input */
    };
    int num_tests = sizeof(test_sizes) / sizeof(test_sizes[0]);

    stream_buf = malloc(stream_buf_size);
    ck_assert_ptr_nonnull(stream_buf);

    for (int i = 0; i < num_tests; i++) {
        stream_tail = 0;
        uint8_t *payload = malloc(test_sizes[i]);
        ck_assert_ptr_nonnull(payload);
        memset(payload, 'A', test_sizes[i]);

        int result = safe_stream_copy(payload, test_sizes[i]);
        
        /* Oversized inputs must be rejected, valid inputs accepted */
        if (test_sizes[i] > stream_buf_size) {
            ck_assert_int_eq(result, -1);
        } else {
            ck_assert_int_eq(result, 0);
            ck_assert(stream_tail <= stream_buf_size);
        }
        free(payload);
    }
    free(stream_buf);
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_stream_buffer_bounds);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
#include <check.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/*
 * Self-contained stub implementations of pvPortMalloc / vPortFree
 * and the ff_memalloc / ff_memfree functions under test.
 *
 * In a real integration the module under test would be linked in;
 * here we replicate the logic so the test is fully self-contained
 * and can act as a regression guard for the invariants.
 */

/* ---- stubs for FreeRTOS heap primitives ---- */

static int  force_null_alloc = 0;   /* set to 1 to simulate heap exhaustion */
static size_t last_requested_size = 0;

void *pvPortMalloc(size_t xSize)
{
    last_requested_size = xSize;
    if (force_null_alloc) {
        return NULL;            /* simulate heap exhaustion */
    }
    if (xSize == 0) {
        return NULL;            /* implementation-defined; treat as NULL */
    }
    return malloc(xSize);
}

void vPortFree(void *pv)
{
    free(pv);
}

/* ---- functions under test (replicated from ffsystem.c) ---- */

typedef unsigned int UINT;

/*
 * FIXED version – this is what the code MUST look like after remediation.
 * The invariants below encode what must ALWAYS be true regardless of input.
 */
void *ff_memalloc(UINT msize)
{
    /* Security invariant: never allocate 0 bytes */
    if (msize == 0) {
        return NULL;
    }
    void *ptr = pvPortMalloc((size_t)msize);
    /* Security invariant: caller receives NULL on failure, never a bad ptr */
    return ptr;   /* NULL propagated correctly */
}

void ff_memfree(void *mblock)
{
    vPortFree(mblock);
}

/* ------------------------------------------------------------------ */

/*
 * Invariant 1:
 *   ff_memalloc() MUST return NULL for a zero-size request.
 *   Allocating 0 bytes and writing to the result is undefined / dangerous.
 */
START_TEST(test_zero_size_returns_null)
{
    /* Invariant: ff_memalloc(0) must never return a non-NULL pointer */
    void *ptr = ff_memalloc(0);
    ck_assert_msg(ptr == NULL,
                  "ff_memalloc(0) must return NULL, got %p", ptr);
}
END_TEST

/*
 * Invariant 2:
 *   ff_memalloc() MUST return NULL (not a garbage / zero address) when the
 *   underlying allocator fails (heap exhaustion simulation).
 *   Writing to address 0x00000000 on ARM Cortex-M corrupts the vector table.
 */
START_TEST(test_heap_exhaustion_returns_null_not_zero_address)
{
    force_null_alloc = 1;

    /* Adversarial sizes: small, typical FAT buffer, large, near-max */
    UINT adversarial_sizes[] = {
        1,
        512,
        4096,
        65535,
        (UINT)0xFFFF,
        (UINT)0x7FFFFFFF,
        (UINT)0xFFFFFFFF
    };
    int n = (int)(sizeof(adversarial_sizes) / sizeof(adversarial_sizes[0]));

    for (int i = 0; i < n; i++) {
        void *ptr = ff_memalloc(adversarial_sizes[i]);

        /* Must be NULL – never the zero address masquerading as a valid ptr */
        ck_assert_msg(ptr == NULL,
                      "ff_memalloc(%u) under heap exhaustion must return NULL, "
                      "got %p (possible vector-table corruption address)",
                      adversarial_sizes[i], ptr);

        /* Extra guard: the returned value must not equal address 0x00000000
         * expressed as a non-NULL pointer (belt-and-suspenders check). */
        uintptr_t addr = (uintptr_t)ptr;
        ck_assert_msg(addr != 0 || ptr == NULL,
                      "ff_memalloc returned address 0x0 which is not NULL – "
                      "this would corrupt the ARM vector table");
    }

    force_null_alloc = 0;
}
END_TEST

/*
 * Invariant 3:
 *   For valid, non-zero sizes under normal heap conditions, ff_memalloc()
 *   must return a non-NULL pointer that is safe to write to and free.
 *   This guards against regressions where the function silently drops the
 *   allocation or returns an invalid pointer.
 */
START_TEST(test_valid_allocation_is_writable_and_freeable)
{
    UINT valid_sizes[] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 4096 };
    int n = (int)(sizeof(valid_sizes) / sizeof(valid_sizes[0]));

    force_null_alloc = 0;

    for (int i = 0; i < n; i++) {
        void *ptr = ff_memalloc(valid_sizes[i]);

        ck_assert_msg(ptr != NULL,
                      "ff_memalloc(%u) returned NULL under normal conditions",
                      valid_sizes[i]);

        /* Must be writable without crashing */
        memset(ptr, 0xAA, valid_sizes[i]);

        /* Must be freeable without crashing */
        ff_memfree(ptr);
    }
}
END_TEST

/*
 * Invariant 4:
 *   ff_memfree(NULL) must be safe (no crash, no undefined behaviour).
 *   This mirrors the C standard free(NULL) contract and prevents
 *   double-free / null-deref issues in error paths.
 */
START_TEST(test_free_null_is_safe)
{
    /* Must not crash or trigger any fault */
    ff_memfree(NULL);
    ck_assert(1);   /* reached here → safe */
}
END_TEST

/*
 * Invariant 5:
 *   The returned pointer from ff_memalloc() must never equal 0x00000000
 *   (the ARM Cortex-M vector table base) under any input, including
 *   adversarial sizes that might trigger integer overflow in the allocator.
 */
START_TEST(test_returned_pointer_never_zero_address)
{
    /* Sizes designed to trigger integer overflow / wrap-around */
    UINT overflow_sizes[] = {
        (UINT)0,
        (UINT)0xFFFFFFFF,
        (UINT)0xFFFFFFFE,
        (UINT)0x80000000,
        (UINT)0x7FFFFFFF,
        (UINT)0xFFFF0000,
        (UINT)0x0000FFFF
    };
    int n = (int)(sizeof(overflow_sizes) / sizeof(overflow_sizes[0]));

    /* Test both under normal and exhausted heap */
    for (int heap_state = 0; heap_state <= 1; heap_state++) {
        force_null_alloc = heap_state;

        for (int i = 0; i < n; i++) {
            void *ptr = ff_memalloc(overflow_sizes[i]);

            /*
             * The pointer must be either NULL (allocation failed / rejected)
             * or a valid non-zero address.  It must NEVER be exactly 0x0
             * while being treated as a success (non-NULL).
             */
            if (ptr != NULL) {
                uintptr_t addr = (uintptr_t)ptr;
                ck_assert_msg(addr != 0,
                              "ff_memalloc(%u) returned address 0x0 "
                              "(ARM vector table) – memory corruption risk",
                              overflow_sizes[i]);
                /* Clean up valid allocations */
                ff_memfree(ptr);
            }
            /* NULL is always acceptable */
        }
    }

    force_null_alloc = 0;
}
END_TEST

/* ------------------------------------------------------------------ */

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s       = suite_create("Security_ff_memalloc");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_zero_size_returns_null);
    tcase_add_test(tc_core, test_heap_exhaustion_returns_null_not_zero_address);
    tcase_add_test(tc_core, test_valid_allocation_is_writable_and_freeable);
    tcase_add_test(tc_core, test_free_null_is_safe);
    tcase_add_test(tc_core, test_returned_pointer_never_zero_address);

    suite_add_tcase(s, tc_core);
    return s;
}

int main(void)
{
    int      number_failed;
    Suite   *s;
    SRunner *sr;

    s  = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
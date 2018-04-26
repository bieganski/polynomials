#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>
#include <string.h>
#include <stdlib.h>
#include "poly.h"
#include "cmocka.h"

static jmp_buf jmp_at_exit;
static int exit_status;

extern int calculator_main();


int mock_main() {
    if (!setjmp(jmp_at_exit))
        return calculator_main();
    return exit_status;
}


void mock_exit(int status) {
    exit_status = status;
    longjmp(jmp_at_exit, 1);
}

int mock_fprintf(FILE* const file, const char *format, ...) CMOCKA_PRINTF_ATTRIBUTE(2, 3);
int mock_printf(const char *format, ...) CMOCKA_PRINTF_ATTRIBUTE(1, 2);

/**
 * Buffers that printf/fprintf mocks write to.
 */
static char fprintf_buffer[256];
static char printf_buffer[256];
static int fprintf_position = 0;
static int printf_position = 0;


int mock_fprintf(FILE* const file, const char *format, ...) {
    int return_value;
    va_list args;

    assert_true(file == stderr);
    assert_true((size_t)fprintf_position < sizeof(fprintf_buffer));

    va_start(args, format);
    return_value = vsnprintf(fprintf_buffer + fprintf_position,
                             sizeof(fprintf_buffer) - fprintf_position,
                             format,
                             args);
    va_end(args);

    fprintf_position += return_value;
    assert_true((size_t)fprintf_position < sizeof(fprintf_buffer));
    return return_value;
}


int mock_printf(const char *format, ...) {
    int return_value;
    va_list args;

    assert_true((size_t)printf_position < sizeof(printf_buffer));

    va_start(args, format);
    return_value = vsnprintf(printf_buffer + printf_position,
                             sizeof(printf_buffer) - printf_position,
                             format,
                             args);
    va_end(args);

    printf_position += return_value;
    assert_true((size_t)printf_position < sizeof(printf_buffer));
    return return_value;
}


/**
 * Buffers for functions using stdin.
 */
static char input_stream_buffer[256];
static int input_stream_position = 0;
static int input_stream_end = 0;
int read_char_count;


int mock_scanf(const char *format, ...) {
    va_list fmt_args;
    int ret;

    va_start(fmt_args, format);
    ret = vsscanf(input_stream_buffer + input_stream_position, format, fmt_args);
    va_end(fmt_args);

    if (ret < 0) { /* ret == EOF */
        input_stream_position = input_stream_end;
    }
    else {
        assert_true(read_char_count >= 0);
        input_stream_position += read_char_count;
        if (input_stream_position > input_stream_end) {
            input_stream_position = input_stream_end;
        }
    }
    return ret;
}


int mock_getchar() {
    if (input_stream_position < input_stream_end)
        return input_stream_buffer[input_stream_position++];
    else
        return EOF;
}


int mock_ungetc(int c, FILE *stream) {
    assert_true(stream == stdin);
    if (input_stream_position > 0)
        return input_stream_buffer[--input_stream_position] = c;
    else
        return EOF;
}


/**
 * Function called before tests from second group.
 */
static int test_setup(void **state) {
    (void) state;

    memset(fprintf_buffer, 0, sizeof(fprintf_buffer));
    memset(printf_buffer, 0, sizeof(printf_buffer));
    printf_position = 0;
    fprintf_position = 0;

    /* 0 means success */
    return 0;
}


static void init_input_stream(const char *str) {
    memset(input_stream_buffer, 0, sizeof(input_stream_buffer));
    input_stream_position = 0;
    input_stream_end = strlen(str);
    assert_true((size_t)input_stream_end < sizeof(input_stream_buffer));
    strcpy(input_stream_buffer, str);
}

/**
 * 'p' - zero polynomial, 'count' = 0
 */
static void test_polyzero_countzero(void **state) {
    (void) state;
    Poly p = PolyZero();
    Poly test = PolyZero();
    Poly res = PolyCompose(&p, 0, NULL);
    assert_true(PolyIsEq(&res, &test));
}

/**
 * 'p' - zero polynomial, 'count' = 1, x[0] - const polynomial.
 */
static void test_polyzero_countone_polyconst(void **state) {
    (void) state;
    Poly p = PolyZero();
    Poly arr[1];
    polyTab[0] = PolyFromCoeff(3);
    Poly test = PolyZero();
    Poly res = PolyCompose(&p, 1, arr);
    assert_true(PolyIsEq(&res, &test));
}

/**
 * 'p' - const polynomial, 'count' = 0.
 */
static void test_polyconst_countzero(void **state) {
    (void) state;
    Poly p = PolyFromCoeff(3);
    Poly arr[1];
    Poly test = PolyFromCoeff(3);
    Poly res = PolyCompose(&p, 0, arr);
    assert_true(PolyIsEq(&res, &test));
}

/**
 * 'p' - const polynomial, 'count' = 1, x[0] - const polynomial =/= 'p'.
 */
static void test_polyconst_countone_polyconst(void **state) {
    (void) state;
    Poly p = PolyFromCoeff(5);
    Poly arr[1];
    arr[0] = PolyFromCoeff(3);
    Poly test = PolyFromCoeff(5);
    Poly res = PolyCompose(&p, 1, arr);
    assert_true(PolyIsEq(&res, &test));
}

/**
 * 'p' - polynomial 1*x_0, 'count' = 0.
 */
static void test_polyvarzero_countzero(void **state) {
    (void) state;
    Poly p1 = PolyFromCoeff(5);
    Mono m1 = MonoFromPoly(&p1, 7);
    Poly p = PolyAddMonos(1, &m1);
    Poly arr[1];
    Poly test = PolyZero();
    Poly res = PolyCompose(&p, 0, arr);
    assert_true(PolyIsEq(&res, &test));
    PolyDestroy(&p);
}

/**
 * 'p' - polynomial x_0, 'count' = 1, x[0] - const polynomial,
 */
static void test_polyvarzero_countone_polyconst(void **state) {
    (void) state;
    Poly p1 = PolyFromCoeff(3);
    Mono m1 = MonoFromPoly(&p1, 4);
    Poly p = PolyAddMonos(1, &m1);
    Poly arr[1];
    arr[0] = PolyFromCoeff(-2);
    Poly test = PolyFromCoeff(48);
    Poly res = PolyCompose(&p, 1, arr);
    assert_true(PolyIsEq(&res, &test));
    PolyDestroy(&p);
}

/**
 * 'p' - polynomial x_0, 'count' = 1, x[0] - polynomial x_0.
 */
static void test_polyvarzero_countone_polyvarzero(void **state) {
    (void) state;
    Poly p1 = PolyFromCoeff(1);
    Mono m1 = MonoFromPoly(&p1, 2);
    Poly p2 = PolyFromCoeff(2);
    Mono m2 = MonoFromPoly(&p2, 1);
    Poly p3 = PolyFromCoeff(1);
    Mono m3 = MonoFromPoly(&p3, 0);
    Mono monos1[] = {m1, m2, m3};
    Poly p = PolyAddMonos(3, monos1);
    Poly arr[1];
    Poly p4 = PolyFromCoeff(1);
    Mono m4 = MonoFromPoly(&p4, 1);
    Poly p5 = PolyFromCoeff(-1);
    Mono m5 = MonoFromPoly(&p5, 0);
    Mono monos2[] = {m4, m5};
    polyTab[0] = PolyAddMonos(2, monos2);
    Poly p6 = PolyFromCoeff(1);
    Mono m6 = MonoFromPoly(&p6, 2);
    Poly test = PolyAddMonos(1, &m6);
    Poly res = PolyCompose(&p, 1, arr);
    assert_true(PolyIsEq(&res, &test));
    PolyDestroy(&p);
    PolyDestroy(&polyTab[0]);
    PolyDestroy(&test);
    PolyDestroy(&res);
}

/**
 * COMPOSE no argument
 */
static void test_compose_noparameter(void **state) {
    (void) state;
    init_input_stream("COMPOSE\n");
    assert_int_equal(mock_main(), 0);
    assert_string_equal(fprintf_buffer, "ERROR 1 WRONG COUNT\n");
}

/**
 * COMPOSE minimal argumentem
 */
static void test_compose_mincount(void **state) {
    (void) state;
    init_input_stream("COMPOSE 0\n");
    assert_int_equal(mock_main(), 0);
    assert_string_equal(fprintf_buffer, "ERROR 1 STACK UNDERFLOW\n");
}

/**
 * COMPOSE maximal argumentem
 */
static void test_compose_maxcount(void **state) {
    (void) state;
    init_input_stream("COMPOSE 4294967295\n");
    assert_int_equal(mock_main(), 0);
    assert_string_equal(fprintf_buffer, "ERROR 1 STACK UNDERFLOW\n");
}

/**
 * COMPOSE '-1' argumentem
 */
static void test_compose_negcount(void **state) {
    (void) state;
    init_input_stream("COMPOSE -1\n");
    assert_int_equal(mock_main(), 0);
    assert_string_equal(fprintf_buffer, "ERROR 1 WRONG COUNT\n");
}

/**
 * COMPOSE maximal argumentem + 1
 */
static void test_compose_overcount(void **state) {
    (void) state;
    init_input_stream("COMPOSE 4294967296\n");
    assert_int_equal(mock_main(), 0);
    assert_string_equal(fprintf_buffer, "ERROR 1 WRONG COUNT\n");
}

/**
 * COMPOSE maximal argumentem + some big number
 */
static void test_compose_bigcount(void **state){
    (void) state;
    init_input_stream("COMPOSE 64564564545645\n");
    assert_int_equal(mock_main(), 0);
    assert_string_equal(fprintf_buffer, "ERROR 1 WRONG COUNT\n");
}

/**
 * COMPOSE with letter combination argument
 */
static void test_compose_lettercount(void **state) {
    (void) state;
    init_input_stream("COMPOSE SsBf\n");
    assert_int_equal(mock_main(), 0);
    assert_string_equal(fprintf_buffer, "ERROR 1 WRONG COUNT\n");
}

/**
 * COMPOSE with letter and number combination argument
 */
static void test_compose_letnumcount(void **state){
    (void) state;
    init_input_stream("COMPOSE 1a2B3c4D5E\n");
    assert_int_equal(mock_main(), 0);
    assert_string_equal(fprintf_buffer, "ERROR 1 WRONG COUNT\n");
}

int main() {
    const struct CMUnitTest tests1[] = {
            cmocka_unit_test(test_polyzero_countzero),
            cmocka_unit_test(test_polyzero_countone_polyconst),
            cmocka_unit_test(test_polyconst_countzero),
            cmocka_unit_test(test_polyconst_countone_polyconst),
            cmocka_unit_test(test_polyvarzero_countzero),
            cmocka_unit_test(test_polyvarzero_countone_polyconst),
            cmocka_unit_test(test_polyvarzero_countone_polyvarzero)
    };
    const struct CMUnitTest tests2[] = {
            cmocka_unit_test_setup(test_compose_noparameter, test_setup),
            cmocka_unit_test_setup(test_compose_mincount, test_setup),
            cmocka_unit_test_setup(test_compose_maxcount, test_setup),
            cmocka_unit_test_setup(test_compose_negcount, test_setup),
            cmocka_unit_test_setup(test_compose_overcount, test_setup),
            cmocka_unit_test_setup(test_compose_bigcount, test_setup),
            cmocka_unit_test_setup(test_compose_lettercount, test_setup),
            cmocka_unit_test_setup(test_compose_letnumcount, test_setup),
    };

    return cmocka_run_group_tests(tests1, NULL, NULL) || cmocka_run_group_tests(tests2, NULL, NULL);
}

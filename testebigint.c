  /* Hugo Freires 2321223 3WA */
  /* Saulo Canto Matricula 3WB */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "bigint.h"

/* ==== utilitários de teste ==== */

/* imprime um BigInt como bytes little-endian */
static void dump_hex(const char *tag, BigInt x) {
    printf("%s = {", tag);
    for (int i = 0; i < (int)sizeof(BigInt); i++) {
        printf("0x%02X%s", x[i], (i == (int)sizeof(BigInt)-1) ? "" : ",");
    }
    printf("}\n");
}

/* compara dois BigInt; em caso de erro, mostra o diff e aborta */
static void expect_equal(const char *msg, BigInt got, BigInt exp) {
    if (memcmp(got, exp, sizeof(BigInt)) != 0) {
        printf("FAIL: %s\n", msg);
        dump_hex(" got", got);
        dump_hex(" exp", exp);
        assert(!"BigInt mismatch");
    } else {
        printf("OK  : %s\n", msg);
    }
}

/* fábrica: cria BigInt a partir de long usando big_val */
static void from_long(BigInt out, long v) {
    big_val(out, v);
}

/* ==== testes unitários por função ==== */

static void test_big_val(void) {
    BigInt x, e;

    from_long(e, 0); big_val(x, 0); expect_equal("big_val(0)", x, e);

    /* exemplo do enunciado: 1 */
    {
        unsigned char e1[16] = {
            0x01,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0
        };
        big_val(x, 1);
        memcpy(e, e1, 16);
        expect_equal("big_val(1)", x, e);
    }

    /* exemplo do enunciado: -2 */
    {
        unsigned char em2[16] = {
            0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
            0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
        };
        big_val(x, -2);
        memcpy(e, em2, 16);
        expect_equal("big_val(-2)", x, e);
    }

    /* -1: todos 0xFF */
    big_val(x, -1);
    for (int i=0;i<16;i++) e[i]=0xFF;
    expect_equal("big_val(-1)", x, e);
}

static void test_big_comp2(void) {
    BigInt a, r, e;

    from_long(a, -6);
    from_long(e, 6);
    big_comp2(r, a);
    expect_equal("comp2(-6) == 6", r, e);

    from_long(a, 0);
    from_long(e, 0);
    big_comp2(r, a);
    expect_equal("comp2(0) == 0", r, e);

    /* propriedade: -(-x) == x para alguns valores */
    long samples[] = {1, -1, 12345, -99999, 0x7FFFFFFF, (long)0x8000000000000000};
    for (int k=0;k<(int)(sizeof(samples)/sizeof(samples[0]));k++){
        from_long(a, samples[k]);
        big_comp2(r, a);          /* r = -a */
        big_comp2(r, r);          /* r = -(-a) */
        from_long(e, samples[k]);
        expect_equal("comp2 twice == identity", r, e);
    }
}

static void test_big_sum_sub(void) {
    BigInt a,b,r,e,zero={0};

    from_long(a, -5);
    from_long(b, 12);
    from_long(e, 7);
    big_sum(r, a, b);
    expect_equal("sum(-5,12)==7", r, e);

    from_long(a, 123456);
    big_sub(r, a, a);
    expect_equal("sub(a,a)==0", r, zero);

    /* sub como a + (-b) deve bater com big_sub */
    from_long(a, 7777);
    from_long(b, -999);
    BigInt nb, sum_res;
    big_comp2(nb, b);     /* nb = -b */
    big_sum(sum_res, a, nb);
    big_sub(r, a, b);
    expect_equal("sub vs comp2+sum", r, sum_res);
}

static void test_shifts(void) {
    BigInt a,r,e;

    /* shl(1,8) == 0x0100 */
    from_long(a, 1);
    big_shl(r, a, 8);
    for (int i=0; i<16; i++) { e[i] = 0; }  // <-- agora com chaves
    e[1] = 0x01;
    expect_equal("shl(1,8)==0x0100", r, e);

    /* shr lógico: 0x0100 >> 4 == 0x0010 */
    big_shr(r, r, 4);
    for (int i=0; i<16; i++) { e[i] = 0; }  // <-- aqui também
    e[0] = 0x10;
    expect_equal("shr(0x0100,4)==0x0010", r, e);

    /* sar: -1 >> n == -1 */
    from_long(a, -1);
    big_sar(r, a, 37);
    from_long(e, -1);
    expect_equal("sar(-1,37)==-1", r, e);

    /* shr vs sar para negativo devem diferir */
    from_long(a, -256); /* ...FF 00 00 */
    big_shr(r, a, 4);   BigInt sr; memcpy(sr, r, 16);
    big_sar(r, a, 4);   BigInt ar; memcpy(ar, r, 16);
    if (memcmp(sr, ar, 16) == 0) {
        dump_hex("shr(-256,4)", sr);
        dump_hex("sar(-256,4)", ar);
        assert(!"shr vs sar (negativo) deveriam diferir");
    } else {
        printf("OK  : shr vs sar divergem para negativo (como esperado)\n");
    }

    /* bordas: n=0 copia; n>=128 zera (lógico) ou preenche com sinal (aritmético) */
    from_long(a, 1);
    big_shl(r, a, 0);  expect_equal("shl n=0 copia", r, a);
    big_shr(r, a, 130); /* >=128 → 0 */ 
    BigInt zero = {0};  expect_equal("shr n>=128 == 0", r, zero);

    from_long(a, -42);
    big_sar(r, a, 200); /* >=128 → tudo FF (negativo) */
    for (int i=0;i<16;i++) e[i]=0xFF;
    expect_equal("sar n>=128 preenche com sinal", r, e);
}

static void test_mul(void) {
    BigInt a,b,r,e;

    from_long(a, 7); from_long(b, 6); from_long(e, 42);
    big_mul(r, a, b); expect_equal("mul(7,6)==42", r, e);

    from_long(a, -3); from_long(b, 11); from_long(e, -33);
    big_mul(r, a, b); expect_equal("mul(-3,11)==-33", r, e);

    from_long(a, -5); from_long(b, -7); from_long(e, 35);
    big_mul(r, a, b); expect_equal("mul(-5,-7)==35", r, e);

#if defined(__GNUC__) || defined(__clang__)
    /* oráculo com __int128: compara muitos casos a partir de long */
    for (long x = -1000; x <= 1000; x += 73) {
        for (long y = -1000; y <= 1000; y += 71) {
            __int128 m = (__int128)x * (__int128)y; /* produto com 128 bits */
            /* converte m para BigInt (little-endian) */
            BigInt exp;
            for (int i=0;i<16;i++) exp[i] = (unsigned char)(((unsigned __int128)m >> (8*i)) & 0xFFu);
            from_long(a, x); from_long(b, y);
            big_mul(r, a, b);
            if (memcmp(r, exp, 16) != 0) {
                printf("mul oracle FAIL for x=%ld y=%ld\n", x, y);
                dump_hex(" got", r);
                dump_hex(" exp", exp);
                assert(!"mul oracle mismatch");
            }
        }
    }
    printf("OK  : mul oracle (__int128) para amostras\n");
#endif
}

/* ==== MAIN: executa todos os testes ==== */
int main(void) {
    printf("=== Testes BigInt (TDD) ===\n");
    test_big_val();
    test_big_comp2();
    test_big_sum_sub();
    test_shifts();
    test_mul();
    printf("=== Todos os testes passaram. ===\n");
    return 0;
}

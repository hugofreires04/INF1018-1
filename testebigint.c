  /* Hugo Freires 2321223 3WA */
  /* Saulo Canto 2320940 3WB */

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

static void test_shift_matrix(void) {
    int ns[] = {0,1,2,7,8,15,16,31,32,63,64,127};
    int NN = (int)(sizeof(ns)/sizeof(ns[0]));

    /* padrões de bits */
    BigInt PAT_AA, PAT_55, PAT_FF, PAT_00; // 0xAA.., 0x55.., 0xFF.., 0x00..
    for (int i=0;i<16;i++) {
        PAT_AA[i]=0xAA; PAT_55[i]=0x55; PAT_FF[i]=0xFF; PAT_00[i]=0x00;
    }

    /* shifting zero continua zero (lógico e aritmético) */
    for (int k=0;k<NN;k++) {
        int n = ns[k];
        BigInt r;
        big_shl(r, PAT_00, n); expect_equal("shl(0,n)==0", r, PAT_00);
        big_shr(r, PAT_00, n); expect_equal("shr(0,n)==0", r, PAT_00);
        big_sar(r, PAT_00, n); expect_equal("sar(0,n)==0", r, PAT_00);
    }

    /* sar == shr para números não-negativos */
    {
        BigInt pos, r1, r2;
        from_long(pos, 0x12345678);
        for (int k=0;k<NN;k++) {
            int n = ns[k];
            big_shr(r1, pos, n);
            big_sar(r2, pos, n);
            expect_equal("sar == shr (não-negativo)", r1, r2);
        }
    }

    /* shr vs sar devem divergir para negativos quando n>0 */
    {
        BigInt neg, r1, r2;
        from_long(neg, -0x1234);
        for (int k=0;k<NN;k++) {
            int n = ns[k];
            big_shr(r1, neg, n);
            big_sar(r2, neg, n);
            if (n==0) {
                expect_equal("shr vs sar n=0 (iguais)", r1, r2);
            } else {
                if (memcmp(r1, r2, 16)==0) {
                    dump_hex("shr neg", r1);
                    dump_hex("sar neg", r2);
                    assert(!"shr vs sar deveriam diferir para negativo com n>0");
                }
            }
        }
    }

    /* padrões chatos (só sanity: não crasha e executa) */
    {
        BigInt r;
        for (int k=0;k<NN;k++) {
            int n = ns[k];
            big_shl(r, PAT_AA, n);
            big_shr(r, PAT_AA, n);
            big_sar(r, PAT_AA, n);

            big_shl(r, PAT_55, n);
            big_shr(r, PAT_55, n);
            big_sar(r, PAT_55, n);

            big_shl(r, PAT_FF, n);
            big_shr(r, PAT_FF, n);
            big_sar(r, PAT_FF, n);
        }
        printf("OK  : padrões 0xAA/0x55/0xFF shiftados em vários n\n");
    }
}

static void test_inplace(void) {
    /* comp2 in-place: agora a tua função suporta */
    {
        BigInt x, exp;
        from_long(x, 12345);
        from_long(exp, -12345);
        big_comp2(x, x); /* in-place */
        expect_equal("comp2 in-place", x, exp);
    }

    /* sum in-place: res == a */
    {
        BigInt a,b,exp;
        from_long(a, 100); from_long(b, 23);
        from_long(exp, 123);
        big_sum(a, a, b); /* res==a */
        expect_equal("sum in-place (res==a)", a, exp);
    }

    /* sub in-place: res == a */
    {
        BigInt a,b,exp;
        from_long(a, 100); from_long(b, 77);
        from_long(exp, 23);
        big_sub(a, a, b);
        expect_equal("sub in-place (res==a)", a, exp);
    }

    /* shl/shr/sar in-place */
    {
        BigInt x, exp;
        from_long(x, 1);           /* 1 << 12 = 4096 */
        from_long(exp, 4096);
        big_shl(x, x, 12);
        expect_equal("shl in-place", x, exp);

        from_long(x, 0x0100);      /* 0x0100 >> 8 = 0x01 */
        from_long(exp, 1);
        big_shr(x, x, 8);
        expect_equal("shr in-place", x, exp);

        from_long(x, -1024);       /* aritmético preserva sinal */
        big_sar(x, x, 5);          /* apenas valida que não corrompe / comportamento consistente */
        printf("OK  : sar in-place executado\n");
    }

    /* mul in-place (res==a) não é típica, mas vamos validar alguns casos pequenos: */
    {
        BigInt a,b,exp;
        from_long(a, 7); from_long(b, 6); from_long(exp, 42);
        big_mul(a, a, b);
        expect_equal("mul in-place (res==a)", a, exp);
    }
}

static void test_props(void) {
    /* a + (-a) == 0 para vários valores de long */
    {
        long vals[] = {0,1,-1,7,-7,123456,-99999,0x7FFFFFFF,-0x7FFFFFFF};
        for (int k=0;k<(int)(sizeof(vals)/sizeof(vals[0]));k++) {
            BigInt a,na,sum,zero={0};
            from_long(a, vals[k]);
            big_comp2(na, a);
            big_sum(sum, a, na);
            expect_equal("a + (-a) == 0", sum, zero);
        }
    }

    /* comutatividade: a+b == b+a ; a*b == b*a (mód 2^128) */
    {
        long X[] = {0,1,-1,2,-2,13,-17,12345,-54321};
        int NX = (int)(sizeof(X)/sizeof(X[0]));
        for (int i=0;i<NX;i++) for (int j=0;j<NX;j++) {
            BigInt a,b,r1,r2;

            from_long(a, X[i]); from_long(b, X[j]);
            big_sum(r1, a, b);  big_sum(r2, b, a);
            expect_equal("comutatividade soma", r1, r2);

            big_mul(r1, a, b);  big_mul(r2, b, a);
            expect_equal("comutatividade mul", r1, r2);
        }
    }

    /* associatividade da soma: (a+b)+c == a+(b+c) (mód 2^128) */
    {
        long X[] = {0,1,-1,2,-2,1000,-1000};
        for (int i=0;i<7;i++) for (int j=0;j<7;j++) for (int k=0;k<7;k++) {
            BigInt a,b,c,t1,t2,s1,s2;
            from_long(a, X[i]); from_long(b, X[j]); from_long(c, X[k]);
            big_sum(t1, a, b);      big_sum(s1, t1, c);
            big_sum(t2, b, c);      big_sum(s2, a, t2);
            expect_equal("associatividade soma", s1, s2);
        }
    }

    /* distributividade: a*(b+c) == a*b + a*c (mód 2^128) */
    {
        long A[] = {0,1,-1,7,-7,123,-123};
        long B[] = {0,2,-2,5,-5,11,-11};
        for (int i=0;i<7;i++) for (int j=0;j<7;j++) for (int k=0;k<7;k++) {
            BigInt a,b,c,bc,ab,ac,sum,prod;
            from_long(a, A[i]); from_long(b, B[j]); from_long(c, B[k]);
            big_sum(bc, b, c);          /* bc = b+c */
            big_mul(prod, a, bc);       /* a*(b+c) */
            big_mul(ab, a, b);          /* a*b */
            big_mul(ac, a, c);          /* a*c */
            big_sum(sum, ab, ac);       /* a*b + a*c */
            expect_equal("distributividade", prod, sum);
        }
    }

    /* equivalência: (num não-negativo) shl k == mul por 2^k; shr k == div trunc por 2^k */
    {
        long base[] = {0,1,2,3,7,15,16,31,32,63};
        int KN[] = {0,1,2,7,8,15,16,31,32,63};
        BigInt twoPowK, rShl, rMul, rShr, a, b;

        for (int bi=0; bi<(int)(sizeof(base)/sizeof(base[0])); bi++) {
            from_long(a, base[bi]);               /* não-negativo */
            for (int ki=0; ki<(int)(sizeof(KN)/sizeof(KN[0])); ki++) {
                int k = KN[ki];

                /* shl */
                big_shl(rShl, a, k);

                /* multiplica por 2^k — só até k<63 para evitar UB em shift de long */
                if (k < 63) {
                    from_long(twoPowK, 1L << k);
                    big_mul(rMul, a, twoPowK);
                    expect_equal("shl == mul por 2^k (k<63)", rShl, rMul);
                }

                /* shr (lógico) == dividir por 2^k (trunc) para não-negativo, k<63 */
                if (k < 63) {
                    long divval = base[bi] >> k;
                    from_long(b, divval);
                    big_shr(rShr, a, k);
                    expect_equal("shr == div 2^k (não-negativo)", rShr, b);
                }
            }
        }
        printf("OK  : equivalências shl/mul e shr/div (não-negativo, k<63)\n");
    }
}



/* ==== MAIN: executa todos os testes ==== */
int main(void) {
    printf("=== Testes BigInt (TDD) ===\n");
    test_big_val();
    test_big_comp2();
    test_big_sum_sub();
    test_shifts();
    test_inplace();        
    test_shift_matrix();   
    test_props();          
    test_mul();
    printf("=== Todos os testes passaram. ===\n");
    return 0;
}

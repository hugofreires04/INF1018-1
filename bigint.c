  /* Hugo Freires 2321223 3WA */
  /* Saulo Canto 2320940 3WB */

#include "bigint.h"
#include <stdio.h>
#include "string.h"

/* res = val (extensão de sinal para 128 bits) */
void big_val (BigInt res, long val){
    /* zera os 16 bytes do resultado (evita lixo nos bytes altos) */
    for (int i = 0; i < (int)sizeof(BigInt); i++) {  // <-- cast para (int)
        res[i] = 0;
    }

    int neg = (val < 0);    // 1 se o número é negativo, 0 se é não-negativo

    unsigned long temp = (unsigned long)val; // copia os bits para unsigned (shifts/máscaras sem sinal)
    unsigned char parte = 0;                  // guarda o último byte (LSB) de temp

    /* escreve os bytes do 'long' em ordem little-endian: byte 0 = LSB */
    for (int i = 0; i < (int)sizeof(long); i++) {
        parte = (unsigned char)(temp & 0xFF);  // pega o LSB
        res[i] = parte;                        // grava no BigInt
        temp = temp >> 8;                      // avança para o próximo byte
    }

    if (neg == 0){
        /* se não-negativo, a extensão de sinal é 0x00 nos bytes altos (já estão zerados) */
        return;
    }

    /* se negativo, extensão de sinal com 0xFF nos bytes mais significativos */
    for (int i = (int)sizeof(long); i < (int)sizeof(BigInt); i++) { // <-- cast para (int)
        res[i] = 0xFF;
    }
}

/* res = -a  (complemento de 2: ~a + 1) */
void big_comp2(BigInt res, BigInt a){

    unsigned int carry = 1; // inicia em 1 por causa do "+1" do complemento de 2

    for (int i = 0; i < (int)sizeof(BigInt); i++) {
        unsigned int inv = (unsigned int)(~a[i]) & 0xFF;  // inverte bits do byte e limita a 8 bits
        unsigned int soma = inv + carry;                  // soma +1 (e possíveis vai-uns)
        res[i] = (unsigned char)(soma & 0xFF);            // guarda só o byte atual
        carry = soma >> 8;                                // carry (vai-um) para o próximo byte
    }
}

/* res = a + b (módulo 2^128) */
void big_sum (BigInt res, BigInt a, BigInt b) {

    unsigned int carry = 0;

    for (int i = 0; i < (int)sizeof(BigInt); i++) {
        // soma byte a byte + carry anterior
        unsigned int soma = (unsigned int)a[i] + (unsigned int)b[i] + carry;
        res[i] = (unsigned char)(soma & 0xFF); // guarda só 8 bits
        carry = soma >> 8;                     // transbordo vira carry pro próximo byte
    }
}

/* res = a - b (implementação por borrow) */
void big_sub (BigInt res, BigInt a, BigInt b) {

    unsigned int prox = 0; // "borrow" (empresta 1) do próximo byte

    for (int i = 0; i < (int)sizeof(BigInt); i++) {
        int sub = (int)a[i] - (int)b[i] - (int)prox;

        if (sub < 0) {
            sub += 256; // empresta 1 byte (2^8)
            prox = 1;   // sinaliza borrow pro próximo
        } else {
            prox = 0;
        }

        res[i] = (unsigned char)(sub & 0xFF);
    }
}

/* res = a << n (deslocamento lógico à esquerda) */
/* Little-endian: a[0] = LSB, a[15] = MSB. In-place SAFE via buffer temporário. */
void big_shl (BigInt res, BigInt a, int n) {
    if (n <= 0) {                    /* n=0 (ou negativo): copia */
        if (res != a) memcpy(res, a, sizeof(BigInt));
        return;
    }
    if (n >= 8 * (int)sizeof(BigInt)) {   /* n >= 128: tudo 0 */
        for (int i = 0; i < (int)sizeof(BigInt); i++) res[i] = 0;
        return;
    }

    int byte_shift = n / 8;   /* deslocamento inteiro em bytes */
    int bit_shift  = n % 8;   /* deslocamento “miúdo” em bits  */

    BigInt tmp;

    /* 1) desloca por BYTES para a esquerda:
          tmp[i] recebe a[i - byte_shift]; fora do range entra 0 */
    for (int i = 0; i < (int)sizeof(BigInt); i++) {
        int src = i - byte_shift;
        tmp[i] = (src >= 0 && src < (int)sizeof(BigInt)) ? a[src] : 0x00;
    }

    /* 2) desloca por BITS dentro dos bytes (LSB -> MSB), propagando carry */
    if (bit_shift != 0) {
        unsigned int carry = 0;
        for (int i = 0; i < (int)sizeof(BigInt); i++) {
            unsigned int v = ((unsigned int)tmp[i] << bit_shift) | carry;
            tmp[i] = (unsigned char)(v & 0xFF);
            carry  = v >> 8;  /* transborda para o próximo byte */
        }
    }

    /* 3) escreve no destino (seguro mesmo se res == a) */
    memcpy(res, tmp, sizeof(BigInt));
}


/* res = a >> n (lógico) */
void big_shr (BigInt res, BigInt a, int n) {
    /* casos triviais */
    if (n <= 0) {
        memcpy(res, a, sizeof(BigInt));
        return;
    }
    if (n >= 8 * (int)sizeof(BigInt)) {
        /* deslocou >= 128 bits → vira 0 */
        for (int i = 0; i < (int)sizeof(BigInt); i++) res[i] = 0;
        return;
    }

    /* separamos n em deslocamento de bytes e de bits */
    int byte_shift = n / 8;   /* quantos bytes inteiros mover */
    int bit_shift  = n % 8;   /* quantos bits dentro do byte */

    BigInt tmp;

    /* 1) desloca por BYTES para a direita:
          byte i de tmp vem do byte (i + byte_shift) de a
          se estourar, entra 0 (lógico) */
    for (int i = 0; i < (int)sizeof(BigInt); i++) {
        int src = i + byte_shift;
        tmp[i] = (src < (int)sizeof(BigInt)) ? a[src] : 0x00;
    }

    /* 2) desloca por BITS dentro dos bytes:
          varre do topo (MSB) para a base (LSB),
          carregando para o byte menor os bits que cairam do byte maior */
    if (bit_shift != 0) {
        unsigned int carry = 0; /* bits baixos do byte anterior */
        for (int i = (int)sizeof(BigInt) - 1; i >= 0; i--) {
            unsigned int v = ((unsigned int)tmp[i] >> bit_shift) | (carry << (8 - bit_shift));
            carry = tmp[i] & ((1u << bit_shift) - 1u); /* guarda os bits que sobraram */
            tmp[i] = (unsigned char)(v & 0xFF);
        }
    }

    /* 3) copia para o resultado (seguro mesmo se res == a) */
    memcpy(res, tmp, sizeof(BigInt));
}


/* res = a >> n (aritmético: preserva o sinal) */
void big_sar (BigInt res, BigInt a, int n) {
    /* casos triviais */
    if (n <= 0) {
        memcpy(res, a, sizeof(BigInt));
        return;
    }

    /* descobre o bit de sinal original (bit mais significativo do último byte) */
    unsigned char sign = (a[(int)sizeof(BigInt) - 1] & 0x80) ? 0xFF : 0x00;

    if (n >= 8 * (int)sizeof(BigInt)) {
        /* deslocou >= 128 bits → tudo vira sinal */
        for (int i = 0; i < (int)sizeof(BigInt); i++) res[i] = sign;
        return;
    }

    int byte_shift = n / 8;
    int bit_shift  = n % 8;

    BigInt tmp;

    /* 1) desloca por BYTES para a direita:
          byte i de tmp vem de a[i + byte_shift]
          se estourar, entra 'sign' (replicação do bit de sinal) */
    for (int i = 0; i < (int)sizeof(BigInt); i++) {
        int src = i + byte_shift;
        tmp[i] = (src < (int)sizeof(BigInt)) ? a[src] : sign;
    }

    /* 2) desloca por BITS dentro dos bytes:
          varre do MSB para o LSB,
          com carry inicial "cheio" (111..1) se o número é negativo,
          para injetar 1s pela esquerda; senão carry = 0 */
    if (bit_shift != 0) {
        unsigned int carry = (sign == 0xFF) ? ((1u << bit_shift) - 1u) : 0u;
        for (int i = (int)sizeof(BigInt) - 1; i >= 0; i--) {
            unsigned int v = ((unsigned int)tmp[i] >> bit_shift) | (carry << (8 - bit_shift));
            carry = tmp[i] & ((1u << bit_shift) - 1u);
            tmp[i] = (unsigned char)(v & 0xFF);
        }
    }

    memcpy(res, tmp, sizeof(BigInt));
}

/* res = a * b (módulo 2^128) via shift-and-add */
void big_mul (BigInt res, BigInt a, BigInt b) {
    BigInt acc; // acumulador do resultado parcial
    for (int i = 0; i < (int)sizeof(BigInt); i++) acc[i] = 0; // <-- cast para (int)

    BigInt sh;  // "versão deslocada" de a, que anda 1 bit a cada passo
    memcpy(sh, a, sizeof(BigInt));

    // percorre cada um dos 128 bits de b (varrendo por bytes, e dentro de cada byte, por bits)
    for (int byte = 0; byte < (int)sizeof(BigInt); byte++) { // <-- cast para (int)
        unsigned char bj = b[byte]; // byte atual de b (8 bits)
        for (int bit = 0; bit < 8; bit++) {
            if (bj & 1) {          // se o bit atual de b é 1
                BigInt temp;       // soma o parcial: acc += sh
                big_sum(temp, acc, sh);
                memcpy(acc, temp, sizeof(BigInt));
            }
            BigInt temp2;
            big_shl(temp2, sh, 1); // sh <<= 1 (prepara para o próximo bit)
            memcpy(sh, temp2, sizeof(BigInt));
            bj >>= 1;              // avança para o próximo bit do byte bj
        }
    }

    memcpy(res, acc, sizeof(BigInt)); // resultado final
}
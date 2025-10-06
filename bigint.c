  /* Hugo Freires 2321223 3WA */
  /* Saulo Canto Matricula 3WB */

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
    /* zera res (não estritamente necessário; aqui mantém padrão) */
    for (int i = 0; i < (int)sizeof(BigInt); i++) {
        res[i] = 0;
    }

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
    for (int i = 0; i < (int)sizeof(BigInt); i++) {
        res[i] = 0;
    }

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
    for (int i = 0; i < (int)sizeof(BigInt); i++) {
        res[i] = 0;
    }

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
void big_shl (BigInt res, BigInt a, int n){
    for (int i = 0; i < (int)sizeof(BigInt); i++) {
        res[i] = 0;
    }

    // casos triviais
    if (n <= 0) {
        for (int i = 0; i < (int)sizeof(BigInt); i++){
            res[i] = a[i]; // copia
        }
        return;
    }
    if (n >= 8 * (int)sizeof(BigInt)) { // <-- cast para (int)
        return;            // deslocou >=128 bits → tudo 0
    }

    int byte_shift = n / 8; // quantos bytes inteiros deslocar
    int bit_shift  = n % 8; // quantos bits dentro do byte

    // 1) desloca por bytes: copia a[src] para res[i], com src = i - byte_shift
    for (int i = 0; i < (int)sizeof(BigInt); i++) {
        int src = i - byte_shift;
        if (src >= 0 && src < (int)sizeof(BigInt)) { // <-- casts para (int)
            res[i] = a[src];
        } else {
            res[i] = 0; // zeros entrando pela direita
        }
    }

    // 2) desloca os bits dentro de cada byte, propagando carry para os bytes mais altos
    if (bit_shift != 0) {
        unsigned int carry = 0;
        for (int i = 0; i < (int)sizeof(BigInt); i++) {
            unsigned int v = ((unsigned int)res[i] << bit_shift) | carry;
            res[i] = (unsigned char)(v & 0xFF);
            carry  = v >> 8;  // bits que transbordaram sobem para o próximo byte
        }
    }
}

/* res = a >> n (lógico) */
void big_shr (BigInt res, BigInt a, int n) {
    for (int i = 0; i < (int)sizeof(BigInt); i++) { // <-- cast para (int)
        res[i] = 0;
    }

    if (n <= 0) {
        memcpy(res, a, sizeof(BigInt));
        return;
    }

    if (n >= 8 * (int)sizeof(BigInt)) { // <-- cast para (int)
        return; // tudo 0
    }

    int byte_shift = n / 8;
    int bit_shift  = n % 8;

    // 1) desloca por bytes para a direita: byte i recebe de i+byte_shift
    for (int i = 0; i < (int)sizeof(BigInt); i++) { // <-- cast para (int)
        int src = i + byte_shift;
        if (src < (int)sizeof(BigInt)) {           // <-- cast para (int)
            res[i] = a[src];
        } else {
            res[i] = 0; // zeros entrando pela esquerda
        }
    }

    // 2) desloca por bits, varrendo do topo para a base (MSB → LSB),
    //    injetando no byte atual os bits baixos do byte anterior (carry)
    if (bit_shift != 0) {
        unsigned int carry = 0;
        for (int i = (int)sizeof(BigInt) - 1; i >= 0; i--) { // <-- cast para (int)
            unsigned int v = ((unsigned int)res[i] >> bit_shift) | (carry << (8 - bit_shift));
            carry = res[i] & ((1u << bit_shift) - 1u); // guarda os bits que "caem" pro próximo
            res[i] = (unsigned char)(v & 0xFF);
        }
    }
}

/* res = a >> n (aritmético: preserva o sinal) */
void big_sar (BigInt res, BigInt a, int n) {
    if (n <= 0) {
        memcpy(res, a, sizeof(BigInt));
        return;
    }

    if (n >= 8 * (int)sizeof(BigInt)) { // <-- cast para (int)
        // tudo vira 0xFF..FF se negativo, ou 0x00..00 se não-negativo
        unsigned char fill = (a[(int)sizeof(BigInt) - 1] & 0x80) ? 0xFF : 0x00; // <-- cast para (int)
        for (int i = 0; i < (int)sizeof(BigInt); i++) { // <-- cast para (int)
            res[i] = fill;
        }
        return;
    }

    int byte_shift = n / 8;
    int bit_shift  = n % 8;
    unsigned char fill = (a[(int)sizeof(BigInt) - 1] & 0x80) ? 0xFF : 0x00; // bit de sinal original

    // 1) desloca por bytes preenchendo com fill (replicação do sinal)
    for (int i = 0; i < (int)sizeof(BigInt); i++) { // <-- cast para (int)
        int src = i + byte_shift;
        if (src < (int)sizeof(BigInt)) {           // <-- cast para (int)
            res[i] = a[src];
        } else {
            res[i] = fill; // entra sinal à esquerda
        }
    }

    // 2) desloca por bits com “carry” inicial = 111..1 se negativo, ou 0 se positivo
    if (bit_shift != 0) {
        unsigned int carry = (fill == 0xFF) ? ((1u << bit_shift) - 1u) : 0u;
        for (int i = (int)sizeof(BigInt) - 1; i >= 0; i--) { // <-- cast para (int)
            unsigned int v = ((unsigned int)res[i] >> bit_shift) | (carry << (8 - bit_shift));
            carry = res[i] & ((1u << bit_shift) - 1u);
            res[i] = (unsigned char)(v & 0xFF);
        }
    }
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
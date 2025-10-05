//#include "bigint.h"
#include <stdio.h>

#define NUM_BITS 128
typedef unsigned char BigInt[NUM_BITS/8];

void big_val (BigInt res, long val){
    
    for (int i = 0; i < sizeof(BigInt); i++) {  // zera o vetor resultado
        res[i] = 0;
    }

    int neg = (val < 0);    // 1 se o numero é negativo, 0 se é positivo

    unsigned long temp = (unsigned long)val; // cria uma variável temporaria pra n mexer em val
    unsigned char parte = 0;    // variavel que representa o ultimo byte de temp

    for (int i = 0; i < (int)sizeof(long); i++) {
        parte = (unsigned char)(temp & 0xFF);    //pega o ultimo byte da variável
        res[i] = parte; //coloca no resultado o último byte de temp
        temp = temp>>8; //"incrementa" temp
    }

    if (neg == 0){
        return; // se for positivo, mantém os bytes mais significativos zerados
    }

    for (int i = (int)sizeof(long); i < (int)sizeof(BigInt); i++) { // se for negativo, passa os útimos bytes para 0xFF
        res[i] = 0xFF;
    }
}

void big_comp2(BigInt res, BigInt a){
    for (int i = 0; i < (int)sizeof(BigInt); i++) {  // zera o vetor resultado
        res[i] = 0;
    }

    unsigned int carry = 1; // inicia em 1 pois complemento de 2 = (~a) + 1

    for (int i = 0; i < (int)sizeof(BigInt); i++) {
        unsigned int inv = (unsigned int)(~a[i]) & 0xFF;  // usa o operador ~, por isso, faz as conversões necessárias para int e "& 0xFF" para pegar apenas o byte necessario

        unsigned int soma = inv + carry;

        res[i] = (unsigned char)(soma & 0xFF); // pega apenas o byte relevante da soma

        carry = soma >> 8;
    }
}

void big_sum (BigInt res, BigInt a, BigInt b) {
    for (int i = 0; i < (int)sizeof(BigInt); i++) {
        res[i] = 0;
    }

    unsigned int carry = 0;

    for (int i = 0; i < (int)sizeof(BigInt); i++) {
        unsigned int soma = (unsigned int)a[i] + (unsigned int)b[i] + carry;
        res[i] = (unsigned char)(soma & 0xFF);
        carry = soma >> 8;
    }
}

void big_sub (BigInt res, BigInt a, BigInt b) {
    for (int i = 0; i < (int)sizeof(BigInt); i++) {
        res[i] = 0;
    }

    unsigned int prox = 0;

    for (int i = 0; i < (int)sizeof(BigInt); i++) {
        int sub = (int)a[i] - (int)b[i] - (int)prox;

        if (sub < 0) {
            sub += 256;
            prox = 1;
        } else {
            prox = 0;
        }

        res[i] = (unsigned char)(sub & 0xFF);
    }
}

void big_shl (BigInt res, BigInt a, int n){
    for (int i = 0; i < (int)sizeof(BigInt); i++) {
        res[i] = 0;
    }

    if (n <= 0) {
        for (int i = 0; i < (int)sizeof(BigInt); i++){
            res[i] = a[i];
        } 
        return;
    }
    if (n >= 8 * (int)sizeof(BigInt)) {
        return;
    }

    int byte_shift = n / 8;
    int bit_shift  = n % 8;


    int dif = byte_shift; // deslocamento em bytes
    for (int i = 0; i < (int)sizeof(BigInt); i++) {
        int src = i - dif; // índice da origem no vetor 'a'

        if (src >= 0 && src < (int)sizeof(BigInt)) {
            res[i] = a[src];   // copia o byte deslocado
        } else {
            res[i] = 0;        // bytes "novos" entram como 0
        }
    }

    if (bit_shift != 0) {
        unsigned int carry = 0;
        for (int i = 0; i < (int)sizeof(BigInt); i++) {
            unsigned int v = ((unsigned int)res[i] << bit_shift) | carry;
            res[i] = (unsigned char)(v & 0xFF);
            carry  = v >> 8;  // transborda pro próximo byte
        }
    }

}

int main(void){

    BigInt res = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    long val = -2;

    big_val(res,val);

    for(int i = 0;i<sizeof(BigInt);i++){
        printf(" {0x%02X} , ", (unsigned char)res[i]);
    }
    printf("\n");

    BigInt a, b;
    big_val(a, -6);
    big_comp2(res,a);

    for(int i = 0;i<sizeof(BigInt);i++){
        printf(" {0x%02X} , ", (unsigned char)res[i]);
    }
    printf("\n");

    big_val(b, -5);
    big_sum(res,a,b);

    for(int i = 0;i<sizeof(BigInt);i++){
        printf(" {0x%02X} , ", (unsigned char)res[i]);
    }
    printf("\n");

    big_val(a, 5);
    big_val(b, -5);
    big_sub(res,a,b);

    for(int i = 0;i<sizeof(BigInt);i++){
        printf(" {0x%02X} , ", (unsigned char)res[i]);
    }
    printf("\n");

    big_val(a, 5);
    big_shl(res,a,32);

    for(int i = 0;i<sizeof(BigInt);i++){
        printf(" {0x%02X} , ", (unsigned char)res[i]);
    }
    printf("\n");

}

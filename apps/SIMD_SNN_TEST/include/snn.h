#ifndef __SNN_H__
#define __SNN_H__

#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#define SEED 1

typedef unsigned short us16x4_t __attribute__((vector_size (8)));
typedef unsigned char  us8x8_t __attribute__ ((vector_size (8)));

uint64_t __rv_svr(us16x4_t taulrvr, uint64_t acc){
    uint64_t ret;
    asm volatile(
        ".insn r 0xb, 0x7, 0x3, %0, %1, %2"
            :"=r"(ret)
            :"r"(taulrvr), "r"(acc)
    );
    return ret;
}

/*SUM: 4 16-bit data accumulation, with/without acc register*/
uint64_t __rv_sum( us16x4_t rs, us16x4_t masks, uint8_t hasAcc){

    uint64_t ret;
    if(hasAcc == 1){
        asm volatile(
            ".insn r 0xb, 0x5, 0x1, %0, %1, %2"
                :"=r"(ret)
                :"r"(rs),"r"(masks)
        );
    }
    else{
        asm volatile(
            ".insn r 0xb, 0x5, 0x0, %0, %1, %2"
                :"=r"(ret)
                :"r"(rs),"r"(masks)
        );
    }
    
    return ret;
}


/*NUP: neuron update, with/without timestamp*/
us16x4_t __rv_nup(us16x4_t rs1, us16x4_t rs2, uint8_t hasTs){
    us16x4_t ret;
    if(hasTs == 1){
        asm volatile(
            ".insn r 0xb, 0x0, 0x1, %0, %1, %2"
                :"=r"(ret)
                :"r"(rs1),"r"(rs2)
        );
    }
    else{
        asm volatile(
            ".insn r 0xb, 0x0, 0x0, %0, %1, %2"
                :"=r"(ret)
                :"r"(rs1),"r"(rs2)
        );
    }
    
    return ret;
}

/*EXP: calculate exp function with 4 value*/
us16x4_t __rv_exp(us16x4_t rs1){
    us16x4_t ret;
    asm volatile(
            ".insn r 0xb, 0x2, 0x0, %0, %1, x0"
                :"=r"(ret)
                :"r"(rs1)
        );
    return ret;
}

us16x4_t  __rv_tdr(us16x4_t rs1, us16x4_t rs2){
    us16x4_t ret;
    asm volatile(
            ".insn r 0xb, 0x4, 0x0, %0, %1, %2"
                :"=r"(ret)
                :"r"(rs1), "r"(rs2)
        );
    return ret;
}

/*BPO: back-propagation output-hidden ksi calculation*/
us16x4_t __rv_bpo(us16x4_t rs1, us16x4_t rs2){
    us16x4_t ret;
    asm volatile(
            ".insn r 0xb, 0x1, 0x1, %0, %1, %2"
                :"=r"(ret)
                :"r"(rs1), "r"(rs2)
        );
    return ret;
}

#define WEIGHT_FIX_POINT(x) x * 4096
#define NU16_FIX_POINT(x) x * 256

#define SUCCESS_MSG \
    printf("*****************\n");\
    printf("TEST PASSED!\n");\
    printf("*****************\n\n")

#define FAIL_MSG \
    printf("*****************\n");\
    printf("TEST FAILED!\n");\
    printf("*****************\n\n")

#endif /*__SNN_H__*/

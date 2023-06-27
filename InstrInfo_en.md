# Indtroduction for RV-SNN2

## Content

<!-- vim-markdown-toc GFM -->

* [Introduction](#introduction)
* [Details of Intructions](#details-of-intructions)
    * [Neuron Update Instruction](#neuron-update-instruction)
    * [Time Dependence Rule Instruction](#time-dependence-rule-instruction)
    * [Back-Propagation Output Direction Parameter Computing Instruction](#back-propagation-output-direction-parameter-computing-instruction)
    * [EXP Computing Instruction](#exp-computing-instruction)
    * [16-bit Sum Instruction](#16-bit-sum-instruction)
    * [Network Parameters Setting Instruction](#network-parameters-setting-instruction)
* [Programming guide](#programming-guide)
    * [The Reference of RV-SNN2 Instructions](#the-reference-of-rv-snn2-instructions)
    * [Example](#example)
    * [Executing the Compiled Program](#executing-the-compiled-program)

<!-- vim-markdown-toc -->
## Introduction

RV-SNN2 instruction extensions are realized on the RISC-V SIMD Superscalar Dual-Issue Processor [Polaris](https://github.com/ByeBeihai/Polaris).
We add a SNN accelerating module in the executive stage as a sub-component to realize the RV-SNN2 extended Instructions.

RV-SNN2 has 3 types, 6 instructions:

- Neuron Update Instruction: NUP
- Synaptic Plasticity and Universal Computational Instructions: TDR BPO EXP SUM16
- Network Parameters Setting Instruction: SVR

|No.|Mnemonic|Description|Operation|
|:-:|:--|:--|:--|
|1|NUP rd, rs1, rs2|16-bit Neuron Updating Instruction|if ts_flag=1:<br>&emsp;&emsp;rd.H[i]=rs1.H[x]<br>&emsp;&emsp;rd.L[x]=NUP(rs1.L[x], rs2[x])<br>else:<br>&emsp;&emsp;rd[x]=NUP(rs1[x], rs2[x])<br>(x=3..0)|
|2|TDR rd, rs1, rs2|Time Dependence Rule Instruction|rd[x]=rs1.H[x]-rs2.H[x]|
|3|BPO rd, rs1, rs2|Back-Propagation Output Direction Parameter Computing Instruction|if rs2[x]=0:<br>&emsp;&emsp;rd[x]=rs1[x]==1?-1:0<br>else if rs2[x]=1:<br>&emsp;&emsp;rd[x]=rs1[x]==1?1:0<br>(x=3..0)|
|4|EXP rd,rs1|EXP Computing Instruction|rd[x]=exp(rs1[x])|
|5|SUM16 rd, rs1, rs2|16-bit Sum Instruction|if `acc_flag`=1:<br>for x in [0,3]:<br>&emsp;&emsp;rd += rs2[x]==1?rs1[x]:0<br>else:<br>for x in [0,3]:<br>&emsp;&emsp;tmp += rs2[x]==1?rs1[x]:0<br>&emsp;&emsp;rd = tmp + `sum_accumulate`<br>(x=3..0)|
|6|SVR rd, rs1, rs2|Network Parameters Setting Instruction|The SNN Register File (SRF) in SNNU:<br>$V_{reset}$=rs1[0];<br>$\mu$=rs1[1];<br>$\tau$=rs1[2];<br>`sum_accumulate`=rs2|

## Details of Intructions

### Neuron Update Instruction

In Neuron Update Instruction (NUP), `rs1` specifies the registers that store 4 16-bit neurons, `rs2` specifies the registers that store the inputs of 4 neurons, and `ts_flag` indicates whether the neuron data has timestamp data.

| |`ts_flag`=1|`ts_flag`=0|
|:-- |:---|:---|
|rs1[15:8]|`Timestamp[7:0]`|`Neural membrane potential[15:8]`|
|rs1[7:0]|`Neural membrane potential[7:0]`|`Neural membrane potential[7:0]`|

The neuron update instruction uses the following formula to update the LIF model:

$$
V_{next}=V+dV=V-(V>>\tau)+((V_{rest}+\sum{wS})>>\tau)
$$

where $V$ is the current membrane potential, $\tau$ is the time constant of the LIF neuron, and $\sum{wS}$ is the sum of the product of the input pulse and its transmitted synaptic weight. 

The function interface is as follows:

```c
us16x4_t __rv_nup(us16x4_t rs1, us16x4_t rs2, uint8_t hasTs);
```

---

### Time Dependence Rule Instruction

In the Time Dependence Rule command (TDR), `rs1` and `rs2` respectively specify the storage of 4 neuron data with time stamps. The TDR command will only calculate the difference of the time stamps, and the neuron membrane potential will not participate in the calculation.

The function interface is as follows:

```c
us16x4_t  __rv_tdr(us16x4_t rs1, us16x4_t rs2);
```

---

### Back-Propagation Output Direction Parameter Computing Instruction


The back-propagation output direction parameter calculation instruction (BPO) solves the output parameter $\xi_o$ according to the following formula:

$$
 \xi_o=
 \begin{cases}
 1& \text{, when target neuron fires in [t-4,t]}\\
 -1& \text{,when non-target neuron fires in [t-4,t]}\\
 0& \text{, others}
 \end{cases}
$$

In the instruction, `rs1` specifies the firing results of 4 neurons, and `rs2` specifies the target neuron (`rs2[x]=1` means that the current nerve is the target neuron).。

The function interface is as follows:

```c
us16x4_t __rv_bpo(us16x4_t rs1, us16x4_t rs2);
```

---

### EXP Computing Instruction

The exponential function calculation instruction (EXP) supports simultaneous calculation of exponential function values corresponding to four 16-bit fixed-point numbers. At present, the range of convergence domain is $\theta\in[-2.0794,2.0794]$, that is to say, the value of the exponential function obtained within this range is meaningful.

|16-bit fixed point number `n`|`n[15]`|`n[14:11]`|`n[10:0]`|
|:-:|:-:|:-:|:-:|
||Sign bit|Integer bit|Decimal bit|

The function interface is as follows:

```c
us16x4_t __rv_exp(us16x4_t rs1);
```

---

### 16-bit Sum Instruction

In the 16-bit sum instruction (SUM16), `acc_flag` is used to indicate whether the accumulation register in the SRF register participates in the calculation. If `acc_flag=1`, it participates in the accumulation calculation, otherwise it does not participate in the accumulation. The output of the last instruction is a 64-bit number

The function interface is as follows:

```c
us16x4_t __rv_sum(us16x4_t rs1);
```

---

### Network Parameters Setting Instruction

In the network parameter setting instruction (SVR), `rs1` specifies the neuron initial membrane potential $V_{rest}$, time constant $\tau$ and synaptic plasticity learning rate $\mu$.

|`rs1`|`rs1[0]`|`rs1[1]`|`rs1[2]`|`rs1[3]`|
|:-:|:-:|:-:|:-:|:-:|
|Element|$V_{rest}$|$\mu$|$\tau$|reverse|

---
## Programming guide

### The Reference of RV-SNN2 Instructions

For SNN extensions, we provide a functional inline assembly interface. By cloning the [AM](https://github.com/OSCPU/nexus-am) project or this repo, you can create your project in the `apps/` directory, and add the import of `snn.h` to the program source code. Invoke extension instructions by means of function calls.

The built-in function interfaces proposed in the chapter [Details of Intructions](#details-of-intructions) are all defined in `snn.h`, you can copy the following code to implement `snn.h`.

```c
// snn.h
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

```

### Example

This section is taken from the neuron update section in the command function test program:

```c
// NUP test
    #if !__DEBUG__
    uint32_t n = 100;
    #endif
    #if __NUP_TEST__
    printf("Neural without Time Stamp Intr test\n");
    us16x4_t taulrvr = {0, 0, 4, 0};
    uint32_t tau = 4;
    __rv_svr(taulrvr, 0); // 设置网络参数 rs1:{0:vr, 1:lr, 2:tau, 3:保留全0} rs2:0
    us16x4_t nu_without_ts = {0, 0, 0, 0}; 
    us16x4_t s_without_ts = {100, 100, 100, 100};
    uint16_t nu_without_ts_ref[4] = {0, 0, 0, 0};
    for(int i = 0; i < n; i++){
        nu_without_ts = __rv_nup(nu_without_ts, s_without_ts, 0); // NUP 指令，最后一个参数为ts_flag
        for (int j = 0; j < 4; j++){
            nu_without_ts_ref[j] = nu_without_ts_ref[j] + (s_without_ts[j] >> tau) - (nu_without_ts_ref[j] >> tau);
            
            if(nu_without_ts[j] != nu_without_ts_ref[j]){
                FAIL_MSG;
                printf("%x \n", nu_without_ts[j]);
                printf("%x \n", nu_without_ts_ref[j]);
                return 0;
            }
        }
    }
    SUCCESS_MSG;
    printf("Neural with Time Stemp Intr test\n");
    us16x4_t nu_with_ts = {0, 0, 0, 0};
    us16x4_t s_with_ts = {100, 100, 100, 100};
    uint16_t nu_withTS_ref[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    for (int i = 0; i < n; i++){
        nu_with_ts = __rv_nup(nu_with_ts, s_with_ts, 1); // NUP 指令，ts_flag = 1
        for(int j = 0; j < 4; j++){
            nu_withTS_ref[j * 2 + 1] = nu_withTS_ref[j * 2 + 1] + (s_with_ts[j] >> tau) - (nu_withTS_ref[j * 2 + 1] >> tau);
            if(nu_with_ts[j] != nu_withTS_ref[j * 2 + 1] + (nu_withTS_ref[j * 2] << 8)){
            FAIL_MSG;
            printf("%x \t", nu_with_ts[j]);
            printf("%x %x %x\n", nu_withTS_ref[j], nu_withTS_ref[j *2 +1], nu_withTS_ref[j * 2 + 1] + (nu_withTS_ref[j] << 8));
            return 0;
            }
        }    
    }
    SUCCESS_MSG;
    #endif
```

### Executing the Compiled Program

Run the following command in the root directory of this repo:

```bash
$ ./emu -b 0 -e 0 -i <the path to your executive file>
```

There is a test program in the `ready-to-run` folder in the repository, run with the above command:

```bash
$ ./emu -b 0 -e 0 -i ./ready-to-run/cpuTest.bin
```

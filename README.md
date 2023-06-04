# RV-SNN2ָ���չ˵��

## Ŀ¼
[TOC]

## ���

RV-SNN2��չָ���ǻ���RISC-V�ܹ�˫����˳������[Polaris](https://github.com/ByeBeihai/Polaris)ʵ�ֵġ������ͨ�ô������У����Ǽ�����һ�������õ��������������ģ����ʵ��RV-SNN2��չָ�

RV-SNN2��Ҫ��6��ָ���Ϊ���ࣺ

- ��Ԫ����ָ�NUP
- ͻ����������ͨ�ü���ָ�TDR BPO EXP SUM16
- ����������ָ�SVR

|No.|���Ƿ�|����|����|
|:-:|:--|:--|:--|
|1|NUP rd, rs1, rs2|16������Ԫ����|if ts_flag=1:<br>&emsp;&emsp;rd.H[i]=rs1.H[x]<br>&emsp;&emsp;rd.L[x]=NUP(rs1.L[x], rs2[x])<br>else:<br>&emsp;&emsp;rd[x]=NUP(rs1[x], rs2[x])<br>(x=3..0)|
|2|TDR rd, rs1, rs2|ʱ����������ָ��|rd[x]=rs1.H[x]-rs2.H[x]|
|3|BPO rd, rs1, rs2|���򴫲���������������ָ��|if rs2[x]=0:<br>&emsp;&emsp;rd[x]=rs1[x]==1?-1:0<br>else if rs2[x]=1:<br>&emsp;&emsp;rd[x]=rs1[x]==1?1:0<br>(x=3..0)|
|4|EXP rd,rs1|ָ����������ָ��|rd[x]=exp(rs1[x])|
|5|SUM16 rd, rs1, rs2|16�����ۼ�ָ��|if `acc_flag`=1:<br>for x in [0,3]:<br>&emsp;&emsp;rd += rs2[x]==1?rs1[x]:0<br>else:<br>for x in [0,3]:<br>&emsp;&emsp;tmp += rs2[x]==1?rs1[x]:0<br>&emsp;&emsp;rd = tmp + `sum_accumulate`<br>(x=3..0)|
|6|SVR rd, rs1, rs2|����������ָ��|����SNNU�ڲ�SRF�Ĵ�����:<br>$V_{reset}$=rs1[0];<br>$\mu$=rs1[1];<br>$\tau$=rs1[2];<br>`sum_accumulate`=rs2|

## ָ��ϸ��

### ��Ԫ����ָ��

��Ԫ����ָ��(Neuron Update)�У�`rs1`ָ������4��16 bit��Ԫ�ļĴ�����`rs2`ָ������4����Ԫ����ļĴ�����`ts_flag`ָʾ��Ԫ�����Ƿ����ʱ������ݡ�

| |`ts_flag`=1|`ts_flag`=0|
|:-- |:---|:---|
|rs1[15:8]|`ʱ�������[7:0]`|`��ԪĤ��λ[15:8]`|
|rs1[7:0]|`��ԪĤ��λ[7:0]`|`��ԪĤ��λ[7:0]`|

��Ԫ����ָ��ʹ�����¹�ʽ����LIFģ�͵ĸ��£�

$$
V_{next}=V+dV=V-(V>>\tau)+((V_{rest}+\sum{wS})>>\tau)
$$

����$V$Ϊ��ǰĤ��λ��$\tau$ΪLIF��Ԫʱ�䳣����$\sum{wS}$Ϊ�����������䴫��ͻ��Ȩ�صĳ˻�֮�͡�

�����ӿ�����:

```c
us16x4_t __rv_nup(us16x4_t rs1, us16x4_t rs2, uint8_t hasTs);
```

---

### ʱ����������ָ��

ʱ����������ָ��(Time Dependence Rule)�У�rs1��rs2�ֱ�ָ���˴��д�ʱ�����4����Ԫ���ݣ�TDRָ��ֻ�����ʱ����Ĳ�ֵ����ԪĤ��λ���������㡣

�����ӿ�����:

```c
us16x4_t  __rv_tdr(us16x4_t rs1, us16x4_t rs2);
```

---

### ���򴫲���������������ָ��

���򴫲���������������ָ��(Back-Propagation Output direction parameter)�������¹�ʽ���������$\xi_o$������⣺

 $$
 \xi_o=
 \begin{cases}
 1& \text{��Ŀ����Ԫ��[t-4,t]ʱ���ڲ�����}\\
 -1& \text{����Ŀ����Ԫ��[t-4,t]ʱ���ڷ���}\\
 0& \text{�������}
 \end{cases}
 $$

��ָ���У�rs1ָ��4����Ԫ�ķ��Ž����rs2ָ��Ŀ����Ԫ��rs2[x]=1��ʾ��ǰ��ΪĿ����Ԫ����

�����ӿ�����:

```c
us16x4_t __rv_bpo(us16x4_t rs1, us16x4_t rs2);
```

---

### ָ����������ָ��

ָ����������ָ��(EXPonential function calculation)֧��ͬʱ����4��16���صĶ�������Ӧ��ָ������ֵ��Ŀǰ������ΧΪ$\theta\in[-2.0794,2.0794]$,Ҳ����˵���������Χ�������ָ������ֵ��������ġ�

|16���ض�����`n`|`n[15]`|`n[14:11]`|`n[10:0]`|
|:-:|:-:|:-:|:-:|
|����|����λ|����λ|С��λ|

�����ӿ�����:

```c
us16x4_t __rv_exp(us16x4_t rs1);
```

---

### 16���������ָ��

16���������ָ��(SUM16)�У�`acc_flag`����ָʾSRF�Ĵ����е��ۼӼĴ����Ƿ������㣬��`acc_flag=1`����뵽�ۼӼ����У����򲻲����ۼӡ����ָ���������һ��64λ����

�����ӿ�����:

```c
us16x4_t __rv_exp(us16x4_t rs1);
```

---

### �����������ָ��

�����������ָ��(Set Reset Value)�У�`rs1`ָ����������Ԫ��ʼĤ��λ$V_{rest}$��ʱ�䳣��$\tau$�Լ�ͻ��������ѧϰ��$\mu$��

|`rs1`|`rs1[0]`|`rs1[1]`|`rs1[2]`|`rs1[3]`|
|:-:|:-:|:-:|:-:|:-:|
|Ԫ��|$V_{rest}$|$\mu$|$\tau$|reverse|

---
## ���ָ��

### ָ�������

����SNN��չ�������ṩ�˺���ʽ���������ӿڡ�ͨ����¡[AM�ֿ�](https://github.com/OSCPU/nexus-am)�ֿ⣬��`apps/`Ŀ¼���½���Ĺ��̣��ڳ���Դ�����м���`snn.h`������Ϳ���ͨ���������õķ�ʽ������չָ�

[ָ��ϸ��](#ָ��ϸ��)һ����������ڽ������ӿڶ�������`snn.h`�У�����Ը������´���ʵ��`snn.h`

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

### ����

�ⲿ�ֽ���ָ��ܲ��Գ����е���Ԫ���²���

```c
// NUP test
    #if !__DEBUG__
    uint32_t n = 100;
    #endif
    #if __NUP_TEST__
    printf("Neural without Time Stamp Intr test\n");
    us16x4_t taulrvr = {0, 0, 4, 0};
    uint32_t tau = 4;
    __rv_svr(taulrvr, 0); // ����������� rs1:{0:vr, 1:lr, 2:tau, 3:����ȫ0} rs2:0
    us16x4_t nu_without_ts = {0, 0, 0, 0}; 
    us16x4_t s_without_ts = {100, 100, 100, 100};
    uint16_t nu_without_ts_ref[4] = {0, 0, 0, 0};
    for(int i = 0; i < n; i++){
        nu_without_ts = __rv_nup(nu_without_ts, s_without_ts, 0); // NUP ָ����һ������Ϊts_flag
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
        nu_with_ts = __rv_nup(nu_with_ts, s_with_ts, 1); // NUP ָ�ts_flag = 1
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

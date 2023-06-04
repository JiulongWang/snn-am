# RV-SNN2指令集扩展说明

## 目录
[TOC]

## 简介

RV-SNN2扩展指令是基于RISC-V架构双发射顺序处理器[Polaris](https://github.com/ByeBeihai/Polaris)实现的。在这个通用处理器中，我们加入了一个可配置的脉冲神经网络加速模块来实现RV-SNN2扩展指令。

RV-SNN2主要有6条指令，分为三类：

- 神经元更新指令：NUP
- 突触可塑性与通用计算指令：TDR BPO EXP SUM16
- 神经网络配置指令：SVR

|No.|助记符|描述|操作|
|:-:|:--|:--|:--|
|1|NUP rd, rs1, rs2|16比特神经元更新|if ts_flag=1:<br>&emsp;&emsp;rd.H[i]=rs1.H[x]<br>&emsp;&emsp;rd.L[x]=NUP(rs1.L[x], rs2[x])<br>else:<br>&emsp;&emsp;rd[x]=NUP(rs1[x], rs2[x])<br>(x=3..0)|
|2|TDR rd, rs1, rs2|时间依赖规则指令|rd[x]=rs1.H[x]-rs2.H[x]|
|3|BPO rd, rs1, rs2|反向传播输出方向参数计算指令|if rs2[x]=0:<br>&emsp;&emsp;rd[x]=rs1[x]==1?-1:0<br>else if rs2[x]=1:<br>&emsp;&emsp;rd[x]=rs1[x]==1?1:0<br>(x=3..0)|
|4|EXP rd,rs1|指数函数运算指令|rd[x]=exp(rs1[x])|
|5|SUM16 rd, rs1, rs2|16比特累加指令|if `acc_flag`=1:<br>for x in [0,3]:<br>&emsp;&emsp;rd += rs2[x]==1?rs1[x]:0<br>else:<br>for x in [0,3]:<br>&emsp;&emsp;tmp += rs2[x]==1?rs1[x]:0<br>&emsp;&emsp;rd = tmp + `sum_accumulate`<br>(x=3..0)|
|6|SVR rd, rs1, rs2|神经网络配置指令|配置SNNU内部SRF寄存器组:<br>$V_{reset}$=rs1[0];<br>$\mu$=rs1[1];<br>$\tau$=rs1[2];<br>`sum_accumulate`=rs2|

## 指令细节

### 神经元更新指令

神经元更新指令(Neuron Update)中，`rs1`指定存有4个16 bit神经元的寄存器，`rs2`指定存有4个神经元输入的寄存器，`ts_flag`指示神经元数据是否带有时间戳数据。

| |`ts_flag`=1|`ts_flag`=0|
|:-- |:---|:---|
|rs1[15:8]|`时间戳数据[7:0]`|`神经元膜电位[15:8]`|
|rs1[7:0]|`神经元膜电位[7:0]`|`神经元膜电位[7:0]`|

神经元更新指令使用如下公式进行LIF模型的更新：

$$
V_{next}=V+dV=V-(V>>\tau)+((V_{rest}+\sum{wS})>>\tau)
$$

其中$V$为当前膜电位，$\tau$为LIF神经元时间常数，$\sum{wS}$为输入脉冲与其传递突触权重的乘积之和。

函数接口如下:

```c
us16x4_t __rv_nup(us16x4_t rs1, us16x4_t rs2, uint8_t hasTs);
```

---

### 时间依赖规则指令

时间依赖规则指令(Time Dependence Rule)中，rs1、rs2分别指定了存有带时间戳的4个神经元数据，TDR指令只会计算时间戳的差值，神经元膜电位不会参与计算。

函数接口如下:

```c
us16x4_t  __rv_tdr(us16x4_t rs1, us16x4_t rs2);
```

---

### 反向传播输出方向参数计算指令

反向传播输出方向参数计算指令(Back-Propagation Output direction parameter)根据如下公式对输出参数$\xi_o$进行求解：

 $$
 \xi_o=
 \begin{cases}
 1& \text{当目标神经元在[t-4,t]时间内不发放}\\
 -1& \text{当非目标神经元在[t-4,t]时间内发放}\\
 0& \text{其他情况}
 \end{cases}
 $$

在指令中，rs1指定4个神经元的发放结果，rs2指定目标神经元（rs2[x]=1表示当前神经为目标神经元）。

函数接口如下:

```c
us16x4_t __rv_bpo(us16x4_t rs1, us16x4_t rs2);
```

---

### 指数函数计算指令

指数函数计算指令(EXPonential function calculation)支持同时计算4个16比特的定点数对应的指数函数值。目前收敛域范围为$\theta\in[-2.0794,2.0794]$,也就是说，在这个范围内求出的指数函数值是有意义的。

|16比特定点数`n`|`n[15]`|`n[14:11]`|`n[10:0]`|
|:-:|:-:|:-:|:-:|
|描述|符号位|整数位|小数位|

函数接口如下:

```c
us16x4_t __rv_exp(us16x4_t rs1);
```

---

### 16比特数求和指令

16比特数求和指令(SUM16)中，`acc_flag`用于指示SRF寄存器中的累加寄存器是否参与计算，若`acc_flag=1`则参与到累加计算中，否则不参与累加。最后指令输出的是一个64位数。

函数接口如下:

```c
us16x4_t __rv_exp(us16x4_t rs1);
```

---

### 网络参数配置指令

网络参数配置指令(Set Reset Value)中，`rs1`指定包含了神经元初始膜电位$V_{rest}$、时间常数$\tau$以及突触可塑性学习率$\mu$。

|`rs1`|`rs1[0]`|`rs1[1]`|`rs1[2]`|`rs1[3]`|
|:-:|:-:|:-:|:-:|:-:|
|元素|$V_{rest}$|$\mu$|$\tau$|reverse|

---
## 编程指导

### 指令的引入

对于SNN扩展，我们提供了函数式的内联汇编接口。通过克隆[AM仓库](https://github.com/OSCPU/nexus-am)仓库，在`apps/`目录下新建你的工程，在程序源代码中加入`snn.h`的引入就可以通过函数调用的方式调用扩展指令。

[指令细节](#指令细节)一章中提出的内建函数接口都定义在`snn.h`中，你可以复制以下代码实现`snn.h`

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

### 例子

这部分截于指令功能测试程序中的神经元更新部分

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

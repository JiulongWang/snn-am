# The Spiking Neural Network Unit in Wenquxing23

## 0 Introduction

Wenquxing23 is a low power consumption SNN processor which is integrated with an
 SNN accelerating module to enable the SNN training with back-propagation.
 The baseline of Wenquxing23 is [`Polaris`](https://github.com/ByeBeihai/Polaris).
 In this document it will introduce the SNN Unit of Wenquxing23 in detail.

---

## 1 SNN Instruction Extensions

Please Check [this document](./InstrInfo.md) for detail.

---

## 2 Overall View of SNN Unit

The Spiking Neural Network Unit (SNNU) is integrated into the pipeline of `Polaris`
 Processor as a sub-component with a configurable number of issues.
 This component has a two-stage pipeline: Issue stage and Executive stage.

SNNU includes three parts:

- SNN Issue unit (SNNISU) for the re-decoding of SNN instructions;
- LIF Neuron Unit (LNU) for SIMD sum and updating the LIF-module neuron according to the
 formula of Leaky Integrate-and-fire (LIF) Module;
- Synapse Unit (SU) for synaptic plasticity and common function computing,
 including exponential function.

---

## 3 SNN Issue Unit

The SNN Issue Unit (SNNISU) decodes RVSNN instructions and reasonably
 sends instructions to next stage.
 The operand is divided into 4 16-bit data for SIMD computing in SNNISU.

An SNN register file (SRF) is integrated into SNNISU for temporarily
 storing some useful parameters.
 The data in SRF will not participate in the computing of other components,
 which means, in other words, the data of SRF is only valid in SNNU.

---

## 4 LIF Neural Unit

A LIF Neuron Unit can update 4 LIF neurons at the same time.
 It accept the operands (the divided data) and operator from previous stage.
 The neuron update follows the LIF simplified formula:

$$
V_{next}=V+dV=V-(V>>\tau)+((V_{rest}+\sum{wS})>>\tau)
$$

where $V_{next}$ is the next membrane potential;
 $V$ is current membrane potential from 16-bit operands;
 $V_{rest}$ is reset membrane potential after neuron reaching the threshold voltage
 from SRF;
 $\sum{wS}$ is input stimulation from 16-bit operands;
 $\tau$ is the time constant for LIF modle fetched from SRF.

There are two structures for membrane potential $V$: with or without Time Stamp.
 All 16 bits of LIF without time stamp represent the membrane potential;
 The upper 8 bits of LIF with time stamp represent the time stamp of this neuron,
 and lower 8 bits stand for membrane potential.
 The time stamp stores the spiking time of neurons.
 It is one of the important parameters in Spike-Timing Dependent Plasticity (STDP).

The LNU can handle these two structures, which can be configured by setting the
`ts_flag` to `1`.

---

## 5 Synapse Unit

The synapse unit (SU) mainly handles the synaptic plasticity and exponential
function computing.
SU contains 3 parts:

- Back-Propagation Output direction parameter computing (BPO computing);
- Time Dependence Rule computing (TDR computing);
- EXPonential function computing (EXP computing).

SU achieves the BPO computing according to the following formula:

 $$
 \xi_o=
 \begin{cases}
 1& \text{, when target neuron fires in [t-4,t]}\\
 -1& \text{,when non-target neuron fires in [t-4,t]}\\
 0& \text{, others}
 \end{cases}
 $$

where $\xi_o$ is the output parameter of Back-Propagation STDP (BP-STDP).

The TDR computing aims to calculate the difference of time stamps ($\Delta t$) between
 different neurons. $\Delta t$ is used for forward STDP training.

The EXP computing is realized by using the CORDIC algorithm
 using 16-bit fixed-point number.
 The region of convergence is also extended, from (-1.1182, 1.1182) to (-2.079, 2.079).

## 6 Configure SNNU in Wenquxing23

Both SNNU and SNN instruction extensions can be configured
 by changing the `setting.scala` chisel file.
 Setting the parameter `Polaris_SNN_WAY_NUM` can configure the SNNU:

- when `Polaris_SNN_WAY_NUM = 0`, SNNU will not be generated;
- when `Polaris_SNN_WAY_NUM = 1`, the project will generate one-way SNNU which
 only handles one instruction once;
- when `Polaris_SNN_WAY_NUM = 2`, the project will generate two-way SNNU which
 handles two instructions once.

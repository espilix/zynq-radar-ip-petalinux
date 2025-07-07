# Overview

## Functionalities
1. Pulse radar functionality
2. Basic DSP for signal processing
3. Moving object detection
4. Velocity estimation using Doppler effect

## Pulse Radar IP & DSP
1. Pulse Radar IP Components:
   - Pulse generator (TX)
   - ADC interface (RX)
   - Range processing (FFT/matched filter)
   - Doppler processing (FFT across pulses)
   - CFAR (Constant False Alarm Rate) detection
   - Target detection and velocity estimation

2. DSP Components:
   - Windowing
   - FFT processing
   - Moving Target Indicator (MTI)
   - Doppler processing
   - Peak detection

---

# **Radar IP Design: Pulse Radar with Doppler Processing**

## **Radar IP Architecture Overview**

```
┌─────────────────────────────────────────────────────────────────┐
│                    PULSE RADAR IP                               │
├─────────────────────────────────────────────────────────────────┤
│  TX PATH:                                                       │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐              │
│  │ Pulse Gen   │→ │ Waveform    │→ │ DAC         │→ TX_OUT      │
│  │ (PRF Timer) │  │ Generator   │  │ Interface   │              │
│  └─────────────┘  └─────────────┘  └─────────────┘              │
│                                                                 │
│  RX PATH:                                                       │
│  RX_IN → ┌─────────────┐  ┌─────────────┐  ┌─────────────┐      │
│          │ ADC         │→ │ Range       │→ │ Doppler     │      │
│          │ Interface   │  │ Processing  │  │ Processing  │      │
│          └─────────────┘  └─────────────┘  └─────────────┘      │
│                                ↓                ↓               │
│                          ┌─────────────┐  ┌─────────────┐       │
│                          │ MTI Filter  │  │ CFAR        │       │
│                          │ (Clutter    │  │ Detection   │       │
│                          │ Rejection)  │  │             │       │
│                          └─────────────┘  └─────────────┘       │
│                                                ↓                │
│                                        ┌─────────────┐          │
│                                        │ Target      │          │
│                                        │ Detection   │          │
│                                        │ & Velocity  │          │
│                                        └─────────────┘          │
├─────────────────────────────────────────────────────────────────┤
│  CONTROL & DATA:                                                │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐              │
│  │ AXI4-Lite   │  │ AXI4-Stream │  │ Memory      │              │
│  │ Control     │  │ Data Out    │  │ Controller  │              │
│  └─────────────┘  └─────────────┘  └─────────────┘              │
└─────────────────────────────────────────────────────────────────┘
```

---

## **Radar IP Specifications**

### **Radar Parameters**
- **PRF (Pulse Repetition Frequency):** 1 kHz - 10 kHz
- **Pulse Width:** 1 μs - 100 μs
- **Range Resolution:** 1.5 m (10 MHz bandwidth)
- **Maximum Range:** 15 km
- **Doppler Resolution:** Configurable based on CPI (Coherent Processing Interval)
- **Velocity Range:** ±200 m/s (assuming 10 GHz carrier)

### **DSP Chain**
1. **Range Processing:** FFT-based matched filtering
2. **MTI (Moving Target Indicator):** 2-pulse canceller
3. **Doppler Processing:** FFT across pulses
4. **CFAR Detection:** Cell-Averaging CFAR
5. **Target Extraction:** Peak detection and velocity estimation

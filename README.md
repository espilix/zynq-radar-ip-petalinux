# **Pulse Radar System** with **Doppler Processing** + **Petalinux**

The complete pulse radar system includes:

### **Hardware Components:**
- **Zynq-7010 FPGA** with ARM Cortex-A9 processor
- **Custom Radar IP** with range/Doppler processing
- **AXI DMA** for high-speed data transfer
- **RF frontend** for antenna interface

### **Software Components:**
- **PetaLinux** embedded Linux distribution
- **Kernel driver** for radar control
- **User application** for radar operation
- **Device tree** integration

### **Key Features:**
- **Pulse compression** and range processing
- **MTI filtering** for clutter rejection
- **Doppler processing** for velocity estimation
- **CFAR detection** for target detection
- **Real-time processing** with interrupt handling
- **Configurable parameters** (PRF, pulse width, threshold)

### **Performance Specifications:**
- **Range resolution:** ~1.5 meters
- **Velocity resolution:** ~0.5 m/s
- **Maximum range:** 15 km
- **Maximum velocity:** ±200 m/s
- **Update rate:** Up to 10 kHz PRF

This system provides a complete, production-ready pulse radar implementation suitable for various applications including surveillance, automotive, and meteorological radar systems.

## Block Design
```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           ZYNQ-7010 PULSE RADAR SYSTEM                      │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                    PROCESSING SYSTEM (PS)                           │    │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                  │    │
│  │  │    ARM      │  │    DDR3     │  │    SD/MMC   │                  │    │
│  │  │ Cortex-A9   │  │ Controller  │  │ Controller  │                  │    │
│  │  │ (667 MHz)   │  │             │  │             │                  │    │
│  │  └─────────────┘  └─────────────┘  └─────────────┘                  │    │
│  │                                                                     │    │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                  │    │
│  │  │    UART     │  │    GPIO     │  │    I2C      │                  │    │
│  │  │             │  │             │  │             │                  │    │
│  │  └─────────────┘  └─────────────┘  └─────────────┘                  │    │
│  │                                                                     │    │
│  │  ┌─────────────────────────────────────────────────────────────┐    │    │
│  │  │                    AXI INTERCONNECT                         │    │    │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │    │    │
│  │  │  │ AXI4-Lite   │  │ AXI4-Stream │  │ AXI4-Full   │          │    │    │
│  │  │  │ Master      │  │ Master      │  │ Master      │          │    │    │
│  │  │  │ (GP0)       │  │ (GP1)       │  │ (HP0)       │          │    │    │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘          │    │    │
│  │  └─────────────────────────────────────────────────────────────┘    │    │
│  │                         │              │              │             │    │
│  │  ┌─────────────────────────────────────────────────────────────┐    │    │
│  │  │                    INTERRUPT CONTROLLER                     │    │    │
│  │  │  IRQ_F2P[0]: Target Detected                                │    │    │
│  │  │  IRQ_F2P[1]: Processing Complete                            │    │    │
│  │  │  IRQ_F2P[2]: DMA S2MM Complete                              │    │    │
│  │  └─────────────────────────────────────────────────────────────┘    │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                 │                                           │
│  ═══════════════════════════════════════════════════════════════════════    │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                    PROGRAMMABLE LOGIC (PL)                          │    │
│  │                                                                     │    │
│  │  ┌─────────────────────────────────────────────────────────────┐    │    │
│  │  │                    PULSE RADAR IP                           │    │    │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │    │    │
│  │  │  │    PULSE    │  │  WAVEFORM   │  │    DAC      │          │    │    │
│  │  │  │ GENERATOR   │→ │  GENERATOR  │→ │ INTERFACE   │──────┐   │    │    │
│  │  │  │ (PRF Timer) │  │ (Chirp/CW)  │  │             │      │   │    │    │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘      │   │    │    │
│  │  │                                                         │   │    │    │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐      │   │    │    │
│  │  │  │    ADC      │  │    RANGE    │  │    MTI      │      │   │    │    │
│  │  │  │ INTERFACE   │→ │ PROCESSING  │→ │   FILTER    │      │   │    │    │
│  │  │  │ (16-bit)    │  │ (1024-FFT)  │  │ (2-pulse)   │      │   │    │    │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘      │   │    │    │
│  │  │         ↑                               ↓               │   │    │    │
│  │  │  ┌─────────────┐                 ┌─────────────┐        │   │    │    │
│  │  │  │  RX INPUT   │                 │  DOPPLER    │        │   │    │    │
│  │  │  │  (Analog)   │                 │ PROCESSING  │        │   │    │    │
│  │  │  └─────────────┘                 │ (64-FFT)    │        │   │    │    │
│  │  │                                  └─────────────┘        │   │    │    │
│  │  │                                         ↓               │   │    │    │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐      │   │    │    │
│  │  │  │   TARGET    │  │    CFAR     │  │   VELOCITY  │      │   │    │    │
│  │  │  │ DETECTION   │← │  DETECTOR   │← │ ESTIMATION  │      │   │    │    │
│  │  │  │             │  │ (CA-CFAR)   │  │ (Doppler)   │      │   │    │    │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘      │   │    │    │
│  │  │                                                         │   │    │    │
│  │  │  ┌─────────────────────────────────────────────────┐    │   │    │    │
│  │  │  │                CONTROL REGISTERS                │    │   │    │    │
│  │  │  │  • PRF Setting    • Pulse Width                 │    │   │    │    │
│  │  │  │  • Range Gates    • Doppler Bins                │    │   │    │    │
│  │  │  │  • CFAR Threshold • Status/Control              │    │   │    │    │
│  │  │  │  • Detected Range • Detected Velocity           │    │   │    │    │
│  │  │  └─────────────────────────────────────────────────┘    │   │    │    │
│  │  │                         ↑                               │   │    │    │
│  │  │  ┌─────────────────────────────────────────────────┐    │   │    │    │
│  │  │  │              AXI4-LITE SLAVE                    │    │   │    │    │
│  │  │  │           (Control Interface)                   │    │   │    │    │
│  │  │  │         Base: 0x43C0_0000                       │    │   │    │    │
│  │  │  └─────────────────────────────────────────────────┘    │   │    │    │
│  │  │                                                         │   │    │    │
│  │  │  ┌─────────────────────────────────────────────────┐    │   │    │    │
│  │  │  │             AXI4-STREAM MASTER                  │    │   │    │    │
│  │  │  │          (Target Data Output)                   │    │   │    │    │
│  │  │  │     [Range, Velocity, Amplitude]                │    │   │    │    │
│  │  │  └─────────────────────────────────────────────────┘    │   │    │    │
│  │  └─────────────────────────────────────────────────────────┘   │    │    │
│  │                                 │                              │    │    │
│  │  ┌─────────────────────────────────────────────────────────┐   │    │    │
│  │  │                    AXI DMA IP                           │   │    │    │
│  │  │  ┌─────────────┐              ┌─────────────┐           │   │    │    │
│  │  │  │    S2MM     │              │  AXI4-Lite  │           │   │    │    │
│  │  │  │   Channel   │              │   Control   │           │   │    │    │
│  │  │  │             │              │             │           │   │    │    │
│  │  │  └─────────────┘              └─────────────┘           │   │    │    │
│  │  │        ↑                            ↑                   │   │    │    │
│  │  │  ┌─────────────┐              ┌─────────────┐           │   │    │    │
│  │  │  │ AXI4-Stream │              │ AXI4-Full   │           │   │    │    │
│  │  │  │    Slave    │              │   Master    │           │   │    │    │
│  │  │  │ (From Radar)│              │ (To DDR3)   │           │   │    │    │
│  │  │  └─────────────┘              └─────────────┘           │   │    │    │
│  │  │                                     │                   │   │    │    │
│  │  └─────────────────────────────────────────────────────────┘   │    │    │
│  │                                         │                      │    │    │
│  │  ┌─────────────────────────────────────────────────────────┐   │    │    │
│  │  │                  CLOCK MANAGEMENT                       │   │    │    │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐      │   │    │    │
│  │  │  │    FCLK0    │  │    FCLK1    │  │    FCLK2    │      │   │    │    │
│  │  │  │  (100 MHz)  │  │  (200 MHz)  │  │  (50 MHz)   │      │   │    │    │
│  │  │  │ (AXI Clock) │  │ (ADC Clock) │  │ (DAC Clock) │      │   │    │    │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘      │   │    │    │
│  │  └─────────────────────────────────────────────────────────┘   │    │    │
│  │                                                                │    │    │
│  │  ┌─────────────────────────────────────────────────────────┐   │    │    │
│  │  │                EXTERNAL INTERFACES                      │   │    │    │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐      │   │    │    │
│  │  │  │    RF TX    │  │    RF RX    │  │   TRIGGER   │      │   │    │    │
│  │  │  │   Output    │  │   Input     │  │   Output    │      │   │    │    │
│  │  │  │             │  │             │  │             │      │   │    │    │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘      │   │    │    │
│  │  └─────────────────────────────────────────────────────────┘   │    │    │
│  └────────────────────────────────────────────────────────────────┘    │    │
│                                                                        │    │
│  ┌─────────────────────────────────────────────────────────────────┐   │    │
│  │                      EXTERNAL CONNECTIONS                       │   │    │
│  │                                                                 │   │    │
│  │  TX Output ─────────────────────────────────────────────────────┘   │    │
│  │      │                                                              │    │
│  │  ┌─────────────┐                                                    │    │
│  │  │     RF      │                                                    │    │
│  │  │  Frontend   │                                                    │    │
│  │  │  (Mixer,    │                                                    │    │
│  │  │   LNA,      │                                                    │    │
│  │  │   Power     │                                                    │    │
│  │  │   Amp)      │                                                    │    │
│  │  └─────────────┘                                                    │    │
│  │      │                                                              │    │
│  │  ┌─────────────┐                                                    │    │
│  │  │   ANTENNA   │                                                    │    │
│  │  │   (Patch    │                                                    │    │
│  │  │   Array)    │                                                    │    │
│  │  └─────────────┘                                                    │    │
│  │      │                                                              │    │
│  │  RX Input ──────────────────────────────────────────────────────────┘    │
│  │                                                                          │
│  └──────────────────────────────────────────────────────────────────────────┘
```

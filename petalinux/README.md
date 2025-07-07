## **Address Map**

| Component      | Base Address | Size | Description              |
|----------------|--------------|------|--------------------------|
| Pulse Radar IP | 0x43C0_0000  | 64KB | Main radar processing IP |
| AXI DMA        | 0x4040_0000  | 64KB | DMA for data transfer    |
| AXI GPIO       | 0x4120_0000  | 64KB | General purpose I/O      |
| AXI Timer      | 0x4200_0000  | 64KB | System timer             |
| AXI UART       | 0x4060_0000  | 64KB | Debug UART               |

---

## **Register Map - Pulse Radar IP**

| Offset | Name              | Access | Description                |
|--------|-------------------|--------|----------------------------|
| 0x00   | CONTROL           | R/W    | Control register           |
| 0x04   | STATUS            | R      | Status register            |
| 0x08   | PRF               | R/W    | Pulse repetition frequency |
| 0x0C   | PULSE_WIDTH       | R/W    | Pulse width setting        |
| 0x10   | RANGE_GATES       | R/W    | Number of range gates      |
| 0x14   | DOPPLER_BINS      | R/W    | Number of Doppler bins     |
| 0x18   | DETECTED_RANGE    | R      | Detected target range      |
| 0x1C   | DETECTED_VELOCITY | R      | Detected target velocity   |
| 0x20   | THRESHOLD         | R/W    | CFAR threshold             |
| 0x24   | TARGET_COUNT      | R      | Number of detected targets |
| 0x28   | PROCESSING_TIME   | R      | Processing time in cycles  |
| 0x2C   | VERSION           | R      | IP version                 |

---

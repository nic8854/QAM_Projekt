# API Documentation
## Component Diagram

```mermaid
flowchart TD
DataProvider -->|uint32_t binary queue| PacketEncoder;
PacketEncoder -->|uint64_t Packet queue| QamModulator;
QamModulator -->|256x uint8_t sine buffer|DacDataRelay;
DacDataRelay --> id1([Wireless Transmitter]);
id2([Wireless Receiver]) --> AdcDataRelay;
AdcDataRelay -->|256x uint8_t sine buffer| QamDemodulator;
QamDemodulator -->|uint8_t binary queue| PacketDecoder;
PacketDecoder -->|uint32_t binary queue| GuiDriver;
```


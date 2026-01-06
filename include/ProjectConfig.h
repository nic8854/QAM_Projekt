// ProjectConfig.h
#pragma once

// ------------------------------ PROGRAM CONFIG ------------------------------ //

// Mode select

//#define QAM_TX_MODE             // Transmitter
//#define QAM_RX_MODE             // Receiver
#define QAM_TRX_MODE            // Transceiver

// Route select
#if defined(QAM_TRX_MODE)

  #define TRX_ROUTE_PACKET      // PacketEncoder -> PacketDecoder (without Modem)
  //#define TRX_ROUTE_MODEM       // Modulator -> Demodulator (digital loopback)
  //#define TRX_ROUTE_FULL        // Modulator -> DAC -> ADC -> Demodulator (full)

#endif
// ---------------------------------------------------------------------------- //
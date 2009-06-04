#include "WProgram.h"
#include "HardwareSerial.h"
#include "./Constants.h"
#include "./XBee.h"

#define DEBUG 0
#define POWER_LEVEL "0"
#define CHANNEL "E"
#define PAN "6666"

namespace {
  byte buffer[MAX_PACKET_SIZE] = {};
  byte wptr = 0;
  byte hostAddress[8] = {};
  bool haveReceived = false;
  
  bool readOK() {
    unsigned long startTime = millis();
    byte buf[3];
    
    while (Serial1.available() < 3 && (millis() - startTime) < 1000);
    while (Serial1.available() > 3) Serial1.read();
    if (Serial1.available() < 3) return false;
    buf[0] = Serial1.read();
    buf[1] = Serial1.read();
    buf[2] = Serial1.read();
    return buf[0] == 'O' && buf[1] == 'K' && buf[2] == '\r';
  }
  
  inline void copy_short(void *dest, const void *src, byte len) {
    char *d = reinterpret_cast<char *>(dest);
    const char *s = reinterpret_cast<const char *>(src);
    while (len--)
      *d++ = *s++;
  }
  
#if DEBUG
#define DBGPRINT(X) Serial.println(X)
#else
#define DBGPRINT(X) do{}while(false)
#endif
}

XBee::RXData XBee::rxdata = {0, 0, 0, 0, 0, 0xFF, -128, -128, -128, -128};
XBee::TXData XBee::txdata = {0xFF, 0xFF, 0xFF, 0xFF};

void XBee::init() {
  DBGPRINT("XBee: Initializing...");
  
  // Switch into command mode by sending +++.
  do {
    delay(1200);
    Serial1.print("+++");
    delay(1000);
  } while (!readOK());
  
  // Reset to factory configuration.
  do {
    Serial1.print("ATRE\r");
  } while (!readOK());
  
  // Switch to non-escaped API mode.
  do {
    Serial1.print("ATAP1\r");
  } while (!readOK());
  
  // Set power level.
  do {
    Serial1.print("ATPL" POWER_LEVEL "\r");
  } while (!readOK());
  
  // Set channel.
  do {
    Serial1.print("ATCH" CHANNEL "\r");
  } while (!readOK());
  
  // Set PAN.
  do {
    Serial1.print("ATID" PAN "\r");
  } while (!readOK());
  
  // Set local address.
  do {
    Serial1.print("ATMYFFFF\r");
  } while (!readOK());
  
  // Switch out of command mode.
  do {
    Serial1.print("ATCN\r");
  } while (!readOK());
    
  DBGPRINT("XBee: OK");
}

bool XBee::receive() {
  bool ret = false;
  
  while (Serial1.available()) {
    buffer[wptr++] = Serial1.read();
    if (wptr == 1) {
      // Check for the delimiter.
      if (buffer[0] != 0x7E)
        wptr = 0;
    } else if (wptr >= 3) {
      // Check for legal length.
      unsigned int len = buffer[1] * 256 + buffer[2];
      if (len > MAX_PACKET_SIZE - 4) {
        wptr = 0;
      } else if (wptr == len + 4) {
        // Packet is finished. Check checksum.
        byte b = 0;
        for (byte i = 0; i <= len; i++)
          b += buffer[3 + i];
        if (b == 0xFF) {
          // Checksum is OK. Check packet type.
          if (buffer[3] == 0x8A && len == 2) {
            // Modem status.
            if (buffer[4] == 0)      DBGPRINT("XBee: hardware reset");
            else if (buffer[4] == 1) DBGPRINT("XBee: WDT reset");
            else if (buffer[4] == 4) DBGPRINT("XBee: synchronization lost");
            else                     DBGPRINT("XBee: unknown modem status");
          } else if (buffer[3] == 0x80) {
            if (len == 11 + sizeof(rxdata)) {
              // Copy packet payload to output structure.
              copy_short(&rxdata, &buffer[14], sizeof(rxdata));
              // Save sender's address.
              copy_short(hostAddress, &buffer[4], 8);
              // Mark that we have ever received a packet.
              haveReceived = true;
              ret = true;
            } else {
              DBGPRINT("XBee: received data of wrong size");
            }
          } else {
            DBGPRINT("XBee: received unknown packet");
          }
        }
        // Discard packet.
        wptr = 0;
      }
    }
  }
  
  return ret;
}

void XBee::send() {
  static byte buffer[15 + sizeof(txdata)];
  
  // If we've never received a packet, just give up (we have nobody to send to).
  if (!haveReceived)
    return;
  
  // Prepare packet.
  buffer[0] = 0x7E;
  buffer[1] = (sizeof(buffer) - 4) / 256;
  buffer[2] = (sizeof(buffer) - 4) % 256;
  buffer[3] = 0x00;
  buffer[4] = 0x00;
  copy_short(&buffer[5], hostAddress, 8);
  buffer[13] = 0x00;
  copy_short(&buffer[14], &txdata, sizeof(txdata));
  
  // Checksum.
  buffer[sizeof(buffer) - 1] = 0;
  for (byte i = 3; i < sizeof(buffer) - 1; i++)
    buffer[sizeof(buffer) - 1] += buffer[i];
  buffer[sizeof(buffer) - 1] = 0xFF - buffer[sizeof(buffer) - 1];
  
  // Transmit!
  for (byte i = 0; i < sizeof(buffer); i++)
    Serial1.print(buffer[i], BYTE);
}

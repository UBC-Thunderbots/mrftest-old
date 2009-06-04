#ifndef XBEE_H
#define XBEE_H

#define MAX_PACKET_SIZE 32

namespace XBee {
  struct RXData {
    char vx;
    char vy;
    char vt;
    byte dribble;
    byte kick;
    byte emergency;
    char vxMeasured;
    char vyMeasured;
    unsigned char reboot;
    char extra;
  } __attribute__((packed));
  
  struct TXData {
    byte vGreen[2];
    byte vMotor[2];
  } __attribute__((packed));
  
  void init();
  bool receive();
  void send();
  extern RXData rxdata;
  extern TXData txdata;
}

#endif

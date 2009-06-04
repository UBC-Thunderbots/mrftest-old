#ifndef XBEE_H
#define XBEE_H

#define MAX_PACKET_SIZE 32

namespace XBee {
  struct RXData {
    char vx;
    char vy;
    char vtheta;
    byte dribble;
    byte kick;
    byte emergency;
    char vxMeasured;
    char vyMeasured;
    char extra1;
    char extra2;
  } __attribute__((packed));
  
  struct TXData {
    byte vGreenHigh;
    byte vGreenLow;
    byte vMotorHigh;
    byte vMotorLow;
  } __attribute__((packed));
  
  void init();
  bool receive();
  void send();
  extern RXData rxdata;
  extern TXData txdata;
}

#endif

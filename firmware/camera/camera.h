#include <stdint.h>
#include <stdbool.h>
#include "i2c.h"

#define CAM_IP 0x21
#ifndef CAMERA_H
#define CAMERA_H

typedef struct
{
	uint8_t reg;
	uint8_t value;
} cam_setting_t;


bool camera_init(cam_setting_t*, unsigned int);
bool camera_write2register(cam_setting_t);
uint8_t camera_read_reg(uint8_t);

#endif

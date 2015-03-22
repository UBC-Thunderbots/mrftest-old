#include "i2c.h"
#include "camera.h"
#include <stdbool.h>
#include <stdint.h>

#define CAM_IP 0x21

bool camera_init (cam_setting_t* setting_list, unsigned int list_size)
{
	if (!i2c_init())
	{
		//ERROR: Failure to initialize I2C interface
		return false;
	}

	// Start write 
	if (!i2c_start_com(CAM_IP, 0x0))
	{
		//ERROR: Failure to start I2C transmission to camera
		return false;
	}	

	// Send all the settings
	volatile unsigned int i = 0;
	for (i = 0; i < list_size; i++)
	{
		camera_write2register(setting_list[i]);
	}

	i2c_stop_tx();

	// Check if all the commands went through
	for (i = 0; i < list_size; i++)
	{
		if (camera_read_reg(setting_list[i].reg) != setting_list[i].value)
			return false;
	}
	return true;
}


//NOTE: this function will never fail because NACK is assumed to represent success
void camera_write2register(cam_setting_t setting)
{
	i2c_write_data(setting.reg);
	i2c_write_data(setting.value);
}


//NOTE: Each read needs a stop condition
uint8_t camera_read_reg(uint8_t reg)
{
	// Write the address to device
	if (!i2c_start_com(CAM_IP, 0x0))
	{
		//ERROR: Failure to start I2C transmission to camera
		return false;
	}
	i2c_write_data(reg);

	// Receive data at address
	if (!i2c_start_com(CAM_IP, 0x1))
	{
		//ERROR: Failure to start receiving from camera
		return false;
	}

	i2c_stop_rx();
	return i2c_read_data();

}


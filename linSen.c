/*
 * linSen.c
 *
 * Author: carsten
 */

#include <stdint.h>	// XintY_t-types
#include <stdlib.h>	// NULL
#include <stdio.h>	//fprintf, printf

#include "linSen.h"
#include "i2c.h"
#include "linSen-socket.h"

linSen_interface_t linSen_access_interface = interface_NONE;
linSen_interface_t linSen_relay_interface = interface_NONE;

int linSen_init(const char* dev, linSen_interface_t interface) {
	if (interface == interface_I2C) {
		linSen_access_interface = interface;
		return i2c_init(dev, LINSEN_I2C_SLAVE_ADDRESS);
	} else if ((interface == interface_SOCKET) && (dev == NULL)) {
		linSen_relay_interface = interface;
		return linSen_socket_server_init();
	} else if ((interface == interface_SOCKET) && (dev != NULL)) {
		linSen_access_interface = interface;
		return linSen_socket_client_init(dev);
	} // else
	return -1;
}

int linSen_close(void) {
	if (linSen_access_interface == interface_I2C) {
		i2c_close();
	} else if (linSen_access_interface == interface_SOCKET) {
		linSen_socket_client_close();
	}
	if (linSen_relay_interface == interface_SOCKET) {
		linSen_socket_server_close();
	}
	return 0;
}

int linSen_process(void) {
	int result;
	
	if (linSen_relay_interface == interface_SOCKET) {
		result = linSen_socket_server_process();
		if (result < 0) return result;
	}
	return 0;
}

int linSen_set_exposure(int value) {
	switch (linSen_access_interface) {
		case interface_I2C: {
			return i2c_write_l(LINSEN_EXP_ADDR, (uint32_t)value);
		}
		case interface_SOCKET: {
			return linSen_socket_client_write(LINSEN_EXP_WRITE_STRING, value);
		}
		default:;
	}
	return -1;
}

int linSen_get_exposure(void) {
	int value = 0;
	int result;

	switch (linSen_access_interface) {
		case interface_I2C: {
			uint32_t _value;
			i2c_read_l(LINSEN_EXP_ADDR, &_value);
			value = _value;
			break;
		}
		case interface_SOCKET: {
			result = linSen_socket_client_read(LINSEN_EXP_READ_STRING, &value);
			if (result < 0) return result;
			break;
		}
		default:;
	}
	return value;
}

int linSen_set_pxclk(int value) {
	switch (linSen_access_interface) {
		case interface_I2C: {
			return i2c_write_w(LINSEN_PIX_CLK_ADDR, (uint16_t)value);
		}
		case interface_SOCKET: {
			return linSen_socket_client_write(LINSEN_PIX_CLK_WRITE_STRING, value);
		}
		default:;
	}
	return -1;
}

int linSen_get_pxclk(void) {
	int value = 0;
	int result;

	switch (linSen_access_interface) {
		case interface_I2C: {
			uint16_t _value;
			i2c_read_w(LINSEN_PIX_CLK_ADDR, &_value);
			value = _value;
			break;
		}
		case interface_SOCKET: {
			result = linSen_socket_client_read(LINSEN_PIX_CLK_READ_STRING, &value);
			if (result < 0) return result;
			break;
		}
		default:;
	}
	return value;
}

int linSen_set_brightness(int value) {
	switch (linSen_access_interface) {
		case interface_I2C: {
			return i2c_write_w(LINSEN_BRIGHT_ADDR, (uint16_t)value);
		}
		case interface_SOCKET: {
			return linSen_socket_client_write(LINSEN_BRIGHT_WRITE_STRING, value);
		}
		default:;
	}
	return -1;
}

int linSen_get_brightness(void) {
	int value = 0;
	int result;

	switch (linSen_access_interface) {
		case interface_I2C: {
			uint16_t _value;
			i2c_read_w(LINSEN_BRIGHT_ADDR, &_value);
			value = _value;
			break;
		}
		case interface_SOCKET: {
			result = linSen_socket_client_read(LINSEN_BRIGHT_READ_STRING, &value);
			if (result < 0) return result;
			break;
		}
		default:;
	}
	return value;
}

int linSen_get_global_result(void) {
	int value = 0;
	int result;

	switch (linSen_access_interface) {
		case interface_I2C: {
			int16_t _value;
			i2c_read_w(LINSEN_GLOBAL_RES_ADDR, (uint16_t*)&_value);
			value = _value;
			break;
		}
		case interface_SOCKET: {
			result = linSen_socket_client_read(LINSEN_RESULT_READ_STRING, &value);
			if (result < 0) return result;
			break;
		}
		default:;
	}
	return value;
}

int linSen_get_result_id(void) {
	int value = 0;
	int result;

	switch (linSen_access_interface) {
		case interface_I2C: {
			int16_t _value;
			i2c_read_w(LINSEN_RES_ID_ADDR, (uint16_t*)&_value);
			value = _value;
			break;
		}
		case interface_SOCKET: {
			result = linSen_socket_client_read(LINSEN_RES_ID_READ_STRING, &value);
			if (result < 0) return result;
			break;
		}
		default:;
	}
	return value;
}

int linSen_get_raw(uint16_t* frame, int size){
	int result = -1;
	
	if (frame) {
		switch (linSen_access_interface) {
			case interface_I2C: {
				return i2c_read_n_w(LINSEN_RAW_ADDR, frame, size);
				break;
			}
			case interface_SOCKET: {
				result = linSen_socket_client_read(LINSEN_RAW_READ_STRING, (int*)frame);
				break;
			}
			default:;
		}
	}
	return result;		
}

int linSen_get_data(linSen_data_t* data){
	int result = -1;
	
	if (data) {
		switch (linSen_access_interface) {
			case interface_I2C: {
				#define BUFFER_SIZE	((LINSEN_GLOBAL_RES_ADDR-LINSEN_EXP_ADDR+2)/2)
				uint16_t buffer[BUFFER_SIZE];
				
				result = i2c_read_n_w(LINSEN_EXP_ADDR, buffer, BUFFER_SIZE);
				if (result) {
					data->exposure = (buffer[1] << 16) | buffer[0];
					data->pixel_clock = buffer[2];
					data->brightness = buffer[3];
					data->pixel_number_x = buffer[4];
					data->pixel_number_y = buffer[5];
					data->result_scalar_number = buffer[6];
					data->result_id = buffer[7];
					data->global_result = (int8_t)buffer[8];
				}
				break;
			}
			case interface_SOCKET: {
				result = linSen_socket_client_read(LINSEN_DATA_READ_STRING, (int*)data);
				break;
			}
			default:;
		}
	}
	return result;		
}

/*
 * linSen.c
 *
 * Last modified: 18.06.2017
 *
 * Author: racnets
 */
#include <stdint.h>	// XintY_t-types
#include <stdlib.h>	// NULL
#include <stdio.h>	//fprintf, printf
#include <string.h> //memcpy

#include "main.h"  // verpose_printf(), debug_printf(), info_printf()

#include "linSen.h"
#include "i2c.h"
#include "linSen-socket.h"

linSen_interface_t linSen_access_interface = interface_NONE;
linSen_interface_t linSen_relay_interface = interface_NONE;


/*
 * initialize sockets
 * 
 * @return EXIT_SUCCESS, EXIT_SUCCESS
 */
int linSen_init(const char* dev, int address, linSen_interface_t interface) {
	int result = EXIT_SUCCESS;

	debug_printf("called");

	if (interface == interface_I2C) {
		linSen_access_interface = interface;
		if (i2c_init(dev, address) < 0) result = EXIT_FAILURE;
	} else if ((interface == interface_SOCKET) && (dev == NULL)) {
		linSen_relay_interface = interface;
		if (linSen_socket_server_init() < 0) result = EXIT_FAILURE;
	} else if ((interface == interface_SOCKET) && (dev != NULL)) {
		linSen_access_interface = interface;
		if (linSen_socket_client_init(dev) < 0) result = EXIT_FAILURE;
	}

	debug_printf("returns: %d", result);		
	return result;
}

/*
 * close sockets
 * 
 * @return EXIT_SUCCESS, EXIT_SUCCESS
 */
int linSen_close(void) {
	int result = EXIT_SUCCESS;

	debug_printf("called");

	if (linSen_access_interface == interface_I2C) {
		if (i2c_close() < 0) result = EXIT_FAILURE;
	} else if (linSen_access_interface == interface_SOCKET) {
		if (linSen_socket_client_close() < 0) result = EXIT_FAILURE;
	}
	if (linSen_relay_interface == interface_SOCKET) {
		if (linSen_socket_server_close() < 0) result = EXIT_FAILURE;
	}

	debug_printf("returns: %d", result);		
	return result;
}

/*
 * linSen process
 * 
 * provides socket server functionality
 * 
 * @return EXIT_SUCCESS, EXIT_SUCCESS
 */
int linSen_process(void) {
	int result = EXIT_SUCCESS;

	debug_printf("called");
	
	if (linSen_relay_interface == interface_SOCKET) {
		result = linSen_socket_server_process();
	}

	debug_printf("returns: %d", result);		
	return result;
}

/*
 * linSen set exposure
 * 
 * @val value: exposure value
 * @return non-negative written size(success), -1 (failure)
 */
int linSen_set_exposure(int value) {
	int result = -1;

	debug_printf("called");
	
	switch (linSen_access_interface) {
		case interface_I2C: {
			result = i2c_write_l(LINSEN_EXP_ADDR, (uint32_t)value);
			break;
		}
		case interface_SOCKET: {
			result = linSen_socket_write_int(LINSEN_EXP_WRITE_STRING, value);
			break;
		}
		default:;
	}
	
	debug_printf("returns: %d", result);		
	return result;
}

/*
 * linSen get brightness 
 * 
 * @return non-negative exposure value(success), -1 (failure)
 */
int linSen_get_exposure(void) {
	int result = -1;

	debug_printf("called");

	switch (linSen_access_interface) {
		case interface_I2C: {
			uint32_t _value;
			if (i2c_read_l(LINSEN_EXP_ADDR, &_value) == 4) result = _value;
			break;
		}
		case interface_SOCKET: {
			int _value;
			if (linSen_socket_read(LINSEN_EXP_READ_STRING, &_value, sizeof(uint32_t)) == EXIT_SUCCESS) result = _value;			
			break;
		}
		default:;
	}
	
	debug_printf("returns: %d", result);		
	return result;
}

/*
 * linSen set pixel clock
 * 
 * @val value: pixel clock value
 * @return non-negative written size(success), -1 (failure)
 */
int linSen_set_pxclk(int value) {
	int result = -1;

	debug_printf("called");
	
	switch (linSen_access_interface) {
		case interface_I2C: {
			result = i2c_write_w(LINSEN_PIX_CLK_ADDR, (uint16_t)value);
			break;
		}
		case interface_SOCKET: {
			result = linSen_socket_write_int(LINSEN_PIX_CLK_WRITE_STRING, value);
			break;
		}
		default:;
	}

	debug_printf("returns: %d", result);		
	return result;
}

/*
 * linSen get brightness 
 * 
 * @return non-negative pixel clock value(success), -1 (failure)
 */
int linSen_get_pxclk(void) {
	int result = -1;

	debug_printf("called");

	switch (linSen_access_interface) {
		case interface_I2C: {
			uint16_t _value;
			if (i2c_read_w(LINSEN_PIX_CLK_ADDR, &_value) == 2) result = _value;
			break;
		}
		case interface_SOCKET: {
			int _value;
			if (linSen_socket_read(LINSEN_PIX_CLK_READ_STRING, &_value, sizeof(uint16_t)) == EXIT_SUCCESS) result = _value;
			break;
		}
		default:;
	}

	debug_printf("returns: %d", result);		
	return result;
}


/*
 * linSen set brightness
 * 
 * @val value: brightness value
 * @return non-negative written size(success), -1 (failure)
 */
int linSen_set_brightness(int value) {
	int result = -1;

	debug_printf("called");

	switch (linSen_access_interface) {
		case interface_I2C: {
			result = i2c_write_w(LINSEN_BRIGHT_ADDR, (uint16_t)value);
			break;
		}
		case interface_SOCKET: {
			result = linSen_socket_write_int(LINSEN_BRIGHT_WRITE_STRING, value);
			break;
		}
		default:;
	}

	debug_printf("returns: %d", result);		
	return result;
}

/*
 * linSen get brightness 
 * 
 * @return non-negative brightness value(success), -1 (failure)
 */
int linSen_get_brightness(void) {
	int result = -1;
	debug_printf("called");

	switch (linSen_access_interface) {
		case interface_I2C: {
			uint16_t _value;
			if (i2c_read_w(LINSEN_BRIGHT_ADDR, &_value) == 2) result = _value;
			break;
		}
		case interface_SOCKET: {
			int _value;
			if (linSen_socket_read(LINSEN_BRIGHT_READ_STRING, &_value, sizeof(uint16_t)) == EXIT_SUCCESS) result = _value;
			break;
		}
		default:;
	}

	debug_printf("returns: %d", result);		
	return result;
}


/*
 * linSen get global result
 * @val *error: 1 if error occured
 * @return result value
 */
int linSen_get_global_result(int *error) {
	int result = 0;

	debug_printf("called");
	
	if (error != NULL) *error = 0;	

	switch (linSen_access_interface) {
		case interface_I2C: {
			int16_t _value;
			if (i2c_read_w(LINSEN_GLOBAL_RES_ADDR, (uint16_t*)&_value) == 2) result = _value;
			else {
				if (error != NULL) *error = 1;
			}
			break;
		}
		case interface_SOCKET: {
			int _value;
			if (linSen_socket_read(LINSEN_RESULT_READ_STRING, &_value, sizeof(int16_t)) == EXIT_SUCCESS) result = _value;
			else {
				if (error != NULL) *error = 1;
			}
			break;
		}
		default:;
	}

	debug_printf("returns: %d", result);		
	return result;
}

/*
 * linSen get result identifier
 * 
 * @return non-negative result identifier(success), -1 (failure)
 */
int linSen_get_result_id(void) {
	int result = -1;

	debug_printf("called");

	switch (linSen_access_interface) {
		case interface_I2C: {
			int16_t _value;
			if (i2c_read_w(LINSEN_RES_ID_ADDR, (uint16_t*)&_value) == 2) result = _value;
			break;
		}
		case interface_SOCKET: {
			int _value;
			if (linSen_socket_read(LINSEN_RES_ID_READ_STRING, &_value, sizeof(int16_t)) == EXIT_SUCCESS) result = _value;
			break;
		}
		default:;
	}

	debug_printf("returns: %d", result);		
	return result;
}

/*
 * linSen get raw sensor data
 * 
 * @param *frame: pointer to frame
 * @param size: size of frame (in words!!!)
 * @return non-negative size(success), -1 (failure)
 */
int linSen_get_raw(uint16_t* frame, int size){
	int result = -1;
	debug_printf("called");

	if (frame == NULL) return result;
	
	switch (linSen_access_interface) {
		case interface_I2C: {
			result = i2c_read_n_w(LINSEN_RAW_ADDR, frame, size);
			break;
		}
		case interface_SOCKET: {
			if (linSen_socket_read(LINSEN_RAW_READ_STRING, (int*)frame, size*sizeof(uint16_t)) == EXIT_SUCCESS) result = size*sizeof(uint16_t);
			break;
		}
		default:;
	}

	debug_printf("returns: %d", result);		
	return result;
}

/*
 * linSen get data values
 * 
 * @param *data:
 * @return non-negative size(success), -1 (failure)
 */
int linSen_get_data(linSen_data_t* data){
	int result = -1;
	debug_printf("called");
	
	if (data == NULL) return result;
	
	switch (linSen_access_interface) {
		case interface_I2C: {
			const int buffer_size =	(LINSEN_GLOBAL_RES_ADDR - LINSEN_EXP_ADDR + 2) / 2;
			uint16_t buffer[buffer_size];
				
			result = i2c_read_n_w(LINSEN_EXP_ADDR, buffer, buffer_size);
			
			data->exposure = (buffer[1] << 16) | buffer[0];
			data->pixel_clock = buffer[2];
			data->brightness = buffer[3];
			data->pixel_number_x = buffer[4];
			data->pixel_number_y = buffer[5];
			data->result_scalar_number = (buffer[6] & 0x00FF);
			data->result_id = buffer[7];
			data->global_result = (int16_t)buffer[8];
			
			break;
		}
		case interface_SOCKET: {
			result = linSen_socket_read(LINSEN_DATA_READ_STRING, (int*)data, sizeof(linSen_data_t));
			break;
		}
		default:;
	}

	debug_printf("returns: %d", result);		
	return result;
}

/*
 * linSen quadPix get data values
 * 
 * @param *data:
 * @return non-negative size(success), -1 (failure)
 */
int linSen_qp_get_data(linSen_qp_data_t *data){
	const int buffer_size =	sizeof(uint16_t) + 2*4*sizeof(int32_t);
	int result = -1;
	debug_printf("called");
	
	if (data == NULL) return result;
	
	switch (linSen_access_interface) {
		case interface_I2C: {
			uint16_t buffer[buffer_size];
				
			result = i2c_read_n_w(LINSEN_QP_AVG_ADDR, buffer, buffer_size);
			if (result == buffer_size) {
				data->average = (buffer[1] << 16) | buffer[0];
				memcpy(data->filtered, &buffer[2], 4*sizeof(uint32_t));
			}
			break;
		}
		case interface_SOCKET: {
			result = linSen_socket_read(LINSEN_DATA_READ_STRING, (int*)data, buffer_size);
			break;
		}
		default:;
	}
	return result;		
}

/*
 * linSen quadPix get filtered value
 * 
 * @return non-negative value(success), -1 (failure)
 */
int linSen_qp_get_avg(void) {
	int result = -1;
	debug_printf("called");

	switch (linSen_access_interface) {
		case interface_I2C: {
			int16_t _value;
			if (i2c_read_w(LINSEN_QP_AVG_ADDR, (uint16_t*)&_value) == 2) result = _value;
			break;
		}
		case interface_SOCKET: {
			int _value;
			if (linSen_socket_read(LINSEN_QP_AVG_READ_STRING, &_value, sizeof(int16_t)) == EXIT_SUCCESS) result = _value;
			break;
		}
		default:;
	}

	debug_printf("returns: %d", result);
	return result;
}

/*
 * linSen quadPix get raw value
 * 
 * @param *frame:
 * @param size:
 * @return non-negative size(success), -1 (failure)
 */
int linSen_qp_get_raw(uint32_t* frame, int size){
	int result = -1;
	debug_printf("called");
	
	if (frame == NULL) return result;

	switch (linSen_access_interface) {
		case interface_I2C: {
			result = i2c_read_n_w(LINSEN_QP_RAW_ADDR, (uint16_t*)frame, size*sizeof(uint32_t)/sizeof(uint16_t));
			break;
		}
		case interface_SOCKET: {
			result = linSen_socket_read(LINSEN_QP_RAW_READ_STRING, (int*)frame, sizeof(uint32_t)*size);
			break;
		}
		default:;
	}

	debug_printf("returns: %d", result);		
	return result;
}

/*
 * linSen quadPix get filtered value
 * 
 * @param *frame:
 * @param size
 * @return non-negative size(success), -1 (failure)
 */
int linSen_qp_get_filt(uint32_t *frame, int size){
	int result = -1;
	debug_printf("called");
	
	if (frame == NULL) return result;
	
	switch (linSen_access_interface) {
		case interface_I2C: {
			result = i2c_read_n_w(LINSEN_QP_FIL_ADDR, (uint16_t*)frame, size*sizeof(uint32_t)/sizeof(uint16_t));
			break;
		}
		case interface_SOCKET: {
			result = linSen_socket_read(LINSEN_QP_FIL_READ_STRING, (int*)frame, sizeof(uint32_t)*size);
			break;
		}
		default:;
	}

	debug_printf("returns: %d", result);		
	return result;
}

/*
 * linSen servo get position value
 * 
 * @param chan: servo channel
 * @return non-negative position(success), -1 (failure)
 */
int linSen_servo_get_pos(int chan){
	int result = -1;
	debug_printf("called");
	
	if (chan < 0) return result;
	if (chan >= LINSEN_SERVO_NUMBER) return result;
	
	switch (linSen_access_interface) {
		case interface_I2C: {
			int16_t _value;
			if (i2c_read_w(LINSEN_SERVO_0_POS_ADDR, (uint16_t*)&_value) == 2) result = _value;
			break;
		}
		case interface_SOCKET: {
			int _value;
			if (linSen_socket_read(LINSEN_SERVO_0_POS_READ_STRING, &_value, sizeof(uint16_t)) == EXIT_SUCCESS) result = _value;
			break;
		}
		default:;
	}
	
	debug_printf("returns: %d", result);		
	return result;
}

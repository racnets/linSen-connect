/*
 * i2c.c
 *
 *  Created on: 03.01.2014
 *      Author: carsten
 */

#include <unistd.h>			// read, write
#include <linux/i2c-dev.h>	// I2C_SLAVE
#include <fcntl.h>			// open
#include <sys/ioctl.h>		// ioctl
#include <stdint.h>
#include <stdlib.h>			// malloc
#include <string.h>			// memcpy

int i2c_fd = 0;
uint8_t buf[4];

int i2c_init(const char *dev, int address) {
	i2c_fd = open(dev, O_RDWR);
	if (i2c_fd < 0) return 1;

	return ioctl(i2c_fd, I2C_SLAVE, address);
}

int i2c_de_init(void) {
	return close(i2c_fd);
}

int i2c_read_w(uint8_t address, uint16_t *value) {
	if (i2c_fd) {
		buf[0] = address;
		write(i2c_fd, buf, 1);
		read(i2c_fd, buf, 2);

		if (value == NULL) return -1;
		else *value = (uint16_t) ((buf[1] << 8) | buf[0]);
		return 1;
	} else return -1;
}

int i2c_write_w(uint8_t address, uint16_t value) {
	if (i2c_fd) {
		buf[0] = address;
		buf[1] = value;
		buf[2] = value >> 8;

		return write(i2c_fd, buf, 3);
	} else return -1;
}

int i2c_read_l(uint8_t address, uint32_t *value) {
	if (i2c_fd) {
		buf[0] = address;
		write(i2c_fd, buf, 1);
		read(i2c_fd, buf, 4);

		if (value == NULL) return -1;
		else *value = (uint32_t) ((buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0] );
		return 1;
	} else return -1;
}

int i2c_write_l(uint8_t address, uint32_t value) {
	if (i2c_fd) {
		buf[0] = address;
		buf[1] = value;
		buf[2] = value >> 8;
		buf[3] = value >> 16;
		buf[4] = value >> 24;

		return write(i2c_fd, buf, 5);
	} else return -1;
}

int i2c_read_n_w(uint8_t address, uint16_t* value, int count) {
	int result;
	int _size = count * sizeof(uint16_t);
	uint8_t* _buf = malloc(_size);
	
	if (value == NULL) return -1;
	if (count < 1) return -1;
	
	if (i2c_fd) {
		buf[0] = address;
		result = write(i2c_fd, buf, 1);
		if (result <= 0) return result;
		result = read(i2c_fd, _buf, _size);
		if (result <= 0) return result;

		memcpy(value, _buf, _size);
		free(_buf);
		return result;
	} else return -1;
}

int i2c_read_b(uint8_t address, uint8_t* value) {
	if (value == NULL) return -1;

	if (i2c_fd) {
		buf[0] = address;
		write(i2c_fd, buf, 1);
		read(i2c_fd, buf, 1);

		*value = buf[0];
		return 1;
	} else return -1;
}

int i2c_write_b(uint8_t address, uint8_t value) {
	if (i2c_fd) {
		buf[0] = address;
		buf[1] = value;
		write(i2c_fd, buf, 2);

		return 1;
	} else return -1;
}

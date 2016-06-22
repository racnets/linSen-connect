/*
 * i2c.c
 *
 * Author: racnets
 * raspi does not support clock-streching correctly:
 * http://elinux.org/BCM2835_datasheet_errata#p35_I2C_clock_stretching
 * so sometimes the ACK gets interpreted as MSB :-|
 * 
 */

#include <unistd.h>			// read, write
#include <linux/i2c-dev.h>	// I2C_SLAVE
#include <fcntl.h>			// open
#include <sys/ioctl.h>		// ioctl
#include <stdint.h>			// XintY_t-types
#include <stdlib.h>			// malloc
#include <string.h>			// memcpy

#include "main.h"  // verpose_printf(), debug_printf(), info_printf()
#include "i2c.h"

int i2c_fd = 0;
const int addr_size = 1;
const int byte_size = 2;
const int word_size = 2;
const int long_size = 4;

/*
 * initialize i2c
 * 
 * @param *dev: device descriptor
 * @param address: chip address
 * @return non-negative(success), -1(failure)
 */
int i2c_init(const char *dev, int address) {
	debug_printf("called with 'dev: %s \taddress %#x", dev, address);

	i2c_fd = open(dev, O_RDWR);
	
	/* failed? */
	if (i2c_fd == -1) return -1;
	
	int result = ioctl(i2c_fd, I2C_SLAVE, address);
	debug_printf("returns: %d", result);	
	return result;
}

/* 
 * close i2c file
 * 
 * @return 0(success), -1(failure)
 */
int i2c_close(void) {
	debug_printf("called");

	int result = close(i2c_fd);
	
	debug_printf("returns: %d", result);	
	return result;
}

/*
 * i2c read byte
 * 
 * @param address: address to read from
 * @param *value: value to write to
 * @return number of writen bytes(success), negative(failure)
 */
int i2c_read_b(uint8_t address, uint8_t* value) {
	uint8_t buf[byte_size];

	debug_printf("called with address: %#x \t*value: %#x", address, (int)value);

	/* check for valid value address */
	if (value == NULL) return -1;

	/* check for i2c file descriptor */
	if (i2c_fd == 0) {
		info_printf("no i2c device defined");
		return -1;
	}

	buf[0] = address;
	/* write address */
	if (write(i2c_fd, buf, 1) != 1) {
		info_printf("address write failed");
		return -1;
	}
	/* read */
	if (read(i2c_fd, buf, byte_size) != byte_size) {
		info_printf("read failed");
		return -1;
	}
	
	*value = buf[0];

	debug_printf("returns: %d", byte_size);
	return byte_size;
}

/*
 * i2c write byte
 * 
 * @param address: address to read from
 * @param value: value to write
 * @return number of writen bytes(success), negative(failure)
 */
int i2c_write_b(uint8_t address, uint8_t value) {
	const int buf_size = addr_size + byte_size;
	uint8_t buf[buf_size];

	debug_printf("called with address: %#x \t*value: %#x", address, (int)value);

	/* check for i2c file descriptor */
	if (i2c_fd == 0) {
		info_printf("no i2c device defined");
		return -1;
	}

	buf[0] = address;
	buf[1] = value;

	if (write(i2c_fd, buf, buf_size) != buf_size) {
		info_printf("write failed");
		return -1;
	}
	
	debug_printf("returns: %d", buf_size);
	return buf_size;
}

/*
 * i2c read word
 * 
 * @param address: address to read from
 * @param *value: value to write to
 * @return number of writen bytes(success), negative(failure)
 */
int i2c_read_w(uint8_t address, uint16_t *value) {
	uint8_t buf[word_size];

	debug_printf("called with address: %#x \t*value: %#x", address, (int)value);

	/* check for valid value address */
	if (value == NULL) return -1;

	/* check for i2c file descriptor */
	if (i2c_fd == 0) {
		info_printf("no i2c device defined");
		return -1;
	}

	buf[0] = address;
	/* write address */
	if (write(i2c_fd, buf, 1) != 1) {
		info_printf("address write failed");
		return -1;
	}
	/* read */
	if (read(i2c_fd, buf, word_size) != word_size) {
		info_printf("read failed");
		return -1;
	}
	
	*value = (uint16_t) ((buf[1] << 8) | buf[0]);

	debug_printf("returns: %d", word_size);
	return word_size;
}

/*
 * i2c write word
 * 
 * @param address: address to read from
 * @param value: value to write
 * @return number of writen bytes(success), negative(failure)
 */
int i2c_write_w(uint8_t address, uint16_t value) {
	const int buf_size = addr_size + word_size;
	uint8_t buf[buf_size];

	debug_printf("called with address: %#x \t*value: %#x", address, (int)value);

	/* check for i2c file descriptor */
	if (i2c_fd == 0) {
		info_printf("no i2c device defined");
		return -1;
	}

	buf[0] = address;
	buf[1] = value;
	buf[2] = value >> 8;

	if (write(i2c_fd, buf, buf_size) != buf_size) {
		info_printf("write failed");
		return -1;
	}
	
	debug_printf("returns: %d", buf_size);
	return buf_size;
}

/*
 * i2c read long
 * 
 * @param address: address to read from
 * @param *value: value to write to
 * @return number of writen bytes(success), negative(failure)
 */
int i2c_read_l(uint8_t address, uint32_t *value) {
	uint8_t buf[long_size];

	debug_printf("called with address: %#x \t*value: %#x", address, (int)value);

	/* check for valid value address */
	if (value == NULL) return -1;

	/* check for i2c file descriptor */
	if (i2c_fd == 0) {
		info_printf("no i2c device defined");
		return -1;
	}

	buf[0] = address;
	/* write address */
	if (write(i2c_fd, buf, 1) != 1) {
		info_printf("address write failed");
		return -1;
	}
	/* read */
	if (read(i2c_fd, buf, long_size) != long_size) {
		info_printf("read failed");
		return -1;
	}
	
	*value = (uint32_t) ((buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0] );

	debug_printf("returns: %d", long_size);
	return long_size;
}

/*
 * i2c write long
 * 
 * @param address: address to read from
 * @param value: value to write
 * @return number of writen bytes(success), negative(failure)
 */
int i2c_write_l(uint8_t address, uint32_t value) {
	const int buf_size = addr_size + long_size;
	uint8_t buf[buf_size];

	debug_printf("called with address: %#x \t*value: %#x", address, (int)value);

	/* check for i2c file descriptor */
	if (i2c_fd == 0) {
		info_printf("no i2c device defined");
		return -1;
	}

	buf[0] = address;
	buf[1] = value;
	buf[2] = value >> 8;
	buf[3] = value >> 16;
	buf[4] = value >> 24;

	if (write(i2c_fd, buf, buf_size) != buf_size) {
		info_printf("write failed");
		return -1;
	}
	
	debug_printf("returns: %d", buf_size);
	return buf_size;
}

/*
 * i2c read n words
 * 
 * @param address: address to read from
 * @param *value: value to write to
 * @param count: n
 * @return number of read bytes(success), negative(failure)
 */
int i2c_read_n_w(uint8_t address, uint16_t* value, int count) {
	debug_printf("called with address: %#x \t*value: %#x", address, (int)value);

	/* check for valid value address */
	if (value == NULL) return -1;

	/* check for i2c file descriptor */
	if (i2c_fd == 0) {
		info_printf("no i2c device defined");
		return -1;
	}

	int buf_size = count * word_size;
	uint8_t *buf = malloc(buf_size);

	buf[0] = address;
	/* write address */
	if (write(i2c_fd, buf, 1) != 1) {
		info_printf("address write failed");
		return -1;
	}
	/* read */
	if (read(i2c_fd, buf, buf_size) != buf_size) {
		info_printf("read failed");
		return -1;
	}
	
	/* copy to value address */
	memcpy(value, buf, buf_size);
	
	/* clean up */
	free(buf);

	debug_printf("returns: %d", buf_size);
	return buf_size;
}

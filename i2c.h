/*
 * i2c.h
 *
 *  Created on: 03.01.2014
 *      Author: carsten
 */

#ifndef I2C_H_
#define I2C_H_
#include <stdint.h>

int i2c_init(const char *dev, int address);
int i2c_de_init(void);
int i2c_read_b(uint8_t address, uint8_t *value);
int i2c_write_b(uint8_t address, uint8_t value);
int i2c_read_w(uint8_t address, uint16_t *value);
int i2c_write_w(uint8_t address, uint16_t value);
int i2c_read_l(uint8_t address, uint32_t *value);
int i2c_write_l(uint8_t address, uint32_t value);

#endif /* I2C_H_ */

/*
 * i2c.h
 *
 * Author: carsten
 */

#ifndef I2C_H_
#define I2C_H_

int i2c_init(const char *dev, int address);
int i2c_close(void);
int i2c_read_b(uint8_t address, uint8_t* value);
int i2c_write_b(uint8_t address, uint8_t value);
int i2c_read_w(uint8_t address, uint16_t* value);
int i2c_write_w(uint8_t address, uint16_t value);
int i2c_read_l(uint8_t address, uint32_t* value);
int i2c_write_l(uint8_t address, uint32_t value);
int i2c_read_n_w(uint8_t address, uint16_t* value, int count);


#endif /* I2C_H_ */

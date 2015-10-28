/*
 * linSen.h
 *
 * Author: carsten
 */

#ifndef LINSEN_H_
#define LINSEN_H_

#define LINSEN_I2C_SLAVE_ADDRESS	0x18

#define LINSEN_EXP_ADDR				0x80
#define LINSEN_PIX_CLK_ADDR			0x84
#define LINSEN_BRIGHT_ADDR			0x86
#define LINSEN_PIX_NUMBER_X_ADDR	0x88
#define LINSEN_PIX_NUMBER_Y_ADDR	0x8a
#define LINSEN_SCALAR_NUMBER_ADDR	0x8c
#define LINSEN_RES_ID_ADDR			0x8e
#define LINSEN_GLOBAL_RES_ADDR		0x90
#define LINSEN_RAW_ADDR				0xFF

#define LINSEN_EXP_READ_STRING		"rE"
#define LINSEN_PXI_CLK_READ_STRING	"rP"
#define LINSEN_BRIGHT_READ_STRING	"rB"
#define LINSEN_RESULT_READ_STRING	"rX"
#define LINSEN_RES_ID_READ_STRING	"rXi"
#define LINSEN_DATA_READ_STRING		"rD"
#define LINSEN_RAW_READ_STRING		"rR"

typedef struct {
	int exposure;
	int pixel_clock;
	int brightness;
	int pixel_number_x;
	int pixel_number_y;
	int result_scalar_number;
	int result_id;
	int global_result;
} linSen_data_t;

typedef enum {
	interface_NONE,
	interface_I2C,
	interface_SOCKET
} linSen_interface_t;

int linSen_init(const char* dev, linSen_interface_t);
int linSen_close(void);

int linSen_set_exposure(int value);
int linSen_get_exposure(void);

int linSen_set_pxclk(int value);
int linSen_get_pxclk(void);

int linSen_set_brightness(int value);
int linSen_get_brightness(void);

int linSen_get_result_id(void);
int linSen_get_global_result(void);

int linSen_get_raw(uint16_t* frame, int size);

int linSen_get_data(linSen_data_t* data);

int linSen_process();
#endif /* LINSEN_H_ */

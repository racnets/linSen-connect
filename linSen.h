/*
 * linSen.h
 *
 * Last modified: 18.06.2017
 *
 * Author: racnets 
 */

#ifndef LINSEN_H_
#define LINSEN_H_

#define LINSEN_I2C_SLAVE_ADDRESS  0x18

#define LINSEN_EXP_ADDR            0x80
#define LINSEN_PIX_CLK_ADDR        0x84
#define LINSEN_BRIGHT_ADDR         0x86
#define LINSEN_PIX_NUMBER_X_ADDR   0x88
#define LINSEN_PIX_NUMBER_Y_ADDR   0x8a
#define LINSEN_SCALAR_NUMBER_ADDR  0x8c
#define LINSEN_RES_ID_ADDR         0x8e
#define LINSEN_GLOBAL_RES_ADDR     0x90
#define LINSEN_RAW_ADDR            0xF0

#define LINSEN_EXP_READ_STRING       "rE"
#define LINSEN_PIX_CLK_READ_STRING   "rP"
#define LINSEN_BRIGHT_READ_STRING    "rB"
#define LINSEN_EXP_WRITE_STRING      "wE"
#define LINSEN_PIX_CLK_WRITE_STRING  "wP"
#define LINSEN_BRIGHT_WRITE_STRING   "wB"
#define LINSEN_RESULT_READ_STRING    "rX"
#define LINSEN_RES_ID_READ_STRING    "rXi"
#define LINSEN_DATA_READ_STRING	     "rD"
#define LINSEN_RAW_READ_STRING       "rR"

#define LINSEN_MIN_PIX_CLK  5       /* kHz */
#define LINSEN_MAX_PIX_CLK  705     /* kHz */
#define LINSEN_MIN_EXP      175     /* us */
#define LINSEN_MAX_EXP      100000  /* us */
#define LINSEN_MIN_BRIGHT   0
#define LINSEN_MAX_BRIGHT   4096    /* 12bit */

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

typedef struct {
	int average;
	int raw[4];
	int filtered[4];
} linSen_qp_data_t;

typedef enum {
	interface_NONE,
	interface_I2C,
	interface_SOCKET
} linSen_interface_t;

int linSen_init(const char* dev, int address, linSen_interface_t interface);
int linSen_close(void);

int linSen_set_exposure(int value);
int linSen_get_exposure(void);

int linSen_set_pxclk(int value);
int linSen_get_pxclk(void);

int linSen_set_brightness(int value);
int linSen_get_brightness(void);

int linSen_get_result_id(void);
int linSen_get_global_result(int *error);

int linSen_get_raw(uint16_t* frame, int size);

int linSen_get_data(linSen_data_t* data);

int linSen_process();

/* quad Pix addon */
#define LINSEN_QP_RAW_ADDR  0xF1
#define LINSEN_QP_AVG_ADDR  0x70
#define LINSEN_QP_FIL_ADDR  0x72

#define LINSEN_QP_RAW_READ_STRING   "rQR"
#define LINSEN_QP_AVG_READ_STRING   "rQA"
#define LINSEN_QP_FIL_READ_STRING   "rQF"
#define LINSEN_QP_DATA_READ_STRING  "rQD"

int linSen_qp_get_raw(uint32_t* frame, int size);
int linSen_qp_get_avg(void);
int linSen_qp_get_filt(uint32_t* frame, int size);

/* servo addon */
#define LINSEN_SERVO_NUMBER      1
#define LINSEN_SERVO_0_POS_ADDR  0x38

#define LINSEN_SERVO_0_POS_READ_STRING   "rS0P"
#define LINSEN_SERVO_0_POS_WRITE_STRING  "wS0P"

int linSen_servo_get_pos(int chan);
int linSen_servo_set_pos(int chan, unsigned int pos);

#endif /* LINSEN_H_ */

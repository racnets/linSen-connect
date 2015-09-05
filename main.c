#include <stdint.h>
#include <unistd.h>		//close, sleep
#include <stdio.h>		//fprintf, printf
#include <stdlib.h>		//atoi, atof
#include <string.h>		//strcmp
#include <getopt.h>		//getoptlong
#include <sys/time.h>	//gettimeofday

#include "linSen.h"
#include "i2c.h"
#include "socket-server.h"

#define I2C_SLAVE_ADDRESS	0x18

static uint8_t automatic = 0;
static uint8_t socket = 0;
static const char *file = NULL;

static uint8_t continuous = 0;
static uint8_t verbose = 0;

static uint8_t i2c = 0;
static const char *i2c_dev = "/dev/i2c-0";

static uint8_t val_read = 0;
static uint8_t val_write = 0;
static uint8_t bright = 0;
static uint16_t bright_val = 0;
static uint8_t pxclk = 0;
static uint16_t pxclk_val = 0;
static uint8_t exp = 0;
static uint32_t exp_val = 0;
static uint8_t g_result = 0;
static int16_t g_result_val = 0;

static uint8_t manual = 0;
//~ static uint16_t readAddr;
static uint16_t shutter = 0;
static double time = 0;
static uint8_t grab = 0;
static uint8_t quit = 0;

double getTime() {
	struct timeval tp;
	gettimeofday( &tp, NULL );
	return tp.tv_sec + tp.tv_usec/1E6;
}

static void print_usage(const char *prog)
{
	printf("Usage: %s [-cvX][-BEiP[arg]][-fkt arg]\n", prog);
	puts(" general\n"
	     "  -f arg  --file=arg       log file to write to\n"
	     "  -g      --grab           grab frame\n"
	     "  -i[dev] --i2c[=dev]      use i2c interface - default /dev/i2c-0\n"
	     "  -k arg  --socket=arg     write using socket\n"
	     "  -c      --cont           run continuously\n"
	     "  -t arg  --time=arg       run sepcified time\n"
	     "  -v      --verbose        be verbose\n"
	     " linSen specific\n"
	     "  -r      --read           reads value(s)\n"
	     "  -w      --write          writes value(s)\n"
	     "  -E[arg] --exp[=arg]      exposure\n"
	     "  -P[arg] --pxclk[=arg]    pixel clock\n"
	     "  -B[arg] --bright[=arg]   brightness\n"
	     "  -X      --result         global result scalar\n"
	);
	exit(1);
}

static void parse_opts(int argc, char *argv[])
{
	//forbid error message
	opterr=0;

	while (1) {
		static const struct option lopts[] = {
			{ "device",  required_argument, 0, 'D' },
			{ "shutter", required_argument, 0, 'S' },
			{ "file",    required_argument, 0, 'f' },
			{ "grab",    no_argument,       0, 'g' },
			{ "help",    no_argument,       0, 'h' },
			{ "i2c",     optional_argument, 0, 'i' },
			{ "socket",  no_argument,       0, 'k' },
			{ "cont",    no_argument,       0, 'c' },
			{ "time",    required_argument, 0, 't' },
			{ "verbose", no_argument,       0, 'v' },
			{ "read",    no_argument,       0, 'r' },
			{ "write",   no_argument,       0, 'w' },
			{ "exp",     optional_argument, 0, 'E' },
			{ "pxclk",   optional_argument, 0, 'P' },
			{ "bright",  optional_argument, 0, 'B' },
			{ "result",  no_argument,       0, 'X' },
			{ "auto",    no_argument,       0, 'a' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "D:f:i::S:t:B::E::P::acghkmrvwX", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
			case 'c':
				continuous = 1;
				break;
			case 'g':
				grab = 1;
				break;
			case 'i':
				i2c = 1;
				if (optarg) i2c_dev = optarg;
				break;
			case 'r':
				val_read = 1;
				break;
			case 'w':
				val_write = 1;
				break;
			case 'B':
				bright = 1;
				if (optarg) bright_val = atoi(optarg);
				break;
			case 'E':
				exp = 1;
				if (optarg) exp_val = atoi(optarg);
				break;
			case 'P':
				pxclk = 1;
				if (optarg) pxclk_val = atoi(optarg);
				break;
			case 'X':
				g_result = 1;
				break;
			case 'v':
				verbose = 1;
				break;
			case 'a':
				automatic = 1;
				break;
			case 't':
				time = atof(optarg);
				break;
			case 'S':
				shutter = atoi(optarg);
				break;
			case 'm':
				manual = 1;
				break;
			case 'k':
				socket = 1;
				break;
			case 'f':
				file = optarg;
				break;
			case 'h':
				print_usage(argv[0]);
				break;
			default:;
		}
	}
}

int main(int argc, char *argv[])
{
	int ret;
	int fd;
	FILE* lfd = NULL;
	double t, t0;

	printf("\nlinSen connect tool\n\n");
	
	if (argc < 2) {
		print_usage(argv[0]);
		return EXIT_SUCCESS;
	}
	
	parse_opts(argc, argv);

	if (i2c) {
		// init i2c
		int ret = linSen_i2c_init(i2c_dev, I2C_SLAVE_ADDRESS);
		if (verbose || ret) {
			printf("i2c initialization with address 0x%x over device %s\n", I2C_SLAVE_ADDRESS, i2c_dev);
			if (ret) printf("failed with error: %d\n",ret);
			else printf("succesfull\n");
		}
	}

	if (!continuous) {
		if (i2c && val_read) {
			if (bright) printf("brightness: %d\n", linSen_get_brightness());
			if (pxclk) printf("pixel clock: %d kHz\n", linSen_get_pxclk());
			if (exp) printf("exposure: %d µs\n", linSen_get_exposure());
			if (g_result) printf("global result scalar: %d\n", linSen_get_global_result());
		}
		
		if (i2c && val_write) {
			if (bright) {
				printf("set brightness set point to %d\n", bright_val);
				linSen_set_brightness(bright_val);
			}
			if (pxclk) {
				printf("set pixel clock to %d kHz\n", pxclk_val);
				linSen_set_pxclk(pxclk_val);
			}
			if (exp) {
				printf("set exposure to %d µs\n", exp_val);
				linSen_set_exposure(exp_val);
			}
		}
	} else {
		/* build header */
		if (bright) fprintf(stdout, "brightness\t");
		if (pxclk) fprintf(stdout, "pixel clock\t");
		if (exp) fprintf(stdout, "exposure\t");
		if (g_result) fprintf(stdout, "global result\t");
		fprintf(stdout, "\n");
	}

	while (continuous) {
		static int last_id = -1;
		int _id = linSen_get_result_id();
		if (last_id != _id) {
			fprintf(stdout, "%d\t", _id);
			if (bright) fprintf(stdout, "%d\t", linSen_get_brightness());
			if (pxclk) fprintf(stdout, "%d\t", linSen_get_pxclk());
			if (exp) fprintf(stdout, "%d\t" , linSen_get_exposure());
			if (g_result) fprintf(stdout, "%d\t", linSen_get_global_result());
			fprintf(stdout, "\n");
			last_id = _id;
		}
	}	
	
	if (i2c) linSen_i2c_de_init();
	
	return EXIT_SUCCESS;
}

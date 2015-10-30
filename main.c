#include <errno.h>		//errno
#include <signal.h>		//catch SIGTERM
#include <stdint.h>
#include <unistd.h>		//close, sleep
#include <stdio.h>		//fprintf, printf
#include <stdlib.h>		//atoi, atof
#include <string.h>		//strcmp, strerror
#include <getopt.h>		//getoptlong
#include <sys/time.h>	//gettimeofday

#include "linSen.h"
#include "i2c.h"
#include "linSen-socket.h"
#ifdef GTK_GUI
#include "gtk-viewer.h"
#endif //GTK_GUI


static uint8_t automatic = 0;
static const char *file = NULL;

static uint8_t continuous = 0;
static uint8_t verbose = 0;

/* interfaces */
static uint8_t i2c = 0;
static const char* i2c_dev = "/dev/i2c-0";

static uint8_t socket_client = 0;
static uint8_t socket_server = 0;
static char* socket_server_addr = NULL;

static uint8_t val_read = 0;
static uint8_t val_write = 0;
static uint8_t bright = 0;
static uint16_t bright_val = 0;
static uint8_t pxclk = 0;
static uint16_t pxclk_val = 0;
static uint8_t exp = 0;
static uint32_t exp_val = 0;
static uint8_t g_result = 0;
//~ static int16_t g_result_val = 0;

static uint8_t manual = 0;
//~ static uint16_t readAddr;
static uint16_t shutter = 0;
static double time = 0;
static uint8_t grab = 0;

double getTime() {
	struct timeval tp;
	gettimeofday( &tp, NULL );
	return tp.tv_sec + tp.tv_usec/1E6;
}

static void print_usage(const char *prog)
{
	printf("\nUsage: %s [-cvX][-BEiP[arg]][-fkt arg]\n", prog);
	puts(" general\n"
	     "  -f arg  --file=arg       log file to write to\n"
	     "  -g      --grab           grab frame\n"
	     "  -i[dev] --i2c[=dev]      use i2c interface - default /dev/i2c-0\n"
	     "  -k[arg]  --socket        provides socket server interface if arg is empty, otherwise use socket interface\n"
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
			{ "socket",  optional_argument, 0, 'k' },
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

		c = getopt_long(argc, argv, "D:f:i::k::S:t:B::E::P::acghmrvwX", lopts, NULL);

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
				if (optarg) {
					socket_client = 1;
					socket_server_addr = optarg;
				} else socket_server = 1;
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

void sig_handler(int signo) {
static int repeatedly = 0;

	switch (signo) {
		case SIGINT:
			socket_client = 0;
			socket_server = 0;
			continuous = 0;
			
			if (repeatedly++) exit(EXIT_SUCCESS);
			break;
		default:;
	}	
}

int main(int argc, char *argv[])
{
	//~ int fd;
	//~ FILE* lfd = NULL;
	//~ double t, t0;
	int result;

	printf("\nlinSen connect tool\n\n");
	
	if (argc < 2) {
		print_usage(argv[0]);
		return EXIT_SUCCESS;
	}
	
	if (signal(SIGINT, sig_handler) == SIG_ERR) printf("can't catch SIGINT\n");

	parse_opts(argc, argv);

	if (i2c && socket_client) {
		printf("can't use i2c and client socket interface simultaneously! Set socket to server functionality, instead!\n");
		socket_client = 0;
		socket_server = 1;
	}
	
	if (i2c) {
		// setup & initalize i2c
		printf("setup i2c via: %s", i2c_dev);

		result = linSen_init(i2c_dev, interface_I2C);
		if (result < 0) {
			printf(" ...failed with error %d: %s\n", errno, strerror(errno));
			i2c = 0;
		} else printf(" ...finished\n");
	}

	if (socket_client) {
		// setup & initalize socket as client
		printf("setup socket as client via: %s", socket_server_addr);

		result = linSen_init(socket_server_addr, interface_SOCKET);
		if (result < 0) {
			printf(" ...failed with error %d: %s\n", errno, strerror(errno));
			socket_client = 0;
		}
		else printf(" ...finished\n");
	}
	
	if (!i2c && !socket_client) {
		printf("no valid linSen-interface defined!\n");
		print_usage(argv[0]);
		return EXIT_SUCCESS;	
	}
	
	if (socket_server) {
		// setup & initalize socket as server
		printf("setup socket as server");

		result = linSen_init(NULL, interface_SOCKET);
		if (result < 0) {
			printf(" ...failed with error %d: %s\n", errno, strerror(errno));
			socket_server = 0;
		}
		else printf(" ...finished\n");
	}
	
#ifdef GTK_GUI
	viewer_init(&argc, &argv);
#else
	printf("no GUI supported for current host OS configuration\n");
#endif //GTK_GUI

	if (!continuous && (i2c || socket_client)) {
		if (val_read) {
			if (bright) printf("brightness: %d\n", linSen_get_brightness());
			if (pxclk) printf("pixel clock: %d kHz\n", linSen_get_pxclk());
			if (exp) printf("exposure: %d µs\n", linSen_get_exposure());
			if (g_result) printf("global result scalar: %d\n", linSen_get_global_result());
		}
		
		if (val_write) {
			if (bright) {
				printf("set brightness set point to %d...", bright_val);
				result = linSen_set_brightness(bright_val);
				if (result < 0) printf("failed\n");
				else printf("done\n");
			}
			if (pxclk) {
				printf("set pixel clock to %d kHz\n", pxclk_val);
				linSen_set_pxclk(pxclk_val);
			}
			if (exp) {
				printf("set exposure to %d µs...", exp_val);
				result = linSen_set_exposure(exp_val);
				if (result < 0) printf("failed\n");
				else printf("done\n");
			}
		}
		if (grab) {
				linSen_data_t linSen_data;
				uint16_t* frame;
				int i,j;

				printf("try to grab a complete set of sensor values:\n");

				result = linSen_get_data(&linSen_data);
				printf("\tpixel number: %d x %d\n", linSen_data.pixel_number_x, linSen_data.pixel_number_y);
				printf("\tlocal scalar number: %d\n", linSen_data.result_scalar_number);

				frame = malloc(linSen_data.pixel_number_x * linSen_data.pixel_number_y * sizeof(uint16_t));
				result = linSen_get_raw(frame, linSen_data.pixel_number_x * linSen_data.pixel_number_y);
				printf("\tgot: %d bytes\n", result);
				
				if (result) {
					for (i=0;i<linSen_data.pixel_number_y;i++) {
						for (j=0;j<linSen_data.pixel_number_x;j++) {
							if (!(j%linSen_data.result_scalar_number)) printf("\n");
							printf("\t%d", frame[i * linSen_data.pixel_number_x + j]);
						}
					}
				}
				printf("\n");
			}
	} else {
		/* build header */
		if (bright) fprintf(stdout, "brightness\t");
		if (pxclk) fprintf(stdout, "pixel clock\t");
		if (exp) fprintf(stdout, "exposure\t");
		if (g_result) fprintf(stdout, "global result\t");
		fprintf(stdout, "\n");
	}

	while (continuous || socket_server) {
		//~ printf("main loop\n");
		if (continuous) {
			linSen_data_t linSen_data;
			static int last_id = -1;

			result = linSen_get_data(&linSen_data);

			if (result < 0) {
			} else if (last_id != linSen_data.result_id) {
				printf("%d\t", linSen_data.result_id);
				//~ fprintf(stdout, "%d\t", _id);
				if (bright) fprintf(stdout, "%d\t", linSen_data.result_id);
				if (pxclk) fprintf(stdout, "%d\t", linSen_data.pixel_clock);
				if (exp) fprintf(stdout, "%d\t" , linSen_data.exposure);
				if (g_result) fprintf(stdout, "%d\t", linSen_data.global_result);
				fprintf(stdout, "\n");
				
				if (grab) {
					uint16_t* frame;

					frame = malloc(linSen_data.pixel_number_x * linSen_data.pixel_number_y * sizeof(uint16_t));
					result = linSen_get_raw(frame, linSen_data.pixel_number_x * linSen_data.pixel_number_y);
				
					if (result) {
#ifdef GTK_GUI						
						if (viewer_set_image(frame, linSen_data.pixel_number_x, linSen_data.pixel_number_y) == EXIT_FAILURE) {
						// exit
						break;
						}
						viewer_update();
#endif //GTK_GUI
					}
				}
				last_id = linSen_data.result_id;
			}
		}
		
		// socket
		if (socket_server) {
			if (linSen_process() < 0) break;
		}
	}
	
	printf("\nlinSen connect tool closed\n");

	return linSen_close();
}

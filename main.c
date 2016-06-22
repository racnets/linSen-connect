#include <errno.h>		//errno
#include <signal.h>		//catch SIGTERM
#include <stdint.h>
#include <unistd.h>		//close, sleep
#include <stdio.h>		//fprintf, printf
#include <stdlib.h>		//atoi, atof
#include <string.h>		//strcmp, strerror
#include <getopt.h>		//getoptlong
#include <sys/time.h>	//gettimeofday

#include "main.h"

#include "linSen.h"
#include "i2c.h"
#include "linSen-socket.h"
#ifdef GTK_GUI
#include "gtk-viewer.h"
#endif //GTK_GUI

typedef enum {
	FLAG_CLEARED = 0,
	FLAG_SET = 1,
} flag_type;

typedef struct {
	flag_type flag;
	union {
		int value;
		char *dev;
		char *server_address;
		char *file;
	};
} param_type;

struct {
	param_type brightness;
	param_type pixel_clock;
	param_type exposure;
	param_type global_result;
	param_type frame;
	param_type gui;
} linSen_param;

struct {
	param_type raw;
	param_type average;
	param_type filtered;
} quad_param;

struct {
	param_type continuous;
	param_type verbose;
	param_type gui;
    param_type log;
    param_type mark;
    /* socket */
	param_type socket_client;
	param_type socket_server;
	/* interfaces */
	/* i2c */
	param_type i2c;
	/* commands */
	/* read */
	param_type val_read;
	/* write */
	param_type val_write;
} prog_param = {
	.i2c.dev = "/dev/i2c-0",
	.mark.value = 100
};


static double time = 0;

/* get time in seconds */
double getTime() {
	struct timeval tp;
	gettimeofday( &tp, NULL );
	return tp.tv_sec + tp.tv_usec/1E6;
}

void set_flag(param_type *param) {
	param->flag = FLAG_SET;
}

void clear_flag(param_type *param) {
	param->flag = FLAG_CLEARED;
}

int is_set(param_type param) {
	return (param.flag == FLAG_SET);
}

int verbose() {
	return is_set(prog_param.verbose);
}

/* print usage */
static void print_usage(const char *prog)
{
	printf("\nUsage: %s [-cvX][-BEiP[arg]][-fkt arg]\n", prog);
	puts(" general\n"
	     "  -f arg  --file=arg       log file to write to\n"
	     "  -g      --gui            show GUI\n"
	     "  -i[dev] --i2c[=dev]      use i2c interface - default /dev/i2c-0\n"
	     "  -k[arg]  --socket        provides socket server interface if arg is empty, otherwise use socket interface\n"
	     "  -c      --cont           run continuously\n"
	     "  -t arg  --time=arg       run specified time in seconds\n"
	     "  -v      --verbose        be verbose\n"
	     "  -r      --read           reads value(s)\n"
	     "  -w      --write          writes value(s)\n"
	     " linSen related\n"
	     "  -E[arg] --exp[=arg]      exposure\n"
	     "  -P[arg] --pxclk[=arg]    pixel clock\n"
	     "  -B[arg] --bright[=arg]   brightness\n"
	     "  -F      --frame          frame\n"
	     "  -R      --result         global result scalar\n"
	     " quadPix related\n"
	     "  -Q      --quad           quadPix raw values\n"
	     "  -A      --qavg           average quadPix value\n"
	     "  -X      --qfilt          quadPix filtered values\n"
	);
	exit(1);
}

/* options parser */
static void parse_opts(int argc, char *argv[])
{
	// forbid error message
	opterr=0;

	while (1) {
		static const struct option lopts[] = {
			{ "device",  required_argument, 0, 'D' },
			{ "file",    required_argument, 0, 'f' },
			{ "gui",     no_argument,       0, 'g' },
			{ "help",    no_argument,       0, 'h' },
			{ "i2c",     optional_argument, 0, 'i' },
			{ "socket",  optional_argument, 0, 'k' },
			{ "cont",    no_argument,       0, 'c' },
			{ "time",    required_argument, 0, 't' },
			{ "verbose", no_argument,       0, 'v' },
			{ "read",    no_argument,       0, 'r' },
			{ "write",   no_argument,       0, 'w' },
			{ "mark",    optional_argument, 0, 'm' },			
			{ "exp",     optional_argument, 0, 'E' },
			{ "pxclk",   optional_argument, 0, 'P' },
			{ "bright",  optional_argument, 0, 'B' },
			{ "result",  no_argument,       0, 'R' },
			{ "frame",   no_argument,       0, 'F' },
			{ "auto",    no_argument,       0, 'a' },
			{ "quad",    no_argument,       0, 'Q' },
			{ "qavg",    no_argument,       0, 'A' },
			{ "qfilt",   no_argument,       0, 'X' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		/*
		 * :	required argument
		 * ::	optional argument
		 * 		no_argument
		*/
		c = getopt_long(argc, argv, "D:f:i::k::t:B::E::m::P::AcFghQrvwRX", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
			case 'c':
				set_flag(&prog_param.continuous);
				break;
			case 'g':
				set_flag(&prog_param.gui);
				break;
			case 'i':
				set_flag(&prog_param.i2c);
				if (optarg) prog_param.i2c.dev = optarg;
				break;
			case 'r':
				set_flag(&prog_param.val_read);
				break;
			case 'w':
				set_flag(&prog_param.val_write);
				break;
			case 'm':
				set_flag(&prog_param.mark);
				if (optarg) prog_param.mark.value = atoi(optarg);
				break;
			case 'B':
				set_flag(&linSen_param.brightness);
				if (optarg) linSen_param.brightness.value = atoi(optarg);
				break;
			case 'E':
				set_flag(&linSen_param.exposure);
				if (optarg) linSen_param.exposure.value = atoi(optarg);
				break;
			case 'P':
				set_flag(&linSen_param.pixel_clock);
				if (optarg) linSen_param.pixel_clock.value = atoi(optarg);
				break;
			case 'F':
				set_flag(&linSen_param.frame);
				break;
			case 'R':
				set_flag(&linSen_param.global_result);
				break;
			case 'Q':
				set_flag(&quad_param.raw);
				break;
			case 'X':
				set_flag(&quad_param.filtered);
				break;
			case 'A':
				set_flag(&quad_param.average);
				break;
			case 'v':
				set_flag(&prog_param.verbose);
				break;
			case 't':
				time = atof(optarg);
				set_flag(&prog_param.continuous);
				break;
			case 'k':
				if (optarg) {
					set_flag(&prog_param.socket_client);
					prog_param.socket_client.server_address = optarg;
				} else set_flag(&prog_param.socket_server);
				break;
			case 'f':
				set_flag(&prog_param.log);
				prog_param.log.file = optarg;
				break;
			case 'h':
				print_usage(argv[0]);
				break;
			default:;
		}
	}
}

/* signal handler 
 * safely terminates the program
 */
void sig_handler(int signo) {
	static int repeatedly = 0;

	switch (signo) {
		case SIGINT:
			clear_flag(&prog_param.socket_client);
			clear_flag(&prog_param.socket_server);
			clear_flag(&prog_param.gui);
			clear_flag(&prog_param.continuous);
			
			/* forced, dirty exit */
			if (repeatedly++) exit(EXIT_SUCCESS);
			break;
		default:;
	}	
}

int main(int argc, char *argv[]) {
	int result;
	// log file descriptor
	FILE* lfd = NULL;

	printf("\nlinSen connect tool\n\n");

	if (argc < 2) {
		print_usage(argv[0]);
		return EXIT_SUCCESS;
	}

	// set up signal handler
	if (signal(SIGINT, sig_handler) == SIG_ERR) printf("can't catch SIGINT\n");

	// optional argument parser
	parse_opts(argc, argv);
	
	// check, if not more than one client interface was declared 
	if (is_set(prog_param.i2c) && is_set(prog_param.socket_client)) {
		printf("can't use i2c and client socket interface simultaneously! Set socket to server functionality, instead!\n");
		clear_flag(&prog_param.socket_client);
		set_flag(&prog_param.socket_server);
	}

	// i2c interface
	if (is_set(prog_param.i2c)) {
		// setup & initalize i2c
		printf("setup i2c via: %s", prog_param.i2c.dev);

		result = linSen_init(prog_param.i2c.dev, interface_I2C);
		if (result < 0) {
			printf(" ...failed with error %d: %s\n", errno, strerror(errno));
			clear_flag(&prog_param.i2c);
		} else printf(" ...finished: %d\n", result);
	}
	
	// socket client interface
	if (is_set(prog_param.socket_client)) {
		// setup & initalize socket as client
		printf("setup socket as client via: %s", prog_param.socket_client.server_address);

		result = linSen_init(prog_param.socket_client.server_address, interface_SOCKET);
		if (result == EXIT_FAILURE) {
			printf(" ...failed with error %d: %s\n", errno, strerror(errno));
			clear_flag(&prog_param.socket_client);
		}
		else printf(" ...finished\n");
	}
		
	// check, if at least one interface was declared
	if (!is_set(prog_param.i2c) && !is_set(prog_param.socket_client)) {
		printf("no valid linSen-interface defined!\n");
		if (!is_set(prog_param.gui)) {
			print_usage(argv[0]);
			return EXIT_SUCCESS;	
		}
	}
	
	// socket server interface
	if (is_set(prog_param.socket_server)) {
		// setup & initalize socket as server
		printf("setup socket as server");

		result = linSen_init(NULL, interface_SOCKET);
		if (result < 0) {
			printf(" ...failed with error %d: %s\n", errno, strerror(errno));
			clear_flag(&prog_param.socket_server);
		}
		else printf(" ...finished\n");
	}
	
	// benchmark
	if (is_set(prog_param.mark)) {
		if (is_set(prog_param.i2c)) {
			double t1, t2, t;
			int result, i;
			printf("benchmark i2c connection - %d reads each\n", prog_param.mark.value);
			printf("\tbyte read:");
			{
				uint8_t value;
				t = 0;
				i = prog_param.mark.value;
				while (i) {
					t1 = getTime();
					result = i2c_read_b(0x00, &value);
					t2 = getTime();
					if (result == 1) {
						t +=  t2-t1;
						i--;
					}
				}
				printf("\t %f\n", prog_param.mark.value/t);
			}
			printf("\tword read:");
			{
				uint16_t value;
				t = 0;
				i = prog_param.mark.value;
				while (i) {
					t1 = getTime();
					result = i2c_read_w(0x00, &value);
					t2 = getTime();
					if (result == 2) {
						t +=  t2-t1;
						i--;
					}
				}
				printf("\t %f\n", prog_param.mark.value/t);
			}
			printf("\tlong read:");
			{
				uint32_t value;
				t = 0;
				i = prog_param.mark.value;
				while (i) {
					t1 = getTime();
					result = i2c_read_l(0x00, &value);
					t2 = getTime();
					if (result == 4) {
						t +=  t2-t1;
						i--;
					}
				}
				printf("\t %f\n", prog_param.mark.value/t);
			}
			printf("\tblock read 32 bytes:");
			{
				uint16_t value[16];
				t = 0;
				i = prog_param.mark.value;
				while (i) {
					t1 = getTime();
					result = i2c_read_n_w(0x00, value, 16);
					t2 = getTime();
					if (result) {
						t +=  t2-t1;
						i--;
					}
				}
				printf("\t %f\n", prog_param.mark.value/t);
			}
		}
		return EXIT_SUCCESS;
	}
	
	if (is_set(prog_param.log)) {
		lfd = fopen(prog_param.log.file, "w");
	} else lfd = stdout;
	
	if (is_set(prog_param.gui)) {
		// GUI interface
#ifdef GTK_GUI
		// set up GUI
		viewer_init(&argc, &argv);
#else
		printf("no GUI supported for current host OS configuration\n");
		clear_flag(&prog_param.gui);
#endif //GTK_GUI
	}
	
	if (is_set(prog_param.continuous)) {
		// prepare header
		fprintf(lfd, "id");
		if (is_set(linSen_param.brightness)) fprintf(lfd, "\tbright");
		if (is_set(linSen_param.pixel_clock)) fprintf(lfd, "\tpxclk");
		if (is_set(linSen_param.exposure)) fprintf(lfd, "\texp");
		if (is_set(linSen_param.global_result)) fprintf(lfd, "\tresult");
		if (is_set(quad_param.average)) fprintf(lfd, "\tq_average");
		if (is_set(quad_param.filtered)) fprintf(lfd, "\tq_filt_0 \tq_filt_1 \tq_filt_2 \tq_filt_3");
		if (is_set(quad_param.raw)) fprintf(lfd, "\tq_raw_0 \tq_raw_1 \tq_raw_2 \tq_raw_3");
	}

	if (time) {
		time += getTime();
	}

	if (is_set(prog_param.gui) || is_set(prog_param.continuous) || is_set(prog_param.socket_server)) {
		while (is_set(prog_param.gui) || is_set(prog_param.continuous) || is_set(prog_param.socket_server)) {
			if (is_set(prog_param.i2c) || is_set(prog_param.socket_client)) {
				// socket server functionality 
				if (is_set(prog_param.socket_server)) {
					if (linSen_process() < 0) break;
				}
				// acquire data for GUI or continous CLI mode
				if (is_set(prog_param.gui) || is_set(prog_param.continuous)) {
					linSen_data_t linSen_data;
					static int last_id = -1;

					result = linSen_get_data(&linSen_data);

					if (result < 0) {
					} else if (last_id != linSen_data.result_id) {
						debug_printf("result_id: %d", linSen_data.result_id);
						if (is_set(prog_param.continuous)) {
							fprintf(lfd, "%d\t", linSen_data.result_id);
							if (is_set(linSen_param.brightness)) fprintf(lfd, "\t%d", linSen_data.brightness);
							if (is_set(linSen_param.pixel_clock)) fprintf(lfd, "\t%d", linSen_data.pixel_clock);
							if (is_set(linSen_param.exposure)) fprintf(lfd, "\t%d" , linSen_data.exposure);
							if (is_set(linSen_param.global_result)) fprintf(lfd, "\t%d", linSen_data.global_result);
							if (is_set(quad_param.raw)) {
								uint32_t frame[4];
								int i;

								result = linSen_qp_get_raw(frame, 4);
								if (result) {
									for (i=0;i<4;i++) {
										fprintf(lfd, "\t%d", frame[i]);
									}
								}
							}
							fprintf(lfd, "\n");
						}
#ifdef GTK_GUI			
						if (is_set(prog_param.gui)) {
							/* sent to linSen data to GUI */
							viewer_set_linSen_data(&linSen_data);
							
							/* sent linSen raw data to GUI, if requested */
							if (viewer_want_linSen_raw()) {
								uint16_t* frame;

								frame = malloc(linSen_data.pixel_number_x * linSen_data.pixel_number_y * sizeof(uint16_t));
								result = linSen_get_raw(frame, linSen_data.pixel_number_x * linSen_data.pixel_number_y);
						
								if (result) {									
									if (viewer_set_linSen_raw(frame, linSen_data.pixel_number_x, linSen_data.pixel_number_y) == EXIT_FAILURE) {
									// exit
									break;
									}								
								}
							}
							
							/* sent linSen quadPix data to GUI, if requested */
							if (viewer_want_quadPix_raw()) {
								uint32_t frame[4];

								result = linSen_qp_get_raw(frame, 4);
								
								viewer_set_quadPix_raw(frame, 2, 2);
							}

							/* sent linSen quadPix result to GUI, if requested */
							if (viewer_want_quadPix_result()) {
								uint32_t frame[4];

								result = linSen_qp_get_filt(frame, 4);
								
								viewer_set_quadPix_result((int32_t)frame, 2, 2);
							}
						}
#endif //GTK_GUI
						last_id = linSen_data.result_id;
					}
					if (time && (time < getTime())) {
						// stop after defined time - if given
						clear_flag(&prog_param.continuous);
						clear_flag(&prog_param.gui);
					}
				}
			}
#ifdef GTK_GUI
			if (is_set(prog_param.gui)) {
				if (viewer_update() == EXIT_FAILURE) {
					//exit 
					break;
				}
			}
#endif //GTK_GUI
		}
	} else {
		// intermittent CLI print out presentation
		if (is_set(prog_param.i2c) || is_set(prog_param.socket_client)) {
			if (is_set(prog_param.val_read)) {
				if (is_set(linSen_param.brightness)) printf("brightness: %d\n", linSen_get_brightness());
				if (is_set(linSen_param.pixel_clock)) printf("pixel clock: %d kHz\n", linSen_get_pxclk());
				if (is_set(linSen_param.exposure)) printf("exposure: %d µs\n", linSen_get_exposure());
				if (is_set(linSen_param.global_result)) printf("global result scalar: %d\n", linSen_get_global_result(NULL));
				if (is_set(linSen_param.frame)) {
					linSen_data_t linSen_data;
					uint16_t* frame;
					int i,j;

					printf("try to grab a complete set of sensor values:\n");
					result = linSen_get_data(&linSen_data);
					if (result < 0) {
						info_printf("failed getting linSen data: %d\n", result);
					} else {
						printf("\tbrightness: %d\n", linSen_data.brightness);
						printf("\tpixel clock: %d kHz\n", linSen_data.pixel_clock);
						printf("\texposure: %d µs\n", linSen_data.exposure);
						printf("\tglobal result id: %d\n", linSen_data.result_id);
						printf("\tglobal result scalar: %d\n", linSen_data.global_result);
						printf("\tpixel number: %d x %d\n", linSen_data.pixel_number_x, linSen_data.pixel_number_y);
						printf("\tlocal scalar number: %d\n", linSen_data.result_scalar_number);
						frame = malloc(linSen_data.pixel_number_x * linSen_data.pixel_number_y * sizeof(uint16_t));
						result = linSen_get_raw(frame, linSen_data.pixel_number_x * linSen_data.pixel_number_y);
						printf("\tgot: %d bytes\n", result);

						int k;
						if (linSen_data.result_scalar_number) k = linSen_data.result_scalar_number; else k = 8;

						if (result > 0) {
							for (i=0;i<linSen_data.pixel_number_y;i++) {
								for (j=0;j<linSen_data.pixel_number_x;j++) {
									if (!(j%k)) printf("\n");
									printf("\t%d", frame[i * linSen_data.pixel_number_x + j]);
								}
							}
						}
						printf("\n");
					}
				}
				if (is_set(quad_param.average)) printf("quadPix sensor average: %d\n", linSen_qp_get_avg());
				if (is_set(quad_param.raw)) {
					uint32_t frame[4];
					int i;

					printf("quadPix sensor values: \n");
					result = linSen_qp_get_raw(frame, 4);
					if (result > 0) {
						for (i=0;i<4;i++) {
							printf("\t%d", frame[i]);
						}
					} else printf("failed");
					printf("\n");
				}
				if (is_set(quad_param.filtered)) {
					uint32_t frame[4];
					int i;

					printf("quadPix filtered sensor values: \n");
					result = linSen_qp_get_filt(frame, 4);
					if (result > 0) {
						for (i=0;i<4;i++) {
							printf("\t%d", frame[i]);
						}
					} else printf("failed");
					printf("\n");
				}
					
			}
			if (is_set(prog_param.val_write)) {
				if (is_set(linSen_param.brightness)) {
					printf("set brightness set point to %d...", linSen_param.brightness.value);
					result = linSen_set_brightness(linSen_param.brightness.value);
					if (result < 0) printf("failed\n");
					else printf("done\n");
				}
				if (is_set(linSen_param.pixel_clock)) {
					printf("set pixel clock to %d kHz\n", linSen_param.pixel_clock.value);
					linSen_set_pxclk(linSen_param.pixel_clock.value);
				}
				if (is_set(linSen_param.exposure)) {
					printf("set exposure to %d µs...", linSen_param.exposure.value);
					result = linSen_set_exposure(linSen_param.exposure.value);
					if (result < 0) printf("failed\n");
					else printf("done\n");
				}
			}
		}
	}

	if (is_set(prog_param.log)) fclose(lfd);

	printf("\nlinSen connect tool closed\n");

	return linSen_close();
}

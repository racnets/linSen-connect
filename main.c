/*
 * main.c
 *
 * Last modified: 18.06.2017
 *
 * Author: racnets
 */
#include <errno.h>     //errno
#include <signal.h>    //catch SIGTERM
#include <stdint.h>
#include <unistd.h>    //close, sleep
#include <stdio.h>     //fprintf, printf
#include <stdlib.h>    //atoi, atof, strtol
#include <string.h>    //strcmp, strerror
#include <getopt.h>    //getoptlong
#include <sys/time.h>  //gettimeofday

#include "main.h"

#include "linSen.h"
#include "i2c.h"
#include "linSen-socket.h"
#ifdef GTK_GUI
#include "gtk-viewer.h"
#endif //GTK_GUI

typedef enum {
	FLAG_CLEARED = 0,
	FLAG_SET = 1
} flag_type;

typedef struct {
	flag_type set;
	flag_type get;
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
	param_type pos;
} servo_param;

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
	param_type i2c_addr;
	/* commands */
	/* read */
	param_type val_read;
	/* write */
	param_type val_write;
} prog_param = {
	.i2c.dev = "/dev/i2c-0",
	.mark.value = 100,
	.i2c_addr.value = LINSEN_I2C_SLAVE_ADDRESS
};


static double time = 0;

/* get time in seconds */
double getTime() {
	struct timeval tp;
	gettimeofday( &tp, NULL );
	return tp.tv_sec + tp.tv_usec/1E6;
}

void set_set(param_type *param) {
	param->set = FLAG_SET;
}

void clear_set(param_type *param) {
	param->set = FLAG_CLEARED;
}

int is_set(param_type param) {
	return (param.set == FLAG_SET);
}
void set_get(param_type *param) {
	param->get = FLAG_SET;
}

void clear_get(param_type *param) {
	param->get = FLAG_CLEARED;
}

int is_get(param_type param) {
	return (param.get == FLAG_SET);
}

int verbose() {
	return is_set(prog_param.verbose);
}

/* print usage */
static void print_usage(const char *prog)
{
	printf("\nUsage: %s [-fgiakcvmEPBFRQAXp]\n", prog);
	puts(" general\n"
	     "  -f arg  --file=arg       log file to write to\n"
	     "  -g      --gui            show GUI\n"
	     "  -i[dev] --i2c[=dev]      use i2c interface - default /dev/i2c-0\n"
	     "  -a addr --addr[=]addr    i2c slave address - if not set, default is used\n"
	     "  -k[arg] --socket         provides socket server interface if arg is not set, otherwise use socket interface\n"
	     "  -c[arg] --cont[=arg]     run continuously for given time if arg is set, otherwise endless\n"
	     "  -v      --verbose        be verbose\n"
	     "  -m[arg] --mark[=arg]     i2c benchmark - optional count of consecutive read and writes\n"
	     " linSen related\n"
	     "  -E[arg] --exp[=arg]      get or set exposure\n"
	     "  -P[arg] --pxclk[=arg]    get or set pixel clock\n"
	     "  -B[arg] --bright[=arg]   get or set brightness\n"
	     "  -F      --frame          get frame\n"
	     "  -R      --result         get global result scalar\n"
	     " quadPix related\n"
	     "  -Q      --quad           get quadPix raw values\n"
	     "  -A      --qavg           get average quadPix value\n"
	     "  -X      --qfilt          get quadPix filtered values\n"
	     " servo related\n"
	     "  -p[arg] --pos[=arg]      get or set servo position\n"
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
			{ "file",    required_argument, 0, 'f' },
			{ "gui",     no_argument,       0, 'g' },
			{ "help",    no_argument,       0, 'h' },
			{ "i2c",     optional_argument, 0, 'i' },
			{ "addr",    required_argument, 0, 'a' },
			{ "socket",  optional_argument, 0, 'k' },
			{ "cont",    optional_argument, 0, 'c' },
			{ "verbose", no_argument,       0, 'v' },
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
			{ "pos",     optional_argument, 0, 'p' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		/*
		 * :	required argument
		 * ::	optional argument
		 * 		no_argument
		*/
		c = getopt_long(argc, argv, "a:c::f:i::k::B::E::m::p::P::AFghQvRX", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
			case 'c':
				set_set(&prog_param.continuous);
				if (optarg) time = atof(optarg);
				break;
			case 'g':
				set_set(&prog_param.gui);
				break;
			case 'i':
				set_set(&prog_param.i2c);
				if (optarg) prog_param.i2c.dev = optarg;
				break;
			case 'a':
				set_set(&prog_param.i2c_addr);
				prog_param.i2c_addr.value = strtoul(optarg, NULL, 16);
				break;
			case 'm':
				set_get(&prog_param.mark);
				if (optarg) prog_param.mark.value = atoi(optarg);
				break;
			case 'B':
				if (optarg) {
					linSen_param.brightness.value = atoi(optarg);
					set_set(&linSen_param.brightness);
				} else {
					set_get(&linSen_param.brightness);
				}
				break;
			case 'E':
				if (optarg) {
					linSen_param.exposure.value = atoi(optarg);
					set_set(&linSen_param.exposure);
				} else {
					set_get(&linSen_param.exposure);
				}
				break;
			case 'P':
				if (optarg) {
					linSen_param.pixel_clock.value = atoi(optarg);
					set_set(&linSen_param.pixel_clock);
				} else {
					set_get(&linSen_param.pixel_clock);
				}
				break;
			case 'F':
				set_get(&linSen_param.frame);
				break;
			case 'R':
				set_get(&linSen_param.global_result);
				break;
			case 'Q':
				set_get(&quad_param.raw);
				break;
			case 'X':
				set_get(&quad_param.filtered);
				break;
			case 'A':
				set_get(&quad_param.average);
				break;
			case 'v':
				set_set(&prog_param.verbose);
				break;
			case 'k':
				if (optarg) {
					set_set(&prog_param.socket_client);
					prog_param.socket_client.server_address = optarg;
				} else {
					set_set(&prog_param.socket_server);
				}
				break;
			case 'f':
				set_set(&prog_param.log);
				prog_param.log.file = optarg;
				break;
			case 'p':
				if (optarg) {
					servo_param.pos.value = atoi(optarg);
					set_set(&servo_param.pos);
				} else {
					set_get(&servo_param.pos);
				}
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
			clear_set(&prog_param.socket_client);
			clear_set(&prog_param.socket_server);
			clear_set(&prog_param.gui);
			clear_set(&prog_param.continuous);
			
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

//	if (argc < 2) {
//		print_usage(argv[0]);
//		return EXIT_SUCCESS;
//	}


	/* set up signal handler */
	if (signal(SIGINT, sig_handler) == SIG_ERR) printf("can't catch SIGINT\n");

	/* optional argument parser */
	parse_opts(argc, argv);
	
	/* check, if not more than one client interface was declared */
	if (is_set(prog_param.i2c) && is_set(prog_param.socket_client)) {
		printf("can't use i2c and client socket interface simultaneously! Set socket to server functionality, instead!\n");
		clear_set(&prog_param.socket_client);
		set_set(&prog_param.socket_server);
	}

	/* i2c interface */
	if (is_set(prog_param.i2c)) {
		/* setup & initalize i2c */
		printf("setup i2c via: %s", prog_param.i2c.dev);
		printf(" @ address: %d (%#x)", prog_param.i2c_addr.value, prog_param.i2c_addr.value);

		result = linSen_init(prog_param.i2c.dev, prog_param.i2c_addr.value, interface_I2C);
		if (result < 0) {
			printf(" ...failed with error %d: %s\n", errno, strerror(errno));
			clear_set(&prog_param.i2c);
		}
		else printf(" ...finished\n");
	}
	
	/* socket client interface */
	if (is_set(prog_param.socket_client)) {
		// setup & initalize socket as client
		printf("setup socket as client via: %s", prog_param.socket_client.server_address);

		result = linSen_init(prog_param.socket_client.server_address, 0, interface_SOCKET);
		if (result == EXIT_FAILURE) {
			printf(" ...failed with error %d: %s\n", errno, strerror(errno));
			clear_set(&prog_param.socket_client);
		}
		else printf(" ...finished\n");
	}
		
	/* check, if at least one interface was declared */
	if (!is_set(prog_param.i2c) && !is_set(prog_param.socket_client)) {
		printf("no valid linSen-interface defined!\n");
		if (!is_set(prog_param.gui)) {
			print_usage(argv[0]);
			return EXIT_SUCCESS;	
		}
	}
	
	/* socket server interface */
	if (is_set(prog_param.socket_server)) {
		// setup & initalize socket as server
		printf("setup socket as server");

		result = linSen_init(NULL, 0, interface_SOCKET);
		if (result < 0) {
			printf(" ...failed with error %d: %s\n", errno, strerror(errno));
			clear_set(&prog_param.socket_server);
		}
		else printf(" ...finished\n");
	}
	
	/* benchmark i2c */
	if (is_get(prog_param.mark) && (is_set(prog_param.i2c))) {
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
		return EXIT_SUCCESS;
	}
	
	/* prepare log file */
	if (is_set(prog_param.log)) {
		lfd = fopen(prog_param.log.file, "w");
	} else lfd = stdout;
	
	/* prepare GUI */
	if (is_set(prog_param.gui)) {
		/* GUI interface */
#ifdef GTK_GUI
		/* set up GUI */
		viewer_init(&argc, &argv);
#else
		printf("no GUI supported for current host OS configuration\n");
		clear_set(&prog_param.gui);
#endif //GTK_GUI
	}
	
	/* prepare header for logging or continuos CLI operation */
	if (is_set(prog_param.continuous)) {
		fprintf(lfd, "date\tid");
		if (is_get(linSen_param.brightness)) fprintf(lfd, "\tbright");
		if (is_get(linSen_param.pixel_clock)) fprintf(lfd, "\tpxclk");
		if (is_get(linSen_param.exposure)) fprintf(lfd, "\texp");
		if (is_get(linSen_param.global_result)) fprintf(lfd, "\tresult");
		if (is_get(quad_param.average)) fprintf(lfd, "\tq_average");
		if (is_get(quad_param.raw)) fprintf(lfd, "\tq_raw_0 \tq_raw_1 \tq_raw_2 \tq_raw_3");
		if (is_get(quad_param.filtered)) fprintf(lfd, "\tq_filt_0 \tq_filt_1 \tq_filt_2 \tq_filt_3");
		if (is_get(servo_param.pos)) fprintf(lfd, "\tservo0");			
		fprintf(lfd, "\n");
	}

	/* set end-time for contious operation */
	if (time) {
		time += getTime();
	}

	/* contious or gui operation */
	while (is_set(prog_param.gui) || is_set(prog_param.continuous) || is_set(prog_param.socket_server)) {
		if (is_set(prog_param.i2c) || is_set(prog_param.socket_client)) {
			/* socket server functionality */
			if (is_set(prog_param.socket_server)) {
				if (linSen_process() < 0) break;
			}
			
			/* acquire data for GUI or continous CLI mode */
			if (is_set(prog_param.gui) || is_set(prog_param.continuous)) {
				linSen_data_t linSen_data;
				__attribute__((__unused__)) static int last_id = -1;

				result = linSen_get_data(&linSen_data);
				if (result < 0) {
				}
				
				if (is_set(prog_param.continuous)) {
					/* contious operation - logging */
					fprintf(lfd, "%f\t%d\t", getTime(), linSen_data.result_id);
					if (is_get(linSen_param.brightness)) fprintf(lfd, "\t%d", linSen_data.brightness);
					if (is_get(linSen_param.pixel_clock)) fprintf(lfd, "\t%d", linSen_data.pixel_clock);
					if (is_get(linSen_param.exposure)) fprintf(lfd, "\t%d" , linSen_data.exposure);
					if (is_get(linSen_param.global_result)) fprintf(lfd, "\t%d", linSen_data.global_result);					
					if (is_get(quad_param.average)) fprintf(lfd, "\t%d", linSen_qp_get_avg());
					if (is_get(quad_param.raw)) {
						uint32_t frame[4];
						int i;

						result = linSen_qp_get_raw(frame, 4);
						if (result) {
							for (i=0;i<4;i++) {
								fprintf(lfd, "\t%d", frame[i]);
							}
						}
					}
					if (is_get(quad_param.filtered)) {
						uint32_t frame[4];
						int i;

						result = linSen_qp_get_filt(frame, 4);
						if (result) {
							for (i=0;i<4;i++) {
								fprintf(lfd, "\t%d", frame[i]);
							}
						}
					}
					if (is_get(servo_param.pos)) fprintf(lfd, "\t%d", linSen_servo_get_pos(0));
					fprintf(lfd, "\n");
				}
#ifdef GTK_GUI			
				if (is_set(prog_param.gui) && (last_id != linSen_data.result_id)) {
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
								
						viewer_set_quadPix_result((int32_t *)frame, 2, 2);
					}
				}
#endif //GTK_GUI
				last_id = linSen_data.result_id;

				if (time && (time < getTime())) {
					// stop after defined time - if given
					clear_set(&prog_param.continuous);
					clear_set(&prog_param.gui);
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
	} // while
	if (!is_set(prog_param.gui) && !is_set(prog_param.continuous) && !is_set(prog_param.socket_server)) {	
		/* intermittent CLI print out presentation */
		if (is_set(prog_param.i2c) || is_set(prog_param.socket_client)) {
			/* setter */
			if (is_set(linSen_param.brightness)) {
				printf("set brightness set point to %d...", linSen_param.brightness.value);
				result = linSen_set_brightness(linSen_param.brightness.value);
				if (result < 0) printf("failed\n");
				else printf("done\n");
			}
			if (is_set(linSen_param.pixel_clock)) {
				printf("set pixel clock to %d kHz\n", linSen_param.pixel_clock.value);
				result = linSen_set_pxclk(linSen_param.pixel_clock.value);
				if (result < 0) printf("failed\n");
				else printf("done\n");
			}
			if (is_set(linSen_param.exposure)) {
				printf("set exposure to %d µs...", linSen_param.exposure.value);
				result = linSen_set_exposure(linSen_param.exposure.value);
				if (result < 0) printf("failed\n");
				else printf("done\n");
			}
			if (is_set(servo_param.pos)) {
				printf("set servo position to %d ...", servo_param.pos.value);
				result = linSen_servo_set_pos(0, servo_param.pos.value);
				if (result < 0) printf("failed\n");
				else printf("done\n");
			}

			/* getter */
			if (is_get(linSen_param.brightness)) printf("brightness: %d\n", linSen_get_brightness());
			if (is_get(linSen_param.pixel_clock)) printf("pixel clock: %d kHz\n", linSen_get_pxclk());
			if (is_get(linSen_param.exposure)) printf("exposure: %d µs\n", linSen_get_exposure());
			if (is_get(linSen_param.global_result)) printf("global result scalar: %d\n", linSen_get_global_result(NULL));
			if (is_get(linSen_param.frame)) {
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
			if (is_get(quad_param.average)) printf("quadPix sensor average: %d\n", linSen_qp_get_avg());
			if (is_get(quad_param.raw)) {
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
			if (is_get(quad_param.filtered)) {
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
			if (is_get(servo_param.pos)) fprintf(lfd, "servo pos: %d\n", linSen_servo_get_pos(0));
		}
	}

	if (is_set(prog_param.log)) fclose(lfd);

	printf("\nlinSen connect tool closed\n");

	return linSen_close();
}

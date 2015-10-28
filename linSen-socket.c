#include <errno.h>		//errno
#include <sys/types.h>	// legacy support - may be needed for socket.h
#include <sys/socket.h>	// accept, connect, recv, send, socket
#include <netinet/in.h>	// sockaddr_in
#include <arpa/inet.h>	// htons(), inet_aton()
#include <limits.h>		// INT_MAX
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "linSen.h"
#include "linSen-socket.h"

#define BUF 1024
#define DEFTOSTRING(x) #x

int server_socket;
int client_socket;
int accepted_socket;

struct sockaddr_in address = {0};
socklen_t addrlen;
char buffer[BUF];

/* 
 * waits - non-blocking - for data to receive
 * allocates memory for the data to return
 */
int linSen_socket_receive(int dev, char** val, int* len) {
	int result;
	
	result = recv(dev, buffer, BUF-1, MSG_DONTWAIT);
	if( result > 0) {
		buffer[result++] = '\0';
	} else return result;
	
	*len = result;
	*val = malloc(result);
	memcpy(*val, buffer, result);
	return result;
}

/* 
 * sends data
 */
int linSen_socket_send(int dev, char* val, int len) {
	//~ printf("linSen_socket_send(%s,%d) called\n", val, len);
	if (dev < 0) return -1;

	// send
	return send(dev, val, len, 0);
}

/*
 * sends string
 */
int linSen_socket_send_string(int dev, char* val) {
	return linSen_socket_send(dev, val, strlen(val));
}

/*
 * close socket
 */
int linSen_socket_server_close(void) {
	return close(server_socket);
}

int linSen_socket_client_close(void) {
	if (client_socket) linSen_socket_send_string(client_socket, "exit");
	
	return close(client_socket);
}


int linSen_socket_server_init(void) {
	int result;
	
	// create socket
	server_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

	const int y = 1;
  	result = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));
  	if (result < 0) return result;
	
	// bind socket to port
	address.sin_family = AF_INET;
	address.sin_port = htons(15000);
	address.sin_addr.s_addr = INADDR_ANY;	
	result = bind(server_socket, (struct sockaddr *) &address, sizeof (address));
  	if (result < 0) return result;
	
	// listen
	result = listen(server_socket, 3);
  	if (result < 0) return result;
	
	addrlen = sizeof(struct sockaddr_in);
	
	return result;
}	

char* linSen_socket_get_client_address(void) {
	if (accepted_socket > 0) return inet_ntoa(address.sin_addr);
	else return "undefined";
}

/* 
 * waits - non-blocking - for a client to connect 
 */
int linSen_socket_server_wait_for_client(void) {
	accepted_socket = accept(server_socket, (struct sockaddr *) &address, &addrlen);
	if (accepted_socket > 0) {
		return address.sin_addr.s_addr;
	} else return accepted_socket;
}

int linSen_socket_server_process(void) {
	int result;
	static enum {
		STATE_UNKNOWN, 
		STATE_WAIT, 
		STATE_RX, 
		STATE_TX_DATA,
		STATE_TX_BRIGHTNESS,
		STATE_TX_EXPOSURE, 
		STATE_TX_PIXELCLOCK,
		STATE_TX_RESULT,
		STATE_TX_RESULT_ID,
		STATE_TX_GRAB, 
		STATE_TX, 
		STATE_TX_UNKNOWN, 
		STATE_EXIT
	} state = STATE_WAIT;
	
	linSen_data_t linSen_data;
	static char* _buffer;
	static int _buffer_size = 0;

	//~ printf("linSen_socket_server_process() called. State: %d\n", state);
	switch (state) {
		case STATE_WAIT: {
			result = linSen_socket_server_wait_for_client();
			if (result > 0) {
				printf("\tclient(%s) connected!\n", linSen_socket_get_client_address());
				state = STATE_RX;
			} else {
				int _errno = errno;
				if (_errno == EAGAIN) {
					//~ printf("\tretry in 1s\n");
					sleep(1);
				} else {
					printf("\tfailed with error %d: %s\n", _errno, strerror(_errno));
					return result;
				}
			};
			break;
		}
		case STATE_RX: {
			int size;
			
			result = linSen_socket_receive(accepted_socket, &_buffer, &size);
			if (result < 0) {
				if (errno != EAGAIN) {
					printf("\treceive failed with error %d: %s\n", errno, strerror(errno));
					return result;
				} else break;
			} else if (result > 0) {
				printf("\treceived %d bytes: \"%s\"!\n", size, _buffer);
					// parse received command
				if ((strcmp("grab", _buffer) == 0)
					|| (strcmp("g", _buffer) == 0)) { 
					state = STATE_TX_GRAB;
				} else if ((strcmp("data", _buffer) == 0)
					|| (strcmp("d", _buffer) == 0)) { 
					state = STATE_TX_DATA;
				} else if (strcmp(LINSEN_EXP_READ_STRING, _buffer) == 0) {
					state = STATE_TX_EXPOSURE;
				} else if (strcmp(LINSEN_PXI_CLK_READ_STRING, _buffer) == 0) {
					state = STATE_TX_PIXELCLOCK;
				} else if (strcmp(LINSEN_BRIGHT_READ_STRING, _buffer) == 0) {
					state = STATE_TX_BRIGHTNESS;
				} else if (strcmp(LINSEN_RESULT_READ_STRING, _buffer) == 0) {
					state = STATE_TX_RESULT;
				} else if (strcmp(LINSEN_RES_ID_READ_STRING, _buffer) == 0) {
					state = STATE_TX_RESULT_ID;
				} else if (strcmp(LINSEN_DATA_READ_STRING, _buffer) == 0) {
					state = STATE_TX_DATA;
				} else if (strcmp(LINSEN_RAW_READ_STRING, _buffer) == 0) {
					state = STATE_TX_GRAB;
				} else if ((strcmp("quit", _buffer) == 0)
					|| (strcmp("exit", _buffer) == 0) 
					|| (strcmp("q", _buffer) == 0)) { 
					state = STATE_EXIT;
				} else state = STATE_TX_UNKNOWN;
				free(_buffer);
			}
			break;
		}
		case STATE_TX_BRIGHTNESS: {
			result = linSen_get_brightness();
			
			if (result < 0) {
				printf("\tget linSen brightness failed with error %d: %s\n", errno, strerror(errno));
				return result;
			}
			char tx_str[strlen(LINSEN_BRIGHT_READ_STRING) + strlen(DEFTOSTRING(INT_MAX))];
			sprintf(tx_str, "%s%d", LINSEN_BRIGHT_READ_STRING, result);
			_buffer_size = strlen(tx_str);
			_buffer = malloc(_buffer_size);
			memcpy(_buffer, tx_str, _buffer_size);
			
			state = STATE_TX;
			break;
		}
		case STATE_TX_EXPOSURE: {
			result = linSen_get_exposure();
			
			char tx_str[strlen(LINSEN_EXP_READ_STRING) + strlen(DEFTOSTRING(INT_MAX))];
			sprintf(tx_str, "%s%d", LINSEN_EXP_READ_STRING, result);
			_buffer_size = strlen(tx_str);
			_buffer = malloc(_buffer_size);
			memcpy(_buffer, tx_str, _buffer_size);
			
			state = STATE_TX;
			break;
		}
		case STATE_TX_PIXELCLOCK: {
			result = linSen_get_pxclk();
			
			char tx_str[strlen(LINSEN_PXI_CLK_READ_STRING) + strlen(DEFTOSTRING(INT_MAX))];
			sprintf(tx_str, "%s%d", LINSEN_PXI_CLK_READ_STRING, result);
			_buffer_size = strlen(tx_str);
			_buffer = malloc(_buffer_size);
			memcpy(_buffer, tx_str, _buffer_size);
			
			state = STATE_TX;
			break;
		}
		case STATE_TX_RESULT: {
			result = linSen_get_global_result();
			
			char tx_str[strlen(LINSEN_RESULT_READ_STRING) + strlen(DEFTOSTRING(INT_MAX))];
			sprintf(tx_str, "%s%d", LINSEN_RESULT_READ_STRING, result);
			_buffer_size = strlen(tx_str);
			_buffer = malloc(_buffer_size);
			memcpy(_buffer, tx_str, _buffer_size);
			
			state = STATE_TX;
			break;
		}
		case STATE_TX_RESULT_ID: {
			result = linSen_get_result_id();
			
			char tx_str[strlen(LINSEN_RES_ID_READ_STRING) + strlen(DEFTOSTRING(INT_MAX))];
			sprintf(tx_str, "%s%d", LINSEN_RES_ID_READ_STRING, result);
			_buffer_size = strlen(tx_str);
			_buffer = malloc(_buffer_size);
			memcpy(_buffer, tx_str, _buffer_size);
			
			state = STATE_TX;
			break;
		}
		case STATE_TX_DATA: {
			result = linSen_get_data(&linSen_data);
			if (result < 0) {
				printf("\tget linSen data failed with error %d: %s\n", errno, strerror(errno));
				return result;
			}

			_buffer_size = strlen(LINSEN_DATA_READ_STRING) + sizeof(linSen_data_t);
			_buffer = malloc(_buffer_size);
			sprintf(_buffer, "%s", LINSEN_DATA_READ_STRING);
			memcpy(&_buffer[strlen(LINSEN_DATA_READ_STRING)], &linSen_data, sizeof(linSen_data_t));
			
			state = STATE_TX;
			break;
		}
		case STATE_TX_GRAB: {
			result = linSen_get_data(&linSen_data);
			if (result < 0) {
				printf("\tget linSen data failed with error %d: %s\n", errno, strerror(errno));
				return result;
			}

			int _header_size = strlen(LINSEN_RAW_READ_STRING);
			_buffer_size = _header_size + ((linSen_data.pixel_number_x * linSen_data.pixel_number_y)) * sizeof(uint16_t);
			_buffer = malloc(_buffer_size);
			sprintf(_buffer, "%s", LINSEN_RAW_READ_STRING);
			uint16_t* _payload = (uint16_t*)(_buffer + _header_size);
			result = linSen_get_raw(_payload, linSen_data.pixel_number_x * linSen_data.pixel_number_y);
			if (result) {
				state = STATE_TX;						
			} else if (result < 0) {
				printf("\tget raw linSen sensor values failed with error %d: %s\n", errno, strerror(errno));
				return result;
			}
			break;
		}
		case STATE_TX: {
			result = linSen_socket_send(accepted_socket, _buffer, _buffer_size);
			free(_buffer);
			if (result < 0) {
				printf("\tsend failed with error %d: %s\n", errno, strerror(errno));
				return result;
			} else {
				printf("\tsend %d bytes!\n", result);
				state = STATE_RX;
			}			
			break;
		}
		case STATE_TX_UNKNOWN: {
			result = linSen_socket_send_string(accepted_socket, "unknown");
			if (result < 0) {
				printf("\tsend failed with error %d: %s\n", errno, strerror(errno));
				return result;
			} else {
				printf("\tsend %d bytes!\n", result);
				state = STATE_RX;
			}			
			break;
		}
		case STATE_EXIT: {
			printf("\tconnection closed by client\n");
			// linSen_socket_server_close();
			state = STATE_WAIT;
			break;
		}
		default:;
	}
	
	return 0;
}

int linSen_socket_client_init(const char *addr) {
	// create socket
	client_socket = socket(AF_INET, SOCK_STREAM, 0);

	// prepare connection
	address.sin_family = AF_INET;
	address.sin_port = htons(15000);
	inet_aton(addr, &address.sin_addr);
	
	if (client_socket < 0) return client_socket;
	
	return connect(client_socket, (struct sockaddr *) &address, sizeof(address));
}

int linSen_socket_client_process(void) {
	return EXIT_SUCCESS;
}

int linSen_socket_client_read(char* str, int *val) {
	char* recv_str;
	int len, result;
	enum {STATE_WAIT, STATE_RECEIVED} state = STATE_WAIT;
	
	//~ printf("linSen_socket_client_read(%s,%x) called\n",str,(int)val);

	if (client_socket < 0) return -1;
	if (str == NULL) return -1;
	
	result = linSen_socket_send_string(client_socket, str);
	
	while (state == STATE_WAIT) {
		result = linSen_socket_receive(client_socket, &recv_str, &len);

		if (result < 0) {
			if (errno == EAGAIN) {
				//~ printf("sleep\n");
				usleep(10000);
			} else {
				printf("\treceive failed with error %d: %s\n", errno, strerror(errno));
				return result;
			}
		} else if (result > 0) {
			//~ printf("hope for: %s - received: %s\n", str, recv_str);
			// build header to look for
			char* search_str = str;
			int search_str_len = strlen(search_str);
			//~ printf("search for string(%d): %s\n", search_str_len, search_str);
			// parse received command
			if (strncmp(search_str, recv_str, search_str_len) == 0) {
				//~ printf("found expected header: %s\n", search_str);
				
				if (strncmp(LINSEN_DATA_READ_STRING, recv_str, search_str_len) == 0) {
					// linSen_data_t struct value read
					memcpy(val, &recv_str[search_str_len], sizeof(linSen_data_t));
					//~ linSen_data_t* linSen_data = (linSen_data_t*)val; 
					//~ printf("extracted values:");
					//~ printf("\n\texposure: %d", linSen_data->exposure);
					//~ printf("\n\tpixel_clock: %d", linSen_data->pixel_clock);
					//~ printf("\n\tbrightness: %d", linSen_data->brightness);
					//~ printf("\n\tpixel_number: %d", linSen_data->pixel_number);
					//~ printf("\n\tresult_scalar_number: %d", linSen_data->result_scalar_number);
					//~ printf("\n\tresult_id: %d", linSen_data->result_id);
					//~ printf("\n\tglobal_result: %d", linSen_data->global_result);
					//~ printf("\n");
				} else if (strncmp(LINSEN_RAW_READ_STRING, recv_str, search_str_len) == 0) {
					memcpy(val, &recv_str[search_str_len], len - 1 - search_str_len);					
				} else {
					// single value read
					*val = atoi(&recv_str[search_str_len]);
					//~ printf("extracted value: %d\n", *val);
				}
			};
						
			free(recv_str);
			state = STATE_RECEIVED;
		}
	}	
	return result;
}
	
	

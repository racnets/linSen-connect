#include <errno.h>		// errno
#include <sys/types.h>	// legacy support - may be needed for socket.h
#include <sys/socket.h>	// accept, connect, recv, send, socket
#include <netinet/in.h>	// sockaddr_in
#include <arpa/inet.h>	// htons(), inet_aton()
#include <limits.h>		// INT_MAX
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "main.h"		// verbose(), verpose_printf()

#include "linSen.h"
#include "linSen-socket.h"

// socket buffer size
#define BUF 1024
#define DEFTOSTRING(x) #x

int server_socket;
int client_socket;
int accepted_socket;

struct sockaddr_in address = {0};
socklen_t addrlen;

typedef enum {
	STATE_UNKNOWN, 
	STATE_WAIT, 
	STATE_RX,
	STATE_PARSE,
	STATE_TX_DATA,
	STATE_TX_BRIGHTNESS,
	STATE_TX_EXPOSURE, 
	STATE_TX_PIXELCLOCK,
	STATE_RX_EXPOSURE,
	STATE_RX_PIXELCLOCK,
	STATE_RX_BRIGHTNESS,
	STATE_TX_RESULT,
	STATE_TX_RESULT_ID,
	STATE_TX_GRAB, 
	STATE_TX_QP_GRAB, 
	STATE_TX_QP_FILT, 
	STATE_TX_QP_AVG, 
	STATE_TX, 
	STATE_TX_UNKNOWN, 
	STATE_EXIT
} socket_state_t;

typedef struct {
	char* identifier;
	socket_state_t state;
} parse_id_state_pair_t;

const parse_id_state_pair_t parse_id_state_pair[] = {
	{LINSEN_EXP_READ_STRING, 		STATE_TX_EXPOSURE},
	{LINSEN_PIX_CLK_READ_STRING, 	STATE_TX_PIXELCLOCK},
	{LINSEN_BRIGHT_READ_STRING,		STATE_TX_BRIGHTNESS},
	{LINSEN_EXP_WRITE_STRING,		STATE_RX_EXPOSURE},
	{LINSEN_PIX_CLK_WRITE_STRING,	STATE_RX_PIXELCLOCK},
	{LINSEN_BRIGHT_WRITE_STRING,	STATE_RX_BRIGHTNESS},
	{LINSEN_RESULT_READ_STRING,		STATE_TX_RESULT},
	{LINSEN_RES_ID_READ_STRING,		STATE_TX_RESULT_ID},
	{LINSEN_DATA_READ_STRING,		STATE_TX_DATA},
	{LINSEN_RAW_READ_STRING,		STATE_TX_GRAB},
	{LINSEN_QP_RAW_READ_STRING,		STATE_TX_QP_GRAB},
	{LINSEN_QP_AVG_READ_STRING,		STATE_TX_QP_AVG},
	{LINSEN_QP_FIL_READ_STRING,		STATE_TX_QP_FILT},
	{"quit",						STATE_EXIT},
	{"exit",						STATE_EXIT}
};
	

/* 
 * receive data using the defined device
 * 
 * waits - non-blocking - for data to receive
 * allocates memory for the data to return
 * 
 * @param dev: device
 * @param **data: data 
 * @param *len: data length
 * @return number of received bytes(success) or negative value(failure)
 */
int linSen_socket_receive(int dev, char **data, int *len) {
	debug_printf("called with dev: %d \t**data: %#x \t*len: %#x", dev, (int)data, (int)len);

	/* receive */
	char buffer[BUF];
	int result = recv(dev, buffer, BUF-1, MSG_DONTWAIT);	

	//~ if( result > 0) {
		//~ if (buffer[result] != '\0') buffer[result++] = '\0';
	//~ } else return result;

	if (result > 0) {
		*len = result;
		*data = malloc(result);
		memcpy(*data, buffer, result);
	}
		
	debug_printf("returns: %d", result);
	return result;
}

/* 
 * sends byte data using the defined device
 * 
 * @param dev: device
 * @param *str: data
 * @param len: data length
 * @return number of sent bytes(success) or -1 (failure)
 * 
 */
int linSen_socket_send(int dev, char *str, int len) {
	debug_printf("called with dev: %d \tstr: %s \tlen: %d", dev, str, len);

	if (dev < 0) return -1;

	/* send */
	int result = send(dev, str, len, 0);
	
	debug_printf("returns: %d", result);
	return result;
}

/*
 * sends strings using the defined device
 * 
 * @param dev: device
 * @param *str: string data
 * @return number of sent characters(success)(including null-termination) or -1 (failure) * 
 */
int linSen_socket_send_string(int dev, char *str) {
	debug_printf("called with dev: %d \tstr: %s", dev, str);
	
	/* send string - including null-termination */
	int result = linSen_socket_send(dev, str, strlen(str)+1);
	
	debug_printf("returns: %d", result);
	return result;
}

/*
 * close server socket
 * 
 * @return 0(success) or -1 (failure)
 */
int linSen_socket_server_close(void) {
	debug_printf("called");

	/* close server socket */
	int result = close(server_socket);

	debug_printf("returns: %d", result);
	return result;
}

/*
 * close client server
 * 
 * @return 0(success) or -1 (failure)
 */
int linSen_socket_client_close(void) {
	debug_printf("called");

	/* sent socket client the command to exit connection */ 
	if (client_socket) linSen_socket_send_string(client_socket, "exit");
	
	/* close client socket */
	int result = close(client_socket);

	debug_printf("returns: %d", result);
	return result;
}

/*
 * initialize server socket
 * 
 * @return 0(success) or -1 (failure)
 */
int linSen_socket_server_init(void) {
	debug_printf("called");

	/* create IPv4 stream socket - non-blocking */
	server_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

	if (server_socket == -1) return -1;
	
	/* set socket options */
	const int y = 1;
  	int result = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));
	if (result == -1) return -1;
	
	/* bind socket to port */
	address.sin_family = AF_INET;
	address.sin_port = htons(15000);
	address.sin_addr.s_addr = INADDR_ANY;
	/* get size of address*/
	addrlen = sizeof(address);
	result = bind(server_socket, (struct sockaddr *) &address, addrlen);
	if (result == -1) return -1;
	
	/* listen */
	result = listen(server_socket, 3);
	if (result == -1) return -1;

	//~ addrlen = sizeof(struct sockaddr_in);
	
	debug_printf("returns: %d", result);
	return result;
}	

/*
 * returns the address of the connected socket client
 * 
 * @return string of the connected socket client
 */
char* linSen_socket_get_client_address(void) {
	debug_printf("called");

	if (accepted_socket > 0) return inet_ntoa(address.sin_addr);
	else return "undefined";
}

/* 
 * waits - non-blocking - for a client to connect 
 * 
 * @return connected client address(success) or -1(failure)
 */
int linSen_socket_server_wait_for_client(void) {
	debug_printf("called");
	
	/* accept a connection on a socket */
	int result = accept(server_socket, (struct sockaddr *) &address, &addrlen);
	if (result > -1) {
		/* connected client access */
		accepted_socket = result;
	} else result = -1;
	
	debug_printf("returns: %d", result);
	return result;
}

/* TODO: comment refactorate */
/* linSen socket server process - recurrently called by main
 * 
 * @return EXIT_SUCCESS, EXIT_SUCCESS
 */
int linSen_socket_server_process(void) {
	int result;
	static socket_state_t state = STATE_WAIT;
	
	static char* _rx_buffer = NULL;
	static int _rx_buffer_size = 0;
	static char* _tx_buffer;
	static int _tx_buffer_size = 0;
	static char* parse_buffer;
	static int parsed_bytes = 0;

	debug_printf("called - state: %d", state);

	switch (state) {
		case STATE_WAIT: {
			/* wait for client to connect */
			result = linSen_socket_server_wait_for_client();
			if (result == -1) {
				if (errno == EAGAIN) {
					verbose_printf("\tretry in 1s");
					sleep(1);
					break;
				} else {
					info_printf("\tfailed with error %d: %s\n", errno, strerror(errno));
					return EXIT_FAILURE;
				}
			} else {
				info_printf("\tclient(%s) connected!\n", linSen_socket_get_client_address());
				state = STATE_RX;
			}
			/* no need for break */
		}		
		case STATE_RX: {
			/* receive bytes */
			/* if there are more unparsed bytes continue with parsing and skip receiving */
			if (parsed_bytes < _rx_buffer_size) {
				state = STATE_PARSE;
			} else {
				/* cleanup rx buffer */
				if (_rx_buffer_size > 0) {
					free(_rx_buffer);
					_rx_buffer_size = 0;
				}
				/* prepare to receive new bytes */
				result = linSen_socket_receive(accepted_socket, &_rx_buffer, &_rx_buffer_size);
				if (result < 0) {
					if (errno == EAGAIN) {
						/* wait and return */
						usleep(10000);
						break;
					} else {
						info_printf("\treceive failed with error %d: %s\n", errno, strerror(errno));
						return EXIT_FAILURE;
					}
				} else {
					/* prepare for parsing */
					debug_printf("\treceived %d bytes: \"%s\"!", _rx_buffer_size, _rx_buffer);
					state = STATE_PARSE;
					parsed_bytes = 0;
				}
			}
			/* no need for break */
		}
		case STATE_PARSE: {
			/* parse previously received bytes */
			parse_buffer = &_rx_buffer[parsed_bytes];
			/* get identifier-state array size */
			int i = sizeof(parse_id_state_pair) / sizeof(parse_id_state_pair[0]);
			/* search in identifier-state list for parsed buffer */
			
			while (--i >= 0) {
				if (strncmp(parse_id_state_pair[i].identifier, parse_buffer, strlen(parse_id_state_pair[i].identifier)) == 0) {				
					state = parse_id_state_pair[i].state;
					debug_printf("found \"%s\" - new state: %d", parse_id_state_pair[i].identifier, state);
					break;
				}
			}
			
			/* not found */
			if (i < 0) {
				verbose_printf("couldn't parse: \"%s\"", parse_buffer);
				state = STATE_TX_UNKNOWN;
				break;
			}

			/* calculate parsed bytes */
			parsed_bytes = (int)strchr(parse_buffer, '\0') - (int)_rx_buffer + 1;
						
			debug_printf("\tgot %d bytes - parsed: %d", _rx_buffer_size, parsed_bytes);
			if (parsed_bytes < _rx_buffer_size) {
				debug_printf("\tmissed: %s", &_rx_buffer[parsed_bytes]);
			} 
			
			break;
		}
		case STATE_RX_BRIGHTNESS: {
			int _val = atoi(&parse_buffer[strlen(LINSEN_BRIGHT_WRITE_STRING)]);
			verbose_printf("\treceived(%s): set birghtness to %d!", parse_buffer, _val);
			linSen_set_brightness(_val);
			
			state = STATE_RX;
			break;
		}
		case STATE_RX_EXPOSURE: {
			int _val = atoi(&parse_buffer[strlen(LINSEN_EXP_WRITE_STRING)]);
			verbose_printf("\treceived(%s): set exposure to %d!", parse_buffer, _val);
			linSen_set_exposure(_val);

			state = STATE_RX;			
			break;
		}
		case STATE_RX_PIXELCLOCK: {
			int _val = atoi(&parse_buffer[strlen(LINSEN_PIX_CLK_WRITE_STRING)]);
			verbose_printf("\treceived(%s): set pixel clock to %d!", parse_buffer, _val);
			linSen_set_pxclk(_val);

			state = STATE_RX;			
			break;
		}			
		case STATE_TX_BRIGHTNESS: {
			/* get brightness */
			verbose_printf("\treceived(%s): birghtness request", parse_buffer);
			result = linSen_get_brightness();			
			if (result < 0) {
				info_printf("\tget linSen brightness failed with error %d: %s\n", errno, strerror(errno));
				return EXIT_FAILURE;
			}
			
			/* build string */
			_tx_buffer = malloc(strlen(LINSEN_BRIGHT_READ_STRING) + strlen(DEFTOSTRING(INT_MAX)) + 1);
			sprintf(_tx_buffer, "%s%d", LINSEN_BRIGHT_READ_STRING, result);
			/* get real string length */
			_tx_buffer_size = strlen(_tx_buffer);
			
			state = STATE_TX;
			break;
		}	
		case STATE_TX_EXPOSURE: {
			/* get exposure */
			verbose_printf("\treceived(%s): exposure  request", parse_buffer);
			result = linSen_get_exposure();
			if (result < 0) {
				info_printf("\tget linSen exposure failed with error %d: %s\n", errno, strerror(errno));
				return EXIT_FAILURE;
			}
			
			/* build string */
			_tx_buffer = malloc(strlen(LINSEN_EXP_READ_STRING) + strlen(DEFTOSTRING(INT_MAX)) + 1);
			sprintf(_tx_buffer, "%s%d", LINSEN_EXP_READ_STRING, result);
			/* get real string length */
			_tx_buffer_size = strlen(_tx_buffer);
			
			state = STATE_TX;
			break;
		}
		case STATE_TX_PIXELCLOCK: {
			/* get pixel clock */
			verbose_printf("\treceived(%s): pixel clock request", parse_buffer);
			result = linSen_get_pxclk();
			if (result < 0) {
				info_printf("\tget linSen pixel clock failed with error %d: %s\n", errno, strerror(errno));
				return EXIT_FAILURE;
			}
			
			/* build string */
			_tx_buffer = malloc(strlen(LINSEN_PIX_CLK_READ_STRING) + strlen(DEFTOSTRING(INT_MAX)) + 1);
			sprintf(_tx_buffer, "%s%d", LINSEN_PIX_CLK_READ_STRING, result);
			/* get real string length */
			_tx_buffer_size = strlen(_tx_buffer);
			
			state = STATE_TX;
			break;
		}
		case STATE_TX_RESULT: {
			/* get result */
			verbose_printf("\treceived(%s): global result request", parse_buffer);			
			int _error;
			result = linSen_get_global_result(&_error);
			if (_error) {
				info_printf("\tget linSen gobal result failed with error %d: %s\n", errno, strerror(errno));
				return EXIT_FAILURE;
			}
			
			/* build string */
			_tx_buffer = malloc(strlen(LINSEN_RESULT_READ_STRING) + strlen(DEFTOSTRING(INT_MAX)) + 1);
			sprintf(_tx_buffer, "%s%d", LINSEN_RESULT_READ_STRING, result);
			/* get real string length */
			_tx_buffer_size = strlen(_tx_buffer);
			
			state = STATE_TX;
			break;
		}
		case STATE_TX_RESULT_ID: {
			/* get result ID */
			verbose_printf("\treceived(%s): result ID request", parse_buffer);			
			result = linSen_get_result_id();
			if (result < 0) {
				info_printf("\tget linSen gobal result ID failed with error %d: %s\n", errno, strerror(errno));
				return EXIT_FAILURE;
			}
			
			/* build string */
			_tx_buffer = malloc(strlen(LINSEN_RES_ID_READ_STRING) + strlen(DEFTOSTRING(INT_MAX)) + 1);
			sprintf(_tx_buffer, "%s%d", LINSEN_RES_ID_READ_STRING, result);
			/* get real string length */
			_tx_buffer_size = strlen(_tx_buffer);
			
			state = STATE_TX;
			break;
		}
		case STATE_TX_DATA: {
			/* get linSen data */
			verbose_printf("\treceived(%s): linSen data set request", parse_buffer);
			linSen_data_t linSen_data;
			result = linSen_get_data(&linSen_data);
			if (result < 0) {
				verbose_printf("\tget linSen data failed with error %d: %s\n", errno, strerror(errno));
				return EXIT_FAILURE;
			}

			/* build string */
			_tx_buffer_size = strlen(LINSEN_DATA_READ_STRING) + sizeof(linSen_data_t);
			_tx_buffer = malloc(_tx_buffer_size);
			sprintf(_tx_buffer, "%s", LINSEN_DATA_READ_STRING);
			memcpy(&_tx_buffer[strlen(LINSEN_DATA_READ_STRING)], &linSen_data, sizeof(linSen_data_t));
			
			state = STATE_TX;
			break;
		}
		case STATE_TX_GRAB: {
			/* get linSen raw data */
			verbose_printf("\treceived(%s): linSen raw data request", parse_buffer);			
			linSen_data_t linSen_data;
			result = linSen_get_data(&linSen_data);
			if (result < 0) {
				verbose_printf("\tget linSen raw data failed with error %d: %s\n", errno, strerror(errno));
				return EXIT_FAILURE;
			}

			int _header_size = strlen(LINSEN_RAW_READ_STRING);
			_tx_buffer_size = _header_size + ((linSen_data.pixel_number_x * linSen_data.pixel_number_y)) * sizeof(uint16_t);
			_tx_buffer = malloc(_tx_buffer_size);
			sprintf(_tx_buffer, "%s", LINSEN_RAW_READ_STRING);
			uint16_t* _payload = (uint16_t*)(_tx_buffer + _header_size);
			result = linSen_get_raw(_payload, linSen_data.pixel_number_x * linSen_data.pixel_number_y);
			if (result < 0) {
				verbose_printf("\tget linSen raw sensor values failed with error %d: %s\n", errno, strerror(errno));
				return EXIT_FAILURE;
			}
			
			state = STATE_TX;
			break;
		}
		case STATE_TX_QP_GRAB: {
			/* get quadPix raw data */
			int _header_size = strlen(LINSEN_QP_RAW_READ_STRING);
			_tx_buffer_size = _header_size + 4 * sizeof(uint32_t);
			_tx_buffer = malloc(_tx_buffer_size);
			sprintf(_tx_buffer, "%s", LINSEN_QP_RAW_READ_STRING);
			uint32_t* _payload = (uint32_t*)(_tx_buffer + _header_size);
			result = linSen_qp_get_raw(_payload, 4);
			if (result < 0) {
				verbose_printf("\tget raw linSen quadpix addon sensor values failed with error %d: %s\n", errno, strerror(errno));
				return EXIT_FAILURE;
			}
			
			state = STATE_TX;
			break;
		}
		case STATE_TX_QP_FILT: {
			/* get quadPix filtered data */
			int _header_size = strlen(LINSEN_QP_FIL_READ_STRING);
			_tx_buffer_size = _header_size + 4 * sizeof(uint32_t);
			_tx_buffer = malloc(_tx_buffer_size);
			sprintf(_tx_buffer, "%s", LINSEN_QP_FIL_READ_STRING);
			uint32_t* _payload = (uint32_t*)(_tx_buffer + _header_size);
			result = linSen_qp_get_filt(_payload, 4);
			if (result < 0) {
				verbose_printf("\tget filtered linSen quadpix addon sensor values failed with error %d: %s\n", errno, strerror(errno));
				return EXIT_FAILURE;
			}

			state = STATE_TX;
			break;
		}
		case STATE_TX_QP_AVG: {
			/* get quadPix average brightness data */
			result = linSen_qp_get_avg();
			if (result < 0) {
				verbose_printf("\tget linSen quadpix average brightness value failed with error %d: %s\n", errno, strerror(errno));
				return EXIT_FAILURE;
			}
			
			/* build string */
			_tx_buffer = malloc(strlen(LINSEN_QP_AVG_READ_STRING) + strlen(DEFTOSTRING(INT_MAX)) + 1);
			sprintf(_tx_buffer, "%s%d", LINSEN_QP_AVG_READ_STRING, result);
			/* get real string length */
			_tx_buffer_size = strlen(_tx_buffer);
			
			state = STATE_TX;
			break;
		}
		case STATE_TX: {
			/* send */
			result = linSen_socket_send(accepted_socket, _tx_buffer, _tx_buffer_size);
			free(_tx_buffer);
			if (result < 0) {
				verbose_printf("\tsend failed with error %d: %s\n", errno, strerror(errno));
				return EXIT_FAILURE;
			} else {
				verbose_printf("\tsend %d bytes!", result);
				state = STATE_RX;
			}			
			break;
		}
		case STATE_TX_UNKNOWN: {
			/* unknown state */
			result = linSen_socket_send_string(accepted_socket, "unknown");
			if (result < 0) {
				verbose_printf("\tsend failed with error %d: %s\n", errno, strerror(errno));
				return EXIT_FAILURE;
			} else {
				verbose_printf("\tsend %d bytes: \"unknown\" !", result);
				state = STATE_RX;
			}			
			break;
		}
		case STATE_EXIT: {
			/* exit */
			verbose_printf("\tconnection closed by client\n");
			//~ linSen_socket_server_close();
			state = STATE_WAIT;
			break;
		}
		default:;
	}
	
	debug_printf("returns: %d\tstate: %d", state, EXIT_SUCCESS);
	return EXIT_SUCCESS;
}

/*
 * socket client initialization
 * 
 * @return EXIT_SUCCESS, EXIT_FAILURE
 */ 
int linSen_socket_client_init(const char *addr) {
	debug_printf("called with addr: %s", addr);

	/* create IPv4 stream socket */
	client_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (client_socket < 0) {
		info_printf("failed with error %d: %s\n", errno, strerror(errno));
		return EXIT_FAILURE;
	}
	
	/* prepare connection */
	address.sin_family = AF_INET; /* IPv$ */
	address.sin_port = htons(15000); /* port */
	/* converts address to binary */
	if (inet_aton(addr, &address.sin_addr) == 0) {
 		info_printf("invalid address %s - error: %d: %s\n", addr, errno, strerror(errno));
 		return EXIT_FAILURE;
	}
	
	/* connect */
	if (connect(client_socket, (struct sockaddr *) &address, sizeof(address)) != 0) {
 		info_printf("failed to connect to %s - error: %d: %s\n", addr, errno, strerror(errno));
		return EXIT_FAILURE;
	};
	
	debug_printf("returns: %d", EXIT_SUCCESS);	
	return EXIT_SUCCESS;
}

/*
 * read linSen data via socket
 * 
 * @param *str: data request string identifier as defined in linSen.h
 * @param *data: data to read to
 * @param size: expected data size
 * @return EXIT_SUCCESS, EXIT_FAILURE
 */
int linSen_socket_read(char *str, int *data, int size) {
	char* recv_str;
	int recv_len;
	int result;
	
	debug_printf("called with str: %s \t*data: %#x \tsize: %d", str, (int)data, size);

	/* check for valid socket client connection */
	if (client_socket < 0) {
		info_printf("no socket client defined!");
		return EXIT_FAILURE;
	}

	/* check for request string */
	if (str == NULL) {
		info_printf("no data request string defined!");
		return EXIT_FAILURE;
	}
	
	/* sent request string */
	result = linSen_socket_send_string(client_socket, str);
	if (result != (strlen(str)+1)) {
		info_printf("failed requesting: %s!", str);
		return EXIT_FAILURE;
	}
	
	/* wait for receive */
	do {
		/* receive data via socket - non-blocking */
		result = linSen_socket_receive(client_socket, &recv_str, &recv_len);
		if ((result < 0) && (errno == EAGAIN)) {
			/* retry */
			debug_printf("sleep - wait for reception of %s", str);
			usleep(10000);
		} else if (result <= 0) {
			info_printf("\treceive failed with error %d: %s\n", errno, strerror(errno));
			return EXIT_FAILURE;
		}
	} while (result < 0);
	
	/* build header to compare against */
	char *search_str = str;
	int search_str_len = strlen(search_str);
	debug_printf("search for string(%d): %s", search_str_len, search_str);
	
	/* compare expected header against received header */
	if (strncmp(search_str, recv_str, search_str_len) != 0) {
		/* header mismatch */
		int rec_str_len = (search_str_len < recv_len)? search_str_len: recv_len;
		info_printf("header mismatch - found: %.*s \texpected: %s", rec_str_len, recv_str, search_str);
		
		/* clean up */
		free(recv_str);
		
		return EXIT_FAILURE;
	}
		
	/* compare header against known values in order to extract payload */
	if ((strncmp(LINSEN_DATA_READ_STRING, recv_str, search_str_len) == 0) ||
		(strncmp(LINSEN_RAW_READ_STRING, recv_str, search_str_len) == 0) || 
		(strncmp(LINSEN_QP_RAW_READ_STRING, recv_str, search_str_len) == 0) ||
		(strncmp(LINSEN_QP_FIL_READ_STRING, recv_str, search_str_len) == 0)) 
	{
		/* compare expected size against received data size */
		if (size == (recv_len - search_str_len)) { 
			verbose_printf("read: %s \textracted %d bytes", search_str, size);
			/* extract linSen data struct */
			memcpy(data, &recv_str[search_str_len], size);
		} else {
			info_printf("data size mismatch - got: %d \texpected: %d", recv_len - search_str_len, size);
			return EXIT_FAILURE;
		}
	} else {
		/* single value read */
		*data = atoi(&recv_str[search_str_len]);
		verbose_printf("read: %s \textracted value: %d", search_str, *data);
	}
	
	/* clean up */
	free(recv_str);

	debug_printf("returns: %d", EXIT_SUCCESS);	
	return EXIT_SUCCESS;
}

/*
 * write linSen single integer data via socket - human read-able 
 * 
 * @param *str: data write string identifier as defined in linSen.h
 * @param data: data to write
 * @return EXIT_SUCCESS, EXIT_FAILURE
 */
	
int linSen_socket_write_int(const char* str, int data) {
	int result = EXIT_SUCCESS;

	debug_printf("called with str: %s \tdata: %d", str, data);

	/* check for valid socket client connection */
	if (client_socket < 0) {
		info_printf("no socket client defined!");
		return EXIT_FAILURE;
	}

	/* check for request string */
	if (str == NULL) {
		info_printf("no data request string defined!");
		return EXIT_FAILURE;
	}

	/* build string to send */
	char *tx_str = malloc(strlen(str) + strlen(DEFTOSTRING(INT_MAX)));
	sprintf(tx_str, "%s%d", str, data);

	debug_printf("build tx_str: %s", tx_str);

	/* send string */
	result = linSen_socket_send_string(client_socket, tx_str);
	if (result != (strlen(tx_str) + 1)) {
		info_printf("\tfailed with error %d: %s\n", errno, strerror(errno));
		result = EXIT_FAILURE;
	} else {
		verbose_printf("\tsuccessfully send %s!", tx_str);
	}

	/* clean up */
	free(tx_str);
	
	debug_printf("returns: %d", result);	
	return result;
}
	

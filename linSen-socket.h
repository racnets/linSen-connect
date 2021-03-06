/*
 * linSen-socket.h
 *
 * Last modified: 18.06.2017
 *
 * Author: racnets 
 */

#ifndef LINSEN_SOCKET_H_
#define LINSEN_SOCKET_H_

#include <stdint.h>

int linSen_socket_receive(int dev, char** val, int* len);
int linSen_socket_send(int dev, char* val, int len);
int linSen_socket_send_string(int dev, char* val);

int linSen_socket_server_init(void);
int linSen_socket_server_close(void);
int linSen_socket_server_process(void);
int linSen_socket_server_wait_for_client(void);
char* linSen_socket_get_client_address(void);

int linSen_socket_client_init(const char *addr);
int linSen_socket_client_close(void);

int linSen_socket_read(char *str, int *data, int size);
int linSen_socket_write_int(const char* str, int data);

#endif /* LINSEN_SOCKET_H_ */

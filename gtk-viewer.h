/*
 * gtk-viewer.h
 * 
 * Last modified: 18.06.2017
 *
 * Author: racnets 
 */

#ifndef GTK_VIEWER_H
#define GTK_VIEWER_H

int viewer_want_linSen_raw(void);
int viewer_set_linSen_raw(const uint16_t *img, int width, int height);
int viewer_set_linSen_data(linSen_data_t *data);
int viewer_update(void);
int viewer_init(int *argc, char **argv[]);


int viewer_want_quadPix_raw(void);
int viewer_set_quadPix_raw(const uint32_t *data, int width, int height);
int viewer_want_quadPix_result(void);
int viewer_set_quadPix_result(const int32_t *data, int width, int height);

	
#endif /* GTK_VIEWER_H */

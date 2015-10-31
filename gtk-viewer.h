/*
 * gtk-viewer.h
 */

#ifndef GTK_VIEWER_H
#define GTK_VIEWER_H

int viewer_set_image(const uint16_t* img, int width, int height);
int viewer_set_data(linSen_data_t *data);
void viewer_update(void);
int viewer_init(int *argc, char **argv[]);
	
#endif /* GTK_VIEWER_H */

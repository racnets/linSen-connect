#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>	// XintY_t
#include <string.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <cairo.h>

#include "gtk-viewer.h"

cairo_surface_t *image = NULL;
GtkWidget *window;
GtkWidget *darea;

#define vprintf(s) printf(s)
static gboolean on_draw_event(GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {      
	//~ printf("on_draw_event called\n");
	cairo_t *cr;
	cr = gdk_cairo_create(widget->window);

	int width, height;
	gtk_window_get_size(GTK_WINDOW(window), &width, &height);

	double scale, scale_width, scale_height, origin_x = 0, origin_y = 0;
	int image_height, image_width;
	image_width = cairo_image_surface_get_width(image);
	image_height = cairo_image_surface_get_height(image);
	scale_width = (float)width / image_width;
	scale_height = (float)height / image_height;
	if (scale_width < scale_height) {
		scale = scale_width;
		origin_y = (height-image_height)/2.0/scale;
	} else {
		scale = scale_height;
		origin_x = (width-image_width)/2.0/scale;
	}

	//~ printf("scale %f (%f/%f\n", scale, scale_width, scale_height);
	//~ printf("origin %f/%f\n", origin_x, origin_y);
	//~ printf("size %d, %d\n", width, height);
	//~ printf("image size %d, %d\n",image_width, image_height);
	
	if (image != NULL) {
		//~ printf("cairo status: %s\n", cairo_status_to_string(cairo_surface_status(image)));
		//~ cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);

		cairo_scale(cr, scale, scale);
		cairo_set_source_surface(cr, image, origin_x, origin_y);
		cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_FAST);
  		cairo_paint(cr);
	} else {
		//~ printf("no image data to show\n");
		cairo_set_source_rgb(cr, 0, 0, 0); 
		cairo_select_font_face(cr, "Arial", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

		cairo_set_font_size(cr, 16);
		cairo_move_to(cr, 20, 100);
		cairo_show_text(cr, "no image data");  
	}
	cairo_destroy(cr);

	return TRUE;
}

static gboolean on_destroy_event(GtkWidget *widget, GdkEventExpose *event, gpointer user_data) {
	//~ gtk_main_quit ();

    cairo_surface_destroy(image);
	gtk_widget_destroy(window);    	
	return TRUE;
}


int viewer_init(int *argc, char **argv[]) {
	// initialize gtk
	gtk_init(argc, argv);

	// create window
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(window, "destroy", G_CALLBACK(on_destroy_event), NULL);
    
	// create drawing area
	GtkWidget *darea;
	darea = gtk_drawing_area_new();
	// add area to window
	gtk_container_add(GTK_CONTAINER(window), darea);	
	// connect events - "draw" for gtk3
	g_signal_connect(G_OBJECT(darea), "expose-event", G_CALLBACK(on_draw_event), NULL); 

	// show window
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 400, 400); 
	gtk_window_set_title(GTK_WINDOW(window), *argv[0]);  	
	gtk_widget_show_all(window);

	return EXIT_SUCCESS;
}

void viewer_update() {
	//~ gtk_main();
	while (gtk_events_pending())
		gtk_main_iteration();
}

int viewer_set_image(const uint16_t* img, int width, int height) {
	// if window is destroyed return
	if (!window->window) return EXIT_FAILURE;

	// setup image
	if (image != NULL) {
	    cairo_surface_destroy(image);
	}
	
	// get valid stride for cairo
	int stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB30, width);
	
	// reorganize memory - convert 10bit grayscale to RGB30 stored in 32bit
	static uint32_t* frame;
	frame = malloc(height*stride); 
	int i;
	for (i=0;i<(height*width);i++) {
		// truncate to 16bit to 12bit and reduce to 10bit
		uint16_t p = (img[i] & 0x0FFF) >> 2;
		// convert 10bit grey to RGB30-grey;
		frame[i] = (p << 20) | (p << 10) | p;
		//~ printf("%d %d %d\t%d: %x %x\n", width, height, stride, img[i], p, frame[i]);
	}		
	
    image = cairo_image_surface_create_for_data((unsigned char*)frame, CAIRO_FORMAT_RGB30, width, height, stride);
		
	// force redraw
	gtk_widget_queue_draw(window);
	return EXIT_SUCCESS;
}

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>	// XintY_t
#include <string.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <cairo.h>

#include "linSen.h"
#include "gtk-viewer.h"

#define DEFTOSTRING(x) #x

cairo_surface_t *image = NULL;
GtkWidget* window;

GtkAdjustment* exposure_adj;
GtkAdjustment* pixel_clock_adj;
GtkAdjustment* brightness_adj;
GtkAdjustment* result_adj;
GtkWidget* exposure_slider;
GtkWidget* pixel_clock_slider;
GtkWidget* brightness_slider;
GtkWidget* frame_label;    
GtkWidget* size_label;    
GtkWidget* local_result_number_label;
GtkWidget* result_slider;


linSen_data_t linSen_data;

//~ GtkWidget* darea;

static gboolean on_draw_event(GtkWidget* widget, GdkEventExpose *event, gpointer user_data) {      
	//~ printf("on_draw_event called\n");
	cairo_t *cr;
	cr = gdk_cairo_create(widget->window);

	int width, height;
	width = widget->allocation.width;
	height = widget->allocation.height;

	double scale, scale_width, scale_height, origin_x = 0, origin_y = 0;
	int image_height, image_width;
	image_width = cairo_image_surface_get_width(image);
	image_height = cairo_image_surface_get_height(image);
	scale_width =(float)width / image_width;
	scale_height =(float)height / image_height;
	if(scale_width < scale_height) {
		scale = scale_width;
		origin_y =(height-image_height)/2.0/scale;
	} else {
		scale = scale_height;
		origin_x =(width-image_width)/2.0/scale;
	}

	//~ printf("scale %f(%f/%f\n", scale, scale_width, scale_height);
	//~ printf("origin %f/%f\n", origin_x, origin_y);
	//~ printf("size %d, %d\n", width, height);
	//~ printf("image size %d, %d\n",image_width, image_height);
	
	if(image != NULL) {
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

static gboolean on_destroy_event(GtkWidget* widget, GdkEventExpose *event, gpointer user_data) {
	//~ gtk_main_quit();

    //~ cairo_surface_destroy(image);
	gtk_widget_destroy(window);    	
	return TRUE;
}

static void cb_exposure_adj_value_change(GtkAdjustment* adjustment) {
	int value = (int)gtk_adjustment_get_value(adjustment);
	if (linSen_data.exposure != value) linSen_set_exposure(value);
}

static void cb_pixel_clock_adj_value_change(GtkAdjustment* adjustment) {
	int value = (int)gtk_adjustment_get_value(adjustment);
	if (linSen_data.pixel_clock != value) linSen_set_pxclk(value);
}

static void cb_brightness_adj_value_change(GtkAdjustment* adjustment) {
	int value = (int)gtk_adjustment_get_value(adjustment);
	if (linSen_data.brightness != value) linSen_set_brightness(value);
}

int viewer_init(int *argc, char **argv[]) {
	// initialize gtk
	gtk_init(argc, argv);

	// create window
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(window, "destroy", G_CALLBACK(on_destroy_event), NULL);
    
    GtkWidget* label;
   	GtkWidget* darea;
    GtkWidget* hbox;
    GtkWidget* vbox;
    GtkWidget* vvbox;
    GtkWidget* box;
    GtkWidget* hSeparator;
    GtkWidget* spin_button;
    GtkWidget* button;
    GtkWidget* grid;

    const int spacing = 18;
    const int small_spacing = 6;
    
    //~ gtk_...box_new(homogeneous, spacing);
    //~ gtk_box_pack_...(*box, *child, expand, fill, padding);
	//~ gtk_adjustment_new(gdouble value, gdouble lower, gdouble upper, gdouble step_increment, gdouble page_increment, gdouble page_size);
	// global container
	box = gtk_vbox_new(FALSE, 0);	
	gtk_container_add(GTK_CONTAINER(window), box);	


	// 1st grid: linSen setting values
	grid = gtk_widget_new(GTK_TYPE_TABLE, "n-columns", 6, "n-rows", 3, "column-spacing", 0, "row-spacing", 0, FALSE);
    gtk_box_pack_start(GTK_BOX(box), grid, FALSE, FALSE, small_spacing);
	
	// 1st column: labels
	label = gtk_widget_new(GTK_TYPE_LABEL, "label", "exposure:", "xalign", 1.0, NULL);
    gtk_table_attach(GTK_TABLE(grid), label, 0, 1, 0, 2, GTK_FILL, GTK_SHRINK, small_spacing, 0);
	label = gtk_widget_new(GTK_TYPE_LABEL, "label", "pixel clock:", "xalign", 1.0, NULL);
    gtk_table_attach(GTK_TABLE(grid), label, 0, 1, 2, 4, GTK_FILL, GTK_SHRINK, small_spacing, 0);
	label = gtk_widget_new(GTK_TYPE_LABEL, "label", "average brightness:", "xalign", 1.0, NULL);
    gtk_table_attach(GTK_TABLE(grid), label, 0, 1, 4, 6, GTK_FILL, GTK_SHRINK, small_spacing, 0);

	// 2nd column: value fields & slider
    spin_button = gtk_widget_new(GTK_TYPE_SPIN_BUTTON, "digits", 0, "numeric", 1, "width-chars", 6, "xalign", 1.0, NULL);
	exposure_adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(spin_button));
    g_signal_connect(exposure_adj, "value_changed", G_CALLBACK(cb_exposure_adj_value_change), NULL);
    gtk_table_attach(GTK_TABLE(grid), spin_button, 1, 2, 0, 1, GTK_FILL, GTK_SHRINK, small_spacing, 0);
	exposure_slider = gtk_widget_new(GTK_TYPE_HSCALE, "digits", 0, "draw-value", 0, NULL);
	gtk_range_set_adjustment(GTK_RANGE(exposure_slider), exposure_adj);
    gtk_table_attach(GTK_TABLE(grid), exposure_slider, 1, 3, 1, 2, GTK_FILL, GTK_SHRINK, 0, 0);

    spin_button = gtk_widget_new(GTK_TYPE_SPIN_BUTTON, "digits", 0, "numeric", 1, "width-chars", 6, "xalign", 1.0, NULL);
	pixel_clock_adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(spin_button));
    g_signal_connect(pixel_clock_adj, "value_changed", G_CALLBACK(cb_pixel_clock_adj_value_change), NULL);
    gtk_table_attach(GTK_TABLE(grid), spin_button, 1, 2, 2, 3, GTK_FILL, GTK_SHRINK, small_spacing, 0);
	pixel_clock_slider = gtk_widget_new(GTK_TYPE_HSCALE, "digits", 0, "draw-value", 0, NULL);
	gtk_range_set_adjustment(GTK_RANGE(pixel_clock_slider), pixel_clock_adj);
    gtk_table_attach(GTK_TABLE(grid), pixel_clock_slider, 1, 3, 3, 4, GTK_FILL, GTK_SHRINK, 0, 0);

    spin_button = gtk_widget_new(GTK_TYPE_SPIN_BUTTON, "digits", 0, "numeric", 1, "width-chars", 6, "xalign", 1.0, NULL);
	brightness_adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(spin_button));
	gtk_adjustment_configure(brightness_adj, 0, 0, 0x0FFF, 1, 0, 0);
    g_signal_connect(brightness_adj, "value_changed", G_CALLBACK(cb_brightness_adj_value_change), NULL);
    gtk_table_attach(GTK_TABLE(grid), spin_button, 1, 2, 4, 5, GTK_FILL, GTK_SHRINK, small_spacing, 0);
	brightness_slider = gtk_widget_new(GTK_TYPE_HSCALE, "digits", 0, "draw-value", 0, NULL);
	gtk_range_set_adjustment(GTK_RANGE(brightness_slider), brightness_adj);
    gtk_table_attach(GTK_TABLE(grid), brightness_slider, 1, 3, 5, 6, GTK_FILL, GTK_SHRINK, 0, 0);
    
	// 3rd column: units
	label = gtk_widget_new(GTK_TYPE_LABEL, "label", "µs", "xalign", 0, NULL);
    gtk_table_attach(GTK_TABLE(grid), label, 2, 3, 0, 1, GTK_FILL, GTK_SHRINK, small_spacing, 0);
	label = gtk_widget_new(GTK_TYPE_LABEL, "label", "kHz", "xalign", 0, NULL);
    gtk_table_attach(GTK_TABLE(grid), label, 2, 3, 2, 3, GTK_FILL, GTK_SHRINK, small_spacing, 0);

	hSeparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(box), hSeparator, FALSE, FALSE, 0);
   

	// 2nd box: linSen output
	// create drawing area
	darea = gtk_drawing_area_new();
	// add area to window
    gtk_box_pack_start(GTK_BOX(box), darea, TRUE, TRUE, 0);

	hSeparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(box), hSeparator, FALSE, FALSE, 0);
    
    
    // 3rd box: results
	result_slider = gtk_widget_new(GTK_TYPE_HSCALE, "digits", 0, "draw-value", 1, "sensitive", 0, "show-fill-level", 1, NULL);
	gtk_scale_add_mark(GTK_SCALE(result_slider), 0, GTK_POS_TOP, NULL);
	gtk_scale_add_mark(GTK_SCALE(result_slider), 0, GTK_POS_BOTTOM, NULL);
	result_adj = gtk_range_get_adjustment(GTK_RANGE(result_slider));
	gtk_adjustment_configure(result_adj, 0, -10, 10, 1, 0, 0);
    gtk_box_pack_start(GTK_BOX(box), result_slider, FALSE, TRUE, small_spacing);

    //~ gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 0);

	hSeparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(box), hSeparator, FALSE, FALSE, 0);

    
    // 4rd box: control button(s)
	hbox = gtk_hbox_new(FALSE, 0);
	button = gtk_widget_new(GTK_TYPE_BUTTON, "label", "quit", NULL);
    g_signal_connect(button, "clicked", G_CALLBACK(on_destroy_event), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, small_spacing);

	hSeparator = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(box), hSeparator, FALSE, FALSE, 0);

    
    // 5rd box: helper values
	hbox = gtk_hbox_new(FALSE, 0);
	frame_label = gtk_widget_new(GTK_TYPE_LABEL, "label", "0", "xalign", 0, NULL);
    gtk_box_pack_start(GTK_BOX(hbox), frame_label, FALSE, FALSE, small_spacing);

	size_label = gtk_widget_new(GTK_TYPE_LABEL, "label", "0", "xalign", 0, NULL);
    gtk_box_pack_start(GTK_BOX(hbox), size_label, FALSE, FALSE, small_spacing);

	local_result_number_label = gtk_widget_new(GTK_TYPE_LABEL, "label", "0", "xalign", 0, NULL);
    gtk_box_pack_start(GTK_BOX(hbox), local_result_number_label, FALSE, FALSE, small_spacing);

    gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 0);


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
	while(gtk_events_pending())
		gtk_main_iteration();
}

int viewer_set_image(const uint16_t* img, int width, int height) {
	// if window is destroyed return
	if(!window->window) return EXIT_FAILURE;

	// setup image
	if(image != NULL) {
	    cairo_surface_destroy(image);
	}
	
	// get valid stride for cairo
	int stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB30, width);
	
	// reorganize memory - convert 10bit grayscale to RGB30 stored in 32bit
	static uint32_t* frame;
	frame = malloc(height*stride); 
	int i;
	for(i=0;i<(height*width);i++) {
		// truncate to 16bit to 12bit and reduce to 10bit
		uint16_t p =(img[i] & 0x0FFF) >> 2;
		// convert 10bit grey to RGB30-grey;
		frame[i] = (p << 20) | (p << 10) | p;
		//~ printf("%d %d %d\t%d: %x %x\n", width, height, stride, img[i], p, frame[i]);
	}		
	
    image = cairo_image_surface_create_for_data((unsigned char*)frame, CAIRO_FORMAT_RGB30, width, height, stride);
		
	// force redraw
	gtk_widget_queue_draw(window);
	return EXIT_SUCCESS;
}

int viewer_set_data(linSen_data_t* data) {
	static linSen_data_t linSen_data_max;

	// if window is destroyed return
	if(!window->window) return EXIT_FAILURE;
	
	memcpy(&linSen_data, data, sizeof(linSen_data_t));
	
	if (linSen_data.exposure > linSen_data_max.exposure) {
		linSen_data_max.exposure = linSen_data.exposure;
		gtk_adjustment_configure(exposure_adj, linSen_data.exposure, 0, 1.2*linSen_data_max.exposure, 1, 0, 0);
		//~ gtk_scale_clear_marks(GTK_SCALE(exposure_slider));
		//~ gtk_scale_add_mark(GTK_SCALE(exposure_slider), linSen_data_max.exposure, GTK_POS_TOP, NULL);
	} else gtk_adjustment_set_value(exposure_adj, linSen_data.exposure);
	
	if (linSen_data.pixel_clock > linSen_data_max.pixel_clock) {
		linSen_data_max.pixel_clock = linSen_data.pixel_clock;
		gtk_adjustment_configure(pixel_clock_adj, linSen_data.pixel_clock, 0, 1.2*linSen_data_max.pixel_clock, 1, 0, 0); 
		//~ gtk_scale_clear_marks(GTK_SCALE(pixel_clock_slider));
		//~ gtk_scale_add_mark(GTK_SCALE(pixel_clock_slider), linSen_data_max.pixel_clock, GTK_POS_TOP, NULL);
	} else gtk_adjustment_set_value(pixel_clock_adj, linSen_data.pixel_clock);
	
	gtk_adjustment_set_value(brightness_adj, linSen_data.brightness);

	// result
	gtk_adjustment_configure(result_adj, linSen_data.global_result, -linSen_data.pixel_number_x/2, +linSen_data.pixel_number_x/2, 1, 0, 0);

	// frame number
	char _frame_label_str[sizeof("frame: ") + strlen(DEFTOSTRING(INT_MAX))];
	sprintf(_frame_label_str, "frame: %d", linSen_data.result_id);
	gtk_label_set_text(GTK_LABEL(frame_label), _frame_label_str);
	
	// sensor size
	char _size_label_str[sizeof("size:  x ") + 2 * strlen(DEFTOSTRING(INT_MAX))];
	sprintf(_size_label_str, "size: %d x %d", linSen_data.pixel_number_x, linSen_data.pixel_number_y);
	gtk_label_set_text(GTK_LABEL(size_label), _size_label_str);
	
	// local result number
	char _local_result_number_label_str[sizeof("number of local result values: ") + strlen(DEFTOSTRING(INT_MAX))];
	sprintf(_local_result_number_label_str, "number of local result values: %d", linSen_data.result_scalar_number);
	gtk_label_set_text(GTK_LABEL(local_result_number_label), _local_result_number_label_str);
	
	
	return EXIT_SUCCESS;
}

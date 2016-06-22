#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>	// XintY_t
#include <string.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <cairo.h>

#include "main.h"  //verbose(), verpose_printf()

#include "linSen.h"
#include "gtk-viewer.h"

#define DEFTOSTRING(x) #x

GtkWidget* window;

GtkAdjustment* exposure_adj;
GtkAdjustment* pixel_clock_adj;
GtkAdjustment* brightness_adj;
GtkAdjustment* result_adj;
GtkWidget* exposure_toggle;
GtkWidget* pixel_clock_toggle;
GtkWidget* brightness_toggle;
GtkWidget* frame_label;    
GtkWidget* size_label;    
GtkWidget* local_result_number_label;
GtkWidget* result_toggle;
GtkWidget* status_bar_1;
GtkWidget* status_bar_2;
GtkWidget* status_bar_3;
GtkWidget* quadPix_raw_check;
GtkWidget* quadPix_result_check;

linSen_data_t linSen_data;

cairo_surface_t *linSen_raw_image = NULL;
int linSen_raw_request = FALSE;

cairo_surface_t *quadPix_raw_image = NULL;
int quadPix_raw_request = FALSE;


cairo_surface_t *quadPix_result_image = NULL;
int quadPix_result_request = FALSE;

#define PLOTTER_BUFFER 256
#define PLOTTER_ITEMS  4
struct {
	int value[PLOTTER_ITEMS][PLOTTER_BUFFER];
	int write_pos;
} plot_data;

/* 
 * helper
 */
int viewer_want_linSen_raw(void) {
	return (linSen_raw_request == TRUE);
}

int viewer_want_quadPix_raw(void) {
	return (quadPix_raw_request == TRUE);
}

int viewer_want_quadPix_result(void) {
	return (quadPix_result_request == TRUE);
}

static void viewer_draw_image(GtkWidget* widget, cairo_t *cr, cairo_surface_t *image) {
	int width = gtk_widget_get_allocated_width(widget);
	int height = gtk_widget_get_allocated_height(widget);

	if(image != NULL) {
		double scale, scale_width, scale_height;
		//~ double origin_x = 0, origin_y = 0;
		int image_height, image_width;
		image_width = cairo_image_surface_get_width(image);
		image_height = cairo_image_surface_get_height(image);
		scale_width =(float)width / image_width;
		scale_height =(float)height / image_height;
		if(scale_width < scale_height) {
			scale = scale_width;
			//~ origin_y =(height-image_height)/2.0/scale;
		} else {
			scale = scale_height;
			//~ origin_x =(width-image_width)/2.0/scale;
		}
		//~ verbose_printf("scale %f(%f/%f", scale, scale_width, scale_height);
		//~ verbose_printf("origin %f/%f", origin_x, origin_y);
		//~ verbose_printf("size %d, %d", width, height);
		//~ verbose_printf("image size %d, %d",image_width, image_height);
		//~ verbose_printf("cairo status: %s", cairo_status_to_string(cairo_surface_status(image)));
		//~ cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);

		cairo_scale(cr, scale, scale);
		//~ cairo_set_source_surface(cr, image, origin_x, origin_y);
		cairo_set_source_surface(cr, image, 0, 0);
		cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_FAST);
  		cairo_paint(cr);
	} else {
		verbose_printf("no image data to show");
		cairo_set_source_rgb(cr, 255, 255, 255); 
		cairo_select_font_face(cr, "Arial", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

		cairo_set_font_size(cr, 16);
		cairo_move_to(cr, 20, 100);
		cairo_show_text(cr, "no image data");
		cairo_set_source_rgb(cr, 255, 255, 0); 
		cairo_paint(cr);
	}
	//~ cairo_destroy(cr);
}

/*
 * 
 */
static gboolean linSen_raw_draw_event(GtkWidget* widget, cairo_t *cr, gpointer user_data) {      
	linSen_raw_request = TRUE;

	viewer_draw_image(widget, cr, linSen_raw_image);
	
	return TRUE;
}

static void do_plot(cairo_t *cr, cairo_pattern_t *pattern, int index, double scale_w, int height) {
	int max = 0;
	int min = 0;	
	int i;
	// finds extrema
	for (i=0;i<PLOTTER_BUFFER;i++){
		int date = plot_data.value[index][i];
		if (date > max) max = date;
		else if (date < min) min = date;
	}

	// to calculate horizontal scale and zero-line
	double scale_h = (max == min)? 1.0: (double)height / (max - min);
	int neutral = max * scale_h;

	cairo_set_source(cr, pattern);
	cairo_set_line_width(cr,1);
	cairo_save(cr);
	
	// rotate and scale
	cairo_matrix_t matrix;
	cairo_matrix_init(&matrix,
		scale_w, 0,
		0, -scale_h,
		0, neutral
	);
	cairo_transform(cr, &matrix);

	// plot
	for (i=0;i<PLOTTER_BUFFER;i++){
		int pos = plot_data.write_pos - i;
		if (pos < 0) pos += PLOTTER_BUFFER;
		int date = plot_data.value[index][pos];
		cairo_line_to(cr, i, date);
	}
	cairo_restore(cr);
	cairo_stroke(cr);
}

static gboolean plotter_on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data) {      
	int width = gtk_widget_get_allocated_width(widget);
	int height = gtk_widget_get_allocated_height(widget);
	verbose_printf("width/height: %d/%d", width, height);
	double scale_w = (double)width / PLOTTER_BUFFER;

	// white background
	cairo_set_source_rgb(cr, 255, 255, 255); 
	cairo_paint(cr);

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(exposure_toggle))) do_plot(cr, cairo_pattern_create_rgb(1,0,0), 0, scale_w, height);
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pixel_clock_toggle))) do_plot(cr, cairo_pattern_create_rgb(0,1,0), 1, scale_w, height);
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(brightness_toggle))) do_plot(cr, cairo_pattern_create_rgb(0,0,1), 2, scale_w, height);
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(result_toggle))) do_plot(cr, cairo_pattern_create_rgb(0,0,0), 3, scale_w, height);

	return TRUE;
}

static gboolean quadPix_raw_on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
	quadPix_raw_request = TRUE;

	viewer_draw_image(widget, cr, quadPix_raw_image);
	
	return TRUE;
}

static gboolean quadPix_result_on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
	quadPix_result_request = TRUE;

	viewer_draw_image(widget, cr, quadPix_result_image);
	
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

/*
 * initialize GUI
 * 
 * @return EXIT_SUCCESS, EXIT_FAILURE
 */
int viewer_init(int *argc, char **argv[]) {
	GtkBuilder *builder;
	GObject *win;
	GObject *tmp_obj;
	
	/* initialize gtk */
	gtk_init(argc, argv);
	
	/* Construct a GtkBuilder instance and load UI description */
	builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, "linSen-connect.ui", NULL);
	
	/*  CSS */
	GtkCssProvider *provider = gtk_css_provider_new();
	GdkDisplay *display = gdk_display_get_default();
	GdkScreen *screen = gdk_display_get_default_screen(display);

	gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	const gchar* css_path = "gtk-viewer.css";
	GError *error = 0;	
	gtk_css_provider_load_from_path(GTK_CSS_PROVIDER(provider), css_path, &error);
	if (error) printf("error loading css \"%s\": %s\n", css_path, error->message);
	
	g_object_unref(provider);
	

	/* Connect signal handlers and get adjustments to the constructed widgets. */
	/* main window */
	win = gtk_builder_get_object(builder, "window");
	window = GTK_WIDGET(win);
	g_signal_connect(win, "destroy", G_CALLBACK(gtk_widget_destroyed), &window);
	
	/* exposure slider and spin-button */
	tmp_obj = gtk_builder_get_object(builder, "exp_adjust");
	exposure_adj = GTK_ADJUSTMENT(tmp_obj);
	gtk_adjustment_configure(exposure_adj, LINSEN_MIN_EXP, LINSEN_MIN_EXP, LINSEN_MAX_EXP, 1, 0, 0);
	g_signal_connect(exposure_adj, "value_changed", G_CALLBACK(cb_exposure_adj_value_change), NULL);	
	
	/* pixel clock slider and spin-button */
	tmp_obj = gtk_builder_get_object(builder, "pix-clk_adjust");
	pixel_clock_adj = GTK_ADJUSTMENT(tmp_obj);
	gtk_adjustment_configure(pixel_clock_adj, LINSEN_MIN_PIX_CLK, LINSEN_MIN_PIX_CLK, LINSEN_MAX_PIX_CLK, 1, 0, 0);
	g_signal_connect(pixel_clock_adj, "value_changed", G_CALLBACK(cb_pixel_clock_adj_value_change), NULL);	
	
	/* brightness slider and spin-button */
	tmp_obj = gtk_builder_get_object(builder, "bright_adjust");
	brightness_adj = GTK_ADJUSTMENT(tmp_obj);
	gtk_adjustment_configure(brightness_adj, LINSEN_MIN_BRIGHT, LINSEN_MIN_BRIGHT, LINSEN_MAX_BRIGHT, 1, 0, 0);
	g_signal_connect(brightness_adj, "value_changed", G_CALLBACK(cb_brightness_adj_value_change), NULL);	
	
    /* result slider */
   	tmp_obj = gtk_builder_get_object(builder, "result_scale");
	gtk_scale_add_mark(GTK_SCALE(tmp_obj), 0, GTK_POS_TOP, NULL);
	gtk_scale_add_mark(GTK_SCALE(tmp_obj), 0, GTK_POS_BOTTOM, NULL);
   	tmp_obj = gtk_builder_get_object(builder, "result_adjust");
	result_adj = GTK_ADJUSTMENT(tmp_obj);
	
	/* linSen drawing area */
	tmp_obj = gtk_builder_get_object(builder, "linSen_draw-area");
	g_signal_connect(tmp_obj, "draw", G_CALLBACK(linSen_raw_draw_event), NULL); 	
	
	/* quadPix raw drawing area */
	tmp_obj = gtk_builder_get_object(builder, "quadPix-raw_draw-area");
	g_signal_connect(tmp_obj, "draw", G_CALLBACK(quadPix_raw_on_draw_event), NULL); 	

	tmp_obj = gtk_builder_get_object(builder, "quadPix-raw_check");
	quadPix_raw_check = GTK_WIDGET(tmp_obj);
	
	/* quadPix result drawing area */
	tmp_obj = gtk_builder_get_object(builder, "quadPix-result_draw-area");
	g_signal_connect(tmp_obj, "draw", G_CALLBACK(quadPix_result_on_draw_event), NULL); 	

	tmp_obj = gtk_builder_get_object(builder, "quadPix-result_check");
	quadPix_result_check = GTK_WIDGET(tmp_obj);
	
	/* get exposure toggle-button */
	tmp_obj = gtk_builder_get_object(builder, "exp_toggle-button");
	exposure_toggle = GTK_WIDGET(tmp_obj);

	/* get pixel clock toggle-button */
	tmp_obj = gtk_builder_get_object(builder, "pix-clk_toggle-button");
	pixel_clock_toggle = GTK_WIDGET(tmp_obj);

	/* get brightness toggle-button */
	tmp_obj = gtk_builder_get_object(builder, "bright_toggle-button");
	brightness_toggle = GTK_WIDGET(tmp_obj);

	/* get result toggle-button */
	tmp_obj = gtk_builder_get_object(builder, "result_toggle-button");
	result_toggle = GTK_WIDGET(tmp_obj);

	/* plotter drawing area */
	tmp_obj = gtk_builder_get_object(builder, "plotter_draw-area");
	g_signal_connect(tmp_obj, "draw", G_CALLBACK(plotter_on_draw_event), NULL); 	
	
	/* exit buttpon */
	tmp_obj = gtk_builder_get_object(builder, "exit_button");
	g_signal_connect(tmp_obj, "clicked", G_CALLBACK(gtk_widget_destroyed), &window); 	
	
	/* status bar */
	tmp_obj = gtk_builder_get_object(builder, "status-bar_1");
	status_bar_1 = GTK_WIDGET(tmp_obj);
	tmp_obj = gtk_builder_get_object(builder, "status-bar_2");
	status_bar_2 = GTK_WIDGET(tmp_obj);
	tmp_obj = gtk_builder_get_object(builder, "status-bar_3");
	status_bar_3 = GTK_WIDGET(tmp_obj);
	
	
	gtk_widget_show_all(window);
	
	return EXIT_SUCCESS;
}

/*
 * update GUI
 * 
 * needs to be called frequently
 * 
 * @return EXIT_SUCCESS, EXIT_FAILURE
 */
int viewer_update() {
	/* if window was destroyed return */
	if ((window == NULL) || (!gtk_widget_get_has_window(window))) return EXIT_FAILURE;
		
	while(gtk_events_pending())
		gtk_main_iteration();
		
	return EXIT_SUCCESS;
}

/*
 * set up an grey-scale image from 12bit data
 * 
 * @param **image
 * @param *data
 * @param width
 * @param height 
 * @return EXIT_SUCCESS, EXIT_FAILURE
 */
int viewer_create_image_from_data(cairo_surface_t **image, const uint16_t *data, int width, int height) {
	debug_printf("called with *image: %#x\t*data: %#x\twidth: %d\theight: %d", (int)image, (int)data, width, height);

	/* if no data is assigned */
	if (data == NULL) return EXIT_FAILURE;
	
	/* setup image */
	if(*image != NULL) {
	    cairo_surface_destroy(*image);
	}
	
	/* get valid stride for cairo */
	int stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB30, width);
	
	/* reorganize memory - convert 12bit grayscale to RGB30 stored in 32bit */
	static uint32_t* frame;
	frame = malloc(height*stride); 
	int i;
	for(i=0;i<(height*width);i++) {
		/* truncate to 16bit to 12bit and reduce to 10bit */
		uint16_t p =(data[i] & 0x0FFF) >> 2;
		/* convert 10bit grey to RGB30-grey */
		frame[i] = (p << 20) | (p << 10) | p;
		verbose_printf("%d %d %d\t%d: %x %x\n", width, height, stride, data[i], p, frame[i]);
	}		
	
	/* create image */
    *image = cairo_image_surface_create_for_data((unsigned char*)frame, CAIRO_FORMAT_RGB30, width, height, stride);
		
	return EXIT_SUCCESS;
}

/*
 * set linSen quadPix raw data in GUI 
 * 
 * @param *data
 * @param width
 * @param height 
 * @return EXIT_SUCCESS, EXIT_FAILURE
 */
int viewer_set_quadPix_raw(const uint32_t *data, int width, int height) {
	debug_printf("called with *data: %#x \twidth: %d\theight: %d", (int)data, width, height);


	int _datasize = width*height;
	int i;
	double scale;

	/* scale */
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(quadPix_raw_check))) {
		int _max = 1;
		
		for (i=0;i<_datasize;i++) {
			if (data[i] > _max) _max = data[i];
		}
		
		scale = 4096 / _max;

		char _str[sizeof("gain()") + strlen(DEFTOSTRING(INT_MAX))];
		sprintf(_str, "gain(%d)", (int)scale);
		gtk_button_set_label(GTK_BUTTON(quadPix_raw_check), _str);
	} else {
		scale = 1;
		gtk_button_set_label(GTK_BUTTON(quadPix_raw_check), "gain");
	}


	uint16_t *_data = malloc(_datasize);
	for (i=0;i<_datasize;i++) {
		_data[i] = data[i] * scale;
	}

	/* if window was destroyed return */
	if ((window == NULL) || (!gtk_widget_get_has_window(window))) return EXIT_FAILURE;

	/* if no data is assigned return */
	if (data == NULL) return EXIT_FAILURE;

	/* create image using helper function */
	viewer_create_image_from_data(&quadPix_raw_image, _data, width, height);

	/* reset request flag */
	quadPix_raw_request = FALSE;

	/* force redraw */
	gtk_widget_queue_draw(window);

	return EXIT_SUCCESS;
}

/*
 * set linSen quadPix result data in GUI 
 * 
 * @param *data
 * @param width
 * @param height 
 * @return EXIT_SUCCESS, EXIT_FAILURE
 */
int viewer_set_quadPix_result(const int32_t *data, int width, int height) {
	debug_printf("called with *data: %#x \twidth: %d\theight: %d", (int)data, width, height);

	/* if window was destroyed return */
	if ((window == NULL) || (!gtk_widget_get_has_window(window))) return EXIT_FAILURE;
	
	/* if no data is assigned return */
	if (data == NULL) return EXIT_FAILURE;

	int _datasize = width*height;
	uint16_t *_data = malloc(_datasize);
	int i;
	double scale;

	/* scale */
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(quadPix_result_check))) {
		int _max = 1;
		
		for (i=0;i<_datasize;i++) {
			if (data[i] > _max) _max = data[i];
		}
		
		scale = 4096 / _max;

		char _str[sizeof("gain()") + strlen(DEFTOSTRING(INT_MAX))];
		sprintf(_str, "gain(%d)", (int)scale);
		gtk_button_set_label(GTK_BUTTON(quadPix_result_check), _str);
	} else {
		scale = 1;
		gtk_button_set_label(GTK_BUTTON(quadPix_result_check), "gain");
	}

	for (i=0;i<_datasize;i++) {
		_data[i] = data[i] * scale;
	}

	
	/* create image using helper function */
	viewer_create_image_from_data(&quadPix_result_image, _data, width, height);
	
	/* reset request flag */
	quadPix_result_request = FALSE;
			
	/* force redraw */
	gtk_widget_queue_draw(window);
	
	return EXIT_SUCCESS;
}

/*
 * set linSen raw data in GUI 
 * 
 * @param *img: linSen raw data
 * @param width:
 * @param height: 
 * @return EXIT_SUCCESS, EXIT_FAILURE
 */
int viewer_set_linSen_raw(const uint16_t *img, int width, int height) {
	debug_printf("called with *img: %#x \twidth: %d\theight: %d", (int)img, width, height);
	
	/* if window was destroyed return */
	if ((window == NULL) || (!gtk_widget_get_has_window(window))) return EXIT_FAILURE;
	
	/* if no data is assigned return */
	if (img == NULL) return EXIT_FAILURE;
	
	/* create image using helper function */
	viewer_create_image_from_data(&linSen_raw_image, img, width, height);
	
	/* reset request flag */
	linSen_raw_request = FALSE;
			
	/* force redraw */
	gtk_widget_queue_draw(window);
	
	return EXIT_SUCCESS;
}

/*
 * set linSen-data in GUI 
 * 
 * @param *data: linSen-data
 * @return EXIT_SUCCESS, EXIT_FAILURE
 */
int viewer_set_linSen_data(linSen_data_t *data) {
	/* if window is destroyed return */
	if ((window == NULL) || (!gtk_widget_get_has_window(window))) return EXIT_FAILURE;
	
	/* if no data is assigned */
	if (data == NULL) return EXIT_FAILURE;
	
	/* store data set - needed to determine whether a value was changed using GUI */	
	memcpy(&linSen_data, data, sizeof(linSen_data_t));
	
	/* limit linSen-data to bounds used in GUI */
	if (linSen_data.exposure > LINSEN_MAX_EXP) linSen_data.exposure = LINSEN_MAX_EXP;
	if (linSen_data.exposure < LINSEN_MIN_EXP) linSen_data.exposure = LINSEN_MIN_EXP;
	if (linSen_data.pixel_clock > LINSEN_MAX_PIX_CLK) linSen_data.pixel_clock = LINSEN_MAX_PIX_CLK;
	if (linSen_data.pixel_clock < LINSEN_MIN_PIX_CLK) linSen_data.pixel_clock = LINSEN_MIN_PIX_CLK;
	if (linSen_data.brightness > LINSEN_MAX_BRIGHT) linSen_data.brightness = LINSEN_MAX_BRIGHT;
	if (linSen_data.brightness < LINSEN_MIN_BRIGHT) linSen_data.brightness = LINSEN_MIN_BRIGHT;

	/* set config slider values */
	gtk_adjustment_set_value(exposure_adj, linSen_data.exposure);
	gtk_adjustment_set_value(pixel_clock_adj, linSen_data.pixel_clock);
	gtk_adjustment_set_value(brightness_adj, linSen_data.brightness);

	/* set result slider value */
	gtk_adjustment_configure(result_adj, linSen_data.global_result, -linSen_data.pixel_number_x/2, +linSen_data.pixel_number_x/2, 1, 0, 0);

	/* status bar */
	/* frame number*/
	char _frame_label_str[sizeof("frame: ") + strlen(DEFTOSTRING(INT_MAX))];
	sprintf(_frame_label_str, "frame: %d", linSen_data.result_id);
	gtk_statusbar_push(GTK_STATUSBAR(status_bar_1), gtk_statusbar_get_context_id(GTK_STATUSBAR(status_bar_1), "frame"), _frame_label_str);
	
	/* sensor size */
	char _size_label_str[sizeof("size:  x ") + 2 * strlen(DEFTOSTRING(INT_MAX))];
	sprintf(_size_label_str, "size: %d x %d", linSen_data.pixel_number_x, linSen_data.pixel_number_y);
	gtk_statusbar_push(GTK_STATUSBAR(status_bar_2), gtk_statusbar_get_context_id(GTK_STATUSBAR(status_bar_2), "size"), _size_label_str);
	
	/* local result number */
	char _local_result_number_label_str[sizeof("local results: ") + strlen(DEFTOSTRING(INT_MAX))];
	sprintf(_local_result_number_label_str, "local results: %d", linSen_data.result_scalar_number);
	gtk_statusbar_push(GTK_STATUSBAR(status_bar_3), gtk_statusbar_get_context_id(GTK_STATUSBAR(status_bar_3), "local_result_number"), _local_result_number_label_str);
	
	/* fill plot data */
	if (++plot_data.write_pos >= PLOTTER_BUFFER) plot_data.write_pos = 0;
	
	plot_data.value[0][plot_data.write_pos] = linSen_data.exposure;
	plot_data.value[1][plot_data.write_pos] = linSen_data.pixel_clock;
	plot_data.value[2][plot_data.write_pos] = linSen_data.brightness;
	plot_data.value[3][plot_data.write_pos] = linSen_data.global_result;
	
	return EXIT_SUCCESS;
}

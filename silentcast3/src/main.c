/*
 *  Filename: main.c 
 *  App Name: Silentcast <https://github.com/colinkeenan/silentcast>
 *  Copyright © 2017 Colin N Keenan <colinnkeenan@gmail.com>
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 * 
 *  Description: Mark region of screen to be recorded, record to raw mkv,
 *  then convert to animated gif, mp4, or webm according to Configura-
 *  tion. Started with the online "Custom Drawing" tutorial for Gtk3 at
 *  https://developer.gnome.org/gtk3/stable/ch01s05.html Where the ori-
 *  ginal tutorial drew with a small filled rectangle using the mouse to 
 *  draw or clear the picture, this app resizes and moves a stroked 
 *  rectangle on a translucent window. The rectangle defaults to sur-
 *  rounding the window that was active when the app started, including 
 *  title bar and borders unless changed in Rectangle Preferences. It
 *  also allows resizing the active window as the rectangle is resized.
 *  Settings are brought up by F1, making the active window resize is
 *  in F2, and F3 brings up the current ffmpeg command that will be
 *  provided for making the silent, uncompressed mkv recording of the
 *  screen.
 */
#include "SC_X11_get_active_window.h" 
#include "SC_conf_widgets.h"

#define P_SET(A) g_object_set_data (G_OBJECT(widget), #A, A); // preprocessor changes #A to "A"

static void set_rect_around_active_window (GdkRectangle *rect, GdkRectangle *p_actv_win, 
    GdkRectangle *p_extents, gboolean *p_include_extents) {

  if (p_extents->width && p_extents->height && 
      ( *p_include_extents || !(p_actv_win->width && p_actv_win->height) )) {
    rect->width = p_extents->width;
    rect->height = p_extents->height;
    rect->x = p_extents->x;
    rect->y = p_extents->y;
  } else if (p_actv_win->width && p_actv_win->height) {
    rect->width = p_actv_win->width;
    rect->height = p_actv_win->height;
    rect->x = p_actv_win->x;
    rect->y = p_actv_win->y;
  } else fprintf (stderr, "Error: can't draw green rectangle around the active window because either width or height was zero.\n");
}

static void set_rect_around_center_fourth (GdkRectangle *rect, GdkRectangle *p_surface_rect)
{
  rect->width = p_surface_rect->width / 2;
  rect->height = p_surface_rect->height / 2;
  rect->x = p_surface_rect->width / 4;
  rect->y = p_surface_rect->height / 4;
}

static void set_rect_to_previous (GdkRectangle *rect, double previous[2])
{
  double x, y, w, h;
  y = 100000 * modf (previous[0], &x);
  h = 100000 * modf (previous[1], &w);
  rect->x = (int) rint (x);
  rect->y = (int) rint (y);
  rect->width = (int) rint (w);
  rect->height = (int) rint (h);
}

void draw_rect (GtkWidget *widget, GdkRectangle *p_area_rect, cairo_surface_t *surface);
/* Redraw the screen from the surface. Note that the ::draw
 * signal receives a ready-to-be-used cairo_t that is already
 * clipped to only draw the exposed areas of the widget
 */
static gboolean
draw_cb (GtkWidget *widget,
         cairo_t   *cr,
         gpointer   data)
{
  gboolean *p_surface_became_fullscreen = P("p_surface_became_fullscreen");
  if (*p_surface_became_fullscreen) {
    /* Save geometry of the surface so draw_rect will have it.
     * Since the surface widget is fullscreen, the geometry
     * should be the same as would be gotten from gdk_monitor_get_geometry 
     * but that function is only available in gtk3 3.22 which most
     * linux users don't have in March 2017
     */
    gdk_cairo_get_clip_rectangle (cr, P("p_surface_rect"));
    draw_rect (widget, P("p_area_rect"), P("surface")); //this won't be infinite loop because it sets ...fullscreen = FALSE
  }

  /* set compositing operation to replace target with source */
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_surface (cr, P("surface"), 0, 0);
  cairo_paint (cr);

  return FALSE;
}

static void clear_surface (GtkWidget *widget, cairo_surface_t *surface) 
{
  cairo_t *cr;

  cr = cairo_create (surface);
  /* set compositing operation to replace target with source */
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  /* set color to translucent black */
  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.7);

  cairo_paint (cr);
  cairo_destroy (cr);
}

static void draw_text (cairo_t *cr, int tx, int ty, GtkWidget *widget, char *text)
{
#define FONT "Mono Bold 14"

  PangoLayout *layout;
  PangoFontDescription *desc;

  cairo_translate (cr, tx, ty);

  layout = pango_cairo_create_layout (cr);

  pango_layout_set_text (layout, text, -1);
  desc = pango_font_description_from_string (FONT);
  pango_layout_set_font_description (layout, desc);

  /* free font description */
  pango_font_description_free (desc);

  pango_cairo_show_layout (cr, layout);

  /* free the layout object */
  g_clear_object (&layout);
}

void draw_rect (GtkWidget *widget, GdkRectangle *p_area_rect, cairo_surface_t *surface) 
{
  gboolean *p_surface_became_fullscreen = P("p_surface_became_fullscreen");
  clear_surface (widget, surface);

  if (*p_surface_became_fullscreen) {
    *p_surface_became_fullscreen = FALSE; //only do this once after becoming fullscreen, not on every draw
    //set the initial size and position of the rectangle that will be drawn on the surface
    if (!strcmp (P("area"), "e") || !strcmp (P("area"), "i"))
      set_rect_around_active_window (P("p_area_rect"), P("p_actv_win"), P("p_extents"), P("p_include_extents")); 
    else if (!strcmp (P("area"), "c"))
      set_rect_around_center_fourth (P("p_area_rect"), P("p_surface_rect"));
    else
      set_rect_to_previous (P("p_area_rect"), P("previous"));
  }

  //don't let the box move past the lower right corner (or go larger than the monitor width or height)
  GdkRectangle *p_surface_rect = P("p_surface_rect");
  int mon_width = p_surface_rect->width, mon_height = p_surface_rect->height;
  if (p_area_rect->x + p_area_rect->width > mon_width) { 
    if (p_area_rect->width >= mon_width) { p_area_rect->x = 0; p_area_rect->width = mon_width; }
    else p_area_rect->x = mon_width - p_area_rect->width;
  }
  if (p_area_rect->y + p_area_rect->height > mon_height) {
    if (p_area_rect->height >= mon_height) { p_area_rect->y = 0; p_area_rect->height = mon_height; }
    else p_area_rect->y = mon_height - p_area_rect->height;
  }

  double rleft = (double) p_area_rect->x;
  double rupper = (double) p_area_rect->y;
  double rwidth = (double) p_area_rect->width;
  double rheight = (double) p_area_rect->height;

  gtk_widget_queue_draw (widget); 

  cairo_t *cr;
  char char_rleft[5], char_rupper[5], char_rwidth[5], char_rheight[5], text[512];

  /* Paint to the global surface, replacing target with source */
  cr = cairo_create (surface);
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  /* draw green border */
  cairo_set_source_rgb (cr, 0, 1, 0); // green
  cairo_rectangle (cr, rleft - 2, rupper - 2, rwidth + 4, rheight + 4);
  cairo_stroke (cr); // draw the lines (of default width, 2)

  /* define text to be drawn under the rectangle */
  snprintf (char_rleft, 5, "%d", (int) rleft);
  snprintf (char_rupper, 5, "%d", (int) rupper);
  snprintf (char_rwidth, 5, "%d", (int) rwidth);
  snprintf (char_rheight, 5, "%d", (int) rheight);
  strcpy (text, char_rleft); strcat (text, ","); strcat (text, char_rupper); strcat (text, " "); 
  strcat (text, char_rwidth); strcat (text, "x"); strcat (text, char_rheight); strcat (text, "\n\
   F1 About Mouse Controls|Configuration|Preferences\n\
   F2 Set recording area with number keys & resize active window checkbox\n\
   F3 View the ffmpeg command that will record the rectangle area\n\
  ESC Quit, q Quit, F11 Toggle Fullscreen, F4 Begin Recording");

  draw_text (cr, rleft, rupper + rheight + 10, widget, text);

  cairo_destroy (cr);
  gtk_widget_queue_draw_area (widget, rleft - 2, rupper - 2, rwidth + 4, rheight + 4);

  /* clear interior of rectangle */
  cr = cairo_create (surface);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_rectangle (cr, rleft, rupper, rwidth, rheight);
  cairo_fill (cr);
  cairo_destroy (cr);
  gtk_widget_queue_draw_area (widget, rleft, rupper, rwidth, rheight);

  /* resize the active-window if checked in the f2_widget */
  if (*((gboolean *)P("p_should_resize_active"))) {
    int ax = p_area_rect->x, ay = p_area_rect->y, aw = p_area_rect->width, ah = p_area_rect->height;
    if (*((gboolean *)P("p_include_extents"))) {
      int *p_dx = P("p_dx"), *p_dy = P("p_dy"), *p_dw = P("p_dw"), *p_dh = P("p_dh");
      int dx = *p_dx, dy = *p_dy, dw = *p_dw, dh = *p_dh;
      ax = ax - dx; ay = ay - dy; aw = aw - dw; ah = ah - dh;
    } 
    gdk_window_move_resize (P("active_window"), ax, ay, aw, ah);
  }
}

static void position_rect (int x, int y, GdkRectangle *p_area_rect) {
  p_area_rect->x = x;
  p_area_rect->y = y;
}

static void drag_resize_to_preset (double right, double lower, double presets[PRESET_N], GdkRectangle *p_area_rect) 
{
  double rleft = (double) p_area_rect->x;
  double rupper = (double) p_area_rect->y;
  double widthheight;
  unsigned int i = PRESET_N - 1;

  /* "push" the rectangle if move too far left or up */
  if (right - rleft < 16) rleft = right - 16;
  if (lower - rupper < 16) rupper = lower - 16;

  /* get rid of fractional parts of mouse location 
   * and convert to width.height to lookup in presets */
  modf (right, &right);
  modf (lower, &lower);
  widthheight = (right - rleft) + (lower - rupper) / 100000;

  //find the preset that's smaller than the rectangle defined by
  //current mouse position and upper left corner of shown rectangle
  while (i > 0 && !(presets[i] < widthheight)) i--;

  p_area_rect->x = (int) rleft;
  p_area_rect->y = (int) rupper;
  p_area_rect->width = (int) get_w (presets[i]);
  p_area_rect->height = (int) get_h (presets[i]);
}

static void scroll_resize_to_preset (GdkScrollDirection direction, GdkRectangle *p_area_rect, double presets[PRESET_N]) {
  if (direction == GDK_SCROLL_UP || direction == GDK_SCROLL_DOWN) {

    double widthheight = ((double) p_area_rect->width) + ((double) p_area_rect->height)/100000;
    unsigned int i=0;

    if (direction == GDK_SCROLL_UP) {
      //find the preset that's smaller than the current rectangle
      i = PRESET_N - 1;
      while (i > 0 && !(presets[i] < widthheight)) i--;
    } else if (direction == GDK_SCROLL_DOWN){ 
      //find the preset that's bigger than the current rectangle
      i = 0;
      while (i < PRESET_N - 1 && !(presets[i] > widthheight)) i++;
    }

    p_area_rect->width = (int) get_w (presets[i]);
    p_area_rect->height = (int) get_h (presets[i]);
  }
}

static void resize_rect (int right, int lower, GdkRectangle *p_area_rect) {
  /* "push" the rectangle if move too far left or up */
  if (right - p_area_rect->x < 16) p_area_rect->x = right - 16;
  if (lower - p_area_rect->y < 16) p_area_rect->y = lower - 16;
  p_area_rect->width = right - p_area_rect->x;
  p_area_rect->height = lower - p_area_rect->y;
}

static gboolean on_toggled_should_resize_active_checkbox (GtkToggleButton *checkbox, gpointer data)
{
  gboolean *value = data;
  *value = gtk_toggle_button_get_active (checkbox);
  return TRUE;
}

static gboolean on_value_changed_x (GtkSpinButton *spin, gpointer data)
{
  GtkWidget *widget = data; //widget is used in P macro

  cairo_surface_t *surface = P("surface");
  GdkRectangle *p_area_rect = P("p_area_rect");
  p_area_rect->x  = gtk_spin_button_get_value_as_int (spin);
  draw_rect (GTK_WIDGET(data), p_area_rect, surface);

  return TRUE;
}

static gboolean on_value_changed_y (GtkSpinButton *spin, gpointer data)
{
  GtkWidget *widget = data; //widget is used in P macro

  cairo_surface_t *surface = P("surface");
  GdkRectangle *p_area_rect = P("p_area_rect");
  p_area_rect->y = gtk_spin_button_get_value_as_int (spin);
  draw_rect (GTK_WIDGET(data), p_area_rect, surface);
 
  return TRUE;
}

static gboolean on_value_changed_w (GtkSpinButton *spin, gpointer data)
{
  GtkWidget *widget = data;

  cairo_surface_t *surface = P("surface");
  GdkRectangle *p_area_rect = P("p_area_rect");
  p_area_rect->width = gtk_spin_button_get_value_as_int (spin);
  draw_rect (GTK_WIDGET(data), p_area_rect, surface);

  return TRUE;
}

static gboolean on_value_changed_h (GtkSpinButton *spin, gpointer data)
{
  GtkWidget *widget = data;

  cairo_surface_t *surface = P("surface");
  GdkRectangle *p_area_rect = P("p_area_rect");
  p_area_rect->height= gtk_spin_button_get_value_as_int (spin);
  draw_rect (GTK_WIDGET(data), p_area_rect, surface);

  return TRUE;
}

/* f2_widget
 * checkbox for resize active window with rectangle
 * spinbuttons for p_area_rect x, y, width, height
 */
static void show_f2_widget (GtkApplication *app, GtkWidget *widget) 
{
  GtkWidget *f2_widget = gtk_application_window_new (app);
  gtk_window_set_transient_for (GTK_WINDOW(f2_widget), GTK_WINDOW(widget));
  gtk_window_set_title (GTK_WINDOW(f2_widget), "Silentcast F2");
  GtkWidget *f2 = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID(f2), ROW_SPACING);
  gtk_container_add (GTK_CONTAINER(f2_widget), f2);
  GdkRectangle *p_area_rect = P("p_area_rect");
  GtkWidget *x_spin_button = NULL, *y_spin_button = NULL, *w_spin_button = NULL, 
                   *h_spin_button = NULL, *should_resize_active_checkbox = NULL;

  GtkWidget *set_area_label = gtk_label_new (NULL); gtk_widget_set_halign (set_area_label, GTK_ALIGN_START);
  gtk_label_set_markup (GTK_LABEL(set_area_label), BGN_SCTN"Set rectangle position and size"END_SCTN);

  //spinbuttons for setting the area
  GtkAdjustment *adjustmentx = gtk_adjustment_new (p_area_rect->x, 0.0, 9999.0, 1.0, 5.0, 0.0);
  x_spin_button = gtk_spin_button_new (adjustmentx, 1.0, 0);
  g_signal_connect (x_spin_button, "value-changed", 
     G_CALLBACK(on_value_changed_x), widget);
  GtkAdjustment *adjustmenty = gtk_adjustment_new (p_area_rect->y, 0.0, 9999.0, 1.0, 5.0, 0.0);
  y_spin_button = gtk_spin_button_new (adjustmenty, 1.0, 0);
  g_signal_connect (y_spin_button, "value-changed", 
     G_CALLBACK(on_value_changed_y), widget);
  GtkAdjustment *adjustmentw = gtk_adjustment_new (p_area_rect->width, 0.0, 9999.0, 1.0, 5.0, 0.0);
  w_spin_button = gtk_spin_button_new (adjustmentw, 1.0, 0);
  g_signal_connect (w_spin_button, "value-changed", 
     G_CALLBACK(on_value_changed_w), widget);
  GtkAdjustment *adjustmenth = gtk_adjustment_new (p_area_rect->height, 0.0, 9999.0, 1.0, 5.0, 0.0);
  h_spin_button = gtk_spin_button_new (adjustmenth, 1.0, 0);
  g_signal_connect (h_spin_button, "value-changed", 
     G_CALLBACK(on_value_changed_h), widget);

  //checkbox for whether or not to resize the active window with the rectangle
  GtkWidget *should_resize_active_checkbox_lbl = gtk_label_new ("Resize active window with rectangle ");
  gtk_widget_set_halign (should_resize_active_checkbox_lbl, GTK_ALIGN_END);
  should_resize_active_checkbox = gtk_check_button_new (); gtk_widget_set_halign (should_resize_active_checkbox, GTK_ALIGN_START);
  gboolean *p_should_resize_active = P("p_should_resize_active");
  g_signal_connect (should_resize_active_checkbox, "toggled", 
     G_CALLBACK(on_toggled_should_resize_active_checkbox), p_should_resize_active);

  GtkWidget *size_label = gtk_label_new ("size"); gtk_widget_set_halign (size_label, GTK_ALIGN_START);
  GtkWidget *posi_label = gtk_label_new ("position"); gtk_widget_set_halign (posi_label, GTK_ALIGN_START);
  GtkWidget *x_label = gtk_label_new ("x: "); gtk_widget_set_halign (x_label, GTK_ALIGN_END);
  GtkWidget *y_label = gtk_label_new ("y: "); gtk_widget_set_halign (y_label, GTK_ALIGN_END);
  GtkWidget *w_label = gtk_label_new ("     width: "); gtk_widget_set_halign (w_label, GTK_ALIGN_END);
  GtkWidget *h_label = gtk_label_new ("    height: "); gtk_widget_set_halign (h_label, GTK_ALIGN_END);

  //update should_resize_active_checkbox
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(should_resize_active_checkbox),*p_should_resize_active);
  
#define F2_ATCH(A,C,R,W) gtk_grid_attach (GTK_GRID(f2), A, C, R, W, 1) 
  F2_ATCH(gtk_label_new ("      "), 0, 0, 1);                                                      F2_ATCH(gtk_label_new ("      "), 5, 0, 1);
                            F2_ATCH(should_resize_active_checkbox_lbl, 2, 1, 3);              F2_ATCH(should_resize_active_checkbox, 5, 1, 1);
              F2_ATCH(set_area_label, 1, 2, 5);
                                                   F2_ATCH(posi_label, 2, 3, 1);                               F2_ATCH(size_label, 4, 3, 1);
                     F2_ATCH(x_label, 1, 4, 1); F2_ATCH(x_spin_button, 2, 4, 1); F2_ATCH(w_label, 3, 4, 1); F2_ATCH(w_spin_button, 4, 4, 1); 
                     F2_ATCH(y_label, 1, 5, 1); F2_ATCH(y_spin_button, 2, 5, 1); F2_ATCH(h_label, 3, 5, 1); F2_ATCH(h_spin_button, 4, 5, 1);
  F2_ATCH(gtk_label_new ("      "), 0, 6, 1);
  gtk_widget_show_all(f2_widget);
}

static GtkEntryBuffer* get_ffcom (char *ffcom_string, GdkRectangle *rect, int *fps, GtkEntryBuffer *working_dir) 
{
  char char_x[5], char_y[5], char_w[5], char_h[5], char_fps[5];
  strcpy (ffcom_string, "/usr/bin/ffmpeg -f x11grab -s ");
  snprintf (char_x, 5, "%d", rect->x);
  snprintf (char_y, 5, "%d", rect->y);
  snprintf (char_w, 5, "%d", rect->width);
  snprintf (char_h, 5, "%d", rect->height);
  strcat (ffcom_string, char_w); strcat (ffcom_string, "x"); strcat (ffcom_string, char_h); 
  snprintf (char_fps, 5, "%d", *fps);
  strcat (ffcom_string, " -r "); strcat (ffcom_string, char_fps);
  strcat (ffcom_string, " -i "); strcat (ffcom_string, gdk_display_get_name(gdk_display_get_default ()));
  strcat (ffcom_string, "+"); strcat (ffcom_string, char_x); strcat (ffcom_string, ","); strcat (ffcom_string, char_y);
  strcat (ffcom_string, " -c:v ffvhuff -an -y '");
  strcat (ffcom_string, gtk_entry_buffer_get_text (working_dir)); strcat (ffcom_string, "/silentcast/temp.mkv'");
  return gtk_entry_buffer_new (ffcom_string, -1);
}

/* f3_widget just displays the ffmpeg command that would be saved on pressing ENTER */
static void show_f3_widget (GtkApplication *app, GtkWidget *widget) 
{
  GtkWidget *f3_widget = gtk_application_window_new (app);
  gtk_window_set_transient_for (GTK_WINDOW(f3_widget), GTK_WINDOW(widget));
  gtk_window_set_title (GTK_WINDOW(f3_widget), "Silentcast F3");

  GtkWidget *ffcom_entry = gtk_entry_new_with_buffer (get_ffcom(P("ffcom_string"), P("p_area_rect"), P("p_fps"), P("working_dir")));
  gtk_editable_set_editable (GTK_EDITABLE(ffcom_entry), FALSE);
  gtk_container_add (GTK_CONTAINER(f3_widget), ffcom_entry);
  gtk_widget_show_all(f3_widget);
  gdk_window_resize (gtk_widget_get_window(ffcom_entry), 8 * gtk_entry_get_text_length (GTK_ENTRY(ffcom_entry)), 32);
}

/* Pressing F4 triggers this to make the uncompressed recording to silentcast/temp.mkv 
 * The ffmpeg command will save a log in ffcom.log because the appropriate environment 
 * variable was set in setup_widget_data_pointers
 */
static void run_ffcom (GtkWidget *widget) 
{
  GError *err = NULL;

  gtk_window_iconify (GTK_WINDOW(widget)); //get translucent surface out of the way while recording
  //Before running the ffmpeg command, make working directory/silentcast if it doesn't exist
  char silentcast_dir[PATH_MAX];
  strcpy (silentcast_dir, gtk_entry_buffer_get_text (P("working_dir")));
  strcat (silentcast_dir, "/silentcast");
  gsize bytes_written = 0;
  //g_mkdir_with_parents uses special encoding so translate from Gtk's utf8, and directories need execute
  //permission to be able to enter, so need 7 on the users permissions (and don't care about other permissions)
  g_mkdir_with_parents (g_filename_from_utf8 (silentcast_dir, -1, NULL, &bytes_written, &err), 0700);
  if (err) {
    fprintf (stderr, "Error: %s\n", err->message);
    g_error_free (err);
  } else {
    get_ffcom(P("ffcom_string"), P("p_area_rect"), P("p_fps"), P("working_dir"));
    if (!g_spawn_command_line_async (P("ffcom_string"), &err)) {
      fprintf (stderr, "Error: %s\n", err->message);
      g_error_free (err);
    }
  }
}

/* clicking the iconified (minimized) silentcast icon to bring it back fullscreen triggers this to kill ffmpeg */
static void kill_ffcom(GtkWidget *widget) 
{
  GError *err = NULL;
  if (!g_spawn_command_line_sync ("pkill -f ffmpeg", NULL, NULL, NULL, &err)) {
    fprintf (stderr, "Error: %s\n", err->message);
    g_error_free (err);
  }
}

static void toggle_fullscreen_area (GtkWidget *surface_widget, GdkRectangle *p_area_rect, cairo_surface_t *surface) {
  static gboolean area_is_fullscreen = FALSE;
  area_is_fullscreen = !area_is_fullscreen;
  static int prev_y, prev_x, prev_w, prev_h;
  GNotification *notification;

  notification = g_notification_new ("Fullscreen");
  g_notification_set_body (notification, "ENTER->Record, F11->Restore Previous Rectangle");
  if (area_is_fullscreen) {
    g_application_send_notification (G_APPLICATION(gtk_window_get_application(GTK_WINDOW(surface_widget))), "Fullscreen", notification);
    g_object_unref (notification);
    prev_y = p_area_rect->y; prev_x = p_area_rect->x; prev_w = p_area_rect->width; prev_h = p_area_rect->height;
    p_area_rect->y = 0; p_area_rect->x = 0;
    p_area_rect->width = gtk_widget_get_allocated_width (surface_widget);
    p_area_rect->height = gtk_widget_get_allocated_height (surface_widget);
    draw_rect (surface_widget, p_area_rect, surface);
  } else {
    p_area_rect->y = prev_y; p_area_rect->x = prev_x; p_area_rect->width = prev_w; p_area_rect->height = prev_h;
    draw_rect (surface_widget, p_area_rect, surface);
  }
}

/* Handle button press events (i.e. mouse clicks) 
 * by either drawing a rectangle with top-left corner at 
 * the mouse pointer, change the size of the rectangle 
 * to put the lower-right corner at the mouse position,
 * or bring up a menu, depending on which button is pressed.
 */
static gboolean
button_press_event_cb (GtkWidget      *widget,
                       GdkEventButton *event,
                       gpointer        data)
{
  cairo_surface_t *surface = P("surface");
  GdkRectangle *p_area_rect = P("p_area_rect");
  
  /* paranoia check, in case we haven't gotten a configure event */
  if (surface == NULL) return FALSE;

  if (event->button == GDK_BUTTON_PRIMARY) { // left click
    position_rect ((int) event->x, (int) event->y, p_area_rect);
    draw_rect (widget, p_area_rect, surface);
  } else if (event->button == GDK_BUTTON_SECONDARY) { // right click
    resize_rect ((int) event->x, (int) event->y, p_area_rect);
    draw_rect (widget, p_area_rect, surface);
  } else if (event->button == GDK_BUTTON_MIDDLE) { // middle click
    gboolean *p_include_extents = P("p_include_extents");
    *p_include_extents = !*p_include_extents;
    set_rect_around_active_window (p_area_rect, P("p_actv_win"), P("p_extents"), p_include_extents); 
    draw_rect (widget, p_area_rect, surface);
  }

  /* The event is handled, stop processing */
  return TRUE;
}

/* Handle scroll events (wheel on mouse)
 * by increasing or decreasing the size of the rectangle
 * to the next preset size 
 */
static gboolean scroll_event_cb (GtkWidget       *widget,
                                 GdkEventScroll  *event,
                                 gpointer         data)
{
  cairo_surface_t *surface = P("surface");
  GdkRectangle *p_area_rect = P ("p_area_rect");
  double *presets = P("presets");

  /* paranoia check, in case we haven't gotten a configure event */
  if (surface == NULL) return FALSE;

  scroll_resize_to_preset (event->direction, p_area_rect, presets);
  draw_rect (widget, p_area_rect, surface);

  /* The event is handled, stop processing */
  return TRUE;
}

/* Handle motion events (i.e mouse drag) 
 * by continuing to move and draw if button 1 (left) is
 * still held down, or continuing to resize and draw 
 * if button 3 (right) is held down 
 */
static gboolean
motion_notify_event_cb (GtkWidget      *widget,
                        GdkEventMotion *event,
                        gpointer        data)
{
  cairo_surface_t *surface = P("surface");
  GdkRectangle *p_area_rect = P ("p_area_rect");
  double *presets = P("presets");

  /* paranoia check, in case we haven't gotten a configure event */
  if (surface == NULL) return FALSE;

  if (event->state & GDK_BUTTON1_MASK) {
    position_rect (event->x, event->y, p_area_rect);
    draw_rect (widget, p_area_rect, surface);
  } else if (event->state & GDK_BUTTON2_MASK) { 
    drag_resize_to_preset (event->x, event->y, presets, p_area_rect);
    draw_rect (widget, p_area_rect, surface);
  } else if (event->state & GDK_BUTTON3_MASK) {
    resize_rect (event->x, event->y, p_area_rect);
    draw_rect (widget, p_area_rect, surface);
  } 

  /* It's handled, stop processing */
  return TRUE;
}

static gboolean key_event_cb (GtkWidget *widget,
                            GdkEventKey *event,
                                gpointer data)
{
  cairo_surface_t *surface = P("surface");

  if (event->keyval == GDK_KEY_F1) {
    show_f1_widget (GTK_APPLICATION(data), widget);
    return TRUE;
  } else if (event->keyval == GDK_KEY_F2) {
    show_f2_widget (GTK_APPLICATION(data), widget);
    return TRUE;
  } else if (event->keyval == GDK_KEY_F3) {
    show_f3_widget (GTK_APPLICATION(data), widget);
    return TRUE;
  } else if (event->keyval == GDK_KEY_F4) {
    run_ffcom (widget);
    return TRUE;
  } else if (event->keyval == GDK_KEY_F11) {
    GdkRectangle *p_area_rect = P("p_area_rect");
    toggle_fullscreen_area (widget, p_area_rect, surface);
    return TRUE;
  } else if (event->keyval == GDK_KEY_Escape || event->keyval == GDK_KEY_q) {
    gtk_widget_destroy (widget); 
    return TRUE;
  }
  return FALSE; //allow further processing of the keypress if it's not anything silentcast is listening for
}

/* tran_setup copied from 
 * http://stackoverflow.com/questions/36994927/gtk3-window-transparent?answertab=active#tab-top
 */
static void tran_setup(GtkWidget *widget)
{
  GdkScreen *screen;
  GdkVisual *alpha_visual;

  gtk_widget_set_app_paintable(widget, TRUE);
  screen = gdk_screen_get_default();
  alpha_visual = gdk_screen_get_rgba_visual(screen);

  if (alpha_visual != NULL && gdk_screen_is_composited(screen)) {
    gtk_widget_set_visual(widget, alpha_visual);
  } 
}

static void setup_widget_data_pointers (GtkWidget *widget) 
{
  //only safe place to change the environment for spawned ffmpeg (using a simple function) is on startup
  g_setenv ("FFREPORT", "file=ffcom.log:level=32", TRUE);

  //get active window information including geometry that will be used to draw the green rectangle
  static GdkWindow *active_window = NULL;
  Window xwin; 
  Window *xwin_children; 
  ssize_t n; 
  static GdkRectangle actv_win = { 0, 0, 0, 0 }, extents = { 0, 0, 0, 0 };
  if (!SC_get_active_windows_and_geometry (&xwin, &xwin_children, &n, &actv_win, &extents, &active_window)) {
    fprintf (stderr, "No active-window information available due to X11 error.\n");
    //completely exit the application because can't even draw the fullscreen surface for the rectangle if can't get the 
    //monitor_geometry, and can't get the right monitor without the active window
    //Instead though, should put up a dialog explaining that the active window can't be found then use the
    //dialog itself as the active window.
    if (xwin_children) XFree (xwin_children);
    gtk_widget_destroy (widget); 
  }
  if (xwin_children) XFree (xwin_children);
  
  /*need to keep a pointer to the active-window in case it's to be resized with the rectangle*/
  P_SET(active_window);

  /*rectangle geometry*/
  static int dx = 0, dy = 0, dw = 0, dh = 0;
  dx = extents.x - actv_win.x; dy = extents.y - actv_win.y; dw = extents.width - actv_win.width; dh = extents.height - actv_win.height;
  int *p_dx = &dx, *p_dy = &dy, *p_dw = &dw, *p_dh = &dh;
  P_SET(p_dx); P_SET(p_dy); P_SET(p_dw); P_SET(p_dh);
  static GdkRectangle area_rect = { 130, 130, 260, 260 }, *p_area_rect = &area_rect; P_SET(p_area_rect);
  static GdkRectangle *p_actv_win = &actv_win; P_SET(p_actv_win);
  static GdkRectangle *p_extents = &extents; P_SET(p_extents);
  static gboolean surface_became_fullscreen = FALSE, *p_surface_became_fullscreen = &surface_became_fullscreen; P_SET(p_surface_became_fullscreen);

  static gboolean include_extents = TRUE, *p_include_extents = &include_extents;
  P_SET(p_include_extents);

  static double presets[PRESET_N], previous[2]; get_presets (presets, previous); P_SET(presets); P_SET(previous);

  /*should resize active-window boolean*/
  static gboolean should_resize_active = FALSE, *p_should_resize_active = &should_resize_active; P_SET(p_should_resize_active);

  /*variables read from silentcast.conf*/
  static GtkEntryBuffer *working_dir = NULL;
  working_dir = gtk_entry_buffer_new (NULL, -1);  
  static char area[] = "e"; // i e c p for Interior, Entirety, Center, Previous
  static unsigned int fps = 8, *p_fps = &fps;
  static gboolean anims_from_temp = TRUE, gif = TRUE, pngs = FALSE, webm = FALSE, mp4 = FALSE,
                  *p_anims_from_temp = &anims_from_temp, *p_gif = &gif, *p_pngs = &pngs, *p_webm = &webm, *p_mp4 = &mp4; 
  get_conf (working_dir, area, p_fps, p_anims_from_temp, p_gif, p_pngs, p_webm, p_mp4);
  P_SET(working_dir); P_SET(area); P_SET(p_fps); P_SET(p_anims_from_temp); P_SET(p_gif); P_SET(p_pngs); P_SET(p_webm); P_SET(p_mp4);

  /*variable for storing ffcom*/
  char ffcom_string[PATH_MAX + 200]; P_SET(ffcom_string);

  static gboolean surface_became_iconified = FALSE, *p_surface_became_iconified = &surface_became_iconified; P_SET(p_surface_became_iconified);

}

/* Create a new surface in widget's window to store rectangle */
static gboolean
configure_surface_cb (GtkWidget *widget,
              GdkEventConfigure *event,
              gpointer           data) {

  //the translucent fullscreen drawing surface
  static cairo_surface_t *surface = NULL;
  //set a place to store the surface geometry
  static GdkRectangle surface_rect = { 0, 0 ,0, 0 }, *p_surface_rect = &surface_rect; 

  gtk_window_get_size (GTK_WINDOW (widget), &surface_rect.width, &surface_rect.height);
  surface = gdk_window_create_similar_surface (gtk_widget_get_window (widget),
                                               CAIRO_CONTENT_COLOR_ALPHA,
                                               surface_rect.width,
                                               surface_rect.height);
  P_SET(surface);
  P_SET(p_surface_rect);

  /* configure event is handled, no need for further processing. */
  return TRUE;
}

static void write_previous (GdkRectangle previous)
{
  double presets[PRESET_N], prepre[2], pre[PRESET_N + 2];
  get_presets (presets, prepre);
  for (int i=0; i<PRESET_N; i++) pre[i] = presets[i]; 
  pre[PRESET_N] = (double) previous.x + (double) previous.y / 100000; 
  pre[PRESET_N + 1] = (double) previous.width + (double) previous.height / 100000;
  char *filename = "silentcast_presets", contents[(PRESET_N + 2) * 12], char_preset[11];
  snprintf (char_preset, 11, "%f", pre[0]);
  strcpy (contents, char_preset);
  for (int i=1; i<PRESET_N + 2; i++) {
    strcat (contents, "\n");
    snprintf (char_preset, 11, "%f", pre[i]);
    strcat (contents, char_preset);
  }
  g_file_set_contents (filename, contents, -1, NULL);
}

// called when surface widget is closed
gboolean on_surface_widget_destroy (GtkWidget *widget, 
                                      gpointer data)
{
  GtkWidget *active_window = P("active_window");
  cairo_surface_t *surface = P("surface");

  if (surface) cairo_surface_destroy (surface);
  if (active_window ) g_clear_object (&active_window);
  GdkRectangle *rect = P("p_area_rect");
  write_previous (*rect);
  g_application_quit (G_APPLICATION(data));

  return FALSE; //keep processing the destroy signal
}

static gboolean window_state_cb (GtkWidget *widget, GdkEventWindowState *event, gpointer data)
{
  gboolean *p_surface_became_fullscreen = P("p_surface_became_fullscreen");
  gboolean *p_surface_became_iconified = P("p_surface_became_iconified");

  if (event->new_window_state & GDK_WINDOW_STATE_ICONIFIED) 
    *p_surface_became_iconified = TRUE;
  else if (*p_surface_became_iconified) {
    kill_ffcom(widget);
    *p_surface_became_iconified = FALSE;
  } else if (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) 
    *p_surface_became_fullscreen = TRUE;

  return FALSE; //go ahead and change the window state
}

static gboolean event_after_cb (GtkWidget *widget, GdkEventWindowState *event, gpointer data) {
  gboolean *p_surface_became_fullscreen = P("p_surface_became_fullscreen");

  if (*p_surface_became_fullscreen) {
    gtk_widget_queue_draw (widget); //when draw_cb is called after this, I think it will save fullscreen geometry
  }

  return FALSE;
}

//map-event callback (the widget is visible)
static void
activate (GtkApplication *app,
          gpointer        data)
{
  GtkWidget *widget = gtk_application_window_new (app);

  setup_widget_data_pointers (widget);
  tran_setup (widget);
  gtk_widget_add_events (widget, GDK_SCROLL_MASK);
  gtk_window_set_title (GTK_WINDOW (widget), "Silentcast");

  //connect signals to functions
  g_signal_connect (widget, "draw",
                    G_CALLBACK (draw_cb), NULL);
  g_signal_connect (widget, "configure-event", 
                    G_CALLBACK (configure_surface_cb), NULL); //surface gets defined in this configure event and it's only place
                                                              //can setup drawing something on startup
  g_signal_connect (widget, "window-state-event", 
                    G_CALLBACK (window_state_cb), NULL); //set boolean on fullscreen, triggering getting fullscreen geometry, and 
                                                        //trigger kill_ffmpeg on coming out of iconify 
  g_signal_connect (widget, "event-after",
                    G_CALLBACK (event_after_cb), NULL); //seeing if it works to draw the rectangle after fullscreen
  g_signal_connect (widget, "motion-notify-event",
                    G_CALLBACK (motion_notify_event_cb), NULL); //mouse drag events
  g_signal_connect (widget, "button-press-event",
                    G_CALLBACK (button_press_event_cb), NULL); //mouse click events
  g_signal_connect (widget, "scroll-event",
                    G_CALLBACK (scroll_event_cb), NULL); //mouse scroll
  g_signal_connect (widget, "key-release-event", 
                    G_CALLBACK (key_event_cb), app); //keyboard events
  g_signal_connect (widget, "destroy", 
      G_CALLBACK (on_surface_widget_destroy), app);
  
  gtk_widget_show_all (widget);
  gtk_window_fullscreen (GTK_WINDOW (widget)); 
}

int
main (int    argc,
      char **argv)
{
  int status;
  GtkApplication *app;
  
  app = gtk_application_new ("com.github.colinkeenan.silentcast", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}

/*
 *  Filename: SC_temptoanim.c 
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
 * 
 *  Description: Convert temp.mkv or *.png created by Silentcast into
 *  an animated gif or movie according to configuration settings.
 *
 */


/*pointers defined in main.c: P("working_dir") P("area") P("p_fps") P("p_anims_from_temp") P("p_gif") P("p_pngs") P("p_webm") P("p_mp4")
 * Translating variables from old bash version of temptoanim:
 * anims_from: was defined but never actually used in bash temptoanim
 *
 * otype: "png" if *p_pngs && !(*p_gif || *p_webm || *p_mp4), "gif" if *p_gif, "webm" if *p_webm, "mp4" if *p_mp4 
 *        (bash version was run multiple times to get all selected outputs)
 * rm_png: !(*p_pngs)
 * use_pngs: !(*p_anims_from_temp) && (*p_webm || *p_mp4) and was also for using exising pngs without generating them from temp.mkv
 * gen_pngs: *p_pngs || (!(*p_anims_from_temp) && (*p_webm || *p_mp4))
 * fps: *p_fps
 *
 * cut, total_cut, group, count: These were for reducing the number of pngs and adjusting the rate if user chose to do so due to memory limitation
*/

#include "SC_temptoanim.h"

void SC_show_error (GtkWidget *widget, char *err_message) 
{
  GtkWidget *dialog = 
    gtk_message_dialog_new (GTK_WINDOW(widget), 
        GTK_DIALOG_DESTROY_WITH_PARENT, 
        GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", err_message);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

void SC_show_err_message (GtkWidget *widget, char *message, char *errmessage)
{
  char err_message[1200];
  strcpy (err_message, message);
  strcat (err_message, errmessage);
  fprintf (stderr, "%s", err_message);
  SC_show_error (widget, err_message);
}

static void show_file_err (GtkWidget *widget, char filename[PATH_MAX], char *errmessage)
{
  char err_message[PATH_MAX + 200];
  strcpy (err_message, "Error: ");
  strcat (err_message, filename);
  strcat (err_message, errmessage);
  fprintf (stderr, "%s", err_message);
  SC_show_error (widget, err_message);
}

gboolean SC_get_glib_filename (GtkWidget *widget, char filename[PATH_MAX], char *glib_filename)
{
  GError *err = NULL;
  gsize bytes_written = 0;
  glib_filename = g_filename_from_utf8 (filename, -1, NULL, &bytes_written, &err);
  if (err) {
    SC_show_err_message (widget, "Error getting glib encoded filename: ", err->message);
    g_error_free (err);
    return FALSE;
  }
  return TRUE;
}

static gboolean is_file (GtkWidget *widget, char filename[PATH_MAX], char *errmessage)
{
  char *glib_encoded_filename = NULL;
  if (!SC_get_glib_filename (widget, filename, glib_encoded_filename)) return FALSE;
  else if (!g_file_test (glib_encoded_filename, G_FILE_TEST_IS_REGULAR)) {
    show_file_err (widget, filename, errmessage);
    g_free (glib_encoded_filename);
    return FALSE;
  }
  g_free (glib_encoded_filename);
  return TRUE;
}

static gboolean temp_exists (GtkWidget *widget, char silentcast_dir[PATH_MAX])
{
  char filename[PATH_MAX];
  strcpy (filename, silentcast_dir);
  strcat (filename, "/temp.mkv");
  return is_file (widget, filename, "temp.mkv not found, so can't generate anything from it");
}

/* globerr for glob() which requires an error handling function be passed to it
 * and glob() is used in check_for_filepattern
 */
static int globerr(const char *path, int eerrno)
{
  //can't use show_err_message because can't get widget into this function
	fprintf (stderr, "Glob Error, %s: %s", path, strerror(eerrno)); 
	return 0;	/* let glob() keep going */
}

static gboolean get_pngs_glob (GtkWidget *widget, glob_t *p_pngs_glob) //should use globfree (p_pngs_glob) when done with it
{
  //glob returns 0 if successful so tests as true when there's an error
  if (glob ("ew-???.png", 0, globerr, p_pngs_glob))  { //not using any flags, that's what the 0 is
    SC_show_error (widget, "Error: ew-???.png not found, so can't convert them to animated gif, webm, or mp4");
    return FALSE;
  }
  return TRUE;
}

static void delete_pngs (GtkWidget *widget, glob_t *p_pngs_glob)
{
    for (int i=0; i<p_pngs_glob->gl_pathc; i++) { 
      char *glib_encoded_filename = NULL;
      if (SC_get_glib_filename (widget, p_pngs_glob->gl_pathv[i], glib_encoded_filename)) {
        g_remove (glib_encoded_filename);
        g_free (glib_encoded_filename);
      } 
    }
    globfree (p_pngs_glob);
}

static gboolean animgif_exists (GtkWidget *widget, char silentcast_dir[PATH_MAX], gboolean pngs, glob_t *p_pngs_glob)
{
  char filename[PATH_MAX];
  strcpy (filename, silentcast_dir);
  strcat (filename, "/anim.gif");
  if (is_file (widget, filename, 
        "Too many images for the available memory. Try closing other applications, creating a swap file, or removing unecessary images.") 
      && !pngs) {
    //if anim.gif was made and pngs aren't a desired output, delete them
    delete_pngs (widget, p_pngs_glob);
    return TRUE;
  } else return FALSE;
}

void SC_spawn (GtkWidget *widget, char *glib_encoded_working_dir, char *commandstring, int *p_pid)
{
  int argc = 0;
  char **argv = NULL;
  GError *err = NULL;

  if (!g_shell_parse_argv (commandstring, &argc, &argv, &err)) {
    SC_show_err_message (widget, "Error trying to parse the ffmpeg command: ", err->message);
    g_error_free (err);
  } else if (!g_spawn_async (glib_encoded_working_dir, argv, NULL, G_SPAWN_DEFAULT, NULL, NULL, p_pid, &err)) {
    SC_show_err_message (widget, "Error trying to spawn the ffmpeg command: ", err->message);
    g_error_free (err);
  }
}

static void generate_pngs (GtkWidget *widget, char silentcast_dir[PATH_MAX], glob_t *p_pngs_glob, int fps)
{
  delete_pngs (widget, p_pngs_glob); //before generating new pngs, delete any existing ones
  if (temp_exists (widget, silentcast_dir)) {
    char *glib_encoded_silentcast_dir = NULL;
    if (SC_get_glib_filename (widget, silentcast_dir, glib_encoded_silentcast_dir)) {
      char ff_gen_pngs[200];
      int *p_ff_gen_pngs_pid = NULL;
      char char_fps[5]; snprintf (char_fps, 5, "%d", fps);
      //construct the command to generate pngs: ffmpeg -i -temp.mkv -r fps ew-%03d.png
      strcpy (ff_gen_pngs, "/usr/bin/ffmpeg -i temp.mkv -r ");
      strcat (ff_gen_pngs, char_fps);
      strcat (ff_gen_pngs, " ew-%03d.png");
      //spawn it
      SC_spawn (widget, glib_encoded_silentcast_dir, ff_gen_pngs, p_ff_gen_pngs_pid); 
      g_free (glib_encoded_silentcast_dir);
      //need to put up a dialog showing pngs being created with option to cancel (which would kill *p_ff_gen_pngs_pid)
    }
  }
}

static void edit_pngs (GtkWidget *widget, char silentcast_dir[PATH_MAX], glob_t *p_pngs_glob, int *p_fps)
{
}
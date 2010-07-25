/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "gtk/gtk.h"
#include "lib/plugin_main.h"
#include "libgimp/stdplugins-intl.h"

#define ENTRY_WIDTH 100

typedef struct
{
  gdouble radius;
  gint horizontal;
  gint vertical;
} BlurValues;

typedef struct
{
  gint run;
} BlurInterface;


/* Declare local functions.
 */
static void      query  (void);
static void      run    (gchar     *name,
			 gint       nparams,
			 GParam    *param,
			 gint      *nreturn_vals,
			 GParam   **return_vals);

static void gauss_rle_u8 (TileDrawable *drawable,
			  gdouble       horz,
			  gdouble       vert);


static void gauss_rle_u16 (TileDrawable *drawable,
			  gdouble       horz,
			  gdouble       vert);

static void      gauss_rle         (TileDrawable *drawable,
				    gint       horizontal,
				    gint       vertical,
				    gdouble    std_dev);

/*
 * Gaussian blur interface
 */
static gint      gauss_rle_dialog  (void);

/*
 * Gaussian blur helper functions
 */
static gfloat *    make_curve        (gdouble    sigma,
				    gint *     length,
				    gfloat     cutoff);
static gint *    make_curve_gint     (gdouble    sigma,
				    gint *     length);
static void      run_length_encode_u8 (guchar *   src,
				    gint *     dest,
				    gint       bytes,
				    gint       width);
static void      run_length_encode_u16 (guint16 *   src,
				    gint *     dest,
				    gint       bytes,
				    gint       width);
static void      run_length_encode_float (gfloat *   src,
				    gint *     buf_number,
				    gfloat *   buf_values,
				    gint       num_channels,
				    gint       width);
static void      run_length_encode_float16 (guint16 *   src,
				    gint *     buf_number,
				    guint16 *   buf_values,
				    gint       num_channels,
				    gint       width);


static void      gauss_close_callback  (GtkWidget *widget,
					gpointer   data);
static void      gauss_ok_callback     (GtkWidget *widget,
					gpointer   data);
static void      gauss_toggle_update   (GtkWidget *widget,
					gpointer   data);
static void      gauss_entry_callback  (GtkWidget *widget,
					gpointer   data);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static BlurValues bvals =
{
  5.0,  /*  radius  */
  TRUE, /*  horizontal blur  */
  TRUE  /*  vertical blur  */
};

static BlurInterface bint =
{
  FALSE  /*  run  */
};


MAIN ()

static void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_FLOAT, "radius", "Radius of gaussian blur (in pixels > 1.0)" },
    { PARAM_INT32, "horizontal", "Blur in horizontal direction" },
    { PARAM_INT32, "vertical", "Blur in vertical direction" },
  };
  static GParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;

  gimp_install_procedure ("plug_in_gauss_rle",
			  "Applies a gaussian blur to the specified drawable.",
			  "Applies a gaussian blur to the drawable, with specified radius of affect.  The standard deviation of the normal distribution used to modify pixel values is calculated based on the supplied radius.  Horizontal and vertical blurring can be independently invoked by specifying only one to run.  The RLE gaussian blurring performs most efficiently on computer-generated images or images with large areas of constant intensity.  Values for radii less than 1.0 are invalid as they will generate spurious results.",
			  "Spencer Kimball & Peter Mattis",
			  "Spencer Kimball & Peter Mattis",
			  "1995-1996",
			  "<Image>/Filters/Blur/Gaussian Blur",
			  "RGB*, GRAY*, U16_RGB*, U16_GRAY*, FLOAT_RGB*, FLOAT_GRAY*, FLOAT16_RGB*, FLOAT16_GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
  _("Blur");
  _("Gaussian Blur");
}

static void
run (gchar   *name,
     gint     nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  TileDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;
  gdouble radius, std_dev, cutoff;

  INIT_I18N_UI();

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

    switch (gimp_drawable_type (drawable->id))
    {
        case GRAY_IMAGE:
        case GRAYA_IMAGE:
        case RGB_IMAGE:
        case RGBA_IMAGE:
        case U16_GRAY_IMAGE:
        case U16_GRAYA_IMAGE:
        case U16_RGB_IMAGE:
        case U16_RGBA_IMAGE:
          break;
    default:
      gimp_message (_("gauss_rle: cannot operate on other than 8-/16-bit images."));
      /*gauss_rle (drawable, bvals.horizontal, bvals.vertical, std_dev); */
      return;
    }

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_gauss_rle", &bvals);

      /*  First acquire information with a dialog  */
      if (! gauss_rle_dialog ())
	return;
      break;

    case RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 6)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  bvals.radius = param[3].data.d_float;
	  bvals.horizontal = (param[4].data.d_int32) ? TRUE : FALSE;
	  bvals.vertical = (param[5].data.d_int32) ? TRUE : FALSE;
	}
      if (status == STATUS_SUCCESS &&
	  (bvals.radius < 1.0))
	status = STATUS_CALLING_ERROR;
      break;

    case RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_gauss_rle", &bvals);
      break;

    default:
      break;
    }

  /*  Make sure that the drawable is gray or RGB color  */
  if (gimp_drawable_color (drawable->id) || gimp_drawable_gray (drawable->id))
    {
      gimp_progress_init (_("RLE Gaussian Blur"));

      /*  set the tile cache size so that the gaussian blur works well  */
      gimp_tile_cache_ntiles (2 * (MAX (drawable->width, drawable->height) /
				   gimp_tile_width () + 1));

      radius = fabs (bvals.radius) + 1.0;
      cutoff = .000001;
      std_dev = sqrt (-(radius * radius) / (2 * log (cutoff)));

      /*  run the gaussian blur  */

    switch (gimp_drawable_type (drawable->id))
    {
        case GRAY_IMAGE:
        case GRAYA_IMAGE:
        case RGB_IMAGE:
        case RGBA_IMAGE:
	  gauss_rle_u8(drawable, radius, radius);
          break;
        case U16_GRAY_IMAGE:
        case U16_GRAYA_IMAGE:
        case U16_RGB_IMAGE:
        case U16_RGBA_IMAGE:
          d_printf ("16-bit\n");
	  gauss_rle_u16(drawable, radius, radius);
          break;
    default:
      gimp_message (_("gauss_rle: cannot operate on other than 8-/16-bit images."));
      /*gauss_rle (drawable, bvals.horizontal, bvals.vertical, std_dev); */
    }

      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush ();

      /*  Store data  */
      if (run_mode == RUN_INTERACTIVE)
	gimp_set_data ("plug_in_gauss_rle", &bvals, sizeof (BlurValues));
    }
  else
    {
      /* gimp_message ("gauss_rle: cannot operate on indexed color images"); */
      status = STATUS_EXECUTION_ERROR;
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}

static gint
gauss_rle_dialog ()
{
  GtkWidget *dlg;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *button;
  GtkWidget *toggle;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  gchar buffer[12];
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("gauss");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), _("RLE Gaussian Blur"));
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) gauss_close_callback,
		      NULL);

  /*  Action area  */
  button = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) gauss_ok_callback,
                      dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  /*  parameter settings  */
  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (vbox), 10);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  toggle = gtk_check_button_new_with_label (_("Blur Horizontally"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) gauss_toggle_update,
		      &bvals.horizontal);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), bvals.horizontal);
  gtk_widget_show (toggle);

  toggle = gtk_check_button_new_with_label (_("Blur Vertically"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) gauss_toggle_update,
		      &bvals.vertical);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), bvals.vertical);
  gtk_widget_show (toggle);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  label = gtk_label_new (_("Blur Radius: "));
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, FALSE, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_widget_set_usize (entry, ENTRY_WIDTH, 0);
  sprintf (buffer, "%f", bvals.radius);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) gauss_entry_callback,
		      NULL);
  gtk_widget_show (entry);

  gtk_widget_show (hbox);
  gtk_widget_show (vbox);
  gtk_widget_show (frame);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return bint.run;
}

/* Convert from separated to premultiplied alpha, on a single scan line. */
static void
multiply_alpha (guchar *buf, gint width, gint bytes)
{
  gint i, j;
  gdouble alpha;

  for (i = 0; i < width * bytes; i += bytes)
    {
      alpha = buf[i + bytes - 1] * (1.0 / 255.0);
      for (j = 0; j < bytes - 1; j++)
	buf[i + j] *= alpha;
    }
}

/* Convert from premultiplied to separated alpha, on a single scan
   line. */
static void
separate_alpha (guchar *buf, gint width, gint bytes)
{
  gint i, j;
  guchar alpha;
  gdouble recip_alpha;
  gint new_val;

  for (i = 0; i < width * bytes; i += bytes)
    {
      alpha = buf[i + bytes - 1];
      if (alpha != 0 && alpha != 255)
	{
	  recip_alpha = 255.0 / alpha;
	  for (j = 0; j < bytes - 1; j++)
	    {
	      new_val = buf[i + j] * recip_alpha;
	      buf[i + j] = MIN (255, new_val);
	    }
	}
    }
}

static void
gauss_rle_u16 (TileDrawable *drawable,
	   gdouble       horz,
	   gdouble       vert)
{
  GPixelRgn src_rgn, dest_rgn;
  gint    width, height;
  gint    bytes;
  gint    has_alpha;
  guint16 *dest, *dp;
  guint16 *src, *sp;
  gint   *buf, *bb;
  gint    pixels;
  gint    total = 1;
  gint    x1, y1, x2, y2;
  gint    i, row, col, b;
  gint    start, end;
  gint    progress, max_progress;
  gint   *curve;
  gint   *sum = NULL;
  gint    val;
  gint    length;
  gint    initial_p, initial_m;
  gdouble std_dev;

  if (horz < 1.0 && vert < 1.0)
    return;

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  width = (x2 - x1);
  height = (y2 - y1);
  bytes = drawable->bpp;
  has_alpha = gimp_drawable_has_alpha(drawable->id);

  buf = g_new (gint, MAX (width, height) * 2);

  /*  allocate buffers for source and destination pixels  */
  src = g_new (guint16, MAX (width, height) * bytes);
  dest = g_new (guint16, MAX (width, height) * bytes);

  gimp_pixel_rgn_init (&src_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable, 0, 0, drawable->width, drawable->height, TRUE, TRUE);

  progress = 0;
  max_progress = (horz < 1.0 ) ? 0 : width * height * horz;
  max_progress += (vert < 1.0 ) ? 0 : width * height * vert;
	  
  if (has_alpha)
    multiply_alpha ((guchar *) src, height, bytes);

  /*  First the vertical pass  */
  if (vert >= 1.0)
    {
      vert = fabs (vert) + 1.0;
      std_dev = sqrt (-(vert * vert) / (2 * log (1.0 / 65535.0)));
      
      curve = make_curve_gint (std_dev, &length);
      sum = g_new (gint, 2 * length + 1);

      sum[0] = 0;

      for (i = 1; i <= length*2; i++)
	sum[i] = curve[i-length-1] + sum[i-1];
      sum += length;
      
      total = sum[length] - sum[-length];

       for (col = 0; col < width; col++)
	{
	  gimp_pixel_rgn_get_col (&src_rgn, (guchar *) src, col + x1, y1, (y2 - y1));

	  sp = src;
	  dp = dest;

	  for (b = 0; b < bytes; b++)
	    {
	      initial_p = sp[b];
	      initial_m = sp[(height-1) * bytes + b];

	      /*  Determine a run-length encoded version of the row  */
	      run_length_encode_u16 (sp + b, buf, bytes, height);

	      for (row = 0; row < height; row++)
		{
		  start = (row < length) ? -row : -length;
		  end = (height <= (row + length)) ? (height - row - 1) : length;

		  val = 0;
		  i = start;
		  bb = buf + (row + i) * 2;

		  if (start != -length)
		    val += initial_p * (sum[start] - sum[-length]);

		  while (i < end)
		    {
		      pixels = bb[0];
		      i += pixels;
		      if (i > end)
			i = end;
		      val += bb[1] * (sum[i] - sum[start]);
		      bb += (pixels * 2);
		      start = i;
		    }

		  if (end != length)
		    val += initial_m * (sum[length] - sum[end]);

		  dp[row * bytes + b] = val / total;
		}
	    }

	  gimp_pixel_rgn_set_col (&dest_rgn, (guchar *) dest, col + x1, y1, (y2 - y1));
	  progress += height * vert;
	  if ((col % 5) == 0)
	    gimp_progress_update ((double) progress / (double) max_progress);
	}

      /*  prepare for the horizontal pass  */
      gimp_pixel_rgn_init (&src_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, TRUE);
    }

  /*  Now the horizontal pass  */
  if (horz >= 1.0)
    {
      horz = fabs (horz) + 1.0;

      if (horz != vert)
	{
	  std_dev = sqrt (-(horz * horz) / (2 * log (1.0 / 65535.0)));
      
	  curve = make_curve_gint (std_dev, &length);
	  sum = g_new (gint, 2 * length + 1);

	  sum[0] = 0;

	  for (i = 1; i <= length*2; i++)
	    sum[i] = curve[i-length-1] + sum[i-1];
	  sum += length;
	  
	  total = sum[length] - sum[-length];
	}

      for (row = 0; row < height; row++)
	{
	  gimp_pixel_rgn_get_row (&src_rgn, (guchar *) src, x1, row + y1, (x2 - x1));

	  sp = src;
	  dp = dest;

	  for (b = 0; b < bytes; b++)
	    {
	      initial_p = sp[b];
	      initial_m = sp[(width-1) * bytes + b];

	      /*  Determine a run-length encoded version of the row  */
	      run_length_encode_u16 (sp + b, buf, bytes, width);

	      for (col = 0; col < width; col++)
		{
		  start = (col < length) ? -col : -length;
		  end = (width <= (col + length)) ? (width - col - 1) : length;

		  val = 0;
		  i = start;
		  bb = buf + (col + i) * 2;

		  if (start != -length)
		    val += initial_p * (sum[start] - sum[-length]);

		  while (i < end)
		    {
		      pixels = bb[0];
		      i += pixels;
		      if (i > end)
			i = end;
		      val += bb[1] * (sum[i] - sum[start]);
		      bb += (pixels * 2);
		      start = i;
		    }

		  if (end != length)
		    val += initial_m * (sum[length] - sum[end]);

		  dp[col * bytes + b] = val / total;
		}
	    }

	  gimp_pixel_rgn_set_row (&dest_rgn, (guchar *) dest, x1, row + y1, (x2 - x1));
	  progress += width * horz;
	  if ((row % 5) == 0)
	    gimp_progress_update ((double) progress / (double) max_progress);
	}
    }
	  
  if (has_alpha)
    separate_alpha ((guchar *) dest, width, bytes);


  /*  merge the shadow, update the drawable  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));

  /*  free buffers  */
  g_free (buf);
  g_free (src);
  g_free (dest);
}


static void
gauss_rle_u8 (TileDrawable *drawable,
	   gdouble       horz,
	   gdouble       vert)
{
  GPixelRgn src_rgn, dest_rgn;
  gint    width, height;
  gint    bytes;
  gint    has_alpha;
  guchar *dest, *dp;
  guchar *src, *sp;
  gint   *buf, *bb;
  gint    pixels;
  gint    total = 1;
  gint    x1, y1, x2, y2;
  gint    i, row, col, b;
  gint    start, end;
  gint    progress, max_progress;
  gint   *curve;
  gint   *sum = NULL;
  gint    val;
  gint    length;
  gint    initial_p, initial_m;
  gdouble std_dev;

  if (horz < 1.0 && vert < 1.0)
    return;

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);

  width = (x2 - x1);
  height = (y2 - y1);
  bytes = drawable->bpp;
  has_alpha = gimp_drawable_has_alpha(drawable->id);

  buf = g_new (gint, MAX (width, height) * 2);

  /*  allocate buffers for source and destination pixels  */
  src = g_new (guchar, MAX (width, height) * bytes);
  dest = g_new (guchar, MAX (width, height) * bytes);

  gimp_pixel_rgn_init (&src_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable, 0, 0, drawable->width, drawable->height, TRUE, TRUE);

  progress = 0;
  max_progress = (horz < 1.0 ) ? 0 : width * height * horz;
  max_progress += (vert < 1.0 ) ? 0 : width * height * vert;
	  
  if (has_alpha)
    multiply_alpha (src, height, bytes);

  /*  First the vertical pass  */
  if (vert >= 1.0)
    {
      vert = fabs (vert) + 1.0;
      std_dev = sqrt (-(vert * vert) / (2 * log (1.0 / 255.0)));
      
      curve = make_curve_gint (std_dev, &length);
      sum = g_new (gint, 2 * length + 1);

      sum[0] = 0;

      for (i = 1; i <= length*2; i++)
	sum[i] = curve[i-length-1] + sum[i-1];
      sum += length;
      
      total = sum[length] - sum[-length];

       for (col = 0; col < width; col++)
	{
	  gimp_pixel_rgn_get_col (&src_rgn, src, col + x1, y1, (y2 - y1));

	  sp = src;
	  dp = dest;

	  for (b = 0; b < bytes; b++)
	    {
	      initial_p = sp[b];
	      initial_m = sp[(height-1) * bytes + b];

	      /*  Determine a run-length encoded version of the row  */
	      run_length_encode_u8 (sp + b, buf, bytes, height);

	      for (row = 0; row < height; row++)
		{
		  start = (row < length) ? -row : -length;
		  end = (height <= (row + length)) ? (height - row - 1) : length;

		  val = 0;
		  i = start;
		  bb = buf + (row + i) * 2;

		  if (start != -length)
		    val += initial_p * (sum[start] - sum[-length]);

		  while (i < end)
		    {
		      pixels = bb[0];
		      i += pixels;
		      if (i > end)
			i = end;
		      val += bb[1] * (sum[i] - sum[start]);
		      bb += (pixels * 2);
		      start = i;
		    }

		  if (end != length)
		    val += initial_m * (sum[length] - sum[end]);

		  dp[row * bytes + b] = val / total;
		}
	    }

	  gimp_pixel_rgn_set_col (&dest_rgn, dest, col + x1, y1, (y2 - y1));
	  progress += height * vert;
	  if ((col % 5) == 0)
	    gimp_progress_update ((double) progress / (double) max_progress);
	}

      /*  prepare for the horizontal pass  */
      gimp_pixel_rgn_init (&src_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, TRUE);
    }

  /*  Now the horizontal pass  */
  if (horz >= 1.0)
    {
      horz = fabs (horz) + 1.0;

      if (horz != vert)
	{
	  std_dev = sqrt (-(horz * horz) / (2 * log (1.0 / 255.0)));
      
	  curve = make_curve_gint (std_dev, &length);
	  sum = g_new (gint, 2 * length + 1);

	  sum[0] = 0;

	  for (i = 1; i <= length*2; i++)
	    sum[i] = curve[i-length-1] + sum[i-1];
	  sum += length;
	  
	  total = sum[length] - sum[-length];
	}

      for (row = 0; row < height; row++)
	{
	  gimp_pixel_rgn_get_row (&src_rgn, src, x1, row + y1, (x2 - x1));

	  sp = src;
	  dp = dest;

	  for (b = 0; b < bytes; b++)
	    {
	      initial_p = sp[b];
	      initial_m = sp[(width-1) * bytes + b];

	      /*  Determine a run-length encoded version of the row  */
	      run_length_encode_u8 (sp + b, buf, bytes, width);

	      for (col = 0; col < width; col++)
		{
		  start = (col < length) ? -col : -length;
		  end = (width <= (col + length)) ? (width - col - 1) : length;

		  val = 0;
		  i = start;
		  bb = buf + (col + i) * 2;

		  if (start != -length)
		    val += initial_p * (sum[start] - sum[-length]);

		  while (i < end)
		    {
		      pixels = bb[0];
		      i += pixels;
		      if (i > end)
			i = end;
		      val += bb[1] * (sum[i] - sum[start]);
		      bb += (pixels * 2);
		      start = i;
		    }

		  if (end != length)
		    val += initial_m * (sum[length] - sum[end]);

		  dp[col * bytes + b] = val / total;
		}
	    }

	  gimp_pixel_rgn_set_row (&dest_rgn, dest, x1, row + y1, (x2 - x1));
	  progress += width * horz;
	  if ((row % 5) == 0)
	    gimp_progress_update ((double) progress / (double) max_progress);
	}
    }
	  
  if (has_alpha)
    separate_alpha (dest, width, bytes);


  /*  merge the shadow, update the drawable  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));

  /*  free buffers  */
  g_free (buf);
  g_free (src);
  g_free (dest);
}


static void
gauss_rle (TileDrawable *drawable,
	   gint       horz,
	   gint       vert,
	   gdouble    std_dev)
{
    GPixelRgn src_rgn, dest_rgn;
    gint width, height;
    gint bytes;
    gint channel_bytes;
    gint num_channels;
    gint drawable_type;
    gint has_alpha;

    gfloat *f_dest, *f_dp;
    gfloat *f_src, *f_sp;

    guint16 *f16_dest, *f16_dp;
    guint16 *f16_src,  *f16_sp;

    guint16 *u16_dest, *u16_dp;
    guint16 *u16_src, *u16_sp;

    gfloat *f_buf_values; /*rle values array*/
    guint16 *f16_buf_values; /*rle values array*/

    gfloat *f_bb_values;
    guint16 *f16_bb_values;

    gfloat  f_initial_p, f_initial_m;
    guint16 f16_initial_p, f16_initial_m;

    gint *buf_number;   /*rle numbers array*/
    gint *bb_number;  

    gint pixels;
    gint x1, y1, x2, y2;
    gint i, row, col, b;
    gint start, end;
    gint progress, max_progress;
    gfloat *curve;
    gfloat *sum;
    gfloat val;
    gfloat total;
    gint length;
    gfloat cutoff = .000001;
    ShortsFloat u;

    curve = make_curve (std_dev, &length, cutoff);
    sum = malloc (sizeof (gfloat) * (2 * length + 1));

    sum[0] = 0;

    for (i = 1; i <= length*2; i++)
        sum[i] = curve[i-length-1] + sum[i-1];
    sum += length;

    gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
    drawable_type = gimp_drawable_type (drawable->id);

    switch (drawable_type)
    {
        case GRAY_IMAGE:
        case GRAYA_IMAGE:
        case RGB_IMAGE:
        case RGBA_IMAGE:
	  gauss_rle_u8(drawable, horz, vert);
	  return;
          break;
        case INDEXED_IMAGE:
        case INDEXEDA_IMAGE:
        case U16_INDEXED_IMAGE:
        case U16_INDEXEDA_IMAGE:
          d_printf ("INDEXED_IMAGE: Format Not Supported\n");
          break;
        case U16_GRAY_IMAGE:
        case U16_GRAYA_IMAGE:
        case U16_RGB_IMAGE:
        case U16_RGBA_IMAGE:
          d_printf ("U16_*: Format Not Supported\n");
          break;
        case FLOAT16_GRAY_IMAGE:
        case FLOAT16_GRAYA_IMAGE:
        case FLOAT16_RGB_IMAGE:
        case FLOAT16_RGBA_IMAGE:
          d_printf ("FLOAT16_*: Format Not Supported\n");
          break;
        case FLOAT_GRAY_IMAGE:
        case FLOAT_GRAYA_IMAGE:
        case FLOAT_RGB_IMAGE:
        case FLOAT_RGBA_IMAGE:
          d_printf ("FLOAT_*: Format Not Supported\n");
          break;
        default:
          d_printf ("Can't save this image type\n");
    }
    
    width = (x2 - x1);
    height = (y2 - y1);
    bytes = drawable->bpp;
    has_alpha = gimp_drawable_has_alpha(drawable->id);
    num_channels = gimp_drawable_num_channels(drawable->id);
    channel_bytes = bytes/num_channels;

    total = sum[length] - sum[-length];

    /*  allocate data for source and destination pixels  */
    switch (drawable_type)
    {
        case RGB_IMAGE:
        case RGBA_IMAGE:
          g_print ("RGB_*: Format Not Supported\n");
          break;
        case U16_RGB_IMAGE:
        case U16_RGBA_IMAGE:
          break;
        case FLOAT16_RGB_IMAGE:
        case FLOAT16_RGBA_IMAGE:
            buf_number = (gint *) malloc (sizeof (gint) * MAX (width, height));
            f16_buf_values = (guint16 *) malloc (MAX (width, height) * channel_bytes);

            f16_src = (guint16 *) malloc (MAX (width, height) * num_channels * channel_bytes);
            f16_dest = (guint16 *) malloc (MAX (width, height) * num_channels * channel_bytes);
          break;
        case FLOAT_RGB_IMAGE:
        case FLOAT_RGBA_IMAGE:
            buf_number = (gint *) malloc (sizeof (gint) * MAX (width, height));
            f_buf_values = (gfloat *) malloc (MAX (width, height) * channel_bytes);

            f_src = (gfloat *) malloc (MAX (width, height) * num_channels * channel_bytes);
            f_dest = (gfloat *) malloc (MAX (width, height) * num_channels * channel_bytes);
          break;
    }

    gimp_pixel_rgn_init (&src_rgn, drawable, 0, 0, 
            drawable->width, drawable->height, FALSE, FALSE);
    gimp_pixel_rgn_init (&dest_rgn, drawable, 0, 0, 
            drawable->width, drawable->height, TRUE, TRUE);

    progress = 0;
    max_progress = (horz) ? width * height : 0;
    max_progress += (vert) ? width * height : 0;

    if (vert)
    {
        for (col = 0; col < width; col++)
        {
            switch (drawable_type)
            {
                case RGB_IMAGE:
                case RGBA_IMAGE:
                  g_print ("RGB_*: Format Not Supported\n");
                  break;
                case U16_RGB_IMAGE:
                case U16_RGBA_IMAGE:
                  break;
                case FLOAT16_RGB_IMAGE:
                case FLOAT16_RGBA_IMAGE:
                    gimp_pixel_rgn_get_col (&src_rgn, (guchar*)f16_src, col + x1, y1, (y2 - y1));

                    f16_sp = f16_src;
                    f16_dp = f16_dest;
                  break;
                case FLOAT_RGB_IMAGE:
                case FLOAT_RGBA_IMAGE:
                    gimp_pixel_rgn_get_col (&src_rgn, (guchar*)f_src, col + x1, y1, (y2 - y1));

                    f_sp = f_src;
                    f_dp = f_dest;
                  break;
            }

            for (b = 0; b < num_channels; b++)
            {
                switch (drawable_type)
                {
                    case RGB_IMAGE:
                    case RGBA_IMAGE:
                      g_print ("RGB_*: Format Not Supported\n");
                      break;
                    case U16_RGB_IMAGE:
                    case U16_RGBA_IMAGE:
                      break;
                    case FLOAT16_RGB_IMAGE:
                    case FLOAT16_RGBA_IMAGE:
                        f16_initial_p = f16_sp[b];
                        f16_initial_m = f16_sp[(height-1) * num_channels + b];

                        /*  Determine a run-length encoded version of the row  */
                        run_length_encode_float16 (f16_sp + b, buf_number, 
                                f16_buf_values, num_channels, height);
                      break;
                    case FLOAT_RGB_IMAGE:
                    case FLOAT_RGBA_IMAGE:
                        f_initial_p = f_sp[b];
                        f_initial_m = f_sp[(height-1) * num_channels + b];

                        /*  Determine a run-length encoded version of the row  */
                        run_length_encode_float (f_sp + b, buf_number, 
                                f_buf_values, num_channels, height);
                      break;
                }


                for (row = 0; row < height; row++)
                {
                    start = (row < length) ? -row : -length;
                    end = (height <= (row + length)) ? (height - row - 1) : length;

                    val = 0;
                    i = start;
 
                    switch (drawable_type)
                    {
                        case GRAY_IMAGE:
                        case GRAYA_IMAGE:
                        case RGB_IMAGE:
                        case RGBA_IMAGE:
                          g_print ("RGB_*: Format Not Supported\n");
                          break;
                        case INDEXED_IMAGE:
                        case INDEXEDA_IMAGE:
                        case U16_INDEXED_IMAGE:
                        case U16_INDEXEDA_IMAGE:
                          g_print ("INDEXED_IMAGE: Format Not Supported\n");
                          break;
                        case U16_GRAY_IMAGE:
                        case U16_GRAYA_IMAGE:
                        case U16_RGB_IMAGE:
                        case U16_RGBA_IMAGE:
                          break;
                        case FLOAT16_GRAY_IMAGE:
                        case FLOAT16_GRAYA_IMAGE:
                        case FLOAT16_RGB_IMAGE:
                        case FLOAT16_RGBA_IMAGE:
                            bb_number = buf_number + (row + i);
                            f16_bb_values = f16_buf_values + (row + i);

                            if (start != -length)
                                val += FLT(f16_initial_p,u) * (sum[start] - sum[-length]);

                            while (i < end)
                            {
                                pixels = *bb_number;
                                i += pixels;
                                if (i > end)
                                    i = end;
                                val += FLT(*f16_bb_values,u) * (sum[i] - sum[start]);
                                bb_number += pixels;
                                f16_bb_values += pixels;
                                start = i;
                            }

                            if (end != length)
                                val += FLT(f16_initial_m,u) * (sum[length] - sum[end]);
                          
                            f16_dp[row * num_channels + b] = FLT16 (val / total, u);
                            gimp_pixel_rgn_set_col (&dest_rgn, (guchar*)f16_dest, 
                                    col + x1, y1, (y2 - y1));
                          break;
                        case FLOAT_GRAY_IMAGE:
                        case FLOAT_GRAYA_IMAGE:
                        case FLOAT_RGB_IMAGE:
                        case FLOAT_RGBA_IMAGE:
                            bb_number = buf_number + (row + i);
                            f_bb_values = f_buf_values + (row + i);

                            if (start != -length)
                                val += FLT(f_initial_p,u) * (sum[start] - sum[-length]);

                            while (i < end)
                            {
                                pixels = *bb_number;
                                i += pixels;
                                if (i > end)
                                    i = end;
                                val += FLT(*f_bb_values,u) * (sum[i] - sum[start]);
                                bb_number += pixels;
                                f_bb_values += pixels;
                                start = i;
                            }

                            if (end != length)
                                val += FLT(f_initial_m,u) * (sum[length] - sum[end]);

                            //f_dp[row * num_channels + b] = FLT (val / total, u);
                            f_dp[row * num_channels + b] = 1.0;
                            gimp_pixel_rgn_set_col (&dest_rgn, (guchar*)f_dest, 
                                    col + x1, y1, (y2 - y1));
                          break;
                    }
                }
            }

            progress += height;
            if ((col % 5) == 0)
                gimp_progress_update ((double) progress / (double) max_progress);
        }

        /*  prepare for the horizontal pass  */
        gimp_pixel_rgn_init (&src_rgn, drawable, 0, 0, 
                drawable->width, drawable->height, FALSE, TRUE);
    }

    if (horz)
    {
        for (row = 0; row < height; row++)
        {

            switch (drawable_type)
            {
                case RGB_IMAGE:
                case RGBA_IMAGE:
                  break;
                case U16_RGB_IMAGE:
                case U16_RGBA_IMAGE:
                  break;
                case FLOAT16_RGB_IMAGE:
                case FLOAT16_RGBA_IMAGE:
                    gimp_pixel_rgn_get_row (&src_rgn, (guchar*)f16_src, x1, row + y1, (x2 - x1));

                    f16_sp = f16_src;
                    f16_dp = f16_dest;
                  break;
                case FLOAT_RGB_IMAGE:
                case FLOAT_RGBA_IMAGE:
                    gimp_pixel_rgn_get_row (&src_rgn, (guchar*)f_src, x1, row + y1, (x2 - x1));

                    f_sp = f_src;
                    f_dp = f_dest;
                  break;
            }
            
            for (b = 0; b < num_channels; b++)
            {
                switch (drawable_type)
                {
                    case RGB_IMAGE:
                    case RGBA_IMAGE:
                      break;
                    case U16_RGB_IMAGE:
                    case U16_RGBA_IMAGE:
                      break;
                    case FLOAT16_RGB_IMAGE:
                    case FLOAT16_RGBA_IMAGE:
                        f16_initial_p = f16_sp[b];
                        f16_initial_m = f16_sp[(width-1) * num_channels + b];

                        /*  Determine a run-length encoded version of the row  */
                        run_length_encode_float16 (f16_sp + b, buf_number, 
                                f16_buf_values, num_channels, width);
                      break;
                    case FLOAT_RGB_IMAGE:
                    case FLOAT_RGBA_IMAGE:
                        f_initial_p = f_sp[b];
                        f_initial_m = f_sp[(width-1) * num_channels + b];

                        /*  Determine a run-length encoded version of the row  */
                        run_length_encode_float (f_sp + b, buf_number, 
                                f_buf_values, num_channels, width);
                      break;
                }
                
                for (col = 0; col < width; col++)
                {
                    start = (col < length) ? -col : -length;
                    end = (width <= (col + length)) ? (width - col - 1) : length;

                    val = 0;
                    i = start;
                    switch (drawable_type)
                    {
                        case GRAY_IMAGE:
                        case GRAYA_IMAGE:
                        case RGB_IMAGE:
                        case RGBA_IMAGE:
                          g_print ("RGB_*: Format Not Supported\n");
                          break;
                        case INDEXED_IMAGE:
                        case INDEXEDA_IMAGE:
                        case U16_INDEXED_IMAGE:
                        case U16_INDEXEDA_IMAGE:
                          g_print ("INDEXED_IMAGE: Format Not Supported\n");
                          break;
                        case U16_GRAY_IMAGE:
                        case U16_GRAYA_IMAGE:
                        case U16_RGB_IMAGE:
                        case U16_RGBA_IMAGE:
                          break;
                        case FLOAT16_GRAY_IMAGE:
                        case FLOAT16_GRAYA_IMAGE:
                        case FLOAT16_RGB_IMAGE:
                        case FLOAT16_RGBA_IMAGE:
                            bb_number = buf_number + (col + i);
                            f16_bb_values = f16_buf_values + (col + i);

                            if (start != -length)
                                val += FLT(f16_initial_p,u) * (sum[start] - sum[-length]);

                            while (i < end)
                            {
                                pixels = *bb_number;
                                i += pixels;
                                if (i > end)
                                    i = end;
                                val += FLT(*f16_bb_values, u) * (sum[i] - sum[start]);
                                bb_number += pixels;
                                f16_bb_values += pixels;
                                start = i;
                            }

                            if (end != length)
                                val += FLT(f16_initial_m,u) * (sum[length] - sum[end]);

                            f16_dp[col * num_channels + b] = FLT16 (val / total,u);
                            gimp_pixel_rgn_set_row (&dest_rgn, (guchar*)f16_dest, 
                                    x1, row + y1, (x2 - x1));
                          break;
                        case FLOAT_GRAY_IMAGE:
                        case FLOAT_GRAYA_IMAGE:
                        case FLOAT_RGB_IMAGE:
                        case FLOAT_RGBA_IMAGE:
                            bb_number = buf_number + (col + i);
                            f_bb_values = f_buf_values + (col + i);

                            if (start != -length)
                                val += FLT(f_initial_p,u) * (sum[start] - sum[-length]);

                            while (i < end)
                            {
                                pixels = *bb_number;
                                i += pixels;
                                if (i > end)
                                    i = end;
                                val += FLT(*f_bb_values, u) * (sum[i] - sum[start]);
                                bb_number += pixels;
                                f_bb_values += pixels;
                                start = i;
                            }

                            if (end != length)
                                val += FLT(f_initial_m,u) * (sum[length] - sum[end]);

                            f_dp[col * num_channels + b] = FLT16 (val / total,u);
                            gimp_pixel_rgn_set_row (&dest_rgn, (guchar*)f_dest, 
                                    x1, row + y1, (x2 - x1));
                          break;
                    }
                }
            }

            progress += width;
            if ((row % 5) == 0)
                gimp_progress_update ((double) progress / (double) max_progress);
        }
    }

    /*  merge the shadow, update the drawable  */
    gimp_drawable_flush (drawable);
    gimp_drawable_merge_shadow (drawable->id, TRUE);
    gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));

    /*  free buffers  */
    free (buf_number);
    free (f16_buf_values);
    free (f_buf_values);
    free (f_src);
    free (f16_src);
    free (f_dest);
    free (f16_dest);
}

/*
 * The equations: g(r) = exp (- r^2 / (2 * sigma^2))
 *                   r = sqrt (x^2 + y ^2)
 */

static gfloat *
make_curve (gdouble  sigma,
	    gint    *length,
	    gfloat   cutoff)
{
  gfloat *curve;
  gdouble sigma2;
  gdouble l;
  gfloat temp;
  gint i, n;

  sigma2 = 2 * sigma * sigma;
  l = sqrt (-sigma2 * log (cutoff));

  n = ceil (l) * 2;
  if ((n % 2) == 0)
    n += 1;

  curve = malloc (sizeof (gfloat) * n);

  *length = n / 2;
  curve += *length;
  curve[0] = 1.0;

  for (i = 1; i <= *length; i++)
    {
      temp = (gfloat) exp (- (i * i) / sigma2);
      curve[-i] = temp;
      curve[i] = temp;
    }

  return curve;
}


static void
run_length_encode_u8 (guchar *src,
		   gint   *dest,
		   gint    bytes,
		   gint    width)
{
  gint start;
  gint i;
  gint j;
  guchar last;

  last = *src;
  src += bytes;
  start = 0;

  for (i = 1; i < width; i++)
    {
      if (*src != last)
	{
	  for (j = start; j < i; j++)
	    {
	      *dest++ = (i - j);
	      *dest++ = last;
	    }
	  start = i;
	  last = *src;
	}
      src += bytes;
    }

  for (j = start; j < i; j++)
    {
      *dest++ = (i - j);
      *dest++ = last;
    }
}


static void
run_length_encode_u16 (guint16 *src,
		   gint   *dest,
		   gint    bytes,
		   gint    width)
{
  gint start;
  gint i;
  gint j;
  guint16 last;

  last = *src;
  src += bytes;
  start = 0;

  for (i = 1; i < width; i++)
    {
      if (*src != last)
	{
	  for (j = start; j < i; j++)
	    {
	      *dest++ = (i - j);
	      *dest++ = last;
	    }
	  start = i;
	  last = *src;
	}
      src += bytes;
    }

  for (j = start; j < i; j++)
    {
      *dest++ = (i - j);
      *dest++ = last;
    }
}

static void
run_length_encode_float (gfloat *src,
		   gint   *number,        /*dest*/
		   gfloat *values,        /*dest*/
		   gint    num_channels,
		   gint    width)
{
  gint start;
  gint i;
  gint j;

  gfloat last;

  last = *src;
  src += num_channels;
  start = 0;

  for (i = 1; i < width; i++)
    {
      if (*src != last)
	{
	  for (j = start; j < i; j++)
	    {
	      *number++ = i - j;
	      *values++ = last;
	    }
	  start = i;
	  last = *src;
	}
      src += num_channels;
    }

  for (j = start; j < i; j++)
    {
      *number++ = i - j;
      *values++ = last;
    }
}

static void
run_length_encode_float16 (guint16 *src,
		   gint   *number,        /*dest*/
		   guint16 *values,        /*dest*/
		   gint    num_channels,
		   gint    width)
{
  gint start;
  gint i;
  gint j;

  gfloat last;

  last = *src;
  src += num_channels;
  start = 0;

  for (i = 1; i < width; i++)
    {
      if (*src != last)
	{
	  for (j = start; j < i; j++)
	    {
	      *number++ = i - j;
	      *values++ = last;
	    }
	  start = i;
	  last = *src;
	}
      src += num_channels;
    }

  for (j = start; j < i; j++)
    {
      *number++ = i - j;
      *values++ = last;
    }
}


/*  Gauss interface functions  */

static void
gauss_close_callback (GtkWidget *widget,
		      gpointer   data)
{
  gtk_main_quit ();
}

static void
gauss_ok_callback (GtkWidget *widget,
		   gpointer   data)
{
  bint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
gauss_toggle_update (GtkWidget *widget,
		       gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}

static void
gauss_entry_callback (GtkWidget *widget,
		      gpointer   data)
{
  bvals.radius = atof (gtk_entry_get_text (GTK_ENTRY (widget)));
  if (bvals.radius < 1.0)
    bvals.radius = 1.0;
}

static gint *
make_curve_gint (gdouble  sigma,
	    gint    *length)
{
  gint *curve;
  gdouble sigma2;
  gdouble l;
  gint temp;
  gint i, n;

  sigma2 = 2 * sigma * sigma;
  l = sqrt (-sigma2 * log (1.0 / 255.0));

  n = ceil (l) * 2;
  if ((n % 2) == 0)
    n += 1;

  curve = g_new (gint, n);

  *length = n / 2;
  curve += *length;
  curve[0] = 255;

  for (i = 1; i <= *length; i++)
    {
      temp = (gint) (exp (- (i * i) / sigma2) * 255);
      curve[-i] = temp;
      curve[i] = temp;
    }

  return curve;
}

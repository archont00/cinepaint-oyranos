/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpgradientselect_pdb.c
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* NOTE: This file is autogenerated by pdbgen.pl */

#include <string.h>

#include "libgimp/gimp.h"

/**
 * gimp_gradients_popup:
 * @gradients_callback: The callback PDB proc to call when gradient selection is made.
 * @popup_title: Title to give the gradient popup window.
 * @initial_gradient: The name of the pattern to set as the first selected.
 * @sample_size: Size of the sample to return when the gradient is changed.
 *
 * Invokes the Gimp gradients selection.
 *
 * This procedure popups the gradients selection dialog.
 *
 * Returns: TRUE on success.
 */
gboolean
gimp_gradients_popup (gchar *gradients_callback,
		      gchar *popup_title,
		      gchar *initial_gradient,
		      gint   sample_size)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gboolean success = TRUE;

  return_vals = gimp_run_procedure ("gimp_gradients_popup",
				    &nreturn_vals,
				    GIMP_PDB_STRING, gradients_callback,
				    GIMP_PDB_STRING, popup_title,
				    GIMP_PDB_STRING, initial_gradient,
				    GIMP_PDB_INT32, sample_size,
				    GIMP_PDB_END);

  success = return_vals[0].data.d_status == GIMP_PDB_SUCCESS;

  gimp_destroy_params (return_vals, nreturn_vals);

  return success;
}

/**
 * gimp_gradients_close_popup:
 * @gradients_callback: The name of the callback registered for this popup.
 *
 * Popdown the Gimp gradient selection.
 *
 * This procedure closes an opened gradient selection dialog.
 *
 * Returns: TRUE on success.
 */
gboolean
gimp_gradients_close_popup (gchar *gradients_callback)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gboolean success = TRUE;

  return_vals = gimp_run_procedure ("gimp_gradients_close_popup",
				    &nreturn_vals,
				    GIMP_PDB_STRING, gradients_callback,
				    GIMP_PDB_END);

  success = return_vals[0].data.d_status == GIMP_PDB_SUCCESS;

  gimp_destroy_params (return_vals, nreturn_vals);

  return success;
}

/**
 * gimp_gradients_set_popup:
 * @gradients_callback: The name of the callback registered for this popup.
 * @gradient_name: The name of the gradient to set as selected.
 *
 * Sets the current gradient selection in a popup.
 *
 * Sets the current gradient selection in a popup.
 *
 * Returns: TRUE on success.
 */
gboolean
gimp_gradients_set_popup (gchar *gradients_callback,
			  gchar *gradient_name)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gboolean success = TRUE;

  return_vals = gimp_run_procedure ("gimp_gradients_set_popup",
				    &nreturn_vals,
				    GIMP_PDB_STRING, gradients_callback,
				    GIMP_PDB_STRING, gradient_name,
				    GIMP_PDB_END);

  success = return_vals[0].data.d_status == GIMP_PDB_SUCCESS;

  gimp_destroy_params (return_vals, nreturn_vals);

  return success;
}

/**
 * _gimp_gradients_get_gradient_data:
 * @name: The gradient name (\"\" means current active gradient).
 * @sample_size: Size of the sample to return when the gradient is changed.
 * @width: The gradient sample width (r,g,b,a).
 * @grad_data: The gradient sample data.
 *
 * Retrieve information about the specified gradient (including data).
 *
 * This procedure retrieves information about the gradient. This
 * includes the gradient name, and the sample data for the gradient.
 *
 * Returns: The gradient name.
 */
gchar *
_gimp_gradients_get_gradient_data (gchar    *name,
				   gint      sample_size,
				   gint     *width,
				   gdouble **grad_data)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gchar *ret_name = NULL;

  return_vals = gimp_run_procedure ("gimp_gradients_get_gradient_data",
				    &nreturn_vals,
				    GIMP_PDB_STRING, name,
				    GIMP_PDB_INT32, sample_size,
				    GIMP_PDB_END);

  *width = 0;

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
    {
      ret_name = g_strdup (return_vals[1].data.d_string);
      *width = return_vals[2].data.d_int32;
      *grad_data = g_new (gdouble, *width);
      memcpy (*grad_data, return_vals[3].data.d_floatarray,
	      *width * sizeof (gdouble));
    }

  gimp_destroy_params (return_vals, nreturn_vals);

  return ret_name;
}

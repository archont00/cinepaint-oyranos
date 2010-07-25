/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * gimpchainbutton.h
 * Copyright (C) 1999-2000 Sven Neumann <sven@gimp.org> 
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

/* 
 * This implements a widget derived from GtkTable that visualizes
 * it's state with two different pixmaps showing a closed and a 
 * broken chain. It's intented to be used with the GimpSizeEntry
 * widget. The usage is quite similar to the one the GtkToggleButton
 * provides. 
 */

#ifndef __GIMP_CHAIN_BUTTON_H__
#define __GIMP_CHAIN_BUTTON_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GIMP_TYPE_CHAIN_BUTTON            (gimp_chain_button_get_type ())
#define GIMP_CHAIN_BUTTON(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_CHAIN_BUTTON, GimpChainButton))
#define GIMP_CHAIN_BUTTON_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CHAIN_BUTTON, GimpChainButtonClass))
#define GIMP_IS_CHAIN_BUTTON(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_CHAIN_BUTTON))
#define GIMP_IS_CHAIN_BUTTON_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CHAIN_BUTTON))

typedef struct GimpChainButton       GimpChainButton;
typedef struct GimpChainButtonClass  GimpChainButtonClass;

typedef enum
{
  GIMP_CHAIN_TOP,
  GIMP_CHAIN_LEFT,
  GIMP_CHAIN_BOTTOM,
  GIMP_CHAIN_RIGHT
} GimpChainPosition;


struct GimpChainButton
{
  GtkTable           table;

  GimpChainPosition  position;
  GtkWidget         *button;
  GtkWidget         *line1;
  GtkWidget         *line2;
  GtkWidget         *pixmap;
  GdkPixmap         *broken;
  GdkBitmap         *broken_mask;
  GdkPixmap         *chain;
  GdkBitmap         *chain_mask;
  gboolean           active;
};

struct GimpChainButtonClass
{
  GtkTableClass parent_class;

  void (* toggled)  (GimpChainButton *gcb);
};


GtkType     gimp_chain_button_get_type   (void);
GtkWidget * gimp_chain_button_new        (GimpChainPosition  position);
void        gimp_chain_button_set_active (GimpChainButton   *gcb,
					  gboolean           is_active);
gboolean    gimp_chain_button_get_active (GimpChainButton   *gcb);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CHAIN_BUTTON_H__ */

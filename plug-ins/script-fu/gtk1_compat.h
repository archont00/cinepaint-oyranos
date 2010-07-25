#include <gtk/gtk.h> /* for GTK_MAJOR_VERSION */
#include <glib.h>

#if GTK_MAJOR_VERSION < 2

# define g_ascii_strtod(text,end)           atof (text)
# define g_ascii_dtostr(text, len, value_d) sprintf (text, "%g", value_d)
# define g_ascii_isdigit(text)              isdigit (text)
# define g_ascii_toupper(text)              toupper (text)
# define g_ascii_tolower(text)              tolower (text)

#else

# define    gtk_color_selection_set_opacity(colorsel,has_opacity) \
	gtk_color_selection_set_has_opacity_control(colorsel,(gboolean)has_opacity)

#endif



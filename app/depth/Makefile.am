## cinepaint/app/depth/Makefile.am

## Copyright July 15, 2003, Robin.Rowe@MovieEditor.com

## License MIT (http://opensource.org/licenses/mit-license.php)

## Process this file with automake to produce Makefile.in


noinst_LTLIBRARIES = libdepth.la

libdepth_la_LIBADD = \
	$(top_builddir)/lib/libcinepaint.la \
	$(GTK_LIBS) \
	$(X_LIBS) \
	$(XMU_LIB)

libdepth_la_SOURCES = \
	bezier_select.c \
	bezier_select.h \
	bezier_selectP.h \
	bfp.h \
	boundary.c \
	brightness_contrast.c \
	brush_select.c \
	brush_select.h \
	brushgenerated.c \
	channel.c \
	color_balance.c \
	color_picker.c \
	color_select.c \
	colormaps.c \
	commands.c \
	curves.c \
	desaturate.c \
	displaylut.c \
	displaylut.h \
	dodgeburn.c \
	equalize.c \
	float16.c \
	float16.h \
	free_select.c \
	gamma_expose.c \
	histogram.c \
	hue_saturation.c \
	image_render.c \
	info_window.c \
	invert.c \
	layer.c \
	levels.c \
	paint_core_16.c \
	paint_funcs_area.c \
	paint_funcs_row_float.c \
	paint_funcs_row_float.h \
	paint_funcs_row_float16.c \
	paint_funcs_row_float16.h \
	paint_funcs_row_u16.c \
	paint_funcs_row_u16.h \
	paint_funcs_row_u8.c \
	paint_funcs_row_u8.h \
	paint_funcs_row_bfp.c \
	paint_funcs_row_bfp.h \
	plug_in.c \
	posterize.c \
	tag.c \
	text_tool.c \
	threshold.c \
	trace.c \
	transform_core.c \
	xcf.c



AM_CPPFLAGS = -DDATADIR=\""$(programdatadir)"\" -DDOTDIR=\""$(programdotdir)"\"



INCLUDES = \
	$(X_CFLAGS) \
	-DPYTHONDIR=\""$(pythondir)"\" \
	$(OYRANOS_CFLAGS) \
	-I$(top_srcdir) \
	-I$(includedir)



## libdepth_la_LDFLAGS = -version-info $(MAJOR_VERSION):$(MINOR_VERSION):$(MICRO_VERSION)



.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done



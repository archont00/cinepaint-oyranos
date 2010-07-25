## app/Makefile.am
## Process this file with automake to produce Makefile.in

scriptdata =

bin_PROGRAMS = cinepaint $(CINEPAINT_REMOTE)

EXTRA_PROGRAMS = \
	cinepaint-remote

SUBDIRS = depth 

## rsr: where did these come from?
##	flipbook.c \
##	flipbook.h \
##	flipbook_cmds.c \
##	flipbook_cmds.h \

cinepaint_SOURCES = \
	about_dialog.c \
	about_dialog.h \
	actionarea.c \
	actionarea.h \
	airbrush.c \
	airbrush.h \
	app_procs.c \
	app_procs.h \
	appenv.h \
	apptypes.h \
	asupsample.c \
	asupsample.h \
	base_frame_manager.c \
	base_frame_manager.h \
	batch.c \
	batch.h \
	blend.c \
	blend.h \
	boundary.h \
	brightness_contrast.h \
	brush.c \
	brush.h \
	brush_edit.c \
	brush_edit.h \
	brush_header.h \
	brushgenerated.h \
	brushlist.c \
	brushlist.h \
	brushlistF.h \
	brushlistP.h \
	bucket_fill.c \
	bucket_fill.h \
	bugs_dialog.c \
	bugs_dialog.h \
	buildmenu.c \
	buildmenu.h \
	by_color_select.c \
	by_color_select.h \
	canvas.c \
	canvas.h \
	channel.h \
	channel_cmds.c \
	channel_cmds.h \
	channel_ops.c \
	channel_ops.h \
	channel_pvt.h \
	channels_dialog.c \
	channels_dialog.h \
	clone.c \
	clone.h \
	cms.c \
	cms.h \
	color_area.c \
	color_area.h \
	color_balance.h \
	color_correction.h \
	color_correction.c \
	color_correction_gui.h \
	color_correction_gui.c \
	color_panel.c \
	color_panel.h \
	color_picker.h \
	color_select.h \
	color_transfer.c \
	color_transfer.h \
	colormaps.h \
	commands.h \
	convert.c \
	convert.h \
	convolve.c \
	convolve.h \
	crop.c \
	crop.h \
	cursorutil.c \
	cursorutil.h \
	curves.h \
	datafiles.c \
	datafiles.h \
	desaturate.h \
	devices.c \
	devices.h \
	dialog_types.h \
	disp_callbacks.c \
	disp_callbacks.h \
	dnd.c \
	dnd.h \
	dodgeburn.h \
	draw_core.c \
	draw_core.h \
	drawable.c \
	drawable.h \
	drawable_cmds.c \
	drawable_cmds.h \
	edit_cmds.c \
	edit_cmds.h \
	edit_selection.c \
	edit_selection.h \
	ellipse_select.c \
	ellipse_select.h \
	equalize.h \
	eraser.c \
	eraser.h \
	errors.c \
	errors.h \
	expose_image.c \
	expose_image.h \
	filename.c \
	filename.h \
	fileops.c \
	fileops.h \
	flatbuf.c \
	flatbuf.h \
	flip_tool.c \
	flip_tool.h \
	floating_sel.c \
	floating_sel.h \
	floating_sel_cmds.c \
	floating_sel_cmds.h \
	frac.c \
	frac.h \
	free_select.h \
	fsdither.h \
	fuzzy_select.c \
	fuzzy_select.h \
	gamma_expose.h \
	gdisplay.c \
	gdisplay.h \
	gdisplayP.h \
	gdisplay_cmds.c \
	gdisplay_cmds.h \
	gdisplay_ops.c \
	gdisplay_ops.h \
	general.c \
	general.h \
	gimage.c \
	gimage.h \
	gimage_cmds.c \
	gimage_cmds.h \
	gimage_mask.c \
	gimage_mask.h \
	gimage_mask_cmds.c \
	gimage_mask_cmds.h \
	global_edit.c \
	global_edit.h \
	gradient.c \
	gradient.h \
	gtk_debug_helpers.h \
    gtk_debug_helpers.c \
	gtkhwrapbox.c \
	gtkhwrapbox.h \
	gtkvwrapbox.c \
	gtkvwrapbox.h \
	gtkwrapbox.c \
	gtkwrapbox.h \
	gximage.c \
	gximage.h \
	histogram.h \
	histogram_tool.c \
	histogram_tool.h \
	hue_saturation.h \
	image_map.c \
	image_map.h \
	image_render.h \
	indexed_palette.c \
	indexed_palette.h \
	info_dialog.c \
	info_dialog.h \
	info_window.h \
	install.c \
	install.h \
	interface.c \
	interface.h \
	internal_procs.c \
	internal_procs.h \
	invert.h \
	iscissors.c \
	iscissors.h \
	layer.h \
	layer_cmds.c \
	layer_cmds.h \
	layer_pvt.h \
	layer_select.c \
	layer_select.h \
	layers_dialog.c \
	layers_dialog.h \
	layers_dialogP.h \
	layout.c \
	layout.h \
	levels.h \
	list.c \
	list.h \
	listF.h \
	listP.h \
	look_profile.c \
	look_profile.h \
	magnify.c \
	magnify.h \
	main.c \
	marching_ants.h \
	measure.c \
	measure.h \
	minimize.c \
	minimize.h \
	move.c \
	move.h \
	noise.c \
	noise.h \
	object.c \
	object.h \
	objectF.h \
	objectP.h \
	ops_buttons.c \
	ops_buttons.h \
	paint_core_16.h \
	paint_funcs.h \
	paint_funcs_area.h \
	paint_options.h \
	paintbrush.c \
	paintbrush.h \
	palette.c \
	palette.h \
	pattern_header.h \
	pattern_select.c \
	pattern_select.h \
	patterns.c \
	patterns.h \
	pencil.c \
	pencil.h \
	perspective_tool.c \
	perspective_tool.h \
	pixel_region.c \
	pixel_region.h \
	pixelarea.c \
	pixelarea.h \
	pixelrow.c \
	pixelrow.h \
	pixmaps.h \
	plug_in.h \
	plugin_loader.c \
	plugin_loader.h \
	posterize.h \
	procedural_db.c \
	procedural_db.h \
	rc.c \
	rc.h \
	rect_select.c \
	rect_select.h \
	rect_selectP.h \
	regex_fake.h \
	resize.c \
	resize.h \
	rotate_tool.c \
	rotate_tool.h \
	scale.c \
	scale.h \
	scale_tool.c \
	scale_tool.h \
	scroll.c \
	scroll.h \
	selection.c \
	selection.h \
	selection_options.h \
	shear_tool.c \
	shear_tool.h \
	shmbuf.c \
	shmbuf.h \
	signal_type.c \
	signal_type.h \
	smudge.c \
	smudge.h \
	spline.c \
	spline.h \
	store_frame_manager.c \
	store_frame_manager.h \
	tag.h \
	tango_tools_pixmaps.h \
	temp_buf.c \
	temp_buf.h \
	text_tool.h \
	threshold.h \
	tile.c \
	tile.h \
	tile_cache.c \
	tile_cache.h \
	tile_manager.c \
	tile_manager.h \
	tile_manager_pvt.h \
	tile_swap.c \
	tile_swap.h \
	tilebuf.c \
	tilebuf.h \
	tips_dialog.c \
	tips_dialog.h \
	tool_options.c \
	tool_options.h \
	tools.c \
	tools.h \
	trace.h \
	trace_macros.h \
	transform_core.h \
	transform_tool.c \
	transform_tool.h \
	undo.c \
	undo.h \
	undo_cmds.c \
	undo_cmds.h \
	vector2d.c \
	vector2d.h \
	wilber.h \
	xcf.h \
	zoom.c \
	zoom.h \
	zoombookmark.c \
	zoombookmark.h \
	menus.c \
	menus.h 

cinepaint_remote_SOURCES = \
	cinepaint-remote.c

## menus.c depends upon store_frame_manager.o

EXTRA_DIST = \
	buttons/anchor.xpm	\
	buttons/anchor_is.xpm	\
	buttons/area.xpm		\
	buttons/area_is.xpm	\
	buttons/backwards.xpm	\
	buttons/backwards_is.xpm	\
	buttons/delete.xpm	\
	buttons/delete_is.xpm	\
	buttons/duplicate.xpm	\
	buttons/duplicate_is.xpm	\
	buttons/forward.xpm	\
	buttons/forward_is.xpm	\
	buttons/look_profile_apply.xpm \
	buttons/look_profile_apply_all.xpm \
	buttons/look_profile_on.xpm \
	buttons/look_profile_open.xpm \
	buttons/lower.xpm		\
	buttons/lower_is.xpm	\
	buttons/new.xpm		\
	buttons/new_is.xpm	\
	buttons/picker_black	\
	buttons/picker_black.xpm	\
	buttons/picker_black_is.xpm	\
	buttons/picker_gray	\
	buttons/picker_gray.xpm	\
	buttons/picker_gray_is.xpm	\
	buttons/picker_mask	\
	buttons/picker_white	\
	buttons/picker_white.xpm	\
	buttons/picker_white_is.xpm	\
	buttons/play_backwards.xpm \
	buttons/play_backwards_is.xpm \
	buttons/play_forward.xpm	\
	buttons/play_forward_is.xpm \
	buttons/raise.xpm		\
	buttons/raise_is.xpm	\
	buttons/stop.xpm		\
	buttons/stop_is.xpm	\
	buttons/update.xpm	\
	buttons/update_is.xpm	\
	buttons/channel.xbm	\
	buttons/eye.xbm		\
	buttons/ibg.xbm		\
	buttons/ifg.xbm		\
	buttons/iflip.xbm		\
	buttons/isave.xbm		\
	buttons/istep.xbm		\
	buttons/layer.xbm		\
	buttons/linked.xbm	\
	buttons/mask.xbm		

library_includedir=${pkgincludedir}/app
library_include_HEADERS = \
	trace.h \
	errors.h

AM_CPPFLAGS = \
	-DPRECISION_CONFIG=$(precision)	\
	-DLOCALEDIR=\""$(localedir)"\"  \
	-DLINUX=$(LINUX)\
	-DREGEX_MALLOC

##	-DLIBDIR=\""$(programplugindir)"\" \
##	-DDATADIR=\""$(programdatadir)"\" \
##	-DCINEPAINTDIR=\""$(programdotdir)"\" \

INCLUDES = \
	$(X_CFLAGS) \
	$(OYRANOS_CFLAGS) \
	-I$(top_srcdir)	\
	-I$(includedir)

cinepaint_LDADD = \
	./depth/libdepth.la \
	$(top_builddir)/lib/libcinepaint.la \
	$(GTK_LIBS) \
	$(X_LIBS) \
	$(OYRANOS_LIBS) \
	$(LCMS_LIB)

cinepaint_remote_LDADD = \
	$(GTK_LIBS) \
	$(X_LIBS) \
	$(XMU_LIB)

## DEPS = $(top_builddir)/lib/libcinepainti.a

## cinepaint_DEPENDENCIES = $(DEPS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

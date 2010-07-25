## Process this file with automake to produce Makefile.in

pluginlibdir = $(programplugindir)/plug-ins

noinst_LTLIBRARIES = libbr_core.la

EXTRA_DIST = 

libbr_core_la_SOURCES = \
	i18n.hpp\
	Rgb.hpp\
	Rgb_utils.hpp\
	Vec2.hpp\
	Vec2_utils.hpp\
	Scheme1D.hpp\
	Scheme2D.hpp\
	Scheme2D_utils.hpp\
	DynArray1D.hpp\
	DynArray2D.hpp\
	br_version.hpp\
	br_types.hpp\
	br_types_array.hpp\
	br_defs.hpp\
	br_macros.hpp\
	br_macros_varia.hpp\
	br_macros.cpp\
	br_enums.hpp\
	br_enums_strings.hpp\
	br_enums_strings.cpp\
	br_eventnames.hpp\
	br_messages.hpp\
	br_messages.cpp\
	br_Image.hpp\
	br_Image.cpp\
	br_Image_utils.hpp\
	br_Image_utils.cpp\
	br_ImgScheme1D.hpp\
	br_ImgScheme2D.hpp\
	br_PackBase.hpp\
	br_PackBase.cpp\
	br_PackImgScheme2D.hpp\
	br_PackImage.hpp\
	br_PackImage_utils.hpp\
	WeightFuncBase.hpp\
	WeightFunc_U8.hpp\
	WeightFunc_U8.cpp\
	WeightFunc_U16.hpp\
	WeightFunc_U16.cpp\
	ResponseFunc_U16.hpp\
	ResponseFunc_U16.cpp\
	spline.hpp\
	spline.cpp\
	Z_MatrixGenerator.hpp\
	ResponseSolverBase.hpp\
	ResponseSolver.hpp\
	HdrCalctorBase.hpp\
	HdrCalctorBase.cpp\
	HdrCalctor_RGB_U8.hpp\
	HdrCalctor_RGB_U8.cpp\
	HdrCalctor_RGB_U16_as_U8.hpp\
	HdrCalctor_RGB_U16_as_U8.cpp\
	FollowUpValuesBase.hpp\
	FollowUpValues_RGB_U8.hpp\
	FollowUpValues_RGB_U8.cpp\
	FollowUpValues_RGB_U16_as_U8.hpp\
	FollowUpValues_RGB_U16_as_U8.cpp\
	HistogramData.hpp\
	HistogramData.cpp\
	histogram_U8.hpp\
	histogram_U8.cpp\
	histogram_U16.hpp\
	histogram_U16.cpp\
	mergeHdr_PackSch2D_RGB_U8.hpp\
	mergeHdr_PackSch2D_RGB_U8.cpp\
	mergeHdr_PackSch2D_RGB_U16.hpp\
	mergeHdr_PackSch2D_RGB_U16.cpp\
	BadPixelProtocol.hpp\
	BadPixelProtocol.cpp\
	curve_files.hpp\
	curve_files.cpp\
	input_statistics.hpp\
	input_statistics.cpp\
	testtools.hpp\
	testtools.cpp\
	Distributor.hpp\
	Distributor.cpp\
	FilePtr.hpp\
	FilePtr.cpp\
	ProgressInfo.hpp\
	TheProgressInfo.hpp\
	TheProgressInfo.cpp\
	Exception.hpp\
	Exception.cpp\
	StatusLineBase.hpp\
	Br2HdrManager.hpp\
	Br2HdrManager.cpp\
	Br2Hdr.hpp\
	EventReceiver.hpp\
	RefpicReceiver.hpp\
	correl_RGB.hpp\
	CorrelMaxSearchBase.hpp\
	CorrelMaxSearchBase.cpp\
	CorrelMaxSearch_RGB.hpp\
	DisplcmFinderBase.hpp\
	DisplcmFinderBase.cpp\
	DisplcmFinderCorrel_.hpp\
	DisplcmFinderCorrel_.cpp\
	DisplcmFinderCorrel.hpp\
	DisplcmFinderCorrel_RGB.hpp\
	Run_DisplcmFinder.hpp\
	Run_DisplcmFinder.cpp\
	path_utils.hpp\
	path_utils.cpp\
	readEXIF.hpp\
	readEXIF.cpp


AM_CXXFLAGS = \
	$(X_CXXFLAGS)

INCLUDES = \
	$(X_CFLAGS) \
	$(FLTK_CFLAGS) \
	-I$(top_srcdir)	\
	-I$(includedir) \
        -I..


## libbr_core doesn't need neither Gtk nor cinepaint.la nor Fltk
#libbr_core_la_LIBADD = \
#        $(top_builddir)/lib/libcinepaint.la \
#        $(GTK_LIBS)


#DEPS = $(top_builddir)/lib/libcinepaint.la

#libbr_core_la_DEPENDENCIES = $(DEPS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
	@for subdir in $(SUBDIRS); do \
	  files=`cd $$subdir; $(MAKE) files | grep -v "make\[[1-9]\]"`; \
	  for file in $$files; do \
	    echo $$subdir/$$file; \
	  done; \
	done

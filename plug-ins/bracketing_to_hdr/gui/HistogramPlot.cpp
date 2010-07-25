/*
 * HistogramPlot.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
 *
 * Copyright (c) 2005-2006  Hartmut Sbosny  <hartmut.sbosny@gmx.de>
 *
 * LICENSE:
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
/**
  HistogramPlot.cpp
*/

#include "HistogramPlot.hpp"
#include "../br_core/Br2Hdr.hpp"            // br::Br2Hdr::Instance()
#include "../br_core/HistogramData.hpp"
#include "../br_core/histogram_U8.hpp"      // compute_histogram_RGB_U8()
#include "../br_core/histogram_U16.hpp"     // compute_histogram_RGB_U16()
#include "../br_core/br_defs.hpp"           // BR_DEBUG_RECEIVER
#include "../br_core/br_enums_strings.hpp"  // DATA_TYPE_STR[] etc
#include "TNT/tnt_array1d_utils.hpp"        // <<-Op for Array1D<>


using namespace br;

static Br2HdrManager &  theBr2Hdr = Br2Hdr::Instance();

#if 0
static void msg_no_impl (const Image & img)
{
    std::cout << "Sorry, no histogram implementation for that image structure:"
      << "\n    channels: " << img.channels()
      << ",  data type: " << DATA_TYPE_SHORT_STR[img.data_type()]
      << ",  storing scheme: " << STORING_SCHEME_STR[img.storing()] 
      << '\n';   
}
#endif

/**+*************************************************************************\n
  Ctor...
******************************************************************************/
HistogramPlot::HistogramPlot(int X, int Y, int W, int H, const char* la)
  : 
    CurvePlotClass (X,Y,W,H, la),
    pic_     (0),               // default: first image
    channel_ (-1),              // default: all channels
    plot_ (*this)               // to make code more readable
{ 
    CTOR(label())
    begin();
      retick_under_resize (true);
    end();                      // we leave a closed group
    build();    
}

/**+*************************************************************************\n
  build()   -   WITHOUT redraw()!
  
  Compute the histogram of image \a pic_ and build the plot widget.
******************************************************************************/
void HistogramPlot::build()
{
    //PRINTF(("%s() [pic=%d, ch=%d]...\n",__func__,pic_,channel_));
    plot_.clear();    // --> empty_look(true)
    
    if (! theBr2Hdr.size()) return;                // no images
    IF_FAIL_DO (pic_ < theBr2Hdr.size(), return);  // `pic_´ out of range
    
    if (channel_ < 0) build_all_channels();
    else              build_single_channel();
}


/**+*************************************************************************\n
  build_single_channel()   -   WITHOUT redraw()!
  
  Compute the histogram of image \a pic_ for channel \a channel_ and build the
   plot widget. Realy this function name?

  Zum Referenzzaehlen: Die lokalen Array1D-Objekte \a X, \a Y etc. werden am
   Funktionsende wieder freigeben. Aber die per add(...) gemachten Haendelkopien
   leben solange dieses Widget lebt.
******************************************************************************/
void HistogramPlot::build_single_channel()
{
    IF_FAIL_DO (0 <= channel_ && channel_ < 3, return);
    
    const BrImage &  image = theBr2Hdr.image(pic_);
    HistogramData  histo;
    
    /*  The X-values of the histogram */
    Array1D<PlotValueT>  X(256);
    
    switch (image.image_type())
    {
    case IMAGE_RGB_U8:
        compute_histogram_RGB_U8 (histo, image, channel_);
        for (int i=0; i < 256; i++)  X[i]=i;        // 0..255
        break;
        
    case IMAGE_RGB_U16:
        compute_histogram_RGB_U16 (histo, image, channel_);
        for (int i=0; i < 256; i++)  X[i] = i*257;  // 0..65535
        break;
    
    default:
        NOT_IMPL_IMAGE_TYPE (image.image_type());
        return;
    }
      
    /*  The Y-values of the histogram */
    Array1D<PlotValueT>  Y (histo.data.dim());
    PlotValueT           fac = 100.0 / image.n_pixel();
    for (int i=0; i < Y.dim(); i++)
      Y[i] = fac * histo.data[i];  // ratio in percent

    //std::cout << Y;  
    Fl_Color  color_histo[3] = {FL_RED, FL_GREEN, FL_BLUE};
    
    plot_.make_current_cartes();   // needed for plot_.add()
    plot_.start_insert();          // init_minmax() + empty_look(false)
    plot_.add (X,Y);
    plot_.back().color (color_histo[channel_]);
    plot_.style (STYLE_HISTOGRAM);
    plot_.set_y_minmax (0.0, plot_.ymax());
}


/**+*************************************************************************\n
  build_all_channels()   -   WITHOUT redraw()!
  
  Compute the histograms of image \a pic_ for all channels and build the plot.
******************************************************************************/
void HistogramPlot::build_all_channels()
{
    const BrImage &  image = theBr2Hdr.image(pic_);
    HistogramData  histo[3];
    
    /*  The X-values of the histogram */
    Array1D<PlotValueT>  X(256);
    
    switch (image.image_type())
    {
    case IMAGE_RGB_U8:
        compute_histogram_RGB_U8 (histo, image);
        for (int i=0; i < 256; i++)  X[i]=i;        // 0..255
        break;
        
    case IMAGE_RGB_U16:
        compute_histogram_RGB_U16 (histo, image);
        for (int i=0; i < 256; i++)  X[i] = i*257;  // 0..65535
        break;
        
    default:
        NOT_IMPL_IMAGE_TYPE (image.image_type());
        return;
    }
    
    /*  The Y-values of the histograms */
    Array1D<PlotValueT>  Y_r (histo[0].data.dim());
    Array1D<PlotValueT>  Y_g (histo[1].data.dim());
    Array1D<PlotValueT>  Y_b (histo[2].data.dim());
    PlotValueT           fac = 100.0 / image.n_pixel();
    for (int i=0; i < Y_r.dim(); i++)   // Assume identic dim. for all channels
    {
      Y_r[i] = fac * histo[0].data[i];  // ratio in percent
      Y_g[i] = fac * histo[1].data[i];  // ratio in percent
      Y_b[i] = fac * histo[2].data[i];  // ratio in percent
    }
    
    plot_.make_current_cartes();   // needed for plot_.add()
    plot_.start_insert();          // init_minmax() + empty_look(false)
    plot_.add (X,Y_r);  plot_.back().color (FL_RED);
    plot_.add (X,Y_g);  plot_.back().color (FL_GREEN);
    plot_.add (X,Y_b);  plot_.back().color (FL_BLUE);
    plot_.style (STYLE_LINES);
    plot_.set_y_minmax (0.0, plot_.ymax());
}

// END OF FILE

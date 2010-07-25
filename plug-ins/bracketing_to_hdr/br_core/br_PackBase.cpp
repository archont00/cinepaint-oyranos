/*
 * br_PackBase.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  br_PackBase.cpp
    
  Base class of the Pack classes
*/

#include <cmath>                  // log()
#include <cassert>                // assert()
#include "br_PackBase.hpp"
#include "br_Image.hpp"           // BrImgVector (Ctor argument)
#include "br_enums_strings.hpp"   // DATA_TYPE_STR[] etc.
#include "br_macros_varia.hpp"    // SPUR()


namespace br {

/////////////////////////////////////////
///
///  PackBase  -  class implementation
///
/////////////////////////////////////////

/**+*************************************************************************\n
  Ctor

  @param imgVec: vector of BrImage's (input images).

  - Allocs some vectors of dimension imgVec.size_active()
  - Reads dim1 and dim2 from \a imgVec.intersection and computes buffer_size
  - Takes over the identic meta data of the activated images
  - Checks consictency between these images
  - Reads the individual data ID, time, logtime into our vectors. 
  
  @internal Intersection wird genommen wie von imgVec geliefert, es finden hier
   keine Aktualisierungsbemuehungen statt.
******************************************************************************/
PackBase::PackBase (BrImgVector & imgVec) 
  : 
    dim1_           (0),
    dim2_           (0),
    channels_       (0),
    byte_depth_     (0),
    bit_depth_      (0),
    buffer_size_    (0),
    data_type_      (DATA_NONE),
    storing_scheme_ (STORING_NONE),
    ids_            (imgVec.size_active()),  // allokiert
    times_          (ids_.size()),           // allokiert
    logtimes_       (ids_.size())            // allokiert
{
    CTOR("")
    /*  Have we enough active images? Check ungallant, besser werfen */
    assert (ids_.size() > 1);
    
    /*  The intersection area of all activated images */
    const Rectangle &  section = imgVec.intersection();
    assert (! section.is_empty());
    
    /*  Take over the image data... */
    bool first = true;
    int k = 0;                          // counts active images
    for (int i=0; i < imgVec.size(); i++)
    {
      BrImage & img = imgVec.image(i);
      if (img.active()) 
      { 
        if (first) {
          /*  Take over the identic meta data */
          dim1_           = section.dim1();
          dim2_           = section.dim2();
          //dim1_           = img.dim1();   // since intersection() it can differ
          //dim2_           = img.dim2();
          channels_       = img.channels();
          byte_depth_     = img.byte_depth();
          bit_depth_      = img.bit_depth();
          buffer_size_    = (img.buffer_size() * section.n_pixel()) / img.n_pixel();
          data_type_      = img.data_type();
          storing_scheme_ = img.storing();
        
          first = false;
        } 
        else {
          /*  Check consistency with first image... (ungallant) */
          //assert (img.dim1() == dim1_); since intersection() no longer necessary
          //assert (img.dim2() == dim2_);
          assert (img.channels() == channels_);
          assert (img.byte_depth() == byte_depth_);
          assert (img.bit_depth() == bit_depth_);
          assert (img.data_type() == data_type_);
          assert (img.storing() == storing_scheme_);
          assert (k < (int)size());        // size_active() could be wrong
        }
      
        /*  Take over the individual data */
        ids_     [k] = img.imageID();
        times_   [k] = img.time();
        logtimes_[k] = log( times_[k] );
      
        SPUR(("use image [%d] \"%s\", time=%f\n",
            i, img.name(), img.exposure.time))
      
        k++;
      }
    }   
}

/**+*************************************************************************\n
  Returns Pack position (index) of image with ID \a id or -1 if no such image.
******************************************************************************/
int PackBase::pos_of_ID (ImageID id) const
{
    if (id == 0) return -1;  // Short cut asserting that no image has ID==0!

    for (int i=0; i < size(); i++)
      if (imageID(i) == id)  return i;
    
    return -1;
}

/**+*************************************************************************\n
  Sets the i-th exposure time to \a tm and the logtime to log(tm).
******************************************************************************/
void PackBase::set_time (int i, double tm)
{
    times_[i] = tm;
    logtimes_[i] = log(tm); 
}

/**+*************************************************************************\n
  report()
******************************************************************************/
void PackBase::report (ReportWhat what)  const
{
    std::cout << "=== Pack Report === [" << size() << "]:"
              << "\n  [ " << width() << " x " << height() << " ]";
    if (what & REPORT_BASE) {
      std::cout 
        << "\n  [Base Data...]"
        << "\n    channels   : " << channels()
        << "\n    DataType   : " << DATA_TYPE_STR[data_type()]
        << "\n    byte_depth : " << byte_depth()
        << "\n    bit_depth  : " << bit_depth()
        << "\n    buffer_size: " << buffer_size() 
        << "\n    storing scheme: " << STORING_SCHEME_STR[storing()];
    }
    if (what & REPORT_EXPOSE_INFO) {
      std::cout << "\n  [Times...] ";
    
      for (int i=0; i < size(); i++) {
        std::cout << "\n    [" << i << "]  " << time(i) 
                  << ",  " << logtime(i)
                  << "\t\t(ID=" << imageID(i) <<')';
      }
    }
    std::cout << '\n';            
}


}  // namespace "br"

// END OF FILE

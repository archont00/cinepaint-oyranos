/*
 * FollowUpValues_RGB_U16_as_U8.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file FollowUpValues_RGB_U16_as_U8.hpp
*/
#ifndef FollowUpValues_RGB_U16_as_U8_hpp
#define FollowUpValues_RGB_U16_as_U8_hpp

#include "FollowUpValuesBase.hpp"
#include "br_types.hpp"           // type `uint16', `PlotValueT'
#include "br_types_array.hpp"     // type `ChannelFollowUpArray'
#include "br_PackImgScheme2D.hpp" // PackImgScheme2D<>
#include "Rgb.hpp"                // Rgb<>
#include "br_macros.hpp"          // CTOR(), DTOR()
#include "DynArray2D.hpp"         // DynArray2D<>



namespace br {

using namespace TNT;
 
/**===========================================================================

  @class  FollowUpValues_RGB_U16_as_U8

  Fuer ein U16-Pack werden reduzierte Folgewerte ermittelt, d.h. die 16-bit
   Daten werden auf 8-bit reduziert (Hauptwerte wie Folgewerte). Das gibt 
   256 Zaehlkanaele (256 reduzierte Hauptwerte 0..255), und die Folgewerte werden
   ebenfalls als reduzierte summiert. 
  
*============================================================================*/
class FollowUpValues_RGB_U16_as_U8 : public FollowUpValuesBase
{
    //  The Pack-type we work up (Ctor argument)
    typedef PackImgScheme2D< Rgb<uint16> >  Pack;

    //  Type for summation
    typedef double sum_t;         // double or uint64

    //  Helper structure for summation
    struct ChSum {                // "ChannelSum"
      sum_t  s, s2;               // Sum(z) and Sum(z^2); double or uint64
      ChSum(): s(0), s2(0)  {}    // Default-Ctor: zeros
    };

private:

    //  Number of counting intervalls as a constant
    static const unsigned  N_Grid = 256;
    
    //  The maximal possible *reduced* z-value
    static const uint8   z_max_red_  = 255;

    //  The pack given in the Ctor
    Pack  pack_;                  // object, no ref!

    //  Number of layers or images in the pack
    int  nlayers_;                // == pack_.size()

    //  Three statistic arrays (for each channel)
    ChannelFollowUpArray  follow_[3];

    
public:
    
    /**
    *   Ctor & Dtor...
    *
    *   @param pack: The pack of images the calculation shall be done for.
    */
    FollowUpValues_RGB_U16_as_U8 (const Pack &);
    ~FollowUpValues_RGB_U16_as_U8()                {DTOR("")}
    
    /**
    *   The References of the (computed) statistics arrays. 
    *    Externally needed by `Z_MatrixGenerator´.
    */
    const ChannelFollowUpArray & channelArray_R() const  {return follow_[0];}
    const ChannelFollowUpArray & channelArray_G() const  {return follow_[1];}
    const ChannelFollowUpArray & channelArray_B() const  {return follow_[2];}
    const ChannelFollowUpArray & channelArray (int channel) const  
            {return follow_[channel];}      // Attention: no bounds check!

    //=====================================================
    //  Realisation of the abstract interface...
    //   Description: see base-class `FollowUpValuesBase´.
    //=====================================================
    
    void compute_z_average (int refpic);
    
    Array1D <PlotValueT>  
         get_CurvePlotdataX()  const;
    
    void get_CurvePlotdataY (
            Array1D <PlotValueT> & Y,      
            Array1D <PlotValueT> & YerrLo,
            Array1D <PlotValueT> & YerrHi,
            int                  pic,
            int                  channel,
            bool                 bounded = true)
      const;

    Array1D <PlotValueT> 
         get_CurvePlotdataY (
            int             pic,
            int             channel)
      const;

    void write_CurvePlotdata (
            const char*     fname,
            int             pic,
            int             channel,
            bool            bounded = true) 
      const;

    Array1D<PlotValueT> 
         get_RefpicHistogramPlotdata (
            int             channel,
            size_t*         maxval,
            PlotValueT      scale_minval,
            PlotValueT      scale_maxval )
      const;

    //===========================================
    // ... End of abstract interface realisation
    //===========================================
            
    /**
    *   List the statistics array on stdout (debug helper).
    */
    void list_ChannelArray (int channel) const;


private:
    //===================
    //  Debug helpers...
    //===================
    
    void list_ChSum (const DynArray2D<ChSum>& h, char c) const;
    void analyze_ChSum (const DynArray2D<ChSum>& h, char c) const;
};


}  // namespace "br"

#endif  // FollowUpValues_RGB_U16_as_U8_hpp

// END OF FILE

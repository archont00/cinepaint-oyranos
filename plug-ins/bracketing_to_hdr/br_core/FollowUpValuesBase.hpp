/*
 * FollowUpValuesBase.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file FollowUpValuesBase.hpp
  
  Abstract base class of the \c FollowUpValues classes. Interface definition.
   Originally intented both for RGB- and Channel-Packs - maybe to wide.
  
  @note Die drei K1_FollowUpValues-Objekte bei kanalweiser Speicherung
   beitzen keine Funktionen mit "channel"-Parameter. Na dann ignoriere doch
   einfach dort das channel-Argument!
*/
#ifndef FollowUpValuesBase_hpp
#define FollowUpValuesBase_hpp

#include "br_types.hpp"         // type PlotValueT
#include "br_PackBase.hpp"      // class PackBase
#include "TNT/tnt_array1d.hpp"  // TNT::Array1D<>


namespace br {

/**========================================================================

  @class FollowUpValuesBase
 
  Abstract base class of the \c FollowUpValues classes. Interface definition.
  
*=========================================================================*/
class FollowUpValuesBase 
{
private:

    /**  PackBase reference. 
         Each \c FollowUpValues derived from \c FollowUpValuesBase must init 
          it - with its \c Pack. Const or non-const?     */
    const PackBase &  packBase_;

protected:
    
    /**  Reference picture, for which the last computation was done  */
    int  refpic_;    // to set in computation methods!
    
    /**  Number of items resp. intervalls wherein we subsum the z-values.
          Second dimension of the ChannelFollowUpArray's.               */
    unsigned  n_grid_;
    
public:    
    //=====================================
    //  Concret (non-abstract) interface...
    //=====================================
    
    /**
    *   Virtual Dtor
    */
    virtual ~FollowUpValuesBase()  {}       

    int refpic() const             {return refpic_;}
    unsigned n_grid() const        {return n_grid_;}
    
    
    //=====================================
    //  Abstract interface...
    //=====================================
    
    /**
    *   Compute the three statistics arrays for the given \a refpic via the 
    *    z_average() method. Mittlerer Folgewert in jedem Folgebild zum Wert
    *    \a z im Ref-Bild \a refpic.
    *
    *   @param refpic: Reference picture
    */
    virtual void compute_z_average (int refpic) = 0;
    

    /**
    *   @return Array of the X-values (co-domain) of the plot curves.
    */
    virtual TNT::Array1D <PlotValueT>  
         get_CurvePlotdataX() const  = 0;
       
    
    /**
    *   Extract the Y-values of the follow-up curve inclusive error bar values
    *    of image \a pic (to the \a refpic used in compute_*()) from the data 
    *    array computed by compute_*().
    *
    *   @param Y: (O) [n_zvals]-Feld der Folgewerte (Ordinate)
    *   @param YerrLo: (O) [n_zvals]-Feld der unteren Streuungsgrenze
    *   @param YerrHi: (O) [n_zvals]-Feld der oberen Streuungsgrenze
    *   @param pic: (I) wanted picture, in [0..nlayers_)
    *   @param channel: (I) wanted color channel, in {0,1,2}
    *   @param bounded=true: (I) Reale Wertgrenzen beruecksichtigen?
    *      Wenn true, dann \a YerrLo und \a YerrHi so justiert, dass die Fehlerbalken 
    *      die z-Werte 0 und z_max nicht ueberschreiten; wenn false, symmetrisch
    *      zum z-Wert (koennen dann auch irreale Werte wie z > z_max annehmen).
    */
    virtual void 
         get_CurvePlotdataY (
            TNT::Array1D <PlotValueT> & Y,      
            TNT::Array1D <PlotValueT> & YerrLo,
            TNT::Array1D <PlotValueT> & YerrHi,
            int                         pic,
            int                         channel,
            bool                        bounded = true)
      const = 0;

    
    /** 
    *   Return the mere Y-values (without error bars) of the follow-up curve of
    *    image \a pic (to the used \a refpic in compute_*()) from the data 
    *    array computed by compute_*().
    *
    *   @param pic: (I) wanted picture, in [0..nlayers_)
    *   @param channel: (I) wanted color channel, in {0,1,2}
    *   @return: [n_zvals]-Array1D of follow-up Y-values \c follow_[pic][].z; 
    *      here allocated.
    */
    virtual TNT::Array1D <PlotValueT> 
         get_CurvePlotdataY (
            int             pic,
            int             channel)
      const = 0;

    
    /**
    *   Write the follow-up curve of image \a pic (to the used \a refpic) into
    *    a file in gnuplot readable form.
    *  
    *   @param fname: (I) file name
    *   @param pic: (I) wanted picture, in [0..nlayers)
    *   @param channel: (I) wanted channel, in {0,1,2}
    *   @param bounded=true: (I) Reale Wertgrenzen beruecksichtigen? (s.o.)
    */
    virtual void 
         write_CurvePlotdata (
            const char*     fname,
            int             pic,
            int             channel,
            bool            bounded = true) 
      const = 0;

    
    /**
    *   Return plotable histogram data \a n(z) for the \a refpic, i.e. the
    *    number \a n of pixels with z-value \a z in the reference image.
    *
    *   The histogram data are scaled to the intervall [scale_minval,
    *    scale_maxval]. For axis ticking the *real* maximal histogram value
    *    ('n') is given in \a maxval. 
    *
    *   @param channel: (I) which channel; in {0,1,2} for RGB
    *   @param maxval:  (O) maximal value (n) in the histogram
    *   @param scale_minval: (I) scale min. histogram value (n=0) to this
    *   @param scale_maxval: (I) scale max. histogram value to this
    *   @return Array1D of histogram data \a n(z). Null-Array for error,
    *      e.g. `channel' out of range.
    *
    *   @todo  Ansatz nicht allgemein genug. Fuer U16 kann kein Vektor von 65535
    *     Werten (fuer jedes \a z ein \a n) mehr geliefert werden. Es braucht
    *     dann Intervalle, z.B. generell 256 an der Zahl. Die X-Achse geht
    *     dann nicht mehr automatisch von 0..256 und sollte mitgeteilt
    *     werden, ebenso die Intervallparameter.
    */
    virtual TNT::Array1D<PlotValueT> 
         get_RefpicHistogramPlotdata (
            int             channel,
            size_t*         maxval,
            PlotValueT      scale_minval,
            PlotValueT      scale_maxval ) 
      const = 0;


protected:
    
    /**
    *   Ctor  -  protected to avoid attempted instances of abstract class
    *
    *   @param pack: The base of the Pack the calculation shall be done for.
    *   @param ngrid: Number of counting intervalls; usually 256.
    *
    *   Principle: Each \c FollowUpValues is to init with a \c Pack - the base 
    *    of the \c FollowUpValues's is to init with the base of the \c Pack's.
    */
    FollowUpValuesBase (const PackBase & base, int ngrid) 
      : packBase_(base),     
        refpic_  (-1),               // mark for "no computation by now"
        n_grid_  (ngrid)
      {}

};


}  // namespace "br"

#endif  // FollowUpValuesBase_hpp

// END OF FILE


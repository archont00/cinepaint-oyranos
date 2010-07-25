/*
 * br_PackBase.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file br_PackBase.hpp
  
  Base class of the Pack classes.
   
  Contents:
   - class `PackBase'  
*/
#ifndef br_PackBase_hpp
#define br_PackBase_hpp

#include <vector>                 // std::vector<>
#include "TNT/tnt_array1d.hpp"    // TNT::Array1D<>
#include "br_Image.hpp"           // BrImgVector (Ctor argument)
#include "br_macros.hpp"          // ASSERT(), RELAX


namespace br {

/**===========================================================================

  @class  PackBase
 
  Basis der Pack-Klassen. Pack's sind typisiert, PackBase enthaelt das
   Typunabhaengige der Pack's.

  Ein "Pack" vereint die Bilder (oder Kanaele) einer HDR-Rechnung, es ist das
   numerische Basisobjekt. Alle Bilder werden von identischer Dimension und
   Datentyp angenommen. Gehalten werden nur die numerisch noetigen
   Informationen. Zusaetzlich bereitgestellt wird vor allem ein *Vektor* der
   Belichtungszeiten, der fuer die HDR-Rechnungen nuetzlich.
  
  Konstruiert wird ein Pack aus einem BrImgVector als Argument, wobei nur die
   aktivierten Bilder uebernommen werden.
  
  Fuer die Referenzvariante hier enthaelt der vector nicht lediglich uchar- oder
   ImageData-Zeiger, sondern ref-zaehlende uchar-Objekte. Um die Uebergabe vom 
   BrImgVector einfach zu gestalten, am besten dieselben Minimalobjekte, die
   den Puffer schon in BrImgVector verwalten. Z.Z. sind das Image, abgeleitet
   von Array1D<uchar>. Der minimale Gehalt waere also das Array1D<char> selbst.
   Bedeutete einen vector von Array1D<uchar>. Aber doch wohl besser noch dim1,
   dim2 etc hinzu, also von Image-Objekten, aber ohne Exposure- und Statistik-
   Daten, also kein *Br*Image!
   
  Pack besitzt den Standard-Copy-Ctor, kopiert wird elementweise. Das waeren die
   std::vector'en (mit ihren Image-Objekten) sowie die zentralen Metadaten. Die
   Bildpuffer selbst in Image aber kopieren referenzzaehlend. Falls das immer 
   noch zu aufwendig, koennte ein Pack auch noch in ein ref-zaehlendes Haendel
   gewickelt werden.
   
  Verworfene IDEE: Pack von Image ableiten, weil das gerade alle Metadaten
   enthielte. Zuviel waere nur der `name_', aber der koennte sinnvoll umgewidmet
   werden. Und zuviel natuerlich der Datenpuffer selbst, der vom Array1D<uchar>
   verwaltet wird. Das schon ernster. Der  muesste auf Groesse Null gesetzt werden.
   Die Dimensionsangabe dim() stimmte dann nicht mehr mit buffer_size() im
   Sinne des Pack ueberein, in Image muessten dim() und buffer_size() entkoppelt
   werden durch eine eigene Variable `buffer_size_'.
   
   - Vorteile waeren: Alles fuer Image verfuegbare Funktionalitaet sogleich auch
      fuer Pack verfuegbar; z.B. CHECK_IMG_TYPE(img). 
   - Gefahr und Nachteil: Operationen die auch mit dem Puffer operieren, gaeben 
      hier nichts oder wuerden gar verwirrt.

=============================================================================*/
class PackBase
{
private:

    //  The identic meta data...
    int       dim1_;              // outer dimension
    int       dim2_;              // inner dimension      
    int       channels_;
    int       byte_depth_;             
    int       bit_depth_;
    size_t    buffer_size_;       // memory of *one* image 
    DataType  data_type_; 
    StoringScheme storing_scheme_;

    //  Vector of the image ID's
    std::vector <ImageID>  ids_;
   
    //  Vector of the exposure times and logarithm of it
    TNT::Array1D<double>  times_, logtimes_; 
  
public:

    // Ctor & Dtor...
    PackBase (BrImgVector &);
    virtual ~PackBase()                 {DTOR("")}
    
    int       size() const              {return ids_.size();}  // number of images
    int       dim1() const              {return dim1_;}
    int       dim2() const              {return dim2_;}
    int       width() const             {return dim2_;}
    int       height() const            {return dim1_;}
    size_t    n_pixel() const           {return (size_t)(dim1_) * dim2_;}
    int       channels() const          {return channels_;}
    int       byte_depth() const        {return byte_depth_;}
    int       bit_depth() const         {return bit_depth_;}
    size_t    buffer_size() const       {return buffer_size_;}  // "bytesize"
    DataType  data_type() const         {return data_type_;}
    StoringScheme storing() const       {return storing_scheme_;}

    //  ID of the i-th image:
    ImageID imageID (int i) const  {return (0<=i && i<size()) ? ids_[i] : 0;}
    
    //  Position (index) of image with ID `id' or -1 if no such image
    int pos_of_ID (ImageID id) const;
  
    //  Exposure time of the i-th image:
    double time    (int i) const        {return times_[i];}
    double logtime (int i) const        {return logtimes_[i];}
  
    //  Refs of the exposure [log] time vectors:
    const TNT::Array1D<double> & timeVec()    const  {return times_;}
    const TNT::Array1D<double> & logtimeVec() const  {return logtimes_;}

    //  Sets the i-th exposure time to `tm' and the logtime to log(tm)
    void set_time (int i, double tm);
      
    virtual void report (ReportWhat = REPORT_DEFAULT) const;
};



/**
  These macros check the pack type and output a msg [only], if something wrong:
*/
#ifdef BR_CHECK_PACK_TYPE

# define CHECK_PACK_TYPE_U8(pack) \
                ASSERT(pack.data_type() == DATA_U8, RELAX);\
                ASSERT(pack.byte_depth() == 1, RELAX);\
                ASSERT(pack.channels() == 1, RELAX);

# define CHECK_PACK_TYPE_RGB_U8(pack) \
                ASSERT(pack.data_type() == DATA_U8, RELAX);\
                ASSERT(pack.byte_depth() == 1, RELAX);\
                ASSERT(pack.channels() == 3, RELAX);\
                ASSERT(pack.storing() == STORING_INTERLEAVE, RELAX);

# define CHECK_PACK_TYPE_U16(pack) \
                ASSERT(pack.data_type() == DATA_U16, RELAX);\
                ASSERT(pack.byte_depth() == 2, RELAX);\
                ASSERT(pack.channels() == 1, RELAX);

# define CHECK_PACK_TYPE_RGB_U16(pack) \
                ASSERT(pack.data_type() == DATA_U16, RELAX);\
                ASSERT(pack.byte_depth() == 2, RELAX);\
                ASSERT(pack.channels() == 3, RELAX);\
                ASSERT(pack.storing() == STORING_INTERLEAVE, RELAX);
#else

# define CHECK_PACK_TYPE_U8(pack)
# define CHECK_PACK_TYPE_RGB_U8(pack)
# define CHECK_PACK_TYPE_U16(pack)
# define CHECK_PACK_TYPE_RGB_U16(pack)

#endif


}  // namespace "br"

#endif // PackRef_hpp

// END OF FILE

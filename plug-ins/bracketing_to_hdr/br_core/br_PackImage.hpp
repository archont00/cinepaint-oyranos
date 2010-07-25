/*
 * br_PackImage.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file  br_PackImage.hpp  -  Pack of untyped *ref-counting* Image objects
  
  Scheint nicht mehr verwendet zu werden. Pruefen. 
  
  Unterschied zu `PackImgScheme2D´:  Letzteres haelt Vektor von typisierten 
   ImgScheme2D's, wir hier Vektor von untypisierten `Image's. D.h. letzeres
   ist Template, wir hier nicht.
  
   
  Ein Pack vereint die Bilder (oder Kanaele) einer HDR-Errechnung, es ist das
   numerische Basisobjekt einer solchen. Alle Bilder werden von identischer 
   Dimension und Datentyp angenommen. Gehalten werden nur die numerisch noetigen
   Informationen. Zusaetzlich bereitgestellt wird ein *Vektor* der Belichtungs-
   zeiten.
  
  Konstruiert wird ein Pack aus einem BrImgVector als Argument, wobei nur
   die aktivierten Bilder uebernommen werden.
  
  Fuer die Referenzvariante hier enthaelt der vector nicht schlichte uchar-Zeiger,
   sondern ref-zaehlende uchar-Objekte. Um die Uebergabe vom BrImgVector einfach
   zu gestalten, am besten dieselben Minimalobjekte, die den Puffer schon in
   BrImgVector verwalten. Z.Z. sind das Image, abgeleitet von Array1D<uchar>. 
   Der minimale Gehalt duerfte der Array1D<char> selbst sein. Bedeutete einen
   vector von Array1D<uchar>. Aber wohl doch besser noch dim1, dim2 etc dazu,
   also von Image-Objekten, aber ohne Exposure- und Statistik- Daten, also
   kein *Br*Image.
   
  Pack besitzt den Standard-Copy-Ctor, kopiert wird elementweise. Das waeren die
   std::vector'en (mit ihren Image-Objekten) sowie die zentralen Metadaten. Die
   Bildpuffer selbst aber kopieren referenzzaehlend. Falls das zu aufwendig,
   koennte ein Pack auch noch in ein ref-zaehlendes Haendel gewickelt werden.
   
  IDEE: Pack von Image ableiten, denn das enthaelt gerade alle Metadaten.
   Zuviel waere nur der `name_', aber der koennte durchaus sinnvoll verwendet
   werden. Und zuviel natuerlich der Datenpuffer selbst, der vom Array<uchar>
   verwaltet wird. Das schon ernster. Der  muesste auf Groesse Null gesetzt werden.
   Die Dimensionsangabe dim() stimmte dann nicht mehr mit buffer_size() im 
   Sinne des Pack ueberein, in Image muessten dim() und buffer_size()
   entkoppelt werden durch eine eigene Variable `buffer_size_'.
   
   Vorteile: Alles fuer Image verfuegbare Funktionalitaet waere sogleich auch 
    fuer Pack verfuegbar; z.B. CHECK_IMG_TYPE(img). 
   Gefahr darin: Operationen die auch mit dem Puffer operieren, gaeben hier
    nichts oder wuerden gar verwirrt.
   
  Content:
   - class `PackImage'  
*/
#ifndef br_PackImage_hpp
#define br_PackImage_hpp


#include <vector>
#include "br_Image.hpp"         // BrImgVector (Ctor argument), Image
#include "br_PackBase.hpp"


namespace br {


/**===========================================================================

  @class  PackImage
 
  Storing scheme: vector of ref counting `Image' objects.  

=============================================================================*/
class PackImage : public PackBase
{
private:

    //  Vector of the `Image's maintaining untyped data buffers...
    std::vector <Image>  images_;

public:

    //  Ctor...
    PackImage (BrImgVector &);
    
    //  The i-the image...
    const Image & operator[] (int i) const    {return images_[i];}
    Image &       operator[] (int i)          {return images_[i];}  
    
    //  Data buffer of the i-th image...
    const unsigned char* buffer (int i) const {return images_[i].buffer();}
    unsigned char*       buffer (int i)       {return images_[i].buffer();}
    
    //void Report(ReportWhat = REPORT_DEFAULT) const;   
};



//------------------------------------------------------------
// Ctor...
//  - Take over the activated images only
//-------------------------------------------------------------
PackImage::PackImage (BrImgVector & imgVec) 
  : 
    PackBase (imgVec),
    images_  (size())                   // allokiert
{
    //  Take over activated images...
    //  (We rely on consistancy check by PackBase)
    int k = 0;                          // counts active images
    for (int i=0; i < imgVec.size(); i++)
    {
      BrImage & img = imgVec.Image(i);
      if (img.active()) 
      { 
        images_[k++] = img;             // ref counting copy
      }
    }   
}



}  // namespace "br"

#endif // PackImage_hpp

// END OF FILE

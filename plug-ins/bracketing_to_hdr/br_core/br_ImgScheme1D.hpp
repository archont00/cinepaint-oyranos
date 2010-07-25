/*
 * br_ImgScheme1D.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file br_ImgScheme1D.hpp
  
  Typcasted 1D-views on `Image' objects
  
  Content:
   - ImgView1D<>
   - ImgScheme1D<>
  
  FRAGE: Wie macht man das ImgScheme1D eines Image-Objekts bzgl. des Image-
   Bildpuffers referenzzaehlend und damit speicherblockierend?  
  
  ANTWORT: Indem die Scheme-Klasse ein Array1D<uchar>-Element erhaelt, das im
   Scheme-Ctor mit dem uebergebenen Image-Objekt initialisiert wird (Image ist
   von Array1D<uchar> abgeleitet!). Der Copy-Ctor des Array1D erhoeht dann den
   Ref-Zaehler des Image-Bildpuffers. Ebenso wird bei jedem Kopieren von
   ImgScheme1D's (default Copy-Op!) das Array1D-Element als Element kopiert,
   dh. dessen ref-zaehlender Copy-Ctor springt an.
   
  FRAGE: Bei den nicht ref-zaehlenden View's Kopieren nicht besser verbieten?
*/
#ifndef br_ImgScheme1D_hpp
#define br_ImgScheme1D_hpp


#include "Scheme1D.hpp"         // Scheme1D<>
//#include "Scheme1D_utils.hpp"   // '<<' for Scheme1D<>
#include "br_Image.hpp"         // br::Image


namespace br {

/**============================================================================

  @class ImgView1D  -  template
  
  Typecasted 1D-view of an Image object using Scheme1D<> *without* data locking. 
  
  - Provides "[]"-syntax. 
  - No increasing of the img buffers ref-counter (no data locking!) 
  - No ref-counting copy-semantik concerning the image buffer.

*============================================================================*/
template <typename T>           
class ImgView1D : public Scheme1D <T>
{
public:
    ImgView1D (Image & img) 
      : Scheme1D<T> (img.n_pixel(), img.buffer())  {}
    
    ImgView1D (const Image & img) 
      : Scheme1D<T> (img.n_pixel(), img.buffer())  {}
      
    ImgView1D() {}  // Default Ctor: NULL scheme
    
    // sprechender Name fuer Bild-Kontexte
    int n_pixel() const         {return Scheme1D<T>::dim();}
};   


/**============================================================================

  @class ImgScheme1D  -  template
 
  Typecasted 1D-view of an Image object using Scheme1D<> *with* data locking. 
   
  - Provides "[]"-syntax. 
  - Increases the ref-counter of the image buffer (data locking!) 
  - Ref-counting copy-semantik for the image buffer.

*============================================================================*/  
template <typename T>           
class ImgScheme1D : public Scheme1D <T>
{
    TNT::Array1D<unsigned char>  my_refcopy_of_img_buffer;

public:
    ImgScheme1D (Image & img) 
      : Scheme1D<T> (img.n_pixel(), img.buffer()),
        my_refcopy_of_img_buffer (img)
      {}
    
    ImgScheme1D (const Image & img) 
      : Scheme1D<T> (img.n_pixel(), img.buffer()),
        my_refcopy_of_img_buffer (img)
      {}
      
    ImgScheme1D() {}    // Default Ctor: NULL scheme
    
    // sprechender Name fuer Bild-Kontexte
    int n_pixel() const         {return Scheme1D<T>::dim();}
};   

#if 0
template <typename T>
std::ostream& operator<<(std::ostream &s, const ImgScheme1D<T> &A)
{
    s << "Image-" << (Scheme1D<T>&)A; 
    return s;
}

template <typename T>
std::ostream& operator<<(std::ostream &s, const ImgView1D<T> &A)
{
    s << "Image-View-" << (Scheme1D<T>&) A; 
    return s;
}
#endif


}  // namespace "br"

#endif  // br_ImgScheme1D_hpp

// END OF FILE

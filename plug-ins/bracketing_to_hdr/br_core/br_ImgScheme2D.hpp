/*
 * br_ImgScheme2D.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file br_ImgScheme2D.hpp  
  
  Typcasting 2D views on br::Image objects.
  
  Content:
   - ImgArray2DView<>
   - ImgArray2D<>
   - ImgScheme2DView<>
   - ImgScheme2D<>
  
  FRAGE: Wie macht man das ImgScheme2D eines Image-Objekts bzgl. des Image-
   Bildpuffers referenzzaehlend und damit speicherblockierend?  
  
  ANTWORT: Indem die Scheme-Klasse ein Array1D<uchar>-Element erhaelt, das im
   Scheme-Ctor mit dem uebergebenen Image-Objekt initialisiert wird (Image ist
   von TNT::Array1D<uchar> abgeleitet!). Der Copy-Ctor des Array1D erhoeht dann
   den Ref-Zaehler des Image-Bildpuffers. Ebenso wird bei jedem Kopieren von
   ImgScheme2D's (default Copy-Op!) das Array1D-Element als Element kopiert, dh.
   dessen ref-zaehlender Copy-Ctor springt an.
  [Nur fuer ImgArray2DView<>:]
   Die auch moegliche Variante, das ImgScheme von Array1D<uchar> *abstammen* zu
   lassen und dafuer das Array2D<T> als Element zu halten, hat den Nachteil,
   dass die SubArray-Funktionalitaet fuer das Scheme dann erst nachgebaut werden
   muss.
   
  FRAGE: Bei den nicht ref-zaehlenden View's Kopieren nicht besser verbieten?
   Aber was schadet es? Es droht keine Gefahr eine Doppelfreigabe o. dgl.
*/
#ifndef br_ImgScheme2D_hpp
#define br_ImgScheme2D_hpp


#include "TNT/tnt_array2d.hpp"  // TNT::Array2D<>
#include "Scheme2D.hpp"         // Scheme2D<>
#include "Scheme2D_utils.hpp"   // <<-Op for Scheme2D<>
#include "br_Image.hpp"         // br::Image


namespace br {

/**==========================================================================\n

  @class ImgArray2DView  
  
  Template. Typecasted 2D-view of an br::Image object using the TNT::Array2D
   template (C-array Ctor). Provides \c "[][]" array syntax. 
 
   - <b>No</b> increasing of br::Image buffer's ref counter (no data locking!)
   - <b>No</b> ref counting copy semantic concerning the br::Image buffer.
   - If you need data locking and ref counting copying use ImgArray2D.
  
  @note TNT::Array2D<> allocates an auxillary array of dim1() T-pointers. 
  
  Unterschied zur Alternative  ImgScheme2DView:
   - Nachteil: Alloziiert und wartet zusaetzliches [dim1]-Feld von Zeigern.
   - Sonstiges: <tt>[][]</tt>-Zugriff hat eine int-Multipl. weniger, dafuer
      eine Dereferenzierung mehr; scheint gleichschnell.
   
*============================================================================*/
template <typename T>  
class ImgArray2DView : public TNT::Array2D<T>
{
public:
    ImgArray2DView (Image & img) 
      : TNT::Array2D<T> (img.dim1(), img.dim2(), (T*)img.buffer())
      { IF_FAIL_DO (sizeof(T) == img.byte_per_pixel(), RELAX); }
    
    ImgArray2DView (const Image & img) 
      : TNT::Array2D<T> (img.dim1(), img.dim2(), (T*)img.buffer())
      { IF_FAIL_DO (sizeof(T) == img.byte_per_pixel(), RELAX); }
};


/**==========================================================================\n

  @class ImgArray2D
 
  Template. Typecasted 2D-view of an br::Image object using the TNT::Array2D
   template (C-array Ctor). Provides \c "[][]" array syntax.
 
   - Increases the ref counter of the br::Image data buffer (data locking!)
   - Ref counting copy semantic for the br::Image buffer.
   - If you both won't: use ImgArray2DView.
  
  @note TNT::Array2D<> allocates an auxillary array of dim1() T-pointers. 
  
  Unterschied zur Alternative  ImgScheme2D:
   - Nachteil: Alloziiert und wartet zusaetzliches [dim1]-Feld von Zeigern.
   - Sonstiges: <tt>[][]</tt>-Zugriff hat eine int-Multipl. weniger, dafuer
      eine Dereferenzierung mehr; scheint gleichschnell.
   
*============================================================================*/
template <typename T>  
class ImgArray2D : public TNT::Array2D<T>
{
    TNT::Array1D<unsigned char>  my_imgbuffer_refcopy_;

public:
    ImgArray2D (Image & img) 
      : TNT::Array2D<T> (img.dim1(), img.dim2(), (T*)img.buffer()),
        my_imgbuffer_refcopy_(img)
      { IF_FAIL_DO (sizeof(T) == img.byte_per_pixel(), RELAX); }
    
    ImgArray2D (const Image & img) 
      : TNT::Array2D<T> (img.dim1(), img.dim2(), (T*)img.buffer()),
        my_imgbuffer_refcopy_(img)
      { IF_FAIL_DO (sizeof(T) == img.byte_per_pixel(), RELAX); }
};



/**==========================================================================\n

  @class ImgScheme2DView
    
  Template. Typecasted 2D-view of an br::Image object using the Scheme2D template.
   Provides \c "[][]" array syntax. 

   - <b>No</b> increasing of br::Image buffer's ref counter (no data locking!)
   - <b>No</b> ref counting copy semantic concerning the br::Image buffer.
   - If you need data locking and copy semantic use ImgScheme2D.
  
  Unterschied zur Alternative  ImgArray2DView:
   - Vorteil: alloziiert und wartet kein zusaetzliches [dim1]-Feld von Zeigern
   - Sonstiges: <tt>[][]</tt>-Zugriff hat eine int-Multpl. mehr, dafuer eine
      Dereferenzierung weniger, scheint gleichschnell.

  @internal Besserer Name wohl "ImgView2D".    
  
*============================================================================*/
template <typename T>           
class ImgScheme2DView : public Scheme2D <T>
{
public:
    ImgScheme2DView (Image & img) 
      : Scheme2D<T> (img.dim1(), img.dim2(), img.buffer())
      { IF_FAIL_DO (sizeof(T) == img.byte_per_pixel(), RELAX); }
    
    ImgScheme2DView (const Image & img) 
      : Scheme2D<T> (img.dim1(), img.dim2(), img.buffer())
      { IF_FAIL_DO (sizeof(T) == img.byte_per_pixel(), RELAX); }
      
    ImgScheme2DView() {}  // Default Ctor: NULL view. Here useless.
};   


/**==========================================================================\n

  @class ImgScheme2D
 
  Template. Typecasted 2D-view of an br::Image object using the Scheme2D template.
    Provides \c "[][]" array syntax. 

   - Increases the ref counter of the br::Image buffer (data locking!)
   - Ref counting copy semantic for the br::image buffer.
   - If you both won't: use ImgScheme2DView.
   
  Unterschied zur Alternative  ImgArray2D:
   - Vorteil: alloziiert und wartet kein zusaetzliches [dim1]-Feld von Zeigern
   - Sonstiges: <tt>[][]</tt>-Zugriff hat eine int-Multpl. mehr, dafuer eine
      Dereferenzierung weniger, scheint gleichschnell.
   
*============================================================================*/  
template <typename T>           
class ImgScheme2D : public Scheme2D <T>
{
    TNT::Array1D<unsigned char>  my_imgbuffer_refcopy_;

public:
    ImgScheme2D (Image & img) 
      : Scheme2D<T> (img.dim1(), img.dim2(), img.buffer()),
        my_imgbuffer_refcopy_(img)
      { IF_FAIL_DO (sizeof(T) == img.byte_per_pixel(), RELAX); }
    
    ImgScheme2D (const Image & img) 
      : Scheme2D<T> (img.dim1(), img.dim2(), img.buffer()),
        my_imgbuffer_refcopy_(img)
      { IF_FAIL_DO (sizeof(T) == img.byte_per_pixel(), RELAX); }
    
    ImgScheme2D() {}    // Default Ctor: NULL scheme. Used as return value.
    
    ImgScheme2D<T> subscheme (int i0, int i1, int j0, int j1);
    int ref_count() const       {return my_imgbuffer_refcopy_.ref_count();}
    
private:
    /*  Private Ctor for usage in subscheme() */
    ImgScheme2D (int dim1, int dim2, int stride, TNT::Array1D<unsigned char> bufref)
      : Scheme2D<T> (dim1, dim2, bufref.data(), stride),
        my_imgbuffer_refcopy_(bufref)
      {}
};   


/**+*************************************************************************\n
  Create a new view to a subarray defined by the boundaries [i0][i0] and
   [i1][j1].  The size of the subarray is (i1-i0) by (j1-j0).  If either
   of these lengths are zero or negative, the subarray view is null.
   
  @internal Funktion verdeckt Basisfunktion gleichen Names, die ein Scheme2D
   zuruecklieferte. Durch Instanziieren eines ref-zaehlenden ImgScheme2D hat das
   Return-Objekt den Ref-Zaehler des Bildpuffers heraufgesetzt, wie es sein muss.
******************************************************************************/
template <typename T>
ImgScheme2D<T> ImgScheme2D<T>::subscheme (int i0, int i1, int j0, int j1)
{
    int d1 = i1 - i0 + 1;  // new dim1
    int d2 = j1 - j0 + 1;  // new dim2
    
    /*  If either length is zero or negative, this is an invalide
       subscheme. Return a NULL scheme. */
    if (d1<1 || d2<1)
      return ImgScheme2D<T>();
    
    /*  The corresponding boundaries of the *uchar* buffer */
    int b0 = (i0 * Scheme2D<T>::stride() + j0) * sizeof(T);
    int b1 = (i1 * Scheme2D<T>::stride() + j1 + 1) * sizeof(T) - 1;
    
    /*  Construct an ImgScheme2D with a subarray of the uchar imgbuffer */
    return ImgScheme2D<T> (d1, d2, Scheme2D<T>::stride(), my_imgbuffer_refcopy_.subarray(b0,b1));
}


template <typename T>
std::ostream& operator<<(std::ostream &s, const ImgScheme2D<T> &A)
{
    s << "Image-" << static_cast< Scheme2D<T> >( A );
    return s;
}

template <typename T>
std::ostream& operator<<(std::ostream &s, const ImgScheme2DView<T> &A)
{
    s << "Image-View-" << static_cast< Scheme2D<T> >( A );
    return s;
}



#if 0
//=============================================================================
// GEDANKE: ImgScheme2D von Image ableiten. Konstruieren dann mit *default* 
//  Copy-Ctor von Image (kopiert nur Zeiger). Vorteil: Alle imgData-Funktionen
//  sogleich verfuegbar. Nachteil: Kopiert auch unnoetige Elemente wie `DataType'.
// BUG: Da Dtor von Image freigibt, koennen keine automatischen ImgScheme2D-
//  Objekte erzeugt werden von Image-Objekten, die eingereiht sind. LOesung(en):
//  Image-Dtor virtual und Scheme2D ueberschreibt diesen.
//=============================================================================
template <typename T>           
class ImgScheme2D : public Image
{
public:
    ImgScheme2D (Image       & img) : Image (img)  {}
    ImgScheme2D (const Image & img) : Image (img)  {}  
    
    const T* operator[] (int i) const {return & ((T*)buffer())[ i * dim2() ];}
    T*       operator[] (int i)       {return & ((T*)buffer())[ i * dim2() ];}
};     
#endif


}  // namespace "br"

#endif  // br_ImgScheme2D_hpp

// END OF FILE

/*
 * br_PackImgScheme2D.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file  br_PackImgScheme2D.hpp
  
  Speicherschema: separate Bilder.
  
  Content:
   - class `PackImgScheme2DView2D<>'  --  no ref-counting
   - class `PackImgScheme2D<>'        --  ref-counting
  
  Speicher hier nur uebernommen, was hier leicht, da Eingangsobjekt `BrImgVector' 
   schon richtige Struktur hat (separate Bilder).
   
  Typ-Pruefungen:
   PackBase prueft zwar auf GLEICHartigkeit der Bilder (Dimension, Kanalanzahl,
   Datentyp, Speicherschema), kann aber nicht auf ABSOLUTE Typen (U8 oder U16
   etc.) pruefen, die dort nicht bekannt sind. Es koennte also unvermerkt ein
   U8-Pack aus U16-Bilder kreiert werden, was misslich. Eine Moeglichkeit einer
   fruehen Pruefung boete ein Funktionszeiger, der dem PackBase-Ctor mitgegeben
   wird (default=0), und den spaetere Instanzierungen geeignet setzen koennten.
   (Muesste bis dorthin durch alle Erben durchgeschleift werden.)
   
   Eine andere hier realisierte: Via Mehrfachvererbung als erstes eine
   Typpruefklasse zu vererben, die nichts als den Typ prueft und gegebenenfalls
   wirft.
*/
#ifndef br_PackImgScheme2D_hpp
#define br_PackImgScheme2D_hpp


#include <cassert>
#include "br_Image.hpp"         // BrImgVector (Ctor argument), Image
#include "br_PackBase.hpp"      // PackBase
#include "br_ImgScheme2D.hpp"   // ImgScheme2D<>



#ifdef VERBOSE
#  define VERBOSE_PRINTF(args)    printf(args);
#else
#  define VERBOSE_PRINTF(args)
#endif


namespace br {

/**===========================================================================

  @class  PackImgScheme2DView
 
  Template. Image buffers maintained by a *built-in* array of ImgScheme2DView<>s.

   - No ref-counting for the image buffers, i.e. no memory locking
   - Copying of instances prevented (privatisation of Copy-Ctor and =-Op).

  @internal Besserer Name wohl "PackImgView2D".
  
*============================================================================*/
template <typename T>
class PackImgScheme2DView : public PackBase
{
  private:
    ImgScheme2DView< T >* schemes_;

  public:
    //  Ctor & Dtor...
    PackImgScheme2DView (BrImgVector &);
    ~PackImgScheme2DView()                      {delete[] schemes_;}
   
    //  Scheme2D of the i-th image...
    const ImgScheme2DView<T>& operator[] (int i) const {return schemes_[i];}
    ImgScheme2DView<T>&       operator[] (int i)       {return schemes_[i];}
  
  private:
    //  Prevent copying...  
    PackImgScheme2DView (const PackImgScheme2DView &);
    PackImgScheme2DView & operator= (const PackImgScheme2DView &);
};

/**+*************************************************************************\n
  Ctor
******************************************************************************/
template <typename T>
PackImgScheme2DView<T>::PackImgScheme2DView (BrImgVector & imgVec) 
  : 
    PackBase (imgVec)           // --> size() & consistency check
{
    schemes_ = new ImgScheme2DView<T>[ size() ]; // array of NULL schemes
     
    /*  Take over activated images... (relies on PackBase settings) */
    const Rectangle & section = imgVec.intersection();
    int iact = 0;               // counts active images
    for (int i=0; i < imgVec.size(); i++)
    {
      BrImage & img = imgVec.image(i);
      if (img.active()) 
      { 
        Vec2_int d = img.active_offset();
        int i0 = section.y0()    + d.y;     // dim1 == height
        int i1 = section.ylast() + d.y;
        int k0 = section.x0()    + d.x;     // dim2 == width
        int k1 = section.xlast() + d.x;
        schemes_[iact ++] = ImgScheme2DView<T>( img ).subscheme (i0,i1, k0,k1);
        
        /*  Rough test whether active_offset()s are consistent */
        assert (i0 >= 0  && i0 < img.dim1());
        assert (i1 >= i0 && i1 < img.dim1());
        assert (k0 >= 0  && k0 < img.dim2());
        assert (k1 >= k0 && k1 < img.dim2());
      }
    }   
}


/**===========================================================================

  @class  PackImgScheme2D
 
  Template. Image buffers maintained by a *std::vector* of ImgScheme2D<>s.

   - Ref-counting for the image buffers, i.e. memory locking
   - Copying of instances allowed

*============================================================================*/
template <typename T>
class PackImgScheme2D : public PackBase
{
  private:
    std::vector< ImgScheme2D<T> >  schemes_;

  public:
    //  Ctor & Dtor...
    PackImgScheme2D (BrImgVector &);
    ~PackImgScheme2D()                             {}
   
    //  Scheme2D of the i-th image...
    const ImgScheme2D<T>& operator[] (int i) const {return schemes_[i];}
    ImgScheme2D<T>&       operator[] (int i)       {return schemes_[i];}
};


/**+*************************************************************************\n
  Ctor...
   
  Ctor der PackBase Basisklasse hat size() gesetzt sowie dim1(), dim2() auf 
   die \a imgVec.intersection() Werte. Jetzt werden aus den \a intersection
   Indizes und den individuellen active_offset()-Werten jedes aktiven Bildes
   die individuellen Subschemes gefertigt und im Vektor \a schemes_ abgelegt.
  
  @note Es werden hier also korrekte \a intersection- und active_offset()-Werte
   vorausgesetzt!
******************************************************************************/
template <typename T>
PackImgScheme2D<T>::PackImgScheme2D (BrImgVector & imgVec) 
  : 
    PackBase (imgVec),          // --> size() & consistency check
    schemes_ (size())           // allocates vector of `size' NULL schemes
{
    /*  Take over the activated images... (relies on some PackBase settings) */
    /*  The intersection area of all activated images */
    const Rectangle & section = imgVec.intersection();
    
    VERBOSE_PRINTF(("Intersection: dim1=%d  dim2=%d  [x0=%d  x1=%d] x [y0=%d  y1=%d]\n",
        section.dim1(), section.dim2(), section.x0(), section.xlast(), 
        section.y0(), section.ylast())); 
    
    int iact = 0;               // counts the active images
    for (int i=0; i < imgVec.size(); i++)
    {
      BrImage & img = imgVec.image(i);
      if (img.active()) 
      { 
        Vec2_int d = img.active_offset();
        int i0 = section.y0()    + d.y;     // dim1 == height
        int i1 = section.ylast() + d.y;
        int k0 = section.x0()    + d.x;     // dim2 == width
        int k1 = section.xlast() + d.x;
        schemes_[iact ++] = ImgScheme2D<T>( img ).subscheme (i0,i1, k0,k1);
#      if VERBOSE        
        const ImgScheme2D<T> & s = schemes_[iact-1];
        printf ("Image:   dim1=%d  dim2=%d\n", img.dim1(), img.dim2());
        printf ("Scheme:  dim1=%d  dim2=%d  stride=%d\n", s.dim1(), s.dim2(), s.stride());
#      endif
        /*  Rough test whether active_offset()s are consistent */
        assert (i0 >= 0  && i0 < img.dim1());
        assert (i1 >= i0 && i1 < img.dim1());
        assert (k0 >= 0  && k0 < img.dim2());
        assert (k1 >= k0 && k1 < img.dim2());
      }
    }   
}


template <typename T>
std::ostream& operator<< (std::ostream &s, const PackImgScheme2DView<T> &pack)
{
    s << "PackImgScheme2D<" << typeid(T).name() << "> [images: " << pack.size()
      << "] [ " << pack.dim1() << " x " << pack.dim2() << " ]:\n";
    
    for (int i=0; i < pack.size(); i++)
    {
      s << "[" << i << "]: " << pack[i];
    }
    return s;
}

template <typename T>
std::ostream& operator<< (std::ostream &s, const PackImgScheme2D<T> &pack)
{
    s << "PackImgScheme2D<" << typeid(T).name() << "> [images: " << pack.size()
      << "] [ " << pack.dim1() << " x " << pack.dim2() << " ]:\n";
    
    for (int i=0; i < pack.size(); i++)
    {
      s << "[" << i << "]: " << pack[i];
    }
    return s;
}


}  // namespace "br"


/**
  Typedefs & Spezialisierungen...

  Beachte: Die unten *abgeleitete* Variante Pack_ImgScheme2D_RGB_U8
   ist dann nicht mehr identisch mit dem einfachen "typedef"
   PackImgScheme2D_RGB_U8_simple! In Templates zu beachten!
*/ 

#include "br_types.hpp"         // uint8, uint16
#include "Rgb.hpp"              // Rgb<>

namespace br {

// Einfachste Variante:
typedef PackImgScheme2D< Rgb<uint8> >  PackImgScheme2D_RGB_U8_simple;   

//================
// Mit Typ-Check:
//================
// Aufgabe: Check (mit eventl. Wurf) *vor* Vollzug des PackBase-Ctors.
// Moeglichkeiten: 
//  o Mehrfachvererbung, wobei als erstes eine Check-Klasse vererbt wird, die
//     nur prueft und evntl. wirft
//  o PackImgScheme2D<> Ctor hat einen Funktionszeiger, der in den Speziali-
//     sierungen passend zu richten ist auf check_ImgType_RGB_U8() etc.
//  o Eine virtuelle Funktion geht natuerlich nicht, da die im :-Bereich noch 
//     nicht initialisiert.

// Test muesste eigentlich am ersten *aktivierten* Bild durchgefuehrt werden und
//  dann auch size_active()>0 pruefen. Ausser, ImgVector stellt sicher, dass 
//  nur gleichartige Bilder aufgenommen werden. Dann ist auch ein nichtaktives
//  geeignet.

#if 0
//  Mehrfachvererbung: Test noch vor PackBase-Ctor...
struct PackTypeCheck_RGB_U8
{
    PackTypeCheck_RGB_U8 (BrImgVector & imgVec) 
      {CHECK_IMG_TYPE_RGB_U8 (imgVec.image(0));
       throw 1;   // demo
      }
};

struct PackImgScheme2D_RGB_U8 : public PackTypeCheck_RGB_U8, 
                                public PackImgScheme2D< Rgb<uint8> > 
{
    PackImgScheme2D_RGB_U8 (BrImgVector & imgVec) 
      : PackTypeCheck_RGB_U8 (imgVec),
        PackImgScheme2D< Rgb<uint8> > (imgVec)  
      {}
};

#else
//  Keine Mehrfachvererbung: Test erst am Schluss...
struct PackImgScheme2D_RGB_U8 : public PackImgScheme2D< Rgb<uint8> > 
{
    PackImgScheme2D_RGB_U8 (BrImgVector & imgVec) 
      : PackImgScheme2D< Rgb<uint8> > (imgVec)
      {
        CHECK_IMG_TYPE_RGB_U8 (imgVec.image(0));
      }
};
#endif


struct PackImgScheme2D_RGB_U16 : public PackImgScheme2D< Rgb<uint16> > 
{
    PackImgScheme2D_RGB_U16 (BrImgVector & imgVec) 
      : PackImgScheme2D< Rgb<uint16> > (imgVec)
      {
        CHECK_IMG_TYPE_RGB_U16 (imgVec.image(0));
      }
};


}  // namespace "br"

#endif // PackImgScheme2D_hpp

// END OF FILE

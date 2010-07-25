/*
 * br_PackImage_utils.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  br_PackImage_utils.hpp
*/
#ifndef br_PackImage_utils_hpp
#define br_PackImage_utils_hpp


#include "br_PackImage.hpp"
#include "br_ImgScheme2D.hpp"


namespace br {


//  Sind lokale (built-in) Vektoren von Scheme2D's oder Scheme2DView's 
//   schneller als Vektoren auf dem Heap? Zum Testen diese Makros: Generieren
//   den entsprechenden Quellcode.

#define LOCAL_SCHEME2D_VEC_OF_PackImage( vec, type, srcpack ) \
    ImgScheme2D< type >  vec [ srcpack.size() ];\
    for (int i=0; i < srcpack.size(); i++)\
       vec[i] = ImgScheme2D< type >( srcpack[i] );

#define LOCAL_VIEW2D_VEC_OF_PackImage( vec, type, srcpack ) \
    ImgScheme2DView< type >  vec [ srcpack.size() ];\
    for (int i=0; i < srcpack.size(); i++)\
       vec[i] = ImgScheme2DView< type >( srcpack[i] );

       

/**==================================================================
//
//  View2D_PackImage<>  &  ImgScheme2D_PackImage<>  
//
//  So ImgScheme2D<> ein Image typisiert, so dienen diese Templates der
//   Typisierung eines PackImage. Liefern Index-Zugriff `P[img][y][x]'.
//
//===================================================================
//
//  View2D_PackImage<>  -  template class
//
//  Vektor von ImgScheme2DView's, die nicht ref-zaehlend kopieren und Bild-
//   puffer nicht blockieren. Kopieren waere gefaehrlich. Daher auch nur 
//   minimale Impl. eines Vektors ohne braucbares Kopierverhalten.
//
//  VORSICHT: Allokiert im Ctor, zerstoert im Dtor, aber default Copy-Ctor!
//   Solche Objekte duerfen also nicht kopiert werden! Daher bzgl. Funktionen
//   versteckt. Zum Dank autom. Selbstzerstoerung an jedem Blockende.
//
//  FRAGE: Warum kein vector von ImgScheme2D<>s, sondern dynamisch allokiert?
//   Weil std::vector auch nur dynamisch allokiert! So Minimalcode (unter 
//   Verzicht auf Kopieren). Noch minimaler mit malloc()? Nein, Objekt dann
//   nicht richtig initiallisiert.
//
//===================================================================*/
template <typename T>
class View2D_PackImage
{
    int size_, dim1_, dim2_;
    ImgScheme2DView<T>* schemes_;

public:
    
    View2D_PackImage (PackImage & pack)
      : size_(pack.size()),
        dim1_(pack.dim1()),
        dim2_(pack.dim2())
      {
        schemes_ = new ImgScheme2DView<T>[ size_ ];
        
        for (int i=0; i < size_; i++)
           schemes_[i] = ImgScheme2DView<T>( pack[i] );
      }  
    View2D_PackImage (const PackImage & pack)
      : size_(pack.size()),
        dim1_(pack.dim1()),
        dim2_(pack.dim2())
      {
        schemes_ = new ImgScheme2DView<T>[ size_ ];
        
        for (int i=0; i < size_; i++)
           schemes_[i] = ImgScheme2DView<T>( pack[i] );
      }  
    ~View2D_PackImage()            {delete[] schemes_;}
    
    int size() const                    {return size_;}
    int dim1() const                    {return dim1_;}
    int dim2() const                    {return dim2_;}
    
    const ImgScheme2DView<T>& operator[] (int i) const {return schemes_[i];}
    ImgScheme2DView<T>&       operator[] (int i)       {return schemes_[i];}
    
private:
    // Prevent copying...  
    View2D_PackImage (const View2D_PackImage &);
    View2D_PackImage & operator= (const View2D_PackImage &);
};    


/**==================================================================
//
//  ImgScheme2D_PackImage<>  -  template class
//
//  Da hiesige Vektor-Elemente referenzzaehlend kopieren, waehlen wir auch
//   eine anstaendig kopierende Vektorklasse (std::vector). Damit ganzes
//   ImgScheme2D_PackImage kopierfaehig, Kopier-Op nicht mehr versteckt.
//
//===================================================================*/
template <typename T>
class ImgScheme2D_PackImage
{
    int  dim1_, dim2_;
    std::vector< ImgScheme2D<T> >  schemes_;

public:
    
    ImgScheme2D_PackImage (PackImage & pack)
      : schemes_(pack.size()),
        dim1_(pack.dim1()),
        dim2_(pack.dim2())
      {
        for (int i=0; i < pack.size(); i++)
           schemes_[i] = ImgScheme2D<T>( pack[i] );
      }  
    ImgScheme2D_PackImage (const PackImage & pack)
      : schemes_(pack.size()),
        dim1_(pack.dim1()),
        dim2_(pack.dim2())
      {
        for (int i=0; i < pack.size(); i++)
           schemes_[i] = ImgScheme2D<T>( pack[i] );
      }  
    ~ImgScheme2D_PackImage()       {}
    
    int size() const                    {return schemes_.size();}
    int dim1() const                    {return dim1_;}
    int dim2() const                    {return dim2_;}
    
    const ImgScheme2D<T>& operator[] (int i) const {return schemes_[i];}
    ImgScheme2D<T>&       operator[] (int i)       {return schemes_[i];}
};    


   
template <typename T>
std::ostream& operator<<(std::ostream &s, const View2D_PackImage<T> &pack)
{
    s << "View2D_PackImage<" << typeid(T).name() << "> [images: " 
      << pack.size() 
      << "] [ " << pack.dim1() << " x " << pack.dim2() << " ]:\n";
    
    for (int i=0; i < pack.size(); i++)
    {
      s << "[" << i << "]: " << pack[i];
    }
    return s;
}

template <typename T>
std::ostream& 
operator<<(std::ostream &s, const ImgScheme2D_PackImage<T> &pack)
{
    s << "View2D_PackImage<" << typeid(T).name() << "> [images: " 
      << pack.size()
      << "] [ " << pack.dim1() << " x " << pack.dim2() << " ]:\n";
    
    for (int i=0; i < pack.size(); i++)
    {
      s << "[" << i << "]: " << pack[i];
    }
    return s;
}



}  // namespace "br"

#endif  // br_PackImage_utils_hpp

// END OF FILE

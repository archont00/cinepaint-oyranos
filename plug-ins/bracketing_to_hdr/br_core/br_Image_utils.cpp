/*
 * br_Image_utils.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  br_Image_utils.hpp  --  auch viel fuer Tests, nicht alles wird hier bleiben.
*/

#include <iostream>

#include "br_Image_utils.hpp"           // declarations
#include "br_enums_strings.hpp"         // DATA_TYPE_SHORT_STR[] ...
#include "br_Image.hpp"
#include "br_ImgScheme2D.hpp"
#include "Rgb.hpp"                      // Rgb<>
#include "Rgb_utils.hpp"                // <<-Op for Rgb<>

// Fuer die Testroutinen zum Fuellen der Puffer:
#include <cmath>                        // pow()
#include "TNT/tnt_stopwatch_.hpp"
#include "input_statistics.hpp"


using std::cout;

namespace br {

void listBuffer_U8 (const Image &img)
{
    CHECK_IMG_TYPE_U8 (img);
    ImgScheme2DView<uint8>  A(img);
    cout << A;
}
void listBuffer_RGB_U8 (const Image &img)
{
    CHECK_IMG_TYPE_RGB_U8 (img);
    ImgScheme2DView< Rgb<uint8> >  A(img);
    cout << A;
}

void listBuffer_U16 (const Image &img)
{
    CHECK_IMG_TYPE_U16 (img);
    ImgScheme2DView<uint16>  A(img);
    cout << A;
}
void listBuffer_RGB_U16 (const Image &img)
{
    CHECK_IMG_TYPE_RGB_U16 (img);
    ImgScheme2DView< Rgb<uint16> >  A(img);
    cout << A;
}

void listBuffer_F32 (const Image &img)
{
    CHECK_IMG_TYPE_F32 (img);
    ImgScheme2DView<float>  A(img);
    cout << A;
}
void listBuffer_RGB_F32 (const Image &img)
{
    CHECK_IMG_TYPE_RGB_F32 (img);
    ImgScheme2DView< Rgb<float> >  A(img);
    cout << A;
}

#if 0
//====================================
// Versuch einer allgemeinen Loesung.
//  NACHTEIL: Erzwingt listBuffer_() und Scheme2DView<>-Instanzierungen fuer
//   alle aufgefuehrten Typen, auch wenn dann gar nicht benutzt. Naja, viel ist
//   das freilich nicht.
//  Note: Typ steckt nicht in data_type() allein, sondern auch in channels()
//   und storing()! 

//  Hilfsfunktion...
static void error_msg_no_impl (std::ostream &s, const Image &img)
{
    s << "Sorry, no `Image' ostream implementation for that data structure:"
      << "\n    channels: " << img.channels()
      << ",  data type: " << DATA_TYPE_SHORT_STR[img.data_type()]
      << ",  storing scheme: " << STORING_SCHEME_STR[img.storing()] 
      << '\n';   
}

std::ostream& operator<< (std::ostream &s, Image &img)
{
    switch (img.channels())
    {
    case 1:
        switch (img.data_type())
        {
        case DATA_U8:  listBuffer_U8 (img); break;
        case DATA_U16: listBuffer_U16 (img); break;
        case DATA_F32: listBuffer_F32 (img); break;
        default:       error_msg_no_impl (s,img); break;
        } 
        break; 
      
    case 3:
        switch (img.data_type())
        {
        case DATA_U8:
            switch (img.storing())
            {
            case STORING_INTERLEAVE: listBuffer_RGB_U8 (img); break;
            default:                 error_msg_no_impl (s,img); break;
            }
            break;
          
        case DATA_U16:
            switch (img.storing())
            {
            case STORING_INTERLEAVE: listBuffer_RGB_U16 (img); break;
            default:                 error_msg_no_impl (s,img); break;
            }
            break;

        case DATA_F32:
            switch (img.storing())
            {
            case STORING_INTERLEAVE: listBuffer_RGB_F32 (img); break;
            default:                 error_msg_no_impl (s,img); break;
            }
            break;
          
        default:
            error_msg_no_impl (s,img); break;
        }
        break;
      
    default:
        error_msg_no_impl (s,img); break;
    }    
    return s;
}
#endif 

void list_ImgVec_Buffers_RGB_U8 (const BrImgVector &imgVec)
{ 
    printf ("=== BrImgVector: buffers === [%d]\n", imgVec.size());  
    for (int i=0; i < imgVec.size(); i++)
    {
      printf("[%2d]: ", i);  
      listBuffer_RGB_U8 (imgVec.image(i));
    }
}

void list_ImgVec_Buffers_RGB_U16 (const BrImgVector &imgVec)
{ 
    printf ("=== BrImgVector: buffers === [%d]\n", imgVec.size());  
    for (int i=0; i < imgVec.size(); i++)
    {
      printf("[%2d]: ", i);  
      listBuffer_RGB_U16 (imgVec.image(i));
    }
}


//============================================================
//  Zu Testzwecken Funktionen zum Fuellen von Image-Objekten...
//============================================================

void fillBuffer_M_U8 (Image & img)     // "M" fuer manuell
{
    CHECK_IMG_TYPE_U8 (img);
    uint8* buf = img.buffer();
    int cols = img.dim2();
  
    for (int i=0; i < img.dim1(); i++)
    for (int j=0; j < img.dim2(); j++)   
       buf [i * cols + j] = i + j;
}

void fillBuffer_A_U8 (Image & img, uint8 add)  // "A" fuer Array2D
{
    CHECK_IMG_TYPE_U8 (img);
    ImgArray2DView<uint8>  A(img);
  
    for (int i=0; i < A.dim1(); i++)
    for (int j=0; j < A.dim2(); j++)  
       A[i][j] = i + j + add;
}

//  Die schliessliche Loesung: mit ImgScheme2DView<>...
void fillBuffer_U8 (Image & img, uint8 add)
{
    CHECK_IMG_TYPE_U8 (img);
    ImgScheme2DView<uint8>  A(img);
  
    for (int i=0; i < A.dim1(); i++)
    for (int j=0; j < A.dim2(); j++)  
       A[i][j] = i + j + add;
}

void fillBuffer_RGB_U8 (Image & img, uint8 add)
{
    CHECK_IMG_TYPE_RGB_U8 (img);
    ImgScheme2DView< Rgb<uint8> >  A(img);
  
    for (int i=0; i < A.dim1(); i++)
    for (int j=0; j < A.dim2(); j++)  
       A[i][j] = Rgb<uint8> (i+j+add, i+j+add + 1, i+j+add + 2);
       //A[i][j] = Rgb<uint8> ((uint8)(10*(i+1)+j+1)); // Index-Daten
}

void fillBuffer_RGB_U16 (Image & img, uint16 add)
{
    CHECK_IMG_TYPE_RGB_U16 (img);
    ImgScheme2DView< Rgb<uint16> >  A(img);
  
    for (int i=0; i < A.dim1(); i++)
    for (int j=0; j < A.dim2(); j++)  
//       A[i][j] = Rgb<uint16> (i+j+add, i+j+add + 1, i+j+add + 2);
       A[i][j] = Rgb<uint16> ((i+j+add)<<8, (i+j+add + 1)<<8, (i+j+add + 2)<<8);
}


/*****************************************************************************
  Erzeuge RGB_U8-Bilder eines BrImgVectors und fuelle diese...
*****************************************************************************/
void create_ImgVec_RGB_U8 (BrImgVector & imgVec, int xdim, int ydim, int N, 
                           bool stats)
{
    char s[80]; 
  
    cout <<__func__<<"(): "<<N<<" x ["<<xdim<<" x "<<ydim<<"]...\n";
    TNT::StopWatch uhr;  uhr.start();  
  
    for (int i=0; i < N; i++)
    {
      sprintf (s, "img#%d - Rgb u8", i);

      BrImage img (xdim, ydim, IMAGE_RGB_U8, s, pow(2., i));
      fillBuffer_RGB_U8 (img, i);
      if (stats) compute_input_stats_RGB_U8 (img);
    
      imgVec.add (img);
    }
    cout << '\t'; uhr.result();
}

/*****************************************************************************
  Blosse Erzeugung der Bilder eines BrImgVectors ohne Fuellung...
*****************************************************************************/
void pure_create_ImgVec_RGB_U8 (BrImgVector &imgVec, int xdim, int ydim, int N)
{
    char s[80]; 
  
    for (int i=0; i < N; i++)
    {
      sprintf (s, "img#%d - Rgb u8", i);
      imgVec.add (BrImage(xdim, ydim, IMAGE_RGB_U8, s, pow(2.,i)).getRef());
    }
}

/*****************************************************************************
  Fuelle Bilder eines BrImgVector... (richtige Sortierung hier garantiert)
*****************************************************************************/
void fill_ImgVec_RGB_U8 (BrImgVector & imgVec)
{
    cout <<__func__<<"()...\n";
    for (int i=0; i < imgVec.size(); i++)
       fillBuffer_RGB_U8 (imgVec.image(i), i);
}

/*****************************************************************************
  Berechne InputStatistiken eines BrImgVectors (aber keine Umsortierung)
*****************************************************************************/
void compute_input_stats_RGB_U8 (BrImgVector & imgVec)
{
    cout <<__func__<<"()...\n";
    for (int i=0; i < imgVec.size(); i++)
       compute_input_stats_RGB_U8 (imgVec.image(i));
}


/*****************************************************************************
  Erzeuge RGB_U16-Bilder eines BrImgVectors und fuelle diese...
*****************************************************************************/
void create_ImgVec_RGB_U16 (BrImgVector & imgVec, int xdim, int ydim, int N, 
                            bool stats)
{
    char s[80]; 
    
    cout <<__func__<<"(): "<<N<<" x ["<<xdim<<" x "<<ydim<<"]...\n";
    TNT::StopWatch uhr;  uhr.start();  
  
    for (int i=0; i < N; i++)
    {
      sprintf (s, "img#%d - Rgb u16", i);
      BrImage img (xdim, ydim, IMAGE_RGB_U16, s, pow(2., i));
      fillBuffer_RGB_U16 (img, i);
      if (stats) compute_input_stats_RGB_U16 (img);
   
      imgVec.add (img);
    }
    cout << '\t'; uhr.result();
}


}  // namespace "br"


// END OF FILE

/*
 * br_Image_utils.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file  br_Image_utils.hpp
  
  Zur Namenswahl: Moeglich waere ein Name und alles an den Argumenttypen
   festzumachen:
     - list_RGB_U8 (const Image &);
     - list_RGB_U8 (const BrImgVector &);
     - list_RGB_U8 (const Pack &); 
   Ist das praktisch? 
   Wahlweise auch listBuffer_U8(...) oder list_buffer_U8(...)
   
  @todo Die Funktionen zum Fuellen von Bildern mit Testwerten gehoeren hier
   eigentlich nicht rein.
*/
#ifndef br_Image_utils_hpp
#define br_Image_utils_hpp


#include <ostream>
#include "br_types.hpp"       // uint8, uint16
#include "br_Image.hpp"       // Image, BrImgVector


namespace br {

void listBuffer_U8      (const Image &);
void listBuffer_RGB_U8  (const Image &);
void listBuffer_U16     (const Image &);
void listBuffer_RGB_U16 (const Image &);
void listBuffer_F32     (const Image &);
void listBuffer_RGB_F32 (const Image &);

//  Versuch eines allgemeinen Loesung:
//std::ostream& operator<< (std::ostream &s, Image &);

void list_ImgVec_Buffers_RGB_U8  (const BrImgVector &);
void list_ImgVec_Buffers_RGB_U16 (const BrImgVector &);


//============================================================
//  Zu Testzwecken Funktionen zum Fuellen von Image-Objekten...
//============================================================

void fillBuffer_M_U8 (Image &);                 // "M" fuer manuell
void fillBuffer_A_U8 (Image &, uint8 add=0);    // "A" fuer Array2D
void fillBuffer_U8 (Image &, uint8 add=0);      // finale Loesung mit Img2DView

void fillBuffer_RGB_U8  (Image &, uint8 add=0);
void fillBuffer_RGB_U16 (Image &, uint16 add=0);

//  Erzeuge Bilder eines BrImgVectors und fuelle diese
void create_ImgVec_RGB_U8  (BrImgVector &, int xdim, int ydim, int N, bool stats=false);
void create_ImgVec_RGB_U16 (BrImgVector &, int xdim, int ydim, int N, bool stats=false);

//  Blosse Erzeugung der Bilder eines BrImgVectors ohne Fuellung
void pure_create_ImgVec_RGB_U8 (BrImgVector &, int xdim, int ydim, int N);

//  Fuelle Bilder eines BrImgVectors (richtige Sortierung hier garantiert)
void fill_ImgVec_RGB_U8 (BrImgVector &);

//  Berechne InputStatistiken eines BrImgVectors (aber keine Umsortierung)
void compute_input_stats_RGB_U8 (BrImgVector &);


} // namespace "br"

#endif // br_Image_utils_hpp

// END OF FILE

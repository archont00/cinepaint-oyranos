/*
 * mergeHdr_PackSch2D_RGB_U16.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  mergeHdr_PackSch2D_RGB_U16.cpp
  
  Contents: 
   - merge_Hdr_RGB_U16()    :  merging of an HDR-image
   - merge_LogHdr_RGB_U16() :  merging of an logarithmic HDR-image
*/

#include <cassert>                      // assert()
#include <iostream>                     // cout
#include <limits>                       // numeric_limits<>
#include <cmath>                        // exp()

#include "TNT/tnt_array1d.hpp"          // TNT::Array1D<>
#include "TNT/tnt_stopwatch.hpp"        // TNT::Stopwatch

#include "i18n.hpp"                     // macro _()
#include "Rgb.hpp"                      // Rgb<>
#include "Rgb_utils.hpp"                // <<-Op for Rgb<>
#include "br_PackImgScheme2D.hpp"       // PackImgScheme2D_RGB_U16
#include "br_Image.hpp"                 // br::Image
#include "WeightFunc_U16.hpp"           // WeightFunc_U16
#include "ResponseFunc_U16.hpp"         // ResponseFunc_U16
#include "TheProgressInfo.hpp"          // TheProgressInfo
#include "BadPixelProtocol.hpp"         // BadPixelProtocol
#include "mergeHdr_PackSch2D_RGB_U16.hpp"



namespace br {

using namespace TNT;
using std::min;
using std::max;
using std::cout;



/**+*************************************************************************\n
  merge_Hdr_RGB_U16()  --  Description see: merge_Hdr_RGB_U8().
  
  Provisorisch, um ins momentane HdrManager-Regime zu passen: Wir interpretieren
   die logX-Arrays als Gitterwerte einer tabellierten Responsefunktion zu den
   Stuetzstellen z=256*i+127. Die Spline-y2-Tabellen werden hier erzeugt. Besser
   sollten diese Responsefunktionen als Argument uebergeben werden, was erlaubte,
   die Funktionen ausserhalb zu initialisieren (spline oder tabellieren) und so 
   mehrfach zu verwenden.
******************************************************************************/
br::Image    // IMAGE_RGB_F32
merge_Hdr_RGB_U16 (const br::PackImgScheme2D_RGB_U16  & pack,
                   const TNT::Array1D<double>   & logX_R,
                   const TNT::Array1D<double>   & logX_G,
                   const TNT::Array1D<double>   & logX_B,
                   const WeightFunc_U16         & weight,
                   bool                         mark_bad_pixel, 
                   bool                         protocol_to_file,
                   bool                         protocol_to_stdout )
{
    cout <<__func__<< "(Array1D)...\n";
    
    assert (logX_R.dim() == 256);
    assert (logX_G.dim() == 256);
    assert (logX_B.dim() == 256);
    
    //  Create interpolating response functions from the table values
    ResponseFunc_U16  resp_R (logX_R),
                      resp_G (logX_G),
                      resp_B (logX_B);
    
    return merge_Hdr_RGB_U16 (pack, resp_R, resp_G, resp_B, weight, mark_bad_pixel,
                              protocol_to_file, protocol_to_stdout);
}

/**+*************************************************************************\n
  merge_Hdr_RGB_U16()  --  see: merge_Hdr_RGB_U8()
******************************************************************************/
br::Image    // IMAGE_RGB_F32
merge_Hdr_RGB_U16 (const br::PackImgScheme2D_RGB_U16 &  pack,
                   const ResponseFunc_U16 &  logX_R,
                   const ResponseFunc_U16 &  logX_G,
                   const ResponseFunc_U16 &  logX_B,
                   const WeightFunc_U16   &  weight,
                   bool                      mark_bad_pixel, 
                   bool                      protocol_to_file,
                   bool                      protocol_to_stdout )
{
    cout <<__func__<< "(ResponseFunc)...\n";
    //assert (weight.have_table());

    //  Init protocol stuff
    const char*       fname = protocol_to_file ? BadPixelProtocol::default_fname() : 0;
    BadPixelProtocol  protocol (fname, protocol_to_stdout);
    bool              make_protocol = protocol.file() || protocol_to_stdout;
    cout << "\t(to_file=" << protocol_to_file 
         << ",  to_stdout=" << protocol_to_stdout
         << ",  make_protocol=" << make_protocol << ")\n";
    
    //  Init progress info
    TheProgressInfo::start (1.0, _("Merging HDR image..."));
    
    int          nlayers = pack.size();
    int          dim1    = pack.dim1();
    int          dim2    = pack.dim2();
    const uint16 z_max   = 65535;
    
    const Array1D<double> & logtimes = pack.logtimeVec(); // short name

    //  Determine min. threshold z-value, where weight fnct switches to != 0
    uint16  z_threshold_min = 0;
    while ((z_threshold_min < z_max) && (weight(z_threshold_min) == 0.0)) 
      z_threshold_min ++;
    cout << "\tz-threshold-min = " << (int)z_threshold_min << '\n';

    //===========
    // 2D-Sicht
    //===========
    //  Alloc (untyped buffer for) HDR image (dim2==W, dim1==H)
    Image  img (dim2, dim1, IMAGE_RGB_F32, "HDR: RGB 32-bit Float");
    //  ...and create a typecasted 2D-view on it
    ImgScheme2DView< Rgb<float> >  hdr (img);

    //  Smallest and largest resolvable HDR value of this image series
    Rgb<float> val_min (exp (logX_R(0) - logtimes[nlayers-1]),
                        exp (logX_G(0) - logtimes[nlayers-1]),
                        exp (logX_B(0) - logtimes[nlayers-1]));

    Rgb<float> val_max (exp (logX_R(z_max) - logtimes[0]),
                        exp (logX_G(z_max) - logtimes[0]),
                        exp (logX_B(z_max) - logtimes[0]));

    float max_float = std::numeric_limits<float>::max();
    Rgb<float>  h_min ( max_float);  // Init with largest possible value
    Rgb<float>  h_max (-max_float);  // Init with least possible value

    Stopwatch uhr;  uhr.start();
    float progress_add = 1.0 / dim1;  // progress forward per line

    //  for all pixels...
    for (int y=0; y < dim1; y++)
    {
      for (int x=0; x < dim2; x++)
      {
        Rgb<double> sum_w (0.0);          // for summation of weights
        Rgb<double> sum_e (0.0);          // for summation of radiance values
      
        for (int p=0; p < nlayers; p++)
        {
          Rgb<uint16>  z = pack [p] [y] [x];
          Rgb<double>  w;
          if (weight.have_table())  
               w = Rgb<double>(weight[z.r], weight[z.g], weight[z.b]);
          else w = Rgb<double>(weight(z.r), weight(z.g), weight(z.b));

          sum_w += w;
//        sum_e += w * Rgb<double>(..., ..., ...)
          sum_e.r += w.r * (logX_R(z.r) - logtimes[p]);
          sum_e.g += w.g * (logX_G(z.g) - logtimes[p]);
          sum_e.b += w.b * (logX_B(z.b) - logtimes[p]);
        }
      
        Rgb<float>&  h = hdr [y] [x];     // short name

        //  Compute "h = exp (sum_e / sum_w)", consider cases with sum_w==0
        //
        //  Ob sum_w==0 durch z==0 oder z==zmax (in allen Bildern) verursacht
        //   worden war, wird festgestellt, indem der Wert in (irgend)einem Bild
        //   angeschaut wird, wir nehmen Bild 0. Theoretisch ist sum_w==0 auch in
        //   Kombination von z==0 UND z==zmax moeglich, aber pathologisch und hier
        //   vorerst nicht beruecksichtigt. 
      
        //  Kanaele bei Nullwichtung separat auf Grenzwerte setzen
        if (sum_w.r != 0.0)
          h.r = exp (sum_e.r / sum_w.r);        // float <- double
        else {
          if (pack [0][y][x].r < z_threshold_min) // nehmen beispielhaft Bild 0
               h.r = val_min.r;            
          else h.r = val_max.r;                 // also z > z_threshold_max
        }

        if (sum_w.g != 0.0)
          h.g = exp (sum_e.g / sum_w.g);        // float <- double
       else {
          if (pack [0][y][x].g < z_threshold_min) // nehmen beispielhaft Bild 0
               h.g = val_min.g;
          else h.g = val_max.g;                 // also z > z_threshold_max
        }

        if (sum_w.b != 0.0)
          h.b = exp (sum_e.b / sum_w.b);        // float <- double
        else {
          if (pack [0][y][x].b < z_threshold_min) // nehmen beispielhaft Bild 0
               h.b = val_min.b;
          else h.b = val_max.b;                 // also z > z_threshold_max
        }

        //  Protokoll-Ausgabe: vor dem Markieren, da Letzteres `h´ aendert und
        //   danach nur die Markierungswerte noch im Protokoll erschienen.
        if (make_protocol)  
          protocol.out (x,y, sum_w, h, val_min, val_max);
             
        //  Mark "bad" (cropped, unresolved) pixel
        if (mark_bad_pixel) {
          if (sum_w.r == 0) 
            if (h.r == val_min.r)  h.r = 1.0;     // Kontrast zum Minimum
            else                   h.r = 0.0;     // Kontrast zum Maximum
      
          if (sum_w.g == 0) 
            if (h.g == val_min.g)  h.g = 1.0;     // Kontrast zum Minimum
            else                   h.g = 0.0;     // Kontrast zum Maximum

          if (sum_w.b == 0) 
            if (h.b == val_min.b)  h.b = 1.0;     // Kontrast zum Minimum
            else                   h.b = 0.0;     // Kontrast zum Maximum
        }  
      
        //  MinMax feststellen... (fakultativ)
        if (h.r < h_min.r) h_min.r = h.r; else
        if (h.r > h_max.r) h_max.r = h.r;

        if (h.g < h_min.g) h_min.g = h.g; else
        if (h.g > h_max.g) h_max.g = h.g;

        if (h.b < h_min.b) h_min.b = h.b; else
        if (h.b > h_max.b) h_max.b = h.b;
      
      }  // for all x (columns)
      
      TheProgressInfo::forward (progress_add);
    
    }  // for all y (lines)

        
    //  Weil bei streng fallenden Folgen obige else-Zweige nie erreicht
    //   wuerden, pruefen, ob h_max geaendert wurde; wenn nicht, ist
    //   Maximalwert der erste, denn wie gesagt: streng fallend...
    
    if (h_max.r == -max_float) h_max.r = hdr[0][0].r;
    if (h_max.g == -max_float) h_max.g = hdr[0][0].g;
    if (h_max.b == -max_float) h_max.b = hdr[0][0].b;

    double zeit = uhr.stop();
    printf ("\tTime for HDR merging: %f sec\n", zeit);

    cout << "min. resolvable HDR value = " << val_min << '\n';
    cout << "   image's min. HDR value = " << h_min   << '\n';
    cout << "max. resolvable HDR value = " << val_max << '\n';
    cout << "   image's max. HDR value = " << h_max   << '\n';

    TheProgressInfo::finish();
    return img;
}


/**+*************************************************************************\n
  merge_Hdr_RGB_U16 (..., TNT::Array1D)  
   
  Provisorisch... see: merge_Hdr_RGB_U16(..., TNT::Array1D).
******************************************************************************/
br::Image    // IMAGE_RGB_F32
merge_LogHdr_RGB_U16 (const br::PackImgScheme2D_RGB_U16 &  pack,
                      const TNT::Array1D<double> &  logX_R,
                      const TNT::Array1D<double> &  logX_G,
                      const TNT::Array1D<double> &  logX_B,
                      const WeightFunc_U16 &        weight,
                      bool                          mark_bad_pixel, 
                      bool                          protocol_to_file,
                      bool                          protocol_to_stdout )
{
    cout <<__func__<< "(Array1D)...\n";
    
    assert (logX_R.dim() == 256);
    assert (logX_G.dim() == 256);
    assert (logX_B.dim() == 256);
    
    //  Create interpolating response functions from the table values
    ResponseFunc_U16  resp_R (logX_R),
                      resp_G (logX_G),
                      resp_B (logX_B);
    
    return merge_LogHdr_RGB_U16 (pack, resp_R, resp_G, resp_B, weight, mark_bad_pixel, 
                                 protocol_to_file, protocol_to_stdout); 
}

/**+*************************************************************************\n
  merge_LogHdr_RGB_U16 (ResponseFunc_U16)  --  see: merge_Hdr_RGB_U16().
******************************************************************************/
br::Image    // IMAGE_RGB_F32
merge_LogHdr_RGB_U16 (const br::PackImgScheme2D_RGB_U16 &  pack,
                      const ResponseFunc_U16 &  logX_R,
                      const ResponseFunc_U16 &  logX_G,
                      const ResponseFunc_U16 &  logX_B,
                      const WeightFunc_U16   &  weight,
                      bool                      mark_bad_pixel, 
                      bool                      protocol_to_file,
                      bool                      protocol_to_stdout )
{
    cout <<__func__<< "( ResponseFunc_U16 )...\n";
    //assert (weight.have_table());

    //  Init protocol stuff
    const char*       fname = protocol_to_file ? BadPixelProtocol::default_fname() : 0;
    BadPixelProtocol  protocol (fname, protocol_to_stdout);
    bool              make_protocol = protocol.file() || protocol_to_stdout;
    
    cout << "\t(to_file=" << protocol_to_file 
         << ",  to_stdout=" << protocol_to_stdout
         << ",  make_protocol=" << make_protocol << ")\n";
    
    //  Init progress info
    TheProgressInfo::start (1.0, _("Merging logarithmic HDR image..."));

    //  "Marken-Werte" zur Markierung null-gewichteter Pixel. Sollten Werte
    //   sein, die moeglichst nicht als (logarithm) HDR-Werte auftreten.
    const float  mark_for_Min = 1e30;
    const float  mark_for_Max = 1e31;
    
    int          nlayers = pack.size();
    int          dim1    = pack.dim1();
    int          dim2    = pack.dim2();
    const uint16 z_max   = 65535;
    
    const Array1D<double> & logtimes = pack.logtimeVec(); // short name

    //  Determine min. threshold z-value, where weight fnct switches to != 0
    uint16  z_threshold_min = 0;
    while ((z_threshold_min < z_max) && (weight(z_threshold_min) == 0.0)) 
      z_threshold_min ++;
    cout << "\tz-threshold-min = " << (int)z_threshold_min << '\n';
    
    //==========
    // 2D-Sicht
    //==========
    //  Alloc (untyped buffer for) HDR image (dim2==W, dim1==H)
    Image  img (dim2, dim1, IMAGE_RGB_F32, "HDR: RGB 32-bit Float");
    //  ...and create a typecasted 2D view on it
    ImgScheme2DView< Rgb<float> >  hdr (img);

    //  Smallest and largest resolvable log.HDR value of this image series
    Rgb<float> val_min (logX_R(0) - logtimes[nlayers-1],
                        logX_G(0) - logtimes[nlayers-1],
                        logX_B(0) - logtimes[nlayers-1]);

    Rgb<float> val_max (logX_R(z_max) - logtimes[0],
                        logX_G(z_max) - logtimes[0],
                        logX_B(z_max) - logtimes[0]);

    float max_float = std::numeric_limits<float>::max();
    Rgb<float> h_min ( max_float);    // Init with largest possible value
    Rgb<float> h_max (-max_float);    // Init with smallest possible value

    Stopwatch uhr;  uhr.start();
    float progress_add = 0.95 / dim1;  // progress forward per line

    //  For all pixels...
    for (int y=0; y < dim1; y++)
    {
      for (int x=0; x < dim2; x++)
      {
        Rgb<double> sum_w (0.0);      // for summation of weights
        Rgb<double> sum_e (0.0);      // for summation of radiance values

        for (int p=0; p < nlayers; p++)
        {
          Rgb<uint16>  z = pack [p] [y] [x];
          Rgb<double>  w;  
          if (weight.have_table())  
               w = Rgb<double>(weight[z.r], weight[z.g], weight[z.b]);
          else w = Rgb<double>(weight(z.r), weight(z.g), weight(z.b)); 
          
          sum_w += w;
          //  sum_e += w * Rgb<double>(..., ..., ...)
          sum_e.r += w.r * (logX_R(z.r) - logtimes[p]);
          sum_e.g += w.g * (logX_G(z.g) - logtimes[p]);
          sum_e.b += w.b * (logX_B(z.b) - logtimes[p]);
        }
      
        Rgb<float>&  h = hdr [y] [x];     // short name

        //  Compute "h = sum_e / sum_w", consider cases with sum_w==0!
        //   Comment see merge_Hdr_RGB_U8().
      
        /*  Kanaele bei Nullwichtung separat auf Grenzwerte setzen */
        if (sum_w.r != 0.0)
          h.r = sum_e.r / sum_w.r;              // float <- double
        else {
          if (pack [0][y][x].r < z_threshold_min) // nehmen bspielhaft Bild 0
               h.r = val_min.r;
          else h.r = val_max.r;                 // also z > z_threshold_max
        }

        if (sum_w.g != 0.0)
          h.g = sum_e.g / sum_w.g;              // float <- double
        else {
          if (pack [0][y][x].g < z_threshold_min) // nehmen bspielhaft Bild 0
               h.g = val_min.g;
          else h.g = val_max.g;                 // also z > z_threshold_max
        }

        if (sum_w.b != 0.0)
          h.b = sum_e.b / sum_w.b;              // float <- double
        else {
          if (pack [0][y][x].b < z_threshold_min) // nehmen bspielhaft Bild 0
               h.b = val_min.b;
          else h.b = val_max.b;                 // also z > z_threshold_max
        }

        //  Protokoll-Ausgabe: VOR dem Markieren, da Letzteres HDR-Wert aendert
        //   und danach nur die Markierungswerte noch im Protokoll erschienen.
        //   Allerdings werden hier die *unskalierten* Werte ausgegeben!
        if (make_protocol)  
          protocol.out (x,y, sum_w, h, val_min, val_max);
             
        //  Mark "bad" (cropped, zero-weighted, unresolved) pixel. Weil wir anschliessend
        //   noch skalieren muessen, koennen nicht hier schon die Kontrastwerte gesetzt
        //   werden. Wir setzen stattdessen "Marken-Werte", die wir spaeter wieder 
        //   identifizieren koennen. Zugleich sind -- obligat fuer eine Skalierung --
        //   die Min-Max-Werte zu suchen. Die Marken-Werte sind dabei natuerlich 
        //   auszusparen. Deshalb sind Min-Max-Suche und Markierung hier verschraenkt.
        if (mark_bad_pixel) 
        {
          if (sum_w.r == 0) {
            if (h.r == val_min.r)  h.r = mark_for_Min;
            else                   h.r = mark_for_Max;
          } 
          else {  //  MinMax feststellen
            if (h.r < h_min.r) h_min.r = h.r; else
            if (h.r > h_max.r) h_max.r = h.r;
          }
        
          if (sum_w.g == 0) {
            if (h.g == val_min.g)  h.g = mark_for_Min;
            else                   h.g = mark_for_Max;
          }
          else {  //  MinMax feststellen
            if (h.g < h_min.g) h_min.g = h.g; else
            if (h.g > h_max.g) h_max.g = h.g;
          }

          if (sum_w.b == 0) {
            if (h.b == val_min.b)  h.b = mark_for_Min;
            else                   h.b = mark_for_Max;
          }
          else {  //  MinMax feststellen
            if (h.b < h_min.b) h_min.b = h.b; else
            if (h.b > h_max.b) h_max.b = h.b;
          }
        }  
        else {    //  MinMax feststellen ohne Markieren (einfach)
          if (h.r < h_min.r) h_min.r = h.r; else
          if (h.r > h_max.r) h_max.r = h.r;

          if (h.g < h_min.g) h_min.g = h.g; else
          if (h.g > h_max.g) h_max.g = h.g;

          if (h.b < h_min.b) h_min.b = h.b; else
          if (h.b > h_max.b) h_max.b = h.b;
        }
        
      }  // for all x (columns)
        
      TheProgressInfo::forward (progress_add);

    }  // for all y (lines)
    
    
    //  Weil bei streng fallenden Folgen obige else-Zweige nie erreicht wuerden,
    //   pruefen, ob h_max geaendert wurde; wenn nicht, ist Maximalwert der
    //   erste, denn wie gesagt: *streng fallend*.
    //   Falls allerdings "bad pixel" markiert wurden, koennte erste Pixel einen
    //   Markierungswert haben. In diesem Fall ist der erste nichtmarkierte zu
    //   suchen, welcher dann auch der erste in die Min-Max-Suche eingegangene.
    if (mark_bad_pixel) {
      int i;
      if (h_max.r == -max_float) {      // Suche ersten Nichtmarkierten
        i = 0;
        while ((hdr.view1d(i).r == mark_for_Min || 
                hdr.view1d(i).r == mark_for_Max) && i < hdr.size())  i++;
        h_max.r = hdr.view1d(i).r;
      }                
      if (h_max.g == -max_float) {      // Suche ersten Nichtmarkierten
        i = 0;
        while ((hdr.view1d(i).g == mark_for_Min || 
                hdr.view1d(i).g == mark_for_Max) && i < hdr.size())  i++;
        h_max.g = hdr.view1d(i).g;
      }                
      if (h_max.b == -max_float) {      // Suche ersten Nichtmarkierten
        i = 0;
        while ((hdr.view1d(i).b == mark_for_Min || 
                hdr.view1d(i).b == mark_for_Max) && i < hdr.size())  i++;
        h_max.b = hdr.view1d(i).b;
      }                
    }
    else {  
      if (h_max.r == -max_float) h_max.r = hdr[0][0].r;
      if (h_max.g == -max_float) h_max.g = hdr[0][0].g;
      if (h_max.b == -max_float) h_max.b = hdr[0][0].b;
    }

    double zeit = uhr.stop();
    printf ("\tTime for Log-HDR merging: %f sec\n", zeit);

    cout << "min. resolvable HDR value = " << val_min << '\n';
    cout << "   image's min. HDR value = " << h_min   << '\n';
    cout << "max. resolvable HDR value = " << val_max << '\n';
    cout << "   image's max. HDR value = " << h_max   << '\n';

    //  Logarithm domain is (-infty, +infty). Normalize values to [0,1]:
    //   Scale so, that overall-max -> 1, overall-min -> 0:
    float max_all = max (max(h_max.r, h_max.g), max(h_max.g, h_max.b));  
    float min_all = min (min(h_min.r, h_min.g), min(h_min.g, h_min.b));  
    float fac_all = 1.0 / (max_all - min_all);

    cout << "max_all=" << max_all << "  min_all=" << min_all << "  fac_all=" << fac_all << '\n'; 
    
    //  Falls "bad pixel" markiert wurden, diese jetzt raussuchen und auf ihre
    //   Kontrastwerte setzen. Falls nicht markiert wurde, schlicht skalieren.
    if (mark_bad_pixel)
      for (int i=0; i < hdr.size(); i++)          // 1D-View
      {
        Rgb<float>& h = hdr.view1d(i);            // short name
      
        if (h.r == mark_for_Min)       h.r = 1.0;     // Kontrast zum Minimum
        else if (h.r == mark_for_Max)  h.r = 0.0;     // Kontrast zum Maximum
        else                           h.r = fac_all * (h.r - min_all);
        
        if (h.g == mark_for_Min)       h.g = 1.0;     // Kontrast zum Minimum
        else if (h.g == mark_for_Max)  h.g = 0.0;     // Kontrast zum Maximum
        else                           h.g = fac_all * (h.g - min_all);
        
        if (h.b == mark_for_Min)       h.b = 1.0;     // Kontrast zum Minimum
        else if (h.b == mark_for_Max)  h.b = 0.0;     // Kontrast zum Maximum
        else                           h.b = fac_all * (h.b - min_all);
      }
    else
      for (int i=0; i < hdr.size(); i++)          // 1D-View
        hdr.view1d(i) = fac_all * (hdr.view1d(i) - min_all);
      
    TheProgressInfo::finish();
    return img;
}


#if 0
/**+*************************************************************************\n
  merge_LogHdr_RGB_U16()
  
  Einfach gemacht: Berechnen erst normales HDR und logarithmieren dann. So 
   natuerlich langsamer statt schneller.
   
  @note  Das hier praktiziertee schlichte Logarithmieren setzt HDR-Werte <> 0
   voraus. Wenn das nicht mehr erfuellt, waere fuer hdr==0 ein kleiner Wert zu setzen
   (in der Groessenordnung des kleinsten log-Werte fuer hdr!=0), um anschliessend
   sinnvoll auf [0,1] zu skalieren. "-oo" waere natuerlich sinnlos. Praktisch
   koennte man hdr=limits<float>.min() setzen, aber aus Min-Max-Suche aussparen,
   und bei Skalierung die limit-Werte herausfischen und sinnvoll setzen.
******************************************************************************/
br::Image    // IMAGE_RGB_F32
merge_LogHdr_RGB_U16 (const br::PackImgScheme2D_RGB_U16 &  pack,
                      const TNT::Array1D<double> &  logX_R,
                      const TNT::Array1D<double> &  logX_G,
                      const TNT::Array1D<double> &  logX_B,
                      const WeightFunc_U16       &  weight,
                      bool                          mark_bad_pixel,
                      ProgressInfo *                progressinfo,
                      bool                          protocol_to_file,
                      bool                          protocol_to_stdout )
{
    cout <<__func__<< "(Array1D)...\n";
    
    //  Compute ordinary HDR
    Image  img = merge_Hdr_RGB_U16 (pack, logX_R, logX_G, logX_B, weight, mark_bad_pixel,
                                    progressinfo, protocol_to_file, protocol_to_stdout);
    img.report();
    if (img.is_null()) return img;
    
    //  Typecasted 1D view on image data
    ImgView1D< Rgb<float> >  hdr (img);
    
    //  Logarithm the first value
    hdr[0] = Rgb<float> (log(hdr[0].r), log(hdr[0].g), log(hdr[0].b));
    //  and init min-max-values with it
    Rgb<float> h_min = hdr[0];
    Rgb<float> h_max = hdr[0];
    
    //  For the rest of the pixels
    for (int i=1; i < hdr.dim(); i++)
    {
      Rgb<float>&  h = hdr[i];          // short name
      h.r = log (h.r);
      h.g = log (h.g);
      h.b = log (h.b);
      
      //  MinMax feststellen
      if (h.r < h_min.r) h_min.r = h.r; else
      if (h.r > h_max.r) h_max.r = h.r;

      if (h.g < h_min.g) h_min.g = h.g; else
      if (h.g > h_max.g) h_max.g = h.g;

      if (h.b < h_min.b) h_min.b = h.b; else
      if (h.b > h_max.b) h_max.b = h.b;
    }
    cout << "min = " << h_min << '\n';
    cout << "max = " << h_max << '\n';

    //  Scale so, that overall-max -> 1, overall-min -> 0:
    float max_all = max (max(h_max.r, h_max.g), max(h_max.g, h_max.b));  
    float min_all = min (min(h_min.r, h_min.g), min(h_min.g, h_min.b));  
    float fac_all = 1.0 / (max_all - min_all);

    cout << "max_all=" << max_all << "  min_all=" << min_all << "  fac_all=" << fac_all << '\n'; 
      
    for (int i=0; i < hdr.dim(); i++)
      hdr[i] = fac_all * (hdr[i] - min_all);
    
    return img;
}                         
#endif


}  // namespace "br"

// END OF FILE

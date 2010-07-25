/*
 * mergeHdr_PackSch2D_RGB_U8.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  mergeHdr_PackSch2D_RGB_U8.cpp
  
  Contents: 
   - merge_Hdr_RGB_U8()
   - merge_LogHdr_RGB_U8()
   
  @todo ProgressInfo strings like "Merge HDR image..." are used both here and in
    the U16 case (file "mergeHdr_PckSch2D_RGB_U16.cpp"). Could be centralized.
    Would secure message consistency as well as reduce POTFILES.in entries.
*/

#include <cassert>                      // assert()
#include <iostream>                     // std::cout
#include <limits>                       // numeric_limits<>
#include <cmath>                        // exp()

#include "TNT/tnt_array1d.hpp"          // TNT::Array1D<>
#include "TNT/tnt_stopwatch.hpp"        // TNT::Stopwatch

#include "i18n.hpp"                     // macro _()
#include "Rgb.hpp"                      // Rgb<>
#include "Rgb_utils.hpp"                // <<-Op for Rgb<>
#include "br_PackImgScheme2D.hpp"       // PackImgScheme2D_RGB_U8
#include "br_Image.hpp"                 // br::Image
#include "WeightFunc_U8.hpp"            // WeightFunc_U8
#include "TheProgressInfo.hpp"          // TheProgressInfo
#include "BadPixelProtocol.hpp"         // BadPixelProtocol
#include "mergeHdr_PackSch2D_RGB_U8.hpp" // prototypes



namespace br {

using namespace TNT;
using std::min;
using std::max;
using std::cout;


/**+*************************************************************************\n
  merge_Hdr_RGB_U8() 
  
  Merging of a Pack of typed \a ImgScheme2D schemes of RGB_U8 image buffers 
   into one single HDR(float) Rgb-image using three given response curves, a
   (in the Pack) given vector of exposure times and a given weight function.
   
  @param pack: Pack of typed \a ImgScheme2D schemes of RGB_U8 image buffers.

  @param logX_R|G|B: Logarithm. inverse of the response function <i>z=f(X)</i>. 
     \a logX_R[z]: Logarithm exposure \a log(X) to the pixel value \a z in 
     channel \a R accordingly to <pre>
        z = f(X) = f(E*t) 
        f^-1(z) = X = E*t  
        g(z) = log(X) = log(E)+log(t)  
     with
        g(z) := log[f^-1(z)]. </pre>
 
  @param weight: Weight function object; weight[z]: weight of value z.
    Almost jene auch bei der logX_*-Berechnung verwendete. 

  @param progressinfo: Pointer to a ProgressInfo instance. Default: 0.
 
  @param protocol_to_file: A bad-pixel-protocol to a file? Default: false.
     (Using default filename of BadPixelProtocol.)
     
  @param protocol_to_stdout: A bad-pixel-protocol to stdout? Default: false.
  
  @return HDR<float>-RGB-Bild: die aus <i>log(E)=log(X)-log(t)</i> resultierenden
     E-Daten: logE(z) = logX(z) - logTimes, oder vielmehr delogarithmiert:
     exp(logE).
 
  
  @note Beachte, dass die Scheme2D der Pack-Ausgangsdaten Sub-Arrays darstellen
   koennen. Benutze fuer diese daher nur die "[][]"-Syntax oder linearisiere
   allenfalls innerhalb einer Zeile. Fuer das hier erzeugte HDR-Bild dagegen ist
   klar, dass es zusammenhaengend; da kann auch view1d() zur Anwendung kommen.

  
  <h3>Bemerkungen:</h3>
  
  <b>Unaufgeloeste HDR-Werte:</b>
   Einfach gesagt: Pixelwerte z=255 im kuerzestbelichteten Bild oder z=0 im
   laengstbelichteten sind unaufgeloest (nicht im Arbeitsbereich). Weil wir alle
   z-Werte wichten und durch breitere Nullraender der Gewichtsfunktionen auch
   mehr unaufgeloest sein koennen, machen wir es am Gewicht fest. Eine vereinfachte
   Alternative zur Markierungsweise unten waere, zum Schluss das hellste und das
   dunklelste Bild noch einmal durchzugehen und die Stellen mit z=0 bzw. z=255
   zu markieren.
  
   Wenn ein Pixel bzgl. eines Kanals in ALLEN Bildern das Gewicht 0 erhaelt,
   bekommen wir sum_w = sum_e = 0 und die Operation "sum_e / sum_w" kann nicht
   ausgefuehrt werden ("0/0"). Der HDR-Wert (jenes Kanals) ist an dieser Stelle 
   unbestimmt, er muss geeignet <i>gesetzt</i> werden. Weil Wichtungsfunktionen
   gewoehnlich nur an den Raendern bei z=0 und z=zmax Null werden, wird aus sum_w=0
   auf "z=0 oder z=max" (in allen Bildern) geschlossen und dass der unbekannte HDR-Wert 
   (E) folglich  unterhalb bzw. oberhalb des durch diese Belichtungsreihe noch
   Aufloesbaren liegt. Wir setzen ihn daher fuer z=0 auf die untere Aufloesungsgrenze 
   (min. HDR-, sprich E-Wert) und fuer z=zmax auf die obere (max. HDR-, sprich E-Wert).
   
   Der kleinste reale E-Wert, den diese Belichtungsreihe erfassen koennte, ist
   der zu <b>z=0</b> im Bild mit der <b>laengsten</b> Belichtungszeit, der groesste
   noch erfassbare der zu <b>z=zmax</b> im Bild mit der <b>kuerzesten</b> 
   Belichtungszeit. Gemaess <pre>
            logE = logX - logtimes </pre>
   und dem Umstand, dass die Bilder in aufsteigender Belichtungszeit geordnet, 
   also <pre>
            logE_min = logX[0] - logtimes[nlayers-1]
            logE_max = logX[z_max] - logtimes[0]  </pre>
   
  <b>Nachtrag:</b>
   Wir arbeiten unten mit der UNTERSTELLUNG, dass Wichtungsfunktionen w(z) nur von
   den Raendern her und in zusammenhaengenden Bereichen Null liefern koennen, d.h.
   dass es zwei Schwellwerte \a z_threshold_min und \a z_threshold_max gebe mit
   <pre>
      w(z) == 0 fuer  z .<. z_threshold_min (zu dunkel -> min. HDR-Wert)
      w(z) == 0 fuer  z .>. z_threshold_max (zu hell -> max. HDR-Wert)
      w(z) != 0 sonst. </pre>
   
   Stossen wir auf ein sum_w==0, wird anhand des z-Wertes entschieden, ob der 
   HDR-Wert auf min. oder max. HDR-Wert gesetzt wird. Fuer Wichtungsfunktionen mit
   Nullbereichen in der Mitte wuerde das nicht mehr funktionieren, der zweite else-
   Zweig in der Abfrage unten verwiese nicht mehr notwendig auf z.>.z_threshold_max.

   Die Klaerung, ob null-gewichtete Pixel auf Min- oder Max-HDR-Wert zu setzen
   sind anhand der Abfrage
   <pre>
      if (sum_w.r != 0)  berechne sum_e.r/sum_w.r;
      else if (pixel.r .<. z_threshold_min) set_to_min;
      else                                  set_to_max;  </pre>
      
   funktioniert auch fuer Wichtungsfunktionen, die gar nicht oder nur einseitig 
   Null werden: Wenn gar nicht Null, kann sum_w==0 nicht auftreten, wenn einseitig
   links Null, kann sum_w nur fuer linke z.<.z_threshold_min Null werden und das 
   ist der erste else-Zweig, wenn einseitig rechts Null, ist z_threshold_min==0
   und die Bedingung z.<.z_threshold_min kann fuer solche z nicht zutreffen, 
   sprich, es wird richtigerweise im zweiten else-Zweig auf Max gesetzt. Wenn eine
   Wichtungsfunktion komplett 0, ist z_threshold_min=255 und alle z-Werte ausser
   z=255 sind kleiner als z_threshold_min und werden auf min-HDR gesetzt; z=255
   auf max-HDR.
   
  <b>Kontrastierungen:</b>
   Das Markieren null-gewichter Pixel (d.h. von HDR-Werten, die auf min-HDR oder
   max-HDR gesetzt wuerden) durch Kontrastfarben geschieht wie folgt: Wuerde ein
   HDR-Wert auf min-HDR gesetzt), bekommt er den Wert 1.0 (Kontrast zur angenommenen
   dunklen Umgebung), wuerde er auf max-HDR gesetzt, bekommt er den Wert 0.0 (Kontrast
   zur angenommenen hellen Umgebung. Schief (nicht in allen Kanaelen) abgeschnittene
   HDR-Pixel erscheinen dadurch farbig:  <pre>
    unten: nur R abgeschnitten --> RGB = (1, wenig, wenig)  (rot)
           nur G abgeschnitten --> RGB = (wenig, 1, wenig)  (gruen)
           R+G   abgeschnitten --> RGB = (1, 1, wenig)
    oben:  nur R abgeschnitten --> RGB = (0, viel, viel)    (cyan)
           R+G   abgeschnitten --> RGB = (0, 0, viel)       (blau) </pre>
           
  @todo Der Kontrastwert "1.0" hat den Nachteil, dass er in einem HDR-Bild, das z.B.
   bis 10^3 reicht, bei entsprechender Skalierung nicht mehr auffaellt. Alternativ
   koennte als Kontrast zum Dunklen auch max-HDR (val_max) oder der Bildmaximalwert
   (h_max) genommen werden. Fuer letzteres muesste Kontrastieren in einem zweiten
   Durchlauf erfolgen, da die Min-Max-Werte ja erst ermittelt werden muessen, was
   wiederum verlangte, wie im log-Fall die "0/0"-Faelle aus der Min-Max-Ermittlung
   zunaechst auszusparen. \n
  hdr_max != h_max kann dann eintreten, wenn die wegen "0/0" irgendwie zu setzenden
   Werte nicht in Min-Max-Berechnung eingehen, sondern wie im log-Fall zunaechst 
   ausgespart werden und erst anschliessend auf endgueltigen Wert gesetzt werden, 
   z.B. eben dann auf h_max.
  
  @todo Es koennte die Option angeboten werden, schief abgeschnittene Pixel (die
   naemlich auffallen) komplett auf schwarzen min-HDR- bzw. komplett auf weissen
   max-HDR-Wert zu setzen. Oder anhand der Umgebung korrigieren? Welches natuerlich
   nur moeglich, wenn die Umgebung nicht selbst gestoert.

******************************************************************************/
br::Image    // IMAGE_RGB_F32
merge_Hdr_RGB_U8 (const PackImgScheme2D_RGB_U8 & pack,
                  const TNT::Array1D<double> &   logX_R,
                  const TNT::Array1D<double> &   logX_G,
                  const TNT::Array1D<double> &   logX_B,
                  const WeightFunc_U8 &          weight,
                  bool                           mark_bad_pixel, 
                  bool                           protocol_to_file,
                  bool                           protocol_to_stdout )
{
    cout <<__func__<< "()...\n";
    assert (logX_R.dim() == 256);
    assert (logX_G.dim() == 256);
    assert (logX_B.dim() == 256);
    assert (weight.have_table());
    //assert (weight.table.dim() == 256);
    //list (weight);

    //  Init protocol stuff
    const char*       fname = protocol_to_file ? BadPixelProtocol::default_fname() : 0;
    BadPixelProtocol  protocol (fname, protocol_to_stdout);
    bool              make_protocol = protocol.file() || protocol_to_stdout;
    
    cout << "\t(make_protocol=" << make_protocol
         << ",  to_file=" << protocol_to_file 
         << ",  to_stdout=" << protocol_to_stdout << ")\n";

    //  Init progress info
    TheProgressInfo::start (1.0, _("Merging HDR image..."));
        
    int         nlayers = pack.size();
    int         dim1    = pack.dim1();
    int         dim2    = pack.dim2();
    const uint8 z_max   = 255;

    const Array1D<double> & logtimes = pack.logtimeVec(); // short name
   
    //  Determine min. threshold z-value, where weight fnct switches to != 0
    uint8  z_threshold_min = 0;
    while ((z_threshold_min < z_max) && (weight[z_threshold_min] == 0.0)) 
      z_threshold_min ++;
    cout << "\tz-threshold-min = " << (int)z_threshold_min << '\n';
    
    //============
    //  2D Sicht
    //============
    //  Alloc (untyped buffer for) HDR image (dim2==W, dim1==H)
    Image  img (dim2, dim1, IMAGE_RGB_F32, "HDR: RGB 32-bit Float");
    //  ...and create a typecasted 2D view on it
    ImgScheme2DView< Rgb<float> >  hdr (img);

    //  Smallest and largest resolvable HDR value of this image series
    Rgb<float> val_min (exp (logX_R[0] - logtimes[nlayers-1]),
                        exp (logX_G[0] - logtimes[nlayers-1]),
                        exp (logX_B[0] - logtimes[nlayers-1]));

    Rgb<float> val_max (exp (logX_R[z_max] - logtimes[0]),
                        exp (logX_G[z_max] - logtimes[0]),
                        exp (logX_B[z_max] - logtimes[0]));

    float max_float = std::numeric_limits<float>::max();
    Rgb<float> h_min ( max_float);  // Init with largest possible value
    Rgb<float> h_max (-max_float);  // Init with least possible value

    Stopwatch uhr;  uhr.start();
    float progress_add = 1.0 / dim1;  // progress forward per line

    //  For all pixels...
    for (int y=0; y < dim1; y++) 
    {
      for (int x=0; x < dim2; x++) 
      {
        Rgb<double> sum_w (0.0);          // for summation of weights
        Rgb<double> sum_e (0.0);          // for summation of radiance values

        for (int p=0; p < nlayers; p++)
        {
          Rgb<uint8>  z = pack [p] [y] [x];
          Rgb<double>  w (weight[z.r], weight[z.g], weight[z.b]);

          sum_w += w;
//        sum_e += w * Rgb<double>(..., ..., ...)
          sum_e.r += w.r * (logX_R[z.r] - logtimes[p]);
          sum_e.g += w.g * (logX_G[z.g] - logtimes[p]);
          sum_e.b += w.b * (logX_B[z.b] - logtimes[p]);
        }
      
        Rgb<float>&  h = hdr [y] [x];     // short name

        //==========
        //  Compute "h = exp (sum_e / sum_w)", consider cases with sum_w==0
        //
        //  Ob sum_w==0 durch z<z_threshold_min [frueher: z==0] oder z>z_threshold_max
        //   [frueher: z==z_max] (in allen Bildern) verursacht worden war, wird festgestellt,
        //   indem der Wert in (irgend)einem  Bild angeschaut wird; wir nehmen Bild 0.
        //   Theoretisch ist sum_w==0 auch in Kombination von z==0 UND z==zmax moeglich,
        //   aber pathologisch und hier voererst nicht beruecksichtigt.
        // 
        //  WARNUNG:  w!=0 ist nicht ausreichend fuer sicheres exp(sum_e/sum_w)! 
        //   Sicherer waere 
        //      if (sum_w.r != 0)
        //        if (sum_e.r/sum_w.r <= max_exp_arg)  h.r=exp(...)
        //        else h.r=...?

#     if 0
        /*  Kanaele bei Nullwichtung separat auf Grenzwerte setzen */
        if (sum_w.r != 0.0)
          h.r = exp (sum_e.r / sum_w.r);        // float <- double
        else {
          if (pack [0][y][x].r < z_threshold_min) // nehmen bspielhaft Bild 0
                 h.r = val_min.r;
            else h.r = val_max.r;                 // also z > z_threshold_max
        }
      
        if (sum_w.g != 0.0)
          h.g = exp (sum_e.g / sum_w.g);        // float <- double
        else {
          if (pack [0][y][x].g < z_threshold_min) // nehmen bspielhaft Bild 0
               h.g = val_min.g;
          else h.g = val_max.g;                 // also z > z_threshold_max
        }
      
        if (sum_w.b != 0.0)
          h.b = exp (sum_e.b / sum_w.b);        // float <- double
        else {
          if (pack [0][y][x].b < z_threshold_min) // nehmen bspielhaft Bild 0
               h.b = val_min.b;
          else h.b = val_max.b;                 // also z > z_threshold_max
        }
#     else
        /*  Probeweise: Sobald ein Kanal nullgewichtet, ganzen RGB auf Grenzwert
             setzen. Vermeidet schief abgeschnittene Farben. */
        if (sum_w.r == 0.0) 
            if (pack [0][y][x].r < z_threshold_min) // nehmen bspielhaft Bild 0
                 h = val_min;   
            else h = val_max;                       // also z > z_threshold_max        
        else
        if (sum_w.g == 0.0)
            if (pack [0][y][x].g < z_threshold_min) // nehmen bspielhaft Bild 0
                 h = val_min;   
            else h = val_max;                       // also z > z_threshold_max
        else
        if (sum_w.b == 0.0)
            if (pack [0][y][x].b < z_threshold_min) // nehmen bspielhaft Bild 0
                 h = val_min;
            else h = val_max;                       // also z > z_threshold_max
        else {                                  // also sum_w != 0
          h.r = exp (sum_e.r / sum_w.r);        // float <- double
          h.g = exp (sum_e.g / sum_w.g);        // float <- double
          h.b = exp (sum_e.b / sum_w.b);        // float <- double
        }
#     endif      
        //  Die Farbkanaele wurden oben unabhaengig behandelt. Fuer sum_w!=0 ist alles ok,
        //   fuer sum_w.x == 0 aber wurde x-ter Kanal auf Min- bzw. Max-Wert gesetzt.
        //   Falls total sum_w==0, nullte Wichtungsfunktion *alle* Kanaele, Pixel gesetzt 
        //   auf Maximal-"Weiss" bzw. Maximal-"Schwarz". Das ist halbwegs ok, der Pixel kann auch
        //   im Helligkeits-Zoom nicht anders als Schwarz oder Weiss erscheinen. Problematischer,
        //   wenn nur teilw. gesetzt wurde (nur einige sum_w.x==0). Durch das Setzen wurde 
        //   die RGB-Relation geaendert und im Helligkeits-Zoom koennen diese als echte Farben
        //   erscheinen, allerdings verfaelschte, "schief abgeschnittene" Farben.

#     if 0
        //===========
        //  BEISPIEL einer effektiven Suche folgender Art:  (aufgehoben)
        //===========
        //  Klaere, ob schlechter (=gesetzter) Pixel, ob dabei vollstaendig oder
        //   teilweise und ob minimaler oder maximaler Wert gesetzt wurde.
        // 
        //   An moeglichen 0/1-Belegungen des Rgb `sum_w´ gibt es die 2^3=8 Moeglichkeiten
        //     000  100 010 001  110 101 011  111
        //   111 ist regulaer, 000 Totalabschnitt, schiefe Farben geben die 6 Faelle
        //          100 010 001  110 101 011
        enum What {
            OK       = 0,
            PURE_MIN = 1,
            PART_MIN = 2,
            PURE_MAX = 3,
            PART_MAX = 4
        }; 
        What what = OK;

        if (sum_w.r == 0) {                     // also sum==0**
          if (sum_w.g != 0 || sum_w.b != 0)     //   also sum==001, 010, 011
            if (h.r == val_min.r) 
                 what = PART_MIN;
            else what = PART_MAX;
          else                                  //   also sum=000
            if (h.r == val_min.r)
                 what = PURE_MIN;
            else what = PURE_MAX;
        }
        else                                    // also sum==1**
        if (sum_w.g == 0) {                     //   also sum==100, 101
          if (h.g == val_min.g)
               what = PART_MIN;
          else what = PART_MAX;
        }
        else                                    // also sum=11*  
        if (sum_w.b == 0) {                     //   also sum==110
          if (h.b == val_min.b)
               what = PART_MIN;
          else what = PART_MAX;
        }
            
        switch (what) 
        {
        case PURE_MIN:
          cout << "pure MIN ["<<x<<", "<<y<<"] = " << h << "   sum_w="<<sum_w<<'\n';
          break;
        case PART_MIN:
          cout << "part MIN ["<<x<<", "<<y<<"] = " << h << "   sum_w="<<sum_w<<'\n';           
          break;
        case PURE_MAX:
          cout << "pure MIN ["<<x<<", "<<y<<"] = " << h << "   sum_w="<<sum_w<<'\n';
          break;
        case PART_MAX:          
          cout << "part MAX ["<<x<<", "<<y<<"] = " << h << "   sum_w="<<sum_w<<'\n';           
          break;
        default: ;  // OK
        }
#     endif      // Beispiel einer Suche

        //  Protokoll-Ausgabe: VOR dem Markieren, da letzteres HDR-Wert aendert
        //   und danach nur die Markierungswerte noch im Protokoll erschienen.
        if (make_protocol)  
          protocol.out (x,y, sum_w, h, val_min, val_max);
             
        //  Mark "bad" (cropped, zero-weighted, unresolved) pixel
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

    
    //  Weil bei streng fallenden Folgen obige else-Zweige nie erreicht wuerden,
    //   pruefen, ob `h_max´ geaendert wurde; wenn nicht, ist Maximalwert der 
    //   erste, denn wie gesagt: streng fallend...
    
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
  merge_LogHdr_RGB_U8()
  
  Merging of a Pack of typed \a ImgScheme2D schemes of RGB_U8 image buffers 
   into one single <i>logarithmic</i> HDR(float) Rgb-image using three given
   response curves, a (in the Pack) given vector of exposure times and a given
   weight function.

  Logarithm. HDR-Bild extra zu bauen deshalb interessant, weil beim gewoehnl.
   fuer alle Werte ein `exp(z)´ auftritt, das hier gespart werden kann. Bei 
   N Pixeln und P LDR-Bildern sind das 3*N*P exp()-Operationen.
  
  @param *: see: merge_Hdr_RGB_U8().
     
  @return logarithmiertes HDR<float>-Bild: die gemaess <i>log(E)=log(X)-log(t)</i>
    resultierenden <i>log(E)</i>-Daten, skaliert auf [0,1].
   
  @note Beachte, dass die Scheme2D der Pack-Ausgangsdaten Sub-Arrays darstellen
   koennen. Benutze fuer diese daher nur die "[][]"-Syntax oder linearisiere
   allenfalls innerhalb einer Zeile. Fuer das hier erzeugte HDR-Bild dagegen ist
   klar, dass es zusammenhaengend; da kann auch view1d() zur Anwendung kommen.

******************************************************************************/
br::Image    // IMAGE_RGB_F32
merge_LogHdr_RGB_U8 (const PackImgScheme2D_RGB_U8 & pack,
                     const TNT::Array1D<double> &   logX_R,
                     const TNT::Array1D<double> &   logX_G,
                     const TNT::Array1D<double> &   logX_B,
                     const WeightFunc_U8 &          weight,
                     bool                           mark_bad_pixel, 
                     bool                           protocol_to_file,
                     bool                           protocol_to_stdout )
{
    cout <<__func__<< "()...\n";
    assert (logX_R.dim() == 256);
    assert (logX_G.dim() == 256);
    assert (logX_B.dim() == 256);
    assert (weight.have_table());
    //assert (weight.dim() == 256);

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
    const uint8  z_max   = 255;
    
    const Array1D<double> & logtimes = pack.logtimeVec(); // short name

    //  Determine min. threshold z-value, where weight fnct switches to != 0
    uint8  z_threshold_min = 0;
    while ((z_threshold_min < z_max) && (weight[z_threshold_min] == 0.0)) 
      z_threshold_min ++;
    cout << "\tz-threshold-min = " << (int)z_threshold_min << '\n';
    
    //==========
    // 2D-Sicht
    //==========
    //  Alloc (untyped buffer for) HDR image (dim2==W, dim1==H)
    Image  img (dim2, dim1, IMAGE_RGB_F32, "HDR: RGB 32-bit Float");
    //  ...and create a typecasted 2D view on it
    ImgScheme2DView< Rgb<float> >  hdr (img);

    //  Smallest and largest resolvable log. HDR value for this series
    Rgb<float> val_min (logX_R[0] - logtimes[nlayers-1],
                        logX_G[0] - logtimes[nlayers-1],
                        logX_B[0] - logtimes[nlayers-1]);

    Rgb<float> val_max (logX_R[z_max] - logtimes[0],
                        logX_G[z_max] - logtimes[0],
                        logX_B[z_max] - logtimes[0]);

    float max_float = std::numeric_limits<float>::max();
    Rgb<float> h_min ( max_float);    // Init with largest possible value
    Rgb<float> h_max (-max_float);    // Init with smallest possible value

    Stopwatch uhr;  uhr.start();
    float progress_add = 0.95 / dim1;  // progress forward per line

    //  For all pixels
    for (int y=0; y < dim1; y++) 
    {
      for (int x=0; x < dim2; x++) 
      {
        Rgb<double> sum_w (0.0);          // for summation of weights
        Rgb<double> sum_e (0.0);          // for summation of radiance values
      
        for (int p=0; p < nlayers; p++)
        {
          Rgb<uint8>  z = pack [p] [y] [x];
          Rgb<double>  w (weight[z.r], weight[z.g], weight[z.b]);
        
          sum_w += w;
          //  sum_e += w * Rgb<double>(..., ..., ...)
          sum_e.r += w.r * (logX_R[z.r] - logtimes[p]);
          sum_e.g += w.g * (logX_G[z.g] - logtimes[p]);
          sum_e.b += w.b * (logX_B[z.b] - logtimes[p]);
        }
      
        Rgb<float>&  h = hdr [y] [x];     // short name

        //  Compute "h = sum_e / sum_w", consider cases with sum_w==0!
        //   Comment see: merge_Hdr_RGB_U8().

        //  Kanaele bei Nullwichtung separat auf Grenzwerte setzen
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
        //   auszusparen. Deshalb sind Min-Max-Suche und Markieren hier verschraenkt.
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
    //   Falls allerdings "bad pixel" markiert wurden, koennte erster Pixel einen
    //   Markierungswert haben. In diesem Fall ist der erste nichtmarkierte zu
    //   suchen, welcher naemlich auch der erste in die Min-Max-Suche eingegangene.
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
    else {  // !mark_bad_pixel
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
#ifdef SCALE_EACH_CHANNEL_INDEPENDENTLY
    //
    //  Scale each channel independently to [0,1]. Causes color shifts!!
    //   Aufbewahrt wegen der schneidig-schoenen Rgb-Schreibweise.
    //   Aber nicht mehr auf aktuellem Stand, Markierung unberuechsichtigt!
    // 
    Rgb<float> fac = Rgb<float>(1.0) / (h_max - h_min);

    for (int i=0; i < hdr.size(); i++)          // 1D-View 
      hdr.view1d(i) = fac * (hdr.view1d(i) - h_min);
#else
    //
    //  Scale so, that overall-max -> 1, overall-min -> 0:
    //
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
#endif
      
    TheProgressInfo::finish();
    return img;
}


}  // namespace "br"

// END OF FILE

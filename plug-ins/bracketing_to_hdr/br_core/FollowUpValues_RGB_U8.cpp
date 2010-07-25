/*
 * FollowUpValues_RGB_U8.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  FollowUpValues_RGB_U8.cpp
*/

#include <cmath>                  // sqrt()
#include "FollowUpValues_RGB_U8.hpp"
#include "br_types_array.hpp"     // type `ChannelFollowUpArray'
#include "FilePtr.hpp"            // FilePtr
#include "br_macros.hpp"          // CTOR(), DTOR(), ASSERT(), IF_FAIL_DO()
#include "DynArray1D.hpp"         // DynArray1D<>
#include "DynArray2D.hpp"         // DynArray2D<>
#include "TheProgressInfo.hpp"    // TheProgressInfo
#include "i18n.hpp"               // macro _()


//#define BR_DEBUG_FOLLOWUP

#ifdef BR_DEBUG_FOLLOWUP
#  define DEBUG_COUT(s)  std::cout << s;
#else
#  define DEBUG_COUT(s)
#endif


namespace br {

using namespace TNT;
using namespace std;

/*==============================
* 
*       IMPLEMTATION
* 
*==============================*/
/**+*************************************************************************\n
  Ctor.
******************************************************************************/
FollowUpValues_RGB_U8::FollowUpValues_RGB_U8 (const Pack & pack) 
  : 
    FollowUpValuesBase (pack, N_Grid),  // N_Grid = 256 (number of z-vals)
    pack_    (pack),                    // ref counting copy
    nlayers_ (pack_.size())
{ 
    CTOR("") 

    //  Allocate the statistics arrays...
    follow_[0] = ChannelFollowUpArray (nlayers_, N_Grid);
    follow_[1] = ChannelFollowUpArray (nlayers_, N_Grid); 
    follow_[2] = ChannelFollowUpArray (nlayers_, N_Grid);
}


/**+*************************************************************************\n
  compute_z_average()
 
  Mittlerer Folgewert in jedem Folgebild zum Wert \a z im Ref-Bild \a refpic.
 
  VORSCHLAG: Die Summation kann lokale Summations-Strukturen benutzen, die
   u.U. auf uint64-Typen basieren und Anzahl-Element enthalten kann, und erst
   anschliessend auf globalere Strukturen umrechnen. Letztere koennten dann 
   platzsparend auch aus floats statt doubles bestehen. Die lokale Summations-
   struktur kann auch das Feld fuer das Ref-Bild einsparen, z.Z. der Einfachheit
   halber nicht getan.

  ChSum h[nlayers][n_zvals]: 
   h[p][z].{s,s2} beschreibt den Folgewert im Bild `p' zum Hauptwert `z' 
   (im RefBild `refpic') bzgl. Mittelwert (s) und Streuung (s2). Bsp.:
   Sei refpic==0. Dann sagt  h[3][56].s == 99.1: der mittlere Folgewert in 
   Bild 3 zum Hauptwert z=56 (in Bild 0) ist z=99.1. Seine Streuung ist
   h[3][56].s2.

  @note 
   - Fuer Summation double notwendig fuer Genauigkeit, floats geben u.U. negative
      s2.
******************************************************************************/
void
FollowUpValues_RGB_U8::compute_z_average (int refpic)
{
    DEBUG_COUT ("FollowUpValues_RGB_U8::"<<__func__<< "(refpic=" << refpic << ")...\n");
    
    ASSERT (0 <= refpic && refpic < pack_.size(), return);
    refpic_ = refpic;
    
    // Init progress info
    TheProgressInfo::start (1.0, _("Computing follow-up values..."));
#if 0
    //===========================
    //  Lokale Summations-Felder:       (ISO C++ forbids variable-size arrays)
    //===========================
    //  h[p][z] beschreibt Folgewert in Bild `p' zum Hauptwert `z' im Ref-Bild 
    //   `refpic'. Genullt per Default-Ctor von `ChSum'.
    ChSum h_R [nlayers_][N_Grid],
          h_G [nlayers_][N_Grid],
          h_B [nlayers_][N_Grid];

    //  n[z] == Anzahl Pixel mit red. Hauptwert z. Ungenullt!
    Rgb<size_t>  nn [N_Grid];
    
    for (unsigned i=0; i < N_Grid; i++)       // nullen
      nn[i] = Rgb<size_t>(0);
#else
    //===========================
    //  Lokale Summations-Felder:
    //===========================
    //  h[p][z] beschreibt Folgewert in Bild `p' zum Hauptwert `z' im Ref-Bild
    //   `refpic'. Genullt per Default-Ctor von `ChSum'.
    DynArray2D <ChSum>  h_R (nlayers_, N_Grid),
                        h_G (nlayers_, N_Grid),
                        h_B (nlayers_, N_Grid);
    
    //  n[z] == Anzahl Pixel mit Hauptwert z. Explizit genullt.
    DynArray1D <Rgb<size_t> >  nn (N_Grid, Rgb<size_t>(0));
#endif    
    
    //cout << "sizeof() = " << sizeof(h_R) << '\n';      
    //cout << "sizeof() = " << sizeof(nn) << '\n';      

    for (int y=0; y < pack_.dim1(); y++)  // 2D view because can be subarrays!
    {
      for (int x=0; x < pack_.dim2(); x++)
      {
        //  Hauptwert:
        Rgb <uint8>  z = pack_[refpic] [y] [x];

        // Anzahl Pixel mit diesem z-Hauptwert feststellen; ist zugleich Anzahl
        //  natuerlich dann auch zugehoeriger Folgewerte. Ausserhalb der p-Schleife,
        //  da fuer alle Bilder identisch.
        nn[z.r].r ++;
        nn[z.g].g ++;
        nn[z.b].b ++;

        for (int p=0; p < nlayers_; p++)
        {
          if (p==refpic) continue;
          Rgb <uint8>  zf = pack_[p] [y] [x];

          h_R[p][z.r].s  += zf.r;
          h_R[p][z.r].s2 += (sum_t)zf.r * zf.r;

          h_G[p][z.g].s  += zf.g;
          h_G[p][z.g].s2 += (sum_t)zf.g * zf.g;

          h_B[p][z.b].s  += zf.b;
          h_B[p][z.b].s2 += (sum_t)zf.b * zf.b;
        }
      }
      TheProgressInfo::value ( float(y+1) / pack_.dim1() ); 
    }  
    //list_ChSum (h_R, 'R');
    //analyze_ChSum (h_R,'R');
    //analyze_ChSum (h_G,'G');
    //analyze_ChSum (h_B,'B');

    // Mittelwerte und Streuung berechnen und in Feld `follow_' eintragen.
    //  Note: Die nn[]-Daten sind fuer alle Bilder gleich (Verschwendung)
    //  Note: `follow' koennte durchaus auch Rgb-Feld hier sein.
    for (int p=0; p < nlayers_; p++)
    {
      if (p == refpic)           // Hauptwerte eintragen...
        for (unsigned i=0; i < N_Grid; i++) 
        {
          follow_[0][p][i].z  = i;        // R-channel
          follow_[0][p][i].n  = nn[i].r; 
          follow_[0][p][i].dz = 0;
          
          follow_[1][p][i].z  = i;        // G-channel
          follow_[1][p][i].n  = nn[i].g; 
          follow_[1][p][i].dz = 0;
          
          follow_[2][p][i].z  = i;        // B-channel
          follow_[2][p][i].n  = nn[i].b; 
          follow_[2][p][i].dz = 0;
        }
      else 
        for (unsigned i=0; i < N_Grid; i++)
        {
          int N;  
          double EX, EX2;  // Fuer Summe EX^2-EX*EX double, nicht float!
                           //  Auch nicht (u)llong, wegen Div durch N.
          N = nn[i].r;
          if (N) {
            EX  = double(h_R[p][i].s) / N;        // EX
            EX2 = double(h_R[p][i].s2) / N;       // E(X^2)
          } else 
            EX  = EX2 = 0;
          
          follow_[0][p][i].n  = N;
          follow_[0][p][i].z  = EX;
          follow_[0][p][i].dz = EX2 - EX*EX;      // DX^2 = E(X^2) - (EX)^2
        
          N = nn[i].g;
          if (N) {
            EX  = double(h_G[p][i].s) / N;
            EX2 = double(h_G[p][i].s2) / N;
          } else
            EX  = EX2 = 0;
          
          follow_[1][p][i].n  = N;
          follow_[1][p][i].z  = EX;
          follow_[1][p][i].dz = EX2 - EX*EX;
          
          N = nn[i].b;
          if (N) {
            EX  = double(h_B[p][i].s) / N;
            EX2 = double(h_B[p][i].s2) / N;
          } else
            EX  = EX2 = 0;
            
          follow_[2][p][i].n  = N;
          follow_[2][p][i].z  = EX;
          follow_[2][p][i].dz = EX2 - EX*EX;
        }
    }
    TheProgressInfo::finish();
}

/**+*************************************************************************\n
  get_CurvePlotdataX() - The common X-values of the plot curves: 0,1,...,255.
******************************************************************************/
Array1D <PlotValueT>                         
FollowUpValues_RGB_U8::get_CurvePlotdataX() const
{
    Array1D <PlotValueT>  X (N_Grid);    
    
    for (int i=0; i < X.dim(); i++)  
      X[i] = i;
    
    return X;  
}

/**+*************************************************************************\n
  get_CurvePlotdataY() - Plotkurve inkl. Streuung aus den \a ChannelFollowUpArray's.
******************************************************************************/
void 
FollowUpValues_RGB_U8::get_CurvePlotdataY (
      
      Array1D <PlotValueT> & Y,      // O: [n_]-Feld der Folgewerte (Ordinate)
      Array1D <PlotValueT> & YerrLo, // O: [n_]-Feld der unteren Streuung
      Array1D <PlotValueT> & YerrHi, // O: [n_]-Feld der obereren Streuung
      int                  pic,      // I: wanted picture, in [0..nlayers)
      int                  channel,  // I: wanted channel, in {0,1,2}
      bool                 bounded   // I: Wertgrenzen beruecksichtigen?
      )
    const
{
    DEBUG_COUT ("FollowUpValues_RGB_U8::" <<__func__ << "(pic=" << pic
         << ", channel=" << channel << ", bounded=" << bounded << ")...\n");
    ASSERT (0 <= pic && pic < nlayers_,   return);
    ASSERT (0 <= channel && channel <= 3, return);

    Y      = Array1D <PlotValueT> (N_Grid);   // allokiert
    YerrLo = Array1D <PlotValueT> (N_Grid);   // allokiert
    YerrHi = Array1D <PlotValueT> (N_Grid);   // allokiert

    const ExpectFollowUpValue* h = follow_[channel][pic];               
    PlotValueT z_max = z_max_;     // PlotValueT <-- uint8

    for (unsigned i=0; i < N_Grid; i++)
    {
      if (h[i].dz < 0) printf("i=%d, DZ^2 = %f < 0!\n", i, h[i].dz);
      PlotValueT s   = h[i].z;     // PlotValueT <-- sum_t
      PlotValueT ds  = 0.5 * sqrt(h[i].dz);     
      PlotValueT sLo = s-ds;
      PlotValueT sHi = s+ds;
      
      if (bounded)    // Schranken beruecksichtigen... 
      {
        if (sHi > z_max) {
          sLo -= (sHi - z_max);    // ...nach unten verschieben
          sHi = z_max;
        } 
        else if (sLo < 0.0) {
          sHi -= sLo;              // ...nach oben verschieben
          sLo = 0.0;
        }
      }
      Y[i]      = s;
      YerrLo[i] = sLo;       
      YerrHi[i] = sHi;
    }

#   ifdef BR_DEBUG_FOLLOWUP
    debug_write_CurvePlotdata (pic, channel, Y, YerrLo, YerrHi);
#   endif  
}


/**+*************************************************************************\n
  get_CurvePlotdataY() -  Plotkurve ohne Streuung aus den \a ChannelFollowUpArray's
******************************************************************************/
Array1D <PlotValueT>                         
FollowUpValues_RGB_U8::get_CurvePlotdataY (int pic, int channel) const
{
    DEBUG_COUT ("FollowUpValues_RGB_U8::" <<__func__<< "(pic=" << pic
         << ", channel=" << channel << ")...\n");
    
    IF_FAIL_RETURN (0 <= pic && pic < nlayers_,   Array1D<PlotValueT>());
    IF_FAIL_RETURN (0 <= channel && channel <= 3, Array1D<PlotValueT>());

    Array1D <PlotValueT>  Y (N_Grid);     // allokiert

    const ExpectFollowUpValue* h = follow_[channel][pic];;

    for (unsigned i=0; i < N_Grid; i++) 
      Y[i] = h[i].z;              // PlotValueT <-- sum_t

#   ifdef BR_DEBUG_FOLLOWUP
    debug_write_CurvePlotdata (pic, channel, Y);
#   endif  
        
    return Y;
}


/**+*************************************************************************\n
  write_CurvePlotdata()
******************************************************************************/
void
FollowUpValues_RGB_U8::write_CurvePlotdata (
      const char* fname, 
      int         pic, 
      int         channel, 
      bool        bounded 
      )  
    const
{
    DEBUG_COUT ("FollowUpValues_RGB_U8::" <<__func__ << "(pic=" << pic
         << ", channel=" << channel << ", bounded=" << bounded << ")...\n");
    
    //  Check to avoid abborts in get_CurvePlotdataY()...
    ASSERT(0 <= pic && pic < nlayers_,   return);
    ASSERT(0 <= channel && channel <= 3, return);

    Array1D <PlotValueT>  Y, YerrLo, YerrHi;

    get_CurvePlotdataY (Y, YerrLo, YerrHi, pic, channel, bounded);
    if (Y.is_empty()) return;         // provisional error handling

    FilePtr f (fname, "w+");          // Ueberschreibt ohne Warnung!

    for (int i=0; i < Y.dim(); i++)
    {  
      fprintf (f, "%d %f %f %f\n", i, Y[i], YerrLo[i], YerrHi[i]);
    }
}


/**+*************************************************************************\n
  get_RefpicHistogramPlotdata()  -  Return the histogram plotdata of z-values 
                                     of the reference picture.
 
  The histogram data are scaled to the intervall [scale_minval, scale_maxval].
   For axis ticking the *real* maximal histogram value (number 'n') is given
   in \a maxval.

  z-values (co-domain) \in [0..255] == [0..zmax], i.e. \a n_zvals values.
******************************************************************************/
Array1D <PlotValueT> 
FollowUpValues_RGB_U8::get_RefpicHistogramPlotdata (
      int        channel,
      size_t*    maxval,
      PlotValueT scale_minval, 
      PlotValueT scale_maxval ) const
{
    ASSERT (0 <= channel & channel < 3, return Array1D<PlotValueT>());

    //  The "refpic line" in `follow_'... (btw: all lines have the same 'n's)
    const ExpectFollowUpValue* line = follow_[channel][refpic_];

    //  Find the maximal value (n-entry)...
    size_t mava = line[0].n;
    for (unsigned i=1; i < N_Grid; i++)
      if (line[i].n > mava) mava = line[i].n;

    DEBUG_COUT ("max. histo value = " << mava << '\n');

    //  Scale domain [0..mava] to [scale_minval..scale_maxval]
    //  Note: mava==0 duerfte fuer !follow.is_empty() nie vorkommen, da die
    //   insgesamt n_pixel Werte ja irgendwo sein muessen.
    IF_FAIL_DO (mava>0, mava=1);
    PlotValueT fac = (scale_maxval - scale_minval) / mava;   

    DEBUG_COUT ("FollowUpValues_RGB_U8::" << __func__ 
                << "(): n_pixel=" << pack_.dim1() * pack_.dim2()
                << "  maxval=" << mava
                << "  scale_minval=" << scale_minval
                << "  scale_maxval=" << scale_maxval
                << "  fac = " << fac << '\n');

    Array1D <PlotValueT>  H (N_Grid); 
    for (unsigned i=0; i < N_Grid; i++)
      H[i] = fac * line[i].n + scale_minval;

    if (maxval) *maxval = mava;  
    return H;
}


//=============================
//  For internal usage only...
//=============================

void FollowUpValues_RGB_U8::debug_write_CurvePlotdata (
        int pic, 
        int channel, 
        const Array1D <PlotValueT> & Y,     
        const Array1D <PlotValueT> & YerrLo,
        const Array1D <PlotValueT> & YerrHi 
    ) 
    const
{
    char c = (channel==0) ? 'R' : (channel==1) ? 'G' : 'B';
    char fname[64];
    sprintf (fname, "follow_%d_%d_%c-042.dat", refpic_, pic, c);
     
    FilePtr f (fname, "w+");          // Ueberschreibt ohne Warnung!
    
    for (int i=0; i < Y.dim(); i++)
    {  
      fprintf (f, "%d %f %f %f\n", i, Y[i], YerrLo[i], YerrHi[i]);
    }
}


void FollowUpValues_RGB_U8::debug_write_CurvePlotdata (
      int pic, 
      int channel, 
      const Array1D <PlotValueT> & Y     
      )  
    const
{
    char c = (channel==0) ? 'R' : (channel==1) ? 'G' : 'B';
    char fname[64];
    sprintf (fname, "follow_%d_%d_%c-042.dat", refpic_, pic, c);
    
    FilePtr f (fname, "w+");          // Ueberschreibt ohne Warnung!
    
    for (int i=0; i < Y.dim(); i++) 
    {
      fprintf (f, "%d %f\n", i, Y[i]);
    }
}

/**+*************************************************************************\n
  `dz´ eigentlich dz^2; daher Wurzel gezogen wie in get_CurvePlotdata().
******************************************************************************/
void FollowUpValues_RGB_U8::list_ChannelArray (int channel) const
{
#if 0    
    cout <<__func__<< "( channel=" << channel << ")...\n";
    if (channel<0 || channel>2) return;
    const ChannelFollowUpArray & F = follow_[channel];
    for (int i=0; i < F.dim2(); i++) {
      cout << i << ": n=" << F[0][i].n;  // `n' is for all identic.
      for (int p=0; p < F.dim1(); p++)
        cout << "\t (z:" << F[p][i].z << "\t dz:" << sqrt(F[p][i].dz) << ")";
      cout << '\n';
    }
#else    
    printf("%s( channel=%d )...\n", __func__, channel);
    if (channel<0 || channel>2) return;
    const ChannelFollowUpArray & F = follow_[channel];
    for (int i=0; i < F.dim2(); i++) {
      printf("%3d: n=%3d", i, F[0][i].n);
      for (int p=0; p < F.dim1(); p++)
        printf("   (z:%5.1f  dz:%f)", F[p][i].z, sqrt(F[p][i].dz));
      printf("\n");
    }
#endif    
}

/**+*************************************************************************\n
  Liste ChSum-Summationsfeld 
******************************************************************************/
void FollowUpValues_RGB_U8::list_ChSum (const DynArray2D<ChSum>& h, char c) const
{
    cout << "List ChSum '" << c << "'...\n";
    for (int i=0; i < h.dim2(); i++) {
      cout << i << ":";
      for (int p=0; p < h.dim1(); p++)
        cout << "\t (s:" << h[p][i].s 
             << "\t s2:" << h[p][i].s2 << ")";
      cout << '\n';
    }
}

/**+*************************************************************************\n
  (Debug)Analyse lokal benutzter ChSum-Summationsfelder. 
******************************************************************************/
void FollowUpValues_RGB_U8::analyze_ChSum (const DynArray2D<ChSum>& h, char c) const
{
    cout << "Analyze ChSum '" << c << "': z > z_max? ...\n";
    for (int i=0; i < h.dim2(); i++) {
      bool out = false;
      for (int p=0; p < h.dim1(); p++) {
        if (!(h[p][i].s <= z_max_)) {   // z.B. auch 'nan' - Div. durch 0
          if (!out) {cout << "i=" << i << ',';  out=true;}
          cout << "  p=" << p << ": s=" << h[p][i].s;
        }
      }
      if (out) cout << '\n';
    }
    cout << "Analyze ChSum '" << c << "': s2 < 0? ...\n";
    for (int i=0; i < h.dim2(); i++) {
      bool out = false;
      for (int p=0; p < h.dim1(); p++) {
        if (h[p][i].s2 < 0.0) {
          if (!out) {cout << "i=" << i << ',';  out=true;}
          cout << "  p=" << p << ": s2=" << h[p][i].s2;
        }
      }
      if (out) cout << '\n';
    }
}

}  // namespace "br"

// END OF FILE

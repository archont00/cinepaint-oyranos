/*
 * CorrelMaxSearch_RGB.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file CorrelMaxSearch_RGB.hpp
   
  @internal
  Weil nicht speicherblockierend, die Klasse nicht "Finder" oder "Searcher"
   genannt, sondern "Search" - eine Taetigkeit, kein autonomer Agent.
  
  Die 2D-Felder werden durch Scheme2D<>, nicht mehr durch TNT::Array2D<> beschrieben.
   Vermeidet die Abhaengigkeit von TNT (Scheme2D elementarer).
   
  Allerdings ist Scheme2D<> nicht blockierend, blockieren tut erst ImgScheme2D<>,
   naemlich den Array1D<uchar>-Puffer von br::Image. Wenn nicht blockierend, durch
   die Trennung in Ctor mit Felduebergabe (Speicherbekanntgabe) und Elementfunktionen
   mit Feldzugriff (Speicherzugriff) stets die Gefahr, dass zwischen Ctor und
   Funktionsaufruf der Speicher extern freigegeben wird; - bei einer elementaren
   Funktion koennte das nicht passieren.
   Aber wir koennen einer solchen Klasse ja den Status einer "Kernroutine" geben,
   wo der Anwender wissen muss, was er tut. Der Vorteil einer Klasse gegen eine
   elementare Funktion ist (u.a.), dass Statusvariablen (Fehlerkodes etc.) oder
   Zusatzleistungen wie das Canyon-Feld bequem aufbewart und abgefragt werden
   koennen und weder in eine Return-Struktur noch in die Argumentliste einer
   Funktion hineingezwaengt werden muessen.
      
  Wenn intendiert als Kernroutine, sollten Bereichstest und Fehlermeldungen 
   sparsam gehalten werden.  
   
  Abk: LO = Links-Oben,
       RU = Rechts-Unten
       
  @todo Zur Aufnahme der Canyon-Daten wird derzeit ein TNT::Array2D benutzt.
   Irgendwann mal durch eine eigene ref-zaehlende RefArray2D-Klasse ersetzen ohne
   das Zusatzfeld der TNT::Array2D's fuer die Zeilenadressen.
   
  @internal Was man zur Geschw.steigerung noch versuchen koennte:
   - Fuer U8 evntl. Tabelle der 256 Quadrate; statt <pre>
       double a = arrays[...];
       sA += a;
       sA2 += a*a;             // Gleitkomma-Multipl. </pre>
     dann <pre>
       Unsign a = arrays[...];
       sA += a;
       sA2 += qdr[a];          // mit double-Feld qdr[256] </pre>
        
  @internal Vereinfachungsidee: Eine Routine, der nur zwei Scheme2D's der 
   Dimensionen (Nx,Ny) bzw. (Mx,My) uebergeben werden; die LO-Koordiante (xA,yA), 
   (xB,yB) werden als (0,0) genommen -- es entfallen in den Indexaufrufen die xA 
   und yA Addenten). Die bisherigen Routinen werden Huellen, die aus ihren Schemes
   A, B passende Subschemes fuer diese Funktion erstellen und uebergeben.
*/
#ifndef CorrelMaxSearch_RGB_hpp
#define CorrelMaxSearch_RGB_hpp


#include <cstdio>                    // printf()   
#include <cassert>                   // assert()
#include <cmath>                     // sqrt()        

#include "CorrelMaxSearchBase.hpp"   // CorrelMaxSearchBase
#include "Vec2.hpp"                  // Vec2_int
#include "Rgb.hpp"                   // Rgb<>
#include "Rgb_utils.hpp"             // <<-Op for Rgb<> (debug)
#include "Scheme2D.hpp"              // Scheme2D<>
#include "DynArray1D.hpp"            // DynArray1D<>
#include "TNT/tnt_stopwatch_.hpp"    // TNT::StopWatch 
#include "TNT/tnt_array2d.hpp"       // TNT::Array2D<>  (canyon feld)


/**  Define this for debug output */
//#define DEBUG_CORREL

/**  Define this to be verbose */
//#define VERBOSE_CORREL


#ifdef DEBUG_CORREL
#  include "correl_RGB.hpp"            // correl() template
#  include "TNT/tnt_array2d_utils.hpp" // <<-Op for TNT::Array2D<>
#endif


/**===========================================================================
  
  @class CorrelMaxSearch_RGB

  Seien gegeben zwei RGB-Felder \a A und \a B. Klasse sucht Maximum des
   Korrelationskoeffizienten eines [Nx,Ny]-Gebiets aus \a A mit LO-Punkt bei 
   \a (xA,yA) in einem [Mx,My]-Gebiet von \a B mit LO-Punkt bei \a (xB,yB),
   wobei Mx >= Nx, My >= Ny sein muss (Skizze). \a A und \a B gehen also nicht
   symmetrisch ein, von \a B wird ein groesserer Bereich genommen! <pre>
             xA                          xB            
    .--------+----------------.     .====+==============.
    |                         |   yB|-   LO---------.   |
  yA+        LO----.          |     |    |          |   |
    |        |_____|Ny        |     |    |          |My |
    |           Nx            |     |    |__________|   |
    |A________________________|     |         Mx        |
                                    |                   |
                                    |B__________________|
    +--------xA
        +----xB
    .--------+----------------.      Result: (x,y) - Position of the (Nx,Ny) area.
    |   .====+==============. |                       inside of the (Mx,My) area.
  yA+ yB+....LO----.----.   | |        .---+------.    
    |   |    |_____|Ny  |   | |        |  x,y----.|
    |   |    |  Nx      |My | |        |   |_____||My
    |   |    |____Mx____|   | |        |__________|
    |A__|___________________|_|             Mx
        |                   |
        |B__________________| </pre>
        
  Moeglicher Wertebereich des (x,y)-Resultats: [0,...,Mx-Nx] x [0,...,My-Ny].
  
  Das Resultat, die Verschiebung \a d, ist so zu interpretieren: Maximale Korrelation
   wird erreicht, wenn gegenueber der Ausgangslage A[i] <--> B[j] (beide LO-Punkte
   uebereinander) stattdessen die Werte A[i-d] <--> B[j] bzw. A[i] <--> B[j+d]
   uebereinander gelegt werden:<pre>
  (i)   3            3
   A -|-L-|---    -|-L-|---   d=+1, also nicht A[3] <-> B[3] sondern 
   B -|--L--|-   -|--L--|-                     A[3] <-> B[3+d] oder
  (j) 12345          4                       A[4-d] <-> B[3] </pre>
  
  @note Um dieselbe Suchsituation: jenes A-Gebiet variiert ueber jenem B-Gebiet,
   zu beschreiben, sind in search() und search_sym() unterschiedliche Koordinaten
   anzugeben, LO-Koordianten hier, Zentrumskoordinaten dort! Der errechnete
   Verschiebungsvektor \a d kann im ersten Fall nur positive Werte annehmen, im 
   zweiten auch negative. Anzuwenden ist er in gleicher Weise als Indexaddition bei
   B oder Subtraktion bei A, aber bezogen eben auf die verschiedene <i>A[i] <-> B[j]</i>
   Ausgangslagen.
   
  Die Endung "_pre" der Funktionsnamen bedeutet: mit Vortabellierung von B-Summen.
   Ohne "_pre" also ohne Vortabelierung. Davon scheint allerdings nur eine
   R-Kanal-Variante zu existieren. Frecherweise ist die vortabellierende search_r_pre()
   hoechstens unmerklich schneller als die einfache search_r(). Warum nur, es
   sind doch weniger Operationen???
   
  Die Endung "_sym" bedeutet, dass Gebietsdimensionen und Indizes fuer zwei
   symmetrisch zueinander positioniert gedachte Gebiete angegeben werden, siehe
   Skizze zu search_sym().
  
  Zur Aufbewahrung der correl_max-Daten ("Suchgebirge", "Canyon") genuegen
   float-Zahlen, da damit nicht gerechnet wird. Spart Speicher.
=============================================================================*/
template <typename Unsign>
class CorrelMaxSearch_RGB : public CorrelMaxSearchBase 
{
    Scheme2D <Rgb <Unsign> >    A,B;
  
  public:

    CorrelMaxSearch_RGB       ( const Scheme2D <Rgb<Unsign> >& A_,
                                const Scheme2D <Rgb<Unsign> >& B_ )
       : A(A_), B(B_)  {}
       
    //--------------------
    //  The both virtuals 
    //--------------------
    Vec2_int  search          ( int xA, int yA, int Nx, int Ny,
                                int xB, int yB, int Mx, int My );         
    
    Vec2_int  search_sym      ( int nx, int ny, int mx, int my,
                                int xcA, int ycA, int xcB, int ycB );
    
  private:
      
    Vec2_int  search_pre      ( int xA, int yA, int Nx, int Ny,
                                int xB, int yB, int Mx, int My );
};



/**+*************************************************************************\n
  search_pre()

  Theoretisch schnellere Variante mit vortabellierten B-Summen.
   Mit Rgb-Template statt explizitem Dreischritt wie in search_el_pre().

  NOTIZ: Wenn die aeussere Schleife ueber das [Mx,My]-Gebiet von oben nach
   unten (y-Dim) und die innere von links nach rechts (x-Dim) laeuft, muessen
   bei jedem neuen aeusseren Durchgang die Spaltensummen nach unten verschoben
   werden; als Zeilensummen genuegen hingegen die auf der linken Seite.
   Andersrum waere es andersrum. <pre>
    
        .-.---.-.------. |   |      .-----.--------.
        | |   | | ->   | Ny  |      |-----|        |
        |_|___|_|______| |   My     |_____|________|
        |     |        |     |      |_____|        |
        |_____|________|     |      |_____|________|
           Nx                           andersrum
        ----- Mx ------  </pre>

  @internal Operationen der Art <tt>double d = A[][] * B[][]</tt> mit den 
   Unsign-Feldern \a A und \a B sind vor Ueberlauf zwar sicher fuer \a uint8, denn
   da greift die implizite <tt>T-->int</tt>-Konvertierung fuer alle T = char, uchar,
   short und ushort. Doch sie sind nicht mehr sicher fuer unsigned shorts (ushort, uint16),
   denn auch die werden nur in \a ints (und nicht in \a uints!!) konvertiert,
   und das Produkt z.B. 65535 * 65535 uebersteigt den \a int Bereich! Daher vorher
   explizit in \a double konvertieren. Die Kosten sind bei beiden Varianten also: <pre>
     Unsign a,b
     double d = a * b;   // a-->int, b-->int, res=(a*b)_int, res->double
     double d = double(a) * b; // a-->double, b-->double, d = (a*b)_double.</pre>
   Sprich:
   - Oben: 2x Unsign-->int + int-Multipl + 1x int-->double
   - Unten: 2x Unsign->double + double-Multipl.
   
******************************************************************************/
template <typename Unsign>
Vec2_int
CorrelMaxSearch_RGB<Unsign>::search_pre (  int xA, int yA, int Nx, int Ny,
                                           int xB, int yB, int Mx, int My)
{
#ifdef VERBOSE_CORREL    
    printf ("\n%s()\n",__func__);
    printf ("interne Rgb<double>-Variablen und mit Rgb-Op\n");
    printf ("\tA: LO(x,y)=(%d,%d);  B: LO(x,y)=(%d,%d)\n", xA,yA, xB,yB);
    printf ("\tNx=%d, Ny=%d,  Mx=%d, My=%d\n", Nx,Ny,Mx,My);
    printf ("\tB-LO: x:[%d...%d] und y:[%d...%d]\n", xB,xB+Mx-Nx, yB,yB+My-Ny);
#endif    
    assert (xA + Nx <= A.dim2());
    assert (yA + Ny <= A.dim1());
    assert (xB + Mx <= B.dim2());
    assert (yB + My <= B.dim1());
    assert (Nx <= Mx);
    assert (Ny <= My);

    /*  Neuallokierung loescht alte Instanz autom., sofern nicht mehr referenziert.
         In Testphase Feldwerte vorbelegt mit unmoegl. Wert "-2.0". */
    canyon_ = TNT::Array2D <float> (My-Ny+1, Mx-Nx+1, -2.0);
    
    DynArray1D <Rgb<double> >  cols(Mx);  // fuer Mx vektorielle Spaltensummen
    DynArray1D <Rgb<double> >  rows(My);  // fuer My vektorielle Zeilensummen
    DynArray1D <double>  cols2(Mx);       // fuer Mx quadrierte Spaltensummen
    DynArray1D <double>  rows2(My);       // fuer My quadrierte Zeilensummen

    for (int p=0; p < Mx; p++)
    {
        Rgb<double> s(0.0);
        double s2(0.0);

        for (int i=0; i < Ny; i++)              // Spaltensumme
        {   Rgb<double> b = B [yB+i][xB+p];     // double <-- Unsign
            s  += b;
            s2 += b.length2();
        }
        cols [p] = s;   // vektoriell
        cols2[p] = s2;  // skalar
    }
    for (int p=0; p < My; p++)
    {
        Rgb<double> s(0.0);
        double s2(0.0);

        for (int i=0; i < Nx; i++)              // Zeilensumme
        {   Rgb<double> b = B [yB+p][xB+i];     // double <-- Unsign
            s  += b;
            s2 += b.length2();
        }
        rows [p] = s;   // vektoriell
        rows2[p] = s2;  // skalar
    }

    Vec2_int  d;
    double  rho_max = -2.0;    // kleiner als kleinstmoegl.

    // Fuer Ausgangslage alle Summen (sA,...,sB2,sAB) einmal vollstaendig
    //  berechnen. B-Summen dabei aus den Reihensummen oder Spaltensummen.

    Rgb<double> sA(0.0), sB(0.0);
    double      sA2(0.0), sB2(0.0), sAB(0.0);
                                    //    Oder auch so:
    for (int i=0; i < Ny; i++)      //    for (int i=0; i < Nx; i++)
    {   sB  += rows [i];            //    {   sB  += cols [i];
        sB2 += rows2[i];            //        sB2 += cols2[i];
    }                               //    }

    for (int i=0; i < Ny; i++)
    for (int j=0; j < Nx; j++)
    {
        Rgb<double> a = A[yA+i][xA+j];    // double <-- Unsign
        sA  += a;
        sA2 += a.length2();
        Rgb<double> b = B[yB+i][xB+j];    // double <-- Unsign
        sAB += dot(a,b);
    }

    int   n = Nx * Ny;
    //Rgb<double> EA = sA / (double)n;              // Mittelwert von A
    //Rgb<double> D2A = sA2 / (double)n - EA*EA;    // Streuung von A

#ifdef DEBUG_CORREL
    std::cout << "\tsA  = " << sA << '\n';
    std::cout << "\tsA2 = " << sA2 << '\n';
    //std::cout << "\tsB  = " << sB << '\n';
    //std::cout << "\tsB2 = " << sB2 << '\n';
    //std::cout << "\tE(A)= " << EA << '\n';0
    //std::cout << "\tD2A = " << D2A << '\n';
#endif

    Rgb<double> sB_0 = sB;        // B-Summen am linken Rand merken
    double      sB2_0 = sB2;

    for (int p=0; p <= My-Ny; p++)
    {
        //printf ("p=%d\n",p);
        if (p > 0)            // nach unten verschieben
        {
            // sB_0 = sB_0 + Zeile(p-1+Ny) - Zeile(p-1):
            sB_0  += rows [p-1+Ny] - rows [p-1];
            sB2_0 += rows2[p-1+Ny] - rows2[p-1];

            // alle Spaltensummen um 1 nach unten verschieben
            for (int i=0; i < Mx; i++)
            {   Rgb<double> b1 = B [yB+p-1   ][xB+i];    // zu subtrahieren
                Rgb<double> b2 = B [yB+p-1+Ny][xB+i];    // zu addieren
                cols [i] += b2 - b1;
                cols2[i] += b2.length2() - b1.length2();
            }
            sB  = sB_0;
            sB2 = sB2_0;

            // Korrelation ist stets komplett zu berechnen
            sAB = 0.0;
            for (int i=0; i < Ny; i++)
            for (int j=0; j < Nx; j++)
            {   Rgb<double> a = A[yA+i  ][xA+j];        // q=0
                Rgb<double> b = B[yB+i+p][xB+j];        // double <-- Unsign
                sAB += dot(a,b);  // koennte in int's bei ushort's ueberlaufen
            }
        }

        // Am linken Rand (q=0)
        double  cov, rho;
        cov = sAB - dot(sA,sB)/n;  // eigentlich ist das n*cov, nicht cov

        if (cov == 0.0)
             rho = 0.0;
        else rho = cov / sqrt( (sA2 - sA.length2()/n) * (sB2 - sB.length2()/n));

        canyon_[p][0] = rho;

        if (rho > rho_max)
        {   rho_max = rho;
            d.x = 0;
            d.y = p;
            //printf ("d_LO=(%d,%d),  rho=%.1f\n", d.x, d.y, rho_max);
        }
#ifdef DEBUG_CORREL
        printf ("\td_LO=(%d,%d), in B at (%d,%d)\n", 0, p, xB, yB+p);
        std::cout << "\tsB  = " << sB << '\n';
        std::cout << "\tsB2 = " << sB2 << '\n';
        std::cout << "\tsAB = " << sAB << '\n';
        std::cout << "\trho = " << rho << '\n';
        correl (Nx,Ny, A,xA,yA, B,xB,yB+p);
#endif
        
        // um q nach rechts verschieben
        for (int q=1; q <= Mx-Nx; q++)
        {
            // sB = sB + Spalte(q-1+Nx) - Spalte(q-1):
            sB  += cols [q-1+Nx] - cols [q-1];
            sB2 += cols2[q-1+Nx] - cols2[q-1];

            // Korrelation ist stets komplett zu berechnen
            sAB = 0.0;
            for (int i=0; i < Ny; i++)
            for (int j=0; j < Nx; j++)
            {   Rgb<double> a = A[yA+i  ][xA+j  ];    // double <-- Unsign
                Rgb<double> b = B[yB+i+p][xB+j+q];
                sAB += dot(a,b);  // koennte in int's fuer ushort's ueberlaufen
            }

            cov = sAB - dot(sA,sB)/n;   // eigentlich ist das n*cov, nicht cov
            
            if (cov == 0.0)
                 rho = 0.0;
            else rho = cov / sqrt( (sA2 - sA.length2()/n)*(sB2 - sB.length2()/n));

            canyon_[p][q] = rho;
            
            if (rho > rho_max)
            {   rho_max = rho;
                d.x = q;        // oder d = Vec2_int(q,p)
                d.y = p;
            }
#ifdef DEBUG_CORREL
            printf ("\td_LO=(%d,%d), in B at (%d,%d)\n", q,p, xB+q, yB+p);
            std::cout << "\tsB  = " << sB << '\n';
            std::cout << "\tsB2 = " << sB2 << '\n';
            std::cout << "\tsAB = " << sAB << '\n';
            std::cout << "\trho = " << rho << '\n';
            correl (Nx,Ny, A,xA,yA, B,xB+q,yB+p);
#endif
        }
    }
    rho_max_ = rho_max;
    eval_result (d);
    
#ifdef VERBOSE_CORREL    
    printf ("\td_LO=(%d,%d), in B at (%d,%d), rho= %f, min.grad= %f, auf_Rand= %d\n",
        d.x, d.y, xB+d.x, yB+d.y, rho_max, min_gradient_, border_reached_);
    //std::cout << canyon_;
#endif
    
    return d;
}

/**+*************************************************************************\n
  The official search() routine. Wrapper for the various probe routines, 
   selects one of these (the best).
  
  @param xA,yA: Lage des LO-Punktes im Bild \a A.
  @param Nx,Ny: Dimensionen des Korrelations-Rechtecks
  @param xB,yB: Lage des LO-Punktes im Bild \a B.
  @param Mx,My: Dimensionen des rechteckigen Suchgebiets im Bild \a B.
  @return Verschiebung des LO-Punktes von A zu dem von B, sprich, Lage das A-LO
     im [0...Mx-1] x [0...My-1]-Rechteck (stets >= 0), siehe Skizze.
******************************************************************************/
template <typename Unsign>
Vec2_int
CorrelMaxSearch_RGB<Unsign>::search     ( int xA, int yA, int Nx, int Ny,
                                          int xB, int yB, int Mx, int My )
{
    //return search_ela_pre (xA,yA, Nx,Ny, xB,yB, Mx,My);
    return search_pre (xA,yA, Nx,Ny, xB,yB, Mx,My);
}                                          


/**+*************************************************************************\n
  search_sym():

  Wrapper fuer search()-Funktionen, bei der symmetrisch zueinander positionierte
   Gebiete angenommen werden, und "symmetrische" Koordinaten zu uebergeben sind.
   Zur Zeit aussdem missbraucht zu Testzwecken.
      
  @note Um dieselbe Suchsituation zu beschreiben, sind gegenueber search()
   andere Koordinaten anzugeben! Der errechnete Verschiebungsvektor \a d ist in
   gleicher Weise anzuwenden als Indexaddition bei B oder Subtraktion bei A, 
   aber bezogen auf eben verschiedene <i>A[i] <-> B[j]</i> Ausgangslagen! 
  
  Im Gegensatz zu search() werden hier nicht LO-Punkte, sondern die Mittelpunkte
   der Gebiete angegeben (-> "c" for "center"). Diese werden hier in LO-Punkte
   umgerechnet und damit search() aufgerufen. Der Spiegelsymmetrie gemaess werden
   auch die Gebietsgroessen durch die "halbe" Weite vom Zentrum, durch die
   Pixeldistanzen (nx,ny) und (mx,my) (->Skizze!) angegeben. Die Dimensionen
   (Nx,Ny), (Mx,My) im Sinne search() sind dadurch stets ungerade.
  VORSICHT Bezeichnerfalle: Wohl ist \a nx die "halbe" Breite von \a Nx - modulo
   der "+1" fuer den Zentrumspunkt, aber \a mx ist nicht die halbe Breite zu \a Mx,
   es ist vielmehr die Suchdistanz, die "Haelfte" von Mx-Nx, siehe Skizze! Im Angesicht
   dieser anderen Bedeutungen ist die gegenueber search() geaenderte Argumentreihenfolge
   strikt zu befuerworten.
   Aber auch hier gilt: \a A und \a B gehen nicht symmetrisch ein! Von \a B wird das
   groessere Gebiet verlangt, oder anders gesagt, der Wertebereich fuer \a (xcB,ycB)
   ist limitierter als der fuer \a (xcA,ycA). <pre>
      m=3   n=3   
    |- - -|- - -(c)- - -|- - -|  ("-" sind die Pixel, Trenner "|" nur zur Deutlichkeit)
     xB    xA   xcA              xA = xcA-n
                xcB              xB = xcB-(n+m) 
                                 Nx = 2*nx + 1
                                 Mx = 2*(nx+mx) + 1
    Erlaubte Werte fuer xcA:  nx <= xcA <= dimA-1 - nx
    Erlaubte Werte fuer xcB:  nx+mx <= xcB <= dimB-1 - (mx+nx) 
                                 
                xcA                           xcB       
     .-----------+-------------.     .---------+---------.
     |        .--+--.          |     |   .=====+=====.   |
     |        |  .  |          |     |   |           |   |
     |        |_____|          |     |   |     .     |   |
     |                         |     |   |           |   |
     |A________________________|     |   |___________|   |
                                     |                   |
                                     |B__________________|
               xcA,xcB       
        .---------+---------.
     .--+---.=====+=====.---+--.
     |  |   |  .--+--.  |   |  |
     |  |   |  |  .  |  |   |  |
     |  |   |  |_____|  |   |  |
     |  |   |___________|   |  |
     |A_|___________________|__|
        |B__________________|     </pre>
  
  @return Verschiebung des Mittelpunktes von \a A gegen den von \a B (<0,=0,>0). 
    (Das denkt \a B fest, und \a A beweglich.) Das Ergebnis \a d bedeutet: Gegenueber
    der Ausgangslage A[i] <--> B[j] (beider Gebietszentren uebereinander) ist fuer
    maximale Korrelation A[i-d] <--> B[j] bzw. A[i] <--> B[j+d] uebereinander zu
    legen. Skizze: <pre>
  (i)   3
   A --|L--|--    --|L--|--   d=+1, also nicht A[3] <-> B[3] sondern 
   B -|--L--|-   -|--L--|-                     A[3] <-> B[3+d] oder
  (j) 12345                                  A[4-d] <-> B[4] </pre>
******************************************************************************/
template <typename Unsign>
Vec2_int
CorrelMaxSearch_RGB<Unsign>::search_sym (    int nx, int ny, int mx, int my,
                                             int xcA, int ycA, 
                                             int xcB, int ycB)
{
    int Nx = 2*nx + 1;         // Dim. der korrelierten Ausschnitte
    int Ny = 2*ny + 1;
    int Mx = 2*(nx+mx) + 1;    // Dim. des zu durchgrasenden B-Gebiets
    int My = 2*(ny+my) + 1;
    int xA = xcA - nx;         // PO-Koord. des [Nx,Ny]-Gebiets in A
    int yA = ycA - ny;
    int xB = xcB - (nx+mx);    // PO-Koord. des [Mx,My]-Gebiets in B 
    int yB = ycB - (ny+my);
    
    Vec2_int  d = search_pre (xA,yA, Nx,Ny, xB,yB, Mx,My);
    
    d.x -= mx;
    d.y -= my;

#  if VERBOSE_CORREL
     printf ("Zentrumsshift d_sym = (%d,%d)\n", d.x, d.y);
#  endif            
    
    return d;
}


#endif // CorrelMaxSearch_RGB_hpp

// END OF FILE

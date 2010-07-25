/*
 * correl_RGB.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  correl_RGB.hpp
  
  Templates for computation of the correlation coefficient between Rgb arrays.
  
  @internal Vereinfachungsidee: Zuunterst eine Routine, die nur zwei Scheme2D's 
   identischer Dimension (Nx,Ny) als Argument besitzt; die LO-Koordinaten (xA,yA),
   (xB,yB) werden als (0,0) genommen. Die bisherigen Routinen werden Huellen, die
   aus ihren Schemes A, B passende Subschemes fuer diese Funktion erstellen und
   uebergeben.
*/
#ifndef correl_RGB_hpp__
#define correl_RGB_hpp__

#include <cmath>                // sqrt()
#include "Rgb.hpp"              // Rgb<>
#include "Scheme2D.hpp"         // Scheme2D<>


/*!  Define this for debug output */
//#define DEBUG_CORREL

#ifdef DEBUG_CORREL
#  include <cassert>
#  include <cstdio>
#  include "Rgb_utils.hpp"      // <<-Op for Rgb<>
#endif

/*!  Define this to compile also the probe functions */
//#define CORREL_PROBE


/**  Deklaration der in dieser Datei definierten Templates */
template <typename Unsign>
double
correl        ( int Nx, int Ny,
                const Scheme2D <Rgb <Unsign> >& A, int xA, int yA,
                const Scheme2D <Rgb <Unsign> >& B, int xB, int yB );

template <typename Unsign>
Rgb<double>
correl_spec   ( int Nx, int Ny,
                const Scheme2D <Rgb <Unsign> >& A, int xA, int yA,
                const Scheme2D <Rgb <Unsign> >& B, int xB, int yB );

template <typename Unsign>
double
correl_mono_  ( int Nx, int Ny, 
                const Unsign* A, int dim2A, int xA, int yA, int stepA,
                const Unsign* B, int dim2B, int xB, int yB, int stepB );

template <typename Unsign>
double
correl_mono   ( int Nx, int Ny,
                const Scheme2D <Rgb <Unsign> >& A, int xA, int yA, int channelA,
                const Scheme2D <Rgb <Unsign> >& B, int xB, int yB, int channelB );

template <typename Unsign>
double
correl_mono   ( int channel, int Nx, int Ny,
                const Scheme2D <Rgb <Unsign> >& A, int xA, int yA,
                const Scheme2D <Rgb <Unsign> >& B, int xB, int yB );

template <typename Unsign>
double
correl_sum    ( int Nx, int Ny,
                const Scheme2D <Rgb <Unsign> >& A, int xA, int yA,
                const Scheme2D <Rgb <Unsign> >& B, int xB, int yB );


/**+*************************************************************************\n
  Berechnet den (skalaren) Korrelationskoeffizienten der Farbvektoren zweier 
   [Nx,Ny]-Ausschnitte aus den Rgb-Feldern \a A und \a B mit den LO-Punkten bei
   (xA,yA) und (xB,yB), siehe die Skizze sowie die nachfolgenden Erlaeuterungen. <pre>
             xA                    xB            
    .--------+----------.      .---+----------.
    |                   |      |              |
  yA+        LO----.    |      |              |
    |        |_____|Ny  |    yB+   LO----.    |
    |           Nx      |      |   |_____|Ny  |
    |A__________________|      |      Nx      |
                               |B_____________| </pre>

 <h4>Korrelationskoeffizient \a rho(X,Y):</h4> <pre>
                E[ (X-EX)(Y-EY) ]       E(XY) - (EX)(EY)
    rho(X,Y) = -------------------  =  -------------------
               sqrt( D2(X) D2(Y) )     sqrt( D2(X) D2(Y) ) </pre>
  wobei<pre>
    D2(X) :=  E[(X - EX)^2]  =  E(X^2) - (EX)^2 </pre>
    
  die mittlere quadratische Abweichung und \c EX der Mittelwert-Operator <pre>
    EX := 1/n Sum_i { x_i } . </pre>
  
  Statt X und Y schreiben wir dann A und B. Wir brauchen also die Summen
   - sA  := Sum_i { a_i }
   - sB  := Sum_i { b_i }
   - sA2 := Sum_i { (a_i)^2 }
   - sB2 := Sum_i { (b_i)^2 }
   - sAB := Sum_i { a_i*b_i }
   
  an den Positionen
     <pre>
                sAB - sA*sB
       --------------------------------- .
       sqrt[ (sA2 - sA^2)*(sB2 - sB^2) ] </pre>
      
  Zu jeder Summe gehoert der Faktor \c 1/n der reziproken Elementanzahl, der einmal
   gekuerzt werden kann, <pre>
                 1 - 1/n
       --------------------------- ,
       sqrt[ (1 - 1/n)*(1 - 1/n) ] </pre>
   
   so dass nur die Terme \c sA*sB, \c sA^2 und \c sB^2 damit multipliziert
    werden muessen.
    
  Sind die Groesse X,Y Vektoren, wie hier dreikomponentige RGB-Farben, kann man 
   entweder den Korrelationskoeffizienten fuer jeden Kanal separat berechnen.
   Das Ergebnis ist dann ein 3er-Vektor (spektraler Korrelationskoeffizient).
   Das tut correl_spec(). Fuer einen \a skalaren Ergebniswert kann man die Produkte
   XY, X^2, Y^2 als Skalarprodukte <i>dot(X,Y), dot(X,X), dot(Y,Y)</i> auffassen,
   also etwa <pre>
     E(XY) = 1/n sum_i { X_i DOT Y_i } 
           = 1/n sum_i { X[0]_i*Y[0]_i + X[1]_i*Y[1]_i + X[2]_i*Y[2]_i } </pre>
   EX und EY sind als Vektoren aufzusummieren (die mittleren Farbvektoren
   der beiden Gebiete), waehrend E(X^2), E(Y^2), E(XY) skalare Summen darstellen.
   Zum Schluss sind fuer die Bildungen (EX)^2, (EY)^2 und (EX)(EY) die gemittelten
   Vektoren skalar zu multiplizieren. Oder in "AB"-Bezeichnungsweise: Die Summen
   sAB, sA2, sB2 sind Skalare und die Summen sA, sB sind Vektoren (Rgb's). Dies
   tut diese Funktion.

******************************************************************************/
template <typename Unsign>
double
correl        ( int Nx, int Ny,
                const Scheme2D <Rgb <Unsign> >& A, int xA, int yA,
                const Scheme2D <Rgb <Unsign> >& B, int xB, int yB )
{
#  ifdef DEBUG_CORREL
    printf ("\n%s():\n", __func__);
#  endif
    
    Rgb<double> sA(0.0), sB(0.0);
    double sA2=0.0, sB2=0.0, sAB=0.0;

    for (int i=0; i < Ny; i++)
    for (int j=0; j < Nx; j++)
    {
        Rgb<double> a = A [yA+i][xA+j];   // double <-- Unsign, daher Ref unmoegl.
        sA  += a;
        sA2 += a.length2();
        Rgb<double> b = B [yB+i][xB+j];   // double <-- Unsign, daher Ref unmoegl.
        sB  += b;
        sB2 += b.length2();
        sAB += dot(a,b);
    }
    double n = Nx * Ny;              // `n' nur als double gebraucht
    double cov = sAB - dot(sA,sB)/n; // eigentlich ist das n*cov, nicht cov[arianz]
    double rho;
    if (cov == 0.0)           // bedient das auch den Fall "0/0"?
         rho = 0.0;
    else rho = cov / sqrt( (sA2 - sA.length2()/n) * (sB2 - sB.length2()/n) );

#  ifdef DEBUG_CORREL
    std::cout << "\tsA  = " << sA  << '\n';
    std::cout << "\tsA2 = " << sA2 << '\n';
    std::cout << "\tsB  = " << sB  << '\n';
    std::cout << "\tsB2 = " << sB2 << '\n';
    std::cout << "\tsAB = " << sAB << '\n';
    std::cout << "\tcov = " << cov / (double)n << '\n';
    std::cout << "\trho = " << rho << '\n';
#  endif

    return rho;
}

/**+*************************************************************************\n
  Berechnet fuer alle drei Kanaele separat (spektral) den Korrelationskoeff. zweier
   [Nx,Ny]-Ausschnitte aus den Rgb-Feldern \a A und \a B mit den LO-Punkten bei
   (xA,yA) und (xB,yB) --> Skizze zu correl().
  @return Rgb-Struktur der Korrelationskoeffizienten der drei Kanaele.
******************************************************************************/
template <typename Unsign>
Rgb<double>
correl_spec   ( int Nx, int Ny,
                const Scheme2D <Rgb <Unsign> >& A, int xA, int yA,
                const Scheme2D <Rgb <Unsign> >& B, int xB, int yB )
{
#  ifdef DEBUG_CORREL
    printf ("\n%s():\n", __func__);
#  endif
    
    Rgb<double> sA(0.0), sB(0.0), sA2(0.0), sB2(0.0), sAB(0.0);

    for (int i=0; i < Ny; i++)
    for (int j=0; j < Nx; j++)
    {
        Rgb<double> a = A [yA+i][xA+j];   // double <-- Unsign, daher Ref unmoegl.
        sA  += a;
        sA2 += a*a;
        Rgb<double> b = B [yB+i][xB+j];   // double <-- Unsign, daher Ref unmoegl.
        sB  += b;
        sB2 += b*b;
        sAB += a * b;
    }
    double n = Nx * Ny;                   // `n' nur als double gebraucht
    Rgb<double> cov = sAB - sA*sB/n;      // eigentlich ist das n*cov, nicht cov[arianz]
    Rgb<double> rho;
    if (cov.r == 0.0)           // bedient das auch den Fall "0/0"?
         rho.r = 0.0;
    else rho.r = cov.r / sqrt((sA2.r - sA.r*sA.r/n)*(sB2.r - sB.r*sB.r/n));

    if (cov.g == 0.0)
         rho.g = 0.0;
    else rho.g = cov.g / sqrt((sA2.g - sA.g*sA.g/n)*(sB2.g - sB.g*sB.g/n));

    if (cov.b == 0.0)
         rho.b = 0.0;
    else rho.b = cov.b / sqrt((sA2.b - sA.b*sA.b/n) * (sB2.b - sB.b*sB.b/n));

#  ifdef DEBUG_CORREL
    std::cout << "\tsA  = " << sA  << '\n';
    std::cout << "\tsA2 = " << sA2 << '\n';
    std::cout << "\tsB  = " << sB  << '\n';
    std::cout << "\tsB2 = " << sB2 << '\n';
    std::cout << "\tsAB = " << sAB << '\n';
    std::cout << "\tcov = " << cov / (double)n << '\n';
    std::cout << "\trho = " << rho << '\n';
#  endif

    return rho;
}

/**+*************************************************************************\n
  correl_mono()
    
  Berechnet fuer Kanal \a channel Korrelationskoeff. zweier [Nx,Ny]-Ausschnitte
   aus den Rgb-Feldern \a A und \a B mit den LO-Punkten bei (xA,yA) und (xB,yB)
   --> Skizze bei correl().
  
  @internal Eine optimierte interne Loesung selektiert den Kanal nicht ueber einen
   Indexaufruf "[channel]", sondern ueber eine passend gewaehlte Startadresse
   des uebergebenen Feldes --> correl_mono_(). 
******************************************************************************/
template <typename Unsign>
double
correl_mono (   int channel, int Nx, int Ny,
                const Scheme2D <Rgb <Unsign> >& A, int xA, int yA,
                const Scheme2D <Rgb <Unsign> >& B, int xB, int yB )
{
#  ifdef DEBUG_CORREL
    printf ("\n%s [channel #%d]:\n",__func__, channel );
    assert (0 <= channel && channel < 3);
#  endif
    
    /*  Verschieben Startaddressen um `channel' Unsign's */
    return  correl_mono_<Unsign> ( Nx,Ny,  
                (Unsign*)(A.data()) + channel, A.stride(), xA,yA, 3,
                (Unsign*)(B.data()) + channel, B.stride(), xB,yB, 3 );
}


/**+*************************************************************************\n
  Korreliert Kanal \a channelA des Rgb-Feldes \a A mit Kanal \a channelB des 
   Rgb-Feldes \a B in einem [Nx,Ny]-Gebiet, wobei (xA,yA) die LO-Indizies des
   A-Gebietes und (xB,yB) die LO-Indizes des B-Gebietes --> Skizze bei correl().
******************************************************************************/
template <typename Unsign>
double
correl_mono (   int Nx, int Ny,
                const Scheme2D <Rgb <Unsign> >& A, int xA, int yA, int channelA,
                const Scheme2D <Rgb <Unsign> >& B, int xB, int yB, int channelB )
{
#  ifdef DEBUG_CORREL
    printf ("\n%s [channels #%d <-> #%d]:\n",__func__, channelA, channelB );
    assert (0 <= channelA && channelA < 3);
    assert (0 <= channelB && channelB < 3);
#  endif
    
    /*  Verschieben Startaddressen um `channel' Unsign's */
    return  correl_mono_<Unsign> ( Nx,Ny,  
                (Unsign*)(A.data()) + channelA, A.stride(), xA,yA, 3,
                (Unsign*)(B.data()) + channelB, B.stride(), xB,yB, 3 );
}                


#ifdef CORREL_PROBE
/**+*************************************************************************\n
  Zu Testzwecken diese Funktion mit expliziten Scheme2D-Index-Aufrufen.
******************************************************************************/
template <typename Unsign>
double
correl_mono_probe ( int Nx, int Ny,
                const Scheme2D <Rgb <Unsign> >& A, int xA, int yA, int channelA,
                const Scheme2D <Rgb <Unsign> >& B, int xB, int yB, int channelB )
{
#  ifdef DEBUG_CORREL
    printf ("\n%s [channels #%d <-> #%d]:\n",__func__, channelA, channelB );
    assert (0 <= channelA && channelA < 3);
    assert (0 <= channelB && channelB < 3);
#  endif

    double sA=0.0, sB=0.0, sA2=0.0, sB2=0.0, sAB=0.0;

    for (int i=0; i < Ny; i++)
    for (int j=0; j < Nx; j++)
    {
        double a = A [yA+i][xA+j][channelA];
        sA  += a;
        sA2 += a*a;
        double b = B [yB+i][xB+j][channelB];
        sB  += b;
        sB2 += b*b;
        sAB += a * b;
    }
    double n = Nx * Ny;          // gebraucht nur als double
    double cov = sAB - sA*sB/n;  // eigentlich ist das n*cov, nicht cov
    double rho;
    if (cov == 0.0)
         rho = 0.0; 
    else rho = cov / sqrt( (sA2 - sA*sA/n) * (sB2 - sB*sB/n) );

#  ifdef DEBUG_CORREL
    //printf ("\tE(A) = %f, D^2(A) = %f\n", sA/n, sA2/n-(sA/n)*(sA/n));
    //printf ("\tE(B) = %f, D^2(B) = %f\n", sB/n, sB2/n-(sB/n)*(sB/n));
    printf ("\tsA = %f, sA2 = %f\n", sA, sA2);
    printf ("\tsB = %f, sB2 = %f,  sAB = %f\n", sB, sB2, sAB);
    printf ("\tcov(A,B) = %f, rho(AB) = %f\n", cov/n, rho);
#  endif

    return rho;
}
#endif  // CORREL_PROBE


/**+*************************************************************************\n
  Hilfsfunktion (template) fuer correl_mono().
  
  Gegeben zwei 2D-Felder \a A und \a B, deren Elemente aus je \a stepA \c Unsign
   bzw. \a stepB \c Unsign Objekten bestehen (z.B. RGB aus drei U16).
   Funktion berechnet fuer zwei [Nx,Ny]-Auschnitte aus \a A und \a B den
   Korrelationskoeffizienten bzgl. <b>eines Kanals</b>, wobei (xA,yA) die
   LO-Indizes des A-Gebietes und (xB,yB) die LO-Indizes des B-Gebietes. Die
   Felder werden gegeben durch ihre Startadressen \a A, \a B und ihre "zweite"
   Dimension (Spaltenzahl) \a strideA, \a strideB (real number of \c Unsign elements
   in memory from row to row). Die Parameter \a stepA und \a stepB koennen frei
   gewaehlt werden, durch step=4 koennte ein alpha-Kanal beruecksichtigt
   (uebersprungen) werden (sofern alpha ebenfalls Unsign-Groesse besitzt). Durch
   passendes Ansetzen der Startadressen laesst sich der R-, G- oder B-Kanal auswaehlen.
   Auch verschiedene Kanaele aus \a A und \a B koennen so korreliert werden.
   
  Bei den Feldern \a A und \a B darf es sich auch um Subarrays aus einem groesseren
   Block handeln, es muessen als \a strideA und \a strideB nur die zugehoerigen
   "wahren" zweiten Dimensionen angegeben werden. Ein Scheme2D<> z.B. sollte
   stride(), nicht dim2(), uebergeben, es koennte ein Subarray sein!

  Templateparameter \a Unsign aus reiner Bildergewohnheit so genannt, doch ist
   Template nicht auf Unsign's oder integer-Typen beschraenkt, es koennten auch
   float- oder double-Zahlen sein (wobei bei letzteren das Umkopieren ueberfluessiger 
   Extraaufwand), es wird nur vorausgesetzt, dass die Konvertierung in double 
   moeglich und sinnvoll ist. "ToDouble" passender?
    
  @internal Koennte auch auf \a xA=yA=xB=yB=0 vereinfacht werden, und rufende
   Funktion muesste fuer richtige Startadressesn \a A und \b B sorgen.
******************************************************************************/
template <typename Unsign>
double
correl_mono_ (  int Nx, int Ny, 
                const Unsign* A, int strideA, int xA, int yA, int stepA,
                const Unsign* B, int strideB, int xB, int yB, int stepB )
{
#  ifdef DEBUG_CORREL
    printf("%s()\n",__func__);
#  endif    
    double sA=0.0, sB=0.0, sA2=0.0, sB2=0.0, sAB=0.0;
    int line_cr_A = (strideA - Nx) * stepA;   // "Zeilenvorschub"
    int line_cr_B = (strideB - Nx) * stepB;   // "Zeilenvorschub"
    //const Unsign* pA = &A[(yA * strideA + xA) * stepA]; // besser, falls int != Ptr
    //const Unsign* pB = &B[(yB * strideB + xB) * stepB];
    const Unsign* pA = A + (yA * strideA + xA) * stepA;
    const Unsign* pB = B + (yB * strideB + xB) * stepB;
    for (int i=Ny; i; i--)  
    {
        for (int j=Nx; j; j--)  
        {
            double a = *pA;   // double <-- Unsign
            sA  += a;
            sA2 += a*a;
            double b = *pB;   // double <-- Unsign
            sB  += b;
            sB2 += b*b;
            sAB += a * b;
            pA += stepA;
            pB += stepB;
        }
        pA += line_cr_A;
        pB += line_cr_B;
    }
    double n = Nx * Ny;
    double cov = sAB - sA*sB/n;  // eigentlich ist das n*cov, nicht cov
    double rho;
    if (cov == 0.0)
         rho = 0.0; 
    else rho = cov / sqrt( (sA2 - sA*sA/n) * (sB2 - sB*sB/n) );

#  ifdef DEBUG_CORREL
    //printf ("\tE(A) = %f, D^2(A) = %f\n", sA/n, sA2/n-(sA/n)*(sA/n));
    //printf ("\tE(B) = %f, D^2(B) = %f\n", sB/n, sB2/n-(sB/n)*(sB/n));
    printf ("\tsA = %f, sA2 = %f\n", sA, sA2);
    printf ("\tsB = %f, sB2 = %f,  sAB = %f\n", sB, sB2, sAB);
    printf ("\tcov(A,B) = %f, rho(AB) = %f\n", cov/n, rho);
#  endif
    
    return rho;
}

/**+*************************************************************************\n
  Berechnet skalaren Korrelationskoeffizienten zweier (Nx,Ny)-Auschnitte aus
   zwei Rgb-Feldern (Bildern), wobei als 'Wert' am Ort [i,j] die <b>Summe</b> R+G+B 
   der drei Kanalwerte genommmen wird. Da ein Faktor 1/3 irrelevant, weil sich
   wegkuerzend, also soviel wie der Korrelationskoeffizient der Kanalmittelwerte
   (nicht: Mittelwert der Kanalkorrelationskoeffizienten!). Zum Begriff 
   \a Korrelationskoeffizient siehe correl().
  @return skalarer Korrelationskoeffizient der gemittelten Kanalwerte.
******************************************************************************/
template <typename Unsign>
double
correl_sum (    int Nx, int Ny,
                const Scheme2D <Rgb <Unsign> >& A, int xA, int yA,
                const Scheme2D <Rgb <Unsign> >& B, int xB, int yB )
{
#  ifdef DEBUG_CORREL
    printf ("\n%s():\n", __func__);
#  endif
    
    double sA=0.0, sB=0.0, sA2=0.0, sB2=0.0, sAB=0.0;

    for (int i=0; i < Ny; i++)
    for (int j=0; j < Nx; j++)
    {
        Rgb<double> a = A [yA+i][xA+j];    // double <-- Unsign, daher Ref unmoegl.
        a[0] += a[1];   // Summe in a[0]
        a[0] += a[2];  
        sA  += a[0];           
        sA2 += (a[0] * a[0]);
        Rgb<double> b = B [yB+i][xB+j];    // double <-- Unsign, daher Ref unmoegl.
        b[0] += b[1];   // Summe in b[0]
        b[0] += b[2];
        sB  += b[0];
        sB2 += (b[0] * b[0]);
        sAB += (a[0] * b[0]);
    }
    double n = Nx * Ny;           // `n' nur als double gebraucht!
    double cov = sAB - sA*sB/n;   // Eigentlich ist das n*cov, nicht cov[arianz]
    double rho;
    if (cov == 0.0)
         rho = 0.0;
    else rho = cov / sqrt( (sA2 - sA*sA/n) * (sB2 - sB*sB/n) );

#  ifdef DEBUG_CORREL
    std::cout << "\tsA  = " << sA  << '\n';
    std::cout << "\tsA2 = " << sA2 << '\n';
    std::cout << "\tsB  = " << sB  << '\n';
    std::cout << "\tsB2 = " << sB2 << '\n';
    std::cout << "\tsAB = " << sAB << '\n';
    std::cout << "\tcov = " << cov / (double)n << '\n';
    std::cout << "\trho = " << rho << '\n';
#  endif

    return rho;
}


#endif  // correl_RGB_hpp__

// END OF FILE

/*
 * DisplcmFinderCorrel_.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  DisplcmFinderCorrel_.cpp
*/

#include <cmath>                        // sqrt()

#include "DisplcmFinderCorrel_.hpp"
#include "Vec2_utils.hpp"               // <<-op for Vec2<>
#include "DynArray1D.hpp"               // DynArray1D<>
#include "TNT/tnt_stopwatch_.hpp"


using std::cout;

//#define DEBUG_COUT(s)  std::cout << s;
#define DEBUG_COUT(s)

#define VERBOSE_PRINTF(args)  if (params_.verbose) printf args ;
#define VERBOSE_COUT(s)       if (params_.verbose) std::cout << s ;
#define VERBOSE1_COUT(s)      if (params_.verbose > 1) std::cout << s ;


/*!
  Lokale Hilfsstruktur zur Sammlung einiger Durchlauf-Infos.
*/
struct AreaInfo {
    Vec2_int  d;
    float     rho_max;
    float     min_grad;
    bool      border;
    bool      finished;
    bool      dismissed; 
    bool      rho_too_low;
    
    AreaInfo() : d(0), finished(false), dismissed(false) {}
    void finish()               {finished = true;}
    void dismiss()              {dismissed = finished = true;}
    void low_rho()              {rho_too_low = dismissed = finished = true;}
};


/**+*************************************************************************\n
  Constructor
******************************************************************************/
DisplcmFinderCorrel_::DisplcmFinderCorrel_()
  :
    d_result_ (0),
    error_    (0)
{
    params_ = init_params_;
}



/**+*************************************************************************\n
  @note Wir unterstellen derzeit A und B von gleicher Dimension und beruecksichtigen
   keinen externen Anfangs-Versatz!
  
  <pre>
  xB AinB
  *--|--------------*      B in Ur-Lage; Startgroesse M0
     *-------*             Ur-Lage von A in B (origAinB); Startgroesse N0
     |      *---+---*      A-Lage in B nach 1. Run (new_origAinB)
  |--- d -->|   |          Ergebniswert von search() im 1. Run (d_LO)
     |--dif-|   |          effekt. Verschiebung von A im 1. Run
     |         new_cA      Zentrum der errechneten A-Lage in B
     |  *-------+-------*  um new_cA zentriertes neues B-Gebiet neuer Breite M_new
     |  | *-----+-----*    symmetrisch auf Breite N_new vergroessertes A-Gebiet 
     |  |   *-----+-----*  A-Lage in B nach 2. Run
     |  |-d-|              Ergebniswert von search() im 2. Run (d_LO)
     |    |-|=dif          effekt. Verschiebung von A im 2. Run
     |--dif-|-|            dif1 + dif2 = effekt. Verschiebung von A zur Ur-Lage </pre>
     
  Das denkt die Verschiebung der Zentren von A_0 gegen A_2 als eigentliches
   Verschiebungsmass. Die Zentrenlagen werden bestimmt durch xA+Nx/2. Wenn die
   A-Breiten `N' gerade, haben wir eine Unbestimmtheit von 1 (was bei "--" ist das
   Zentrum?) Dies zu verhindern koennte man auf ungerade M, N und gerade Groessenaenderungen
   achten.
  Beachte, dass oben die A-Lagen in B dargestellt sind. Das A-Gebiet in A selber
   hingegen bleibt an seiner Position originA. Diese aendert sich nur durch 
   Groessenaenderung des A-Gebietes!
   
  Was passiert, wenn am Bildrand die Gebiete abgeschnitten werden? <pre>
  xB AinB            |Bildrand
  *--|--------------*|     B in Ur-Lage; Startgroesse M0
     *-------*             Ur-Lage von A in B (origAinB); Startgroesse N0
     |      *---+---*      A-Lage in B nach 1. Run (new_origAinB)
  |--- d -->|   |          Ergebniswert von search() im 1. Run (d_LO)
     |--dif-|   |          effekt. Verschiebung von A im 1. Run
     |         new_cA      Zentrum der errechneten A-Lage in B
     |  *-------+----|--*  um new_cA zentriertes neues B-Gebiet, abgeschnitten
     |  | *-----+----|*    symmetr. auf Breite new_cA vergroess. A-Gebiet, abgeschnitten
     |  | *----------|--*  A-Lage in B nach 2. Run
     |  |-|=d              Ergebniswert von search() im 2. Run (d_LO)
     |    |dif=0           effekt. Verschiebung von A im 2. Run (==0)
     |-dif--|              dif1 + 0 = effekt. Verschiebung von A zur Ur-Lage </pre>
  
  Hier wurde zeichnerisch unterstellt, dass Bildraender von A und B zusammenfallen.
   Das muss nicht der Fall sein, z.B. trotz gleicher Dim bei einem externen Versatz,
   oder wenn die Dim. verschieden. Beachte: Auch wenn nach einer B-Beschneidung 
   M->M* noch M* > N gilt, kann das A-Gebiet dennoch auch selbst noch die B-Grenzen
   ueberschreiten, wie ja auch oben der Fall (trotz M* > N!). Daher sind die
   A-Gebiete nach ihrer Vergroesserung nicht nur darauf zu pruefen, ob sie die
   A-Grenzen ueberschreiten, sondern auch auf ihr Verbleiben in den B-Grenzen.
     
  Bei Abschneiden von A hat das Verfahren die Moeglichkeit, wieder nach innen
   zu wandern (nach aussen geht nicht), da das Korrelationsgebiet trotz Abschneidens
   ja immer noch groesser (also anders!) sein kann als im 1. Lauf (waere es
   gleichgross, koennte sich nichts aendern, diese Lage war ja bereits das Ergebnis
   dafuer, waere es kleiner, waere das Ergebnis weniger Wert als im ersten Lauf).
   Aber vielleicht kann das Gebiet dann auch generell beendet werden. Zu ueberlegen
   -- wenn finale Groessen der Korrelationsgebiete verschieden sein koennen --, ob
   am Ende nicht auch mit dieser Groesse gewichtet wird?! Die Dimensionen sind im
   Feld `N' verfuegbar.
   
  @internal  Funktion koennte sich stabilisieren als Routine zum vielflaechigen
   Finden einer Verrueckung zwischen zwei Bildern anhand einer gewissen search()-
   Routine, die Verrueckungen zwischen je zwei Teilgebieten feststellt. Dann koennte
   hier auch eine abstrakte "search()"-Funktion als Argument uebergeben werden,
   statt diese per new_search() hier zu kreieren -- aehm, eigentlich dasselbe.
  
  @internal  ProgressInfo: Gemaess der Anfangsweiten M0 und N0 und Zuwaechsen
   rechnen wir erstmal mit drei Durchlaeufen fuer alle Gebiete. Das waere pro
   search()-Aufruf ein Fortschritt um 1 / (3*n_areas). Die Zahl 3*n_areas veringert
   sich entsprechend Gebiete beendet werden, in der ersten Runde pro Gebiet um 2,
   in der zweiten Runde um 1. Leider ist mein ProgressInfo mit Sub-Progressinfos
   noch nicht fertig, womit allererst die Zahl der Bilder zu beruecksichtigen waere.
  
 @internal Die Forderung, M0 muesse wenigsten um 1 groesser sein als N0, ist zu schwach,
  denn die Suchsituation wird dann in jedem Fall als "am Rand" angekommen bewertet
  werden und als Misserfolg abgebrochen. Fuer eine Erfolgsmoeglichkeit muesste
  wenigstens M0 > N0+2 sein. Andererseits kann man auch zwei Bilder als ganzes
  variieren wollen, und da kann 1 Pixel Variationsbreite noch interessant sein.
  Fuer diesen Fall waere eine Extravariable einzufuehren werden, die die Randtests
  suspendiert.
******************************************************************************/
int
DisplcmFinderCorrel_ :: find          ( const VoidScheme2D & schemeA,
                                        const VoidScheme2D & schemeB )
{
    error_ = 1;
    d_result_ = 0;
    
    const Params & prm = params_;  // shorter name
    
    Vec2_int  sizeA (schemeA.xSize(), schemeA.ySize());
    Vec2_int  sizeB (schemeB.xSize(), schemeB.ySize());
    
    /*  Bild mit Suchgebieten ueberdecken... */
    /*  Startdimensionen der B- und A-Gebiete */
    Vec2_int  M0 = prm.start_search_size;
    Vec2_int  N0 = prm.start_correl_size;
    if (M0.x > sizeB.x)  M0.x = sizeB.x;
    if (M0.y > sizeB.y)  M0.y = sizeB.y;
    if (N0.x > sizeA.x)  N0.x = sizeA.x;
    if (N0.y > sizeA.y)  N0.y = sizeA.y;
    
    if (N0.x >= M0.x) N0.x = M0.x - 1;  // wenigstens 1 Pixel zu variieren,
    if (N0.y >= M0.y) N0.y = M0.y - 1;  //  sonst Suche sinnlos TODO zu schwach!
    
    if (N0.x < 1 || N0.y < 1 || (N0.x==1 && N0.y==1))
      return error_;  // Fuer N=1 Korrelation immer "0/0"
    
    VERBOSE_PRINTF(("Bild = %d x %d\n", sizeA.x, sizeB.y));
    VERBOSE_COUT ("M0 = " << M0 << ",  N0 = " << N0 << '\n');  
      
    /*  Ueberdeckungsschema passend zur Bildgeometrie */
    int P = (int)round (sizeB.x / (float)(M0.x)); 
    int Q = (int)round (sizeB.y / (float)(M0.y));
    if (P > prm.max_num_areas_1d) P = prm.max_num_areas_1d;
    if (Q > prm.max_num_areas_1d) Q = prm.max_num_areas_1d;
    VERBOSE_PRINTF(("W=%d  W/Mx= %f  P= %d,   H=%d  H/My= %f  Q= %d\n", 
        sizeB.x, sizeB.x / (float)(M0.x), P,
        sizeB.y, sizeB.y / (float)(M0.y), Q ));
    
    /*  Die Zahl der Suchgebiete */
    int n_areas = P*Q;
    
    /*  Arrays for the n_areas LO-coords in A and in B */
    DynArray1D <Vec2_int>  originA (n_areas);      // L0-Koord. des A-Gebiets in A
    DynArray1D <Vec2_int>  originB (n_areas);      // L0-Koord. des B Gebiets in B
    DynArray1D <Vec2_int>  origAinB (n_areas);     // Pos des A-LO in B vor Rechnung
    DynArray1D <Vec2_int>  new_origAinB (n_areas); // Pos des A-LO in B nach Rechnung
    
    /*  Weil durch Randlagen Gebiete individuell zu reduzieren sind, brauchen
         wir individuelle Dimensionen. Elemente initialisiert mit M0 und N0. */
    DynArray1D <Vec2_int>  M (n_areas, M0);
    DynArray1D <Vec2_int>  N (n_areas, N0);
    
    /*  Justify the B origins (base class routine) */
    justify_origins_for_PxQ (sizeB, M0, P,Q, originB);
    
    /*  Startposition der A-Gebiete in der Mitte der B-Gebiete */
    DEBUG_COUT ("Startpositionen:\n");
    for (int i=0; i < n_areas; i++) {
      originA[i] = originB[i] + (M0-N0) / 2;  // LO in A  [unterstellt ident. Dim und kein Versatz!!]
      origAinB[i] = originA[i];               // A-LO in B
      DEBUG_COUT ("originB = " << originB[i] << ",  originA = " << originA[i] << '\n');
    }
    
    /*  Vektor zur Aufbewahrung der Suchlauf-Infos */
    DynArray1D <AreaInfo>  infos (n_areas);
    
    /*  Erzeuge das CorrelMaxSearch-Tool. HIER AUCH WIEDER VERNICHTEN! 
         We use the nested class as maintainer, which does the deletion. */
    Tool  search (new_search_tool (schemeA, schemeB));
    //CorrelMaxSearchBase*  search = new_search_tool (schemeA, schemeB);
                             
    int n_runs = 0;
    bool all_done = false;
    TNT::StopWatch  uhr;
    
    while (! all_done) 
    {
      printf ("Pass %d...\n", ++ n_runs);
      uhr.start();              // messen Zeiten pro Run, nicht Gesamtzeit
      
      for (int i=0; i < n_areas; i++) {
        if (infos[i].finished) continue;
   
        /*  Die eigentliche Correl-Max-Suche */     
        Vec2_int  d_LO = search().search (originA[i], N[i], originB[i], M[i]);
        
        new_origAinB[i] = originB[i] + d_LO;          // pos of A-origin in B
        infos[i].d += new_origAinB[i] - origAinB[i];  // die Verschiebung. Aufaddiert!
        infos[i].rho_max = search().rho_max();
        infos[i].min_grad = search().min_gradient();
        infos[i].border = search().border_reached();
        
        /*  Bewerten */
        if (search().border_reached())  infos[i].dismiss();
        if (search().rho_max() < prm.min_rho_accepted)  infos[i].low_rho();

        VERBOSE_PRINTF(("Suchgebiet #%d  [%d x %d] in [%d x %d]\n", i, N[i].x, N[i].y, M[i].x, M[i].y));
        VERBOSE_PRINTF(("\td_total=(%d,%d)  d=(%d,%d)  rho_max= %f, grad= %f, border= %d\n", 
            infos[i].d.x, infos[i].d.y, 
            new_origAinB[i].x - origAinB[i].x, 
            new_origAinB[i].y - origAinB[i].y,
            infos[i].rho_max, infos[i].min_grad, infos[i].border));
        DEBUG_COUT ("\toriginB = " << originB[i] << ",  originA = " << originA[i] << '\n');
        DEBUG_COUT ("\td_LO = " << d_LO << ", origAinB = " << origAinB[i] << '\n');
        DEBUG_COUT ("\tnew_origAinB = " << new_origAinB[i] <<'\n');

        //origAinB[i] = new_origAinB[i];  // (*) besser unten, da wir N aendern
      }
      all_done = true;  // below set false, if any area is not finished
      
      /*  Die neuen Gebiete justieren */
      for (int i=0; i < n_areas; i++) {
        if (infos[i].finished) continue;
        
        /*  Zentrum des A-Gebiets in B in seiner Maximumslage */
        Vec2_int  new_centerAinB = new_origAinB[i] + N[i] / 2;
        DEBUG_COUT (i << ":\tnew_centerAinB = " << new_centerAinB << '\n');
        
        /*  Die neuen angestrebten Gebietsgroessen (mglst. gerade Addenten) */
        Vec2_int  M_new = M[i] - prm.subtr_search_dim;  // B-Suchgebiet reduzieren 
        Vec2_int  N_new = N[i] + prm.add_correl_dim;    // Korrel.Gebiet vergroessern
        Vec2_int  M_new_ur = M_new;   // voruebergehend fuer Debugausgabe
        Vec2_int  N_new_ur = N_new;   // dito
        /*  Fordern M-N >= 5, sonst Weitersuche sinnlos */
        if (M_new.x < N_new.x + 5 || M_new.y < N_new.y + 5) {infos[i].finish(); continue;}
        
        /*  Zentrierung des neuen B-Gebietes um das A-Zentrum. */
        originB[i] = new_centerAinB - M_new / 2;
        DEBUG_COUT ("\tneues originB = " << originB[i] << "  M_new = " << M_new <<'\n');
        
        /*  Sichern, dass versetztes B-Gebiet B-Grenzen nicht uebersteigt. Bei
             Randuebertritt abschneiden. Ist dann nicht zentrisch zu centerAinB.*/
        if (originB[i].x < 0) {
          M_new.x += originB[i].x;  originB[i].x = 0;
        }
        else if (originB[i].x + M_new.x >= sizeB.x) {
          M_new.x = sizeB.x - originB[i].x; 
        }
        if (originB[i].y < 0) {
          M_new.y += originB[i].y;  originB[i].y = 0;
        }
        else if (originB[i].y + M_new.y >= sizeB.y) {
          M_new.y = sizeB.y - originB[i].y; 
        }
        
        /*  Symmetrische Vergroesserung des A-Gebiets (Korrelationsflaeche) */
        originA[i] -= (N_new - N[i]) / 2;         // neuer LO in A
        origAinB[i] = new_centerAinB - N_new / 2; // neuer LO von A in B
        //origAinB[i] -= (N_new - N[i]) / 2;      // oder so, wenn oben (*)
        DEBUG_COUT ("\tneues originA = " << originA[i] << "  N_new = " << N_new <<'\n');
        DEBUG_COUT ("\tneues origAinB = " << origAinB[i] << '\n'); 

        /*  Sichern, dass vergroessertes A-Gebiet A-Grenzen nicht uebersteigt. Gegebenenfalls
             abschneiden. Korrelationsgebiet dann nicht symmetrisch vergroessert. Auch
             koennen gerade (unsymmetrische) Dimensionen entstehen. */
        if (originA[i].x < 0) {
          N_new.x += originA[i].x;        // reduziert um den Ueberstand
          origAinB[i].x -= originA[i].x;  // erhoeht um den Ueberstand
          originA[i].x = 0;
        }
        else if (originA[i].x + N_new.x >= sizeA.x) {
          N_new.x = sizeA.x - originA[i].x;  
          // originA.x und origAinB.x unveraendert
        }
        if (originA[i].y < 0) {
          N_new.y += originA[i].y;        // reduziert um den Ueberstand
          origAinB[i].y -= originA[i].y;  // erhoeht um den Ueberstand
          originA[i].y = 0;
        }
        else if (originA[i].y + N_new.y >= sizeA.y) {
          N_new.y = sizeA.y - originA[i].y;
          // originA.y und origAinB.y unveraendert
          if (N_new.y <= N[i].y) {infos[i].finish(); continue;}
        }
        
        /*  Sichern, dass vergroessertes A-Gebiet in seiner B-Lage nicht die B-Grenzen
             uebersteigt. Gegebenfalls abschneiden. */
        if (origAinB[i].x < 0) {
          N_new.x += origAinB[i].x;       // reduziert um den Ueberstand
          originA[i].x -= origAinB[i].x;  // erhoeht um den Ueberstand
          origAinB[i].x = 0;
        }
        else if (origAinB[i].x + N_new.x >= sizeB.x) {
          N_new.x = sizeB.x - origAinB[i].x;
          // origAinB.x und originA.x unveraendert
        }
        if (origAinB[i].y < 0) {
          N_new.y += origAinB[i].y;       // reduziert um den Ueberstand
          originA[i].y -= origAinB[i].y;  // erhoeht um den Ueberstand
          origAinB[i].y = 0;
        }
        else if (origAinB[i].y + N_new.y >= sizeB.y) {
          N_new.y = sizeB.y - origAinB[i].y;
          // origAinB.y und originA.y unveraendert
        }
        
        /*  Falls Korrelationsgebiet nach Reduktionen <= als im vorigen Run,
             Gebiet beenden.*/
        if (N_new.x <= N[i].x  ||  N_new.y <= N[i].y) {
          infos[i].finish(); continue;
        }
        /*  Nach M-Reduktion koennte M <= N geworden sein */
        if (M_new.x <= N_new.x || M_new.y <= N_new.y) {infos[i].finish(); continue;}
             
        //cout << "orig_B = " << originB[i] << ",  orig-A = " << originA[i] << '\n';
        if (M_new != M_new_ur) 
           cout <<'#'<<i<< ": B BESCHNITTEN: M: " << M_new_ur << " -> " << M_new << '\n';
        if (N_new != N_new_ur) 
           cout <<'#'<<i<< ": A BESCHNITTEN: N: " << N_new_ur << " -> " << N_new << '\n';

        M[i] = M_new;
        N[i] = N_new;   
        
        all_done = false;
      }
      putchar('\t'); uhr.result();
    
    } // while (! all_done)
  
    /* Auswerten */
    Vec2_double  sum_dw(0.0), sum_d(0.0), sum_d2(0.0);
    double  sum_w = 0.0;
    int  n_used = 0;
    
    for (int i=0; i < n_areas; i++) {
      if (infos[i].dismissed) continue;
      //double w = 0.5 * (infos[i].rho_max  + 1);  // --> w in [0..1]
      double w = infos[i].rho_max;      // rho in [-1..+1]
      if (w < 0.0) w = 0.0;             // --> w in [0..1]
      VERBOSE1_COUT (i << ": w = " << w << ",  d = " << infos[i].d << '\n');
      Vec2_double  d = infos[i].d;      // double <- int
      sum_d += d;
      sum_d2 += d*d;
      sum_dw += w * d;
      sum_w += w;
      n_used ++;
    }
    if (!n_used) {
      printf("Displacement computation failed: Not any area usable (out of %d).\n", n_areas);
      return error_;
    }
    if (sum_w == 0.0) {
      printf("Displacement computation failed: sum_w = 0 (for %d areas).\n", n_areas);
      return error_;
    }
    sum_d /= double(n_used);    // E(X)
    sum_d2 /= double(n_used);   // E(X^2)
    Vec2_double  cov = sum_d2 - (sum_d * sum_d);  // E(X^2) - (EX)^2
    Vec2_double  sigma (sqrt(cov.x), sqrt(cov.y));
    
    printf("%d areas dismissed, %d of %d used.\n", n_areas - n_used, n_used, n_areas);
    cout << "d_weight = " << sum_dw/sum_w << ",  w_mean = " << sum_w/n_used << '\n';
    cout << "d_raw = " << sum_d << "  +/- (" << sqrt(cov.x) << ", " << sqrt(cov.y) << ")\n";
    
    /*  Versuch: Nur d-Werte innerhalb der Standardabweichung mitnehmen */
    cout << "Wenn nur Werte innerhalb der Standardabweichung genommen:\n";
    Vec2_double  d_mean = sum_d; 
    sum_dw = Vec2_double(0.0);
    sum_w = 0.0;
    n_used = 0;
    for (int i=0; i < n_areas; i++) {
      if (infos[i].dismissed) continue;
      Vec2_double  d = infos[i].d;  // double <- int
      Vec2_double  diff = d - d_mean;
      if (fabs (diff.x) > sigma.x ||
          fabs (diff.y) > sigma.y  > 0.0) {
        cout << i << ": w = " << infos[i].rho_max << ",  d = " << infos[i].d << "  Rejected!\n";
        continue;
      }
      double w = infos[i].rho_max;      // rho in [-1..+1]
      if (w < 0.0) w = 0.0;             // --> w in [0..1]
      VERBOSE1_COUT (i << ": w = " << w << ",  d = " << infos[i].d << '\n');
      sum_dw += w * d;
      sum_w += w;
      n_used ++;
    }
    if (!n_used) {
      printf("Displacement computation failed: Not any area usable (out of %d).\n", n_areas);
      return error_;
    }
    if (sum_w == 0.0) {
      printf("Displacement computation failed: sum_w = 0 (for %d areas).\n", n_areas);
      return error_;
    }
    printf("%d areas dismissed, %d of %d used.\n", n_areas - n_used, n_used, n_areas);
    cout << "d_w* = " << sum_dw/sum_w << ",  w*_mean = " << sum_w/n_used << '\n';
    
    sum_dw /= sum_w;
    d_result_.x = (int)round (sum_dw.x);
    d_result_.y = (int)round (sum_dw.y);
    
    return (error_ = 0);
}



//===================
//  STATIC elements 
//===================
const DisplcmFinderCorrel_::Params  
DisplcmFinderCorrel_::default_params_ = {
        Vec2_int (101,101),     // start dimensions of the search areas
        Vec2_int (21,21),       // start dimensions of the correlation areas
        5,                      // max number of search areas in each dimension
        20,                     // subtract this from search dims after each pass
        20,                     // add this to correl dims after each pass
        0.7f,                   // minimum accepted correlation coeff.
        0  };                   // verbosity level

DisplcmFinderCorrel_::Params
DisplcmFinderCorrel_::init_params_ = DisplcmFinderCorrel_::default_params_;
        
// END OF FILE

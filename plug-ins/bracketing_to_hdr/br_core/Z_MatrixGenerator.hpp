/*
 * Z_MatrixGenerator.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  Z_MatrixGenerator.hpp
*/
#ifndef Z_MatrixGenerator_hpp
#define Z_MatrixGenerator_hpp

#include <cstdio>                  // printf() 
#include "br_types_array.hpp"      // ChannelFollowUpArray
#include "br_macros.hpp"           // ASSERT(), BR_WARNING()
#include "DynArray1D.hpp"          // DynArray1D<>

#undef BR_DEBUG
#include "br_macros_varia.hpp"     // SPUR()

 
/**===========================================================================
  
  @class Z_MatrixGenerator
  
  Template.
  Erzeugung einer "Z-Matrix" \a Z[i,j]<Unsign> der Dimension [nselect, nlayers]
   aus dem von den <tt>FollowUpValues_*</tt>-Klassen gelieferten Statistikfeld
   vom Typ \c ChannelFollowUpArray. Z[i,j]: z-Wert am Ort `i´ in Aufnahme `j´, 
   wobei "Ort i" abstrakt eine Position im Statistikfeld. Die Z-Matrix ist dann
   Ausgangspunkt der Kennlinienberechnung. Klasse umfasst im wesentlichen nur
   die Funktion create_Z(), dennoch als Klasse angelegt, um Debug-Hilfsmittel
   wie list_InfoField() einfach unterzubringen.
 
  Benoetigt keinen Zugriff auf Pack-Daten, daher auch keine Abhaengigkeiten
   hereingebracht. Zu konstruieren lediglich mit `z_max'-Wert. Dann create_Z()
   mit dem \c ChannelFollowUpArray eines Kanals als Argument aufrufen, 
   Dimensionen dieses Array2D's enthalten alle weiteren benoetigten Daten.
 
  <b>Zum Template-Parameter \c Unsign: </b> 
   Typ des Ctor-Arguments `z_max' und des returnierten Array2D (der "Z-Matrix"). 
   \c ChannelFollowUpArray hingegen frei davon. `z_max' koennte leicht auch
   unsigned sein. FRAGE: Koennte das returnierte Feld verallgemeinernd auch
   float-Zahlen statt Unsign-Daten enthalten? Dann waere Template unnnoetig. ANTWORT:
   Nein. Zwar ist im Verfahren zu Berechnung der Antwortfunktion nicht der absolute
   z-Wert relevant, sondern nur die Stelle (Index) seiner Anordnung, allerdings ist
   fuer Float-Zahlen keine Beschraenkung auf z.B. 256 Werte garantiert. Zudem ginge in
   ResponseSolver dann auch keine Wichtungs*tabelle* mit Index-Syntax mehr, dann
   braeuchte es dort Wichtungsfunktion mit float-Argument!  (In \a merge_HDR_*() hingegen
   kommen nur echte z-Werte vor.) Global und "ohne weiteres" koennte Z-Matrix hoechstens
   ein unsigend-Feld werden.
 
  @note Es sind verschiedene Varianten einer Z-Matrix-Generierung denkbar, diese
   -- auf der Basis eine Folgewerte-Statistikfeldes -- ist nur eine davon.
 
*============================================================================*/
template <typename Unsign>
class Z_MatrixGenerator 
{
private:

    /** Eintrag eines [n_zvals]-Infofeldes, darin fuer jede Zeile eines 
         <tt>ChannelFollowUpArray</tt>s markiert, ob brauchbar (ok==true),
         welches die vorhergehende und welches die nachfolgende brauchbare Zeile,
         und ob Zeile bereits fuer Z-Matrix benutzt.   */
    struct ValueInfo {bool ok; int pred; int succ; bool used;};

    Unsign  zmax_;
    int  method_;       /*!<<  1 (default) or 2 (old method) */

public:

    Z_MatrixGenerator (Unsign zmax, int method=1) 
      : zmax_(zmax), method_(method)  
      {}

    /**  The official create_Z() function. Calls the wished method. */
    TNT::Array2D<Unsign> 
      create_Z (const ChannelFollowUpArray &F, int refpic, int nselect)
        {if (method_==2) return create2_Z (F, refpic, nselect);
         else            return create1_Z (F, refpic, nselect);}

    TNT::Array2D<Unsign> 
      create1_Z (const ChannelFollowUpArray &, int refpic, int nselect);
    
    TNT::Array2D<Unsign> 
      create2_Z (const ChannelFollowUpArray &, int refpic, int nselect);

private:
          
    void list_InfoField (const ValueInfo*, int) const;   // Debug helper
};



/**+*************************************************************************\n
  list_InfoField()  --  debug and devel helper.
******************************************************************************/
template <typename Unsign>
void 
Z_MatrixGenerator<Unsign>::list_InfoField (const ValueInfo* aa, 
                                           int n_zvals )    const
{
    printf("List InfoField...\n");

    for (int i=0; i < int(n_zvals); i++)
      printf("%6d: ok=%d, pred=% d, succ=% d\n", 
        i, aa[i].ok, aa[i].pred, aa[i].succ);
}


/**+*************************************************************************\n
  create1_Z()  --  derzeit benutzte Variante. 
 
  @param F: (I) [nImages, n_zvals]-Array2D<float> gemittelter Folgewerte
      eines Kanals wie es von <tt>FollowUpValues</tt>-Klassen geliefert wird.
  @param refpic: (I) Referenz-Bild bei Erstellung von \a F.
  @param nselect: (I) Anzahl auszuwaehlender Schnitte durch Statistikfeld; Dim 1
      des returnierten Feldes.
  @return [nselect, nImages]-Array2D<Unsign>.
 
  <b>Aufgabe:</b>
   Erzeugung einer "Z-Matrix" der Dimension [nselect, nImages] aus einem Feld 
   gemittelter Folgewerte eines Kanals. Typ der Z-Matrix-Eintraege: \c Unsign.
 
  <b>Problem:</b>
   Von den \a n_zvals moegl. Unsign-Werten [0...z_max] sind \a nselect auszuwaehlen. 
   Wie? Gleichabstaendig oder auch unter Beruecksichtigung ihrer Streuung oder...?
 
  <b>Verfahren:</b>
   Gegeben [n_zvals]-Feld \a F von Zeilen (oder Spalten) zu je \a nImages Elementen.
   Aus den \a n_zvals Zeilen sind \a nselect moeglichst gleichabstaendig auszuwaehlen.
   Diese Auswahl vollziehen wir anhand der z-Werte (Zeilen) des <i>Referenzbildes</i>.
   Dabei koennen z-Werte (Zeilen von \a F) allerdings unbrauchbar sein (n==0). An 
   \a first und \a last bestimmen wir zunaechst die erste und die letzte brauchbare
   Zeile. Dazwischen werden dann die \a nselect auszuwaehlenden Werte entnommen, wobei
   wieder Zeilen unbrauchbar sein koennen (n==0). Diese sind auszulassen und 
   stattdessen naechstbenachtbarte zu nehmen. Dazu dient unten das Infofeld \a aa[].
   Die unschoene Abhaengigkeit der ermittelten Antwortfunktion vom Referenzbild
   wie noch bei create2_Z() der Fall wurde beseitigt - zumindest fuer simulierte
   Testbilder.
 
  FRAGE: Das Endresultat sind Unsign-Daten (uchars oder ushorts), doch bei
   numerischen Ops mit uchars und ushorts werden diese zunaechst immer in ints
   konvertiert. Ist es dann nicht effektiver, bei den Unsigns \a z und \a z_max
   gleich mit ints zu rechnen?

  @note Operation mit Unsign z_max der Art `z_max + 1' sind sowieso nur bis
   ushort (uint16) moeglich (implizit gewandelt in uint32). Fuer den maximalen
   uint32 gaebe das 0.
******************************************************************************/
template <typename Unsign> 
TNT::Array2D<Unsign> 
Z_MatrixGenerator<Unsign>::create1_Z (

        const ChannelFollowUpArray &F, 
        int refpic, 
        int nselect )
{ 
    int nlayers = F.dim1();
    int n_zvals = F.dim2();    // statt "n_zvals" besser "n_grid"
    
    /*  Find first useful line (if none it ends with first=zmax_) */
    int first = 1;
    while (F[refpic][first].n == 0  &&  first < zmax_)  first++;
    
    /*  Find last useful line (if none it ends with last=0) */    
    int last = zmax_ - 1;
    while (F[refpic][last].n == 0  &&  last > 0)  last--;

    /*  Number of useful lines */
    int nuseful = 0;
    for (int i=first; i <= last; i++)  if (F[refpic][i].n)  nuseful++;
    
    /*  How many of `nselect' are realy possible */
    int myselect;
    
    if (nuseful < 2) {
      printf ("%s(): Less than 2 useful lines! Return NULL-Array.\n",__func__);
      return TNT::Array2D<Unsign>();    // Null-Array
    }
    else if (nuseful < nselect) {
      myselect = nuseful;               // > 1 garantiert
      printf ("%s(): Only %d useful lines of %d desired\n",__func__, myselect, nselect);
    }
    else
      myselect = nselect;               // > 1 garantiert

    printf("first=%d, last=%d,  nselect=%d, myselect=%d,  nuseful=%d, n_zval=%d\n", 
      first, last, nselect, myselect, nuseful, n_zvals);
      
   /*  Ein [n_zvals]-Infofeld von {bool ok; int pred; int succ; bool used;},
        darin fuer jede Zeile von `F' markiert, ob brauchbar (ok==true),
        welches die vorhergehende und welches die nachfolgende brauchbare
        Zeile, und ob Zeile bereits fuer Z-Matrix benutzt: */
    DynArray1D<ValueInfo>  aa (n_zvals);  // ISO-C++ forbids variable-size arrays
    
    /*  Initialization of `aa[]'... */
    /*  Evaluate aa[].ok */
    for (int i=0;      i < first;   i++)  {aa[i].used = aa[i].ok = false;}
    for (int i=last+1; i < n_zvals; i++)  {aa[i].used = aa[i].ok = false;}
    for (int i=first;  i <= last;   i++)
    {
      aa[i].used = false;
      aa[i].ok = (F[0][i].n > 0);   // ok, if n != 0 (whereat n ident. for all p)
    }
    /*  Evaluate aa[].pred */
    int pred = -1;
    for (int i=0; i < int(n_zvals); i++)
    {
      aa[i].pred = pred;
      if (aa[i].ok) pred = i;
    }
    /*  Evaluate aa[].succ */
    int succ = -1;
    for (int i=n_zvals-1; i >= 0; i--)
    {
      aa[i].succ = succ;
      if (aa[i].ok) succ = i;
    }
      
    //list_InfoField (aa, n_zvals);
        
    /*  Allocate Z-Matrix */
    TNT::Array2D<Unsign>  Z (myselect, nlayers);

    /*  Now select points nearly equidistant... */
    /*  (Fract) step width from first to last for myselect points */
    double dd = double(last - first) / (myselect - 1);  // dd>=1 garantiert

    for (int i=0; i < myselect; i++)
    {
      /*  The wanted position (line) */
      size_t k = size_t(first + i*dd + 0.5);    // rounding for > 0
      SPUR(("\twanted k = %d\n", k));
      
      /*  If wanted line not ok or already used, find nearest suitable */
      if (! aa[k].ok || aa[k].used)
      {
        /*  Find nearest predecessor and successor not yet used */
        int pred = aa[k].pred;
        int succ = aa[k].succ;
        while (succ >= 0 && aa[succ].used) succ = aa[succ].succ; 
        while (pred >= 0 && aa[pred].used) pred = aa[pred].pred;
        if (pred < 0) 
        { 
          aa[k].pred = -1;    // Stopper fuer nachfolgende
          if (succ < 0) 
            BR_WARNING(("Both borders reached: Should not happen!"))
          else 
            k = succ;
        }
        else if (succ < 0)    // pred >= 0
        {
          k = pred;
        }
        else                  // pred >=0 && succ >= 0
        {
          if (k-pred < succ-k)
            k = pred;
          else
            k = succ;
        }
        SPUR(("\tused k = %d\n", k));
      }

      /*  Copy the z-values of the k-th row of F into Z */
      for (int p=0; p < nlayers; p++) 
      {
        Z [i][p] = Unsign(F[p][k].z + 0.5);     // rounding for > 0
        aa[k].used = true;
      }
    }

    return Z;
}


/**+*************************************************************************\n
  create2_Z()  --  urspruengliche Variante (Ueberlappungsidee).
 
  @param F: (I) [nImages, n_zvals]-Array2D<float> gemittelter Folgewerte
      eines Kanals wie es von <tt>FollowUpValues</tt>-Klassen geliefert wird.
  @param refpic: (I) Referenz-Bild bei Erstellung von \a F. Not used here.
  @param nselect: (I) Anzahl auszuwaehlender Schnitte durch Statistikfeld; Dim 1
      des returnierten Feldes.
  @return [nselect, nImages]-Array2D<Unsign>.
 
  <b>Aufgabe:</b>
   Erzeugung einer "Z-Matrix" der Dimension [nselect, nImages] aus einem Feld 
   gemittelter Folgewerte eines Kanals. Typ der Z-Matrix-Eintraege: \c Unsign.
 
  <b>Problem:</b>
   Von den \a n_zvals moegl. Unsign-Werten [0...z_max] sind \a nselect auszuwaehlen. 
   Wie? Gleichabstaendig oder auch unter Beruecksichtigung ihrer Streuung oder...?
 
  <b>Verfahren:</b> 
   Gegeben [n_zvals]-Feld \a F von Zeilen (oder Spalten) zu je \a nImages Elementen.
   Aus den \a n_zvals Zeilen sind \a nselect moeglichst gleichabstaendig auszuwaehlen.
   Dabei koennen Zeilen allerdings unbrauchbar sein (n==0), solche sind auszulassen. 
   Desweiteren wurde in dieser Funktion (obsoleterweise) als Bedingung erhoben,
   dass nur solche Zeilen brauchbar, darin wenigstens zwei benachtbarte Bilder im 
   Arbeitsbereich (AB), sprich, innnerhalb <i>einer Zeile</i> eine gehaltvolle
   Ueberlappung statthat. An \a first und \a last wird zunaechst die erste und letzte
   in diesem Sinne brauchbare Zeile gesucht. Dazwischen werden dann \a nselect Zeilen
   ausgewaehlt. Unbrauchbare Zeilen (n==0) sind dabei auszulassen und stattdessen
   naechstbenachtbarte zu nehmen. Dazu dient unten das Infofeld \a aa[].
 
   Die Ueberlappungsbedingung ist allerdings ein Missverstaendnis. Denn es ist 
   irrelevant, dass eine gehaltvolle Ueberlappung innerhalb einer Zeile ("an einem
   Pixel") statthat, denn alle Pixel werden als gleichartig (mit gleicher 
   Kennlinie) unterstellt. Die beiden Zeilen (0,200) und (40,255) liefern
   ebensogut den gehaltvollen Zusammenhang <tt>t2 <-> z=200,  t1 <-> z=40!</tt>
   Bei der Auswahl blieb das Referenzbild unberuecksichtigt. Im Endeffekt gab 
   die so ermittelte Z-Matrix eine aergerliche Abhaengigkeit der berechneten 
   Antwortfunktion vom bei der Erzeugung des Statistikfeldes \a F zugrunde 
   gelegten <i>Referenzbild</i>. Daher inzwischen ersetzt durch create1_Z().
   
  @todo Die Implemantion von "zwei benachtbarte im AB" unten erkennt zwar Situation
   "xx000", aber nicht "0xx00", dh wenn zwei beachtbarte spaeter. Zudem bisher
   nur fuer \a first implementiert, \a last noch gemaess "irgend zwei" ermittelt!
   Allerdings Funktion ja nun aus dem Rennen.
******************************************************************************/
template <typename Unsign> 
TNT::Array2D<Unsign> 
Z_MatrixGenerator<Unsign>::create2_Z (

        const ChannelFollowUpArray &F, 
        int refpic, 
        int nselect )
{ 
    int nlayers = F.dim1();
    int n_zvals = F.dim2();     // statt "n_zvals" besser "n_grid"

    ASSERT((int)zmax_+1 == n_zvals, RELAX);  /// @bug Fuer U16 ist zmax_+1 != n_grid!

    /*  Bereich feststellen, darin wenigstens zwei Werte im Arbeitsbereich */
    int  first, last;           // gleicher Typ wie `n_zvals'!
    bool ok;

    ok = false;                 // noch keine zwei Werte im AB
    for (first=0; first < n_zvals; first++)
    {
      bool ok1 = false;         // noch kein Wert im AB
      for (int p=0; p < nlayers; p++)
      {
        Unsign z = Unsign(F[p][first].z + 0.5);  // runden fuer > 0
#if 0   
        /*  Zwei AB-Werte in *irgendzwei* Bildern... */
        if (z > 1 && z < zmax_-1)     // im Arbeitsbereich
//      if (z > 0 && z < zmax_)       // im Arbeitsbereich
        {   if (ok1)                  // ein Wert war schon im AB
            {   ok = true;            // folglich jetzt ein zweiter
                printf ("ok-break bei first=%i, p=%i\n", first,p);
                break;                // -> p-Schleife verlassen; wegen
            }                         // ok dann auch first-Schleife
            else
                ok1 = true;           // 1. Wert im AB
        }
#else
        /*  Zwei AB-Werte in *benachtbarten* Bildern... */
        if (ok1)                      // ein Wert war schon im AB
        {   if (z > 1 && z < zmax_-1)
            {   ok = true;            // folglich jetzt ein zweiter
                break;                // --> p-Schleife verlassen; wegen
            }                         //  ok dann auch first-Schleife
            else
                break;                // p-Schleife verlassen, da nicht
        }                             //  mehr benachbart
        else                          // noch kein Wert im AB
            ok1 = (z > 1 && z < zmax_-1); // Erster Wert im AB?
#endif
      }
      if (ok) break;
    }

    ok = false;                       // noch keine zwei Werte im AB
    for (last=zmax_; last > first; last--)
    {
      bool ok1=false;
      for (int p=0; p < nlayers; p++)
      {
        Unsign z = Unsign(F[p][last].z + 0.5);   // runden fuer > 0
        if (z > 1 && z < zmax_-1)     // im Arbeitsbereich (+1)
//      if (z > 0 && z < zmax_)       // im Arbeitsbereich (exakt)
        {   if (ok1)                  // ein Wert war schon im AB
            {   ok = true;            // folglich jetzt ein zweiter
                break;                // --> p-Schleife verlassen; wegen
            }                         //  ok dann auch last-Schleife
            else
                ok1 = true;           // 1. Wert im AB
        }
      }
      if (ok) break;
    }
    /*  Das ueberlappende Intervall ist also [first..last]. */
    printf ("%s(): first = %u, last = %u\n", __func__, first, last);
      
   /*  Ein [n_zvals]-Infofeld von {bool ok; int pred; int succ; bool used;},
        darin fuer jede Zeile von `F' markiert, ob brauchbar (ok==true),
        welches die vorhergehende und welches die nachfolgende brauchbare
        Zeile, und ob Zeile bereits fuer Z-Matrix benutzt: */
    DynArray1D<ValueInfo>  aa (n_zvals);  // ISO C++ forbids variable-size arrays
    
    /*  Initialization of `aa[]'... */
    /*  Evaluate aa[].ok */
    for (int i=0;      i < first;   i++)  {aa[i].used = aa[i].ok = false;}
    for (int i=last+1; i < n_zvals; i++)  {aa[i].used = aa[i].ok = false;}
    for (int i=first;  i <= last;   i++)
    {
      aa[i].used = false;
      aa[i].ok = (F[0][i].n > 0);  // ok, if n != 0 (whereat n identic for all p)
    }
    /*  Evaluate aa[].pred */
    int pred = -1;
    for (int i=0; i < int(n_zvals); i++)
    {
      aa[i].pred = pred;
      if (aa[i].ok) pred = i;
    }
    /*  Evaluate aa[].succ */
    int succ = -1;
    for (int i=n_zvals-1; i >= 0; i--)
    {
      aa[i].succ = succ;
      if (aa[i].ok) succ = i;
    }
      
    //list_InfoField (aa, n_zvals);
        
    /*  Number of useful lines */
    int nuseful = 0;
    for (int i=0; i < n_zvals; i++)  if (aa[i].ok)  nuseful++;

    /*  How many of `nselect' are realy possible */
    int myselect;
    
    if (nuseful < 2) {
      printf ("%s(): Less than 2 useful lines! Return NULL-Array.\n",__func__);
      return TNT::Array2D<Unsign>();    // Null-Array
    }
    else if (nuseful < nselect) {
      myselect = nuseful;               // > 1 garantiert
      printf ("%s(): Only %d useful lines of %d desired\n",__func__, myselect, nselect);
    }
    else
      myselect = nselect;               // > 1 garantiert

    printf("nselect = %d,  myselect = %d,  nuseful = %d,  n_zval = %d\n", 
      nselect, myselect, nuseful, n_zvals);
      
    /*  Allocate Z-Matrix */
    TNT::Array2D<Unsign>  Z (myselect, nlayers);

    /*  Now select points nearly equidistant... */
    /*  (Fract) step width from first to last for myselect points */
    double dd = double(last - first) / (myselect-  1);  // dd>=1 garantiert

    for (int i=0; i < myselect; i++)
    {
      /*  The wanted position (line) */
      size_t k = size_t(first + i*dd + 0.5);    // rounding for > 0
      SPUR(("\twanted k = %d\n", k));
      
      /*  If wanted line not ok or already used, find nearest suitable */
      if (! aa[k].ok || aa[k].used)
      {
        /*  Find nearest predecessor and successor not yet used */
        int pred = aa[k].pred;
        int succ = aa[k].succ;
        while (succ >= 0 && aa[succ].used) succ = aa[succ].succ; 
        while (pred >= 0 && aa[pred].used) pred = aa[pred].pred;
        if (pred < 0) 
        { 
          aa[k].pred = -1;    // Stopper fuer nachfolgende
          if (succ < 0) 
            BR_WARNING(("Both borders reached: Should not happen!"))
          else 
            k = succ;
        }
        else if (succ < 0)    // pred >= 0
        {
          k = pred;
        }
        else                  // pred >=0 && succ >= 0
        {
          if (k-pred < succ-k)
            k = pred;
          else
            k = succ;
        }
        SPUR(("\tused k = %d\n", k));
      }

      /*  Copy the z-values of the k-th row of F into Z */
      for (int p=0; p < nlayers; p++) 
      {
        Z [i][p] = Unsign(F[p][k].z + 0.5);     // rounding for > 0
        aa[k].used = true;
      }
    }

    //list_InfoField (aa, n_zvals);
    //::list (Z);

    return Z;
}


#endif  // Z_MatrixGenerator_hpp

// END OF FILE

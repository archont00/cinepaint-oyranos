/*
 * Fl_Cartesius.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file Fl_Cartesius.cpp
 
  @todo
   - Bug: Annahme, laengester Tick-String sei stets aeusserster, versagt
      zB. fuer y-ticks in [0...1]: "0.5" oder "0.02" laenger als "1"!
   - Bedenke, dass ARANGE_WRAP fuer gezoomte Ansichten keinen Sinn hat!
   - Logarithmische Skale, bisherige ein Witz.
   - Klaeren, welche Koordinaten beim Zoomen sich aendern sollen.
   - Kopfueber-Zeichnen moeglich z.Z. fuer SCALE_FIX und wa > wb (wobei
     weiter wmin .<. wmax). Nach einem SCALE_AUTO ist aber wieder wa .<. wb. 
     Will man die Umkehrung bewahren, waere eine Variable `reverse'
     einzufuehren.
   - Option `changeable_by_ticknums' einbauen, dass Plotgebiet wahlweise
     sich aendernden Tickzahllaengen anpasst (momentan) oder auch fix bleibt.
     Da waere soviel Platz zu reservieren, dass groesste Zahl noch hinpasst.
   - Klare MinMax- und World-Setz-Schnittstelle fuer Fl_Cartesius festlegen,
     damit dort alle von diesen 'globalen' Rahmenwerten abhaengigen Parameter
     zentral gewartet werden koennen. Also z.B. ~_minmax() und ~_world().
   - Klares und einfaches justify()-Regime! Wie sicherstellen, dass 
     justify() immer aufgerufen, wenn notwendig, aber auch nicht oefter?    
     Zur Zeit boolsche Variable `rejustify_'. Aber diese nicht gesetzt
     z.B. in scale_mode(bool) etc. D.h., wer hier live aendert, muss 
     explizites justify() und redraw() aufrufen.
   - Achsenbeschriftung inbesondere im "%E"-Format verbesserungswuerdig.
     Schoen waere ein einmaliges "x 10^n" an der Achse zu plazieren und
     die Tickzahlen davon zu entlasten.
   - Kurvenlabel anbringen. In diesem Kontext wohl rechter Zusatzrand noetig.
   - Lange x-Zahlen koennen rechten Fensterrand ueberschreiten (z.Z. 
     verhindert durch Verschieben dieser Zahl, was aber auch fragwuerdig).
     Sollte man nicht gleich xb() anpassen aehnlich wie bei __Y? Aber dann
     zumindest fuer arrange=WRAP Justageverfahren zirkulaer.
   - Soll der Rahmen ausserhalb des Plotfensters liegen oder innerhalb?
     Wenn innerhalb, verdeckt er Punkte, die genau auf diesem Wert liegen.
     Wenn ausserhalb, ist der entsprechende Tick eine Verdickung am Rahmen.
     Entschieden: Rahmen ausserhalb.
   - Auf wmin==wmax reagieren (intelligent groessenordnungsgemaess umhuellen).
   - Beim Umschalten von scale=AUTO auf FIX springt unter gewissen 
     Umstaenden xa() etwas nach rechts (zB von 46 nach 49) und ich weiss 
     nicht warum. Muesste FIX nicht alles unveraendert lassen??? Und beim
     Zurueckschalten auf AUTO springt es zurueck. Auch da: Warum??
   - Die Font-Setzfunktion heisst "axisfont()", intern nenne ich die Werte
     aber nur "font_size_" etc. Eventuell gibt's 'mal noch "titelfont" etc.,
     dann anpassen.
   - Nachwievor die Namensfrage: w_left(), w_right()... oder 
     wleft(), wright()... oder wLeft(), wRight()... - ?
     Ferner XX(), YY() oder xAxis(), yAxis() oder ... - ?
     Und auch: xmin() oder x_min()? - Hergottnochmal!!!
 
  @note
   - TUeCKE: Wenn scale=AUTO, muss MinMax gesetzt sein, da MinMax verwendet wird!
     Da scale=AUTO gegenwaertig voreingestellt, wird MinMax-Setzen Standard.
     Andernfalls muesste 'wrld' gesetzt werden, etwas setzen muss der User
     natuerlich immer.
   - `win'-Werte werden erst durch justify() gesetzt; vorher ist also
     noch kein x_from_wx() etc. verfuegbar!
   - Fl_Cartesius is a open Group and has to be end()ed by the user.
 
  <b>KLARSTELLUNG:</b> 
   Die MinMax-Werte sind die der Weltobjekte (Kurven), `wrld' sind die
   den Windowpunkten (a,b) zugeordneten Weltkoordinaten und bestimmen
   allein die wrld->win-Abbildung. `Wrld'-Werte koennen also von MinMax
   differieren. Wozu? Z.B. fuer's Zoomen, ferner auch fuer's Einhuellen, aber
   das vielleicht gar nicht benoetigt. Die Information der MinMax-Werte
   brauchen wir aber auf jeden Fall, z.B. fuer's Zuruecksetzen.
  
  @note 
   Wir koennen in AxisInfo soviel privat machen wie wir wollen, AxisInfo
   muss nur Fl_Cartesius zum Freund erklaeren.
 
  CHANGES due to change of the base class Fl_Window -> Fl_Group:
   Fl_Cartesius: 
    - justify() & draw_coord_picker(): add "x()+" and "y()+"
    - draw_x_axis():  add "x()+" for the "right border correction"
    - resize(): new_win(a, a+dW) -->  new_win(a+dX, a+dX+dW).
*/

#include <cstdio>
#include <cmath>            // round(), log10(), pow(); pow10() no more used
#include <cassert>          // assert()
#include <cstring>          // strlen()
#include <algorithm>        // min(), max()
#include <FL/Fl.H>          // event_x()...
#include <FL/Enumerations.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include "Fl_Cartesius.hpp"
#include "fl_print_event.hpp"  // Fltk debug utilities

// Do we want debug output for this file...?
#undef YS_DEBUG
//#define YS_DEBUG               
#include "ys_dbg_varia.hpp"  // DBG_CALL()

// If debug, what do we want to see...
#ifdef YS_DEBUG
  //#define DBG_EVENT
  //#define DBG_TICKING
  //#define DBG_CLASS
  #define DBG_DRAWING
  //#undef DBG_CALL
  //#define DBG_CALL(arg)
#endif

#ifdef DBG_TICKING
  #define SPUR_TICK  SPUR
  #define TRACE_TICK  TRACE
#else
  #define SPUR_TICK(args)
  #define TRACE_TICK(args)
#endif

#ifdef DBG_CLASS
  #define SPUR_CLSS  SPUR
  #define TRACE_CLSS  TRACE
#else
  #define SPUR_CLSS(args)
  #define TRACE_CLSS(args)
#endif

#ifdef DBG_DRAWING
  #define SPUR_DRAW  SPUR
  #define TRACE_DRAW  TRACE
#else
  #define SPUR_DRAW(args)
  #define TRACE_DRAW(args)
#endif


/**+*************************************************************************\n
*  Real round(Real x)  -  rounding function template
*
*  Since not all <cmath>'s provides `round()', defining it by using of
*   `floor()' and `ceil()'. Note: return type == arg type!
*  The template form avoids ambiguity with contingent `round()' functions in
*   <cmath>, because these are non-templates ("full specialized") and have
*   higher priority.
******************************************************************************/
template <typename Real> 
Real round (Real x)  { return x < 0 ? floor(x) : ceil(x); }

//============================================================================
// self explanatory...
//============================================================================
template <class T>
T abs (const T& x) { return x < T() ? -x : x; }


static void
print_key_event()
{    
  printf("event_key() = \"%c\" (%d)\n", Fl::event_key(), Fl::event_key());
  printf("event_length = %d, event_text = \"%s\"\n", 
    Fl::event_length(), Fl::event_text());
  printf("Shift = %d,  Ctrl = %d,  Alt = %d\n",
    Fl::event_shift(), Fl::event_ctrl(), Fl::event_alt());
}

//============================================================================
//
// local non-member functions...
//
//============================================================================

/**+*************************************************************************\n
*  @param s: (I & O) string of a float formated via "%f" ("% f", ...).
*
*  Deletes in \a s all unsignificant zeros behind '.'!
******************************************************************************/
static void del_zeros_from_f_floatstr (char *s)
{  
  char* comma = strrchr(s,'.');
  if (!comma) return;               // nichts zu tun
  int p = strlen(s) - 1;            // letztes Zeichen vor '\0'
  while (s[p]=='0') p--;            // von hinten nach vorn
  
  if (s[p]=='.') s[p  ]=0;          // am Komma abschneiden
  else           s[p+1]=0;  // hinter letzter Kommastelle abschneiden   
} 

/**+*************************************************************************\n
*  @param s: (I & O) string of a float formated via "%E" ("% E", ...).
*
*  Deletes in \a s all unsignificant zeros between '.' and 'E'! If "0E+00"
*  (or " 0E+00" or "-0E+00" etc) would accrued, return "0" resp. " 0".
*  @todo strcpy() wird benutzt fuer ueberlappende Strings.
******************************************************************************/
static void del_zeros_from_E_floatstr (char *s)
{   
  char* comma = strrchr(s,'.');
  if (!comma) return;               // nichts zu tun
  char* E = strrchr(s,'E');
  if (!E) return;                   // not an "%E" format 
  char* p = E;
  while (*(--p)=='0');              // von hinten nach vorn
  
  if (*p=='.')                      // p = "_.000E___"
    if (p[-1]=='0') {               // s = "_0.000E___"
      p[0] = 0;                     // s = "_0.000E___" --> "_0"
      if (s[0]!='0') s[0] = ' ';    // s = "-0" --> " 0"
    }
    else strcpy(p,E);               // kopiert "E___" an '.' pos
  else                              // p = ".x000E___"
    strcpy(++p,E);                  // puts "E___" behind last non-'0' digit
} 


/*============================================================================
* 
*  @class AxisInfo
* 
*============================================================================*/
/**+*************************************************************************\n
*  update_minmax()
*  Dienst fuer Fl_Cartesius::update_minmax(). Liefert true zurueck, wenn sich 
*  was geaendert hat. Damit Funktion auch beim ersten Mal funktioniert, 
*  muessen min-max-Werte passend initialisiert sein (siehe init_minmax())! 
******************************************************************************/
bool AxisInfo::update_minmax (double w0, double w1)
{
  bool changed = false;
  if (w0 < wmin) { wmin = w0;  changed = true; }
  if (w1 > wmax) { wmax = w1;  changed = true; }
  return changed;
}

void AxisInfo::calc_map_coeff() 
{
  if (logarithm_) {
    log_wa = log(wa);
    
    if (a != b)  f = (log(wb) - log_wa) / (b - a);
    else         f = 0.0;      // eigentlich unendlich
  } 
  else {
    if (a != b)  f = (wb - wa) / (b - a);  
    else         f = 0.0;      // eigentlich unendlich
  }
  
  if (f != 0.0) inv_f = 1.0 / f;  else inv_f = 0.0;   // dito
}

double AxisInfo::value (int x) const 
{
  if (logarithm_)
    return exp (f * (x - a) + log_wa);  // Note: exp(a+b) = exp(a)*exp(b)
  else
    return (f * (x - a) + wa);
    //return f*x + const; // const = -f*a + wa
}

int AxisInfo::position (double w) const 
{
  if (logarithm_) 
    return int (round (inv_f * (log(w) - log_wa))) + a;
  else
    return int (round (inv_f * (w - wa))) + a;  
    //return int(round(inv_f * w + const));  // const = -invf*wa + a
}

/**+*************************************************************************\n
*  AxisInfo weiss am besten, was `dw' im Log-Falle bedeutet...
*   Akkurat erst nach adjust_ticks_and_world()!!
******************************************************************************/
double AxisInfo::tickvalue (int index) const
{
  if (logarithm_) return exp (index * dw);      // wohl kaum
  else            return index * dw;
}
  
/**+*************************************************************************\n
*  So, wenn alle Ticks zu zeichnen, fehlt aber noch die eigentliche
*   Tick-Distanz, es sollen entsprechend der Groessenordnung ja Werte
*   ausgelassen werden, also ...(aehm) 
*  Aber dann ist das Funktionsargument `i' hier unbrauchbar, die Schleife
*   beim Anwender (wie Grid) soll ja nicht "von i=-10 to 20" (step 1) laufen,
*   sondern nur ueber die zu zeichnenden Ticks! Das schliessliche "mod 3"
*   bleibt natuerlich, ist es nicht "arg / 3". 
******************************************************************************/
double AxisInfo::tickvalue_log (int i) const    // NOTE: not used
{
  int N = n_tick_values_log;
  if (i >= 0) 
        return exp ((i+2)/N * tick_values_log[ (i+2) % N ]);
  else  return exp ((i-2)/N * tick_values_log[ (2-i) % N ]);
}


/**+*************************************************************************\n
*  dtick_by_pbt()  -  "by pixel between ticks", helper for adjust..()
* 
*  @param wa,wb: world coords to \a a and \a b, which should plot there.
*    (adjust..() calls it with AxisInfo::wa|wb AS WELL AS ~::wmin|wmax, 
*    so other name could be clearer.)
*  @return: distance between ticks in world coords
* 
*  For given window positions \a a,b and arguments \a wa,wb find smallest
*  allowed tick distance \a dtick, i.e. so that are no less than \a pbt_min
*  pixels between ticks. "Allowed tick distances" are values from the 
*  array \a tick_steps.
* 
*  NOTE: As parameter would suffices the difference wb-wa.
*   Einzelwerte im Moment genutzt noch von Debugausgaben.
* 
*  dw     log(dw) floor(logdw) logdw-floor  10^0.43 = dig1
*  0.027  -1.56..   -2           0.43..       2.7
*  0.27   -0.56..   -1           0.43..       2.7
*  2.7     0.43..    0           0.43..       2.7 
*  27      1.43..    1           0.43..       2.7
******************************************************************************/
double AxisInfo::dtick_by_pbt (double wa, double wb)  
{
  TRACE_TICK(("...\n"))
  assert (a!=b);
  
  double q = double(b - a) / pbt_min_;  // q=0 if a==b! 
  double dw = (wb - wa) / q;            // Assume q!=0, ie. a!=b. Ok?
  
  assert (dw != 0.0);                   // Assume wa!=wb
  
  double logdw;
  if (dw > 0) logdw = log10(dw); 
  else        logdw = log10(-dw); 
  
  double logdw_floor = floor (logdw);      
  double dig1        = pow (10., logdw - logdw_floor);   // in [1,10)
  
  SPUR_TICK(("\tworld=[%f, %f], win=[%d, %d], pbt_min=%d\n", 
    wa, wb, a, b, pbt_min_));
  SPUR_TICK(("\tq=%f, dw=%f, logdw=%f, logdw_floor=%f\n", 
    q, dw, logdw, logdw_floor));
  
  // find smallest tick distance not smaller than dig1...
  double dtick = tick_steps[0];            
  int i;
  for (i=0; i < n_tick_steps; i++)     
    if (dig1 <= tick_steps[i]) {           
      dtick = tick_steps[i];               
      break;
    }
  if (i == n_tick_steps)        // No match: dig1 > tick_steps[last].
    logdw_floor += 1.0;         // And dtick=steps[0] from init above
   
  SPUR_TICK(("\tdig1 = %f, dtick = %f\n", dig1, dtick));
  
  dtick *= pow(10., logdw_floor);
  
  if (dw < 0.0) dtick = -dtick;
  
  SPUR_TICK(("\treal dtick =% f\n", dtick));
  SPUR_TICK(("\twa / dtick =% f, floor=% f  floor*dtick=% f\n", wa/dtick,
    floor(wa/dtick), floor(wa/dtick) * dtick));     
  SPUR_TICK(("\t\t\t        ceil=% f   ceil*dtick=% f\n", 
    ceil(wa/dtick), ceil(wa/dtick) * dtick));
  SPUR_TICK(("\twb / dtick =% f, floor=% f  floor*dtick=% f\n", wb/dtick,
    floor(wb/dtick), floor(wb/dtick)*dtick));     
  SPUR_TICK(("\t\t\t        ceil=% f   ceil*dtick=% f\n", 
    ceil(wb/dtick), ceil(wb/dtick) * dtick));

  return dtick;
}

/**+*************************************************************************\n
*  Simple one by one transmission of linear case to log case, poor result.
* 
*  @param wa,wb: log(wa), log(wb), Basis e! 
******************************************************************************/
double AxisInfo::dtick_by_pbt_log (double wa, double wb)  
{
  TRACE_TICK(("...\n"))
  assert (a!=b);
  
  double q = double(b - a) / pbt_min_;  // q=0 if a==b! 
  double dw = (wb - wa) / q;            // Assume q!=0, ie. a!=b. Ok?
  
  assert (dw != 0.0);                   // Assume wa!=wb
  
  double logdw;
  if (dw > 0) logdw = log(dw); 
  else        logdw = log(-dw);         
  
  double logdw_floor = floor (logdw);      
  double dig1        = exp (logdw - logdw_floor);   // in [1,e)
  
  SPUR_TICK(("\tLog-world=[%f, %f], win=[%d, %d], pbt_min=%d\n", 
    wa, wb, a, b, pbt_min_));
  SPUR_TICK(("\tq=%f, dw=%f, log(|dw|)=%f, floor(..)=%f\n", 
    q, dw, logdw, logdw_floor));
  SPUR_TICK(("\tdig1 = exp(log(|dw|) - floor(..)) = %f\n", dig1));
  
  // find allowed tick distance not smaller than dig1...
  double dtick = tick_log_steps[0];
  int i;
  for (i=0; i < n_tick_log_steps; i++) 
  { SPUR_TICK(("\ti=%d,  tickvals=%f,  dig1-tickvals = %e\n",
      i, tick_log_steps[i], dig1 - tick_log_steps[i]))   

    if (tick_log_steps[i] >= dig1) { 
      dtick = tick_log_steps[i];     
      break;
    }
  }
  SPUR_TICK(("\tnach Schleife: dtick = %f, i=%d\n", dtick, i));
  
  if (i == n_tick_log_steps)    // No match: dig1 > tick_log_steps[last].
    logdw_floor += 1.0;         // And dtick=steps[0] from init above
    
  SPUR_TICK(("\tnach Korrektur = %f,  log_floor=%f\n", dtick, logdw_floor));
  
  dtick *= exp (logdw_floor);
  
  if (dw < 0.0) dtick = -dtick;
  
  SPUR_TICK(("\treal dtick =% f\n", dtick));
  return dtick;
}

/**+*************************************************************************\n
*  adjust_ticks_and_world()  -  helper of Fl_Cartesius::justify()
* 
*  Adjust the values `dw', `i0' & `i1' and (if SCALE_AUTO) the 'wrld'-values
*   `wa' & `wb'. Depending on scale_mode start hereby either with MinMax-values
*   (SCALE_AUTO) or 'wrld'-values (SCALE_FIX). Leave an updated world-win-map, 
*   i.e. calc_map_coeff() should be called.
* 
*  SCALE and ARRANGE, MinMax datas and 'wrld' datas:
* 
*   Scale=FIX means: plot with a fix world-win-map regardless how well data 
*   fit into 'win'. 
* 
*   Scale=AUTO means: use (and find) a world-win-map, that fits data optimal
*   into 'win'. Only for Scale=AUTO the MinMax-values are needed!
* 
*   What means "optimal"? Here for Scale=AUTO we have a further selection:
* 
*   Arrange=WRAP means: determine 'wrld' so, that the plotted objects are 
*   wrapped by allowed ticks; ordinarily the 'wrld' values will become 
*   by this greater than MinMax-values.
* 
*   Arrange=EXACT means: MinMax-values match exactly with 'wrld'-values;
*   ordinarily ticks now don't cover the full plot area and ends inside.
******************************************************************************/
void AxisInfo::adjust_ticks_and_world() 
{
  if (logarithm_) adjust_ticks_and_world_log();
  else            adjust_ticks_and_world_lin();
}

void AxisInfo::adjust_ticks_and_world_lin ()
{
  if (scale_mode_==SCALE_AUTO)          // TODO: discards wa>wb
  { 
    // Starting points are MinMax-values (Have to be set before!)
    
    if (arrange_mode_==ARRANGE_WRAP)
    { 
      dw = dtick_by_pbt (wmin, wmax);
 
      i0 = (int)floor( wmin/dw );       // not trunc() 
      i1 = (int)ceil( wmax/dw );        // not trunc()  
      
      new_world (i0 * dw, i1 * dw);     // sets wa,wb and calls calc_map()
    }
    else  // arrange_mode_==ARRANGE_EXACT
    {
      dw = dtick_by_pbt (wmin, wmax);
      
      i0 = (int)ceil( wmin/dw );
      i1 = (int)floor( wmax/dw );
  
      new_world (wmin, wmax);       // match 'wrld' exactly to MinMax
    }
  }
  else  // scale_mode_==SCALE_FIX
  {
    // Starting points are `wrld' coords wa, wb. (Have to be set 
    // before, we rely on it.) MinMax here irrelevant.
    
    dw = dtick_by_pbt (wa, wb);
     
    if (wa*dw < wb*dw) {            // dw < 0 considering
      i0 = (int)ceil( wa/dw ); 
      i1 = (int)floor( wb/dw ); 
    } 
    else {
      i0 = (int)floor( wa/dw ); 
      i1 = (int)ceil( wb/dw ); 
    }
    
    // No "new_world(...)" here, we must not change world values wa,wb!
    //   But 'win' coords could be changed by calling function justify() and
    //   needs nevertheless a map_coeff-recalculation. In the other branches
    //   it is done implicitly by new_world(), here we do it explicitly...
    calc_map_coeff();
  }
}

/**+*************************************************************************\n
*  Simple one by one transmission of linear case to log case, poor result.
******************************************************************************/
void AxisInfo::adjust_ticks_and_world_log ()
{
  if (scale_mode_==SCALE_AUTO)          // TODO: discards wa>wb
  { 
    // Starting points are MinMax-values (Have to be set before!)
    
    double log_wmin = log(wmin);
    double log_wmax = log(wmax);
    
    if (arrange_mode_==ARRANGE_WRAP)
    { 
      dw = dtick_by_pbt_log (log_wmin, log_wmax);
 
      i0 = (int)floor( log_wmin/dw );   // not trunc() 
      i1 = (int)ceil( log_wmax/dw );    // not trunc()  
      
      new_world (exp(i0*dw), exp(i1*dw));
    }
    else  // arrange_mode_==ARRANGE_EXACT
    {
      dw = dtick_by_pbt_log (log_wmin, log_wmax);
      
      i0 = (int)floor( log_wmin/dw );
      i1 = (int)ceil( log_wmax/dw );
  
      new_world (wmin, wmax);       // match 'wrld' exactly to MinMax
    }
  }
  else  // scale_mode_==SCALE_FIX
  {
    // Starting points are `wrld' coords wa, wb. (Have to be set 
    // before, we rely on it.) MinMax here irrelevant.
    
    double log_wa = log(wa);
    double log_wb = log(wb);
    
    dw = dtick_by_pbt_log (log_wa, log_wb);
     
    if (log_wa*dw < log_wb*dw) {    // dw < 0 considering
      i0 = (int)ceil( log_wa/dw ); 
      i1 = (int)floor( log_wb/dw ); 
    } 
    else {
      i0 = (int)floor( log_wa/dw ); 
      i1 = (int)ceil( log_wb/dw ); 
    }
    
    // No "new_world(...)" here, we must not change world values wa,wb!
    //   But 'win' coords could be changed by calling function justify() and
    //   needs nevertheless a map_coeff-recalculation. In the other branches
    //   it is done implicitly by new_world(), here we do it explicitly...
    calc_map_coeff();
  }
}

/**+*************************************************************************\n
*  @param s: Can be a short string like "X:" in front of the tab or a longer
*    one on an own line; then a '\n' is to include.
******************************************************************************/
void AxisInfo::print(const char* s) const
{
  if (abs(wmin) > 1e8 || abs(wmax) > 1e8) 
       printf("%s\tMinMax = [% e, % e]\n", s, wmin,wmax);
  else printf("%s\tMinMax = [% f, % f]\n", s, wmin,wmax);
  
  printf("\t world = [% f, % f]  dw=%f, i0=%d, i1=%d\n", wa, wb, dw, i0, i1);
  printf("\t   win = [%3d, %3d]\tf=%f,  inv_f=%f\n", a, b, f, inv_f);
}


/**===========================================================================
* 
*  @class CoordMarker
*
*  Helper class for Fl_Cartesius.
*  Koennte auch innerhalb Fl_Cartesius definiert werden, da nur benutzt 
*  in handle() zur Emanation eines CoordMarkers.
*  
*  Da ausschliesslich kreiert in Cartesius nach erfolgtem justify(), kam die
*  Idee auf, die Widget-Groesse auf das Plot-Gebiet zu beschraenken. Aber keine
*  gute, da spaetere justify() dieses Gebiet aendern koennen und was ist dann
*  mit unserer Groesse?! Schon besser, ein dem Schriftzug gemaesses Rechteck um
*  (wx,wy), denn das waere kontextbezogen. 
* 
*  @todo Aendern Cartesius-Kinder den Font, wird auch hier so geschrieben.
*   Entweder hier eigene Fontvariable; oder (1) Cartesius erklaert 'mich' 
*   zum Freund und ueber cartes() erhalte 'ich' Zugriff auf Achsenfont; oder
*   (2) Cartesius veroeffentlicht die entsprechenden Variablen.
*   Zur Zeit (1) verwirklicht.
* 
*============================================================================*/
class CoordMarker : public CartesObject
{
  double wx_, wy_;
  char string_[32];     // same size as `str_coord_picker'!

public:
  CoordMarker (double wx, double wy, const char* s) 
   : wx_(wx), wy_(wy) 
   { 
     TRACE_CLSS(("...\n\tcurrent=%p, parent()=%p, window()=%p\n",
       Fl_Group::current(), parent(), window()))
     
     strcpy(string_, s);        // and if (s==0)?
     label(string_);            // useful for debug output
     
     SPUR_CLSS(("\tstring_=\"%s\", x=%d, y=%d, w=%d, h=%d\n", 
       string_, x(), y(), w(), h()))
   }
  
protected:    
  void draw();
};

void CoordMarker::draw()
{ 
  int x = x_from_wx (wx_);
  int y = y_from_wy (wy_);
  fl_color (FL_BLACK);
  fl_line_style (FL_SOLID,0);   // draw a cross 
  fl_xyline (x-3, y, x+3);      
  fl_yxline (x, y-3, y+3);
  // falls CoordMarker Cartesius' Freund ist...
  fl_font (cartes()->font_face_, cartes()->font_size_);
  fl_draw (string_, x+2, y-2);
}


/**===========================================================================
* 
*  @class Fl_Cartesius
* 
*  Obacht: Fl_Cartesius nicht mehr abgeleitet von Fl_Window, d.h. die LO-Ecke
*    ist nicht mehr (0,0), sondern jetzt (x(),y()) und die RU-Ecke nicht mehr
*    (w(),h()), sondern (x()+w(), y()+h())! Wichtig in justify().
* 
*  FRAGE: Fensterbreite und -hoehe != 0 darf fuer das Fl_Window sicher voraus-
*    gesetzt werden. Kontrollieren im Konstruktor sowie resizebox()! Aber
*    wollen wir auch __X.b >__X.a usf. voraussetzen? Hier spielt die
*    Schriftbreite hinein. Man koennte es immerhin so einrichten. 
*  
*  Note: Ctor ends with an empty_look(true), i.e. an [0,1] x [0,1] plot area.
*    Adding of objects using the min-max-machinery needs thus an init_minmax()
*    before! Or we set_minmax(...) of course.
* 
*  The coord picker shall "pick" always a mouse is moving over the visible 
*    plot area, even if the plot window has not the focus. To avoid redrawing
*    *all* if only the coord picker is to update, I (ab)use the FL_DAMAGE_USESR1
*    flag of the FL_DAMAGE mask: send a `damage(FL_DAMAGE_USER1)' in handle()
*    and react correspondingly in draw().
*
*  @todo Should "pick" also, if a *pushed* mouse is moved over the area.
*    (like in gnuplot)
* 
*============================================================================*/
Fl_Cartesius::Fl_Cartesius (int X, int Y, int W, int H, const char* la)
  : Fl_Group (X,Y,W,H, la),
    __X(), __Y()
{
  current_ca_        = this;
  
  color (FL_GRAY);                  // bg color (or FL_LIGHT*)
  axisfont( fl_font(), 12 );        // default: aktueller Font
  box (FL_DOWN_BOX);                // while under development only
  reticking_under_resizing_ = false;
  rejustify_         = true;
  show_minmax_       = false;
  grid_mode_         = GRID_NONE;
  grid_linestyle_    = FL_SOLID;
  color_axis_        = FL_BLACK;
  color_grid_        = FL_WHITE;
  axis_linewidth_    = 2;
  *coord_picker_str_ = 0;
  empty_look_str_    = empty_look_default_str_;
  add_  = (axis_linewidth_ / 2) + 1; // More exact: linewidth of *frame*;
  add1_ = add_ - 1;                  // As for positioning of frame;
  
  empty_look (true);
  justify();
}

void Fl_Cartesius::axisfont (int face, int size)
{
  fl_font (face, size);  
  font_face_    = face;
  font_size_    = size;
  font_height_  = fl_height();
  font_descent_ = fl_descent();
}

/**+*************************************************************************\n
*  Rueckgabestrings sind gueltig bis zum naechsten tick_str()-Aufruf.
*   (Sollte x|y_tick_str() nicht statische Funktionen werden?)
*  Bei Formatfestlegung keep in mind, dass es glatte Zahlen sind. 
*  Idee: Sollte Zahlenformat fuer ganze Achse einheitlich entweder %f oder %E 
*   sein, nicht zwischendurch (beim Eintritt in einen neuen Groessenbereich) 
*   wechseln? Format waere so von wa() und wb() abhaengig, nicht von `w'. 
*   Koennte daher vorab festgelegt werden bei der minmax bzw. world-Setzung. 
*   Wirklich nur dort? Was bei `scale_'- und `arrange_mode_'-Aenderungen?
******************************************************************************/
const char* Fl_Cartesius::x_tick_str (double w) const
{
  static char s[16];
  if (abs(w) < 1E-3) {
    sprintf (s, "%.3E", w);      // "%E"-format secures length < 16
    del_zeros_from_E_floatstr (s);
  } 
  else if (abs(w) < 1E+4) {
    snprintf (s, 10, "%f", w);   // cut at 9 digits
    del_zeros_from_f_floatstr (s);
  }
  else {  // abs(w) >= 1E+4 
    sprintf (s, "%.3E", w);      // "%E"-format secures length < 16
    del_zeros_from_E_floatstr (s);
  }  
  return s;
}

const char* Fl_Cartesius::y_tick_str (double w) const
{
  static char s[16];
  if (abs(w) < 1E-3) {
    sprintf (s, "%.3E", w);      // "%E"-format secures length < 16
    del_zeros_from_E_floatstr (s);
  } 
  else if (abs(w) < 1E+5) {
    snprintf (s, 10, "%f", w);   // cut at 9 digits
    del_zeros_from_f_floatstr (s);
  }
  else {  // abs(w) >= 1E+5 
    sprintf (s, "%.3E", w);      // "%E"-format secures length < 16
    del_zeros_from_E_floatstr (s);
  }  
  return s;
}

/**+*************************************************************************\n
*  Formatiert `coord_picker_str_' aus Weltkoordinaten x,y. 
*   Wenigstens 4 Stellen Genauigkeit werden immer gezeigt.
******************************************************************************/
void Fl_Cartesius::mk_coord_picker_str (double x, double y)
{
  char* s = coord_picker_str_;
  //if (abs(x) < 1E-4) sprintf (s,     "% .3E", x); else 
  //if (abs(x) < 1E-3) snprintf(s, 11, "% .9f", x); else  // cut off
  if (abs(x) < 1E-3) sprintf (s,     "% .3E", x); else 
  if (abs(x) < 1E+9) snprintf(s, 10, "% .8f", x); else  // cut off
  /*      >= 1E+9 */ sprintf (s,     "% .3E", x);
  
  s = & coord_picker_str_[strlen(s)];
  //if (abs(y) < 1E-4) sprintf (s,     ",  % .3E", y); else 
  //if (abs(y) < 1E-3) snprintf(s, 14, ",  % .9f", y); else  // 11+3=14
  if (abs(y) < 1E-3) sprintf (s,     ",  % .3E", y); else 
  if (abs(y) < 1E+9) snprintf(s, 13, ",  % .8f", y); else  // 10+3=13
  /*      >= 1E+9 */ sprintf (s,     ",  % .3E", y);
}

/**+*************************************************************************\n
*  handle()  
*   Call Fl_Group::handle() first of all.
******************************************************************************/
int Fl_Cartesius::handle (int event)
{
#ifdef DBG_EVENT
  static int counter;
  printf("%d ", counter++);  fl_print_event(event,"", label()); 
#endif
  
  static int x_push, y_push, xb2, yb2; 
  static int savefont = fl_font();     // fuer eine Kulanzleistung
  static int savesize = fl_size();  

  int ret = Fl_Group::handle(event); 
  switch (event)
  {
  case FL_MOVE:
    mk_coord_picker_str(wx_from_x(Fl::event_x()), wy_from_y(Fl::event_y()));
    //draw_coord_picker();      // now via the damage() channel...
    damage (FL_DAMAGE_USER1);   // my code word for "draw only coord picker"
    return 1;
  
  case FL_ENTER:
    fl_cursor(FL_CURSOR_CROSS);
      // Fl_Window::cursor() automatisch zurueck bei LEAVE, fl_cursor() nicht
    savefont = fl_font();  // restore it in FL_LEAVE (Kulanzleistung)
    savesize = fl_size();  // ditto
    return 1;
  
  case FL_PUSH:
    x_push = Fl::event_x();
    y_push = Fl::event_y();
    //printf("PUSH, button=%d (%d,%d)\n", Fl::event_button(), x_push,y_push);
    if (Fl::event_button()==2) { xb2 = x_push;  yb2 = y_push; }
    return 1;
  
  case FL_DRAG:
    //printf("DRAG, button=%d (%d,%d)\n", Fl::event_button(), Fl::event_x(), Fl::event_y());
    return 1;

  case FL_RELEASE:
    //printf("RELEASE, button=%d (%d,%d)\n", Fl::event_button(), 
    //  Fl::event_x(), Fl::event_y());
    if (Fl::event_button()==2) 
      if (xb2 == Fl::event_x() && yb2 == Fl::event_y()) {
        printf("CoordMarker!\n");
        //mk_coord_picker_str(wx_from_x(xb2), wy_from_y(yb2));
        // ^^^ unnecessary, as every RELEASE foregoes a MOVE!
        current_cartes(this);   // secures, that 'I' the active Cartesius
        new CoordMarker(wx_from_x(xb2), wy_from_y(yb2), coord_picker_str_);
        redraw();  
      }
    return 1;  // dh alle RELEASE events damit hier fuer bearbeitet erkl.

  case FL_SHORTCUT:     
  case FL_KEYDOWN:
    if (Fl::event_key() == 'l') {
      if (Fl::event_state() & FL_SHIFT) {
        if (ymin()<=0 || ymax()<=0) return 1;
        if (__Y.logarithm()) __Y.logarithm(false); else __Y.logarithm(true);
        justify(); redraw(); return 1;
      }
      else if (Fl::event_state() & FL_CTRL) {
        if (xmin()<=0 || xmax()<=0) return 1;
        if (__X.logarithm()) __X.logarithm(false); else __X.logarithm(true);
        justify(); redraw(); return 1;
      }
    }
    if (Fl::event_state() & FL_SHIFT); else
    if (Fl::event_state() & FL_ALT); else
    if (Fl::event_state() & FL_CTRL);
    else
    { switch (Fl::event_text()[0])
      {
      case '+': zoom (1./sqrt(2.)); return 1; // calls justify() + redraw()
      case '-': zoom (sqrt(2.)); return 1;    
      
      case 'g': 
        if (grid_mode_==GRID_NONE)   gridmode(GRID_XLINES); else
        if (grid_mode_==GRID_XLINES) gridmode(GRID_YLINES); else
        if (grid_mode_==GRID_YLINES) gridmode(GRID_XYLINES); 
        else                         gridmode(GRID_NONE);
        redraw(); return 1;
      
      case 's':  show_minmax_ = !show_minmax_;  redraw();  return 1;
      }
    }
    break;     // nicht bearbeitet -> return Fl_Group::handle()   
    
  case FL_KEYUP:
    break; printf("KEYUP\n"); 
    
  case FL_LEAVE: 
    // Something to reset?...
    fl_cursor(FL_CURSOR_DEFAULT);
    fl_font (savefont, savesize);
    return 1;
  }
  
  return ret;
}

/**+*************************************************************************\n
*  Diskussion der Clipping-Problematik in "Cartesius_Notizen.html".
******************************************************************************/
void Fl_Cartesius::draw()
{ 
  DBG_CALL(printf("draw()-in...\n"); /*fl_print_damages (this);*/ );

  // Check if only the coord picker is to update (before Fl_Group::draw(), 
  //  which clears all!) Note: draw_coord() doesn't need win-world-map... 
  
  if (damage() == FL_DAMAGE_USER1) { // `&'-test fails (MOVE-Problem)
    draw_coord_picker();            
    TRACE_DRAW(("only coord_picker\n"))
    return;
  }
  
  if (rejustify_) justify();    // Do it before draw_children(), so that 
                                //  children have the right map too
  
  // Fl_Group::draw() + clipping around children drawing...
  if (damage() & ~FL_DAMAGE_CHILD) { // redraw the entire thing:
    TRACE_DRAW(("the entire thing, x=%d, y=%d, w=%d, h=%d\n", x(),y(),w(),h()))
    draw_box();
    draw_label();
  }
  TRACE_DRAW(("draw children...\n"))
  fl_push_clip (left(), top(), width(), height());    
  draw_children();

  // Reset our font for empty_string, grid and axis. Setting in FL_ENTER not 
  //  enough, because children could have changed them.
  fl_font (font_face_, font_size_);  
  
  if (empty_look_ && empty_look_str_) 
  {
    int x,y,w,h;
    w = (int)fl_width(empty_look_str_);
    h = fl_height();
    x = (__X.a + __X.b - w) / 2;
    y = (__Y.a + __Y.b) / 2;    // baseline of the font
    fl_color (FL_WHITE); fl_rectf(x-3, y-h, w+6, h+fl_descent()+4);
    fl_color (FL_BLACK); fl_draw(empty_look_str_, x,y);
  }
  fl_pop_clip();  
  // ...End of Fl_Group::draw()
    
  draw_grid();
  draw_axis();
  if (show_minmax_) show_minmax_area();                   
  
  // Redraw (last) picker string always, even no acute mouse moving occurs
  draw_coord_picker();
  
  fl_line_style(0);     // Reset line style for FLTK's own drawings (needed!)
}


/**+*************************************************************************\n
*  Ziel: Groessenaenderungen nur dem Plotgebiet mitteilen, nicht dem Bereich
*    der Achsenbeschriftungen. Ein `justify()' erledigt das automatisch.
*  
*  @todo Secure, that plot area never becomes zero dimensions.
******************************************************************************/
void Fl_Cartesius::resize(int X, int Y, int W, int H)
{ 
  int dX = X-x();
  int dY = Y-y();
  int dW = W-w();  
  int dH = H-h();
  TRACE(("X=%d, Y=%d, W=%d, H=%d\n", X,Y,W,H))
  TRACE(("dX=%d, dY=%d, dW=%d, dH=%d\n", dX,dY,dW,dH))
  
  Fl_Group::resize(X,Y,W,H);  // --> changes x(),y(),w(),h()
  
  if (reticking_under_resizing_) 
  {  
    justify();      // with the new x(),y(),w(),h()!
  }
  else {
    // If "reticking_under_resize' false, then the ticks remain frozen (no 
    //   re-calc., `i0' and `i1' remain) - this seems the behavior also of
    //   gnuplot. Because thus the ticking area (the distance between plot area
    //   and widget borders) keeps invariant, an `justify()' isn't needed.
    //   Needed are only to re-assign the plot area  and to re-calc the
    //   wrld-win-map. The new plot area points a' and b' we get by the
    //   conditions
    //     x() - a =(!) X - a'  i.e  a' = a + X-x()  =  a + dX
    //   and
    //     x() + w() - b =(!) X + W - b'  i.e.  
    //     b' = b + X-x() + W-w()  =  b + dX + dW
    
    __X.new_win(__X.a + dX, __X.b + dX+dW);
    __Y.new_win(__Y.a + dY, __Y.b + dY+dH); 
  }
  
  //redraw();   // wird schon aufgerufen, wuesste nur gern, von wem
}

void Fl_Cartesius::draw_x_axis()
{
  fl_xyline(__X.a-add_, __Y.a-add1_, __X.b+add_);   // top x-Axis
  fl_xyline(__X.a-add_, __Y.b+add_,  __X.b+add_);   // bottom x-Axis
  
  DBG_CALL(  
    fl_color(FL_GREEN); fl_point (__X.a+10,__Y.b+add_);
    fl_color(FL_RED);   fl_point (__X.a+11,__Y.b); 
    fl_color(FL_GREEN); fl_point (__X.a+10,__Y.a-add1_);
    fl_color(FL_RED);   fl_point (__X.a+11,__Y.a); 
    fl_color(FL_BLUE);  fl_xyline (__X.a, __Y.a-4, __X.b); 
    fl_color(FL_RED);   fl_xyline (__X.a-add_, __Y.a-7, __X.b+add_+1);
    fl_color(FL_BLUE); 
    fl_yxline (__X.a+15, __Y.a,       __Y.a       + tick_len_);
    fl_yxline (__X.a+15, __Y.b+add1_, __Y.b+add1_ - tick_len_);  
    fl_color(FL_BLACK); 
  )
  
  for (int i=__X.i0; i <= __X.i1; i++)  
  {                                         
    double wx = __X.tickvalue (i);
    int xx  = x_from_wx (wx);
    
    SPUR_TICK(("\tx-tick at wx=% f\t  x=%3d\n", wx,xx));
    
    fl_yxline(xx, __Y.a      , __Y.a       + tick_len_);  // top ticks
    fl_yxline(xx, __Y.b+add1_, __Y.b+add1_ - tick_len_);  // bottom ticks

    const char* s = x_tick_str( wx );
    int s_w = (int)fl_width(s);
                                
    // old: text at the lower x-axis... (x-centered)
    //fl_draw (s, xx - s_w/2, __Y.b + fl_height() + extraspace_); 
    
    // new: dito with right border correction...
    xx -= s_w/2;
    if (xx + s_w > x()+w()) xx = x()+w() - s_w;        
    fl_draw (s, xx, __Y.b + fl_height() + add1_ + extraspace_); 
  }
}

void Fl_Cartesius::draw_y_axis()
{
  fl_yxline(__X.a-add1_, __Y.a-add_, __Y.b+add_+1);   // left y-Axis
  fl_yxline(__X.b+add_,  __Y.a-add_, __Y.b+add_+1);   // right y-Axis
  
  DBG_CALL(
    fl_color(FL_GREEN); fl_point (__X.a-add1_,__Y.a+10);
    fl_color(FL_RED);   fl_point (__X.a,      __Y.a+11); 
    fl_color(FL_GREEN); fl_point (__X.b+add_, __Y.a+10);
    fl_color(FL_RED);   fl_point (__X.b,      __Y.a+11); 
    fl_color(FL_BLUE);  fl_yxline (__X.a-4, __Y.a, __Y.b); 
    fl_color(FL_RED);   fl_yxline (__X.a-7, __Y.a-add_, __Y.b+add_+1);
    fl_color(FL_BLUE); 
    fl_xyline (__X.a,       __Y.a+15, __X.a       + tick_len_);
    fl_xyline (__X.b+add1_, __Y.a+15, __X.b+add1_ - tick_len_);  
    fl_color(FL_BLACK); 
  )

  for (int i=__Y.i0; i <= __Y.i1; i++)  
  {                                             
    double wy = __Y.tickvalue (i);
    int yy = y_from_wy (wy);
    
    SPUR_TICK(("\ty-tick at wy=% f\t  y=%3d\n", wy,yy));
    
    fl_xyline(__X.a,       yy, __X.a       + tick_len_);  // left ticks
    fl_xyline(__X.b+add1_, yy, __X.b+add1_ - tick_len_);  // right ticks

    const char* s = y_tick_str( wy );
    int s_w = (int)fl_width(s);         
    
    // text at the left y-axis... (y-centered)
    fl_draw (s, __X.a - add1_ - s_w - fl_descent() - extraspace_, 
                yy + fl_height()/2 - fl_descent()); 
  }
}

void Fl_Cartesius::draw_axis()
{
  fl_color(color_axis_);
  fl_line_style(FL_SOLID, axis_linewidth_);
  draw_x_axis();
  draw_y_axis();
}


/**+*************************************************************************\n
*  Note: Doesn't need world->win-map, as using only absolute window coords.
*    Changes due to base change 'Fl_Window -> Fl_Group': add "x()" and "y()"
*  @todo The y in `fl_draw(s,x,y)' is the baseline of a font, concern it!
*  @todo Concern border width more accurately. (currently 2 pixel fix)
******************************************************************************/
void Fl_Cartesius::draw_coord_picker()
{
  if (*coord_picker_str_ != 0) {
    fl_font(font_face_, font_size_);
    fl_color(color());    // clear with bg color of window
    fl_rectf(x()+2, y()+h()-2-fl_height(), 160, fl_height());
    fl_color(FL_BLACK); 
    fl_draw(coord_picker_str_, x()+2, y()+h()-4); // -4 statt -2 simul. Baseline
  }
}


/**+*************************************************************************\n
*  justify()        or  "adjust"?
* 
*  TASK: I want to have tick numbers below the below x-axis and left 
*   beside the left y-axis. The length of the y-numbers (i.e. their values)
*   affects the x-dim ('dx-win') of the plot area 'win'. Contrawise the
*   length of the x-numbers does NOT affect the dy-win, because for `dy-win'
*   only the text HEIGHT is relevant and this is independent of any values. 
*   So we can proceed as follows:
*  
*  Algorithm:
*   1. Determine the y-dim of 'win' (independent of data values).
*   2. With `dy-win' determine the ticks (and ticks values) along the
*      y-axis. 
*   3. Therewith find the max length of y-numbers and determine `dx-win'.
*   4. With `dx-win' determine the ticks along the x-axis.
* 
*  The plot-font (axisfont) is setting in draw() before any drawing. For the
*   calculation here we use our intern font infos and are not reliant on
*   that in the "justify() moment" the current FLTK font is the axisfont.
******************************************************************************/
void Fl_Cartesius::justify()     // justieren
{ 
  // 1. Determine `dy-win'...
  
  __Y.set_a( y() + font_height_ - font_descent_); // halbe Zeichenhoehe frei  
  __Y.set_b( y() + h() - 3 * font_height_);       // space for coord picker
  
  // 2. Determine the ticks along the y-axis...
  
  __Y.adjust_ticks_and_world();         // -> i0,i1,dw
  
  // 3. From y-ticks get max length of y-numbers and determine 'dx-win'...
  
  int len0 = (int)fl_width( y_tick_str(__Y.tickvalue_0()) );
  int len1 = (int)fl_width( y_tick_str(__Y.tickvalue_1()) );
  
  __X.set_a( x() + std::max(len0,len1) + 20 ); // "+20" vorlaeufig
  __X.set_b( x() + w() - 10 );                 // "-10" vorlaeufig

  // 4. Determine the ticks along the x-axis...
  
  __X.adjust_ticks_and_world();
  
  rejustify_ = false;                   
}


/**+*************************************************************************\n
*  update_minmax()   
* 
*  Gedacht fuer sich einreihende Kinder, die ihre MinMax-Daten mitteilen.
******************************************************************************/
void Fl_Cartesius::update_minmax (double X0, double X1, double Y0, double Y1)
{
  bool changed_x = __X.update_minmax (X0,X1);
  bool changed_y = __Y.update_minmax (Y0,Y1);
  
  //TRACE(("x-minmax(): %f,  %f\n", x_min(), xmax()))
  //TRACE(("y-minmax(): %f,  %f\n", y_min(), ymax()))

  if (changed_x && (__X.scale_mode()==SCALE_AUTO)  || 
      changed_y && (__Y.scale_mode()==SCALE_AUTO))
    rejustify_ = true;  // ansonsten bleibt alter Wert
}

/**+*************************************************************************\n
*  @todo In dieser Primitivversion zoomt es immer um den Fixpunkt (0,0).
*   Funktioniert so auch nur fuer SCALE_AUTO, da SCALE_FIX `wrld'-Werte
*   betrachtet. Ferner wird die intendierte Funktion von min-max, die
*   Grenzen der Weltobjekte zu speichern, hier zerstoert. Vielleicht sollte
*   man Zoomen nur mittes SCALE_FIX und `wrld'-Werten.
******************************************************************************/
void Fl_Cartesius::zoom (double f)
{
  set_minmax (f*xmin(), f*xmax(), f*ymin(), f*ymax());
  justify();
  redraw();
}


void Fl_Cartesius::draw_x_gridlines()
{
  for (int i=__Y.i0; i <= __Y.i1; i++)
  { 
    //if (i==0) continue;  // Null auslassen nur wenn Nullachsen
    fl_xyline (left(), y_from_wy(__Y.tickvalue(i)), right());
  }  
}
void Fl_Cartesius::draw_y_gridlines()
{
  for (int i=__X.i0; i <= __X.i1; i++)
  { 
    //if (i==0) continue;  // Null auslassen nur wenn Nullachsen
    fl_yxline (x_from_wx(__X.tickvalue(i)), top(), down());
  }  
}
void Fl_Cartesius::draw_grid()
{
  if (grid_mode_ == GRID_NONE) return;
  fl_color (color_grid_);               // FLTK-Doc: for Win32: color()
  fl_line_style (grid_linestyle_);      //   *before* line_style()!
  if (grid_mode_ & GRID_XLINES) draw_x_gridlines();  
  if (grid_mode_ & GRID_YLINES) draw_y_gridlines(); 
}


void Fl_Cartesius::empty_look (bool b)
{
  if (b) {
    if (children() && !empty_look_) printf("%s(): trotz Kinder??\n",__func__);
    set_minmax (0., 1.0, 0., 1.0);  // determines axis for SCALE=AUTO
    set_world  (0., 1.0, 0., 1.0);  // determines axis for SCALE=FIX
  }
  empty_look_ = b;
  rejustify_ = true;
}

void Fl_Cartesius::empty_look_string (const char* s)
{
  empty_look_str_ = s;
}


//-------------------------------------------- 
// Einige Helferroutinen zu Pruefungszwecken...
//-------------------------------------------- 

void Fl_Cartesius::show_minmax_area() const
{
  // show the min-max values at the plot area...
  int x0 = x_from_wx( xmin() );  
  int x1 = x_from_wx( xmax() );
  int y1 = y_from_wy( ymin() );       // y1 <--> unten
  int y0 = y_from_wy( ymax() );       // y0 <--> oben
  int oldcolor = fl_color();
  fl_color(FL_BLUE);
  fl_line_style(FL_SOLID);
  fl_loop(x0,y0, x1,y0, x1,y1, x0,y1);              // Rechteck
  fl_line(x0,y0, x1,y1);  fl_line(x0,y1, x1,y0);    // Diagonalen
  
  char s[64];
  //snprintf(s,64, "(%.1f, %.1f)", xmin(), ymin());
  snprintf(s,64, "(%s, %s)", x_tick_str(xmin()), y_tick_str(ymin()));
  fl_draw(s, x0+fl_descent(), y1-fl_descent());
  
  //snprintf(s,64, "(%.1f, %.1f)", xmin(), ymax());
  snprintf(s,64, "(%s, %s)", x_tick_str(xmin()), y_tick_str(ymax()));
  fl_draw(s, x0+fl_descent(), y0+fl_height());
  
  //snprintf(s,64, "(%.1f, %.1f)", xmax(), ymax());
  snprintf(s,64, "(%s, %s)", x_tick_str(xmax()), y_tick_str(ymax()));
  fl_draw(s, x1-fl_descent()-int(fl_width(s)), y0+fl_height());

  //snprintf(s,64, "(%.1f, %.1f)", xmax(), ymin());
  snprintf(s,64, "(%s, %s)", x_tick_str(xmax()), y_tick_str(ymin()));
  fl_draw(s, x1-fl_descent()-int(fl_width(s)), y1-fl_descent());
  
  fl_color(oldcolor);
}

void Fl_Cartesius::print_win() const
{
  printf("win: x = [%d, %d], y = [%d, %d]\n", __X.a, __X.b, __Y.a, __Y.b);
}                                         // left(),right(),top(),down();  

void Fl_Cartesius::print_world() const
{
  printf("world: x = [%f, %f], y = [%f, %f]\n", 
    __X.w_left(), __X.w_right(), __Y.w_down(), __Y.w_top());
} 

void Fl_Cartesius::print_minmax() const
{
  printf("x-min = %f,  x-max = %f\n", xmin(), xmax());
  printf("y-min = %f,  y-max = %f\n", ymin(), ymax());
}

void Fl_Cartesius::print_xinfo() const  { __X.print("X-axis:"); }
void Fl_Cartesius::print_yinfo() const  { __Y.print("Y-axis:"); }

void Fl_Cartesius::test_internal_map() const   
{                                               
  printf("x_from_wx (% f) = %3d, \twx_from_x (%3d) = % f\n",
    __X.wa, __X.position(__X.wa), __X.a, __X.value(__X.a)); 
  printf("x_from_wx (% f) = %3d, \twx_from_x (%3d) = % f\n",
    __X.wb, __X.position(__X.wb), __X.b, __X.value(__X.b)); 
  
  printf("y_from_wy (% f) = %3d, \twy_from_y (%3d) = % f\n",
    __Y.wa, __Y.position(__Y.wa), __Y.b, __Y.value(__Y.b)); 
  printf("y_from_wy (% f) = %3d, \twy_from_y (%3d) = % f\n",
    __Y.wb, __Y.position(__Y.wb), __Y.a, __Y.value(__Y.a)); 
}

void Fl_Cartesius::test_global_map() const   
{                                               
  printf("x_from_wx (% f) = %3d, \twx_from_x (%3d) = % f\n",
    __X.wa, x_from_wx(__X.wa), __X.a, wx_from_x(__X.a)); 
  printf("x_from_wx (% f) = %3d, \twx_from_x (%3d) = % f\n",
    __X.wb, x_from_wx(__X.wb), __X.b, wx_from_x(__X.b)); 
  
  printf("y_from_wy (% f) = %3d, \twy_from_y (%3d) = % f\n",
    __Y.wa, y_from_wy(__Y.wa), __Y.b, wy_from_y(__Y.b)); 
  printf("y_from_wy (% f) = %3d, \twy_from_y (%3d) = % f\n",
    __Y.wb, y_from_wy(__Y.wb), __Y.a, wy_from_y(__Y.a)); 
}



//============================================================================
//
//  CartesObject  -  IMPLEMENTATION
//
//============================================================================
/**+*************************************************************************\n
*  Ctor
*  Sicherzustellen ist, dass CartesObject immer auch FLTK-Kind seines 
*  Cartesius, damit es teilnimmt an der draw()- und handle()-Maschinerie. 
*  Wenn noch elternlos, einreihen, wenn falsche Eltern, umgruppieren, beides
*  erledigt add(). Weil zur Fl_Cartesius-Gruppe auch Nicht-CartesObjecte
*  gehoeren koennen, sagen wir, eine Gruppe `g', ist moeglich, dass ein
*  CartesObject innerhalb `g' konstruiert wurde und nun `g' eingereiht ist. 
*  Der einfache Ansatz
*  
*     if (!parent() || parent()!=ca_) ca_->add(this); 
* 
*  vermeinte auch hier falsche Eltern und gruppierte um. Daher die ganze
*  Eltern-Hierarchie zuruecksteigen, ob `ca_' darunter.
******************************************************************************/
CartesObject::CartesObject (const char* la)
  : Fl_Widget (0,0,0,0, la)
{
  SPUR_CLSS(("%s(\"%s\")...\n", __func__, la))
  SPUR_CLSS(("\tcurrent=%p, parent()=%p, window()=%p, current_ca=%p\n",
    Fl_Group::current(), parent(), window(), Fl_Cartesius::current_cartes()))
    
  ca_ = Fl_Cartesius::current_cartes();
  if (!ca_) {
    printf("%s(\"%s\"): No active Fl_Cartesius, throw...\n",__func__,la);
    throw -1;   // TODO: something better
  }

  Fl_Group* g = parent();
  while (g && g != ca_)  
    g = g->parent();
  
  if (!g) {
    SPUR_CLSS(("\tnot yet child of Fl_Cartesius, add\n",__func__,la));
    ca_ -> add(this);
  }
  else 
    SPUR_CLSS(("\tfine, allready child of Fl_Cartesius\n",__func__,la));
  
  XX_ = ca_-> XX();
  YY_ = ca_-> YY();
  
  // Which size shall we give the widget? Best were the real size, clear,
  //   but unknown here. To secure, it comes through the clipping in
  //   Fl_Group::draw_children, we give it here the full Cartesius size...
  w (ca_-> w());     
  h (ca_-> h());     
  
  // a bit more adequate would be the size of the plot region...
  //x (ca_-> left());   
  //y (ca_-> right());
  //w (ca_-> width());
  //h (ca_-> height());
  // ...but often in this moment (of inserting) not yet justified,
  //   (only after justify()), moreover, changeable later!
  
  SPUR_CLSS(("\tx=%d, y=%d, w=%d, h=%d\n", x(),y(),w(),h()))
}


//============================================================================
//
//  The Grid as CartesObject  -  IMPLEMENTATION
//
//============================================================================
void Grid::draw()
{ TRACE_DRAW(("extern grid\n"))
  if (mode_ == GRID_NONE) return;
  fl_color (color());                   // FLTK-Doc: for Win32: fl_color()
  fl_line_style (linestyle_);           //   *before* fl_line_style()!
  if (mode_ & GRID_XLINES) draw_x_lines();  
  if (mode_ & GRID_YLINES) draw_y_lines(); 
}

void Grid::draw_x_lines()
{
  int i0 = YY()-> tick_i0();
  int i1 = YY()-> tick_i1();
  
  for (int i=i0; i <= i1; i++)
    fl_xyline (left(), y_from_wy (YY()->tickvalue(i)), right());
}

void Grid::draw_y_lines()
{
  int i0 = XX()-> tick_i0();
  int i1 = XX()-> tick_i1();
  
  for (int i=i0; i <= i1; i++)
    fl_yxline (x_from_wx (XX()->tickvalue(i)), top(), down());
}


//============================================================================
// 
//  and the CurvePlot-utilities announced in Fl_Cartesius.hpp...
//
//============================================================================

//--------------------------------------------------
// returns next curve style out of enum CRV_STYLE
//-------------------------------------------------- 
CRV_STYLE next_curvestyle (CRV_STYLE oldstyle) 
{
  if (oldstyle == 2) return CRV_STYLE(0);
  else               return CRV_STYLE(oldstyle + 1);
}
 
//----------------------------------------------------------------
// pool of functions for automatic symbol distrib. in CurvePlot
//----------------------------------------------------------------
DrawPointSymbol* point_symbol_pool[] = {
  __draw_cross,                 // without POINT_POINT, but then
  __draw_square,                // in the same order as in POINT_SYMBOL!
  __draw_circle,                
};
const int n_point_symbol_pool = 
  sizeof(point_symbol_pool) / sizeof(point_symbol_pool[0]);

//-------------------------------------------------------------
// translation of pool function pointers into enum constants
//-------------------------------------------------------------
int which_symbol (DrawPointSymbol* f)   
{
  for (int i=0; i < n_point_symbol_pool; i++)
    if (f == point_symbol_pool[i]) 
      return i+1;   // '+1' because first enum is missing in pool
  
  return 0;         // POINT_POINT or 'not found'
}

//-------------------------------------------------
// automatic color and point symbol distribution...
//-------------------------------------------------
static int auto_color_nr_;
static int auto_point_symbol_nr_;

Fl_Color auto_color()
{
  if (auto_color_nr_ >= n_color_pool)  
    auto_color_nr_ = 0;
  
  return color_pool [auto_color_nr_ ++];
}

DrawPointSymbol* auto_point_symbol()
{
  if (auto_point_symbol_nr_ >= n_point_symbol_pool) 
    auto_point_symbol_nr_ = 0;
  
  return point_symbol_pool [auto_point_symbol_nr_ ++];
}
// definition of `auto_...' variables outside of the functions makes
// they (re)setable


//----------------------------------------
// definition of static class variables...
//----------------------------------------

int   X_AxisInfo::pbt_min_default = 45;  // quite reasonable, found by trial
int   Y_AxisInfo::pbt_min_default = 27;  // quite reasonable, found by trial
int   Fl_Cartesius::tick_len_     = 4;     
int   Fl_Cartesius::extraspace_   = 3;
char* Fl_Cartesius::empty_look_default_str_ = "empty";

Fl_Cartesius* Fl_Cartesius::current_ca_ = 0;


// END OF FILE

/*
 * CurveTnt.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file  CurveTnt.hpp  
  
  Plot von TNT::Array1D-Kurven mittels Fl_Cartesius.               
  Hartmut Sbosny, 2005
 
  Diese Datei gehoert *nicht* zur TNT-Bibliothek, doch ich stelle sie in den
   Namensraum "TNT", denn es ist die TNT::Array1D-Implementation einer 
   CurvePlot-Klasse. Daher die Nachsilbe "Tnt" im Dateinamen.
 
  Weil im Namensraum "TNT", nur "Curve" genannt, nicht "CurveTnt", da als
   "TNT::Curve" zu benutzen. Bei "CurveTntPlot" schon wieder vergessen.
 
  @todo  Da das hier ein Template, versuche handle() und Menue abzuseparieren,
    dass die nicht dupliziert werden.
*/
#ifndef CurveTnt_hpp
#define CurveTnt_hpp


#include <cmath>            
#include <cassert>
#include <cstdlib>              // random()
#include <vector>               // used in class CurveTntPlot
#include <algorithm>            // min(), max()

#include <FL/Fl.H>              // Fl::event_x() etc.
#include <FL/fl_draw.H>         // fl_line() etc.
#include "TNT/tnt_array1d.hpp"
#include "TNT/tnt_math_utils.hpp"   // swap()
#include "Fl_Cartesius.hpp"

#undef YS_DEBUG
//#define YS_DEBUG
#include "ys_dbg_varia.hpp"     // SPUR(), TRACE()



namespace TNT {

//----------------------------------
// some min-max helper templates...
//----------------------------------
template <class T>
void find_minmax (const TNT::Array1D<T> & A, T & wmin, T & wmax)
{
  T min, max;
  min = max = A[0];
  for (int i=1; i < A.dim(); i++)
    if (A[i] < min) min=A[i]; else 
    if (A[i] > max) max=A[i];
  wmin = min;
  wmax = max;
}

template <class T>                  // not used until now
void find_minmax_index (const TNT::Array1D<T> & A, 
    T & wmin, T & wmax, int & imin, int & imax)
{
  int ia=0, ib=0;
  T  a, b;  
  a = b = A[0];
  for (int i=1; i < A.dim(); i++)
    if (A[i] < a) {a=A[i]; ia=i;} else 
    if (A[i] > b) {b=A[i]; ib=i;}
  wmin = a;  imin = ia;  
  wmax = b;  imax = ib; 
}

template <class T>
T get_min (const TNT::Array1D<T> & A)
{
  T min = A[0];
  for (int i=1; i < A.dim(); i++)
    if (A[i] < min) min = A[i]; 
  return min;
}

template <class T>
T get_max (const TNT::Array1D<T> & A)
{
  T max = A[0];
  for (int i=1; i < A.dim(); i++)
    if (A[i] > max) max = A[i]; 
  return max;
}



/**============================================================================

  @class Curve

  Wie bei inkonsistenten Dimensionen von xvals, yvals etc. verfahren?
   Fehlerbalken koennten ggbnflls. separat geplotttet werden, aber bei x 
   und y muss es stimmen. Notfalls huelfe ein "to min(xdim,ydim)". Zur 
   Zeit einfaches assert(dimA==dimB) in den Konstruktoren.
  
  Warum gibt es keine Konflikte, wenn neben `Fl_Widget::color() const' 
   hier eine zweite Funktion `color() const' definiert??? Gleiches fuer
   `color(Fl_Color)'! Anwort: Sie wird verdeckt. Trotzdem keine gute Idee.

  FRAGE: Fl_Widget::color() fuer Kurvenfarbe umnutzen oder eigene
   Routine und eigene Farbvariable? BTW: Insofern Curve direkt von 
   Fl_Widget abstammt und eine FL_NO_BOX ist, kaeme die Hintergrundfarbe
   Fl_Widget::color() nie zum Einsatz. Ist die Frage, ob es andere
   Einsatzmoeglichkeiten gibt?

*============================================================================*/
template <class T>
class Curve : public CartesObject
{
  //-----------------------------------------------------------
  // Hilfsklasse, um die Standardsetzungen in den Konstruktoren
  // zu automatisieren...
  //-----------------------------------------------------------
  struct CurveParam
  {
    DrawPointSymbol* draw_point_symbol_; 
    CRV_STYLE style_;
    //Fl_Color color_;        // obsolete, *if* we use Fl_Widget::color()
    bool x_errorbars_;
    bool y_errorbars_;
    bool show_numbers_;
    
    CurveParam()
      : draw_point_symbol_ (__draw_cross),
        style_(STYLE_POINTS),
        //color_(FL_RED),      // obsolete, *if* we use Fl_Widget::color()
        x_errorbars_(false),
        y_errorbars_(false),
        show_numbers_(false)
      {}
  };
  
  //-----------------
  // Variablen...
  //-----------------
  TNT::Array1D<T> xvals, x0_err, x1_err;
  TNT::Array1D<T> yvals, y0_err, y1_err;
  
  CurveParam param;     // default constructor --> default params
  
  T  xmin_,xmax_, ymin_,ymax_;  // of the Curve, not neseccary of the axis, 
                                //  to which can belong several Curves
public:
  Curve (const TNT::Array1D<T> & Y, const char* la=0);
 
  Curve (const TNT::Array1D<T> & X, 
         const TNT::Array1D<T> & Y, const char* la=0); 
  
  Curve (const TNT::Array1D<T> & X, 
         const TNT::Array1D<T> & Y, 
         const TNT::Array1D<T> & dYerr, const char* la=0); 
     
  Curve (const TNT::Array1D<T> & X, 
         const TNT::Array1D<T> & Y, 
         const TNT::Array1D<T> & Y0err, 
         const TNT::Array1D<T> & Y1err, const char* la=0); 
  
  Curve (const TNT::Array1D<T> & X, 
         const TNT::Array1D<T> & Y, 
         const TNT::Array1D<T> & X0err, 
         const TNT::Array1D<T> & X1err, 
         const TNT::Array1D<T> & Y0err, 
         const TNT::Array1D<T> & Y1err, const char* la=0); 
         
/*
  void x_values     (const TNT::Array1D<T>& X)  { xvals = X; }
  void dx_err_values(const TNT::Array1D<T>& dX) { dx_err = dX; }
  void x0_err_values(const TNT::Array1D<T>& X0) { x0_err = X0; }
  void x1_err_values(const TNT::Array1D<T>& X1) { x1_err = X1; }
  
  void y_values     (const TNT::Array1D<T>& Y)  { yvals = Y; }
  void dy_err_values(const TNT::Array1D<T>& dY) { dy_err = dY; }
  void y0_err_values(const TNT::Array1D<T>& Y0) { y0_err = Y0; }
  void y1_err_values(const TNT::Array1D<T>& Y1) { y1_err = Y1; }
*/
  CRV_STYLE style() const       { return param.style_;  }
  void style(CRV_STYLE style)   { param.style_ = style; }
/*                                                        
  Fl_Color color() const        { return param.color_;  } // Bad: hides
  void color(Fl_Color color)    { param.color_ = color; } //  Widget::color()
*/
  DrawPointSymbol* symbol() const { return param.draw_point_symbol_; }
  void symbol(DrawPointSymbol* f) { param.draw_point_symbol_ = f; } 
  
  void find_x_minmax()          { find_minmax (xvals, xmin_,xmax_);
    TRACE(("\"%s\": xmin=%f, xmax=%f\n", label(),xmin_,xmax_)) }
  void find_y_minmax()          { find_minmax (yvals, ymin_,ymax_); 
    TRACE(("\"%s\": ymin=%f, ymax=%f\n", label(),ymin_,ymax_))
   }

  double xmin() const           { return xmin_; }
  double xmax() const           { return xmax_; }
  double ymin() const           { return ymin_; }
  double ymax() const           { return ymax_; }
  
  void x_errorbars (bool val)  
    { param.x_errorbars_ = val;  
      if (x0_err.dim()==0) param.x_errorbars_ = false; 
    }
  void y_errorbars (bool val)  
    { param.y_errorbars_ = val;  
      if (y0_err.dim()==0) param.y_errorbars_ = false; 
    }
  bool x_errorbars()            { return param.x_errorbars_; }
  bool y_errorbars()            { return param.y_errorbars_; }
    
  void show_numbers (bool bo)   { param.show_numbers_ = bo;   }
  bool show_numbers ()          { return param.show_numbers_; }
  
  void invert();
  
protected:
  void draw();
    
private:
  void draw_points();
  void draw_lines();
  void draw_lines_and_points();
  void draw_histogram();
  inline void draw_x_errorbar(int y, int i);
  inline void draw_y_errorbar(int x, int i);
  void draw_point_numbers();
  void declare();   // helper for constructors. Better name?
};


template <class T>
void Curve<T>::declare()        
{
  cartes()-> update_minmax (xmin_,xmax_, ymin_,ymax_);
  
  // if we use Fl_Widget::color(), no setting in CurveParam possibly...
  color (color_pool[0]);    // color(): either Widget::~ or 'my' own
}


template <class T>
Curve<T>::Curve (const TNT::Array1D<T> & Y, const char* la) 
  : CartesObject (la),
    xvals(Y.dim()),     // xvals allocated
    yvals(Y)            // yvals copied (handle)
{
  assert (Y.dim() > 0);         // oder werfen
  // Fuer Y.dim()==0 waeren Zugriffe in find_minmax() Desaster. 
  //  Dagegen dimX==dimY hier garantiert, da xvals hier allokiert.
  
  for (int i=0; i < xvals.dim(); i++)  
    xvals[i] = i;               // Indizes als x-Werte
  xmin_ = 0;              
  xmax_ = xvals.dim()-1;    
  
  find_y_minmax();
  declare();
}
  
template <class T>
Curve<T>::Curve (const TNT::Array1D<T> & X, 
                 const TNT::Array1D<T> & Y, const char* la) 
  : CartesObject (la),
    xvals(X), 
    yvals(Y)
{ 
  assert(X.dim()==Y.dim()); 
  assert(X.dim() > 0);
  
  find_x_minmax();
  find_y_minmax();
  declare();
}

template <class T>
Curve<T>::Curve (const TNT::Array1D<T> & X, 
                 const TNT::Array1D<T> & Y, 
                 const TNT::Array1D<T> & dYerr, const char* la) 
  : CartesObject (la),
    xvals(X), 
    yvals(Y), y0_err(X.dim()), y1_err(X.dim())
{
  assert(X.dim()==Y.dim()); 
  assert(X.dim()==dYerr.dim());
  assert(X.dim() > 0);
  
  for (int i=0; i < dYerr.dim(); i++)
  {
    y0_err[i] = yvals[i] - dYerr[i];
    y1_err[i] = yvals[i] + dYerr[i];
  }
  find_x_minmax();
  ymin_ = get_min (y0_err);     // falsch, wenn y0_err[] > yvals[]
  ymax_ = get_max (y1_err);     // falsch, wenn y1_err[] < yvals[]
  
  param.y_errorbars_ = true;
  declare();
}

template <class T>
Curve<T>::Curve (const TNT::Array1D<T> & X, 
                 const TNT::Array1D<T> & Y, 
                 const TNT::Array1D<T> & Y0err, 
                 const TNT::Array1D<T> & Y1err, const char* la) 
  : CartesObject (la),
    xvals(X), 
    yvals(Y), y0_err(Y0err), y1_err(Y1err)
{
  assert(X.dim()==Y.dim()); 
  assert(X.dim()==Y0err.dim());
  assert(X.dim()==Y1err.dim());
  assert(X.dim() > 0);
  
  find_x_minmax();
  ymin_ = get_min (y0_err);     // falsch, wenn y0_err[] > yvals[]
  ymax_ = get_max (y1_err);     // falsch, wenn y1_err[] < yvals[] 
  
  param.y_errorbars_ = true;
  declare();
}

template <class T>
Curve<T>::Curve (const TNT::Array1D<T> & X, 
                 const TNT::Array1D<T> & Y, 
                 const TNT::Array1D<T> & X0err, 
                 const TNT::Array1D<T> & X1err, 
                 const TNT::Array1D<T> & Y0err, 
                 const TNT::Array1D<T> & Y1err, const char* la)
  : CartesObject (la),
    xvals(X), x0_err(X0err), x1_err(X1err), 
    yvals(Y), y0_err(Y0err), y1_err(Y1err)
{
  assert(X.dim()==Y.dim()); 
  assert(X.dim()==X0err.dim());
  assert(X.dim()==X1err.dim());
  assert(X.dim()==Y0err.dim());
  assert(X.dim()==Y1err.dim());
  assert(X.dim() > 0);
  
  xmin_ = get_min (x0_err);
  xmax_ = get_max (x1_err);
  ymin_ = get_min (y0_err);
  ymax_ = get_max (y1_err);
  
  param.x_errorbars_ = true;
  param.y_errorbars_ = true;
  declare();
}
    
        
template <class T>
void Curve<T>::draw()
{
  SPUR(("Curve::draw()\n"))
  //fl_color (param.color_);  // so, if we use our own color variable
  fl_color (color());         // so, if Fl_Widget::color() is (ab)used
  fl_line_style (FL_SOLID);
  
  switch(param.style_)
  {
  case STYLE_LINES:             draw_lines();            break;
  case STYLE_LINES_AND_POINTS:  draw_lines_and_points(); break;
  case STYLE_HISTOGRAM:         draw_histogram();        break;
  default:                      draw_points();           break;
  }
  if (param.show_numbers_) draw_point_numbers();
}

template <class T>
inline void Curve<T>::draw_x_errorbar(int y, int i)
{
  int x0 = x_from_wx (x0_err[i]);
  int x1 = x_from_wx (x1_err[i]);
  fl_xyline (x0, y, x1);
  fl_yxline (x0, y-2, y+2);
  fl_yxline (x1, y-2, y+2);
}

template <class T>
inline void Curve<T>::draw_y_errorbar(int x, int i)
{
  int y0 = y_from_wy (y0_err[i]);
  int y1 = y_from_wy (y1_err[i]);
  fl_yxline (x, y0, y1);
  fl_xyline (x-2, y0, x+2);
  fl_xyline (x-2, y1, x+2);
}

template <class T>
void Curve<T>::draw_points()
{
  int N = std::min (xvals.dim(), yvals.dim());
  for (int i=0; i < N; i++)
  { 
    int x = x_from_wx (xvals[i]);
    int y = y_from_wy (yvals[i]);
    param.draw_point_symbol_(x, y); 
    if (param.x_errorbars_) draw_x_errorbar (y,i);
    if (param.y_errorbars_) draw_y_errorbar (x,i);
  }
}
  
template <class T>
void Curve<T>::draw_lines()
{
  int N = std::min (xvals.dim(), yvals.dim());
  int x0 = x_from_wx (xvals[0]);
  int y0 = y_from_wy (yvals[0]);
  if (param.x_errorbars_) draw_x_errorbar (y0,0);
  if (param.y_errorbars_) draw_y_errorbar (x0,0);
  
  for (int i=1; i < N; i++)
  { 
    int x = x_from_wx (xvals[i]);
    int y = y_from_wy (yvals[i]);
    fl_line (x0,y0, x,y);
    if (param.x_errorbars_) draw_x_errorbar (y,i);
    if (param.y_errorbars_) draw_y_errorbar (x,i);
    x0 = x;  
    y0 = y;
  }
}
  
template <class T>
void Curve<T>::draw_lines_and_points()
{
  int N = std::min (xvals.dim(), yvals.dim());
  int x0 = x_from_wx (xvals[0]);
  int y0 = y_from_wy (yvals[0]);
  param.draw_point_symbol_(x0,y0);
  if (param.x_errorbars_) draw_x_errorbar (y0,0);
  if (param.y_errorbars_) draw_y_errorbar (x0,0);
  
  for (int i=1; i < N; i++)
  { 
    int x = x_from_wx (xvals[i]);    // XX-> winpos (xvals[i])
    int y = y_from_wy (yvals[i]);    // YY-> winpos (yvals[i])
    fl_line (x0,y0, x,y);
    param.draw_point_symbol_(x,y);
    if (param.x_errorbars_) draw_x_errorbar (y,i);
    if (param.y_errorbars_) draw_y_errorbar (x,i);
    x0 = x;  
    y0 = y;
  }
}  

/**+*************************************************************************\n
  We draw a histogram using FLTK's filled polygon. fl_polygon() allows only
   convex polygons, fl_complex_polygon() allows also concave polygons.
******************************************************************************/
template <class T>
void Curve<T>::draw_histogram()
{
  int N = std::min (xvals.dim(), yvals.dim());
  fl_push_matrix();
  fl_begin_complex_polygon();
  //fl_begin_polygon();
  fl_vertex (x_from_wx(xvals[0]), y_from_wy(0.0));    // von x-Achse beginnend
  for (int i=0; i < N; i++)
    fl_vertex (x_from_wx(xvals[i]), y_from_wy(yvals[i]));
  fl_vertex (x_from_wx(xvals[N-1]), y_from_wy(0.0));  // auf x-Achse endend
  fl_end_complex_polygon();
  //fl_end_polygon();
  fl_pop_matrix();
}

/**+*************************************************************************\n
  Draw the point number at every point.
******************************************************************************/
template <class T>
void Curve<T>::draw_point_numbers()
{
  for (int i=0; i < xvals.dim(); i++)
  {
    char s[8]; snprintf(s,8,"%d", i);
    fl_draw (s, x_from_wx( xvals[i] ), y_from_wy( yvals[i] ) );
  }   
}


template <class T>          // Versuch
void Curve<T>::invert()
{  
  swap (xvals, yvals);
  swap (x0_err, y0_err);
  swap (x1_err, y1_err);
  swap (xmin_, ymin_); 
  swap (xmax_, ymax_);
  swap (param.x_errorbars_, param.y_errorbars_);
  //cartes()-> rejustify();    // better in CurvePlot
}



/**==========================================================================
 
  @class  CurveTntPlot
 
  Verwaltung der Curve<>-Objekte mittles std::vector<>. Weil Curve<>
   von Fl_Widget abgeleitet, und solche Objekt nicht in vector<> gehalten
   werden koennen, vector von *Zeigern* auf Curve<>'s.
 
  Frage: resizable machen schon im Konstruktor?
 
  Bedenkenswert: Unsere Funktionen zur Eingliederung von Curve-Objekten
   heissen wieder add() wie schon das add() unserer Basisklasse Fl_Group. 
   Letzteres wuerde hier verdeckt (trotz andere Argumentliste), daher ein
   `using Fl_Group::add;' noetig. Trotzdem koennte Umbennung in z.B. 
   add_curve() fuer mehr Klarheit sorgen.
 
  @bug Synchronisation zwischen unserem `curves' und Fl_Group's 'array_' kann
     zerstoert werden. Die Curve<>s sind Fl_Widgets und ihre Adressen werden
     auch in Fl_Group's `array_' gehalten (doppelte Buchfuehrung). Ein
     Fl_Group::remove() irgendwoher entfernt sie aus `array_', aber nicht 
     aus `curves'. Loesung?
 
*===========================================================================*/
template <class T>
class CurveTntPlot : public Fl_Cartesius
{
  std::vector< Curve<T>* > curves;  // collects Curve pointers
  T    xmin_,xmax_, ymin_,ymax_;    // min-max of *all curves*
  int  color_nr_;                   // color index of curve next to insert
  int  symbol_nr_;                  // symbol index of curve next... 
  bool show_point_nums_;            // Show point numbers?
  bool errorbars_;                  // Show error bars?

public:
  CurveTntPlot (int X, int Y, int W, int H, const char* la=0)
    : Fl_Cartesius (X,Y,W,H, la),
      color_nr_  (0), 
      symbol_nr_ (0),
      show_point_nums_ (false),    
      errorbars_ (true) 
    {}
  
  using Fl_Group::add;  // un-hide `add(Fl_Widget*)', more add()s come!
    
  void add (const TNT::Array1D<T> & Y, const char* la=0)
    { add_begin();  
      curves.push_back (new Curve<T>(Y, la)); 
      add_end(); }
  
  void add (const TNT::Array1D<T> & X, 
            const TNT::Array1D<T> & Y, const char* la=0) 
    { add_begin(); 
      curves.push_back (new Curve<T>(X,Y, la)); 
      add_end(); }
  
  void add (const TNT::Array1D<T> & X, 
            const TNT::Array1D<T> & Y, 
            const TNT::Array1D<T> & dYerr, const char* la=0) 
    { add_begin();  
      curves.push_back (new Curve<T>(X,Y, dYerr, la));
      add_end(); }
     
  void add (const TNT::Array1D<T> & X, 
            const TNT::Array1D<T> & Y, 
            const TNT::Array1D<T> & Y0err, 
            const TNT::Array1D<T> & Y1err, const char* la=0) 
    { add_begin();  
      curves.push_back (new Curve<T>(X,Y, Y0err,Y1err, la));
      add_end(); }

  void add (const TNT::Array1D<T> & X, 
            const TNT::Array1D<T> & Y, 
            const TNT::Array1D<T> & X0err, 
            const TNT::Array1D<T> & X1err, 
            const TNT::Array1D<T> & Y0err, 
            const TNT::Array1D<T> & Y1err, const char* la=0) 
    { add_begin();  
      curves.push_back (new Curve<T>(X,Y, X0err,X1err, Y0err,Y1err, la));
      add_end(); }
  
  ~CurveTntPlot() { DTOR(label()); }
      
  Curve<T>*       operator[] (int i)       { return curves.at(i); }
  const Curve<T>* operator[] (int i) const { return curves.at(i); }
  Curve<T> &      curve (int i)            { return *(curves.at(i)); }
  const Curve<T>& curve (int i)      const { return *(curves.at(i)); }
  Curve<T> &      back()                   { return *(curves.back());}
  const Curve<T>& back()             const { return *(curves.back());}

  int  handle (int event);
  void clear();    
  void invert();                // invert x-y-coord system
  void style (CRV_STYLE);
  void reorder (int from, int to); // move curve with index `from' to `to'
  
  //unsigned size()        const { return curves.size(); } 
  int      nCurves()       const { return curves.size(); }
  bool     any_invisible() const;
  
private:
  void add_begin();
  void add_end();
};


template <class T>
void CurveTntPlot<T>::add_begin()  // do some stuff before adding a curve
{ 
  // Secure that "I am" the active Fl_Cartesius...
  make_current_cartes();      // old: "current_cartes(this)"
  
  // Bei erster Kurve Min-Max von Cartesius zuruecksetzen? Fraglich, es koennten 
  //   noch andere Welt-Objekte (Nicht-Kurven) in Cartesius eingereiht sein, 
  //   die Min-Max schon belegten. `if (!children())' wieder zu ruecksichtsvoll,
  //   denn nun verhinderten auch Nicht-Weltobjekte die Ruecksetzung. Am
  //   sichersten ruft Anwender dies selbst auf...
  if (!children()) init_minmax();  // Sollte zumindest nicht schaden
}

template <class T>
void CurveTntPlot<T>::add_end()  // do some stuff after adding
{
  Curve<T>& last = *(curves.back());      // last added curve
  if (curves.size() == 1) {
    xmin_ = last.xmin();
    xmax_ = last.xmax();
    ymin_ = last.ymin();
    ymax_ = last.ymax();
  }
  else {
    if (last.xmin() < xmin_)  xmin_ = last.xmin();
    if (last.xmax() > xmax_)  xmax_ = last.xmax();
    if (last.ymin() < ymin_)  ymin_ = last.ymin();
    if (last.ymax() > ymax_)  ymax_ = last.ymax();
  }
                                                
  last.color( color_pool[color_nr_++] );
  if (color_nr_ == n_color_pool) color_nr_ = 0;

  last.symbol( point_symbol_pool[symbol_nr_++] );
  if (symbol_nr_ == n_point_symbol_pool) symbol_nr_ = 0;
        
  TRACE(("x-minmax(): %f,  %f\n", xmin_, xmax_))
  TRACE(("y-minmax(): %f,  %f\n", ymin_, ymax_))
  set_minmax (xmin_,xmax_, ymin_,ymax_);        
  // Z.Z. aktualisiert jede Kurve in ihrem Konstruktor schon per
  //  update_minmax() Cartesius::min-max, das hier dann doppelt! Entweder 
  //  dort raus oder hier. Zudem, set_minmax() setzt nur, justiert aber nicht.
}

template <class T>
void CurveTntPlot<T>::clear()
{
  Fl_Cartesius::clear();    // Destroys all 'my' children
  curves.clear();           // Clears the std::vector
  //init_minmax();          // not anymore, now empty_look(true)!
  empty_look(true);
  color_nr_ = symbol_nr_ = 0;
}

/**+*************************************************************************\n
  handle()  
  
  Same question as in Cartesius: calling Parent::handle() first of all and
   always, or last? First!
******************************************************************************/
template <class T>
int CurveTntPlot<T>::handle (int event)
{
  CRV_STYLE   curvestyle;
  unsigned    index;
  
  int ret = Fl_Cartesius::handle(event);
  switch (event)
  {
  case FL_SHORTCUT:     
  case FL_KEYDOWN:
    if (Fl::event_state() & FL_SHIFT); else 
    if (Fl::event_state() & FL_ALT); else 
    if (Fl::event_state() & FL_CTRL); 
    else 
    { switch (Fl::event_text()[0])
      {
      case 'n': 
        show_point_nums_ = !show_point_nums_;
        printf("show_numbers = %d\n", show_point_nums_);
        for (unsigned i=0; i < curves.size(); i++) 
          if (curves[i]-> visible())
            curves[i]-> show_numbers (show_point_nums_);
        redraw(); return 1;
      
      case 'e':
        if (any_invisible()) {      // alterate individual
          for (unsigned i=0; i < curves.size(); i++) 
            if (curves[i]-> visible()) {
              curves[i]-> x_errorbars (!curves[i]-> x_errorbars());
              curves[i]-> y_errorbars (!curves[i]-> y_errorbars());
            }
        }
        else {                      // alterate all TO the same
          errorbars_ = !errorbars_;
          for (unsigned i=0; i < curves.size(); i++) { 
            curves[i]-> x_errorbars (errorbars_);
            curves[i]-> y_errorbars (errorbars_);
          }
        }
        redraw(); return 1;
      
      case 'l':
        if (curves.size()==0) return 1;
        if (any_invisible()) {      // alterate individual
          for (unsigned i=0; i < curves.size(); i++) 
            if (curves[i]-> visible()) 
              curves[i]-> style (next_curvestyle (curves[i]-> style()));
        }
        else {                      // alterate all TO the same
          curvestyle = next_curvestyle (curves[0]-> style());
          for (unsigned i=0; i < curves.size(); i++) 
            curves[i]-> style (curvestyle);
        }
        redraw(); return 1;
      
      case 'i':
        invert(); redraw(); return 1;
        
      case '1': case '2': case '3': case '4': case '5':
      case '6': case '7': case '8': case '9': 
        //printf("%s, atoi()=%d\n", Fl::event_text(), atoi(Fl::event_text()));
        index = atoi(Fl::event_text()) - 1;
        if (curves.size() <= index) return 1;
        for (unsigned i=0; i < curves.size(); i++) {
          if (i==index) curves[i]-> set_visible();
          else          curves[i]-> hide();
        }
        redraw(); return 1;
      
      case '0':     // make all curves visible
        for (unsigned i=0; i < curves.size(); i++)
          curves[i]-> set_visible();
        redraw(); return 1;  
      
      } // switch text()[0]
    
    } // small letter
    break;   // nicht bearbeitet -> return Fl_Cartesius::handle()
  }  
  return ret;
}

template <class T>                  // Versuch
void CurveTntPlot<T>::invert()      
{
  for (unsigned i=0; i < curves.size(); i++)  
    curves[i]-> invert();
  swap (xmin_, ymin_);
  swap (xmax_, ymax_);  
  // invert axis: for SCALE_AUTO suffices minmax, world for SCALE_FIX
  set_minmax (xmin_,xmax_, ymin_,ymax_);    
  set_world (w_down(), w_top(), w_left(), w_right());
  rejustify();
  // TODO: Zoom beachten, z.Z. zurueckgesetzt (bei SCALE_AUTO)
  print_minmax();
  print_xinfo();
  print_yinfo();
  //print("show_minmax = %d\n", show_minmax_);
}

template <class T>                  
void CurveTntPlot<T>::style (CRV_STYLE st)      
{
  for (unsigned i=0; i < curves.size(); i++) 
    curves[i]-> style (st);
}

template <class T>                  
bool CurveTntPlot<T>::any_invisible() const
{
  for (unsigned i=0; i < curves.size(); i++)
    if (!curves[i]-> visible()) 
      return true;
  
  return false;
}

//---------------------------------------------------------------------
// Reorder curve with index `from' to index `to'. The previous 'to' gets
//  'to+1' for from>to and 'to-1' for from<to. Note: the corresponding Fltk 
//  indices inside Fl_Group (Cartesius) haven't to be the same, because
//  non-curve widgets can exist. To find especially the `to'-index use
//  Fl_Group's `find()'.
// We use vector::insert() and Fl_Group::insert(). Both insert *before*
//  the target index `to'. For to > from we have that's why to insert at 
//  `to+1'. It is assumed, that both functions simply append, if to >= size(). 
//  Fl_Group::insert() removes the source object 'from' automaticaly from 
//  its old pos, for std::vector we have to do it explicitely.
//--------------------------------------------------------------------- 
template <class T>                  
void CurveTntPlot<T>::reorder (int from, int to)
{
  if (from < 0 || from >= nCurves() || to < 0 || to >= nCurves() || from==to)
    return;
  
  // Reorder inside Fl_Group (==Cartesius)...
  if (from > to) 
    insert (*curves[from], find(curves[to]));
  else // from < to
    insert (*curves[from], find(curves[to])+1);
    
  // Reorder inside std::vector (`curves')...
  if (from > to) {
    curves.insert (curves.begin()+to, curves[from]);
    curves.erase (curves.begin()+from+1); // all pos>to shifted by "+1"
  }
  else { // from < to
    curves.insert (curves.begin()+to+1, curves[from]);
    curves.erase (curves.begin()+from);
  }
}


}  // namespace "TNT"

#endif // CurveTnt_hpp

// END OF FILE

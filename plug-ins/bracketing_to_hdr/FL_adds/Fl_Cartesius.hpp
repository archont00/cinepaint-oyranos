/*
 * Fl_Cartesius.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  Fl_Cartesius.hpp  --  derived now from Fl_Group instead from Fl_Window
*/
#ifndef Fl_Cartesius_hpp   
#define Fl_Cartesius_hpp

#include <cstdio>
#include <cmath>            // log()
#include <limits>           // numeric_limits<double>::max()
#include <FL/fl_draw.H>     // FL_SOLID, fl_line()...
#include <FL/Fl_Group.H>           

#undef YS_DEBUG
//#define YS_DEBUG
#include "ys_dbg.hpp"



enum SCALE { SCALE_FIX, SCALE_AUTO };
enum ARRANGE { ARRANGE_EXACT, ARRANGE_WRAP };
enum GRIDMODE { GRID_NONE, GRID_XLINES, GRID_YLINES, GRID_XYLINES };
//enum LOGMODE { LOG_2510, LOG_110 };   // not used
// und wenn, dann nur intern, daher besser in *.cpp oder in AxisInfo

class CoordMarker;
class Fl_Cartesius;
class CartesObject;
class Grid;


// die moegl. Tick-Abstaende im linearen Fall

const double tick_steps[] = { 1.0, 2.0, 5.0 };
const int  n_tick_steps   = sizeof(tick_steps) / sizeof(tick_steps[0]);

// die moegl. Tick-Werte im 2-5-10-Log-Fall

const double tick_values_log[] = { log(2.0), log(5.0), log(10.0) };
const int  n_tick_values_log   = 
                sizeof(tick_values_log) / sizeof(tick_values_log[0]);

// die moeglichen Tick-Abstaende im Log-Fall 

const double tick_log_steps[] = { log(2.0), log(5.0), log(10.0) };
const int  n_tick_log_steps   = 
                sizeof(tick_log_steps) /sizeof(tick_log_steps[0]);

                                
/**+*************************************************************************\n

  @class AxisInfo  --  abstract base class for X_AxisInfo and Y_AxisInfo.

  Mapping between window- and world-coordinates: 
 
    X: [a,b] <-> [wa,wb]   
    Y: [a,b] <-> [wb,wa]  (reverse to X as y=0 means 'top') 
 
   We define a:=left|top, b:=right|down, so that always b > a!
   For common view: smaller values left|down, corresponds to `a' the
   smaller* X-value wx, but the*  bigger* Y-value wy, ie. X: wb > wa and 
   Y: wa > wb. Anyhow we need below an abstract setting of world coords, 
   so we define these setting functions here abstract in the meaning
 
      set_world (w_min, w_max)   for common view.
 
   Whereas settings of win coords are the same for X and Y and can be
   concrete. Fuer die MinMax-Werte gilt natuerlich immer max > min, gleich,
   ob gewoehnliche oder Kopf-ueber-Darstellung.
 
   Weil alle Welt-Zuweisungen aus obigem Grund ueber Funktionen laufen
   sollten, sollten wa,wb eigentlich privat sein!
 
   For huge |w-wa| the function `position()' gives integer overflow.
  
   Ziel fuer Tick-Schnittstelle: Anwender:
 
      for (int i=tick_i0(); i <= tick_i1(); i++)  
        w = tickvalue(i)...
 
   Dh. Zusammenhang zwischen Indizes `i' und Schrittweite `dw' (die im Log-Fall
   ja variieren kann), bleibt AxisInfo-intern. Im Log-Fall aber wahrscheinlich ein 
      
      reset_tick();
      while (next_tick(& w)) { ... }
   
   weniger kompliziert.
******************************************************************************/
class AxisInfo     
{
protected:              // datas accessable for X|Y_AxisInfo
  SCALE  scale_mode_;
  ARRANGE arrange_mode_;
  int    pbt_min_;      // minimal "pixel between ticks" == "pbt"
  
  int    a, b;          // window coords of plot area (a < b!)
  double wa, wb, dw;    // world coords to `a' and `b'; 'dw' tick distance
  double wmin, wmax;    // MinMax world coords of objects to plot
  double f, inv_f;      // win <-> world map coefficients
  double log_wa;        // for logarithmic map
  int    i0, i1;        // indices for tick generation
  bool   logarithm_;    // linear or logarithmic map?

public:  
  AxisInfo (int pbt)   
   : 
     pbt_min_(pbt)
   { 
     scale_mode_   = SCALE_AUTO;           
     arrange_mode_ = ARRANGE_EXACT;
     logarithm_    = false;
     a = b = i0 = i1 = 0;  
     wa = wb = dw = log_wa = f = inv_f = 0.0; 
     init_minmax();
   }
  virtual ~AxisInfo() {}  // avoids warnings concerning virtual fncts
  //~AxisInfo()  {}
   
  void init_minmax()
   { wmin =  std::numeric_limits<double>::max();   // biggest value
     wmax = -std::numeric_limits<double>::max();   // smallest value
   }
  void set_minmax (double min, double max) { wmin=min; wmax=max; }
  bool update_minmax (double w0, double w1);
  
  void set_a (int A)  { a=A; }   // allows to maintain something other
  void set_b (int B)  { b=B; }
  void new_win (int A, int B)  { a=A;  b=B;  calc_map_coeff(); }
      
  // world coords to winpos 'a' and 'b' (reverse for X and Y!)  
  virtual void set_world (double w0, double w1) = 0;   // abstract
  virtual void new_world (double w0, double w1) = 0;   // abstract
  
  void calc_map_coeff();
  void adjust_ticks_and_world();
  
  int    position (double w) const;     // world coord `w' to window pos
  double value    (int p)    const;     // window pos `p' to world coord 
    
  SCALE scale_mode()         const  { return scale_mode_; }
  void scale_mode(SCALE mode)       { scale_mode_ = mode; }
  
  ARRANGE arrange_mode()     const  { return arrange_mode_; }
  void arrange_mode(ARRANGE mode)   { arrange_mode_ = mode; }
  
  bool logarithm()           const  { return logarithm_; }
  void logarithm(bool bo)           { logarithm_ = bo;   }
  
  int  pbt_min()             const  { return pbt_min_; }
  void pbt_min(int pbt)             { pbt_min_ = pbt;  }
  
  int    tick_i0() const            { return i0; }
  int    tick_i1() const            { return i1; }
  double tickvalue (int index) const;     // only after adjust..()!!
  double tickvalue_0() const        { return tickvalue (i0); }
  double tickvalue_1() const        { return tickvalue (i1); }
  double tickvalue_log (int i) const;
  
  void   print (const char* s) const;

private:
  void adjust_ticks_and_world_lin();
  void adjust_ticks_and_world_log();
  double dtick_by_pbt (double, double);
  double dtick_by_pbt_log (double, double);
};



/**+*************************************************************************\n
*
*  @class  X_AxisInfo
*
******************************************************************************/
class X_AxisInfo : public AxisInfo
{
  friend class Fl_Cartesius;
  
  static int pbt_min_default;
  
public:
  X_AxisInfo (int pbt = pbt_min_default) : AxisInfo(pbt) {}
  
  void set_world (double wLeft, double wRight)  // virtual
    { wa = wLeft; wb = wRight; }                // reverse to Y  
  
  void new_world (double wLeft, double wRight)  // virtual
    { wa = wLeft;  wb = wRight;  calc_map_coeff(); }  
 
  int left()        const   { return a;  }
  int right()       const   { return b;  }
  double w_left()   const   { return wa; } 
  double w_right()  const   { return wb; } 
};


/**+*************************************************************************\n
*
*  @class  Y_AxisInfo
*
******************************************************************************/
class Y_AxisInfo : public AxisInfo
{
  friend class Fl_Cartesius;
  
  static int pbt_min_default;

public:  
  Y_AxisInfo (int pbt = pbt_min_default) : AxisInfo(pbt) {}
  
  void set_world (double wDown, double wTop)    // virtual
    { wa = wTop; wb = wDown; }                  // reverse to X
  
  void new_world (double wDown, double wTop)    // virtual
    { wa = wTop;  wb = wDown;  calc_map_coeff(); }   
 
  int top()         const   { return a;  }
  int down()        const   { return b;  }      
  double w_top()    const   { return wa; } 
  double w_down()   const   { return wb; } 
};



/**+*************************************************************************\n
*
*  Fl_Cartesius  --  class providing an x-y-coordinate system.
*
*  Namensgebung "__X" und "__Y" inkonsistent, wenn Vorstriche sonst *globale* 
*   Namen kennzeichnen. Vielleicht "X_" oder "XX_".
*
******************************************************************************/
class Fl_Cartesius : public Fl_Group
{ 
  //friend class CartesObject; // noetig? 
  friend class CoordMarker;             // access to 'my' font infos

public:
  // Noch unklar, ob Anwender da ran muessen|wollen...
  static int tick_len_;                 // length of ticks
  //static int tick_len2_;              // evtl. fuer halbe Striche
  
private:  
  // static...
  static Fl_Cartesius*  current_ca_;    // the active Cartesius
  static int extraspace_;               // between tick numbers and axis     
  
  // non static...  
  X_AxisInfo __X;
  Y_AxisInfo __Y; 
    
  int      font_face_, font_size_, font_height_, font_descent_;
  Fl_Color color_axis_, color_grid_;
  int      grid_linestyle_; 
  int      axis_linewidth_; 
  int      add_, add1_;         // bad names, confusing with "add()"

  GRIDMODE grid_mode_;
  bool     reticking_under_resizing_;
  bool     rejustify_;          // justify() needed?
  bool     show_minmax_;
  bool     empty_look_;
  
  char     coord_picker_str_[32];
  
  const char* empty_look_str_;
  static char* empty_look_default_str_;
  
public:
  Fl_Cartesius (int X, int Y, int W, int H, const char* la=0);
  //~Fl_Cartesius() {}
  
  X_AxisInfo* XX()  {return &__X;}      // Warum hier Zeiger statt Ref zurueck??
  Y_AxisInfo* YY()  {return &__Y;}      //  Entsorgen?
  
  const X_AxisInfo & X() const  {return __X;}
        X_AxisInfo & X()        {return __X;}
  const Y_AxisInfo & Y() const  {return __Y;}
        Y_AxisInfo & Y()        {return __Y;}

  static Fl_Cartesius* current_cartes()         { return current_ca_; }
  static void current_cartes (Fl_Cartesius* ca) { current_ca_ = ca;   }
  void make_current_cartes()                    { current_ca_ = this; }
  
  void resize (int X, int Y, int W, int H); // virtual
  int handle (int event); // virtual
  
  void justify();                           
  void zoom (double);
  //void zoom_in (double f)  { zoom(f);    }
  //void zoom_out (double f) { zoom(1./f); }
  
  void init_minmax ()               { __X.init_minmax(); 
                                      __Y.init_minmax(); }
  void set_x_minmax (double min, double max) {__X.set_minmax (min,max); }
  void set_y_minmax (double min, double max) {__Y.set_minmax (min,max); }
  void set_minmax (double xmin, double xmax, double ymin, double ymax)
                                    { set_x_minmax(xmin,xmax);
                                      set_y_minmax(ymin,ymax); }
  double xmin() const               { return __X.wmin; }
  double xmax() const               { return __X.wmax; }
  double ymin() const               { return __Y.wmin; }
  double ymax() const               { return __Y.wmax; }

  void set_x_world (double w0, double w1)  { __X.set_world (w0,w1); }  
  void set_y_world (double w0, double w1)  { __Y.set_world (w0,w1); }  
  void set_world (double xw0, double xw1, double yw0, double yw1)
                                    { set_x_world(xw0,xw1);
                                      set_y_world(yw0,yw1); }
  
  double wx_from_x (int x)    const { return __X.value (x); } 
  double wy_from_y (int y)    const { return __Y.value (y); }
  int    x_from_wx (double w) const { return __X.position (w); }
  int    y_from_wy (double w) const { return __Y.position (w); }
  
  int left()   const                { return __X.a; } 
  int right()  const                { return __X.b; }
  int top()    const                { return __Y.a; }
  int down()   const                { return __Y.b; }
  int width()  const                { return right() - left() + 1; }
  int height() const                { return down() - top() + 1; } 
  
  double w_left()  const            { return __X.w_left();  }
  double w_right() const            { return __X.w_right(); }
  double w_top()   const            { return __Y.w_top();   }  
  double w_down()  const            { return __Y.w_down();  }  

  void axisfont (int face, int size); 
  
  bool retick_under_resize() const  { return reticking_under_resizing_;}
  void retick_under_resize(bool bo) { reticking_under_resizing_ = bo;}
  
  void scale_mode (SCALE mode)      { __X.scale_mode(mode);
                                      __Y.scale_mode(mode);   }
  void arrange_mode (ARRANGE mode)  { __X.arrange_mode(mode);
                                      __Y.arrange_mode(mode); }

  GRIDMODE gridmode()        const  { return grid_mode_;   }
  void gridmode (GRIDMODE mode)     { grid_mode_ = mode;   }
  
  void update_minmax (double X0, double X1, double Y0, double Y1); 
  void rejustify()                  { rejustify_ = true;   }
  
  bool empty_look()          const  { return empty_look_;  }
  void empty_look (bool b);
  void empty_look_string (const char* s);
    // Makes for b==true a default look for an empty coord system; the string
    //  `s' has to be global, static oder dynamic, but not local.
  void start_insert()               { init_minmax(); 
                                      empty_look(false);   }
protected:  
  void draw(); // virtual
  const char* x_tick_str (double w) const; 
  const char* y_tick_str (double w) const; 

private:
  //void init();
  void mk_coord_picker_str (double x, double y);
  void draw_coord_picker();
  void draw_x_axis();                      
  void draw_y_axis();                           
  void draw_axis();
  void draw_x_gridlines();
  void draw_y_gridlines();
  void draw_grid();

public: // debugging and test helpers
  void show_minmax_area() const;
  void show_minmax (bool b)             { show_minmax_ = b; }
  void print_win() const;
  void print_world() const;
  void print_minmax() const;
  void print_xinfo() const;
  void print_yinfo() const;
  void test_internal_map() const;
  void test_global_map() const;
};


/**+*************************************************************************\n
*
*  @class  CartesObject
*
******************************************************************************/
class CartesObject : public Fl_Widget
{
  Fl_Cartesius* ca_;        
  X_AxisInfo* XX_;          
  Y_AxisInfo* YY_;          

protected:   
public:
  Fl_Cartesius* cartes() { return ca_; }
  X_AxisInfo*   XX()     { return XX_; }    
  Y_AxisInfo*   YY()     { return YY_; }    
  
  const X_AxisInfo & X() const  {return *XX_;}  // Neu. Namen X() <-> XX()
        X_AxisInfo & X()        {return *XX_;}  //  zu ueberdenken.
  const Y_AxisInfo & Y() const  {return *YY_;}
        Y_AxisInfo & Y()        {return *YY_;}
   
  CartesObject (const char* la=0);    // serves as default constr. too

  double wx_from_x (int x)    const { return ca_ -> wx_from_x (x); } 
  double wy_from_y (int y)    const { return ca_ -> wy_from_y (y); }
  int    x_from_wx (double w) const { return ca_ -> x_from_wx (w); }
  int    y_from_wy (double w) const { return ca_ -> y_from_wy (w); }
  
  int left()       const    { return ca_ -> left();   }
  int right()      const    { return ca_ -> right();  }
  int top()        const    { return ca_ -> top();    }
  int down()       const    { return ca_ -> down();   }
  int width()      const    { return ca_ -> width();  }
  int height()     const    { return ca_ -> height(); } 
  
  double w_left()  const    { return ca_ -> w_left(); }
  double w_right() const    { return ca_ -> w_right();}
  double w_top()   const    { return ca_ -> w_top();  }
  double w_down()  const    { return ca_ -> w_down(); } 
};


/**+*************************************************************************\n
*
*  @class  Grid
*
******************************************************************************/
class Grid : public CartesObject
{
  GRIDMODE mode_;
  int  linestyle_; 
  
  void draw_x_lines();
  void draw_y_lines();

public:
  Grid (GRIDMODE mode=GRID_XYLINES, Fl_Color col=FL_WHITE, 
        int ls=FL_SOLID) 
    : mode_(mode), linestyle_(ls)  
    { color(col); }             // use Widget::color() for grid color
  
  GRIDMODE mode() const         { return mode_; }
  void mode (GRIDMODE mode)     { mode_ = mode; }

protected:
  void draw();
};


//============================================================================
//
//  Das Folgende sind von Fl_Cartesius bereitgestellte Hilfsmittel fuer 
//   anschliessende CurvePlot-Implementationen...
//
//============================================================================

//----------------------------------------
// the possible curve styles we provide...
//----------------------------------------
enum CRV_STYLE {
  STYLE_POINTS, 
  STYLE_LINES, 
  STYLE_LINES_AND_POINTS,
  STYLE_HISTOGRAM
};

CRV_STYLE next_curvestyle (CRV_STYLE oldstyle);  

//------------------------------------------
// type of the PointSymbol draw function...
//------------------------------------------
typedef void (DrawPointSymbol)(int x, int y);

// and some of it implementations...

inline void __draw_point(int x, int y)
{ fl_point (x, y); }

inline void __draw_cross(int x, int y)
{ fl_xyline (x-2, y, x+2);
  fl_yxline (x, y-2, y+2);
}
inline void __draw_square(int x, int y)
{ fl_rect (x-2,y-2, 5,5); }

inline void __draw_circle(int x, int y)
{ fl_arc (x-2,y-2, 5,5, 0., 360.); }

//-------------------------------------------------
// encoding of above functions by enum constants...
//-------------------------------------------------
enum POINT_SYMBOL {
  POINT_POINT, 
  POINT_CROSS, 
  POINT_SQUARE, 
  POINT_CIRCLE
};

//---------------------------------------------------------------------
// pool of such functions for automatic symbol distrib. in CurvePlot...
//---------------------------------------------------------------------
extern DrawPointSymbol*  point_symbol_pool [];
extern const int       n_point_symbol_pool;

//-------------------------------------------------------------
// translation of pool function pointers into enum constants...
//-------------------------------------------------------------
int which_symbol (DrawPointSymbol* f);


//------------------------------------------------------------
// pool of colors for automatic color distrib. in CurvePlot...
//------------------------------------------------------------
const Fl_Color color_pool[] = {
  FL_RED, 
  FL_GREEN, 
  FL_BLUE, 
  FL_MAGENTA, 
  FL_CYAN, 
  FL_DARK_RED, 
  FL_DARK_GREEN, 
  FL_DARK_YELLOW, 
  FL_DARK_CYAN
};
const int n_color_pool = sizeof(color_pool) / sizeof(color_pool[0]);

Fl_Color         auto_color();          // automatic color distribution
DrawPointSymbol* auto_point_symbol();   // automatic point symbol distrib.

#endif  // Fl_Cartesius_hpp

// END OF FILE

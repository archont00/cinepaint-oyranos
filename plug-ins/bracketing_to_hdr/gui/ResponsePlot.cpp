/*
 * ResponsePlot.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  ResponsePlot.cpp
*/

#include <FL/Fl_Box.H>                  // Fl_Box
#include <FL/fl_draw.H>                 // fl_color_average(), fl_weight()
#include "ResponsePlot.hpp"
#include "../br_core/i18n.hpp"          // macros _(), N_()
#include "../br_core/Br2Hdr.hpp"        // br:Br2Hdr::Instance()

#include <iostream>
using std::cout;


using namespace br;

static Br2HdrManager &  theBr2Hdr = Br2Hdr::Instance();


/**+*************************************************************************\n
  Ctor
******************************************************************************/
ResponsePlot::ResponsePlot(int X, int Y, int W, int H, const char* la)
  : 
    CurvePlotClass (X,Y,W,H, la),
    plot_ (*this),               // to make code clearer
    which_curves_ (Br2Hdr::USE_NEXT)
{ 
    CTOR(label())
    begin();
      retick_under_resize (true);
    end();                      // leave a closed group
    build();
}


/**+*************************************************************************\n
  build()  -  WITHOUT redraw()!
    
  Builds the plot widget for the response curves of sort \a which_curves_. 
   Further, calctor's weight curves are added.
   
  Note: add_outdated_box() needs correct top() and left() values, which are 
   available not until justify(). So we call it here. After it also the world
   coords w_top(), w_down() etc. are correct. We need them for scaling of the
   weight curves.
  
  Da Wichtungsfunktionen inzwischen permanent (dh. auch ausserhalb Calctor),
   koennten die auch zuerst geplottet (addiert) werden; sparte reorder(). Aber
   wie dann skalieren? Dazu braeuchte es zweites Koord-System!
   
  Plot der Wichtungskurve derzeit Kauderwelsch: Skalieren so, dass Bereich 
   [0..weight.maxval] 2/3 des y-Bereichs einnimmt (also fuer weight.minval > 0
   waere Kurve noch gestauchter) und tragen (unabhaengig von maxval) die Masszahlen
   "0" und "1" an. Negative (sinnlose) Gewichte gaeben hier also Merkwuerdiges.
   
  <b>U16 response curves:</b> Derzeit nur an 256-Stellen tabellierte Kurven. 
   Interpretieren die als y-Werte der x-Stellen 256*i+127 (Mitten der Zaehlintervalle).
   Eine U8-Kurve ist also sogleich als U16-Kurve verwendbar. Zu beachten bleibt,
   dass die U16-Wichtungskurven derzeit fuer die x-Stellen 0,257,...,65535 geplottet
   werden. Differenz duerfte aber kaum sichtbar sein.
******************************************************************************/
void ResponsePlot::build()
{
    //printf("ResponsePlot::%s()[which=%d]...\n",__func__, which_curves_);

    static Fl_Color        color_weight_resp = FL_DARK3;
    static Fl_Color        color_weight_merge = FL_WHITE;
    Fl_Color               colors[3] = {FL_RED, FL_GREEN, FL_BLUE};
    const HdrCalctorBase*  calctor = theBr2Hdr.calctor();      // short name

    plot_.clear();                   // --> empty_look(true)
    plot_.Y().scale_mode (SCALE_AUTO);
    plot_.make_current_cartes();     // wichtig fuer add()
    plot_.start_insert();            // init_minmax() + empty_look(false)

    //======================
    // Add response curves: 
    //======================
    switch (which_curves_)
    {
    case Br2Hdr::USE_NEXT:      // "use_next" curves
    case Br2Hdr::COMPUTED:      // last computed curves
    case Br2Hdr::EXTERNAL:      // external curves
        for (int i=0; i < 3; i++) {
          Array1D<double> & crv = theBr2Hdr.getResponseCurve(which_curves_, i);
          if (! crv.is_empty()) {
            if (theBr2Hdr.data_type() == DATA_U16)  
              plot_.add (xgrid256_for_U16(), crv);
            else
              plot_.add (crv);
            plot_.back().color( colors[i] );
          }
        }
        break;

    default:                    // == CALCTOR_S: Calctor's curves
        if (! calctor) break;
        for (int i=0; i < 3; i++) {
          if (calctor->isResponseReady(i)) {
            plot_.add (calctor->getExposeVals(i));
            plot_.back().color( colors[i] );
          }  
        }
        break;
    }  
    plot_.style(STYLE_LINES);
    
    any_response_curve_ = (plot_.nCurves() > 0);
    
    if (! plot_.nCurves()) {          // Not any curve was added
      plot_.empty_look (true);        // Resets minmax to [0,1]x[0,1]
    }
    
    /*  Justify() to have correct top(), w_top() etc. */
    justify();  
        
    //========================
    //  Add the weight curves: 
    //========================
    if (! plot_.nCurves()) {          // weight curves will be the only curves
      plot_.empty_look (false);               
      plot_.Y().scale_mode (SCALE_FIX);   // to draw weight curves with indent
    }
    
    /*  If a calctor exists, we plot its weight curves, otherwise the intended one.
       NOTE: Could we remove the dynamic cast? Task: We need the weight objects of the
        created Calctor. Probably simplest way: Manager provides their pointers in
        functions like calctor_RGB_U8(). The idea of an abstract weightFunc() in
        CalctorBase fails because the weight objects have different types. */
    const HdrCalctor_RGB_U8* calctor_RGB_U8 = 
                dynamic_cast<const HdrCalctor_RGB_U8*>( calctor );
    const HdrCalctor_RGB_U16_as_U8* calctor_RGB_U16_as_U8 = 
                dynamic_cast<const HdrCalctor_RGB_U16_as_U8*>( calctor );
    
    /*  Show "response weight" in UseExternResponse-Mode only for COMPUTED, in
         the other mode never for EXTERNAL. -- Deactivated, too confusingly! */
    bool show_resp = (theBr2Hdr.isUseExternResponse() && which_curves_==Br2Hdr::COMPUTED)
                  || (!theBr2Hdr.isUseExternResponse() && which_curves_ != Br2Hdr::EXTERNAL)
                  || 1;
    //cout << "show_resp = " << show_resp << '\n';              

    /*  We wish the white curve over the black one. Since add_weight_U8/16() reorders
         currently each weight curve to the bottom, "white" is to add before "black". */
    if (calctor_RGB_U8) {
      add_weight_U8 (calctor_RGB_U8->weightMerge(), color_weight_merge);
      if (show_resp)
        add_weight_U8 (calctor_RGB_U8->weightResp(), color_weight_resp);
    }
    else if (calctor_RGB_U16_as_U8) {
      add_weight_U16 (calctor_RGB_U16_as_U8->weightMerge(), color_weight_merge);
      if (show_resp)
        add_weight_U8 (calctor_RGB_U16_as_U8->weightResp(), color_weight_resp, true);
    }
    else {  // no calctor exists, show the intended weight curves as U8 curves
      WeightFunc_U8* w;
      w = new_WeightFunc_U8 (theBr2Hdr.weightShapeMerge());
      if (w) add_weight_U8 (*w, color_weight_merge);
      delete w;
      if (show_resp) {
        w = new_WeightFunc_U8 (theBr2Hdr.weightShapeResp());
        if (w) add_weight_U8 (*w, color_weight_resp);
        delete w;
      }
    }
    
    /*  Add the "outdated" box if needed (requires left() and top()!) */
    add_outdated_box(); 
}  

/**+*************************************************************************\n
  Return an Array[256] with values 256*i+127 (Mitten der Zaehlintervalle) als
   den zugehoerigen x-Werten einer 256 y-Werte umfassenden tabellierten U16-
   Responsefunktion. Private helper.
******************************************************************************/
Array1D<double>  ResponsePlot::xgrid256_for_U16()
{
    Array1D<double> X(256);
    for (int i=0; i < 256; i++)
      X[i] = 256*i + 127;
    return X;
}

/**+*************************************************************************\n
  Fill a plot curve with values of the given WeightFunc_U8 (scale appropriately)
   and add it to \a plot_. We assume correct plot_.w_down(), ~.w_top(). Currently
   the weight curve is ordered by default to the bottom of the curve stack.
  
  @param like_U16: Plot the U8 weight like a U16 weight, i.e. with x-values
    x_i = i*256+127, i=0..255, the x-range of a WeightFunc_U16. Needed to plot
    the "response weight" of an HdrCalctor_RGB_U16_as_U8, which uses a U8 weight
    for response computation. Default = False.
   
  @note For x_i = i*257 we get small differences to an original U16 weight.   
******************************************************************************/
void ResponsePlot::add_weight_U8 (const WeightFunc_U8 & weight, Fl_Color color, bool like_U16)
{
    Array1D <PlotValueT>  plw (256);    // "plw" for PLot Weight
    Array1D <PlotValueT>  X (256);      // x-values
    
    /*  Scale `plw' so, that 2/3 of the y-range is used. */
    PlotValueT delta_y = plot_.w_top() - plot_.w_down();
    PlotValueT delta_w = weight.maxval(); // - weight.minval();
    PlotValueT abs = plot_.w_down() + 0.1667 * delta_y;
    PlotValueT fac;
    if (delta_w != 0.) fac = 0.6667 * delta_y / delta_w;  
    else               fac = 0.;
      
    if (weight.have_table())
      for (int i=0; i < plw.dim(); i++)  
        plw[i] = fac * weight[i] + abs;
    else 
      for (int i=0; i < plw.dim(); i++)  
        plw[i] = fac * weight(i) + abs;
        
    if (like_U16)
      for (int i=0; i < 256; i++)  X[i] = i*256 + 127;
    else
      for (int i=0; i < 256; i++)  X[i] = i;
      
    plot_.add (X,plw);
    plot_.back().style (STYLE_LINES);
    plot_.back().color (color);
    plot_.reorder (plot_.nCurves()-1, 0);  // puts weight curve onto bottom
}

/**+*************************************************************************\n
  Fill a plot curve with values of the given WeightFunc_U16 (scale appropriately)
   and add it to \a plot_. We assume correct plot_.w_down(), ~.w_top(). Currently
   the weight curve is ordered by default to the bottom of the curve stack.
    
  @note We plot a selection of 256 values in a distance of 257, so we browse
   the full range [0, 257, ..., 65535].  x_i = i*257; i=0..256.
******************************************************************************/
void ResponsePlot::add_weight_U16 (const WeightFunc_U16 & weight, Fl_Color color)
{
    Array1D <PlotValueT>  plw (256);    // "plw" for PLot Weight
    Array1D <PlotValueT>  X (256);      // x-values
    
    /*  Scale `plw´ so, that 2/3 of the y-range is used. */
    PlotValueT delta_y = plot_.w_top() - plot_.w_down();
    PlotValueT delta_w = weight.maxval(); // - weight.minval();
    PlotValueT abs = plot_.w_down() + 0.1667 * delta_y;
    PlotValueT fac;
    if (delta_w != 0.) fac = 0.6667 * delta_y / delta_w;  
    else               fac = 0.;
      
    if (weight.have_table())
      for (int i=0; i < 256; i++)  
        plw[i] = fac * weight[i*257] + abs;
    else 
      for (int i=0; i < 256; i++)  
        plw[i] = fac * weight(i*257) + abs;
    
    for (int i=0; i < 256; i++)         // The x-values as doubles
      X[i] = i*257;
            
    plot_.add (X,plw);
    plot_.back().style (STYLE_LINES);
    plot_.back().color (color);
    plot_.reorder (plot_.nCurves()-1, 0);  // puts weight curve onto bottom
}

/**+*************************************************************************\n
  Add an "outdated" box to the CurvePlot widget if needed. Used by build(), and
   separately by ResponseWinClass::handle_Event() at CCD_OUTDATED events. Uses 
   top() and left(), i.e. needs a justify() before!
  @returns True, if added, False otherwise.
******************************************************************************/
bool  
ResponsePlot::add_outdated_box()
{
    /*  hint, that the shown computed curves reflect no more current params */
    const char* text = _("outdated");

    /*  Return, if all computed curves up-to-date OR no response curves are 
         shown OR external curves are shown OR the "outdate" text is Null */
    if (theBr2Hdr.isComputedResponseUptodate() || !any_response_curve_ || 
        which_curves_== Br2Hdr::EXTERNAL || !text)
      return false;
    
    /*  Return, if "use_next" curves are shown AND not anyone is a computed one */
    if (which_curves_== Br2Hdr::USE_NEXT  && ! theBr2Hdr.anyUsenextCurveComputed())
      return false;
      
    int x,y,w,h;
    //fl_font (fl_font(), 10); // 10er was not available
    w = (int)fl_width (text);
    h = fl_height();
    x = plot_.left() + 15;
    y = plot_.top() + 10;
    //printf("add_outdated_box(): left=%d,  top=%d\n", plot_.left(), plot_.top());
    Fl_Group* save = Fl_Group::current();
    Fl_Group::current (& plot_);
    { Fl_Box* o = new Fl_Box (x,y,w+8,h+4, text);
      o -> color(fl_color_average (FL_YELLOW, FL_WHITE, 0.5));
      o -> box(FL_BORDER_BOX);
      //o -> labelcolor(fl_color_average (FL_YELLOW, FL_WHITE, 0.5));
    }
    Fl_Group::current (save);  // restore previous Fl_Group pointer
    return true;
}        

/**+*************************************************************************\n
  Do the inherited draw, additionally draw a title string, the markers for the
   weight curve and a info about missing response curves.
  
  Hinichtlich Wichtungstabelle ein Notbehelf, wobei wir den Wert '0.1667' von 
   den add_weight_*()-Funktionen hernehmen. Sauberer waere ein zweites Koordsystem,
   angeheftet ans erste, das waere dann auch bei Invertierung korrekt. Nachteil
   zudem, dass auch alle coord_picker() drawings von CurvePlotClass::draw() hier
   durchlaufen. Zur Abhilfe koennte Fl_Cartesius eine "only_coord_picker_drawn"-
   Info liefern.
******************************************************************************/
void ResponsePlot::draw()
{
    CurvePlotClass::draw();
    //printf("ReseponsPlot::draw()...\n");
    
    /*  Draw the title string: */
    fl_font (fl_font(), 16);
    const char* s = _( title_str_[which_curves_] );
    int xt = plot_.left() + (plot_.right() - plot_.left() - (int)fl_width(s)) / 2;
    fl_draw (s, xt, plot_.top() + fl_height() + 5);
    fl_font (fl_font(), 12);
        
    if (plot_.empty_look()) return;
    
    HdrCalctorBase*  calctor = theBr2Hdr.calctor();
    
    /*  Draw the marker for the weight curve: */
    //if (calctor)        // Formerly a weight curve existed only in calctor,
    {                     //  meanwhile we plot a weight curve always.
      int dy = plot_.down() - plot_.top();
      int y0 = plot_.down() - int(0.1667 * dy + 0.5);
      int y1 = plot_.top() + int(0.1667 * dy + 0.5);
      fl_color(FL_WHITE);
      fl_xyline (plot_.left(), y0, plot_.left()+10); 
      fl_xyline (plot_.left(), y1, plot_.left()+10); 
      int add = fl_height()/2 - fl_descent();
      fl_draw("0", plot_.left()+15, y0+add);
      fl_draw("1", plot_.left()+15, y1+add);
    }
    
    /*  Draw messages about missing curves: */
    switch (which_curves_)
    {
    case Br2Hdr::USE_NEXT:  // "use_next" curves
        draw_missing_info (theBr2Hdr.getResponseCurveUsenext(0),
                           theBr2Hdr.getResponseCurveUsenext(1),
                           theBr2Hdr.getResponseCurveUsenext(2));
        break;
    
    case Br2Hdr::COMPUTED:  // computed curves
        draw_missing_info (theBr2Hdr.getResponseCurveComputed(0),
                           theBr2Hdr.getResponseCurveComputed(1),
                           theBr2Hdr.getResponseCurveComputed(2));
        break;
    
    case Br2Hdr::EXTERNAL:  // external curves
        draw_missing_info (theBr2Hdr.getResponseCurveExtern(0),
                           theBr2Hdr.getResponseCurveExtern(1),
                           theBr2Hdr.getResponseCurveExtern(2));
        break;
    
    default:  // == CALCTOR_S: calctor's curves
        if (calctor)
          draw_missing_info (calctor->getExposeVals(0),
                             calctor->getExposeVals(1),
                             calctor->getExposeVals(2));
        break;
    }
}

/**+*************************************************************************\n
  Draw an info about missing response curves.  Private helper of draw().
******************************************************************************/
void
ResponsePlot::draw_missing_info (const Array1D<double> & crvR, 
                                 const Array1D<double> & crvG,
                                 const Array1D<double> & crvB )         
{
    int n_miss = 0;
    if (crvR.is_empty()) n_miss++;
    if (crvG.is_empty()) n_miss++;
    if (crvB.is_empty()) n_miss++;
    
    if (n_miss == 3)          // no curves
    {
      const char* string = _("no curves");
      int x,y,w,h;
      w = (int)fl_width(string);
      h = fl_height();
      x = (plot_.left() + plot_.right() - w) / 2;
      y = (plot_.top() + plot_.down()) / 2;    // baseline of the font
      fl_color (FL_WHITE); fl_rectf(x-3, y-h, w+6, h+fl_descent()+4);
      fl_color (FL_BLACK); fl_draw(string, x,y);
    }
    else if (n_miss > 0) 
    {
      /*  The following response curves are missed */
      char* missing = _("missing:");
      int w = (int)fl_width (missing);
      int x = plot_.right()-50;
      int y = plot_.down()-20;
      fl_color(FL_BLACK);
      fl_draw (missing, x-w, y);
      if (crvR.is_empty()) 
      {
        fl_color (FL_RED);       
        fl_draw (" R", x, y);
        x += (int)fl_width(" R");
      } 
      if (crvG.is_empty()) 
      {
        fl_color (FL_GREEN);     
        fl_draw (" G", x, y);
        x += (int)fl_width(" G");
      } 
      if (crvB.is_empty()) 
      {
        fl_color (FL_BLUE);      
        fl_draw (" B", x, y);
      } 
    }
}



/*****************************************************************************
  static elements:
******************************************************************************/

/*  Title of the response curve diagrams */
const char* ResponsePlot::title_str_[] = {N_("Using next"), N_("Computed"), N_("External"), N_("Calctor's")};

// END OF FILE

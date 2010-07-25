/*
 * FllwUpCrvPlot.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  FllwUpCrvPlot.cpp
*/
#include "FllwUpCrvPlot.hpp"
#include "../br_core/br_defs.hpp"       // BR_DEBUG_RECEIVER
#include "../br_core/Br2Hdr.hpp"        // br:Br2Hdr::Instance()


using namespace br;

static Br2HdrManager &  theBr2Hdr = Br2Hdr::Instance();


/**+*************************************************************************\n
  Ctor
******************************************************************************/
FollowUpCurvePlot::FollowUpCurvePlot(int X, int Y, int W, int H, const char* la)
  : 
    CurvePlotClass (X,Y,W,H, la),
    channel_ (0),               // R channel as default
    plot_ (*this)               // to make code more readable
{ 
    CTOR(label())
    begin();
      retick_under_resize (true);
    end();                      // we leave a closed group
    build();    
}


/**+*************************************************************************\n
  build()   -   WITHOUT redraw()!
  
  Build the plot widget for the follow-up curves existing currently in Calctor.
   For channel `channel_´. 

  Zum Referenzzaehlen: Die lokalen Array1D-Objekte `X', `Y' etc. werden am
   Funktionsende wieder freigeben. Aber die per add(...) gemachten Haendelkopien
   leben solange dieses Widget lebt.
******************************************************************************/
void FollowUpCurvePlot::build()
{
    //PRINTF(("FollowUpCurvePlot::%s()...\n",__func__));
    plot_.clear();                 // --> empty_look(true)
    
    if (! theBr2Hdr.haveFollowUpCurves()) {
      //printf("\thave no curves\n"); 
      return;
    }
    //  I.e. a Calctor exists; and its FollowUpValuesBase reference.
    const HdrCalctorBase*      calctor = theBr2Hdr.calctor();   // short name
    const FollowUpValuesBase & followUp = calctor->fllwUpVals(); // short name
    
    //PRINTF(("\trefpic = %d\n", followUp.refpic() ));
    
    plot_.make_current_cartes();   // needed for plot_.add()
    plot_.start_insert();          // init_minmax() + empty_look(false)
 
    //  The common X-values of the curves...
    Array1D<PlotValueT>  X = followUp.get_CurvePlotdataX();
    
    //  The Y-values of the curves...
    for (int p=0; p < calctor->size(); p++)  
    {
      Array1D<PlotValueT>  Y,Y0,Y1;
      
      if (p == theBr2Hdr.refpic()) {  // refpic's curve is a straight line
        Y = followUp.get_CurvePlotdataY (p, channel_);
        plot_.add(X,Y);
      }
      else {
        followUp.get_CurvePlotdataY (Y,Y0,Y1, p, channel_);
        plot_.add(X,Y,Y0,Y1);
      }
    }
    plot_.style(STYLE_LINES);
  
    //  Behelfs-Histogramm...  (interessant waere mal ein logarithmisches)
    //  Scale so, that `plot_'s full y-range is used:
    size_t  histo_maxval;
    Array1D<PlotValueT>  H = followUp.get_RefpicHistogramPlotdata (
                    channel_, &histo_maxval, plot_.ymin(), plot_.ymax() );
    
    plot_.add(X,H);  
    plot_.back().style(STYLE_LINES);
    plot_.back().color(FL_BLACK);  //FL_LIGHT3);

    //  Histogramm "nach unten":
    //plot_.reorder(plot_->nCurves()-1, 0); 
    //  veraendert leider auch Kurvennummern fuer User, daher erstmal raus
}


/**+*************************************************************************\n
  handle_Event()
******************************************************************************/
void FollowUpCurvePlot::handle_Event (Br2Hdr::Event e)
{
#ifdef BR_DEBUG_RECEIVER
    std::cout<<"FollowUpCurvePlot::"; EventReceiver::handle_Event(e);
#endif
    
    switch (e) 
    {
    case Br2Hdr::FOLLOWUP_UPDATED:
    case Br2Hdr::CALCTOR_DELETED:
        BR_EVENT_HANDLED(e);
        update(); 
        break;
    
    default: 
        BR_EVENT_NOT_HANDLED(e); 
        break;
    }
}

// END OF FILE

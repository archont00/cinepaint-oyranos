/*
 * DisplcmFinder_UI_rest.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
/*!
  DisplcmFinder_UI_rest.cpp
*/

#include "DisplcmFinder_UI.h"
#include "../br_core/Run_DisplcmFinder.hpp"


//using namespace br;


/*!  Fills the menus with the data of the Params structure \a prm. */
void
DisplcmFinder_UI::getValuesFromParams (const DisplcmFinderCorrel::Params & prm)
{
    char  s[80];
    
    sprintf (s, "%d", prm.start_search_size.x);
    input_M_start_ -> value(s);
    
    sprintf (s, "%d", prm.start_correl_size.x);
    input_N_start_ -> value(s);
    
    sprintf (s, "%d", prm.max_num_areas_1d);
    input_max_num_ -> value(s);
    
    sprintf (s, "%d", prm.subtr_search_dim);
    input_subtr_ -> value(s);
    
    sprintf (s, "%d", prm.add_correl_dim);
    input_add_ -> value(s);
    
    sprintf (s, "%.2f", prm.min_rho_accepted);
    input_min_rho_ -> value(s);
    
    if (prm.verbose <= 0) choice_verbose_ -> value(0); else
    if (prm.verbose == 1) choice_verbose_ -> value(1);
    else                  choice_verbose_ -> value(2);
}

/*!  Writes the menu values into the static \a init_param_ structure of
      DisplcmFinderCorrel_. */
void 
DisplcmFinder_UI::putValuesToParams()
{
    int  intVal;
    float  floatVal;
    
    DisplcmFinderCorrel_::Params &  prm = DisplcmFinderCorrel_::ref_init_params();
    
    sscanf (input_M_start_->value(), "%d", &intVal);
    prm.start_search_size  =  Vec2_int(intVal);
    
    sscanf (input_N_start_->value(), "%d", &intVal);
    prm.start_correl_size  =  Vec2_int(intVal);
    
    sscanf (input_max_num_->value(), "%d", &intVal);
    prm.max_num_areas_1d  =  intVal;
    
    sscanf (input_subtr_->value(), "%d", &intVal);
    prm.subtr_search_dim  =  intVal;
    
    sscanf (input_add_->value(), "%d", &intVal);
    prm.add_correl_dim  =  intVal;
    
    sscanf (input_min_rho_->value(), "%f", &floatVal);
    prm.min_rho_accepted  =  floatVal;
    
    prm.verbose  =  choice_verbose_->value();
    printf("VERBOSE = %d\n", prm.verbose);
}


/*!  Setzt Menuedaten wie auch das statische \a init_params_ von DisplcmFinderCorrel_
      auf die Defaultwerte aus DisplcmFinderCorrel_. */
void  
DisplcmFinder_UI::default_values()
{
    getValuesFromParams (DisplcmFinderCorrel_::default_params());
    DisplcmFinderCorrel_::init_params (DisplcmFinderCorrel_::default_params());
}


/*!
  Berechnet Versatz zwischen den auf den Bildnummernwaehlern gewaehlten Bildern.
  @note No check for validity of image numbers, we rely on that the GUI provides
   correct values or that Run_DisplcmFinder::run_single() checks it.
*/
void DisplcmFinder_UI::run_single()
{
    Run_DisplcmFinder::run_single (choice_imgA_->value(), choice_imgB_->value());
}


// END OF FILE

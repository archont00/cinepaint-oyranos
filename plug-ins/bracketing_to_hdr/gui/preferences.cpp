/*
 * preferences.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  preferences.cpp
*/
#include <FL/Fl_Preferences.H>
#include "../br_core/br_version.hpp"
#include "../br_core/ResponseSolverBase.hpp"
#include "../br_core/Br2Hdr.hpp"        // br::Br2Hdr
#include "AllWindows.hpp"               // the global `allWins'
#include "gui_rest.hpp"                 // load_response_curve()
#include "preferences.hpp"


namespace br {

//==============================
//  prototypes of local helpers
//==============================
static int cmp_version (const char* version);


/**+*************************************************************************\n
  Writes the preferences.
  Change-Log:
   0.5.2: 
    - Writing of the version string.
    - method_Z saved
   0.7.0:
    - Option "Show offset buttons" saved.
    - File chooser's start paths saved for images and for curves, and filter
       value for images.
    - lastopen info saved for displcmFinderUI saved
******************************************************************************/
void write_preferences() 
{
  printf("%s()...\n", __func__);
  Br2HdrManager &  theBr2Hdr = Br2Hdr::Instance();

  Fl_Preferences app (Fl_Preferences::USER, "cinepaint", "plug-ins/bracketing_to_hdr");
    
    app.set ("Version", BR_VERSION_STRING);
    
    Fl_Preferences settings (app, "Settings");
      
      settings.set ("show_offset_buttons", allWins.main->is_show_offset_buttons());
      settings.set ("auto_update_followup", theBr2Hdr.isAutoUpdateFollowUp());
      settings.set ("auto_load_response", allWins.main->is_auto_load_response());
      settings.set ("auto_save_prefs", allWins.main->is_auto_save_prefs());      
      settings.set ("use_extern_response", theBr2Hdr.isUseExternResponse());
      settings.set ("solve_mode", theBr2Hdr.solve_mode());
      settings.set ("method_Z", theBr2Hdr.method_Z());

    /*  Which (secondary) windows were open in the last session?
         Solange Fenster noch nicht freigeben, sondern nur versteckt wurden, 
         musste als Kriterium fuer "not open" 'not shown()' dienen. -- Inzwischen
         koennen Huellobjekte freigegeben werden. Aber dennoch mag als Kriterium,
         ein Fenster (deren Huellobjekt) in naechster Sitzung wiederzueroeffnen,
         weiterhin dienen, ob es sichtbar war. Beispiele fuer beides. */
    Fl_Preferences last_open_wins (app, "last_open_windows");
    
      bool onHistogram = allWins.histogram ? (allWins.histogram->window()->shown() ? 1 : 0) : 0;
      bool onTestSuite = allWins.testSuite ? (allWins.testSuite->window()->shown() ? 1 : 0) : 0;
      bool onDisplcmUI = allWins.displcmFinderUI ? (allWins.displcmFinderUI->window()->shown() ? 1 : 0) : 0;
    
      last_open_wins.set ("response", allWins.response ? 1 : 0);
      last_open_wins.set ("followUp", allWins.followUp ? 1 : 0);
      //last_open_wins.set ("histogram", allWins.histogram ? 1 : 0);
      //last_open_wins.set ("testSuite", allWins.testSuite ? 1 : 0);
      last_open_wins.set ("histogram", onHistogram);
      last_open_wins.set ("testSuite", onTestSuite);
      last_open_wins.set ("displcmFinderUI", onDisplcmUI);
            
    /*  Save window positions. We assume that main window always exists. */
    Fl_Preferences main_win_pos (app, "main_window_pos");
      
      main_win_pos.set ("x", allWins.main->window()->x());
      main_win_pos.set ("y", allWins.main->window()->y());
      main_win_pos.set ("w", allWins.main->window()->w());
      main_win_pos.set ("h", allWins.main->window()->h());
    
    if (allWins.response)
    {
      Fl_Preferences response_win_pos (app, "response_window_pos");
      
      response_win_pos.set ("x", allWins.response->window()->x());
      response_win_pos.set ("y", allWins.response->window()->y());
      response_win_pos.set ("w", allWins.response->window()->w());
      response_win_pos.set ("h", allWins.response->window()->h());
    }
    
    if (allWins.followUp) 
    {
      Fl_Preferences followUp_win_pos (app, "followUp_window_pos");
      
      followUp_win_pos.set ("x", allWins.followUp->window()->x());
      followUp_win_pos.set ("y", allWins.followUp->window()->y());
      followUp_win_pos.set ("w", allWins.followUp->window()->w());
      followUp_win_pos.set ("h", allWins.followUp->window()->h());
    }
      
    if (allWins.histogram) 
    {
      Fl_Preferences histogram_win_pos (app, "histogram_window_pos");
      
      histogram_win_pos.set ("x", allWins.histogram->window()->x());
      histogram_win_pos.set ("y", allWins.histogram->window()->y());
      histogram_win_pos.set ("w", allWins.histogram->window()->w());
      histogram_win_pos.set ("h", allWins.histogram->window()->h());
    }
      
    if (allWins.testSuite) 
    {
      Fl_Preferences testSuite_win_pos (app, "testSuite_window_pos");
      
      testSuite_win_pos.set ("x", allWins.testSuite->window()->x());
      testSuite_win_pos.set ("y", allWins.testSuite->window()->y());
      testSuite_win_pos.set ("w", allWins.testSuite->window()->w());
      testSuite_win_pos.set ("h", allWins.testSuite->window()->h());
    }
    
    /*  Save filenames of loaded external response curves */
    Fl_Preferences curves_extern (app, "external_response_curves");
      
      const char* name;
      name = theBr2Hdr.getFilenameCurveExtern(0);
      curves_extern.set ("R", name ? name : "");
      name = theBr2Hdr.getFilenameCurveExtern(1);
      curves_extern.set ("G", name ? name : "");
      name = theBr2Hdr.getFilenameCurveExtern(2);
      curves_extern.set ("B", name ? name : "");
     
    /*  Save file chooser's start paths and filters for images and curves */
    Fl_Preferences file_chooser (app, "file_chooser");
     
      name = allWins.image_path();
      file_chooser.set ("image_path", name ? name : "");
      name = allWins.curve_path();
      file_chooser.set ("curve_path", name ? name : "");
      file_chooser.set ("image_filter_value", allWins.image_filter_value());
}


/**+*************************************************************************\n
  Reads the preferences and applies. Window positions of existing windows could
   be applied already here: would save the permanent variables `main_x' etc in
   class AllWindows. For windows though which not yet exist in this moment (e.g.
   testSuite) we have to store their positions for using them later in their
   creation moment.
******************************************************************************/
void read_preferences() 
{
  printf("%s()...\n", __func__);
  Br2HdrManager &  theBr2Hdr = Br2Hdr::Instance();
  
  int boolValue;     // int! Fl_Pref provides no set()/get() for bool
  int intValue;
  //char buffer[80];
  //double doubleValue;
  char path [FL_PATH_MAX];
  char version_str [16];
  
  Fl_Preferences app (Fl_Preferences::USER, "cinepaint", "plug-ins/bracketing_to_hdr");
  
  app.getUserdataPath (path, sizeof(path));
  app.get ("Version", version_str, "", sizeof(version_str)-1);
  //printf("Version=\"%s\", cmp_version()=%d\n", version_str, cmp_version(version_str));

    /*  Read the settings and set the menubar items of the main window */
    Fl_Preferences settings (app, "Settings");
      
      settings.get ("show_offset_buttons", boolValue, true);
      allWins.main->set_show_offset_buttons (boolValue);
      
      settings.get ("auto_update_followup", boolValue, false);
      theBr2Hdr.setAutoUpdateFollowUp (boolValue);  // no broadcast
      allWins.main->set_auto_update_followup (boolValue);
    
      settings.get ("auto_load_response", boolValue, false);
      allWins.main->set_auto_load_response (boolValue);
      
      settings.get ("auto_save_prefs", boolValue, false);
      allWins.main->set_auto_save_prefs (boolValue);
      
      settings.get ("use_extern_response", boolValue, false);
      theBr2Hdr.setUseExternResponse (boolValue);  // GUI update via broadcast
    
      settings.get ("solve_mode", intValue, -1);
      if (intValue >= 0) allWins.main->solve_mode ((ResponseSolverBase::SolveMode)intValue);
      
      settings.get ("method_Z", intValue, -1);
      if (intValue > 0) allWins.main->method_Z (intValue);
      
    Fl_Preferences last_open_wins (app, "last_open_windows");
    
      last_open_wins.get ("response", boolValue, false);  
      allWins.lastopen_response = boolValue;
      last_open_wins.get ("followUp", boolValue, false);
      allWins.lastopen_followUp = boolValue;
      last_open_wins.get ("displcmFinderUI", boolValue, false);
      allWins.lastopen_displcmFinderUI = boolValue;
      last_open_wins.get ("histogram", boolValue, false);
      allWins.lastopen_histogram = boolValue;
      last_open_wins.get ("testSuite", boolValue, false);
      allWins.lastopen_testSuite = boolValue;
    
    Fl_Preferences main_win_pos (app, "main_window_pos");
      
      main_win_pos.get ("x", allWins.main_x, -1);
      main_win_pos.get ("y", allWins.main_y, -1);
      main_win_pos.get ("w", allWins.main_w, -1);
      main_win_pos.get ("h", allWins.main_h, -1);
    
    Fl_Preferences response_win_pos (app, "response_window_pos");
      
      response_win_pos.get ("x", allWins.response_x, -1);
      response_win_pos.get ("y", allWins.response_y, -1);
      response_win_pos.get ("w", allWins.response_w, -1);
      response_win_pos.get ("h", allWins.response_h, -1);
    
    Fl_Preferences followUp_win_pos (app, "followUp_window_pos");
      
      followUp_win_pos.get ("x", allWins.followUp_x, -1);
      followUp_win_pos.get ("y", allWins.followUp_y, -1);
      followUp_win_pos.get ("w", allWins.followUp_w, -1);
      followUp_win_pos.get ("h", allWins.followUp_h, -1);
    
    Fl_Preferences histogram_win_pos (app, "histogram_window_pos");
      
      histogram_win_pos.get ("x", allWins.histogram_x, -1);
      histogram_win_pos.get ("y", allWins.histogram_y, -1);
      histogram_win_pos.get ("w", allWins.histogram_w, -1);
      histogram_win_pos.get ("h", allWins.histogram_h, -1);
    
      if (allWins.histogram_x != -1 && allWins.histogram_y != -1 &&
          allWins.histogram_w != -1 && allWins.histogram_h != -1)
        allWins.have_saved_pos_histogram = true;  
    
    Fl_Preferences testSuite_win_pos (app, "testSuite_window_pos");
      
      testSuite_win_pos.get ("x", allWins.testSuite_x, -1);
      testSuite_win_pos.get ("y", allWins.testSuite_y, -1);
      testSuite_win_pos.get ("w", allWins.testSuite_w, -1);
      testSuite_win_pos.get ("h", allWins.testSuite_h, -1);
      
      if (allWins.testSuite_x != -1 && allWins.testSuite_y != -1 &&
          allWins.testSuite_w != -1 && allWins.testSuite_h != -1)
        allWins.have_saved_pos_testSuite = true;  

    if (allWins.main->is_auto_load_response()) 
    {    
      Fl_Preferences curves_extern (app, "external_response_curves");
      
      curves_extern.get ("R", path, "", FL_PATH_MAX-1);  load_response_curve (0, path);
      curves_extern.get ("G", path, "", FL_PATH_MAX-1);  load_response_curve (1, path);
      curves_extern.get ("B", path, "", FL_PATH_MAX-1);  load_response_curve (2, path);
      // Note: We re-use `path', because `app's path is not used anymore.
    }
    
    Fl_Preferences file_chooser (app, "file_chooser");
     
      file_chooser.get ("image_path", path, "", FL_PATH_MAX-1);  allWins.image_path (path);
      file_chooser.get ("curve_path", path, "", FL_PATH_MAX-1);  allWins.curve_path (path);
      file_chooser.get ("image_filter_value", intValue, 0);
      allWins.image_filter_value (intValue); 
      
    /*  Handle different versions */
    if (cmp_version (version_str))  // version_str != BR_VERSION_STRING
    {
      /*  The main window default size was often changed between versions, a resize
         to user's saved size could cause weak results. To prevent this resizing in
         allWins.run() we simulate a read error: */
      allWins.main_x = -1;
    }
}


//==============================
//  Definition of local helpers
//==============================

/**+*************************************************************************\n
  @return Less than, equal to, or greater than zero, if BR_VERSION_STRING is found,
   respectively, to be less than, to match, or be greater than \a version.
******************************************************************************/
static int cmp_version (const char* version)
{
    return strcmp (BR_VERSION_STRING, version);
}


}  // namespace "br"

// END OF FILE

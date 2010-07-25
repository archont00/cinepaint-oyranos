/*
 * AllWindows.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file  AllWindows.hpp
   
  Provides a global instance \a allWins holding all toplevel window instances
   or more exact: their "WinClass" wrapper classes. That way it allows
   communication between toplevel windows: AllWindows.hpp *includes* the
   WinClass header files like "gui.h" to know the classes declared there and
   *is included* by the WinClass source files like "gui.cxx" to inform the
   class implementations (via `allWins.****') about other current existing
   toplevel windows. Further, AllWindows provides for each toplevel wrapper
   class the function "do_wrapper()" (e.g. do_response() for ResponseWinClass)
   which creates the corresponding wrapper object if not yet existing and can
   be used in GUI menus.
*/
#ifndef AllWindows_hpp
#define AllWindows_hpp

#include <FL/Fl_File_Chooser.H>
#include "gui.h"
//#include "gui_displcm.h"
#include "EventTesterWinClass.h"
#include "DisplcmFinder_UI.h"

// #include <iostream>
// using std::cout;

/**============================================================================
  
  @struct  AllWindows
   
  Collects the pointers of all top level windows respective their wrapper
   classes. Maintains creation, showing, hiding, deleting. Provides for each
   toplevel wrapper class the function "do_wrapper()" (e.g. do_response() for 
   ResponseWinClass) which creates the corresponding wrapper object if not yet
   existing and can be used in GUI menus.
   
==============================================================================*/
class AllWindows 
{
    char*  image_path_;
    char*  curve_path_;
    int  image_filter_value_;

public:
    MainWinClass *  main;
    ResponseWinClass *  response;
    FollowUpWinClass *  followUp;
    HistogramWinClass *  histogram;
    TestSuiteWinClass *  testSuite;
    HelpAboutWinClass *  helpAbout;
    HelpTutorialWinClass *  helpTutorial;
    EventTesterWinClass *  eventTester;
    //DisplacementsWinClass *  displacements;
    DisplcmFinder_UI *  displcmFinderUI;
    Fl_File_Chooser *  fileChooser;
    
    int  main_x, main_y, main_w, main_h;
    int  main_x_def, main_y_def, main_w_def, main_h_def;  // default positions
    
    int  response_x, response_y, response_w, response_h;
    int  response_x_def, response_y_def, response_w_def, response_h_def;
    
    int  followUp_x, followUp_y, followUp_w, followUp_h;
    int  followUp_x_def, followUp_y_def, followUp_w_def, followUp_h_def;
    
    int  histogram_x, histogram_y, histogram_w, histogram_h;
    bool have_saved_pos_histogram;
    
    int  testSuite_x, testSuite_y, testSuite_w, testSuite_h;
    bool have_saved_pos_testSuite;
    
    //  Which (secondary) windows were opened in the last (saved) session?
    bool lastopen_response, lastopen_followUp, lastopen_histogram, 
         lastopen_testSuite, lastopen_displcmFinderUI;
    
    //  Which (secondary) top windows are to open at start? [Currently unused]
    bool initialopen_response, initialopen_followUp, initialopen_histogram, initialopen_testSuite;
    
        
    AllWindows();
    
    int  run();
    void clear();
    void do_response();
    void do_followUp();
    void do_histogram();
    void do_testSuite();
    void do_helpAbout();
    void do_helpTutorial();
    void do_eventTester();
    //void do_displacements();
    void do_displcmFinderUI();
    
    int  do_file_chooser (                 // O - Number of selected files
             const char *value,            // I - Initial path value
             const char *pattern,          // I - Filename patterns
             int        type,              // I - Type of file to select
             const char *title,            // I - Window title string
             int        pattern_value=0 ); // I - Filename pattern pre-selection

    void        grab_win_positions_as_default();
    void        evaluate_read_win_positions();
    void        reset_win_positions();
    
    void        image_path (const char* path);
    const char* image_path() const              {return image_path_;}
    void        curve_path (const char* path);
    const char* curve_path() const              {return curve_path_;}
    void        image_filter_value (int v)      {image_filter_value_ = v;}
    int         image_filter_value() const      {return image_filter_value_;}
    
    /*  devel & debug helper */
    void report_lastopen();
    
private:
    /*  backup functions for those window wrappers we want to delete (and not only
         to hide) via callbacks. If needed, window positions could be saved here. */
    static void backup_histogram_(void* I)   {((AllWindows*)I)->backup_histogram_i();}
    void        backup_histogram_i()         {/*cout<<__func__<<"()\n";*/ histogram=0;}
    
    static void backup_testSuite_(void* I)   {((AllWindows*)I)->backup_testSuite_i();}
    void        backup_testSuite_i()         {/*cout<<__func__<<"()\n";*/ testSuite=0;}
    
    static void backup_eventTester_(void* I) {((AllWindows*)I)->backup_eventTester_i();}
    void        backup_eventTester_i()       {/*cout<<__func__<<"()\n";*/ eventTester=0;}
};


/**
  Our global instance to collect and maintain the top level windows
*/
extern AllWindows  allWins;


#endif  // AllWindows_hpp

// END OF FILE

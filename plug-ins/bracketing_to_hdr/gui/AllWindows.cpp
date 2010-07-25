/*
 * AllWindows.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  AllWindows.cpp
*/

#include <cstdio>                       // printf()
#include <cstring>                      // strcasecmp()

#include <FL/Fl_Image.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_JPEG_Image.H>

#include "AllWindows.hpp"
#include "EventTesterWinClass.h"
#include "br_messages_gui.hpp"          // init_gui_messages()
#include "preferences.hpp"              // read_preferences(), write_preferences()
#include "../br_core/Exception.hpp"     // br::Exception
#include "../br_core/br_messages.hpp"   // br::message()
#include "../br_core/path_utils.hpp"    // buildTmpPath(), get_dcraw_path()
#include "../br_core/i18n.hpp"          // macro _()


using namespace br;

//==============================
//  Prototypes of local helpers
//==============================
Fl_Image*
my_raw_file_check (const char* fname, uchar *header, int headerlen);


/**+*************************************************************************\n
  Ctor
******************************************************************************/
AllWindows::AllWindows() 
  : 
    image_path_         (0),
    curve_path_         (0),
    image_filter_value_ (0),
    
    main                (0), 
    response            (0), 
    followUp            (0), 
    histogram           (0),
    testSuite           (0), 
    helpAbout           (0),
    helpTutorial        (0),
    eventTester         (0),
    //displacements       (0),
    displcmFinderUI     (0),
    fileChooser         (0),
    
    lastopen_response   (false),
    lastopen_followUp   (false),
    lastopen_histogram  (false),
    lastopen_testSuite  (false),
    lastopen_displcmFinderUI (false)
{
    init_gui_messages();
    
    Fl::background (220,220,220);
    //Fl::scheme ("gtk+");
}      


/**+*************************************************************************\n
  Runs the GUI driven application.
  
  @todo Als obligates Startfenster nur \a main, andere gemaess \a lastopen? Bedarf 
   dann anderes Regimes fuer Default-Groessen von \a response und \a followUp. Aber
   ist derzeit nicht sinnvoll, ohne Response- und Followup-Fenster.
******************************************************************************/
int
AllWindows::run()
{
  int retcode = -1;  // expected by the plugin code for an executation error
  
  try { // to have at least a msg of the unhandled exception
    
    /*  Create the start top level windows (their wrapper classes)*/
    main     = new MainWinClass();
    response = new ResponseWinClass();
    followUp = new FollowUpWinClass();
    
    /*  Positionate the top windows on screen (impossible in FLUID) */
    main     -> window()->position(1,0);  // "(1,0)" due to the "(0,0)"-FLTK bug
    response -> window()->position(850,0);
    followUp -> window()->position(380,0);  //resize(380,0, 520,440);
    
    /*  Send an empty event to initialize some receiver widgets */
    Br2Hdr::Instance().distribEvent().distribute (Br2Hdr::NO_EVENT);
    
    /*  Take the virgin (FLUID-adjusted) sizes as default values */
    grab_win_positions_as_default();
    
    /*  Read the saved preferences and settings */
    read_preferences();
    //report_lastopen();
    
    /*  Check the read positions for plausibility */
    evaluate_read_win_positions();
    
    /*  Apply the read positions. Checks for shortenings take place inside resize() */
    main -> window()->resize (main_x, main_y, main_w, main_h);
    response -> window()->resize (response_x, response_y, response_w, response_h);
    followUp -> window()->resize (followUp_x, followUp_y, followUp_w, followUp_h);
    
    /*  Offer the input file chooser dialog at first */
    main -> load_input_images();
    
    /*  Open contingent windows which were open in the last session */
    if (lastopen_histogram) do_histogram();
    if (lastopen_testSuite) do_testSuite();
    if (lastopen_displcmFinderUI) do_displcmFinderUI();
    //do_displacements();  // zum Testen
    
    /*  Show the start windows -- `main' at last to bring it on top */
    followUp -> show();
    response -> show();
    main -> show();
    
    /*  Fltk-Loop for the main window (life span of the application) */
    while (main->window()->shown()) {
      Fl::wait();
      WidgetWrapper::do_wrappers_deletion();
    }
      
    /*  Save preferences if wished */
    if (main->is_auto_save_prefs()) 
      write_preferences();
    
    printf("OK - \"bracketing_to_hdr\" finished.\n");
    retcode = 0;
  
  }
  catch (br::Exception e) {
    printf("\n*** Unhandled br::Exception ***\n");
    e.report();
  }  
  catch (std::exception e) {
    printf("\n*** Unhandled std::exception ***: %s\n", e.what());
  }
  catch (...) {
    printf("\n*** Unknown exception (no `std') ***\n");
  }

  //clear();    // For the plug-in we can let it do the OS at once
  return retcode;
}

/*!
  Indented for only use in Dtor, otherwise zeroing would be needed.
*/
void AllWindows::clear()
{
    delete fileChooser;
    delete main;
    delete followUp;
    delete response;
    delete histogram;
    delete testSuite;
    delete helpAbout;
    delete helpTutorial;
    delete eventTester;
    //delete displacements;
    delete displcmFinderUI;
    
    if (image_path_) free (image_path_);
    if (curve_path_) free (curve_path_);
}

void AllWindows::do_response()
{
    if (!response) 
      response = new ResponseWinClass();
    
    response->show();  
}

void AllWindows::do_followUp()
{
    if (!followUp)
      followUp = new FollowUpWinClass();
    
    followUp->show();  
}

void AllWindows::do_histogram()
{
    if (!histogram) {
      histogram = new HistogramWinClass();
      WidgetWrapper::register_backup_func (histogram, backup_histogram_, this);
    
      if (have_saved_pos_histogram) 
        histogram->window()->resize (histogram_x, histogram_y, histogram_w, histogram_h);
    }
    histogram->window()->show(); 
}

void AllWindows::do_testSuite()
{
    if (!testSuite) {
      testSuite = new TestSuiteWinClass (Br2Hdr::Instance());
      WidgetWrapper::register_backup_func (testSuite, backup_testSuite_, this);
      
      if (have_saved_pos_testSuite) 
        testSuite->window()->resize (testSuite_x, testSuite_y, testSuite_w, testSuite_h);
    }
    testSuite->show();  
}

void AllWindows::do_helpAbout()
{
    if (!helpAbout)
      helpAbout = new HelpAboutWinClass();
    
    helpAbout->window()->show();
}      

// void AllWindows::do_displacements()
// {
//     if (!displacements) displacements = new DisplacementsWinClass();
//     displacements->window()->show();
// }      

void AllWindows::do_displcmFinderUI()
{
    if (!displcmFinderUI) displcmFinderUI = new DisplcmFinder_UI();
    displcmFinderUI->window()->show();
}      

/****************************************************************************\n
  FLUID-generiertes HelpTutorialWinClass. Komisch, dass Wiederoeffnen stets an
   gleicher Schirmposition.
******************************************************************************/
void AllWindows::do_helpTutorial()
{
    if (!helpTutorial)
      helpTutorial = new HelpTutorialWinClass();

    Fl_Window* win = helpTutorial->window();
    printf("\t(x,y)=(%d,%d),  (w,h)=(%d,%d)", win->x(), win->y(), win->w(), win->h());
    printf("\twindow=%p\n", (void*)win); 
    
    helpTutorial->window()->show();
}      


void AllWindows::do_eventTester()
{
    if (!eventTester) {
      eventTester = new EventTesterWinClass();
      WidgetWrapper::register_backup_func (eventTester, backup_eventTester_, this);
    }
    eventTester->window()->show();
}      


/**+*************************************************************************\n
  do_file_chooser()  -  Adapted from Michael Sweet's "flphoto/album.cxx".

  @param value: (I) Initial path value.
  @param pattern: (I) Filename patterns.
  @param type: (I) Type of file to select.
  @param title: (I) Window title string.
  @param pattern_value: (I): pattern pre-selection, default=0 (first pattern).
  @return  Number of selected files. 
******************************************************************************/
int                                 // O - Number of selected files
AllWindows::do_file_chooser (       
        const char *value,          // I - Initial path value
        const char *pattern,        // I - Filename patterns
        int        type,            // I - Type of file to select
        const char *title,          // I - Window title string
        int        pattern_value )  // I - pattern pre-selection
{
    //  Create the file chooser if it doesn't exist...
    if (!fileChooser)
    {
      if (!value || !*value)
        value = ".";

      Fl_Shared_Image::add_handler (my_raw_file_check);
      Fl_File_Icon::load_system_icons();
    
      fileChooser = new Fl_File_Chooser (value, pattern, type, title);
      fileChooser -> filter_value (pattern_value);
    }
    else
    {
      fileChooser -> type (type);
      fileChooser -> filter (pattern);
      fileChooser -> label (title);
      fileChooser -> filter_value (pattern_value);

      if (value && *value)
        fileChooser -> value (value);
      else
        fileChooser -> rescan();
    }

    //  Show the chooser and wait for something to happen...
    fileChooser->show();

    while (fileChooser->shown())
      Fl::wait();

    return (fileChooser->count());
}


/**+*************************************************************************\n
  Evaluates the read window positions (read by read_preferences()) for 
   plausibility (a read error was marked there by the value "-1").
******************************************************************************/
void 
AllWindows::evaluate_read_win_positions()
{
    if (main_x < 0 || main_y < 0 || main_w < 0 || main_h < 0)
    {
      main_x = main_x_def;
      main_y = main_y_def;
      main_w = main_w_def;
      main_h = main_h_def;
    }
    if (response_x < 0 || response_y < 0 || response_w < 0 || response_h < 0)
    {
      response_x = response_x_def;
      response_y = response_y_def;
      response_w = response_w_def;
      response_h = response_h_def;
    }
    if (followUp_x < 0 || followUp_y < 0 || followUp_w < 0 || followUp_h < 0)
    {
      followUp_x = followUp_x_def;
      followUp_y = followUp_y_def;
      followUp_w = followUp_w_def;
      followUp_h = followUp_h_def;
    }
}

/**+*************************************************************************\n
  Grabs the current positions of some windows as their default values.
******************************************************************************/
void 
AllWindows::grab_win_positions_as_default()
{
    if (main) {
      main_x_def = main->window()->x();
      main_y_def = main->window()->y();
      main_w_def = main->window()->w();
      main_h_def = main->window()->h();
    }
    if (response) {
      response_x_def = response->window()->x();
      response_y_def = response->window()->y();
      response_w_def = response->window()->w();
      response_h_def = response->window()->h();
    }  
    if (followUp) {
      followUp_x_def = followUp->window()->x();
      followUp_y_def = followUp->window()->y();
      followUp_w_def = followUp->window()->w();
      followUp_h_def = followUp->window()->h();
    }  
}   


/**+*************************************************************************\n
  Sets the positions and sizes of windows to their default values.
******************************************************************************/
void 
AllWindows::reset_win_positions()
{
    main->window()->resize (main_x_def, main_y_def, main_w_def, main_h_def);
    response->window()->resize (response_x_def, response_y_def, response_w_def, response_h_def);
    followUp->window()->resize (followUp_x_def, followUp_y_def, followUp_w_def, followUp_h_def);
    
    //  Windows for which we haven't explicite default pos. we simply recreate.
    //   PROBLEM: User settings in these windows get lost.
    if (histogram) {
      bool shown = histogram->window()->shown();
      delete histogram;
      histogram = new HistogramWinClass();
      if (shown) histogram->window()->show();
    }
    have_saved_pos_histogram = false;  // ie. don't resize in do_histogram()!
    
    if (testSuite) {
      bool shown = testSuite->window()->shown();
      delete testSuite;
      testSuite = new TestSuiteWinClass (Br2Hdr::Instance());
      if (shown) testSuite->window()->show();
    }
    have_saved_pos_testSuite = false;  // ie. don't resize in do_testSuite()!
}
    

void 
AllWindows::image_path (const char* path)
{
    if (image_path_) free (image_path_);
    if (path) image_path_ = strdup (path); else image_path_ = 0;
}

void 
AllWindows::curve_path (const char* path)
{
    if (curve_path_) free (curve_path_);
    if (path) curve_path_ = strdup (path); else curve_path_ = 0;
}


void 
AllWindows::report_lastopen()   // devel helper
{
    printf("[last open windows:]\n");
    printf("\tresponse: %d\n", lastopen_response);
    printf("\tfollowUp: %d\n", lastopen_followUp);
    printf("\thistogram: %d\n", lastopen_histogram);
    printf("\ttestSuite: %d\n", lastopen_testSuite);
    printf("\tdisplcmFinderUI: %d\n", lastopen_displcmFinderUI);
}



//==================================
//  Implementation of local helpers
//==================================

/**+*************************************************************************\n
  Function of type fl_check_images(). Laesst `dcraw' ein Vorschaubild in die Datei
   ".preview.ppm" schreiben und returniert dieses eingelesen qua Fl_JPEG_Image.
   Deletion of the returned dyn. Fl_Image handled by Fl_Shared_Image::reload().
******************************************************************************/
Fl_Image*
my_raw_file_check (const char* fname, uchar *header, int headerlen)
{
  const char*  dcrawExe = get_dcraw_path();
  
  if (! dcrawExe) {
    fprintf (stderr, _("Program `dcraw' is required to preview RAW files.\n"));
    return NULL;
  }
  
  /*  Avoid senseless `dcraw' attempts. An alternative would be to allow only
     the RAW file extensions, but their set seems not fix. */
  const char*  ext = strrchr (fname,'.');
  if (ext) ext++;
  if (strcasecmp (ext,"png") == 0) return NULL;
  if (strcasecmp (ext,"xbm") == 0) return NULL;
  if (strcasecmp (ext,"jpg") == 0) return NULL;
  
  char  previewPath [1024];
  char  command [1024];
  
  if (! buildTmpPath (previewPath, ".preview.ppm", 1024))
    return NULL;

  snprintf (command, 1024, "%s -e -v -c \'%s\' > \'%s\' ", dcrawExe, fname, previewPath);

  if (system (command)) 
    return NULL;

  return new Fl_JPEG_Image (previewPath);
}




//===========================================================
//  The global AllWindow instance declared in "AllWindows.h"
//===========================================================
AllWindows  allWins;


// END OF FILE

// generated by Fast Light User Interface Designer (fluid) version 1.0108

#ifndef gui_h
#define gui_h
#include <FL/Fl.H>
#include "../br_core/Br2Hdr.hpp"
#include "../br_core/testtools.hpp"
#include "../br_core/ResponseSolverBase.hpp"
#include "../br_core/EventReceiver.hpp"
#include "../FL_adds/Fl_IntInput_Slider.hpp"
#include "ImageTable.hpp"
#include "OffsetTable.hpp"
#include "ExifTable.hpp"
#include "WidgetWrapper.hpp"
#include "gui_rest.hpp"
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include "ProgressBar.hpp"
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Round_Button.H>
#include "StopvalueChoicer.hpp"
#include <FL/Fl_Check_Button.H>
#include "buttons.hpp"
#include <FL/Fl_Button.H>
#include "StatusLine.hpp"
#include "TheProgressBar.hpp"

class MainWinClass : public br::EventReceiver {
  friend class MainWinMenubarWatcher; 
  int h_offsetbar_; // target height
  int h_timesbar_; // target height
public:
  MainWinClass();
private:
  Fl_Double_Window *window_;
  Fl_Menu_Bar *menubar_;
  static Fl_Menu_Item menu_menubar_[];
  void cb_Open_i(Fl_Menu_*, void*);
  static void cb_Open(Fl_Menu_*, void*);
  void cb_Clear_i(Fl_Menu_*, void*);
  static void cb_Clear(Fl_Menu_*, void*);
  void cb_Clear1_i(Fl_Menu_*, void*);
  static void cb_Clear1(Fl_Menu_*, void*);
  void cb_Quit_i(Fl_Menu_*, void*);
  static void cb_Quit(Fl_Menu_*, void*);
  static Fl_Menu_Item *mbar_item_Init_Calctor_;
  void cb_mbar_item_Init_Calctor__i(Fl_Menu_*, void*);
  static void cb_mbar_item_Init_Calctor_(Fl_Menu_*, void*);
  static Fl_Menu_Item *mbar_item_Compute_Response_;
  void cb_mbar_item_Compute_Response__i(Fl_Menu_*, void*);
  static void cb_mbar_item_Compute_Response_(Fl_Menu_*, void*);
  static Fl_Menu_Item *mbar_item_Compute_HDR_;
  void cb_mbar_item_Compute_HDR__i(Fl_Menu_*, void*);
  static void cb_mbar_item_Compute_HDR_(Fl_Menu_*, void*);
  static Fl_Menu_Item *mbar_item_Compute_logHDR_;
  void cb_mbar_item_Compute_logHDR__i(Fl_Menu_*, void*);
  static void cb_mbar_item_Compute_logHDR_(Fl_Menu_*, void*);
  void cb_Identity_i(Fl_Menu_*, void*);
  static void cb_Identity(Fl_Menu_*, void*);
  void cb_Identity1_i(Fl_Menu_*, void*);
  static void cb_Identity1(Fl_Menu_*, void*);
  void cb_Triangle_i(Fl_Menu_*, void*);
  static void cb_Triangle(Fl_Menu_*, void*);
  void cb_Sinus_i(Fl_Menu_*, void*);
  static void cb_Sinus(Fl_Menu_*, void*);
  void cb_Identity_0_Param_i(Fl_Menu_*, void*);
  static void cb_Identity_0_Param(Fl_Menu_*, void*);
  void cb_Triangle_Param_i(Fl_Menu_*, void*);
  static void cb_Triangle_Param(Fl_Menu_*, void*);
  void cb_Identity2_i(Fl_Menu_*, void*);
  static void cb_Identity2(Fl_Menu_*, void*);
  void cb_Identity3_i(Fl_Menu_*, void*);
  static void cb_Identity3(Fl_Menu_*, void*);
  void cb_Triangle1_i(Fl_Menu_*, void*);
  static void cb_Triangle1(Fl_Menu_*, void*);
  void cb_Sinus1_i(Fl_Menu_*, void*);
  static void cb_Sinus1(Fl_Menu_*, void*);
  void cb_Identity_0_Param1_i(Fl_Menu_*, void*);
  static void cb_Identity_0_Param1(Fl_Menu_*, void*);
  void cb_Triangle_Param1_i(Fl_Menu_*, void*);
  static void cb_Triangle_Param1(Fl_Menu_*, void*);
  void cb_Images_i(Fl_Menu_*, void*);
  static void cb_Images(Fl_Menu_*, void*);
  void cb_Numerics_i(Fl_Menu_*, void*);
  static void cb_Numerics(Fl_Menu_*, void*);
  void cb_Calctor_i(Fl_Menu_*, void*);
  static void cb_Calctor(Fl_Menu_*, void*);
  void cb_Indices_i(Fl_Menu_*, void*);
  static void cb_Indices(Fl_Menu_*, void*);
  void cb_Pack_i(Fl_Menu_*, void*);
  static void cb_Pack(Fl_Menu_*, void*);
  void cb_Status_i(Fl_Menu_*, void*);
  static void cb_Status(Fl_Menu_*, void*);
  void cb_Weight_i(Fl_Menu_*, void*);
  static void cb_Weight(Fl_Menu_*, void*);
  void cb_Offsets_i(Fl_Menu_*, void*);
  static void cb_Offsets(Fl_Menu_*, void*);
  void cb_Intersection_i(Fl_Menu_*, void*);
  static void cb_Intersection(Fl_Menu_*, void*);
  void cb_Follow_i(Fl_Menu_*, void*);
  static void cb_Follow(Fl_Menu_*, void*);
  void cb_Response_i(Fl_Menu_*, void*);
  static void cb_Response(Fl_Menu_*, void*);
  void cb_Histogram_i(Fl_Menu_*, void*);
  static void cb_Histogram(Fl_Menu_*, void*);
  void cb_Displacements_i(Fl_Menu_*, void*);
  static void cb_Displacements(Fl_Menu_*, void*);
  void cb_Offset_i(Fl_Menu_*, void*);
  static void cb_Offset(Fl_Menu_*, void*);
  void cb_TestSuite_i(Fl_Menu_*, void*);
  static void cb_TestSuite(Fl_Menu_*, void*);
  void cb_EventDebugger_i(Fl_Menu_*, void*);
  static void cb_EventDebugger(Fl_Menu_*, void*);
  static Fl_Menu_Item *mbar_item_timesbar_;
  void cb_mbar_item_timesbar__i(Fl_Menu_*, void*);
  static void cb_mbar_item_timesbar_(Fl_Menu_*, void*);
  static Fl_Menu_Item *mbar_item_offsetbar_;
  void cb_mbar_item_offsetbar__i(Fl_Menu_*, void*);
  static void cb_mbar_item_offsetbar_(Fl_Menu_*, void*);
  static Fl_Menu_Item *mbar_item_auto_update_followup_;
  void cb_mbar_item_auto_update_followup__i(Fl_Menu_*, void*);
  static void cb_mbar_item_auto_update_followup_(Fl_Menu_*, void*);
  static Fl_Menu_Item *mbar_item_auto_load_response_;
  static Fl_Menu_Item *mbar_item_downsample_U16_;
  static Fl_Menu_Item *mbar_item_solvemode_auto_;
  void cb_mbar_item_solvemode_auto__i(Fl_Menu_*, void*);
  static void cb_mbar_item_solvemode_auto_(Fl_Menu_*, void*);
  static Fl_Menu_Item *mbar_item_solvemode_use_qr_;
  void cb_mbar_item_solvemode_use_qr__i(Fl_Menu_*, void*);
  static void cb_mbar_item_solvemode_use_qr_(Fl_Menu_*, void*);
  static Fl_Menu_Item *mbar_item_solvemode_use_svd_;
  void cb_mbar_item_solvemode_use_svd__i(Fl_Menu_*, void*);
  static void cb_mbar_item_solvemode_use_svd_(Fl_Menu_*, void*);
  void cb_testmode_i(Fl_Menu_*, void*);
  static void cb_testmode(Fl_Menu_*, void*);
  static Fl_Menu_Item *mbar_item_methodZ_refpic_;
  void cb_mbar_item_methodZ_refpic__i(Fl_Menu_*, void*);
  static void cb_mbar_item_methodZ_refpic_(Fl_Menu_*, void*);
  static Fl_Menu_Item *mbar_item_methodZ_overlap_;
  void cb_mbar_item_methodZ_overlap__i(Fl_Menu_*, void*);
  static void cb_mbar_item_methodZ_overlap_(Fl_Menu_*, void*);
  static Fl_Menu_Item *mbar_item_enforce_refresh_;
  void cb_mbar_item_enforce_refresh__i(Fl_Menu_*, void*);
  static void cb_mbar_item_enforce_refresh_(Fl_Menu_*, void*);
  static Fl_Menu_Item *mbar_item_auto_save_prefs_;
  void cb_Save_i(Fl_Menu_*, void*);
  static void cb_Save(Fl_Menu_*, void*);
  void cb_Default_i(Fl_Menu_*, void*);
  static void cb_Default(Fl_Menu_*, void*);
  void cb_Reset_i(Fl_Menu_*, void*);
  static void cb_Reset(Fl_Menu_*, void*);
  void cb_About_i(Fl_Menu_*, void*);
  static void cb_About(Fl_Menu_*, void*);
  void cb_HDR_i(Fl_Menu_*, void*);
  static void cb_HDR(Fl_Menu_*, void*);
  Fl_Tabs *the_tab_;
  Fl_Group *tab_images_;
  ImageTable *table_images_;
  Fl_Group *tab_exif_;
  ExifTable *table_exif_;
  Fl_Group *tab_offsets_;
  OffsetTable *table_offsets_;
  Fl_Group *grp_timesbar_;
  Fl_Round_Button *radio_timesByEXIF_;
  void cb_radio_timesByEXIF__i(Fl_Round_Button*, void*);
  static void cb_radio_timesByEXIF_(Fl_Round_Button*, void*);
  Fl_Round_Button *radio_timesByStop_;
  void cb_radio_timesByStop__i(Fl_Round_Button*, void*);
  static void cb_radio_timesByStop_(Fl_Round_Button*, void*);
  Fl_Round_Button *radio_timesActiveByStop_;
  void cb_radio_timesActiveByStop__i(Fl_Round_Button*, void*);
  static void cb_radio_timesActiveByStop_(Fl_Round_Button*, void*);
  StopvalueChoicer *stopvalueChoicer_;
  Fl_Check_Button *tggl_normedTimes_;
  void cb_tggl_normedTimes__i(Fl_Check_Button*, void*);
  static void cb_tggl_normedTimes_(Fl_Check_Button*, void*);
  Fl_Group *grp_offsetbar_;
  Fl_Check_Button *bttn_use_offsets_;
  void cb_bttn_use_offsets__i(Fl_Check_Button*, void*);
  static void cb_bttn_use_offsets_(Fl_Check_Button*, void*);
  void cb_Compute_i(ComputeOffsetsButton*, void*);
  static void cb_Compute(ComputeOffsetsButton*, void*);
  ComputeResponseButton *bttn_init_calctor_;
  void cb_M_i(Fl_Button*, void*);
  static void cb_M(Fl_Button*, void*);
// Here we realize Properties default values:
public:
  ~MainWinClass();
  void show();
  Fl_Window* window();
  bool downsample_U16();
  void load_input_images(); 
  void solve_mode (br::ResponseSolverBase::SolveMode); 
  void method_Z(int m);
private:
  void offsetbar_on();
  void offsetbar_off();
  void timesbar_on();
  void timesbar_off();
public:
  bool is_show_offset_buttons() const;
  void set_show_offset_buttons(bool b);
  bool is_auto_load_response() const;
  void set_auto_load_response(bool b);
  bool is_auto_save_prefs() const;
  void set_auto_save_prefs(bool b);
  void set_auto_update_followup(bool b);
private:
  void default_settings(); 
  void handle_Event(br::Br2Hdr::Event); 
};
#include "ResponsePlot.hpp"
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Light_Button.H>

class ResponseWinClass : public br::EventReceiver {
public:
  ResponseWinClass();
private:
  Fl_Double_Window *window_;
  ResponsePlot *plot_;
  Fl_Value_Slider *slider_grid_points_;
  void cb_slider_grid_points__i(Fl_Value_Slider*, void*);
  static void cb_slider_grid_points_(Fl_Value_Slider*, void*);
  Fl_Value_Slider *slider_smoothing_;
  void cb_slider_smoothing__i(Fl_Value_Slider*, void*);
  static void cb_slider_smoothing_(Fl_Value_Slider*, void*);
  Fl_Check_Button *toggle_use_extern_curves_;
  void cb_toggle_use_extern_curves__i(Fl_Check_Button*, void*);
  static void cb_toggle_use_extern_curves_(Fl_Check_Button*, void*);
  Fl_Choice *choice_plot_;
  void cb_choice_plot__i(Fl_Choice*, void*);
  static void cb_choice_plot_(Fl_Choice*, void*);
  static Fl_Menu_Item menu_choice_plot_[];
  static Fl_Menu_Item menu_Load[];
  void cb_All_i(Fl_Menu_*, void*);
  static void cb_All(Fl_Menu_*, void*);
  void cb_R_i(Fl_Menu_*, void*);
  static void cb_R(Fl_Menu_*, void*);
  void cb_G_i(Fl_Menu_*, void*);
  static void cb_G(Fl_Menu_*, void*);
  void cb_B_i(Fl_Menu_*, void*);
  static void cb_B(Fl_Menu_*, void*);
  Fl_Menu_Button *menubttn_save_;
  static Fl_Menu_Item menu_menubttn_save_[];
  void cb_All1_i(Fl_Menu_*, void*);
  static void cb_All1(Fl_Menu_*, void*);
  static Fl_Menu_Item *menubttn_save_item_channel_0;
  void cb_menubttn_save_item_channel_0_i(Fl_Menu_*, void*);
  static void cb_menubttn_save_item_channel_0(Fl_Menu_*, void*);
  static Fl_Menu_Item *menubttn_save_item_channel_1;
  void cb_menubttn_save_item_channel_1_i(Fl_Menu_*, void*);
  static void cb_menubttn_save_item_channel_1(Fl_Menu_*, void*);
  static Fl_Menu_Item *menubttn_save_item_channel_2;
  void cb_menubttn_save_item_channel_2_i(Fl_Menu_*, void*);
  static void cb_menubttn_save_item_channel_2(Fl_Menu_*, void*);
  Fl_Group *group_using_next_;
  Fl_Button *button_extern_all_;
  void cb_button_extern_all__i(Fl_Button*, void*);
  static void cb_button_extern_all_(Fl_Button*, void*);
  Fl_Button *button_computed_all_;
  void cb_button_computed_all__i(Fl_Button*, void*);
  static void cb_button_computed_all_(Fl_Button*, void*);
  void cb_radio_computed__i(Fl_Light_Button*, void*);
  static void cb_radio_computed_(Fl_Light_Button*, void*);
  void cb_radio_extern__i(Fl_Light_Button*, void*);
  static void cb_radio_extern_(Fl_Light_Button*, void*);
  void cb_radio_computed_1_i(Fl_Light_Button*, void*);
  static void cb_radio_computed_1(Fl_Light_Button*, void*);
  void cb_radio_extern_1_i(Fl_Light_Button*, void*);
  static void cb_radio_extern_1(Fl_Light_Button*, void*);
  Fl_Light_Button *radio_computed_[3];
  void cb_radio_computed_2_i(Fl_Light_Button*, void*);
  static void cb_radio_computed_2(Fl_Light_Button*, void*);
  Fl_Light_Button *radio_extern_[3];
  void cb_radio_extern_2_i(Fl_Light_Button*, void*);
  static void cb_radio_extern_2(Fl_Light_Button*, void*);
public:
  void update_activation_radios_Computed(); 
  void update_activation_radios_External(); 
  void update_onoff_radios_Using_Next(); 
  void update_activation_group_Using_Next(); 
  void do_which_curves(br::Br2Hdr::WhichCurves which); 
  void update_after_clear_response_extern(); 
  void show();
  Fl_Window* window();
  ResponsePlot & plot();
private:
  void handle_Event(br::Br2Hdr::Event); 
  void applyResponseCurveComputed(int channel); 
  void applyResponseCurveExtern(int channel_like); 
  void update_save_menu(); 
};
#include "FllwUpCrvPlot.hpp"
#include "RefpicChoicer.hpp"

class FollowUpWinClass {
public:
  FollowUpWinClass();
private:
  Fl_Double_Window *window_;
  FollowUpCurvePlot *plot_;
  void cb_Ref_i(RefpicChoicer*, void*);
  static void cb_Ref(RefpicChoicer*, void*);
  void cb_Channel_i(Fl_Choice*, void*);
  static void cb_Channel(Fl_Choice*, void*);
  static Fl_Menu_Item menu_Channel[];
public:
  ~FollowUpWinClass();
  void show();
  Fl_Window* window();
};

class TestSuiteWinClass : public WidgetWrapper {
  br::Br2HdrManager & m_Br2Hdr; 
  br::ParamResponseLinear param_resp_lin_; 
  br::ParamResponseHalfCos param_resp_cos_; 
public:
  TestSuiteWinClass(br::Br2HdrManager& m) ;
// Wenn doch nicht lokal, "theBr2Hdr" benutzen!
private:
  Fl_Double_Window *window_;
  void cb_window__i(Fl_Double_Window*, void*);
  static void cb_window_(Fl_Double_Window*, void*);
  Fl_IntInput_Slider *input_xdim_;
  Fl_IntInput_Slider *input_ydim_;
  Fl_Value_Slider *input_N_;
  Fl_Check_Button *check_clear_old_;
  Fl_Choice *choice_scene_;
  static Fl_Menu_Item menu_choice_scene_[];
  Fl_Choice *choice_response_;
  static Fl_Menu_Item menu_choice_response_[];
  Fl_Button *bttn_add_;
  void cb_bttn_add__i(Fl_Button*, void*);
  static void cb_bttn_add_(Fl_Button*, void*);
  Fl_Button *bttn_add_simple_;
  void cb_bttn_add_simple__i(Fl_Button*, void*);
  static void cb_bttn_add_simple_(Fl_Button*, void*);
  void cb_Bad_i(Fl_Button*, void*);
  static void cb_Bad(Fl_Button*, void*);
  Fl_Choice *choice_data_type_;
  static Fl_Menu_Item menu_choice_data_type_[];
public:
  ~TestSuiteWinClass();
  void show();
  Fl_Window* window();
private:
  void init_params(); 
  void do_add(); 
};
#include <FL/Fl_Window.H>
#include <FL/Fl_Output.H>

class HelpAboutWinClass {
public:
  HelpAboutWinClass();
private:
  Fl_Window *window_;
  void cb_Close_i(Fl_Button*, void*);
  static void cb_Close(Fl_Button*, void*);
public:
  ~HelpAboutWinClass();
  Fl_Window* window();
};

class HelpTutorialWinClass {
public:
  HelpTutorialWinClass();
private:
  Fl_Window *window_;
  void cb_Close1_i(Fl_Button*, void*);
  static void cb_Close1(Fl_Button*, void*);
public:
  ~HelpTutorialWinClass();
  Fl_Window* window();
};
#include "HistogramPlot.hpp"

class HistogramWinClass : public br::EventReceiver, public WidgetWrapper {
public:
  HistogramWinClass();
private:
  Fl_Double_Window *window_;
  void cb_window_1_i(Fl_Double_Window*, void*);
  static void cb_window_1(Fl_Double_Window*, void*);
  HistogramPlot *plot_;
  void cb_Channel1_i(Fl_Choice*, void*);
  static void cb_Channel1(Fl_Choice*, void*);
  static Fl_Menu_Item menu_Channel1[];
  Fl_Choice *choice_image_;
  void cb_choice_image__i(Fl_Choice*, void*);
  static void cb_choice_image_(Fl_Choice*, void*);
public:
  ~HistogramWinClass();
  Fl_Window* window();
private:
  void handle_Event (br::Br2Hdr::Event); 
  void build_choice_image(); 
};
#endif

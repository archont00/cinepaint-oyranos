// generated by Fast Light User Interface Designer (fluid) version 1.0107

#include <libintl.h>
#include "pdf_dialog.h"
#include "pdf.h"
#include "libgimp/stdplugins-intl.h"

Fl_Double_Window *window=(Fl_Double_Window *)0;

Fl_Choice *choice_interpreter=(Fl_Choice *)0;

static void cb_Ghostscript(Fl_Menu_*, void*) {
  interpreter_text->position(0,1024);
interpreter_text->cut();
interpreter_text->insert(GS_COMMAND_BASE, 0);
}

static void cb_xpdf(Fl_Menu_*, void*) {
  interpreter_text->position(0,1024);
interpreter_text->cut();
interpreter_text->insert(XPDF_COMMAND_BASE, 0);
}

Fl_Menu_Item menu_choice_interpreter[] = {
 {_("Ghostscript"), 0,  (Fl_Callback*)cb_Ghostscript, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {_("xPDF"), 0,  (Fl_Callback*)cb_xpdf, 0, 1, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0}
};

Fl_Multiline_Input *interpreter_text=(Fl_Multiline_Input *)0;

static void cb_Ok(Fl_Return_Button*, void*) {
  take_opts();
window->hide();
}

static void cb_Cancel(Fl_Button*, void*) {
  exit(1);
}

Fl_Choice *choice_export_format=(Fl_Choice *)0;

static void cb_choice_export_format(Fl_Choice*, void*) {
  if((choice_export_format->value() == 0 &&
   !vals.gs_has_tiff32nc) ||
    choice_export_format->value() > 1)
 choice_colourspace->value(1);
}

Fl_Menu_Item menu_choice_export_format[] = {
 {_("TIFF cmyk/rgb"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {_("PSD cmyk/rgb"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {_("PPM rgb"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {_("PS rgb"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {_("PNG48 rgb"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {_("[none]"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0}
};

Fl_Choice *choice_colourspace=(Fl_Choice *)0;

static void cb_choice_colourspace(Fl_Choice* o, void*) {
  if((choice_export_format->value() == 0 &&
   !vals.gs_has_tiff32nc) ||
    choice_export_format->value() > 1)
   o->value(1);
 printf("%d %d\n",choice_export_format->value(),o->value());
}

Fl_Menu_Item menu_choice_colourspace[] = {
 {_("Cmyk"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {_("Rgb"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0}
};

Fl_Value_Slider *slider_resolution=(Fl_Value_Slider *)0;

Fl_Choice *choice_aa_graphic=(Fl_Choice *)0;

Fl_Menu_Item menu_choice_aa_graphic[] = {
 {_("[none]"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {_("AA4"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {_("AA4+Interpolation"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0}
};

Fl_Check_Button *check_aa_text=(Fl_Check_Button *)0;

Fl_Choice *choice_aa_gridfit=(Fl_Choice *)0;

Fl_Menu_Item menu_choice_aa_gridfit[] = {
 {_("0"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {_("1"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {_("2"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0}
};

Fl_Choice *choice_viewer=(Fl_Choice *)0;

static void cb_gv(Fl_Menu_*, void*) {
  interpreter_text->position(0,1024);
interpreter_text->cut();
interpreter_text->insert(XPDF_COMMAND_BASE, 0);
}

static void cb_xpdf1(Fl_Menu_*, void*) {
  interpreter_text->position(0,1024);
interpreter_text->cut();
interpreter_text->insert(XPDF_COMMAND_BASE, 0);
}

static void cb_Acroread(Fl_Menu_*, void*) {
  interpreter_text->position(0,1024);
interpreter_text->cut();
interpreter_text->insert(GS_COMMAND_BASE, 0);
}

Fl_Menu_Item menu_choice_viewer[] = {
 {_("gv (ghostscript)"), 0,  (Fl_Callback*)cb_gv, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {_("xpdf"), 0,  (Fl_Callback*)cb_xpdf1, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {_("Acroread"), 0,  (Fl_Callback*)cb_Acroread, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0}
};

static void cb_View(Fl_Button*, void*) {
  take_opts();
view_doc();
}

Fl_Double_Window* make_window() {
  Fl_Double_Window* w;
  { Fl_Double_Window* o = window = new Fl_Double_Window(385, 455, _("PDF Load Settings"));
    w = o;
    { Fl_Group* o = new Fl_Group(10, 10, 365, 125, _("Options"));
      o->box(FL_ENGRAVED_FRAME);
      o->align(FL_ALIGN_TOP_LEFT|FL_ALIGN_INSIDE);
      { Fl_Choice* o = choice_interpreter = new Fl_Choice(210, 30, 156, 25);
        o->down_box(FL_BORDER_BOX);
        o->menu(menu_choice_interpreter);
        o->value( vals.interpreter );
      }
      { Fl_Multiline_Input* o = interpreter_text = new Fl_Multiline_Input(20, 55, 345, 70, _("Interpreter Command:"));
        o->box(FL_DOWN_BOX);
        o->color(FL_BACKGROUND2_COLOR);
        o->selection_color(FL_SELECTION_COLOR);
        o->labeltype(FL_NORMAL_LABEL);
        o->labelfont(0);
        o->labelsize(14);
        o->labelcolor(FL_FOREGROUND_COLOR);
        o->align(FL_ALIGN_TOP_LEFT);
        o->when(FL_WHEN_CHANGED);
        Fl_Group::current()->resizable(o);
        o->insert( vals.command ,0 );
        o->wrap(1);
      }
      o->end();
      Fl_Group::current()->resizable(o);
    }
    { Fl_Group* o = new Fl_Group(216, 420, 160, 25);
      { Fl_Return_Button* o = new Fl_Return_Button(301, 420, 75, 25, _("OK"));
        o->shortcut(0xff0d);
        o->callback((Fl_Callback*)cb_Ok, (void*)(w));
        w->hotspot(o);
      }
      { Fl_Button* o = new Fl_Button(216, 420, 75, 25, _("Cancel"));
        o->shortcut(0xff1b);
        o->callback((Fl_Callback*)cb_Cancel);
      }
      o->end();
    }
    { Fl_Group* o = new Fl_Group(10, 145, 365, 190, _("Ghostscript Options"));
      o->box(FL_ENGRAVED_FRAME);
      o->align(FL_ALIGN_TOP_LEFT|FL_ALIGN_INSIDE);
      { Fl_Box* o = new Fl_Box(20, 185, 205, 25, _("Intermediate Format:"));
        o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
      }
      { Fl_Choice* o = choice_export_format = new Fl_Choice(210, 185, 155, 25);
        o->down_box(FL_BORDER_BOX);
        o->callback((Fl_Callback*)cb_choice_export_format);
        o->menu(menu_choice_export_format);
        o->value( vals.export_format );
        if(!vals.gs_has_tiff32nc) menu_choice_export_format[0].label(_("TIFF rgb"));
      }
      { Fl_Box* o = new Fl_Box(20, 210, 205, 25, _("Select Colorspace:"));
        o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
      }
      { Fl_Choice* o = choice_colourspace = new Fl_Choice(210, 210, 155, 25);
        o->down_box(FL_BORDER_BOX);
        o->callback((Fl_Callback*)cb_choice_colourspace);
        o->menu(menu_choice_colourspace);
        o->value( vals.colourspace );
        if(vals.export_format>1) o->value(1);
        if(vals.export_format == 0 && !vals.gs_has_tiff32nc) o->value(1);
      }
      { Fl_Value_Slider* o = slider_resolution = new Fl_Value_Slider(105, 245, 260, 25, _("Resolution:"));
        o->type(1);
        o->minimum(36);
        o->maximum(1440);
        o->step(10);
        o->value(72);
        o->align(FL_ALIGN_LEFT);
        if(vals.resolution) o->value(vals.resolution);
      }
      { Fl_Box* o = new Fl_Box(20, 275, 60, 25, _("Soften:"));
        o->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
      }
      { Fl_Choice* o = choice_aa_graphic = new Fl_Choice(210, 275, 155, 25, _("Graphics"));
        o->down_box(FL_BORDER_BOX);
        o->menu(menu_choice_aa_graphic);
        o->value( vals.aa_graphic );
      }
      { Fl_Check_Button* o = check_aa_text = new Fl_Check_Button(210, 300, 145, 25, _("Text"));
        o->down_box(FL_DOWN_BOX);
        o->align(FL_ALIGN_LEFT);
        o->value(vals.aa_text);
      }
      { Fl_Choice* o = choice_aa_gridfit = new Fl_Choice(320, 300, 45, 25, _("GridFitTT"));
        o->down_box(FL_BORDER_BOX);
        o->hide();
        o->deactivate();
        o->menu(menu_choice_aa_gridfit);
        o->value( vals.aa_text );
      }
      o->end();
    }
    { Fl_Group* o = new Fl_Group(11, 345, 365, 65, _("View (external)"));
      o->box(FL_ENGRAVED_FRAME);
      o->align(FL_ALIGN_TOP_LEFT|FL_ALIGN_INSIDE);
      { Fl_Choice* o = choice_viewer = new Fl_Choice(60, 375, 150, 25);
        o->down_box(FL_BORDER_BOX);
        o->menu(menu_choice_viewer);
        o->value( vals.viewer );
      }
      { Fl_Button* o = new Fl_Button(291, 375, 75, 25, _("View"));
        o->tooltip(_("View the document in an external viewer."));
        o->callback((Fl_Callback*)cb_View);
      }
      o->end();
    }
    o->show();
    o->size_range(385, 455);
    o->end();
  }
  return w;
}

void take_opts() {
  const char* text = interpreter_text->value();
  snprintf( vals.command, 1024, text );

  vals.resolution = (int) slider_resolution->value();
  vals.ok = 1;
  vals.interpreter = choice_interpreter->value();
  vals.viewer = choice_viewer->value();
  vals.export_format = choice_export_format->value();
  vals.colourspace = choice_colourspace->value();
  vals.aa_graphic = choice_aa_graphic->value();
  vals.aa_text = check_aa_text->value();
  #ifdef DEBUG
  printf(vals.command);
  printf("\n resolution: %d\n",vals.resolution);
  #endif
}

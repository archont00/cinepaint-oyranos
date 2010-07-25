/*
 * Br2HdrManager.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file Br2HdrManager.hpp
*/
#ifndef Br2HdrManager_hpp
#define Br2HdrManager_hpp


//#include <iostream>
#include "br_types.hpp"            // uint8, uint16
#include "br_Image.hpp"            // BrImage, BrImgVector, ImageHDR
#include "HdrCalctor_RGB_U8.hpp"   // HdrCalctor_RGB_U8
#include "HdrCalctor_RGB_U16_as_U8.hpp"  // HdrCalctor_RGB_U16_as_U8
#include "Distributor.hpp"         // ValueDistributor<>
#include "StatusLineBase.hpp"      // StatusLineBase
#include "ResponseSolverBase.hpp"  // ResponseSolverBase::SolveMode


namespace br {

/**===========================================================================

  @class  Br2HdrManager
  
  The class manages the interplay between input image container and HdrCalctor.
   GUI independent!

  <b>Leistungen:</b>
   - Kontaktierung des input image containers `BrImgVector' mit dem `HdrCalctor'.
   - BrImgVector is type-independent, HdrCalctor's are typed.
   - Kreieren eines Pack aus BrImgVector. Verschiedene Pack-Typen denkbar.
   - Einfuegen eventl. Verschiebungskorrekturen
   - Initialisierung (Erzeugen) des HdrCalctor's passenden Typs.
   - Zentraler Ansprechpartner fuer Auftraege und Anfragen der GUI, Vorhalten 
      aller (numerischen) Parameter. Muss also globales singulaeres Objekt geben.
   - Entgegennahme der Nutzerwuensche, deren Umsetzung + Versenden informierender
      Rundrufe -- das Objekt "strahlt".
   - REGEL: Nur Br2HdrManager soll senden!
  
  Warum numerische Parameter ausser im Calctor auch in Br2HdrManager? 
   Weil Calctoren initialisiert werden muessen und vernichtet werden koennen.
   Nur die Br2HdrManager-Werte ueberdauern.
        
  <b>refpic:</b>
   Wann *automatisch* auf "guenstigen" Wert setzen? Bei jedem init_Calctor()
   oder nur beim Hinzufuegen von Bildern? Z.Z. bei jedem init_Calctor().
  
  Often the weight functions for response computation and HDR-merging will
   be the same. For saving of memory the pointers \a pWeightResp_ and
   \a pWeightMerge_ could be pointed then to the same weight function object.
   For a <b>tabled</b> WeightFunc_U16 of 512 kB this would be even a necessisty,
   but less urgent for WeightFunc_U8 with 2 kB. Currently we handle U16 data with
   HdrCalctor_RGB_U16_as_U8. Here is needed one WeightFunc_U16 only, whereas
   for the response computation a WeightFunc_U8 is used. So maximal either two
   <tt>WeightFunc_U8</tt>'s or one WeightFunc_U8 + one WeightFunc_U16 are needed
   at the same time. To hold the code clearer we create at present always two
   weight function objects, and \a pWeightResp_ and \a pWeightMerge_ never point
   to the same object.
   
  @internal Distribution of the enum type \a Event with ValueDistributor<T>.value(..)
   requires `value(T)' or `value(const T&)'; not works `value(T&)'.

  @internal Das ganze Nachrichten-Schema noch vorlaeufig und in Aenderung begriffen.
   Gelegentlich von Vorteil waeren Bitflags, so dass Ober- und Unterkategorien
   angebbar waeren. Nehmen wir das Problem des Calctor-Veraltens. Ausser durch
   Aenderung der Bildzahl im Vector (IMG_VECTOR_SIZE) kann der Calctor veralten 
   auch durch:
    - De/Aktierung von Bildern
    - Aenderung der aktiven Offset-Werte (sofern use_offsets()==TRUE).
   Beides kann unabhaengig geschehen, fuer beides brauchen wir separate Nachrichten
   (SIZE_ACTIVE und ACTIVE_OFFSETS), denn Widgets koennten nur auf eine Sorte
   reagieren muessen.
   \n
   Was, wenn nun use_offsets()==TRUE und de/aktiviert wird? Die Aktivierungsfuktion
    sendet eine SIZE_ACTIVE-Nachricht, berechnet neue Offsets und sendet eine
    ACTIVE_OFFSETS-Nachricht. Es kann nicht eine gespart werden, da Widgets
    nur auf eine reagieren koennten. Bitflags waeren hier optimal, wir koennten
    SIZE_ACTIVE | ACTIVE_OFFSETS senden. Naja, haben wir nicht. Es bleibt wohl
    nichts, als vorerst gelegentliches Doppelneuzeichnene in Kauf zu nehmen.
    Wo ist das bis jetzt der Fall?
   
  @internal FALLBETRACHTUNG: Im Status use_offsets()==TRUE wird Bild zum Vektor
   hinzugefuegt. IMG_VECTOR_SIZE wird gesendet, dann muessten active Offsets
   aktualisiert werden; fuer das neue Bild ist natuerlich noch kein berechneter
   vorhanden, so dass compute_active_offsets() fehlschlaegt -> alle active offsets=0.
   Vernuenftig? Wohlfeil waere ein "Compute Offsets"-Knopf, der analog dem "Init"-
   Knopf dann gelb aufleuchtet.
  
  @todo Namensgebung ganz schoener Mischmasch. "get_ResponseCurve" oder 
   "getResponseCurve" oder "get_response_curve"? Ferner "ExternResponse" oder
   "ResponseExtern"?

  @todo Klaerungsbeduerftig ist der Status der StatusLine-Sache. Sichtbar derzeit
   ohnehin nicht, ist das Konzept auch unklar. Soll es eine Manager-interne
   Sache sein oder sollen aehnlich ProgressInfo StatusLine-Eintraege von ueberall
   her moeglich sein? Dann waere eine globale Instanz analog TheProgressInfo 
   angezeigt.
   
*============================================================================*/
class Br2HdrManager 
{
public:
    
    /**  @note  Update always "br_eventnames.hpp" too, if you change \c Event! */
    enum Event 
    {  NO_EVENT          = 0,   ///< not set
       IMG_VECTOR_SIZE   = 1,   ///< number of images in imgVec changed
       SIZE_ACTIVE       = 2,   ///< number of activated images in imgVec changed
       CALCTOR_INIT      = 3,   ///< Calctor-init happend
       CALCTOR_REDATED   = 4,   ///< Calctor got up-to-date again without init
       CALCTOR_DELETED   = 5,   ///< Calctor==0
       CCD_UPDATED       = 6,   ///< new response (CCD) curves computed
       CCD_OUTDATED      = 7,   ///< any change happend that could give another
                                ///<   response curves than the existing
       CCD_DELETED       = 8,   ///< Subsumed by CALCTOR_INIT & ~_DELETED?
       HDR_UPDATED       = 9,   ///< HDR image created or updated
       TIMES_CHANGED     = 10,  ///< exposure times changed
       FOLLOWUP_UPDATED  = 11,  ///< Follow-up values updated
       FOLLOWUP_OUTDATED = 12,  ///< Follow-up values outdated
       EXTERN_RESPONSE   = 13,  ///< change in usage of extern response curves
       WEIGHT_CHANGED    = 14,  ///< weight function has been changed
       OFFSET_COMPUTED   = 15,  ///< any offset computed
       ACTIVE_OFFSETS    = 16,  ///< active offsets changed
       EVENT_UNKNOWN     = 17   ///< marks also the end of this set
    };                          
        
    /**  We distinguish and handle three kinds of response curves: 1) the last
       computed curves (via Calctor); 2) external curves loaded from file; 
       3) the curves using next for HDR merging; - and for a testing phase: 
       the curves currently existing in the Calctor (should be always identic
       with the computed ones). */
    enum WhichCurves
    {  USE_NEXT          = 0,
       COMPUTED          = 1,
       EXTERNAL          = 2,
       CALCTOR_S         = 3
    };
        
    enum TimesSetting 
    {  BY_NONE           = 0,   // if times external set or manually edited
       BY_EXIF           = 1,
       BY_STOP           = 2 
    };

 
private:
    //  For functions returning a reference we need a static "null curve"
    static TNT::Array1D <double>  null_response_curve_;

    //  Our input image container
    BrImgVector  imgVec_;
    
    //  The (dynamic created) Hdr-Calculator(s)
    HdrCalctor_RGB_U8*         pCalctor_RGB_U8;  
    HdrCalctor_RGB_U16_as_U8*  pCalctor_RGB_U16_as_U8;  

    //  Base pointer of the current created HdrCalctor
    HdrCalctorBase*  pCalctorBase_;
    
    //  Shape of our weight functions (intended or realized)
    WeightFuncBase::Shape  weight_func_shape_resp_; 
    WeightFuncBase::Shape  weight_func_shape_merge_; 
    
    //  The dynamic created weight function objects for U8 and U16 data
    WeightFunc_U8*   pWeightResp_U8_;
    WeightFunc_U8*   pWeightMerge_U8_;
    WeightFunc_U16*  pWeightMerge_U16_;
    
    //  Is the set of images used in Calctor identic with that currently activated in image container?
    bool  images_uptodate_;
    //  Regime for setting: 
    //   Set it true by: init Calctor
    //   Set it false by: Loading of an image, de/activation of an image

    //  Are the last computed response curves `Xcrv_computed_´ up-to-date?
    bool  response_uptodate_;   
           
    //  Update follow-up values automatically when ever they have got out-of-date?
    bool  auto_update_followup_;
       
    //  Use extern response curves for HDR merging (instead of computing them from image data)
    bool  use_extern_response_;
    
    //  For which reference picture the follow-up values shall be computed?
    int  refpic_;

    //  Numeric parameter: number of selected spot points for response computation
    int  n_selectpoints_;
    
    //  Numeric parameter: smoothing coefficent for response computation
    double  smoothing_;
    
    //  Mode for solving the rectangular `Ax=b' problem: AUTO, USE_QR, USE_SVD
    ResponseSolverBase::SolveMode  solve_mode_;
    
    //  Method for generating the "Z-Matrix" (maybe temporarily)
    int  method_Z_;
    
    //  The "stop value" for automatic time generation
    double  stopvalue_;
    
    //  Mark "bad (unresolved) pixel" in the generated HDR-image by contrast colors?
    bool  mark_bad_pixel_;
    
    //  Generate a bad pixel protocol to file or to stdout while merging?
    bool  protocol_to_file_;
    bool  protocol_to_stdout_;

    
    //  Response curves (X vs. z) for the 3 channels
    TNT::Array1D <double>  Xcrv_use_next_[3];    // using next for merging
    TNT::Array1D <double>  Xcrv_computed_[3];    // last computed curves
    TNT::Array1D <double>  Xcrv_extern_[3];      // loaded from file
    
    //  Which "use_next" response curve is a computed one?
    bool  is_usenext_curve_computed_[3];
    
    //  Filenames of last loaded external response curves
    char*  fname_curve_extern_[3];
    
    //  Distributor(en):
    ValueDistributor <Event>  distrib_event_;
    ValueDistributor <int>    distrib_refpic_;

    //  Abstract base pointer to our StatusLine instance
    StatusLineBase*  statusLineBase_;

    //  For communication between beginAddImages() and endAddImages()
    int  last_size_active_;
    
    //  How the times of a series are to set (BY_NONE, BY_EXIF, BY_STOP)
    TimesSetting  times_setting_;
    
    //  Scale times of a series so that the first (=shortest) time becomes 1.
    bool  normalize_times_;
    
    //  Apply BY_STOP times setting for all or only for actives times?
    bool  for_active_times_;


public:
    //  Ctor & Dtor...
    Br2HdrManager();
    virtual ~Br2HdrManager();
    
    //  Hands out the base pointer of the (current) Calctor...
    const HdrCalctorBase * calctor() const      {return pCalctorBase_;}
    HdrCalctorBase       * calctor()            {return pCalctorBase_;}
    
    ValueDistributor <Event> & distribEvent()   {return distrib_event_;}
    ValueDistributor <int> &   distribRefpic()  {return distrib_refpic_;}

    BrImgVector & imgVector()                   {return imgVec_;}
    
    //==================================================================
    //  General Initialisation && Clearing...
    //==================================================================
          
    //  Init (create) a Calctor
    void init_Calctor();
    
    //  Destroy the existing Calctor
    void clear_Calctor();     
    
    //  Clear the input image container
    void clear_Container();
    
    //  Clear Calctor && container
    void clear_ContAndCalc()            {clear_Calctor(); clear_Container();}
    
    //  Clear the filenames of the last loaded external response curves
    void clear_fname_curve_extern();
    
    //  Clear set of external response curves incl. filenames
    void clear_ResponseExtern();
    
    //==================================================================
    //  Adding of input images...
    //==================================================================
    
    //  Add an image to the input container
    void add_Image (BrImage & img);
    bool check_image_to_add (BrImage & img);
    
    void beginAddImages();
    void endAddImages (bool force=false);
    
    //==================================================================
    //  General status information...
    //==================================================================

    //  Number of images in input container
    int  size() const                   {return imgVec_.size();}
    
    //  Number of activated images in input container
    int  size_active() const            {return imgVec_.size_active();}
    
    //  Are the images used in Calctor up-to-date?
    bool areImagesUptodate() const      {return images_uptodate_;}
    
    //  Are the computed response curves up-to-date?
    bool isComputedResponseUptodate() const {return response_uptodate_;}
                      
    //  The "stop value" for automatic time generation; set via `setTimesByStop()'
    double stopvalue() const            {return stopvalue_;}
    //void   reset_stopvalue(double val) {stopvalue_ = val;}
    
    //=====================================================================
    //  Hand through input container functionality (hides the container)...
    
    DataType data_type() const          {return imgVec_.data_type();}
    
    //==================================================================
    //  Hand through data of the input image with index 'i'...
    //==================================================================
    BrImage &   image      (int i)       {return imgVec_.at(i);}
    const BrImage & image  (int i) const {return imgVec_.at(i);}
    ImageID     imageID    (int i) const;
    const char* name       (int i) const {return imgVec_.at(i).name();}
    bool        isActive   (int i) const {return imgVec_.at(i).active();}
    int         width      (int i) const {return imgVec_.at(i).width();}
    int         height     (int i) const {return imgVec_.at(i).height();}
    double      time       (int i) const {return imgVec_.at(i).exposure.time;}
    double      brightness (int i) const {return imgVec_.at(i).brightness();}
    double      relWkRange (int i) const {return imgVec_.at(i).statistics.r_wk_range;}

    //==================================================================
    //  Setting of data of the input images... (with broadcasting)
    //==================================================================
    
    //  Activate and deactivate the i-th image in input container
    void activate (int i);
    void deactivate (int i);
    
    //  Set exposure time of single input image `i' to value `tm'
    void setTime (int i, double tm);
    
    //  Automatic time series generation by a stop value; sets `stopvalue_'
    void setTimesByStop (double stop);
    
    //  Set series times for the current time setting modes
    void setTimes();
    
    //  Modes for setting of time series
    bool normedTimes() const                    {return normalize_times_;}
    void normedTimes (bool b)                   {normalize_times_ = b;}
    TimesSetting timesSetting() const           {return times_setting_;}
    void timesSetting (TimesSetting ts)         {times_setting_ = ts;}
    bool forActiveTimes() const                 {return for_active_times_;}
    void forActiveTimes (bool b)                {for_active_times_ = b;}
        
    //==================================================================
    //  Setting && getting of numeric Calctor parameters...
    // 
    //  Function name scheme:
    //   T  param() const         - return the current value
    //   void set_param (T val)   - set with broadcasting
    //   void reset_param (T val) - set w/out broadcasting
    //==================================================================
    
    int    nSelectPoints() const         {return n_selectpoints_;}
    void   set_nSelectPoints (int n);
    void   reset_nSelectPoints (int n);    
    
    double smoothing() const             {return smoothing_;}
    void   set_smoothing (double val);
    void   reset_smoothing (double val);

    int    refpic() const                {return refpic_;}
    void   set_refpic (int pic); 
    void   reset_refpic (int pic);
    
    //==========================
    //  Other Calctor parameter
    //==========================
    
    bool   mark_bad_pixel()  const  {return mark_bad_pixel_;}
    void   mark_bad_pixel (bool b)  {mark_bad_pixel_ = b; 
                                     if (calctor()) calctor()->mark_bad_pixel(b);}
    
    bool   protocol_to_file() const  {return protocol_to_file_;}
    void   protocol_to_file (bool b) {protocol_to_file_ = b;
                                      if (calctor()) calctor()->protocol_to_file(b);}
    
    bool   protocol_to_stdout() const  {return protocol_to_stdout_;}
    void   protocol_to_stdout (bool b) {protocol_to_stdout_ = b;
                                        if (calctor()) calctor()->protocol_to_stdout(b);}
    
    ResponseSolverBase::SolveMode solve_mode() const    {return solve_mode_;}
    void   solve_mode (ResponseSolverBase::SolveMode m) {solve_mode_ = m;
                                                         if (calctor()) calctor()->solve_mode(m);}

    int    method_Z() const             {return method_Z_;}
    void   method_Z (int m)             {if (m<1 || m>2) return;
                                         method_Z_ = m;                                                        
                                         if (calctor()) calctor()->method_Z(m);}
                                         
    //==================================================================
    //  Relation between image container and Calctor...
    //==================================================================
    
    //  Returns container index of image with Calctor index `k'
    int getInputIndex (int k) const;
    
    //  Returns Calctor index of input image with container index `k'
    int getCalctorIndex (int k) const;

    //  Returns true, if input image with index `k' is used in Calctor
    bool isUsedInCalctor (int k) const;

    //  Update times in `Calctor' by those of the image container
    void update_CalctorTimes();  
      
    //==================================================================
    //  Follow-up curves...
    //==================================================================

    //  Have we follow-up curves ready in Calctor?
    bool haveFollowUpCurves() const     {return calctor() && calctor()->haveFollowUpCurves();}
    
    //  Shall we update follow-up curves automatically?
    bool isAutoUpdateFollowUp() const   {return auto_update_followup_;} 
    void setAutoUpdateFollowUp (bool b) {auto_update_followup_ = b;}

    //==================================================================
    //  Response curves...
    //==================================================================

    TNT::Array1D<double> & getResponseCurveComputed (int channel)   {return Xcrv_computed_[channel];}
    TNT::Array1D<double> & getResponseCurveExtern (int channel)     {return Xcrv_extern_[channel];}
    TNT::Array1D<double> & getResponseCurveUsenext (int channel)    {return Xcrv_use_next_[channel];}
    TNT::Array1D<double> & getResponseCurve (WhichCurves, int channel);
    
    //  Is the set of use_next|computed|external response curves ready for use?
    bool isUsenextResponseReady() const;
    bool isComputedResponseReady() const;
    bool isExternResponseReady() const;
    bool isResponseReady (WhichCurves) const;
    
    //  Is the set of use_next|computed|external curves empty?
    bool isUsenextResponseEmpty() const;
    bool isComputedResponseEmpty() const;
    bool isExternResponseEmpty() const;
    bool isResponseEmpty (WhichCurves) const;
    
    //  Is any of the "use_next" response curves a computed one?
    bool anyUsenextCurveComputed() const;
    //  Is the "use_next" curve for `channel' a computed one?
    bool isUsenextCurveComputed (int channel)   {return is_usenext_curve_computed_[channel];}
        
    //  Use external response curves only and switch off response computation
    bool isUseExternResponse() const            {return use_extern_response_;}
    void setUseExternResponse (bool b);
    
    bool applyResponseCurveComputed (int channel);
    bool applyResponseCurveExtern (int channel);

    bool write_ResponseCurve (WhichCurves, int channel, const char* fname);
    bool read_ResponseCurve  (int channel, const char* fname);
    
    bool init_ResponseForMerging();
    
    const char* getFilenameCurveExtern (int channel)    {return fname_curve_extern_[channel];}
    
    //==================================================================
    //  Weight function...
    //==================================================================

    void setWeightResp  (WeightFuncBase::Shape);
    void setWeightMerge (WeightFuncBase::Shape);
    void setWeight      (WeightFuncBase::Shape);
    WeightFuncBase::Shape weightShapeResp() const       {return weight_func_shape_resp_;}
    WeightFuncBase::Shape weightShapeMerge() const      {return weight_func_shape_merge_;}
    
    //==================================================================
    //  Computation calls...
    //==================================================================
    
    //  Compute & update follow-up values for the current `refpic_'
    void compute_FollowUpValues();
    void update_FollowUpValues(); 
    void update_FollowUpValues (int channel)    {update_FollowUpValues();}
    void watch_FollowUpValues();
    
    //  Compute response curves (incl. *update* of FollowUpValues)
    void compute_Response();
    
    //  Pure merging of an HDR image
    ImageHDR merge_HDR();
    
    //  Pure merging of an log. HDR image
    ImageHDR merge_LogHDR();
    
    //  Compute response if not existent + merge HDR
    ImageHDR complete_HDR();
    
    //  Compute response if not existent + merge log HDR
    ImageHDR complete_LogHDR();
    
    //  Init Calctor + response + merge
    ImageHDR make_HDR();
    
    //==================================================================
    //  Miscellaneous...
    //==================================================================
    
    //  Set statusline pointer:
    void set_statusline (StatusLineBase* st)    {statusLineBase_ = st;}
      
    //  Simplifying usage of `statusLineBase_' pointer...
    void statusline_out (const char* s)
      { if (statusLineBase_) statusLineBase_-> out(s); }
    
    void statusline_out_default()
      { if (statusLineBase_) statusLineBase_-> out_default(); }
      
    StatusLineBase* statusline()
      { return statusLineBase_; }
      
    void offset (int iA, int iB, Vec2_int offs, bool sendEvent=true);
    bool use_offsets() const                    {return imgVec_.use_offsets();}
    void use_offsets (bool b);
    void refresh_active_intersection();
    void update_active_intersection()
      { if (use_offsets()) refresh_active_intersection(); }
      
    //==================================================================
    //  List some debug information on console...
    //==================================================================
    
    void report_NumericParams() const;
    void report_CalctorParams() const;
    void report_Indices() const;
    void report_Images(ReportWhat w=REPORT_BASE) const  {imgVec_.report(w);}
    void report_StatusResponseCurves() const;
    void report_WeightFunc() const;
    
    
private:
    //  Helper of init_Calctor();
    void create_Calctor_RGB_U8_();
    void create_Calctor_RGB_U16_as_U8_();
    
    //  Helper of create_Calctor_RGB_... and setWeightShape()
    void createWeightResp_U8_(WeightFuncBase::Shape);
    void createWeightMerge_U8_(WeightFuncBase::Shape);
    void createWeightMerge_U16_(WeightFuncBase::Shape);
    
    //  Helper of compute_Response()
    void update_FollowUpValues_aux();
    
    //  Automatic time generation by a stop value; set `stopvalue_'.
    void setAllTimesByStop (double stop);
    void setNormedAllTimesByStop (double stop);
    void setActiveTimesByStop (double stop);
    void setNormedActiveTimesByStop (double stop);
    
    //  Sets exposure times accordingly to EXIF data
    void setAllTimesByEXIF(); 
    void setNormedAllTimesByEXIF(); 
    
    //  Set times for the current times setting modes
    void setAllTimes();
    void setActiveTimes();
    
    //  Helper for time setting functions
    void afterTimeChange();
};


}  // namespace

#endif  // Br2HdrManager_hpp

// END OF FILE

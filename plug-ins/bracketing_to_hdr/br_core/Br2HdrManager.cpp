/*
 * Br2HdrManager.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  Br2HdrManager.cpp 
*/

#include <cassert>
#include "Br2HdrManager.hpp"
#include "br_types.hpp"         // ImageID
#include "ProgressInfo.hpp"     // ProgressInfo
#include "input_statistics.hpp" // compute_input_stats_*()
#include "FilePtr.hpp"
#include "curve_files.hpp"      // read_curve_file()
#include "br_messages.hpp"      // v_message(), message()
#include "br_enums_strings.hpp" // image_type_short_str()
#include "i18n.hpp"             // macros _(), N_()


using namespace br;
using std::cout;


/*  Some multiple used strings. Because they are global strings, they are to
     mark for translation by the N_() macro, whereas the _() macro is used at
     the moment of USAGE. 
*/
const char* str_provide_complete_response__ = 
        N_("Please provide first a complete set of \"using next\" response curves.");

const char* str_computing_followup__ = 
        N_("Computing follow-up values...");

        
//////////////////////////////////////////
///
///   Br2HdrManager  --  Implementation
///
//////////////////////////////////////////

/**+*************************************************************************\n
  Ctor.
******************************************************************************/
Br2HdrManager::Br2HdrManager()
  :
    // ...default Ctor for `imgVec_'
    statusLineBase_             (0)
{   
    pCalctorBase_               = 0;
    pCalctor_RGB_U8             = 0;
    pCalctor_RGB_U16_as_U8      = 0;
    pWeightResp_U8_             = 0;
    pWeightMerge_U8_            = 0;
    pWeightMerge_U16_           = 0;
    weight_func_shape_resp_     = WeightFuncBase::TRIANGLE;
    weight_func_shape_merge_    = WeightFuncBase::TRIANGLE;
    images_uptodate_            = false;          
    response_uptodate_          = false;
    auto_update_followup_       = false;
    use_extern_response_        = false;
    last_size_active_           = 0;
    times_setting_              = BY_NONE;
    normalize_times_            = true;
    for_active_times_           = true;
    
    for (int i=0; i<3; i++) {
      is_usenext_curve_computed_[i] = false;
      fname_curve_extern_[i] = 0;
    }
    
    //  Defaults for numeric parameters:
    refpic_         = 0;
    smoothing_      = 20.0;
    n_selectpoints_ = 50;
    stopvalue_      = 1.0;
    solve_mode_     = ResponseSolverBase::AUTO;
    method_Z_       = 1;        // i.e. selection along the refpic line
}

/**+*************************************************************************\n
  Dtor.
******************************************************************************/
Br2HdrManager::~Br2HdrManager()
{
    clear_ContAndCalc();
    clear_fname_curve_extern();
    
    delete pWeightResp_U8_;
    delete pWeightMerge_U8_;
    delete pWeightMerge_U16_;
}


/**+*************************************************************************\n
  init_Calctor()

  Deallocates the existing Calctor instance (if any) and creates a new one 
   initialized with the data from input image container. We assume all images
   of the image container of the same data_type and image_type. The created Pack
   is of the same data_type, but could have a different storing sheme.

  Secure that we are at the end in a safe state, e.g. bring all relevant
   numerical parameters in consistency with the new Calctor, for instance 
   \a refpic_.
   
  @note Wann Wichtungsobjekte freigeben? Wenn in clear_Calctor(), wo es am
   uebersichtlichsten, muessten bei jeder Calctor-Neuinit. auch die Wichtungsobjekte
   neu angelegt werden (inkl. Taballenberechnung). Da man i.d.R. im Datentyp
   verbleibt, ist das Verschwendung. Zur Zeit werden daher in den create_Calctor()-
   Funktionen die jeweils <b>anderen</b> Wichtungsobjekte geloescht (sofern
   existent), d.h. nur bei einem Typwechsel. Das natuerlich verzwickter.
  
  @internal Wirklich hier nochmal active_offsets und intersection berechnen?
   Naja, es koennte vorher vergessen worden sein.
******************************************************************************/
void Br2HdrManager::init_Calctor()
{
    /*  Destroy old Calctor */
    clear_Calctor();                    // --> CALCTOR_DELETED

    /*  Leave, if creation of a new Calctor is impossible */
    IF_FAIL_DO (size_active() > 1, return);
    
    /*  Compute active offsets and the intersection area */
    refresh_active_intersection();
    //imgVec_.report_offsets();
    IF_FAIL_DO (! imgVec_.intersection().is_empty(), return);
    
    switch (imgVec_.data_type())        // besser image_type()?
    {
    case DATA_U8:
        create_Calctor_RGB_U8_();
        break;
    
    case DATA_U16:
        create_Calctor_RGB_U16_as_U8_();
        break;
    
    default: 
        NOT_IMPL_DATA_TYPE (imgVec_.data_type());
        return;
    }
    
    /*  Set numerical and other parameters in Calctor: */
    calctor()-> solve_mode (solve_mode_);
    calctor()-> method_Z (method_Z_);
    calctor()-> smoothing (smoothing_);
    calctor()-> nselect (n_selectpoints_);
    calctor()-> mark_bad_pixel (mark_bad_pixel_);
    calctor()-> protocol_to_file (protocol_to_file_);
    calctor()-> protocol_to_stdout (protocol_to_stdout_);
    
    /*  Secure consistency of numerical values: */
#if 1    
    /*  Choice a perhaps good refpic (provisionally) */
    reset_refpic (calctor()->size() / 2); 
#else
    /*  Correct the refpic, if old value would be too high */
    if (refpic_ >= calctor()->size())   
      reset_refpic (calctor()->size() - 1);
#endif    

    /*  Reset status variables: */
    images_uptodate_ = true;    // (response_update_ status unchanged)

    /*  Broadcast the "Calctor init" event */
    distrib_event_.distribute (CALCTOR_INIT);
    //distrib_event_.distribute (CCD_OUTDATED); // subsumed by CALCTOR_INIT
    
    /*  Compute follow-up's if auto-mode, else broadcast FOLLOWUP_OUTDATED: */
    watch_FollowUpValues();
}


/**+*************************************************************************\n
  Creates calctor for RGB_U8  -  private helper of init_Calctor().
  @internal Because Calctor_RGB_U8 and Calctor_RGB_U16_as_U8 use the same U8
   weight function for response computation, no such U16 object is to delete.
******************************************************************************/
void Br2HdrManager::create_Calctor_RGB_U8_()
{
    //  Generate the `Pack'
    PackImgScheme2D_RGB_U8  pack_U8 (imgVec_);  // local object!
    
    //  Delete weight_U16 object for merging if any
    if (pWeightMerge_U16_) {delete pWeightMerge_U16_;  pWeightMerge_U16_=0;}
      
    //  Create weight_U8 objects if not yet existent (assert()s if fails)
    if (!pWeightResp_U8_)  createWeightResp_U8_(weight_func_shape_resp_);
    if (!pWeightMerge_U8_) createWeightMerge_U8_(weight_func_shape_merge_);

    //  Create the Calctor for that Pack and weight functions
    pCalctor_RGB_U8 = new HdrCalctor_RGB_U8 (   pack_U8, 
                                                *pWeightResp_U8_, 
                                                *pWeightMerge_U8_); 
    pCalctorBase_ = pCalctor_RGB_U8;
}
/**+*************************************************************************\n
  Creates calctor for RGB_U16_as_U8  -  private helper of init_Calctor().
  @internal Because Calctor_RGB_U8 and Calctor_RGB_U16_as_U8 use the same U8
   weight function for response computation, this object is not to delete.
******************************************************************************/
void Br2HdrManager::create_Calctor_RGB_U16_as_U8_()
{
    //  Generate the `Pack'
    PackImgScheme2D_RGB_U16  pack_U16 (imgVec_);  // local object!
    
    //  Delete weight_U8 object for merging if any
    if (pWeightMerge_U8_) {delete pWeightMerge_U8_;  pWeightMerge_U8_=0;}
      
    //  Create weight_U8 object for response if not yet existent (assert()s if fails)
    if (!pWeightResp_U8_)  createWeightResp_U8_(weight_func_shape_resp_);
    
    //  Create weight_U16 object for merging if not yet existent (assert()s if fails)
    if (!pWeightMerge_U16_)  createWeightMerge_U16_(weight_func_shape_merge_);

    //  Create the Calctor for that Pack and weight functions
    pCalctor_RGB_U16_as_U8 = new HdrCalctor_RGB_U16_as_U8 ( pack_U16,
                                                            *pWeightResp_U8_,
                                                            *pWeightMerge_U16_);
    pCalctorBase_ = pCalctor_RGB_U16_as_U8;
}


/**+*************************************************************************\n
  Clears (delete) the current calctor + send a CALCTOR_DELETED.
   Einfachsterweise wuerden hier auch die zugehoerigen Wichtungsobjekte zerstoert,
   siehe aber die Bemerkung in init_Calctor().
******************************************************************************/
void Br2HdrManager::clear_Calctor()      
{
    if (!pCalctorBase_) return;         // no broadcast then
    
    if (pCalctor_RGB_U8) {
      delete pCalctor_RGB_U8;
      pCalctor_RGB_U8 = 0;
    }
    else {
      delete pCalctor_RGB_U16_as_U8;
      pCalctor_RGB_U16_as_U8 = 0;
    }
    pCalctorBase_ = 0;
    
    distrib_event_.distribute (CALCTOR_DELETED);
    //distrib_event_.distribute (CCD_DELETED);  // subsumed by CALCTOR_DELETED
}

/**+*************************************************************************\n
  Clears the input image container.
******************************************************************************/
void Br2HdrManager::clear_Container()      
{ 
    if (!imgVec_.size()) return;        // no broadcast then
    
    imgVec_.clear(); 
    distrib_event_.distribute (IMG_VECTOR_SIZE);
}

/**+*************************************************************************\n
  Clears the filenames of the last loaded external response curves.
******************************************************************************/
void Br2HdrManager::clear_fname_curve_extern()
{
    for (int i=0; i<3; i++) 
      if (fname_curve_extern_[i]) 
      {
        free (fname_curve_extern_[i]);
        fname_curve_extern_[i] = 0;
      }
}

/**+*************************************************************************\n
  Clears the set of external response curves inclusive their filenames. Also 
   clears all those use_next curves, which were an external one.
******************************************************************************/
void Br2HdrManager::clear_ResponseExtern()
{
    for (int i=0; i<3; i++) {
      Xcrv_extern_[i] = null_response_curve_;
      if (! is_usenext_curve_computed_[i])  // use_next curve was an external
        Xcrv_use_next_[i] = null_response_curve_;
    } 
    clear_fname_curve_extern();
}


///////////////////////////
///
///   Respose Curves
///
///////////////////////////

/**+*************************************************************************\n
  Gets the response curve for \a channel of sort \a which. Currently we have 
   allowed also which==CALCTOR_S, but we can not return a [non-const] reference
   to a temporary object. We return then our static \a null_response_curve_.
******************************************************************************/
TNT::Array1D<double> & 
Br2HdrManager::getResponseCurve (WhichCurves which, int channel)
{
    switch (which) 
    {
      case USE_NEXT:  return Xcrv_use_next_[channel];
      case COMPUTED:  return Xcrv_computed_[channel];
      case EXTERNAL:  return Xcrv_extern_[channel];
      default:        break;              // avoids warning
    }
    return null_response_curve_;        // besser werfen
}

/**+*************************************************************************\n
  Is the set of "use_next" response curves ready for use?
******************************************************************************/
bool Br2HdrManager::isUsenextResponseReady() const
{
    return !Xcrv_use_next_[0].is_empty() && 
           !Xcrv_use_next_[1].is_empty() && 
           !Xcrv_use_next_[2].is_empty();
}
/**+*************************************************************************\n
  Is the set of computed response curves ready for use?
******************************************************************************/
bool Br2HdrManager::isComputedResponseReady() const
{
    return !Xcrv_computed_[0].is_empty() && 
           !Xcrv_computed_[1].is_empty() && 
           !Xcrv_computed_[2].is_empty();
}
/**+*************************************************************************\n
  Is the set of external response curves ready for use?
******************************************************************************/
bool Br2HdrManager::isExternResponseReady() const
{
    return !Xcrv_extern_[0].is_empty() && 
           !Xcrv_extern_[1].is_empty() && 
           !Xcrv_extern_[2].is_empty();
}
/**+*************************************************************************\n
  Is the set of response curves of sort \a which ready for use?
******************************************************************************/
bool Br2HdrManager::isResponseReady (WhichCurves which) const
{
    switch (which) 
    {
      case USE_NEXT:  return isUsenextResponseReady();
      case COMPUTED:  return isComputedResponseReady();
      case EXTERNAL:  return isExternResponseReady();
      default:        return false;               // besser werfen.
    }
}

/**+*************************************************************************\n
  Is the set of "use_next" response curves empty?
******************************************************************************/
bool Br2HdrManager::isUsenextResponseEmpty() const
{
    return Xcrv_use_next_[0].is_empty() && 
           Xcrv_use_next_[1].is_empty() && 
           Xcrv_use_next_[2].is_empty();
}
/**+*************************************************************************\n
  Is the set of computed response curves empty?
******************************************************************************/
bool Br2HdrManager::isComputedResponseEmpty() const
{
    return Xcrv_computed_[0].is_empty() && 
           Xcrv_computed_[1].is_empty() && 
           Xcrv_computed_[2].is_empty();
}
/**+*************************************************************************\n
  Is the set of external response curves empty?
******************************************************************************/
bool Br2HdrManager::isExternResponseEmpty() const
{
    return Xcrv_extern_[0].is_empty() && 
           Xcrv_extern_[1].is_empty() && 
           Xcrv_extern_[2].is_empty();
}
/**+*************************************************************************\n
  Is the set of \a which response curves empty?
******************************************************************************/
bool Br2HdrManager::isResponseEmpty (WhichCurves which) const
{
    switch (which) 
    {
      case USE_NEXT:  return isUsenextResponseEmpty();
      case COMPUTED:  return isComputedResponseEmpty();
      case EXTERNAL:  return isExternResponseEmpty();
      default:        return false;               // besser werfen.
    }
}

/**+*************************************************************************\n
  Is any of the "use_next" response curves a computed one?
******************************************************************************/
bool Br2HdrManager::anyUsenextCurveComputed() const
{
    for (int i=0; i < 3; i++)
      if (is_usenext_curve_computed_[i]) return true;
      
    return false; 
}

/**+*************************************************************************\n
  Applies the computed response curve of channel \a channel for using next.
   Sets \a is_usenext_curve_computed_[channel] true if that curve is not empty,
   otherwise false (an empty curve is not a computed one).
   @returns true, if curve is ready for use (not empty), else false.
******************************************************************************/
bool Br2HdrManager::applyResponseCurveComputed (int channel)   
{
    Xcrv_use_next_[channel] = Xcrv_computed_[channel];
    
    if (Xcrv_use_next_[channel].is_empty()) {
      is_usenext_curve_computed_[channel] = false;
      return false;
    }
    is_usenext_curve_computed_[channel] = true;
    return true;
}
/**+*************************************************************************\n
  Applies the external response curve of channel \a channel for using next.
   Sets \a is_usenext_curve_computed_[channel] false -- also if that curve is 
   empty, because also an empty curve is not a computed one!
   @returns true, if curve is ready for use (not empty), else false.
******************************************************************************/
bool Br2HdrManager::applyResponseCurveExtern (int channel)     
{
    Xcrv_use_next_[channel] = Xcrv_extern_[channel]; 
    is_usenext_curve_computed_[channel] = false; 
    
    if (Xcrv_use_next_[channel].is_empty())
      return false;
    
    return true;
}

/**+*************************************************************************\n
  Installs ("init"?) the intended "use_next" response curves for merging. At
   present, merging is done via the Calctor, so we copy the "next_use" curves
   into Calctor. @returns True, if all curves accepted, false otherwise.
******************************************************************************/
bool Br2HdrManager::init_ResponseForMerging()      
{ 
    if (!calctor()) return false;
    
    bool ok = true;
    for (int i=0; i < 3; i++)
      ok &= calctor()->setExposeVals (i, Xcrv_use_next_[i]);
      
    return ok;
}

/**+*************************************************************************\n
  Sets usage of external response curves to \a b (Y/N), and applies the curves.
******************************************************************************/
void Br2HdrManager::setUseExternResponse (bool b)
{
    use_extern_response_ = b;
    
    if (use_extern_response_) 
      for (int i=0; i < 3; i++) 
        applyResponseCurveExtern (i);
    
    distrib_event_.distribute (EXTERN_RESPONSE);  // re-plots etc.
}

/**+*************************************************************************\n
  Writes a response curve into a text file with one value per line. 
   Auxillary function, throws, if an fopen error happens.
  @warning An existing file of the same name gets overwritten without warning!
  
  @param which: which kind of curves (use_next, computed, external).
  @param channel: which channel.
  @param fname: name of the file.
******************************************************************************/
bool
Br2HdrManager::write_ResponseCurve (WhichCurves which, int channel, const char* fname)
{
    IF_FAIL_RETURN (0 <= channel && channel < 3, false);
    IF_FAIL_RETURN (which < CALCTOR_S, false);
    
    Array1D<double> * crv;
    switch (which) 
    {
      case USE_NEXT:  crv = & Xcrv_use_next_[channel]; break;
      case COMPUTED:  crv = & Xcrv_computed_[channel]; break;
      case EXTERNAL:  crv = & Xcrv_extern_[channel];   break;
      default:        return false;  // CALCTOR_S curves
    }

    FilePtr file(fname, "wt");  // Overwrites without warning!! Can throw!
    
    fprintf (file, "# Response curve, %d values, 8-bit\n", crv->dim());
    
    for (int i=0; i < crv->dim(); i++)
      fprintf (file, "%f\n", (*crv)[i]);
    
    printf("Wrote response curve into file \"%s\"\n", fname);
    return true;
}

/**+*************************************************************************\n
  Reads an external response curve from file \a fname for \a channel. If it 
   fails, old curve remains unchanged. If successfully, the filename is stored,
   and the curve is applied as "use_next" curve for the corresponding channel.
  @returns true, if successfully, false otherwise.
******************************************************************************/
bool
Br2HdrManager::read_ResponseCurve (int channel, const char* fname)
{
    printf("read curve file \"%s\" for channel %d\n", fname, channel);
    if (channel < 0 || channel > 2)  
      return false;
    
    TNT::Array1D<double> crv;
    
    if (read_curve_file (fname, crv) != 0)  
      return false;                 // previous curve remains unchanged
    
    Xcrv_extern_[channel] = crv;    // ref counting copy
    
    /*  store filename */
    if (fname_curve_extern_[channel])  free (fname_curve_extern_[channel]);
    fname_curve_extern_[channel] = strdup (fname);
    
    /*  Apply the loaded curve as "use_next" curve */
    //if (use_extern_response_)  // apply only if external curves are to use
    applyResponseCurveExtern (channel);
    
    return true;
}


///////////////////////////////////////////////
///
///  Adding of input images to the container
///
///////////////////////////////////////////////

/**+*************************************************************************\n
  Checks whether the image \a img (intended to add to the input container) has
   an allowed data type and image type, and whether it matches to the present
   images in the input container. If something is wrong we output a message
   using the "abstract" br::msg_image_ignored() function (pointer). The message
   appears on the GUI screen, if it has been initialized accordingly.
  @return True, if \a img can be added to the input container.
  
  @warning Where we check the meta variable \a image_type() keep in mind, that
   currently this variable does not wrap all possibilities!
******************************************************************************/
bool Br2HdrManager::check_image_to_add (BrImage & img)
{  
    const char* str_other_dim  = _("Image to add has other dimension than the existing image(s):");
    const char* str_other_type = _("Image to add has other type than the existing image(s):");
    const char* str_other_datatype = _("Image to add has other data type than the existing image(s):");
    
    /*  Wir erlauben nur U8 und U16 Daten */    
    if (! (img.data_type() == DATA_U8 || img.data_type() == DATA_U16)) {
      msg_image_ignored (img.name(), 
          _("Images of data type \"%s\" can not be added."), 
          data_type_str(img.data_type())); 
      return false;
    }    
    /*  Falsche image_typen herausfiltern. Diese sind grob falsch: */
    if (img.image_type() <= IMAGE_NONE || img.image_type() >= IMAGE_UNKNOWN) {
      msg_image_ignored (img.name(), 
          _("Images of type \"%s\" can not be added."),
          image_type_short_str(img.image_type()));
      return false;
    }    
    /*  Falls container leer, koennte dieses Bild addiert werden */
    if (!imgVec_.size()) return true;  
    
    /*  Container not empty: Proof consistancy with existing images */
    if (img.data_type() != imgVec_.data_type()) {
      msg_image_ignored (img.name(),
          "%s\n\t\"%s\"  vs.  \"%s\".", 
          str_other_datatype,
          data_type_str(img.data_type()), 
          data_type_str(imgVec_.data_type()));
      return false;
    }
    if (img.image_type() != imgVec_.image_type()) {
      msg_image_ignored (img.name(),
          "%s\n\t\"%s\"  vs.  \"%s\".", 
          str_other_type,
          image_type_short_str(img.image_type()), 
          image_type_short_str(imgVec_.image_type()));
      return false;
    }
    if (img.width() != imgVec_[0].width() ||
        img.height() != imgVec_[0].height()) {
      msg_image_ignored (img.name(),
          "%s\n\t\"%d x %d\"  vs.  \"%d x %d\".", 
          str_other_dim, 
          img.width(), img.height(), imgVec_[0].width(), imgVec_[0].height());
      return false;
    }
    return true;
}

/**+*************************************************************************\n
  Adds the image \a img to the input container.
   Because the added image is set active, calctor gets out-of-date.
******************************************************************************/
void Br2HdrManager::add_Image (BrImage & img)
{  
    //  Has the new image an allowed type and does it fit to those in container?
    if (! check_image_to_add (img))  return;
    
    //  Compute brightness per pixel etc.
    switch (img.image_type())
    {
    case IMAGE_RGB_U8:
        compute_input_stats_RGB_U8 (img); break;
      
    case IMAGE_RGB_U16:
        compute_input_stats_RGB_U16 (img); break;
      
    default:
        NOT_IMPL_IMAGE_TYPE (img.image_type()); 
        return;
    }        
    
    //  Add image to container (ordered by brightness)
    imgVec_.add (img);

    //  Set status variables
    images_uptodate_ = response_uptodate_ = false;
    
    //  Broadcast "image vector size changed"
    distrib_event_.distribute (IMG_VECTOR_SIZE);
    
    //  Create new Calctor if possible? No! Only at the end of a series!
    //if (size_active() > 1) init_Calctor();  // --> CALCTOR_INIT
}


/**+*************************************************************************\n
  Helper for routines, which add one one or more images to the container. They
   can call beginAddImages() in front of all and endAddImages() at the end.
   That way the updating stuff can be done effectively for a series at once. A 
   begin..() without closing end..() or a solitude end..() are (and must be)
   allowed!
     
  beginAddImages() updates some class-global variables figured out later by
   endAddImages().
******************************************************************************/
void Br2HdrManager::beginAddImages()
{
    last_size_active_ = size_active();
}

/**+*************************************************************************\n
  Helper for routines, which add one one or more images to the container. They
   can call beginAddImages() in front of all and endAddImages() at the end.
   That way the updating stuff can be done effectively for a series at once. A 
   begin..() without closing end..() or a solitude end..() are (and must be) 
   allowed!
   
  endAdddImages() sets the times accord. to the current times setting mode, and
   -- if possibly and size_active() has changed since last beginAddImages() --
   it creates a new calctor for the new active image series. If \a force == TRUE,
   a new calctor is created always (default: FALSE).
******************************************************************************/
void Br2HdrManager::endAddImages (bool force)
{
    setTimes();
    
    if (force || (size_active() > 1) && (size_active() != last_size_active_))
      init_Calctor();                   // --> CALCTOR_INIT
}

/**+*************************************************************************\n
  Activates the input image with index \a pic and sends an SIZE_ACTIVE message.
   If use_offsets()==TRUE, active offsets and intersection area are re-computed
   and an ACTIVE_OFFSET is broadcasted.  @internal The up-to-date valuation is
   open for more intelligence, e.g. a check could be done, whether the activated
   images coincide again with those in Calctor
******************************************************************************/
void Br2HdrManager::activate (int pic)
{
    imgVec_.activate (pic);
    images_uptodate_ = response_uptodate_ = false; 
    distrib_event_.distribute (SIZE_ACTIVE);
    
    if (imgVec_.use_offsets()) 
      refresh_active_intersection();
}
/**+*************************************************************************\n
  Deactivates the input image with index \a pic and sends an SIZE_ACTIVE message.
   If use_offsets()==TRUE, active offsets and intersection area are re-computed
   and an ACTIVE_OFFSET is broadcasted.
******************************************************************************/
void Br2HdrManager::deactivate (int pic)
{
    imgVec_.deactivate (pic);
    images_uptodate_ = response_uptodate_ = false; 
    distrib_event_.distribute (SIZE_ACTIVE);
    
    if (imgVec_.use_offsets()) 
      refresh_active_intersection();
}

/**+*************************************************************************\n
  Sets the exposure time of input image \a i to value \a tm and calls 
   afterTimeChange(). Used for single editing.
******************************************************************************/
void Br2HdrManager::setTime (int i, double tm)    
{
    if (tm != time(i)) {
      imgVec_.image(i).exposure.time = tm;
      afterTimeChange();
    }
}
    
//=================================================
//  The public routines for setting of time series
//=================================================

/**+*************************************************************************\n 
  Sets the times of a series accordingly to current times settings modes. There
   are three independend parameters to set via timesSetting(), forActiveTimes()
   and normedTimes():
   - \a times_settings_ (BY_NONE, BY_EXIF, BY_STOP): times are taken respectively
     either as given (except a normalisation can be done), as figured (to figure)
     out from EXIF data or as generated (to generate) by a stop value (using the
     current \a stopvalue_).
   - \a for_active_times_: For generating of a time series either only the active
    image are concernd or all (for BY_EXIF allways all times are set).
   - \a normalize_times_: either normalized times are generated or not. 
******************************************************************************/
void Br2HdrManager::setTimes()
{
     if (for_active_times_) setActiveTimes();
     else                   setAllTimes();
}

/**+*************************************************************************\n 
  Wrapper for the "ByStop" routines. Generates times for the given stopvalue
   argument \a stop. Depending on the current \a for_active_times_ either only
   the active times are concerned or all, and depending on the current 
   \a normalize_times_ either normalized times are generated or not.
******************************************************************************/
void Br2HdrManager::setTimesByStop (double stop)
{
    if (normalize_times_)
      if (for_active_times_)
           setNormedActiveTimesByStop (stop);
      else setNormedAllTimesByStop (stop);
    else  
      if (for_active_times_)
           setActiveTimesByStop (stop);
      else setAllTimesByStop (stop);
} 

//========================================
//  and their (meanwhile) private helpers
//========================================

/**+*************************************************************************\n
  Automatic times generation by a stop value \a stop.
******************************************************************************/
void Br2HdrManager::setAllTimesByStop (double stop) 
{  
    stopvalue_ = stop;
    imgVec_.set_times_by_stop (stop);
    afterTimeChange();
}
void Br2HdrManager::setNormedAllTimesByStop (double stop) 
{  
    stopvalue_ = stop;
    imgVec_.set_normed_times_by_stop (stop);
    afterTimeChange();
}
void Br2HdrManager::setActiveTimesByStop (double stop) 
{  
    stopvalue_ = stop;
    imgVec_.set_active_times_by_stop (stop);
    afterTimeChange();
}
void Br2HdrManager::setNormedActiveTimesByStop (double stop) 
{  
    stopvalue_ = stop;
    imgVec_.set_normed_active_times_by_stop (stop);
    afterTimeChange();
}

/**+*************************************************************************\n 
  Sets exposure times accordingly to EXIF data. For a shutter value <= 0 ('not
   set') the resulting time is 0, whereas a missing ISO or aperture value
   does not zero.
******************************************************************************/
void Br2HdrManager::setAllTimesByEXIF()
{  
    imgVec_.set_times_by_EXIF();
    afterTimeChange();
}
void Br2HdrManager::setNormedAllTimesByEXIF()
{  
    imgVec_.set_normed_times_by_EXIF();
    afterTimeChange();
}

/**+*************************************************************************\n 
  Sets times of all images accordingly to the current times settings modes
   and - if "BY_STOP" - with the current \a stopvalue_.
******************************************************************************/
void Br2HdrManager::setAllTimes()
{
    if (normalize_times_)
      switch (times_setting_) {
        case BY_EXIF: setNormedAllTimesByEXIF(); break;
        case BY_STOP: setNormedAllTimesByStop (stopvalue_); break;
        case BY_NONE: imgVec_.normalize_times(); afterTimeChange(); break;
      }
    else
      switch (times_setting_) {
        case BY_EXIF: setAllTimesByEXIF(); break;
        case BY_STOP: setAllTimesByStop (stopvalue_); break;
        case BY_NONE: break;
      }
}

/**+*************************************************************************\n 
  Sets times of all *activated* images accordingly to the current times setting
   modes and - if "BY_STOP" - with the current \a stopvalue_. For the mode 
   "BY_EXIF" however, up to now no "for actives" routine has been implemented
   (not needed), for "BY_EXIF" allways all times are set.
******************************************************************************/
void Br2HdrManager::setActiveTimes()
{
    if (normalize_times_)
      switch (times_setting_) {
        case BY_EXIF: setNormedAllTimesByEXIF(); break;  // all times!
        case BY_STOP: setNormedActiveTimesByStop (stopvalue_); break;
        case BY_NONE: imgVec_.normalize_active_times(); afterTimeChange(); break;
      }
    else
      switch (times_setting_) {
        case BY_EXIF: setAllTimesByEXIF(); break;  // all times!
        case BY_STOP: setActiveTimesByStop (stopvalue_); break;
        case BY_NONE: break;  // do nothing
      }
}

/**+*************************************************************************\n 
  Privat helper of time setting routines. Synchronizes calctor times, updates
   status variables and sends messages.
******************************************************************************/
void Br2HdrManager::afterTimeChange() 
{
#if 0
    printf("afterTimeChange(): ");
    switch (timesSetting()) {
      case BY_EXIF: printf("BY_EXIF"); break;
      case BY_STOP: printf("BY_STOP"); break;
      case BY_NONE: printf("BY_NONE"); break;
    }
    if (normedTimes()) printf("  NORMED\n"); else printf("\n");
#endif    
    update_CalctorTimes();
    response_uptodate_ = false;
    distrib_event_.distribute (TIMES_CHANGED);
    if (!use_extern_response_) distrib_event_.distribute (CCD_OUTDATED);
}


////////////////////////////////////////////////////
///
///  Relation between image container and Calctor
///
////////////////////////////////////////////////////

/**+*************************************************************************\n
  @return The image ID of the input image with index \a i or 0 if \a i is out
   of range. Hides the concrete input container as well as the data type.
******************************************************************************/
ImageID Br2HdrManager::imageID (int i) const
{
    return (0<=i && i < size()) ? imgVec_[i].imageID() : 0; 
}

/**+*************************************************************************\n
  @param k: Index (position) of an image used in Calctor.
  @return  Index (position) of that image in the input image container if there, 
   or -1 if "not found" OR "k out of range" OR "no Calctor exists".
******************************************************************************/
int Br2HdrManager :: getInputIndex (int k) const
{
    if (!calctor() || k<0 || k >= calctor()->size())  return -1;

    return imgVec_.pos_of_ID (calctor()->imageID(k));    
}

/**+*************************************************************************\n
  @param k: Index (position) of an input image in image container.
  @return  Index (position) of that image in calctor if used there, or -1 if 
   "not used" OR "k out of range" OR "no Calctor exists".
******************************************************************************/
int Br2HdrManager :: getCalctorIndex (int k) const
{
    if (!calctor() || k<0 || k >= size())  return -1;
    
    return calctor()->pos_of_ID (imgVec_[k].imageID());
}

/**+*************************************************************************\n
  @param k: Index (position) of an image in the input image container.
  @return  True, if that image is used in Calctor, false otherwise.
******************************************************************************/
bool Br2HdrManager :: isUsedInCalctor (int k) const
{
    return (getCalctorIndex(k) >= 0);
}

//////////////////////////////////////////////////////////////////////////
///  
///  reset_param(T val)  -  Setting here and - if existent - in Calctor.
///
//////////////////////////////////////////////////////////////////////////
void Br2HdrManager::reset_nSelectPoints (int n)
{   
    n_selectpoints_ = n; 
    if (calctor()) calctor()->nselect(n);
}
void Br2HdrManager::reset_smoothing (double val)
{
    smoothing_ = val; 
    if (calctor()) calctor()->smoothing(val);
}
void Br2HdrManager::reset_refpic (int pic)
{ 
    refpic_ = pic; 
    if (calctor()) calctor()->refpic(pic);
}

////////////////////////////////////////////////
///
///  param(T val)  -  reset() + broadcasting
///
////////////////////////////////////////////////

/**+*************************************************************************\n
  Sets number of selected points for response computation to \a n  + CCD_OUTDATED.
  
  @note Bei der Z-Matrix-Generierung wird nselect auf das Moegliche reduziert.
   Weil Randpunkte z=0 und z=zmax als Muellsammler in jedem Fall ausgespart, ist
   praktisch n==254 der maximale Wert, aber wir begnuegen uns hier mit der formalen
   Forderung n<=256. Zumal auch die GUI Werte bis 256 erlaubt und wenn dieser 
   Wert hier abgewiesen wuerde, gaeb's dort nur Verwirrung.
******************************************************************************/
void Br2HdrManager::set_nSelectPoints (int n) 
{
    if (n != n_selectpoints_  &&  n>1 && n<=256) 
    {
      reset_nSelectPoints (n);
      response_uptodate_ = false;
      if (!use_extern_response_) distrib_event_.distribute (CCD_OUTDATED);
    }
}
/**+*************************************************************************\n
  Sets smoothing coefficient for response computation to \a val  + CCD_OUTDATED.
******************************************************************************/
void Br2HdrManager::set_smoothing (double val) 
{
    if (val != smoothing_  &&  val > 0.0) 
    {
      reset_smoothing (val);
      response_uptodate_ = false;
      if (!use_extern_response_) distrib_event_.distribute (CCD_OUTDATED);
    }
}
/**+*************************************************************************\n
  Sets ref. image for follow-up curves to \a pic (input container index) + 
   CCD_OUTDATED
******************************************************************************/
void Br2HdrManager::set_refpic (int pic)
{
    if (pic != refpic_  &&  pic>=0 && pic<size()) 
    {
      reset_refpic (pic);
      distrib_refpic_.distribute (pic);   // for the "Views"
      //  follow-up: autom. and not by watch_() bec. `refpic´ a pure follow-up param.
      update_FollowUpValues();
      response_uptodate_ = false;
      if (!use_extern_response_) distrib_event_.distribute (CCD_OUTDATED);
    }
}

/**+*************************************************************************\n
  Updates times in Calctor by those of the input container. 
   Private helper, no CCD_OUTDATED or TIMES_CHANGED broadcast.                                   
******************************************************************************/
void Br2HdrManager::update_CalctorTimes() 
{  
    if (!calctor()) return;
    
    for (int i=0; i < size(); i++) 
    {
       int KK = getCalctorIndex(i);
       if (KK >= 0) 
         calctor()->setExposeTime(KK, imgVec_[i].time());
    }
}


/////////////////////////////
///
///   Weight function...
///
/////////////////////////////

/**+*************************************************************************\n
  setWeightResp()  -  Sets the weight function for response computation to one
                       of the shape \a shape.
  
  Falls Calctor existiert, wird Weight-Objekt passenden Types kreiert und
   diesem zugewiesen, andernfalls nur \a weight_shape_ Variable (fuer spaetere 
   Erzeugung) gesetzt.
******************************************************************************/
void Br2HdrManager::setWeightResp (WeightFuncBase::Shape shape)
{
    cout <<__func__<< "( shape=" << shape << " )...\n";
    if (pCalctor_RGB_U8) 
    {
      createWeightResp_U8_(shape);              // assert()s if fails
      pCalctor_RGB_U8->setWeightPtrResp (pWeightResp_U8_);
    }
    else if (pCalctor_RGB_U16_as_U8) 
    {
      createWeightResp_U8_(shape);              // assert()s if fails
      pCalctor_RGB_U16_as_U8->setWeightPtrResp (pWeightResp_U8_);
    }
    else
      weight_func_shape_resp_ = shape;

    response_uptodate_ = false;
    distrib_event_.distribute (WEIGHT_CHANGED);
}

/**+*************************************************************************\n
  setWeightMerge()  -   Sets the weight function for HDR-merging to \a shape.
   
  Falls Calctor existiert, wird Weight-Objekt passenden Types kreiert und
   diesem zugewiesen, andernfalls nur \a weight_shape Variable (fuer spaetere 
   Erzeugung) gesetzt.
******************************************************************************/
void Br2HdrManager::setWeightMerge (WeightFuncBase::Shape shape)
{
    cout <<__func__<< "( shape=" << shape << " )...\n";
    if (pCalctor_RGB_U8) 
    {
      createWeightMerge_U8_(shape);              // assert()s if fails
      pCalctor_RGB_U8->setWeightPtrMerge (pWeightMerge_U8_);
    }
    else if (pCalctor_RGB_U16_as_U8) 
    {
      createWeightMerge_U16_(shape);             // assert()s if fails
      pCalctor_RGB_U16_as_U8->setWeightPtrMerge (pWeightMerge_U16_);
    }
    else
      weight_func_shape_merge_ = shape;

    distrib_event_.distribute (WEIGHT_CHANGED);
}
/**+*************************************************************************\n
  setWeight()  -  Sets the (same) weight function both for response computation
                   and HDR-merging to one of shape \a shape.
******************************************************************************/
void Br2HdrManager::setWeight (WeightFuncBase::Shape shape)
{
    setWeightResp (shape);
    setWeightMerge (shape);
}

/**+*************************************************************************\n
  createWeightResp_U8_()  -  private.
   
  Creates WeightFunc of shape \a shape for U8 response computation and sets
   \a weight_func_shape_resp_. Secure, that we leave a valid \a pWeightResp_U8_
   (not NULL), if needed, fall back to TRIANGLE, otherwise assert().
  
  FRAGE: Altes Weight-Objekt lassen, wenn Erzeugung fehlschlaegt?
  FRAGE: Zeiger bzw. bool zurueckliefern, um Erfolg/Misserfolg anzuzeigen?
******************************************************************************/
void Br2HdrManager::createWeightResp_U8_(WeightFuncBase::Shape shape)
{
    cout <<__func__<< "( shape=" << shape << " )...\n";
    
    delete pWeightResp_U8_;
    
    if ((pWeightResp_U8_ = new_WeightFunc_U8 (shape)))
    {
      pWeightResp_U8_->init_table();
      weight_func_shape_resp_ = shape;
      cout << "New weight function: '" << pWeightResp_U8_->what() << "'\n";
    }
    else {
      cout << "Weight function creation failed: Fall back on:\n";
      createWeightResp_U8_(WeightFuncBase::TRIANGLE);  // recursive
      assert (pWeightResp_U8_);
    }
}
/**+*************************************************************************\n
  createWeightMerge_U8_()  -  private.
   
  Creates WeightFunc of shape \a shape´ for U8 HDR-merging and sets 
   \a weight_func_shape_merge_. Secure, that we leave a valid \a pWeightMerge_U8_
   (not NULL), if needed, fall back to TRIANGLE, otherwise assert().
******************************************************************************/
void Br2HdrManager::createWeightMerge_U8_(WeightFuncBase::Shape shape)
{
    cout <<__func__<< "( shape=" << shape << " )...\n";
    
    delete pWeightMerge_U8_;
    
    if ((pWeightMerge_U8_ = new_WeightFunc_U8 (shape)))
    {
      pWeightMerge_U8_->init_table();
      weight_func_shape_merge_ = shape;
      cout << "New weight function: '" << pWeightMerge_U8_->what() << "'\n";
    }
    else {
      cout << "Weight function creation failed: Fall back on:\n";
      createWeightMerge_U8_(WeightFuncBase::TRIANGLE);  // recursive
      assert (pWeightMerge_U8_);
    }
}
/**+*************************************************************************\n
  createWeightMerge_U16_()  -  private.
   
  Creates WeightFunc of shape \a shape for U16 HDR-merging and set
   \a weight_func_shape_merge_. Secure to leave a valid \a pWeightMerge_U16_
   (not NULL), if needed fall back to TRIANGLE, otherwise assert().
 
  @todo Clear decision needed: Init the table per default or not? Currently we
    do init. 512 kB!
******************************************************************************/
void Br2HdrManager::createWeightMerge_U16_(WeightFuncBase::Shape shape)
{
    cout <<__func__<< "( shape=" << shape << " )...\n";
    
    delete pWeightMerge_U16_;
    
    if ((pWeightMerge_U16_ = new_WeightFunc_U16 (shape)))
    {
      pWeightMerge_U16_->init_table();          // TODO Realy?  512 kByte!!??
      weight_func_shape_merge_ = shape;
      cout << "New weight function: '" << pWeightMerge_U16_->what() << "'\n";
    }
    else {
      cout << "Weight function creation failed: Fall back on:\n";
      createWeightMerge_U16_(WeightFuncBase::TRIANGLE);  // recursive
      assert (pWeightMerge_U16_);
    }
}

//////////////////////////////
///
///   Computation calls
///
//////////////////////////////

/**+*************************************************************************\n
  Computes follow-up values for the current \a refpic_ wrapped by (virtual) 
   GUI stuff.
******************************************************************************/
void Br2HdrManager::compute_FollowUpValues()
{
    if (!calctor()) return;
    
    statusline_out (_(str_computing_followup__));
      
    calctor()-> refpic (refpic_);     // tat eigentl. schon reset_refpic()
    calctor()-> compute_FollowUpValues();
    distrib_event_.distribute (FOLLOWUP_UPDATED);
      
    statusline_out_default();
}

/**+*************************************************************************\n
  Updates follow-up values, i.e. computes only if they are out of date.
******************************************************************************/
void Br2HdrManager::update_FollowUpValues()
{
    if (!calctor()) return;
    
    printf("Br2HdrManager::%s(): refpic=%d, last_refpic=%d\n", __func__,
      refpic_, calctor()->fllwUpVals().refpic());
    
    if (refpic_ != calctor()->fllwUpVals().refpic()) 
    {  
      compute_FollowUpValues();   // --> FOLLOWUP_UPDATED
    }
}

/**+*************************************************************************\n
  Updates follow-up vals if auto-mode, otherwise broadcasts a FOLLOWUP_OUTDATED
   message. User should ordinary call this function.
******************************************************************************/
void Br2HdrManager::watch_FollowUpValues()
{
    if (!calctor()) return;
    
    if (refpic_ != calctor()->fllwUpVals().refpic())
    {  
      if (auto_update_followup_)
        compute_FollowUpValues();   // --> FOLLOWUP_UPDATED
      else 
        distrib_event_.distribute (FOLLOWUP_OUTDATED);
    }
}
 
/**+*************************************************************************\n
  Private helper of compute_Response() without status-text wrapping (because
   already done by compute_Response()).
******************************************************************************/
void Br2HdrManager::update_FollowUpValues_aux()
{
    printf("Br2HdrManager::%s(): refpic=%d, last_refpic=%d\n", __func__,
      refpic_, calctor()->fllwUpVals().refpic());
    
    if (refpic_ != calctor()->fllwUpVals().refpic()) 
    {  
      calctor()-> refpic (refpic_);     // tat eigentl. schon reset_refpic()
      calctor()-> compute_FollowUpValues();
      distrib_event_.distribute (FOLLOWUP_UPDATED);
    }
}

/**+*************************************************************************\n
  compute_Response() 
******************************************************************************/
void Br2HdrManager::compute_Response()
{
    IF_FAIL_DO (calctor(), return);
    
    statusline_out (_("Computing response functions..."));
    
    update_FollowUpValues_aux();
    calctor()-> compute_Response (method_Z_);
    for (int i=0; i<3; i++) 
    {
      Xcrv_computed_[i] = Xcrv_use_next_[i] = calctor()->getExposeVals(i);
      is_usenext_curve_computed_[i] = true;
    }
    response_uptodate_ = true;
    distrib_event_.distribute (CCD_UPDATED); 
    
    statusline_out_default();
}

/**+*************************************************************************\n
  merge_HDR() 

  Pure merging of HDR image using existing "use_next" response curves. 
   Realizes the abstraction "Calctor + broadcast".

  @returns Null-Array if no Calctor exists.
******************************************************************************/
ImageHDR
Br2HdrManager::merge_HDR()
{
    IF_FAIL_RETURN (calctor(), ImageHDR());
    IF_FAIL_RETURN (init_ResponseForMerging(), ImageHDR());
        
    ImageHDR img = calctor()->merge_HDR();
    distrib_event_.distribute (HDR_UPDATED);
    
    return img;
}

/**+*************************************************************************\n
  merge_LogHDR() 

  Pure merging of log HDR image using existing "use_next" response curves. 
   Realizes the abstraction "Calctor + broadcast".

  @returns Null-Array if no Calctor exists.
******************************************************************************/
ImageHDR
Br2HdrManager::merge_LogHDR()
{
    IF_FAIL_RETURN (calctor(), ImageHDR());
    IF_FAIL_RETURN (init_ResponseForMerging(), ImageHDR());
    
    ImageHDR img = calctor()->merge_LogHDR();
    distrib_event_.distribute (HDR_UPDATED);
    
    return img;
}

/**+*************************************************************************\n
  complete_HDR()  --  "to complete": compute response if none + merge HDR. 
   If use_next curves are ready, we use them; else: if !use_extern, we compute
   them before, else we give a warning and return a Null-Array.
  @returns Null-Array if something wrong (!calctor OR incomplete response).
******************************************************************************/
ImageHDR
Br2HdrManager::complete_HDR()
{
    IF_FAIL_RETURN (calctor(), ImageHDR());
    
    if (! isUsenextResponseReady())
      if (use_extern_response_) {
        br::message (_(str_provide_complete_response__));
        return ImageHDR();
      }
      else
        compute_Response();     // --> CCD_UPDATED
    
    return merge_HDR();         // --> HDR_UPDATED
}
 
/**+*************************************************************************\n
  complete_LogHDR()  --  analog complete_HDR() for a logarithmic HDR.
  
  @returns Null-Array if something wrong (!calctor OR incomplete response)
******************************************************************************/
ImageHDR
Br2HdrManager::complete_LogHDR()
{
    IF_FAIL_RETURN (calctor(), ImageHDR());
    
    if (! isUsenextResponseReady())
      if (use_extern_response_) {
        br::message (_(str_provide_complete_response__));
        return ImageHDR();
      }
      else
        compute_Response();     // --> CCD_UPDATED
    
    return merge_LogHDR();      // --> HDR_UPDATED
}

/**+*************************************************************************\n
  make_HDR()  --  init() + Response() + HDR()                       UNUSED
******************************************************************************/
ImageHDR 
Br2HdrManager::make_HDR()
{
    init_Calctor();             // --> CALCTOR_INIT
    compute_Response();         // --> FOLLWUP_UPDATED, CCD_UPDATED
    return merge_HDR();         // --> HDR_UPDATED
}


////////////////////////////////////
///
///   Miscellaneous
///
////////////////////////////////////

/**+*************************************************************************\n
  Es wird ein Versatz \a offs von Bild mit Containerindex \a iB gegen Bild \a iA
   mitgeteilt (im Sinne: A[i] <==> B[i+offs]). Gemaess unserer Konvention, einen
   Versatz stets im Bild mit dem hoeheren Index abzulegen, wird \a offs fuer iB>iA
   bei iB eingetragen, fuer iA>iB aber \a -offs bei iA. Zum Schluss wird eine
   OFFSET_COMPUTED Nachricht verschickt, falls \a sendEvent==TRUE (default: TRUE).
   Auszuschalten sinnvoll, wenn mehrere Offsets unmittelbar hintereinander mitgeteilt
   werden, vermeidet dann unnoetige GUI-Aktualisierungen.
******************************************************************************/
void Br2HdrManager::offset (int iA, int iB, Vec2_int offs, bool sendEvent)
{
    if (iA < 0 || iA >= size() || iB < 0 || iB >= size() || iA==iB)
      return;
    
    if (iB > iA) {
      imgVec_[iB].offset (offs); 
      imgVec_[iB].offset_ID (imgVec_[iA].imageID());
    }else {
      imgVec_[iA].offset (-offs); 
      imgVec_[iA].offset_ID (imgVec_[iB].imageID());
    }
    if (sendEvent)
      distrib_event_.distribute (OFFSET_COMPUTED);
}

/**+*************************************************************************\n
  Dargereicht, damit GUI den use_offsets-Status des Managers aendern kann. Wenn
   \a b mit bisherigem use_offsets()-Wert identisch, machen wir nichts. Wenn
   verschieden, rufen wir refresh_active_intersection() auf -- berechnet active
   offsets und intersection_area neu und verschickt ACTIVE_OFFSETS Nachricht.
******************************************************************************/
void Br2HdrManager::use_offsets (bool b) 
{
    if (b == imgVec_.use_offsets())
      return;
    
    imgVec_.use_offsets (b);    // sets the imgVec_ flag
    
    refresh_active_intersection();
}

/**+*************************************************************************\n
  Computes active offsets and the intersection area independently whether
   use_offsets() is TRUE or FALSE. Denn es ist noetig, die active_offsets auch fuer
   use_offsets()==FALSE berechnen (auf 0 setzen) zu koennen, denn die active_offsets
   und intersection-Werte werden IMMER benutzt. Finally an ACTIVE_OFFSETS (have
   been changed) message is broadcasted. -- Name "refresh_active_intersection()"
   ist Zwitter aus BrImgVector's compute_active_offsets() und calc_intersection(),
   denn hier wird beides gemacht.
  
  IDEE: Wenn active_offsets-Werte unveraendert bleiben, koennte die Nachricht
   vermieden werden. Der Init-Button leuchtete dann nicht unnoetig auf. Dies kann
   effektiv aber nur in BrImgVector::compute_active_offsets() festgestellt 
   werden. Muesste dort Statusvariable "active_offsets_changed" geben.
******************************************************************************/
void Br2HdrManager::refresh_active_intersection()
{
    imgVec_.compute_active_offsets();
    imgVec_.calc_intersection();
    distrib_event_.distribute (ACTIVE_OFFSETS);
}


///////////////////////////////////////////////
///
///   List some debug information on console
///
///////////////////////////////////////////////

/**+*************************************************************************\n
  List numerical parameters.
******************************************************************************/
void Br2HdrManager::report_NumericParams() const 
{ 
    cout << "  [Manager: Numeric Parameters...]"
         << "\n    spot points = " << n_selectpoints_
         << "\n    smoothing   = " << smoothing_
         << "\n    refpic      = " << refpic_
         << "\n    solve_mode  = " << solve_mode_
         << "\n    method_Z    = " << method_Z_
         << '\n';
}

/**+*************************************************************************\n
  List Calctor's numerical parameters.
******************************************************************************/
void Br2HdrManager::report_CalctorParams() const 
{ 
    cout << "  [Manager: Calctor's Numeric Parameters...]";
    if (!calctor()) {
      cout << "\n    (no calctor)\n";  return;
    }
    cout << "\n    spot points = " << calctor()->nselect()
         << "\n    smoothing   = " << calctor()->smoothing()
         << "\n    refpic      = " << calctor()->refpic()
         << "\n    solve_mode  = " << calctor()->solve_mode()
         << "\n    method_Z    = " << calctor()->method_Z()
         << '\n';
}

/**+*************************************************************************\n
  List image indices within input container and Calctor and their ID's.
******************************************************************************/
void Br2HdrManager::report_Indices() const
{
    cout << "  [Manager: Indices...]\n";
    if (calctor())
      for (int i=0; i < size(); i++) 
      {
        int KK = getCalctorIndex(i);
        cout << "    input( " << i << " ) <--> Calctor( " << KK 
             << " ) [ID's: " << imageID(i) << " <=> " 
             << calctor()->imageID(KK) << "]\n";
      }
    else 
      for (int i=0; i < size(); i++) 
      {
        cout << "    input( " << i << " )  [ID=" 
             << imageID(i) << "]   (no Calctor)\n";
      }
}

/**+*************************************************************************\n
  Report, which curves are empty ("0") or not ("1"). Devel-helper.
******************************************************************************/
void Br2HdrManager::report_StatusResponseCurves() const
{
    char channel[3] = {'R','G','B'};
    char present[2] = {'1','-'};
    printf("[Manager: Status of Response Curves]\n");
    printf("      computed  external  using next\n");
    for (int i=0; i < 3; i++) 
    {
      printf("   %c:    %c         %c        %c", channel[i],   
          present [Xcrv_computed_[i].is_empty()],
          present [Xcrv_extern_[i].is_empty()],
          present [Xcrv_use_next_[i].is_empty()] );
      if (Xcrv_use_next_[i].is_empty()) 
        printf("\n");
      else
        printf("  (%c)\n", is_usenext_curve_computed_[i] ? 'c' : 'e');
    }    
}

/**+*************************************************************************\n
  Report weight function. Devel-helper.
******************************************************************************/
void Br2HdrManager::report_WeightFunc() const
{
    printf("[Manager: Weight Functions]\n");
    
    if (pWeightResp_U8_) 
    {
      printf("Weight Function U8 for response: '%s':\n", pWeightResp_U8_->what());
      if (pWeightResp_U8_->have_table())
        for (int i=0; i < 256; i++)
          printf("\ti=%d  w()=%f  w[]=%f\n", i, (*pWeightResp_U8_)(i), (*pWeightResp_U8_)[i]);
      else
        for (int i=0; i < 256; i++)
          printf("\ti=%d  w()=%f\n", i, (*pWeightResp_U8_)(i));
    }
    
    if (pWeightMerge_U8_) 
    {
      printf("Weight Function U8 for merging: '%s':\n", pWeightMerge_U8_->what());
      if (pWeightMerge_U8_->have_table())
        for (int i=0; i < 256; i++)
          printf("\ti=%d  w()=%f  w[]=%f\n", i, (*pWeightMerge_U8_)(i), (*pWeightMerge_U8_)[i]);
      else
        for (int i=0; i < 256; i++)
          printf("\ti=%d  w()=%f\n", i, (*pWeightMerge_U8_)(i));
    }
    
    if (pWeightMerge_U16_)    // we print each 256-th value and the last one
    {
      printf("Weight Function U16 for merging: '%s':\n", pWeightMerge_U16_->what());
      if (pWeightMerge_U16_->have_table()) {
        for (int i=0; i < pWeightMerge_U16_->zmax(); i += 256)
          printf("\ti=%d  w()=%f  w[]=%f\n", i, (*pWeightMerge_U16_)(i), (*pWeightMerge_U16_)[i]);
        int i = pWeightMerge_U16_->zmax();
        printf("\ti=%d  w()=%f  w[]=%f\n", i, (*pWeightMerge_U16_)(i), (*pWeightMerge_U16_)[i]);
      }
      else {
        for (int i=0; i < pWeightMerge_U16_->zmax(); i += 256)
          printf("\ti=%d  w()=%f\n", i, (*pWeightMerge_U16_)(i));
        int i = pWeightMerge_U16_->zmax();
        printf("\ti=%d  w()=%f\n", i, (*pWeightMerge_U16_)(i));
      }          
    }
    
    printf("\tshape response: #%d,  pWeightResp_U8: %s\n", 
        weight_func_shape_resp_, pWeightResp_U8_ ? "e" : 0);
    printf("\tshape merging: #%d,  pWeightMerge_U8: %s   pWeightMerge_U16: %s\n", 
        weight_func_shape_merge_, pWeightMerge_U8_ ? "e" : 0, pWeightMerge_U16_ ? "e" : 0);
}



////////////////////////////////////
///
///   Define the STATIC elements
///
////////////////////////////////////

TNT::Array1D <double> 
Br2HdrManager::null_response_curve_ = TNT::Array1D<double> ();


// END OF FILE

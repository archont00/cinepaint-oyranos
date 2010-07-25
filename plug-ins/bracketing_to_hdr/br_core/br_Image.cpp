/*
 * br_Image.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file br_Image.cpp  
   
  Referenzaehlende Image-Klasse.
*/
#include <iostream>
#include <cmath>                    // pow()
#include <cstring>                  // strlen strcpy

#include "br_types.hpp"             // ImageID
#include "br_enums.hpp"             // DataType, ReportWhat
#include "br_enums_strings.hpp"     // DATA_TYPE_STR[] etc.
#include "br_Image.hpp"             // Image, ExposureData, BrImgVector
#include "br_macros.hpp"            // CTOR(), BR_ERROR(), warn_preamble__


namespace br {


///////////////////////////////////////
///
///  class  Image
///
///////////////////////////////////////

/**+*************************************************************************\n
  Static function, usable in the `:´-scope of a Ctor.
******************************************************************************/
int Image::byte_depth (DataType dtype)
{
     switch (dtype) 
     {
       case DATA_U8:  return 1;
       case DATA_U16: return 2;
       case DATA_F32: return 4;
       case DATA_F64: return 8;
       default: BR_ERROR(("Not handled data type: %d - '%s'", dtype, data_type_str(dtype)));
     }
     return 0;
}

/**+*************************************************************************\n
  Default Ctor
******************************************************************************/
Image::Image()      
  : 
    id_               (0),
    width_            (0),
    height_           (0),
    channels_         (0),
    byte_depth_       (0),
    bit_depth_        (0),
    data_type_        (DATA_NONE),
    storing_scheme_   (STORING_NONE),
    image_type_       (IMAGE_NONE),
    offset_           (0,0),
    offset_id_        (0),
    active_offset_    (0,0),
    active_offset_ok_ (true)
{
    CTOR("Default");
  
    // ID-Vergabe wie im Init-Ctor sinnvoll? Ich denke, nein. Wir erlauben
    //  keine Moeglichkeit, ein solches Leer-Objekt zu fuellen, Objekte
    //  koennen nur kopiert werden, wobei die ImageID mitkopiert wird! 
}                  

/**+*************************************************************************\n
  Ctor with ImageType argument.
  
  @todo Im Fehlerfall besser werfen statt exit(1).
******************************************************************************/
Image::Image (int W, int H, ImageType iType, const char* name)
{
    CTOR(name);
    
    image_type_ = iType;
    
    switch (iType)
    {
      case IMAGE_RGB_U8:
        _Image (W,H, 3, DATA_U8, STORING_INTERLEAVE, name); break;
      
      case IMAGE_RGB_U16:
        _Image (W,H, 3, DATA_U16, STORING_INTERLEAVE, name); break;
      
      case IMAGE_RGB_F32:
        _Image (W,H, 3, DATA_F32, STORING_INTERLEAVE, name); break;
        
      default:
        //image_type_ = IMAGE_UNKNOWN;
        BR_ERROR(("Unhandled ImageType: %d - '%s'\n", iType, image_type_str(iType)));  
        exit(1);                            /// TODO  besser werfen!
    }
}                  

/**+*************************************************************************\n
  Ctor for INTERLEAVE storing schemes 
******************************************************************************/
Image::Image (int W, int H, int channels, DataType dtype, const char* name)
{
    CTOR(name);
    _Image (W,H, channels, dtype, STORING_INTERLEAVE, name);
    eval_image_type();
}                  

/**+*************************************************************************\n
  Private helper for Ctors.   No setting of \a image_type_!!
******************************************************************************/
void Image::_Image (int W, int H, int channels, DataType dtype, 
                        StoringScheme store, const char* name)
{
    id_counter_++;  
    if (id_counter_==0) id_counter_= 1;     // counter overflow: start again with `1´
    id_ = id_counter_;
    
    width_          = W;
    height_         = H;
    channels_       = channels;
    byte_depth_     = byte_depth (dtype);
    bit_depth_      = 8 * byte_depth_;      // default: all bits
    data_type_      = dtype;
    storing_scheme_ = store;
    active_offset_  = Vec2_int (0);
    offset_         = Vec2_int (0);
    offset_id_      = 0;
    active_offset_ok_ = true;

    if (name) {
      name_ = TNT::i_refvec<char> (strlen(name) + 1);
      strcpy (name_.begin(), name);
    }

    int buffer_size = W * H * channels_ * byte_depth_;
    *(TNT::Array1D<unsigned char>*)this = TNT::Array1D<unsigned char> (buffer_size);
}

/**+*************************************************************************\n
  Private helper for Ctors: Determines image_type from data_type, channels and 
   storing_scheme.
******************************************************************************/
void Image::eval_image_type()
{
    if ((channels_ == 3) && (storing_scheme_ == STORING_INTERLEAVE))
      switch (data_type_) 
      {
        case DATA_U8:  image_type_ = IMAGE_RGB_U8;  break;
        case DATA_U16: image_type_ = IMAGE_RGB_U16; break;
        case DATA_F32: image_type_ = IMAGE_RGB_F32; break;
        case DATA_F64: image_type_ = IMAGE_RGB_F64; break;
        default:       image_type_ = IMAGE_UNKNOWN;
      }
    else
      image_type_ = IMAGE_UNKNOWN; 
}

/**+*************************************************************************\n
  Dtor
******************************************************************************/
Image::~Image()
{   
    DTOR(name());
    
    //std::cout << "\tref counter: " << ref_count() << '\n';
    //std::cout << "\tref counter name: " << name_.ref_count() << '\n';
}

void Image::name (const char* s)
{
    if (s) {
      name_ = TNT::i_refvec<char> (strlen(s) + 1);
      strcpy (name_.begin(), s);
    }
    else
      name_ = TNT::i_refvec<char> ();  // Null-vector
}

void Image::report (ReportWhat what)  const
{
    std::cout << "Report for Image \"" << (name() ? name() : "(unnamed)")
              << "\" (ID=" << imageID() << "):"
              << "\n  [ " << width() << " x " << height() << " ]";
    if (what & REPORT_BASE) 
    {
      std::cout << "\n  [Base Data...]"
                << "\n    channels   : " << channels()
                << "\n    DataType   : " << DATA_TYPE_STR[data_type()]
                << "\n    byte_depth : " << byte_depth()
                << "\n    bit_depth  : " << bit_depth()
                << "\n    buffer_size: " << buffer_size() 
                << "\n    storing scheme: " << STORING_SCHEME_STR[storing()]
                << "\n    offset     : (" << offset_.x << "," << offset_.y 
                << ")   offset_ID: " << offset_id_ 
                << "   active_offset: (" << active_offset_.x << ',' 
                                         << active_offset_.y << ')';
    }
    std::cout << '\n';
}


///////////////////////////////////////
///
///  struct ExposureData
///
/////////////////////////////////////// 

ExposureData::ExposureData (float tm)
{
    time     = tm;
    shutter  = 0.0f;    // '0' indicates 'not given'
    aperture = 0.0f;    // '0' indicates 'not given'
    ISO      = 0.0f;    // '0' indicates 'not given'
    active   = true;
}

void ExposureData::report() const
{  
    std::cout << "  [Exposure Data...]"
              << "\n    Time (abstract): " << time
              << "\n    Shutter speed  : " << "1 / " << shutter << " sec"
              << "\n    Aperture       : " << aperture
              << "\n    ISO speed      : " << ISO
              << '\n';
}


///////////////////////////////////////
///
///   ImgStatisticsData
///
/////////////////////////////////////// 

void ImgStatisticsData::report() const
{  
    std::cout << "  [Image Statistics...]"
              << "\n    average brightn. per pixel: " << brightness
              << "\n    pixels at lower bound      : " << n_low
              << "\n    pixels at upper bound      : " << n_high
              << "\n    pixels in working range    : " << r_wk_range*100<< " %"
              << '\n';
}


////////////////////////////////////////////
///
///   BrImage  - subclass of Image
///
//////////////////////////////////////////// 

/*!  Beachte: offset bezieht sich via offset_ID auf bestimmte imageID. Wird offset
   kopiert, muss auch offset_ID kopiert werden. Auch hiesiges Bild bezieht sich 
   dann auf jenes, sei es vorhanden noch oder nicht. FRAGE: Auch active_offset
   etc. kopieren?
*/
void BrImage::copy_meta_data (const BrImage & src)
{
    exposure = src.exposure;
    statistics = src.statistics;
    name (src.name());
    offset (src.offset());
    offset_ID (src.offset_ID());
}

/*!  Determines exposure time from EXIF data. If shutter value <= 0 (not set),
   we end with time = 0.0, whereas a missing ISO or aperture value does not
   zero it (we take them as multipliers and don't multiply then). The absolute
   value of \a time has no meaning for the response recovering algorithm, it
   is a formal value reflecting the *relation* between the exposures. Though,
   if some images of a series have e.g. an ISO value and some have not, then
   this relation would be break nethertheless. For that reason the \a mask
   parameter is introduced, which allows to include/exclude the ISO or/and the
   aperature value from time determination (see BrImgVector::setTimesByEXIF()).
   Default: include all. --  Yet in question is handling of aperture values,
   because a statement "1.4" means in reality sqrt(2)=1.414..., "2.8" means
   sqrt(8) = 2.828... etc. Currently I take simply the square of the EXIF value.
*/
void BrImage::time_by_EXIF (unsigned mask)
{
    exposure.time = 0.0f;
    
    if (exposure.shutter > 0.0f)
      exposure.time = 1.0f / exposure.shutter;
      
    if (exposure.ISO > 0.0f  &&  mask & EXIF_ISO)
      exposure.time *= exposure.ISO;
      
    if (exposure.aperture > 0.0f  &&  mask & EXIF_APERTURE)
      exposure.time /= (exposure.aperture * exposure.aperture);
}


void BrImage::report (ReportWhat what) const
{
    Image::report (what);
    if (what & REPORT_EXPOSE_INFO) exposure.report();
    if (what & REPORT_STATISTICS)  statistics.report();
}


///////////////////////////////////////////////
///
///   BrImgVector  - the image container
///
/////////////////////////////////////////////// 

/**+*************************************************************************\n
  Ctor.
******************************************************************************/
BrImgVector::BrImgVector()
  : 
    size_active_  (0),
    first_active_ (-1),         // indicates 'no first active'
    last_active_  (-1),         // indicates 'no last active'
    use_offsets_  (false) 
{   
    CTOR("")
}

/**+*************************************************************************\n
  Dtor
******************************************************************************/
BrImgVector::~BrImgVector()
{
    DTOR("");
    vec_.clear();
}

/**+*************************************************************************\n
  DataType of the images (all images should have the same)
******************************************************************************/
DataType BrImgVector::data_type() const
{
    if (vec_.size())  return vec_[0].data_type(); 
    else              return DATA_NONE;
}

/**+*************************************************************************\n
  ImageType of the images (all images should have the same)
******************************************************************************/
ImageType BrImgVector::image_type() const
{
    if (vec_.size())  return vec_[0].image_type(); 
    else              return IMAGE_NONE;
}

/**+*************************************************************************\n
  Inserts \a img so, that order in brightness increases. I.e. brightness
   calculation for \a img had to be done before!
  Wrong idea: If time value of \a img is > 0 (indicator for 'has EXIF data') 
   \a img is inserted so, that order in time increases; otherwise, that order in 
   brightness increases. But FAILS, because the images residing already in the
   container have not necessary a time > 0 and ordering could fail.
******************************************************************************/
void BrImgVector::add (BrImage & img)
{
    int i=0;
    while (i < size() && img.brightness() >= vec_[i].brightness())  ++i;
    vec_.insert (vec_.begin()+i, img);

    update_size_active();  // updates size_active_, first_active_, last_active_
    compute_active_offsets();
    calc_intersection();
}

/**+*************************************************************************\n
  Returns container position (index) of image with ID \a id or -1 if no such
   image in the container.
******************************************************************************/
int BrImgVector::pos_of_ID (ImageID id) const
{
    if (id == 0) return -1;  // Shortcut asserting that no image has ID==0!
    
    for (int i=0; i < size(); i++)
      if (vec_[i].imageID() == id) return i;

    return -1;  // no image of that ID in container
}

/**+*************************************************************************\n
  Computes number of activated images in container, the first active and the 
   last active image. Private.
******************************************************************************/
void BrImgVector::update_size_active()
{ 
    size_active_  = 0;
    first_active_ = -1;
    last_active_  = -1;
  
    for (int i=0; i < size(); i++) 
      if (vec_[i].active())  size_active_++;
  
    for (int i=0; i < size(); i++)
      if (vec_[i].active()) {
        first_active_ = i;  break;
      }
    
    for (int i=size()-1; i >= 0; i--) 
      if (vec_[i].active()) {
        last_active_ = i;  break;
      }
}

/**+*************************************************************************\n
  Returns index of next active image behind index `k' or -1 if no such behind.
******************************************************************************/
int BrImgVector::next_active (int k) const
{ 
    for (int i=k+1; i < size(); i++) 
      if (vec_[i].active())
        return i;
        
    return -1;  // no active image behind `k'
}

/**+*************************************************************************\n
  Fuer \a !use_offsets_ werden alle \a active_offset_ Werte auf Null gesetzt und
   true returniert.
  
  Fuer use_offsets_ wird fuer alle aktivierten Bilder der Versatz (Offset) zum ersten
   aktiven Bild aus den lokalen Versaetzen berechnet. Dabei wird beginnend mit dem
   ersten aktiven Bild jeweils das naechste aktive \a next angewaehlt, und von dorther
   ueber die offset_ID's der Zwischenbilder von \a pos zu \a pos zum  Ausgangsbild
   \a i zurueckgehangelt, um die Offset-Werte aufzuaddieren. <pre>
                       <---offs_ID--+
  indices   ....| i | pos |   |   |next|   |   |....
  active          o                 o        o   
  offsets         |<<<<|<<<<<<<<<<<<| </pre>
  
  Dabei kann es vorkommen, dass die Offset-Kette von \a next zurueck zu \a i sich
   nicht schliesst (abbricht oder \a i ueberspringt). Dann wird mit False abgebrochen.
   Um dabei keinen gefaehrlichen Zustand zu hinterlassen, werden alle actice_offsets
   am Anfang genullt. Nach einem Abbruch bleiben die bis dahin erfolgreich berechneten 
   Offsets stehen.
  
  Gefahren: while()-Schleife koennte u.U. nicht terminieren, wenn offset_IDs auch
   "nach hinten" zeigten. Daher in Testphase ein check_offset_IDs() vorangestellt.
  
  @return True, if for all activated images the active offsets could be computed
   (all needed offset chains were complete), otherwise False. True is also returned,
   if not any image was activated. So 'false' indicates a problem with offset
   chains, nothing else.
  
  @internal Derzeit die einzige Stelle, wo \a use_offsets_ ausgewertet wird.
******************************************************************************/
bool BrImgVector::compute_active_offsets()
{ 
    /*  For the case !use_offsets_ as well as to left behind a save state if we
       have to abbort, we zero all active_offset_'s. And we set the active_offset_ok_'s
       TRUE for !use_offsets_ and FALSE (as start) for use_offsets.
       FRAGE: Nicht besser nur Variablen der *aktiven* Bilder setzen? */
    for (int i=0; i < size(); i++) {
      vec_[i].active_offset (Vec2_int(0));
      vec_[i].active_offset_ok (! use_offsets_);
    }     
    
    /*  If offset data shall not be used, we are ready here */  
    if (!use_offsets_) return true;
    
    /*  If no actives, return true (nothing wrong WITH OFFSET CHAIN) */
    if (! size_active()) return true; 

    /*  first active image is always ok, because offset is always 0 */                                
    vec_[first_active()].active_offset_ok (true);
    
    /*  Check whether all offset_ID's refer correctly */    
    if (! check_offset_IDs()) return false; // warning is given there

    /*  For all active images except the last one... */
    for (int i=first_active(); i < last_active(); i++) {
      if (! vec_[i].active()) continue;  // skip inactive images
      
      int next = next_active (i);        // i is active!
      
      if (next < 0) {
        BR_WARNING(("No next active image to #%d. Abborted.\n", i));
        return false;   // no next active image (should not happen)
      }
        
      Vec2_int  offset(0);  // for adding up the inner offsets
      /*  Von `next' ueber die offset_ID's zurueckhangeln zu `i', um Offsets aufzusummieren */
      bool found = false;
      int k = next;
      while (!found) {
        /*  position in BrImgVector, the offset_ID of `k' refers to */
        int pos = pos_of_ID (vec_[k].offset_ID());

        if (pos < i) {  // i uebersprungen oder -1
          BR_WARNING1(("Missing link in offset chain from #%d to #%d (pos=%d)\n", next, i, pos));
          return false;  // offset chain from `next' to `i' not closeable
        }
            
        offset += vec_[k].offset();  // add up the inner offsets
        if (pos == i) {  
          /*  chain closed for `i': compute offset to the first active image, store
               the value in image 'next' and go to next i. */
          printf("offset von #%d zu #%d = (%d,%d)", next, i, offset.x, offset.y);
          offset += vec_[i].active_offset();  // (i-first)+(next-i)=(next-first)
          vec_[next].active_offset (offset);  // store in `next'
          vec_[next].active_offset_ok (true); // set the ok flag
          printf(",  active offs = (%d,%d)\n", offset.x, offset.y);
          found = true;                        // go to next i
        }
        else 
          k = pos;  // next `k' (pos=-1 impossible due to above return for pos < i)
      } 
    }  // for i
    return true;
}

/**+*************************************************************************\n
  Checks, that all offset_IDs refer to positions *before* their image or to -1
   (== offset not computed or reference image no more in container). Returns
   false, if an offset_ID refers to a position behind the image or to the image
   itself. Devel helper.
******************************************************************************/
bool BrImgVector::check_offset_IDs() const
{
    bool res = true;
    for (int i=0; i < size(); i++) {
      int pos = pos_of_ID (vec_[i].offset_ID());
      if (pos >= i) {  // Offset nicht zu einem vorderen Bild
        printf("CORRUPT offset_ID at pos %d:  pos(ID)=%d\n", i, pos);
        res = false; 
      }
    }
    return res;
}

/**+*************************************************************************\n
  Calculates the intersection area of all activated images concerning their
   active_offset values. I.e. compute_active_offsets() should be called before.
******************************************************************************/
void BrImgVector::calc_intersection()
{ 
    int first = first_active();
    if (first < 0) {
      intersection_.set_empty(); 
      return;
    }
    int x0 = 0;  
    int xE = image(first).width();
    int y0 = 0; 
    int yE = image(first).height();
    
    for (int i=first; i < size(); i++) {
      if (! image(i).active()) continue;
      Vec2_int  d = image(i).active_offset();
      /* Ginge etwas effektiver, aber so lesbarer: */
      x0 = std::max (x0, -d.x);
      y0 = std::max (y0, -d.y);
      xE = std::min (xE, image(i).width() - d.x);
      yE = std::min (yE, image(i).height() - d.y);
    }
    //printf("%s(): x0=%d xE=%d,  y0=%d yE=%d\n",__func__,x0,xE,y0,yE);
    intersection_.set (x0, y0, xE-x0, yE-y0);
}


/**+*************************************************************************\n
  Sets time(i) = time(i-1) * 2^stop. The first time is taken as it was, except
   it is 0, then it is set 1.0.
******************************************************************************/
void BrImgVector::set_times_by_stop (double stop)
{
    if (! size()) return;
    
    if (vec_[0].time() <= 0.0f)
      vec_[0].set_time(1.0f);
    
    for (int i=1; i < size(); i++) 
      vec_[i].exposure.time = vec_[i-1].exposure.time * pow(2.0, stop);
}
/**+*************************************************************************\n
  Sets the first time = 1.0 and calls set_times_by_stop().
******************************************************************************/
void BrImgVector::set_normed_times_by_stop (double stop)
{
    if (! size()) return;
    vec_[0].set_time (1.0f);
    set_times_by_stop (stop);
}
/**+*************************************************************************\n
  Sets active_time(i) = active_time(i-1) * 2^stop. First active time ist taken
   as it was, except it is 0, then it is set to 1.0.
******************************************************************************/
void BrImgVector::set_active_times_by_stop (double stop)
{
    if (! size_active()) return;  // i.e. first_active() is now a valid pos
    
    if (vec_[first_active()].time() <= 0.0f)
      vec_[first_active()].set_time (1.0f);
    
    for (int i=first_active(); i < last_active(); i++) 
    { 
      if (! vec_[i].active()) continue;
      int  next = next_active(i);       // i is active
      if (next < 0) return;             // should not be possible
      vec_[next].exposure.time = vec_[i].exposure.time * pow(2.0, stop);
    }
}
/**+*************************************************************************\n
  Sets the first active time = 1.0 and calls set_active_times_by_stop().
******************************************************************************/
void BrImgVector::set_normed_active_times_by_stop (double stop)
{
    if (! size_active()) return;
    vec_[first_active()].set_time (1.0f);
    set_active_times_by_stop (stop);
}

/**+*************************************************************************\n
  Sets exposure times accordingly to EXIF data. For a shutter value <= 0 (not
   set) the resulting time is 0, whereas a missing ISO or aperture value does
   not zero it (taken as multipliers). Aperture as well as ISO value are taken
   into accout only, if *all* images of the series have this EXIF info, because
   otherwise the time *relations* between images would get wrong.
  
  @internal Da koennte nach set_active_times_by_EXIF() verlangt werden, denn
   unter den aktiven koennte es diesbzgl. anders aussehen als unter allen. 
******************************************************************************/
void BrImgVector::set_times_by_EXIF()
{
    bool have_aperture = true;  // Have all images a valid aperture value?
    bool have_ISO = true;       // Have all images a valid ISO value?
    
    for (int i=0; i < size(); i++) {
      have_aperture &= (vec_[i].exposure.aperture > 0.0f);
      have_ISO      &= (vec_[i].exposure.ISO > 0.0f);
    }
    unsigned mask = 0;
    if (have_aperture) mask |= BrImage::EXIF_APERTURE;
    if (have_ISO)      mask |= BrImage::EXIF_ISO;
    
    for (int i=0; i < size(); i++) 
      vec_[i].time_by_EXIF (mask);
}

/**+*************************************************************************\n
  Calls set_times_by_EXIF() and scales times then so, that the first time
   becomes 1. If first time is 0, no scaling is done.
******************************************************************************/
void BrImgVector::set_normed_times_by_EXIF()
{
    set_times_by_EXIF();
    normalize_times();
}

/*!  Scales times of all images so, that the first time becomes 1. If the first
   time is 0, no scaling is done.
*/
void BrImgVector::normalize_times()
{
    if (! size()) return;
    
    float t0 = vec_[0].exposure.time;
    
    if (t0 != 0.0f)
      for (int i=0; i < size(); i++)
        vec_[i].exposure.time /= t0;
}

/*!  Scales times of all *activated* images so, that the first active time
   becomes 1. If the first active time is 0, no scaling is done.
*/
void BrImgVector::normalize_active_times()
{
    if (! size_active()) return;
    
    float t0 = vec_[first_active()].exposure.time;
    
    if (t0 != 0.0f)
      for (int i=first_active(); i <= last_active(); i++) { 
        if (! vec_[i].active()) continue;
        vec_[i].exposure.time /= t0;
      }
}

/*!  Checks whether *all active* times are > 0 (valid); returns TRUE if yes,
   else FALSE. If no images are active, we return TRUE.
*/
bool BrImgVector::have_times()
{
    for (int i=0; i < size(); i++) {
      if (! vec_[i].active()) continue;
      if (vec_[i].time() <= 0.0f) 
        return false;
    }
    return true;
}


/**+*************************************************************************\n
  Reports meta data of vector elements...
******************************************************************************/
void BrImgVector::report (ReportWhat what) const
{ 
    printf ("=== BrImgVector: meta data === [%d]\n", size());  
    for (int i=0; i < size(); i++)
    {
      printf("[%2d]: ", i);  vec_[i].report(what);
    }
}
/**+*************************************************************************\n
  Reports for each image the current offset and active_offsets values.
******************************************************************************/
void BrImgVector::report_offsets() const
{
    if (size() < 1) {
      printf("%s(): no images\n",__func__);
      return;
    }
    char* NoYes[2] = {"No","Yes"};
    for (int i=0; i < size(); i++) {
      printf("%2d (ID %d): offs=(%2d,%2d)  offs_ID=%d,  act_offs=(%2d,%2d)  ok=%d,  is_act=%s\n", 
         i, image(i).imageID(), 
         image(i).offset().x, image(i).offset().y, image(i).offset_ID(),
         image(i).active_offset().x, image(i).active_offset().y,
         image(i).active_offset_ok(),
         NoYes[image(i).active()==true] );
    }
}
/**+*************************************************************************\n
  Reports for each image the current intersection boundaries.
******************************************************************************/
void BrImgVector::report_intersection()
{
    const Rectangle &  S = intersection_;
    printf("Intersection:  x: [%d - %d]  y: [%d - %d]\n", 
        S.x0(), S.xlast(), S.y0(), S.ylast()); 
    for (int i=0; i < size(); i++) {
      const BrImage & img = image(i);
      const Vec2_int & D = img.active_offset();
      printf("%2d: size=(%3d,%3d)  act_offs=(%2d,%2d):  [%d - %d] x [%d - %d]\n",
          i, img.xSize(), img.ySize(), D.x, D.y,
          S.x0() + D.x,  S.xlast() + D.x,
          S.y0() + D.y,  S.ylast() + D.y );
    }     
}

/////////////////////////////////////////
///
///  Non-member helpers
///
/////////////////////////////////////////

//=============================================================================
//  Helper for the macro NOT_IMPL_IMAGE_CASE().
//=============================================================================
void not_impl_image_case (const char* file, int line, const char* func, const Image& img)
{
    fprintf (stderr, "\n%s: [%s:%d] %s():\n\tNot implemented for images of Type='%s', "
                     "Data='%s', Storing='%s', channels=%d.\n", 
        warn_preamble__, file, line, func, 
        IMAGE_TYPE_SHORT_STR [img.image_type()], 
        DATA_TYPE_SHORT_STR  [img.data_type()],
        STORING_SCHEME_STR   [img.storing()],
        img.channels() );
}


/////////////////////////////////////////
///
///   Static elements
///
/////////////////////////////////////////

ImageID Image::id_counter_ = 0;


}  // namespace "br"

// END OF FILE

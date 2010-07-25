/*
 * br_Image.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file  br_Image.hpp
  
  Referenzzaehlende Image-Klasse.
  
  Den vollen Erfolg verhinderte einst noch das `char* name'-Element. Dieses
   entweder bei jeder Copy-Aktion physisch kopieren oder besser auch
   referenzzaehlend. Dazu dient jetzt ein tnt_i_refvec<char>-Element.
   Oder gleich std::string nehmen.
   
  Zu Image passt nur ein ImgVector, der Objekte haelt und nicht ledigl. Zeiger.
   In der Zeigervariante duerften auch hier nie automatische Objekte 
   eingereiht werden, denn einen Zeiger nehmen (kopieren) erhoeht nicht
   den Ref-Zaehler. So wuerden diese Objekte einmal automatisch zerstoert und
   dann ein zweites mal durch ImgVector-Dtor per delete. Nachteil freilich
   der Objekt-Variante, dass ein BrImage schon einige Bytes hat, aber 
   ImgVectoren werden nicht oft kopiert.
   
  ENTSCHEIDUNG: `Image' von Array1D *ableiten* oder (analog `name_') ein 
   Array1D `buffer' als *Element* halten? Ableiten hat Vorteil, dass die 
   Array1D-Funktionalitaet sofort zur Verfuegung steht (z.B. cout gibt lineare
   uchar-Daten aus, ref_count()). Element ist u.U. etwas uebersichtlicher. Zum
   Referenzzaehlen taugt beides, im zweiten Fall wird Objekt zwar per default
   Copy-Ctor kopiert, doch das Element `buffer' bringt dann seinen referenz-
   zaehlenden Copy-Ctor zum Einsatz. Entschieden fuers Ableiten.

*/
#ifndef br_Image_hpp
#define br_Image_hpp


#include <vector>
#include "TNT/tnt_array1d.hpp"
#include "Vec2.hpp"             // Vec2_int
#include "br_types.hpp"         // ImageID
#include "br_enums.hpp"         // DataType, ..., ReportWhat
#include "br_macros.hpp"        // ASSERT(), RELAX, CTOR(), DTOR()



#ifdef BR_CHECK_IMG_TYPE

# define CHECK_IMG_TYPE_U8(img) \
                ASSERT(img.data_type() == DATA_U8, RELAX);\
                ASSERT(img.byte_depth() == 1, RELAX);\
                ASSERT(img.channels() == 1, RELAX);

# define CHECK_IMG_TYPE_RGB_U8(img) \
                ASSERT(img.data_type() == DATA_U8, RELAX);\
                ASSERT(img.byte_depth() == 1, RELAX);\
                ASSERT(img.channels() == 3, RELAX);\
                ASSERT(img.storing() == STORING_INTERLEAVE, RELAX);

# define CHECK_IMG_TYPE_U16(img) \
                ASSERT(img.data_type() == DATA_U16, RELAX);\
                ASSERT(img.byte_depth() == 2, RELAX);\
                ASSERT(img.channels() == 1, RELAX);

# define CHECK_IMG_TYPE_RGB_U16(img) \
                ASSERT(img.data_type() == DATA_U16, RELAX);\
                ASSERT(img.byte_depth() == 2, RELAX);\
                ASSERT(img.channels() == 3, RELAX);\
                ASSERT(img.storing() == STORING_INTERLEAVE, RELAX);

# define CHECK_IMG_TYPE_F32(img) \
                ASSERT(img.data_type() == DATA_F32, RELAX);\
                ASSERT(img.byte_depth() == 4, RELAX);\
                ASSERT(img.channels() == 1, RELAX);

# define CHECK_IMG_TYPE_RGB_F32(img) \
                ASSERT(img.data_type() == DATA_F32, RELAX);\
                ASSERT(img.byte_depth() == 4, RELAX);\
                ASSERT(img.channels() == 3, RELAX);\
                ASSERT(img.storing() == STORING_INTERLEAVE, RELAX);
#else

# define CHECK_IMG_TYPE_U8(img)
# define CHECK_IMG_TYPE_RGB_U8(img)
# define CHECK_IMG_TYPE_U16(img)
# define CHECK_IMG_TYPE_RGB_U16(img)
# define CHECK_IMG_TYPE_F32(img)
# define CHECK_IMG_TYPE_RGB_F32(img)

#endif


/**+*************************************************************************\n
  Macro to give a "not implemented" message
******************************************************************************/
#define NOT_IMPL_IMAGE_CASE(img)    br::not_impl_image_case(__FILE__,__LINE__,__func__,img);


namespace br {

/**===========================================================================

  @class  Rectangle
  
  Besseren Namen finden!

=============================================================================*/
struct Rectangle {
   int  x, y;
   int  w, h;
   
   Rectangle()          {x = y = w = h = 0;}
   Rectangle (int X, int Y, int W, int H) :  x(X), y(Y), w(W), h(H)  {}
   void set (int X, int Y, int W, int H)  {x=X; y=Y; w=W; h=H;}
   int x0() const       {return x;}
   int y0() const       {return y;}
   int xlast() const    {return x + w - 1;}     // last index
   int ylast() const    {return y + h - 1;}     // last index
   int xend() const     {return x + w;}         // index behind last
   int yend() const     {return y + h;}         // index behind last
   int dim1() const     {return h;}             // 1. (=outer) dim in arrays
   int dim2() const     {return w;}             // 2. (=inner) dim in arrays
   int n_pixel() const  {return w * h;}
   bool is_empty() const {return w==0 || h==0;}
   void set_empty()     {x = y = w = h = 0;}
};


/**===========================================================================

  @class  Image

  Base data of an (input) image; without exposure and statistics stuff. 

  Derived from TNT::Array1D<uchar>. Array1D.dim() gibt die Puffergroesse in
   Bytes. Alle hier hinzugefuegten Elemente werden bei einem `a=b' physisch
   kopiert.
 
  Note: Return-Wert von buffer_size() als `size_t' veranschlagt, auch wenn
   gegenwaertiges Array1D.dim() nur `int' zurueckgibt. Ist kuenftiger. Bedenke
   aber, dass auch Index-Operator dann auf size_t gehen muesste.

  Note: Default-Ctor wird benoetigt, wenn zB. ein std::vector<Image>[n]
   angelegt werden soll, der braucht Default-Ctor seiner Objekte!
   
  <b>Zur Offset-Politik:</b> 
   Offsets dienen hier primaer dazu, berechnete Verrueckungen der Bilder einer
   Serie einzuarbeiten. Berechnet werden diese moeglichst zwischen benachtbarten
   Bildern, da dort der Ueberlapp brauchbarer Werte am groessten. Weil Bilder in
   den Bildcontainer auch nach einer Berechnung noch eingefuegt werden koennen,
   brauchen wir auch eine Information, auf welches Bild sich ein Offset-Wert
   bezieht (berechnet wurde), die blosse Position im Container reicht nicht.
   Dazu dient die Variable \a offset_id_, die die eindeutige ID des bzgl. Bildes
   enthaelt. 
   
  We define: \a offset_ is the offset relative to the image with ID \a offset_id_ 
   (image `A') in the following sense: Offset.x = +1 means: for best fit with
   image `A' do correlate A(x,y) <==> (*this)(x+1,y). (Scheint mir intuitiver
   als "A(x+1,y) <==> (*this)(x,y)".) 
  
  Theoretisch koennte ein Bild zu jedem anderen Bild (des Containers und darueberhinaus
   auch zu schon wieder entfernten Bilder) einen (berechneten) Offset-Wert besitzen.
   Wir stiften pro Bild aber nur eine Offset-Variable. Dazu setzen wir fest:
   Wird ein Offset zwischen zwei Bildern des Containers berechnet, wird dieser
   immer im Bild mit der hoeheren Container-Position abgelegt. Weil Bilder im
   Container nach aufsteigender Helligkeit geordnet, also immer im helleren Bild.
   
  Das bedeutet, dass die eingetragenen Offset_ID's immer auf Bildpositionen
   vor der eigenen Position verweisen muessten: pos_of_ID (offset_id) .<. my_pos.
   Wo das nicht der Fall, waere der Zustand korrupt. Die BrImgVector Funktion
   check_offset_IDs() ueberprueft das.
  
  Eine Schwierigkeit entsteht allerdings, wenn ein Offset-Wert von Bild i nach
   Bild k bestand, und zwischen i und k nun ein Bild j eingefuegt wird und
   der Offset zwischen j und k berechnet wird. Die alte "k-i" Offset-Information
   eingetragen bei k ginge dann verloren, die Kette von k nach i waere nicht
   mehr schliessbar, obwohl sie es schon mal war. Sollen wir dann in j den Offset
   zum Bild i als formale Differenz "j-i = (k-i)-(k-j)" eintragen (nicht eigentlich
   berechnet)? Oder sollten wir doch ein Buendel von Offset-Werten zulassen?
   Gleiches, wenn die Offsets "j-i" und "k-j" vorhanden waren und nun Bild j
   entfernt wird. Soll dann bei k die formale Summe "(k-j)+(j-i)" eingetragen 
   werden? Und was ist mit der dabei moeglw. unterschiedlichen Guete berechneter
   Offset-Werte? Wird mir zu kompliziert...
   
  Fuer die Pack-Generation ist nun fuer alle aktivierten Bilder der Offset auf
   einen gemeinsamen Bezugspunkt, praktischerweise das erste aktive Bild relevant,
   ich nenne das den "absoluten Offset". Dazu muss von jedem aktiven Bild ausgehend
   entlang der `offset_ID'-Bilder eine geschlossene Offset-Kette zum ersten Bild
   gebildet werden koennen, entlang derer die Offset-Werte aufzuaddieren sind.
   Die Kette kann dabei auch ueber inaktive Bilder laufen. Diese Berechnung
   erledigt die br::BrImgVector Funktion compute_active_offsets(). Die \a active_offset_
   Variable muesste nicht unbedingt Teil der Klasse Image sein, compute_active_offset()
   koennte die auch ein ein eigenes Feld schreiben. War aber erstmal das Einfachste.
   
  Praktisch: offset_id_ = 0 kann anzeigen, dass kein gueltiger Offset-Wert vorliegt
   (entweder noch nicht berechnet (ein "(0,0)" laesst das nicht erkennen!) oder
   Bezugsbild verschwand und Offset-Kette konnte nicht wieder geknuepft werden.
   
*============================================================================*/
class Image : public TNT::Array1D <unsigned char>
{
    static ImageID id_counter_;         // counting only in Init-Ctor(s)
    
    ImageID   id_;                      // starts with 1, "0" means unset or failure
    int       width_;                   // |
    int       height_;                  // |
    int       channels_;                // | these four define the buffer size
    int       byte_depth_;              // |
    int       bit_depth_;               // could be useful
    DataType  data_type_;               // U8, U16 
    StoringScheme  storing_scheme_;     // INTERLEAVE, CHNNLWISE
    ImageType image_type_;
    Vec2_int  offset_;                  // Offset to image with ID `offset_id_'
    ImageID   offset_id_;               // ID of the image, `offset_' refers to
    Vec2_int  active_offset_;           // Offset to the first active image
    bool      active_offset_ok_;        // Entstammt active_offset_ erfolgr. Berechn.?
    TNT::i_refvec<char> name_;          // ref counting string object
  
public:

    Image();
    Image (int W, int H, int channels, DataType, const char* name=0);
    Image (int W, int H, ImageType, const char* name=0);
    virtual ~Image();               // "virt" avoids warnings (Opportunist!)
    
    ImageID   imageID() const           {return id_;}
    int       width() const             {return width_;}    
    int       height() const            {return height_;}    
    int       dim1() const              {return height_;}  // hides Array1D.dim1()
    int       dim2() const              {return width_;}
    int       xSize() const             {return width_;}   // synonym to width()
    int       ySize() const             {return height_;}  // synonym to height()
    int       channels() const          {return channels_;}
    int       byte_depth() const        {return byte_depth_;}
    int       bit_depth() const         {return bit_depth_;}
    DataType  data_type() const         {return data_type_;}
    StoringScheme storing() const       {return storing_scheme_;}
    ImageType image_type() const        {return image_type_;}
    size_t    buffer_size() const       {return dim();}    // from Array1D
    int       n_pixel() const           {return width_ * height_;}
    int       byte_per_pixel() const    {return channels_ * byte_depth_;}
    
    //---------
    //  Offset
    //---------
    Vec2_int  active_offset() const             {return active_offset_;}
    void      active_offset (const Vec2_int& d) {active_offset_ = d;}
    bool      active_offset_ok() const          {return active_offset_ok_;}
    void      active_offset_ok (bool b)         {active_offset_ok_ = b;}
    Vec2_int  offset() const                    {return offset_;}
    void      offset (const Vec2_int& d)        {offset_ = d;}
    ImageID   offset_ID() const                 {return offset_id_;}
    void      offset_ID (ImageID id)            {offset_id_ = id;}
    
    //-----------------------------
    //  Access to the image buffer
    //-----------------------------
    const unsigned char* buffer() const {return &(*this)[0];}
    unsigned char*       buffer()       {return &(*this)[0];}
    
    //--------------
    //  image name 
    //--------------
    const char* name() const            {return name_.begin();}
    void        name (const char*);
    
    virtual void report (ReportWhat = REPORT_BASE) const;
    
private:
    
    //  Helper for Ctors
    void _Image (int W, int H, int channels, DataType dtype, 
                 StoringScheme store, const char* name);
    //  Helper for Ctors
    void eval_image_type();             
    
    //  *Static* helper for usage already in the `:´-scope
    static int byte_depth (DataType);    


#ifdef BR_CHECK_IMG_TYPE
    //  Functions to create wrong test data...
public:
    void _set_data_type (DataType d)          {data_type_ = d;}
    void _set_storing (StoringScheme ss)      {storing_scheme_ = ss;}
    void _set_channels (int n)                {channels_ = n;}
#endif

};


/**===========================================================================

  @struct  ExposureData

  Collects photographic exposure data of an input image.

*============================================================================*/
struct ExposureData
{
    float   time;       // effective exposure time concerning shutter speed, aperture and ISO
    float   shutter;    // EXIF - denominator of shutter time (1/shutter); in 1/s.
    float   aperture;   // EXIF - Blende
    float   ISO;        // EXIF - sensitivity (Filmempfindlichkeit)
    bool    active;     // Image activated for use? - Eigentlich kein ExposureData
    
    ExposureData (float time=0.0);
    
    void report() const;
};


/**===========================================================================

  @struct  ImgStatisticsData

  Collects statistics data of a single image.

*============================================================================*/
struct ImgStatisticsData
{
    double  brightness;
    int     n_low;      // number of pixels at the lower bound    
    int     n_high;     // number of pixels at the upper bound
    double  r_wk_range; // ratio of pixels within working range
                        //  == 1.0 - (n_low + n_high) / (width*height)
    ImgStatisticsData() 
      : brightness(0.), n_low(0), n_high(0), r_wk_range(0.)  {}
    
    void report() const;
};


/**===========================================================================

  @class  BrImage
    
  Image data  +  exposure data  +  statistics data. 
  Prefix "Br" for "Bracketing".
  
  OPEN: default value of exposure time = "0" or "-1.0"?

*============================================================================*/
class BrImage : public Image
{
public:
    enum
    {   EXIF_APERTURE   = 0x1, 
        EXIF_ISO        = 0x2,
        EXIF_ALL        = 0xFFFF
    };
  
    ExposureData  exposure;
    ImgStatisticsData  statistics;
  
    BrImage() {}
    BrImage (int W, int H, int channels, DataType dtype, const char* name=0,
             float tm=0)
      : Image (W, H, channels, dtype, name), 
        exposure (tm)
      {}  
    BrImage (int W, int H, ImageType iType, const char* name=0, float tm=0)
      : Image (W, H, iType, name), 
        exposure (tm)
      {}  
    
    const BrImage & getRef() const      {return *this;}
    BrImage       & getRef()            {return *this;}
    
    bool   active() const               {return exposure.active;}
    void   active (bool val)            {exposure.active = val;}
    float  time() const                 {return exposure.time;}
    double brightness() const           {return statistics.brightness;}
    
    void   set_time (float tm)          {exposure.time = tm;}
    void   time_by_EXIF (unsigned mask = EXIF_ALL);
    bool   have_EXIF() const            {return exposure.shutter != 0.0f;}
    
    void   copy_meta_data (const BrImage & src);
    void   report (ReportWhat = REPORT_DEFAULT) const;   // virtual
};



/**===========================================================================

  @class  BrImgVector  
  
  Our input image container. 
  Vector of (ref counting) BrImage objects (no pointers).

*============================================================================*/
class BrImgVector
{
    //  Our container for input images
    std::vector <BrImage>  vec_;
    
    //  Number of activated images, first active and last active image pos
    int  size_active_, first_active_, last_active_;

    //  Compute in compute_active_offsets() the offsets from local offsets?
    bool  use_offsets_;
    
    //  The intersection area of all activated images (concerning their active_offsets)
    Rectangle  intersection_;

        
public:
    //--------------
    //  Ctor & Dtor
    //--------------
    BrImgVector();
    ~BrImgVector(); 
    
    //------------------------------
    //  Frequently used information
    //------------------------------
    DataType    data_type() const;
    ImageType   image_type() const;
    int         size() const                  {return vec_.size();}  
    int         size_active() const           {return size_active_;}
    
    //--------------------------------
    //  Adding of images and clearing
    //--------------------------------
    void add (BrImage &);
    void clear()                              {vec_.clear(); update_size_active();}
  
    //-----------------------
    //  Access to the images
    //-----------------------
    BrImage &       operator[] (int i)        {return vec_[i];}
    const BrImage & operator[] (int i) const  {return vec_[i];}
  
    BrImage &       at (int i)                {return vec_.at(i);}
    const BrImage & at (int i) const          {return vec_.at(i);}
    
    BrImage &       image (int i)             {return vec_[i];}
    const BrImage & image (int i) const       {return vec_[i];}
    
    //-----------------------------------
    //  Vector position of image with ID
    //-----------------------------------
    int  pos_of_ID (ImageID) const;
    
    //-------------
    //  Activation
    //-------------
    void activate (int i)                     {vec_.at(i).active(true); update_size_active();}
    void deactivate (int i)                   {vec_.at(i).active(false); update_size_active();} 
    bool active (int i) const                 {return vec_.at(i).active();}
    int  first_active() const                 {return first_active_;}
    int  last_active() const                  {return last_active_;}
    int  next_active (int k) const;
    
    //----------
    //  Offsets
    //----------
    bool use_offsets() const                  {return use_offsets_;}
    void use_offsets (bool b)                 {use_offsets_ = b;}
    bool check_offset_IDs() const;
    bool compute_active_offsets();
    
    //--------------------
    //  Intersection area
    //--------------------
    void calc_intersection();
    const Rectangle & intersection() const    {return intersection_;}
    
    //---------------------------
    //  Seting of exposure times
    //---------------------------
    void set_times_by_stop (double stop);
    void set_normed_times_by_stop (double stop);
    void set_active_times_by_stop (double stop);
    void set_normed_active_times_by_stop (double stop);
    void set_times_by_EXIF();
    void set_normed_times_by_EXIF();
    void normalize_times();
    void normalize_active_times();
    bool have_times();
    
    //----------------
    //  Listing utils
    //----------------
    void report (ReportWhat = REPORT_DEFAULT) const;
    void report_offsets() const;
    void report_intersection();
    
private:
    void update_size_active();
};



/**+*************************************************************************\n
  If we want to emphasize that an `Image' should contain HDR data 
   (type==IMAGE_[RGB]_F32) we use the type identifier "ImageHDR". 
******************************************************************************/
typedef Image  ImageHDR;    



/**+*************************************************************************\n
  Helper for the NOT_IMPL_IMAGE_CASE() macro. Public because used by a public 
   macro.
******************************************************************************/
void not_impl_image_case (const char* file, int line, const char* func, const Image &);


}  // namespace "br"

#endif  // br_Image_hpp

// END OF FILE

/*
 * HdrCalctorBase.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file  HdrCalctorBase.hpp
  
  Content:
   - class HdrCalctorBase - Interface, which each concrete HdrCalctor 
      should provide. Data type independent.
*/
#ifndef HdrCalctorBase_hpp
#define HdrCalctorBase_hpp

#include "TNT/tnt_array1d.hpp"
#include "br_types.hpp"             // ImageID
#include "br_Image.hpp"             // Image, ImageHDR
#include "br_PackBase.hpp"          // PackBase
#include "FollowUpValuesBase.hpp"
#include "ResponseSolverBase.hpp"   // ResponseSolverBase::SolveMode



namespace br {

//using namespace TNT;


/**============================================================================

  @class  HdrCalctorBase 
  
  The idea was to have a single class managing all the computation stuff
   (follow-up curves, Z-Matrix creation, response curves). But meanwhile I
   think, it would be better to be more modulare and let the `HdrManager' 
   do also the computation management. But this is the present state.

  Interface, which each concrete HdrCalctor should provide. Data type 
   independent!
  
  Aim is to hide the concretely used `Pack', i.e. the concrete data type
   (U8, U16,...) as well as the concrete way of storing the pack data, e.g.
   single images or single packed channels, "RGBRGB.." or "RR..GG..BB.."
   format etc.

  HdrCalctorBase has to hands out the type-independent functionality of Pack,
   described in PackBase, and the type-independent functionality of
   FollowUpValues, described in FollowUpValuesBase. The simplest way doing
   this are abstract methods ``PackBase & packBase() const = 0'', which
   the concrete HdrCalctors have to implement using their Pack. The user can
   refer then e.g. to Pack's dim1() via pCalctorBase->packBase().dim1().

  Um das "packBase()" zu sparen liegt der Gedanke nahe, diese Funktionen
   durch CalctorBase nochmal direkt darzureichen, sozwar als schon konkrete,
   was die nicht-abstrakte PackBase-Klasse ja gestatten wuerde. Dazu muss hier
   im CalctorBase die Referenz oder Adresse des Packs des *schliesslichen*
   Calctors bekannt sein. Der Einfall indes, diese im CalctorBase-Ctor als
   Argument mitzuteilen, ist kein guter. Denn vom Pack-Argument des Calctor-
   Ctors ist eine (referenzzaehlende) Kopie zu ziehen (um die Bilddaten fuer 
   die Lebenszeit des Calctors zu sperren). Die Referenz dieser Kopie waere
   CalctorBase mitzuteilen. Doch kann ein in Calctor hinzugekommenes Element
   nicht *vor* der Calctor-Basisklasse initialisiert werden. Und der Code,

   <pre>  Calctor::Calctor (Pack & pck) : CalctorBase(pck), pack_(pck)  {}</pre>

   wuerde CalctorBase die Referenz des Ctor-Arguments \a pck mitteilen, statt
   der eigenen Kopie \a pack_. Wenn \a pck ein Objekt mit kuerzerer Lebensdauer
   als der damit (zB. auf dem Heap) konstruierte Calctor, referenzierte die
   PackBase-Referenz von CalctorBase ein nicht mehr existierendes Objekt.

  So bleiben wohl nur zwei Moeglichkeiten: 
   - In CalctorBase einen <tt>PackBase</tt>-ZEIGER zu haben, den der Calctor-Ctor
     NACHTRAeGLICH zu richten haette:
       <pre>
     Calctor::Calctor (Pack & pck) : CalctorBase(), pack_(pck)
       {CalctorBase::setPackBasePtr(pack_);} </pre>

   - In CalctorBase eine abstrakte packBase()-Methode zu nutzen, die
     die End-Calctoren zu implementieren haben.
 
   Das zweite etwas sicherer insofern, als es nicht vergessen werden kann,
    das erste vielleicht etwas Laufzeit-effizienter.
 
  Aus dem gleichen Grund hier auch noch keine Referenz einer konkreten
   FollowUpValuesBase implementierbar, selbst wenn der Basis-Ctor ohne
   PackBase-Argument auskommt. Denn die Ctoren der konkreten FollowUp-
   Klassen haben dann ein Pack-Argument, doch ein Cast von diesen auf die
   Basis-Referenz hier muesste VOR der Pack-Kopie stattfinden. Daher auch 
   diese als abstrakte Methode impl.: zu definieren von den Erben.

  UNSCHOeN:
   Ist \a refpic Variable hier wirklich noetig? Die im HdrManager ja, weil
   Calctoren nicht ueberdauern, aber hier nochmal? Ermoeglicht zumindest
   abstraktes update_FollowUpValues().

  @todo Die Felder der Antwortkurven \a logXcrv_ und \a Xcrv_, darauf die Erben
    Zugriff benoetigen, aus Bequemlichkeit protected deklariert. Politisch
    korrekt waeren private Variablen und protected Zugriffsfunktionen. Oder
    jene Erben als befreundet eintragen.

=============================================================================*/
class HdrCalctorBase 
{
protected:
      
    //  The three "response curves" we compute for each channel
    TNT::Array1D<double>  logXcrv_[3];  // logarithm: for HDR merging
    TNT::Array1D<double>  Xcrv_[3];     // unlogarithm: for plotting

private:

    //  The reference picture to use for the follow-up curves
    int  refpic_;

    //  The smoothing coefficient for response computation
    double  smoothing_;

    //  The number of "pixels" (tupels) to select for Z-matrix generation
    int  nselect_;

    //  Method for solving the 'Ax=b' problem (AUTO, USE_QR, USE_SVD)
    ResponseSolverBase::SolveMode  solve_mode_;
    
    //  Method for generating the "Z-Matrix" (maybe temporarily)
    int  method_Z_;
    
    //  Mark "bad pixel" in the generated HDR-image by contrast colors?
    bool  mark_bad_pixel_;
    
    //  Generate while merging a bad pixel protocol to file resp. stdout?
    bool  protocol_to_file_;
    bool  protocol_to_stdout_;
      
public:
    
    //========================
    //  Concrete interface...
    //========================

    /**  Virtual Dtor  */
    virtual ~HdrCalctorBase()  {} 
    

    //================================================
    //  Hands through some `PackBase' functionality...
    
    int       size() const              {return packBase().size();}
    int       dim1() const              {return packBase().dim1();}
    int       dim2() const              {return packBase().dim2();}
    size_t    n_pixel() const           {return packBase().n_pixel();}
    int       channels() const          {return packBase().channels();}
    int       byte_depth() const        {return packBase().byte_depth();}
    int       bit_depth() const         {return packBase().bit_depth();}
    DataType  data_type() const         {return packBase().data_type();}
    StoringScheme storing() const       {return packBase().storing();}
    ImageID   imageID (int i) const     {return packBase().imageID(i);}
    int       pos_of_ID (ImageID id) const {return packBase().pos_of_ID(id);}
    
    void setExposeTime (int i, double tm)  {packBase().set_time(i,tm);}

    
    //=======================================
    //  Set and get [numerical] parameters...

    int    refpic()          const  {return refpic_;}
    void   refpic (int pic);

    double smoothing()       const  {return smoothing_;}
    void   smoothing (double sm)    {smoothing_ = sm;  }

    int    nselect()         const  {return nselect_;}
    void   nselect (int n)          {nselect_ = n;   }
    
    bool   mark_bad_pixel()  const  {return mark_bad_pixel_;}
    void   mark_bad_pixel (bool b)  {mark_bad_pixel_ = b;}
    
    bool   protocol_to_file() const  {return protocol_to_file_;}
    void   protocol_to_file (bool b) {protocol_to_file_ = b;}
    
    bool   protocol_to_stdout() const  {return protocol_to_stdout_;}
    void   protocol_to_stdout (bool b) {protocol_to_stdout_ = b;}

    ResponseSolverBase::SolveMode  solve_mode() const   {return solve_mode_;}
    void   solve_mode (ResponseSolverBase::SolveMode m) {solve_mode_ = m;}
        
    int    method_Z() const             {return method_Z_;}
    void   method_Z (int m)             {method_Z_ = m;}
    

    //=====================
    //  Follow-up Values...
    
    bool haveFollowUpCurves() const {return true; /*refpic_ == fllwUpVals().refpic();*/}
    
    
    //====================
    //  Response Curves...

    /**
    *   @brief Response curves ready?
    */
    bool isResponseReady() const;   
    bool isResponseReady (int channel) const;   

    /**
    *   @brief Return the current (last computed) response curve for 'channel'.
    */ 
    TNT::Array1D<double> getExposeVals    (int channel) const;
    TNT::Array1D<double> getLogExposeVals (int channel) const;

    /** 
    *   @brief Take over an external response curve (e.g. for pure merging) 
    */
    bool setExposeVals    (int channel, const TNT::Array1D<double> &);
    bool setLogExposeVals (int channel, const TNT::Array1D<double> &);

    
    //==========================
    //  Reporting and Listing...
      
    /** 
    *   @brief Status and debugging infos on console  
    */
    virtual void report() const;


        
    //========================
    //  Abstract interface...
    //========================

    virtual const PackBase           & packBase() const = 0;
    virtual       PackBase           & packBase() = 0;
    
    virtual const FollowUpValuesBase & fllwUpVals() const = 0;
    virtual       FollowUpValuesBase & fllwUpVals() = 0;

    
    /**
    *   Dimension of the response curve arrays. For 8-bit: dim == 256 == nzvals,
    *    for 16-bit however it can differ from nzvals==65536; e.g. dim==256 in
    *    HdrCalctor_U16_as_U8, where the response curve is represented (at the
    *    moment) by 256 values. More a temporary makeshift.
    */
    virtual int dimResponseData() const = 0;
    
   
    //=====================
    //  Follow-up Values...  
    
    /**
    *   Compute follow-up values: 
    *    The corresponding ref. picture is not given as parameter, it has to 
    *    be hold internally. In opposite to update_...() here the computation
    *    should be executed always.
    */  
    virtual void compute_FollowUpValues() = 0; 
    virtual void compute_FollowUpValues (int channel) = 0; 
    
    
    /**
    *   Update follow-up values: 
    *    Compute only if nesseccay - in fact: compute only, if `refpic' has 
    *    been changed since last computation. It is expected, that a user
    *    can call this function "formally", i.e. very often.
    */  
    virtual void update_FollowUpValues() = 0; 
    virtual void update_FollowUpValues (int channel) = 0; 

      
    //====================
    //  Response Curves...

    /**
    *   Compute the response curves...
    */
    virtual void compute_Response() = 0;             // compute all channels
    virtual void compute_Response (int channel) = 0; 


    //==========================
    //  Assemble an HDR image...

    /**
    *   Merge an HDR image or channel:
    *    Pure merging using the current response curves and weight tables. 
    *    If no response curves exist a Null-Image is returned.
    */
    virtual ImageHDR merge_HDR() = 0;
    virtual ImageHDR merge_LogHDR() = 0;

        
    /**
    *   Complete the HDR image [or channel] calculation, i.e. pure merging,
    *    if response curves already exist, else compute them before.
    */
    virtual ImageHDR complete_HDR() = 0;

    
protected:
    
    /**  
    *    Ctor - protected to avoid attempted instances of abstract class 
    */
    HdrCalctorBase(); 
    
};


}  // namespace

#endif  // HdrCalctorBase_hpp

// END OF FILE

/*
 * HdrCalctor_RGB_U8.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  HdrCalctor_RGB_U8.hpp
*/
#ifndef HdrCalctor_RGB_U8_hpp
#define HdrCalctor_RGB_U8_hpp

#include <cassert>
#include "br_types.hpp"                 // typedef uint8
#include "br_PackImgScheme2D.hpp"
#include "HdrCalctorBase.hpp"
#include "FollowUpValues_RGB_U8.hpp"
#include "WeightFunc_U8.hpp"            // WeightFunc_U8
#include "br_macros.hpp"                // DTOR()



namespace br {

using namespace TNT;

/**===========================================================================
 
  @class  HdrCalctor_RGB_U8
  
  Hdr-Calctor for \c PackImgScheme2D objects of type RGB_U8. Computation of 
   follow-up values, response curves and HDR-merging.
  
  @note
  - Kein Ctor fuer \c BrImgVector Argument anbieten, denn das hiesse, die Packs hier
   zu kreieren, doch in diese Kreation sollen ja noch eventuelle Verschiebungs-
   korrekturen einfliessen. Deshalb dies extern abhandeln. Ctor(en) also nur fuer
   fertige Packs! 
  
  - ACHTUNG: Calctor hat fuer Fortleben des Packs selbst zu sorgen, also
   Kopie des im Ctor mitgeteilten Pack's ziehen, nicht nur Referenz nehmen!
   \n
   ABER seitdem wir hier nur einen Zeiger (und kein Objekt) der Wichtungsfunktion
   halten, ist es mit der Selbstaendigkeit ohnehin vorbei. Wir sind angewiesen
   darauf, dass das externe Wichtungsfunktionsobjekt, darauf unser Zeiger verweist,
   inzwischen draussen nicht vernichtet ward. Konseqent koennte dann auf die
   (Speicher-sicherende) Pack-Kopie auch verzichtet werden.
   
  - Weil wir die WeightFunc (nach ihrer Initialisierung im Ctor) noch wechseln
   koennen wollen, koennen wir <i>keine Referenz</i> einer WeighFunc hier halten,
   sondern nur einen Zeiger. Eine Referenz ist ein Name, mit dem Initialisieren
   ist er festgelegt. Ein Kopieren auf eine Referenz, \a a=b, kopiert auf das Objekt,
   darauf die Referenz \a a verweist, und aendert nicht etwa den Namen.
   
  - Da wir in compute_Response() notfalls ein init_table() fuer die WeightFunc
   vornehmen (weil ResponseSolver die Tabelle verlangt), koennen wir die Adresse des 
   Response-WeightFunc-Objekts nicht unter einem Konstantzeiger ablegen. Demgemaess
   koennen auch die entsprechenden Ctor- und setWeightPtr()-Argumente keine 
   Konstantargumente sein -- Alternative waere nur, in compute_Response() dann
   abzubrechen. Dagegen muss das Merging-WeightFunc-Objekt durch die Klasse nicht
   veraendert werden, da genuegen Konstantzeiger.
   
  - NOTE: Die ganze "HdrCalctor"-Idee ist wohl eher ein Flop, die einzige echte Leistung
   ist das Berechnen der drei Response-Kurven in compute_Response(), alles andere
   ist Verwaltungskram. compute_Response() koennte auch eine separate Funktion sein,
   dann entfiele einiger Verwaltungsaufwand. Freilich haette Br2HdrManager dann mehr
   zu organisieren, insbesondere die Datentypabstraktion durch HdrCalctorBase
   stuende nicht mehr zu Verfuegung. Indes wird diese Datenabstraktion ja weitgehend
   durch die Basisklasse \c PackBase der Pack's geleistet. Darauf koennte der 
   Manager zurueckgehen.
      
*============================================================================*/
class HdrCalctor_RGB_U8 : public HdrCalctorBase
{
public:

    /**
    *   Type of the Pack we work up
    */    
    typedef PackImgScheme2D_RGB_U8   Pack;


private:

    //  The Pack given in the Ctor; copy!
    Pack  pack_;                          // object, no ref!

    //  Our instance to calculate follow-up values
    FollowUpValues_RGB_U8  followUp_;

    //  Pointer to our weight function for response computation
    WeightFunc_U8*  pWeightResp_;
    
    //  Pointer to our weight function for HDR-merging
    const WeightFunc_U8*  pWeightMerge_;

    //  Maximal possible z-value
    static const uint8  z_max_ = 255;

      
public:
     
    /*  Ctor... (`const Pack &' - we take a copy) */
    HdrCalctor_RGB_U8 ( const Pack &, 
                        WeightFunc_U8 &         response, 
                        const WeightFunc_U8 &   merge );
                        
    /*  Dtor */
    ~HdrCalctor_RGB_U8()                                {DTOR("");}


    //==================================================================
    //  The abstract interface of HdrCalctorBase we have to implement...
    //==================================================================
    
    int   dimResponseData() const                       {return 256;}
    
    const br::PackBase & packBase() const               {return pack_;}
          br::PackBase & packBase()                     {return pack_;}
    
    const br::FollowUpValuesBase & fllwUpVals() const   {return followUp_;}
          br::FollowUpValuesBase & fllwUpVals()         {return followUp_;}


    //==================
    //  FolloUpValues...
    
    /*  Compute always */
    void compute_FollowUpValues();
    void compute_FollowUpValues (int channel)   {compute_FollowUpValues();} 

    /*  Compute only, if `refpic' has been changed since last computation */
    void update_FollowUpValues();
    void update_FollowUpValues (int channel)    {update_FollowUpValues();} 

      
    //==================== 
    //  Response curves...  
      
    /*  Compute the response curves */
    void compute_Response();
    void compute_Response (int channel)         {compute_Response();}


    //============================
    //  Assembling an HDR image...

    /*  Merge an HDR image  -  pure merging using the current response curves!
         @return Null-Image, if none. */
    ImageHDR merge_HDR();
    ImageHDR merge_LogHDR();

    /*  Complete an HDR image creation. If response curves exist, merge only, 
         else compute them before. */
    ImageHDR complete_HDR();

    //===============================================//
    //  ...End of abstract HdrCalctorBase interface  //
    //===============================================//

    /*  Get our weight functions */
    const WeightFunc_U8 & weightResp()  const           {return *pWeightResp_;}
    const WeightFunc_U8 & weightMerge()  const          {return *pWeightMerge_;}
    
    /*  Set our weight function pointers */
    void setWeightPtrResp (WeightFunc_U8 *p)            {assert(p); pWeightResp_ = p;}
    void setWeightPtrMerge (const WeightFunc_U8 *p)     {assert(p); pWeightMerge_ = p;}
};


}  // namespace

#endif // HdrCalctor_RGB_U8_hpp
// END OF FILE

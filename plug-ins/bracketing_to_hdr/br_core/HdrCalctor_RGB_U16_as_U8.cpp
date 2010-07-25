/*
 * HdrCalctor_RGB_U16_as_U8.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  HdrCalctor_RGB_U16_as_U8.cpp
*/

#include "TNT/tnt_array1d.hpp"

#include "HdrCalctorBase.hpp"
#include "HdrCalctor_RGB_U16_as_U8.hpp"
#include "br_defs.hpp"                  // compiler switches
#include "br_types.hpp"                 // uint8, RealNum
#include "br_PackImgScheme2D.hpp"
#include "FollowUpValues_RGB_U16_as_U8.hpp"
#include "mergeHdr_PackSch2D_RGB_U16.hpp"
#include "Z_MatrixGenerator.hpp"        // Z_MatrixGenerator<>
#include "ResponseSolver.hpp"           // ResponseSolver<,>    
#include "TheProgressInfo.hpp"          // TheProgressInfo
#include "br_macros.hpp"                // CTOR()
#include "br_messages.hpp"              // v_alert()
#include "i18n.hpp"                     // macro _()



using namespace br;
using namespace TNT;

//==============================
//
//       IMPLEMENTATION
// 
//==============================
/**+*************************************************************************\n
  Ctor. 
  Wir ziehen vom Pack-Argument \a pack eine ref-zaehlende Kopie \a pack_, um die
   Daten fuer die Lebenszeit des Calctors zu sichern.
  
  Eine WeightFunc-Initialisierung mit einem Null-Zeiger ist nicht vorgesehen,
   waere desastroes, deshalb die WeightFunc-Objekte, davon wir die Adressen
   aufbewahren, als Referenzen, nicht als Zeiger zu uebergeben.
******************************************************************************/
HdrCalctor_RGB_U16_as_U8::HdrCalctor_RGB_U16_as_U8 ( 
                                        const Pack &            pack, 
                                        WeightFunc_U8 &         response,
                                        const WeightFunc_U16 &  merge )
  :
    pack_          (pack),              // copy - (buffers: ref counting)
    followUp_      (pack),              // Inits only, no computation
    pWeightResp_   (& response),
    pWeightMerge_  (& merge)
{
    CTOR("")
}



//============================================================
//  IMPLEMENTATION of the abstract HdrCalctorBase interface...
//============================================================

//===================
//  FollowUpValues...

/**+*************************************************************************\n
  Compute follow-up values for the current \a refpic_.
******************************************************************************/
void HdrCalctor_RGB_U16_as_U8::compute_FollowUpValues()
{
    followUp_.compute_z_average (refpic());
}

/**+*************************************************************************\n
  Update follow-up values, i.e. compute only if \a refpic has been changed
   since last computation.
******************************************************************************/
void HdrCalctor_RGB_U16_as_U8::update_FollowUpValues()
{
    printf("HdrCalctor_RGB_U16_as_U8::%s(): refpic=%d, last_refpic=%d\n", 
      __func__, refpic(), fllwUpVals().refpic());

    if (refpic() != fllwUpVals().refpic()) 
      compute_FollowUpValues();
}


//====================
//  Response curves...  

/**+*************************************************************************\n
  Compute all three response curves.
  
  We use reduced (==U8) follow-up values for generation of the Z-Matrix and get
   a numerical problem of the same dimension as for U8-data. The found solution
   `X[256]´ contains the associated exposures X_i to the 256 reduced z-values,
   0..255, with other words, to the original z-values 256*i+127 (middles of the
   counting intervalls).
******************************************************************************/
void 
HdrCalctor_RGB_U16_as_U8::compute_Response()
{
    const uint8  z_max_U8 = 255;      // max. possible z-value of U8 data
  
  try {
    //  Init the weight table if not yet done (needed by solver.compute())
    if (! pWeightResp_->have_table()) pWeightResp_->init_table();
  
    //  Compute the (reduced: U8) follow-up values if not yet done
    update_FollowUpValues();

    //  Init progress info
    TheProgressInfo::start (1.0, _("Computing response curves..."));
    TheProgressInfo::value (0.1);

    //  Init the Z-MatrixGenerator for our reduced (U8) follow-up data
    Z_MatrixGenerator <uint8>  generator (z_max_U8);

    //  Generate the reduced (U8) "Z-matrix" for the R-channel
    Array2D <uint8>  Z = generator.create_Z (followUp_.channelArray_R(), refpic(), nselect());

    //  Init the response solver for U8 data and compute
    ResponseSolver <uint8,RealNum>  solver (Z, pack_.logtimeVec(), z_max_U8);
    solver.solve_mode (solve_mode());   // AUTO, USE_QR, USE_SVD
   
    solver.compute (smoothing(), pWeightResp_->table());   // using U8-weight!
    logXcrv_[0] = solver.getLogExposeVals();
    Xcrv_[0] = solver.getExposeVals();
    TheProgressInfo::value (0.4);

    //  Analog for the G-channel...  
    Z = generator.create_Z (followUp_.channelArray_G(), refpic(), nselect()); 
    solver.exchange_Z_Matrix (Z);
    solver.compute (smoothing(), pWeightResp_->table());   // using U8-weight!
    logXcrv_[1] = solver.getLogExposeVals();
    Xcrv_[1] = solver.getExposeVals();
    TheProgressInfo::value (0.7);

    //  Analog for the B-channel...  
    Z = generator.create_Z (followUp_.channelArray_B(), refpic(), nselect()); 
    solver.exchange_Z_Matrix (Z);
    solver.compute (smoothing(), pWeightResp_->table());   // using U8-weight!
    logXcrv_[2] = solver.getLogExposeVals();
    Xcrv_[2] = solver.getExposeVals();
    TheProgressInfo::finish();
  }
  catch (Exception e) {
    e.report();
    //  Provisional GUI-capable message 
    v_alert(
        _("Exception while computing of response curves.\n\tProbably unsuitable image data.\n\tContext: %s"),
        e.what());
    TheProgressInfo::finish();
  }
}


//=============================
//  Assembling the HDR image...
     
/**+*************************************************************************\n
  Merge the LDR pack into a single HDR(float) Rgb image.
   @returns Null-Image, if response curves not ready.
******************************************************************************/
ImageHDR
HdrCalctor_RGB_U16_as_U8::merge_HDR()
{
    IF_FAIL_RETURN (isResponseReady(), ImageHDR());    

    return merge_Hdr_RGB_U16 (pack_, 
                              logXcrv_[0], logXcrv_[1], logXcrv_[2], 
                              *pWeightMerge_, mark_bad_pixel(),
                              protocol_to_file(), protocol_to_stdout());
}

/**+*************************************************************************\n
  Merge the LDR pack into a single logarithmic HDR(float) Rgb image.
   @returns Null-Image, if response curves not ready.
******************************************************************************/
ImageHDR
HdrCalctor_RGB_U16_as_U8::merge_LogHDR()
{
    IF_FAIL_RETURN (isResponseReady(), ImageHDR());

    return merge_LogHdr_RGB_U16 (pack_,
                                 logXcrv_[0], logXcrv_[1], logXcrv_[2],
                                 *pWeightMerge_, mark_bad_pixel(),
                                 protocol_to_file(), protocol_to_stdout());
}

/**+*************************************************************************\n
  Complete the HDR image calculation, i.e. pure merging, if response curves
   already exists, else compute them before.
******************************************************************************/
ImageHDR
HdrCalctor_RGB_U16_as_U8::complete_HDR()
{
    if (!isResponseReady()) compute_Response();
    return merge_HDR();
}

//==============================================================//
//  ...End of abstract HdrCalctorBase interface implementation  //
//==============================================================//


// END OF FILE

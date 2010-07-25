/*
 * HdrCalctorBase.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  HdrCalctorBase.cpp
*/

#include <cmath>                  // exp()
#include "HdrCalctorBase.hpp"
#include "br_enums_strings.hpp"   // DATA_TYPE_STR[] etc.


using namespace br;
using namespace TNT;

/**+*************************************************************************\n
  Ctor.   Protected to avoid attempted instances of abstract class.
******************************************************************************/
HdrCalctorBase::HdrCalctorBase() 
{
    //  Default values for `refpic_', `smoothing_', `nselect_':
    refpic_     = 0;     // Egal, wir rechnen nicht im Ctor.
    smoothing_  = 1.0;
    nselect_    = 50;    // Z_MatrixGenerator can reduce it to its own value
    
    solve_mode_ = ResponseSolverBase::AUTO;
    method_Z_   = 1;      // i.e. selection along the refpic line
    mark_bad_pixel_ = false;
    protocol_to_file_ = false;
    protocol_to_stdout_ = false;
}


//--------------------------
//  Numerical parameters...

/**+*************************************************************************\n
  Set refpic value to \a pic (as far \a pic is in range)
******************************************************************************/
void
HdrCalctorBase::refpic (int pic)
{
    if (0 <= pic && pic < packBase().size())  
      refpic_ = pic;
    
    //  What about an auto recalc of follow-up values?  Rather no!
    //update_followup();
}


//-----------------------
//  Response curves...  

/**+*************************************************************************\n
 @return  True if response curves ready, false else.
******************************************************************************/
bool
HdrCalctorBase::isResponseReady() const
{
    return ( !logXcrv_[0].is_empty() && 
             !logXcrv_[1].is_empty() &&
             !logXcrv_[2].is_empty() );
}           
             
/**+*************************************************************************\n
 @return  True if response curve for \a channel ready, false else.
******************************************************************************/
bool
HdrCalctorBase::isResponseReady (int channel) const
{
    IF_FAIL_RETURN(0 <= channel && channel < 3, false);

    return ( ! logXcrv_[channel].is_empty() );
}           


/**+*************************************************************************\n
  @return  The current (last computed) logarithm response curve for \a channel
     or a Null-Array, if \a channel out of range.

  @note  What return, if \a channel out of range? If a Null-Array, then the 
   function should return a value, not a ref, otherwise we would have to
   define anywhere a static Null-Array.
******************************************************************************/
Array1D<double>  
HdrCalctorBase::getLogExposeVals (int channel) const
{
    //  if channel out of range return Null-Array:
    IF_FAIL_RETURN(0 <= channel && channel < 3, Array1D<double>()); 
                                                
    return logXcrv_[channel];
}

/**+*************************************************************************\n
  @return  The current (last computed or external set) response curve for 
     \a channel, or a Null-Array if \a channel out of range.
******************************************************************************/
Array1D<double>  
HdrCalctorBase::getExposeVals (int channel) const
{
    //  If channel out of range return Null-Array:
    IF_FAIL_RETURN(0 <= channel && channel < 3, Array1D<double>());
                                                
    return Xcrv_[channel];
}


/**+*************************************************************************\n
  Take over an external response curve array (e.g. for merging) for \a channel.
   For \a Xcrv we allow a Null-Array to zero the current array, otherwise 
   Xcrv.dim() == dimResponseData() is requested.
  
  @return  true if successfully, false otherwise.

  QUESTION: Real copy or ref copy? If we never change in Calctor the curve 
   values  directly a'la \c Xcrv_[i]=..., but do only \c Xcrv_=tmpCrv, where
   \c tmpCrv is a separate Array1D, then a ref copy should be enough, because
   the external curve remains alive (so long an external referent exists). 
   Current state: we make a copy.
******************************************************************************/
bool 
HdrCalctorBase::setExposeVals (int channel, const Array1D<double> & Xcrv) 
{ 
    IF_FAIL_RETURN (0 <= channel, false); 
    IF_FAIL_RETURN (channel < 3, false);      // allg: channel < packBase().size()
    if (Xcrv.is_null()) 
    {
      Xcrv_[channel] = Xcrv;
      logXcrv_[channel] = Xcrv;
      return true;
    }
    IF_FAIL_RETURN (Xcrv.dim() == dimResponseData(), false);
    Xcrv_[channel] = Xcrv.copy();      // Real copy or ref copy, that's the...

    //  Fill `logXcrv_' accordingly (re-alloc if nesseccary)...
    if (logXcrv_[channel].dim() != Xcrv_[channel].dim()) 
       logXcrv_[channel] = Array1D<double> (Xcrv_[channel].dim()); 

    for (int i=0; i < Xcrv_[channel].dim(); i++)
      logXcrv_[channel][i] = log( Xcrv_[channel][i] );   
      
    return true;  
}

/**+*************************************************************************\n
  Take over an external log response curve array (e.g. for merging) for 
   \a channel. See setExposeVals().
******************************************************************************/
bool 
HdrCalctorBase::setLogExposeVals (int channel, const Array1D<double> & logX) 
{ 
    IF_FAIL_RETURN (0 <= channel, false); 
    IF_FAIL_RETURN (channel < 3, false);      // allg: channel < packBase().size()
    if (logX.is_null()) 
    {
      Xcrv_[channel] = logX;
      logXcrv_[channel] = logX;
      return true;
    }
    IF_FAIL_RETURN (logX.dim() == dimResponseData(), false);
    logXcrv_[channel] = logX.copy();   // Real copy or ref copy, that's the...

    //  Fill `Xcrv_' accordingly (re-alloc if nesseccary)...
    if (Xcrv_[channel].dim() != logXcrv_[channel].dim()) 
       Xcrv_[channel] = Array1D<double> (logXcrv_[channel].dim()); 

    for (int i=0; i < logXcrv_[channel].dim(); i++)
      Xcrv_[channel][i] = exp( logXcrv_[channel][i] );   
      
    return true;   
}


//--------------------------------
//  Reporting and debugging...

/**+*************************************************************************\n
  Print status and debugging infos on console.
******************************************************************************/
void 
HdrCalctorBase::report() const
{
    //packBase().report();
    std::cout << "HdrCalctorBase-Report:"
              << "\n  [Pack-Data...]"
              << "\n    size       : " << size()
              << "\n    dim1 x dim2: " << dim1() << " x " << dim2()
              << "  (n_pixel: " << n_pixel() << ')'
              << "\n    channels   : " << channels()
              << "\n    DataType   : " << DATA_TYPE_STR[data_type()]
              << "\n    byte_depth : " << byte_depth()
              << "   (bit_depth: " << bit_depth() << ')'
              << "\n    buffer_size: " << packBase().buffer_size()
              << "\n    storing scheme: " << STORING_SCHEME_STR[storing()]
              << "\n  [Numeric...]"
              << "\n    refpic    : " << refpic() 
              << ",  last_refpic: " << fllwUpVals().refpic()
              << "\n    smoothing : " << smoothing()
              << "\n    nselect   : " << nselect() 
              << "\n    solve mode: " << solve_mode()
              << "\n    method-Z  : " << method_Z()
              << '\n';
}


// END OF FILE

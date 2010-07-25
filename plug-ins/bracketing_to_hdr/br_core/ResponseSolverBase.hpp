/*
 * ResponseSolverBase.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  ResponseSolverBase.hpp
*/
#ifndef ResponseSolverBase_hpp
#define ResponseSolverBase_hpp


namespace br {

/**===========================================================================

  @class ResponseSolverBase
  
  Base class (non-template) of the ResponseSolver template containing some type-
   independent things, in particular enums. Because it is easier to write
   "ResponseSolverBase::AUTO" instead of "ResponseSolver<double,uint8>::AUTO".
  
=============================================================================*/
class ResponseSolverBase 
{
public:
    /** How the rectangular `Ax=b' problem shall be solved. */
    enum SolveMode
    {   AUTO      = 0,                  ///< try QR decomposition first, if rank defect use SVD
        USE_QR    = 1,                  ///< use QR, if rank defect throw exception
        USE_SVD   = 2,                  ///< use SVD (a bit faster than AUTO if QR fails)
        TEST_BOTH = USE_QR | USE_SVD    ///< solve with both and compare (test mode)
    };

    /*  Ctor */
    ResponseSolverBase (SolveMode m) : solve_mode_(m)  {}
    
    /*  Set mode for solving the rectangular 'Ax=b' problem */
    void solve_mode (SolveMode m)               {solve_mode_ = m;}
    
    /*  Return the current solve mode */
    SolveMode solve_mode() const                {return solve_mode_;}

private:
    SolveMode  solve_mode_;
};


}  // namespace "br"

#endif  // ResponseSolverBase

// END OF FILE

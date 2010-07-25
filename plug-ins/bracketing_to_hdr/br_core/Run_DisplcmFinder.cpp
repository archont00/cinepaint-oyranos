/*
 * Run_DisplcmFinder.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
/*!
  Run_DisplcmFinder.cpp
*/

#include "DisplcmFinderCorrel_RGB.hpp"  // DisplcmFinderCorrel_RGB_U8/U16
#include "br_messages.hpp"
#include "TheProgressInfo.hpp"
#include "Br2Hdr.hpp"                   // br::Br2Hdr::Instance()
#include "Run_DisplcmFinder.hpp"
#include "i18n.hpp"                     // macros _(), N_()


using namespace br;

static Br2HdrManager &  theBr2Hdr = Br2Hdr::Instance();

//============================================
//  Local helpers to centralize some messages 
//============================================
static char*  str_compute_displacements_ = N_("Compute Displacements..."); 

/*!  \a imgA and \a imgB are container positions from 0 to size-1, but we
    output numbers from 1 to size accordingly to the GUI numbering.
*/
static void errormsg_1 (int imgA, int imgB, int errorcode)
{
    v_message (_("Sorry, could not compute offset: image #%d <=> image #%d. Error %d."),
        imgA+1, imgB+1, errorcode);
}


/**+*************************************************************************\n
  Berechnet Versatz zwischen Bild \a iA und \a iB (Containerpositionen) und teilt
   diesen \a theBr2Hdr mit. Als Suchparameter werden die in der statischen
   DisplcmFinderCorrel_ Struktur \a init_params_ eingetragenen verwendet,  mit
   denen im Ctor initialisiert wird.
******************************************************************************/
void  
Run_DisplcmFinder :: run_single (int iA, int iB)
{
    if (theBr2Hdr.size() < 1)  // '1', not '2' - allows self-correlations
      return;
    
    if (iA < 0 || iB < 0 || iA >= theBr2Hdr.size() || iB >= theBr2Hdr.size())
      return;
    
    const Image &  imgA = theBr2Hdr.image (iA);
    const Image &  imgB = theBr2Hdr.image (iB);
    
    DisplcmFinderCorrel* finder;
    switch (imgA.image_type())
      {
      case IMAGE_RGB_U8:  finder = new DisplcmFinderCorrel_RGB_U8; break;
      case IMAGE_RGB_U16: finder = new DisplcmFinderCorrel_RGB_U16; break;
      default:            NOT_IMPL_IMAGE_TYPE (imgA.image_type()); return;
      }
    
    finder->find (imgA,imgB);       // the search run
    
    if (finder->error())
      errormsg_1 (iA, iB, finder->error());
    else {
      theBr2Hdr.offset (iA, iB, finder->d_result());
      theBr2Hdr.update_active_intersection();
    }
    delete finder;
}

/**+*************************************************************************\n
  Berechnet fuer alle Bilder des Containers ihre Verrueckung zum Nachbarbild und
   teilt diese \a theBr2Hdr mit. Als Suchparameter werden die in DisplcmFinderCorrel_'s
   statischer \a init_params_ Struktur eingetragenen verwendet, mit denen im Ctor
   initialisiert wird.
  
  FRAGE: Soll bei einen Finder-Fehler die offset_ID genullt werden oder sollte
   der alte Offset-Wert bleiben? Zur Zeit bleibt er.
  
  In \c find(A,B) ist A das bewegliche Gebiet und B das Suchgebiet und das
   Resultat \a d zu lesen als: Den Offset \a d <b>addiere bei B</b> (im Indexsinne
   B[j+d]). Fuer BrImgVector wurde wiederum vereinbart, dass ein in Bild \a i+1
   hinterlegter Offset relativ zum Vorgaengerbild \a i gelte und <b>am Bild i+1 zu
   \a addieren </b> sei. Daher hat nach einem Aufruf \c find(i,i+1) an 
   BrImgVektor die Mitteilung \c offset(i,i+1,d) zu ergehen.
******************************************************************************/
void
Run_DisplcmFinder :: run_for_all()
{
    if (theBr2Hdr.size() < 2) return;
    
    DisplcmFinderCorrel* finder;
    switch (theBr2Hdr.imgVector().image_type()) 
      {
      case IMAGE_RGB_U8:  finder = new DisplcmFinderCorrel_RGB_U8; break;
      case IMAGE_RGB_U16: finder = new DisplcmFinderCorrel_RGB_U16; break;
      default:            NOT_IMPL_IMAGE_TYPE (theBr2Hdr.imgVector().image_type());
                          return;
      }
    
    int error = 0;
    TheProgressInfo::start (1.0f, _(str_compute_displacements_));
    for (int i=0; i < theBr2Hdr.size()-1; i++) {
      
      finder->find (theBr2Hdr.image(i), theBr2Hdr.image(i+1));
      TheProgressInfo::forward (1.0 / (theBr2Hdr.size() - 1));
      
      if (finder->error()) {
        error |= 1;
        errormsg_1 (i, i+1, finder->error());
      }
      else
        theBr2Hdr.offset (i, i+1, finder->d_result());
    }
    TheProgressInfo::finish();
    theBr2Hdr.update_active_intersection();
    delete finder;
}

/**+*************************************************************************\n
  Berechnet fuer alle \a aktivierten Bilder des Containers ihre Verrueckung zum
   naechsten aktiven Bild und teilt diese \a theBr2Hdr mit. Als Suchparameter
   werden die in DisplcmFinderCorrel_'s statischer \a init_params_ Struktur
   eingetragenen verwendet, mit denen im Ctor initialisiert wird.
  
  FRAGE: Soll bei einen Finder-Fehler die offset_ID genullt werden oder sollte
   der alte Offset-Wert bleiben? Zur Zeit bleibt er.
  
  In \c find(A,B) ist A das bewegliche Gebiet und B das Suchgebiet und das
   Resultat \a d zu lesen als: Den Offset \a d <b>addiere bei B</b> (im Indexsinne
   B[j+d]). Fuer BrImgVector wurde wiederum vereinbart, dass ein in Bild \a k
   hinterlegter Offset relativ zum Vorgaengerbild \a i<k gelte und <b>am Bild \a k
   zu \a addieren </b> sei. Daher hat nach einem Aufruf \c find(A,B) an 
   BrImgVektor die Mitteilung \c offset(A,B,d) zu ergehen.
******************************************************************************/
void  
Run_DisplcmFinder :: run_for_all_actives()
{
    int  n_active = theBr2Hdr.imgVector().size_active();
    if (n_active < 2) return;
    
    int  last = theBr2Hdr.imgVector().last_active();
    
    DisplcmFinderCorrel* finder;
    switch (theBr2Hdr.imgVector().image_type())
      {
      case IMAGE_RGB_U8:  finder = new DisplcmFinderCorrel_RGB_U8; break;
      case IMAGE_RGB_U16: finder = new DisplcmFinderCorrel_RGB_U16; break;
      default:            NOT_IMPL_IMAGE_TYPE (theBr2Hdr.imgVector().image_type());
                          return;
      }
    
    int error = 0;
    TheProgressInfo::start (1.0f, _(str_compute_displacements_));
    for (int i=0; i < last; i++) {
      if (! theBr2Hdr.image(i).active()) continue;
      
      int  next = theBr2Hdr.imgVector().next_active(i); // i is active!
      
      finder->find (theBr2Hdr.image(i), theBr2Hdr.image(next));
      TheProgressInfo::forward (1.0 / (n_active - 1));
      
      printf ("Offset of #%d to #%d: (%d,%d).  Error=%d\n", next, i,
          finder->d_result().x,  finder->d_result().y, finder->error());
      
      if (finder->error()) {
        error |= 1;
        errormsg_1 (i, next, finder->error());
      }
      else
        theBr2Hdr.offset (i, next, finder->d_result());
    }
    TheProgressInfo::finish();
    theBr2Hdr.update_active_intersection();
    delete finder;
}


// END OF FILE

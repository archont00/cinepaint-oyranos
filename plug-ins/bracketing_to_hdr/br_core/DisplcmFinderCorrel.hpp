/*
 * DisplcmFinderCorrel.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  DisplcmFinderCorrel.hpp
*/
#ifndef DisplcmFinderCorrel_hpp
#define DisplcmFinderCorrel_hpp


#include "DisplcmFinderCorrel_.hpp"   // DisplcmFinderCorrel_

/**===========================================================================
  
  @class DisplcmFinderCorrel

  Waehrend DisplcmFinderCorrel_ noch frei von br::Image Abhaengigkeiten, kommt
   hier eben jene hinzu: Wir definieren eine oeffentliche find() Funktion fuer
   br::Image Argumente, womit die geerbte DisplcmFinderCorrel_::find() passend
   beschickt wird. Die Klasse ist nach wie vor abstrakt (Ctor protected), weil
   DisplcmFinderCorrel_'s abstrakte new_search_tool() auch hier noch nicht
   definiert ist.
   
  Problem der Speicher-Sperrung: Ein ref-zaehlendes Haendel innerhalb new_search_tool()
   waere sinnlos, weil nach Verlassen der Funktion sofort wieder zerstoert -- 
   CorrelMaxSearch muesste es sich kopieren, was es derzeit nicht tut. Zudem inzwischen
   durch die VoidScheme2D-Schnittstelle sowieso kaum mehr moeglich. So muss
   die new_search_tool() benutzende Stelle fuer die Sperrung sorgen. Diese ist
   find(). Da deren oeffentlich zugaengliche Huelle mit br::Image-Argumenten
   hier definiert wird, kann diese Sperrung hier erfolgen durch Halten von 
   Haendels waehrend der Ausfuehrung der find()-Basisroutine. Oder die den
   Finder nutzende hoehere Stelle sorgt fuer die Sperrung. Sinnlos hingegen waere
   im hiesigen Ctor br::Image Haendel zu nehmen, denn die Image-Argumente fuer
   find() koennen ganz andere sein!
  
=============================================================================*/
class DisplcmFinderCorrel : public DisplcmFinderCorrel_
{
  public:
    int find                  ( const br::Image & imgA,
                                const br::Image & imgB )
      {
//         /*  Hold data-locking handles while running of find() */
//         br::Image  A (imgA);
//         br::Image  B (imgB);
       
        return DisplcmFinderCorrel_::find
              ( VoidScheme2D (imgA.dim1(), imgA.dim2(), imgA.buffer()),
                VoidScheme2D (imgB.dim1(), imgB.dim2(), imgB.buffer()) );
      }
  
  protected:
    DisplcmFinderCorrel()  {}
};


#endif  // DisplcmFinderCorrel_hpp

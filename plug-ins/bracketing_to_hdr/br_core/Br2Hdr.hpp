/*
 * Br2Hdr.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file  Br2Hdr.hpp
*/
#ifndef Br2Hdr_hpp
#define Br2Hdr_hpp


#include "Br2HdrManager.hpp"


namespace br {

/**===========================================================================

  @class Br2Hdr  
  
  Class allowing only a singular (solitary) \c Br2HdrManager instance. If we
   use the Br2Hdr class (and not Br2HdrManager) for instantiations, we can refer
   to any Br2Hdr object only via Br2Hdr::Instance() - and this object is singular.

  Der eigentliche Grund fuer diese Singulaerinstanzklasse war, die Arbeit mit FLUID
   zu erleichtern. Der Versuch, die GUI relativ zu einer variablen Br2HdrManager
   Instanz zu kodieren (was theoretisch mehrere GUI-komplettierte <tt>Manager</tt>-Instanzen
   in einem Programm ermoeglichte), gestaltete sich in FLUID recht beschwerlich. Der 
   Hauptgrund war (ist), dass Klassen, die mittels FLUIDs visuellem Editor "wie Widgets"
   lanciert werden sollen, einen Widget-artigen Ctor "(x,y,w,h,lable)" besitzen
   muessen. Soll eine solche Widget-Klasse Bezug auf eine [<tt>Br2HdrManager</tt>-]
   Datenstruktur nehmen, kann diese daher nicht im Ctor a'la "(x,y,w,h,lable, data)"
   mitgeteilt werden, sondern es muss ein Datenzeiger anschliessend z.B. in FLUIDs
   ExtraCode initialisiert werden. (Was nebenbei bedeutet, dass in der Klasse mit
   Zeigern statt mit Referenzen hantiert werden muss.) Hinzu kommt das Einloggen
   in Distributoren des bzgl. <tt>Managers</tt>, was ebenfalls in FLUID zu notieren waere.
   Weil ich den FLUID-Code kurz halten wollte, und es im Programm ohnehin nur
   eine <tt>Manager</tt>-Instanz gibt, stellte ich den GUI-Code von vorherein auf eine
   singulaere, globale <tt>Manager</tt>-Instanz ab. Separat kodierte Widget-Klassen koennen
   jetzt direkt darauf Bezug nehmen. (Eine Moeglichkeit, dennoch mit variabler
   <tt>Manager</tt>-Instanz den FLUID-Aufwand ertraeglich zu halten, waere, dass solche 
   Widget-Klassen ihre Initialisierungen in einer Funktion extra_init() sammeln,
   die in FLUIDs ExtraCode-Feld einzutragen waere.) Die Singularitaet der 
   <tt>Manager</tt>-Instanz verbindlich festzuschreiben jedenfalls dient 
   <tt>Br2Hdr</tt>.
   
   Insbesondere das Ein- und Ausloggen in <tt>Manager</tt>-Distributoren
   vereinfacht sich jetzt dergestalt, dass dafuer die Klassen \c EventReceiver
   und \c RefpicReceiver bereitstehen: Eine Widget-Klasse oder eine Widgethuellklasse
   hat sich nur von solchem \c Receiver abzuleiten und muss nur noch die
   handle()-Funktion ueberschreiben. (Auch Widget-Klassen koennen hier von
   \c Receivern abgeleitet werden, weil deren Ctoren ohne Argument auskommen
   (der bezuegliche <tt>Manager</tt> steht ja fest, es ist der solitaere). Die
   \c Receiver loggen sich in ihrem Ctor selbstaetig in die Distributoren des
   solitaeren Br2Hdr-Objekts ein, und im Dtor wieder aus.
      
  <b>Falle - obsolet, seit (v0.6.1) Cursor-Funktionen aus Manager entfernt:</b>
   \c Br2HdrManager enthaelt abstrakte GUI-Methoden, die in \c Br2HdrManagerGui
   implementiert werden. Der Einfall, \c Br2Hdr statt von Br2HdrManagerGui
   von Br2HdrManager abzuleiten, und die GUI-Komplettierung dafuer in einer von
   Br2Hdr abgeleiteten Klasse <tt>Br2HdrGui</tt> vorzunehmen, scheitert daran,
   dass dann in Br2Hdr::Instance() ein <tt>Br2HdrManager</tt>-<i>Objekt</i> (und
   kein ...Gui-Objekt) instanziiert wuerde, d.h. auch <tt>Br2HdrGui::Instance()</tt>
   wuerde ein <tt>Br2HdrManager</tt>-<i>Objekt</i> referenzieren und saemtliche
   Ergaenzungen von <tt>Br2HdrGui</tt> waeren unwirksam.
   
   Eine Alternative waere nur, in Br2HdrManager keine virtuellen Funktionen,
   sondern Funktionszeiger zu benutzen, welche Zeiger vom GUI-Kunden an geeigneter
   Stelle, z.B. in AllWindows.cpp geeignet zu setzen waeren. Dann koennten
   Br2Hdr.hpp, EventReceiver.hpp, RefpicReceiver.hpp in ./br_core/ siedeln, sie
   waeren GUI-unabhaengig.

*============================================================================*/
class Br2Hdr : public Br2HdrManager
{
public: 
    static Br2Hdr & Instance()
      {
        static Br2Hdr instance_;
        return instance_;
      }
    
private:
    /*  Private Ctor: allows instantiations only from inside the class! */
    Br2Hdr()  {}
};


}  // namespace

#endif  // Br2Hdr_hpp

// END OF FILE

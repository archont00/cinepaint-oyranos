/*
 * WidgetWrapper.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file WidgetWrapper.hpp"
  
  FRAGE 1: Wie ein Widget aus *seiner* Callback-Funktion heraus loeschen?  
  
  ANTWORT: 
   In der callback-Funktion mittels delete_widget(o). Aber:
    - Vorher aus seiner Gruppe auskoppeln, sonst Speicherverletzung!
    - Vorher durch hide() unsichtbar machen, sonst bleibt Abdruck stehen.
      (Moeglich auch ein redraw() des Eltern-Widgets.)
    - Dann das `delete_widget(o)', welches Widget in ein Array zu loeschender
      Widget eintraegt und im naechsten fl_wait()-Aufruf (also nach jeder Triggerung
      durch ein GUI-Event) die Speicherfreigabe vollzieht.
     
  FRAGE 2: Wie eine Windowhuellklasse, die ein Fl_Window als Element enthaelt, aus
   der Callback-Funktion des Windows heraus vom Heap nehmen? Genauer formuliert:
   Wir haben i.d.R. ein primaeres Window, dessen hide() die Application beendet und
   das bzw. dessen Huellklasse nicht eigens vom Heap zu nehmen waere, und daneben haben
   wir sekundaere Windows, die bzw. deren Huellklassen temporaer erzeugt und auch wieder
   zerstoert werden koennen sollen ueber deren Callbacks. Fuer letztere steht die obige
   Frage. Wobei wir gleich allgemein zu Huellklassen beliebiger Widgets uebergehen
   duerfen.
   
  ANTWORT: Wir adaptieren das FLTK-Verfahren aus "Fl.cxx" des zunaechst Eintragens
   in eine Liste zu loeschender Objekte via `delete_widget()' und der eigentlichen
   Freigabe in der wait()-Schleife durch eine Funktion `do_widgets_deletion()'.
*/
#ifndef WidgetWrapper_hpp
#define WidgetWrapper_hpp


/**=====================================================================
  @class WidgetWrapper
  
  Base class for widget wrapper classes, which shall have the possiblity
   of deletion from inside of the widget's callback.
  
  PROBLEM: Gewoehnlich wird das Objekt einer dynamisch erzeugten Huellklasse
   `A' ueber einen aufbewahrten Zeiger angesprochen, "A* a = new A", welche
   Zeigervariable `a' also dem erzeugten A-Objekt aeusserlich. Oft wird
   maximal ein A-Objekt im Programm gebraucht und in Funktionen wie 
    <pre>
       do_myA() {if (!a) a=new A; a->window()->show();} </pre>
        
   bei Bedarf erzeugt. Der Zeiger `a' muss dazu aktuell gehalten werden,
   insbesondere Null gesetzt, wenn das Objekt zerstoert wird! Wie dieses
   Nullsetzen aber allgemein bewerkstelligen, wenn das Zerstoeren in einer vorab
   gefertigten Funktion WidgetWrapper::do_wrappers_deletion() erfolgt?
  
  Eine Variante waere, in der Funktion delete_wrapper() nicht nur die Adresse
   des freizugebenen Objektes zu uebergeben, sondern auch noch die Adresse der
   in do_wrappers_deletion() Null zu setzenden externen Zeigervariable `a', 
    <pre>
       delete_wrapper (WidgetWrapper *, WidgetWrapper **); </pre>
       
   Allerdings wird delete_wrapper() in der Callback-Funktion des Widgets bemueht,
   d.h. dort muesste die externe Variable `a' schon bekannt sein. Ueblicherweise
   moechte man aber eine Wrapper-Klasse unabhaengig von ihrem spaeteren Gebrauch
   kodieren, unabhaengig insbesondere von der Frage, ob ein solches Objekt 
   spaeter dynamisch erzeugt wird und durch welche Variable dessen Adresse 
   eventuell aufbewahrt wird. Ungenuegend!
   
  Der Zeitpunkt, wo `a' natuerlicherweise bekannt wird, ist der Moment der 
   dynam. Erzeugung, ist die Funktion do_myA(). Der Gedanke daher, fuer jedes
   dynamisch kreierte A-Objekt die Adresse einer Zeigervariable bei
   WidgetWrapper registrieren zu lassen, die in do_wrappers_deletion() dann Null
   gesetzt wuerde. Wie aber, wenn noch weitere Aufraeumarbeiten zu erledigen?
   Allgemeiner lassen wir daher eine Aufraeumfunktion registrieren.
   Man beachte, dass diese Dinge niemals durch den A-Dtor zu erledigen sind,
   denn es sind Dinge, die `A' aeusserlich. Man muss, genauer nun, neben der
   cleanup()-Funktion stets noch die Adresse des A-Objektes mitteilen,
   denn in do_wrappers_deletion() ist zu jedem per delete_wrapper() zur Loeschung 
   angemeldeten A-Objekt die ZUGEHOeRIGE cleanup-Funktion herauszusuchen. Um
   ferner als cleanup-Funktionen auch Elementfunktionen einer Klasse zu ermoeglichen
   -- ueber den Umweg einer statischen Element-Funktion -- ist bei der Registrierung
   noch ein `void* userdata' vorzusehen, was dann der this-Zeiger der die 
   cleanup-Fkt beherbergenden Klasse sein kann. Natuerlich koennen auch Adressen
   anderer Datenstrukturen darin mitgeteilt werden. Schliesslich bemerken wir, dass
   Dinge VOR dem Freigeben von `a' (Retten mancher a-Inhalte) und andere
   DANACH zu erledigen sein koennten. Das stellt anheim, zwei Funktionen zur Registratur
   anzubieten: backup() fuer das DAVOR und cleanup() fuer das DANACH. Sinnigerweise
   kann das Nullsetzen der externen Variable `a' genausogut in backup() erfolgen,
   so dass cleanup() im Moment gar nicht gebraucht wird.
   
   Ein Nachteil mag sein, dass die Felder \a backups_ und \a cleanups_ der registrierten
   Aufraeum-Funktionen auf weit mehr Eintraege anwachsen koennen als das Feld \a dwrappers_
   der zur Loeschung angemeldeten WidgetWrapper-Objekte.
   
   Der typische Gebrauch sieht nun so aus:
<pre>
   // The wrapper class: derived from WidgetWrapper...
   class MyWrapper : public WidgetWrapper {
     Fl_Window* window_;                     // widget (window) as element 

     cb_window_i (Fl_Window* o)  {           // callback of the widget (window)
       o->hide(); 
       WidgetWrapper::delete_wrapper (this); // register MyWrapper for deletion
     }
     [...]
   };

   // An appl. class maintaning a `MyWrapper' object and the FLTK main loop...
   class Appl {
     MyWrapper*  mywrap = 0;  // symbolic for 0-initialisation 
        
     // handle creation and registration of `mywrap'...
     void do_mywrap() {
       if (!mywrap) {
         mywrap = new MyWrapper;
         WidgetWrapper::register_cleanup_func (mywrap, cleanup_, this);
       }
       mywrap->window()->show();
     }

     // The cleanup function(s) for `mywrap'...
     static void cleanup_(void* I)  {((Appl*)I)->cleanup_i();}
     void        cleanup_i()        {mywrap = 0;}

     // The FLTK main loop over the live span of the main window...
     void run() {
       while (mainwindow->shown()) {
         Fl::wait();
         WidgetWrapper::do_wrappers_deletion();   // deletes `mywrap' and sets mywrap=0
       }
     }
     [...]
   };
 </pre>
   
  @note 
  - Bei der Namenswahl der oeffentlichen Funktion bedenken, dass Klasse als
   Basisklasse vorgesehen und Namen genuegend spezifisch sein sollten, um nicht
   zu kollidieren.
   
  - Statt in do_wrappers_deletion() die zugehoerige backup- und/oder cleanup-Funktion
   herauszusuchen, koennte das auch in delete_wrapper() erfolgen. Statt des
   `dwrappers_'-Feldes von WidgetWrapper* braeuchte es dann eines von Tripeln 
   (WidgetWrapper*, BackupFunc*, CleanupFunc*).
   
  - Und wie, wenn in do_myA() einfach davon ausgegangen wuerde, dass sie nur
   aufgerufen wird, wenn ein Objekt zu erzeugen, dh. die Abfrage "if (!a)"
   entfiele? Gefaehrlich, wuerde bei jedem falschen Aufruf von do_myA()
   Speicherleichen hinterlassen!
          
=======================================================================*/
class WidgetWrapper
{
public:
    /**  Type of the backup functions to register */
    typedef  void (BackupFunc) (void*);
    
    /**  Type of the cleanup functions to register */
    typedef  void (CleanupFunc) (void*);
    
    /**  A stored backup entry of object pointer + backup function + userdata */
    struct BackupEntry {
      WidgetWrapper* wr; 
      BackupFunc* func;
      void* userdata;
      BackupEntry (WidgetWrapper* w=0, BackupFunc* f=0, void* u=0) 
        : wr(w), func(f), userdata(u)  {}
    };
    
    /**  A stored cleanup entry of object pointer + cleanup function + userdata */
    struct CleanupEntry {
      WidgetWrapper* wr; 
      CleanupFunc* func;
      void* userdata;
      CleanupEntry (WidgetWrapper* w=0, CleanupFunc* f=0, void* u=0) 
        : wr(w), func(f), userdata(u)  {}
    };

private:
    static int              num_dwrappers_;         // number of registred wrappers
    static int              num_alloc_dwrappers_;   // size of array for registration
    static WidgetWrapper**  dwrappers_;             // array for registration
    
    static int              num_backups_;           // number of registred backups
    static int              num_alloc_backups_;     // size of array for registration
    static BackupEntry*     backups_;               // array for registration
    
    static int              num_cleanups_;          // number of registred cleanups
    static int              num_alloc_cleanups_;    // size of array for registration
    static CleanupEntry*    cleanups_;              // array for registration
public:
    WidgetWrapper() {}
    virtual ~WidgetWrapper() {}                     // `virtual' obligatory!
     
    static void delete_wrapper (WidgetWrapper*);    // register a wrapper object for deletion
    static void do_wrappers_deletion();             // delete all registered objects
    
    static void register_backup_func  (WidgetWrapper* wr, BackupFunc* f, void* udata=0);
    static void register_cleanup_func (WidgetWrapper* wr, CleanupFunc* f, void* udata=0);
private:
    static void condense_backups_();
    static void condense_cleanups_();
};


#endif  // WidgetWrapper_hpp
// END OF FILE

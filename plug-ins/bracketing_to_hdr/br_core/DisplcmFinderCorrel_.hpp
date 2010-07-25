/*
 * DisplcmFinderCorrel_.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  DisplcmFinderCorrel_.hpp
*/
#ifndef DisplcmFinderCorrel__hpp
#define DisplcmFinderCorrel__hpp


#include "DisplcmFinderBase.hpp"        // DisplcmFinderBase
#include "CorrelMaxSearchBase.hpp"      // CorrelMaxSearchBase
#include "Scheme2D.hpp"                 // VoidScheme2D


/**===========================================================================
  
  @class DisplcmFinderCorrel_
  
  Abstrakte Basisklasse der Finder-Klassen auf CorrelMaxSearch-Basis, d.h. auf
   Basis eines skalaren Korrelationskoeffizienten, fuer den die Farbvektoren
   skalar multipliziert werden. Klasse ist unabhaengig vom Typ der Bilddaten
   (U8/U16) oder ihrer Speicherung (RGB,...), daher hier auch keine Abhaengigkeit
   von br::Image hineingebracht (numerische Kernklassse). (Erst in DisplcmFinderCorrel
   kommt br::Image dazu.) Klasse bietet in der Funktion find() eine datentyp-abstrahierte
   Bestimmung eines Versatzes zwischen zwei (abstrakten) Void-Felder, bei der nur
   auf Feldindizes und das Float-Ergebnis eines Korrelationskoeffizienten Bezug
   genommen wird. An oeffentlichen Funktionen wird errechnete Versatz in d_result()
   zurueckgeliefert sowie ein Fehlercode in error(). Ausserdem gibt es Funktionen
   zum Setzen und Lesen gewisser Suchparameter.
   
  Die datentypspezifischen Erben haben die rein-virtuelle new_search_tool() Funktion
   zu definieren, die ein dynamisches CorrelMaxSearch-Objekt erzeugt und den
   Basiszeiger zurueckliefert. Mit diesem Zeiger arbeitet hier find(). Genauer
   gesagt, find(const VoidScheme2D& A, const VoidScheme2D& B) erwartet, dass der
   Aufruf \c new_search_tool(A,B) mit also genau den find() uebergebenen VoidScheme2D
   Argumenten \a A,B ein passend zu diesen Feldstrukturen initialisiertes
   CorrelMaxSearch-Objekt erzeugt.
  
  <b>Prinzip der Zusammenarbeit von \a DisplcmFinderCorrel_ und \a CorrelMaxSearch_RGB<>:
  </b> 
   Das zugrundeliegene allg. Problem ist dieses: Wir haben ein Algorithmus-Template
   \c Algo<T>, dazu vorgesehen, es fuer verschiedene Datentypen T = U8,U16... 
   zu tun, dessen Ergebnisse aber \a T-unabhaengig sind (eine Verschiebung in 
   Integer-Zahlen, ein Korrelationskoeffizient als Float-Zahl). Daneben haben wir
   eine Klasse \c AlgoUser, die ein Algo-Objekt auf abstrakte Weise benutzt, naemlich
   nur auf die Ergebnis-Typen Bezug nimmt, nicht aber auf den T-Typ in Algo.
   Wie das beschreiben? Sowar so beschreiben, dass der AlgoUser-Code nur einmal
   vorhanden ist und nicht a'la AlgoUser_U8, AlgoUser_U16 mehrfach? Meine Loesung:
   
   Algo<> bekommt eine T-unabhaengige abstrakte Basisklasse \c AlgoBase vorgeschaltet,
   die die T-unabhaengige Schnittstelle von Algo bereitstellt. AlgoUser wiederum
   bekommt eine abstrakte Funktion `AlgoBase* new_Algo(...)', mit der die Erzeugung
   eines dynamischen Algo-Objektes passender Bauart abstrakt beschrieben wird,
   und die einen AlgoBase-Zeiger zurueckliefert. AlgoUser kann sich damit ein
   Algo-Objekt erzeugen und arbeitet damit abstrakt ueber dessen Basiszeiger. Die
   Konkretisierungen von AlgoUser zu AlgoUser_U8, AlgoUser_U16 haben dann nur
   noch new_Algo() passend, d.h. T-spezifisch zu definieren.
   
   Der unschoene Punkt ist sicher die "abstrakte" Beschreibung der Erzeugung eines
   passenden Algo-Objektes mittels new_Algo(...). Ein 2D-Array von T-Elementen
   kann da nur durch einen void*-Zeiger und die Felddimensionen (dim1, dim2,
   stride) beschrieben werden, zusammengefasst in der Struktur VoidScheme2D. 
   Die Typkontrolle wird unterbrochen, auch jedes Referenzzaehlen bzgl. des
   Feldes nahezu unmoeglich gemacht.
      
  So ist also der Stand. new_search_tool() entspricht new_Algo() und find()
   nutzt diese Funktion. Die VoidScheme2D-Argumente von new_search_tool()
   beschreiben abstrakt zwei 2D-Arrays von T-Elementen, genauerhin jetzt von
   Rgb<U8> oder Rgb<U16> Elementen, welche Felder dabei durchaus auch Subarrays
   (Parameter stride!) sein duerfen.
   
  @internal Warum drei Parametersaetze statt nur \a param_ und \a default_param_? 
   Weil ich eine permanente Instanz im Programm brauchte, die ueber ein Menue
   eingestellte Parameter behaelt, auch wenn das Menue danach wieder vernichtet
   ward. Das erledigt der statische Parametersatz \a init_param_. Die Alternative
   waere gewesen, das Menueobjekt permant zu halten, was die Sache schief darstellte.
   Anderseits, auf eine instanzspezifische Einstellmoeglichkeit ganz zu verzichten
   und nur die statischen zu haben, waere zu unflexibel. Daher drei: Die Menues setzen
   \a init_params_, im CTOR wird \a params_ mit \a init_params_ initialisiert,
   und die Parametersetzfunktionen aendern das individuelle \a params_. 
   
=============================================================================*/
class DisplcmFinderCorrel_ : public DisplcmFinderBase
{
  public:
    struct Params {                     // Search parameters:
        Vec2_int  start_search_size;    // start dimensions of the search areas
        Vec2_int  start_correl_size;    // start dimensions of the correlation areas
        int       max_num_areas_1d;     // max number of search areas in each dimension
        int       subtr_search_dim;     // subtract this from search dims after each pass
        int       add_correl_dim;       // add this to correl dims after each pass
        float     min_rho_accepted;     // minimum accepted correlation coeff.
        int       verbose;              // verbositiy level (0,1,2)
    };

  private:
    const static Params  default_params_;
    static Params  init_params_;        // the CTOR init parameters
    
    Params    params_;                  // the instance-specific parameters
    Vec2_int  d_result_;                // the computed displacement result
    int       error_;                   // the error code
    
  public:
    virtual ~DisplcmFinderCorrel_() {}  // virtual Dtor fuer alle Faelle
  
    //------------------------
    //  result and error code
    //------------------------
    Vec2_int d_result() const                   { return d_result_; }
    int error() const                           { return error_;    }

    //--------------------------------
    //  parameter setting and reading 
    //--------------------------------
    static const Params & default_params()      { return default_params_;           }
    
    static const Params & init_params()         { return init_params_;              }
    static void init_params (const Params & prm){ init_params_ = prm;               }
    static Params & ref_init_params()           { return init_params_;              }
    
    const Params & params() const               { return params_;                   }
    void      params (const Params & prm)       { params_ = prm;                    }
    void      set_default_params()              { params_ = default_params_;        }
    void      set_init_params()                 { params_ = init_params_;           }

    Vec2_int  start_search_size() const         { return params_.start_search_size; }
    void      start_search_size (Vec2_int v2)   { params_.start_search_size = v2;   }
    Vec2_int  start_correl_size() const         { return params_.start_correl_size; }
    void      start_correl_size (Vec2_int v2)   { params_.start_correl_size = v2;   }
    int       max_num_areas_1d() const          { return params_.max_num_areas_1d;  }
    void      max_num_areas_1d (int n)          { params_.max_num_areas_1d = n;     }
    int       subtr_search_dim() const          { return params_.subtr_search_dim;  }
    void      subtr_search_dim (int d)          { params_.subtr_search_dim = d;     }
    int       add_correl_dim() const            { return params_.add_correl_dim;    }
    void      add_correl_dim (int d)            { params_.add_correl_dim = d;       }
    float     min_rho_accepted() const          { return params_.min_rho_accepted;  }
    void      min_rho_accepted (float f)        { params_.min_rho_accepted = f;     }
    int       verbose() const                   { return params_.verbose;           }
    void      verbose (int v)                   { params_.verbose = v;              }
    
  protected:
    //---------------
    //  Default Ctor  -  protected to avoid attempted instances
    //---------------
    DisplcmFinderCorrel_();
    
    //--------------------------
    //  Runs the maximum search
    //--------------------------
    int find                  ( const VoidScheme2D & schemeA,
                                const VoidScheme2D & schemeB );
    
    //------------------------------------------
    //  Abstract: creates a dynamic search tool 
    //------------------------------------------
    virtual CorrelMaxSearchBase* new_search_tool
                              ( const VoidScheme2D & schemeA,
                                const VoidScheme2D & schemeB ) = 0;
    
    //-------------------------------------------------------------------------
    /*! @internal Private Hilfsklasse zur bequemeren Haltung des von new_search_tool()
         gelieferten SearchTool-Zeigers mit automatischer Freigabe am Bereichsende.
         Erspart das jedmalige `delete...' bei einem vorzeitigen return. Weil 
         new_search_tool() nicht hier selbst aufrufbar (kein Objekt dazu), ist der
         von dort gelieferte Zeiger hier im Ctor zu uebergeben. Ueber die Funktionssyntax
         hat man Zugriff dann zu den search_tool-Elementen. */
    class Tool {
        CorrelMaxSearchBase*  tool_;
      public:  
        Tool (CorrelMaxSearchBase* p)           { tool_ = p; }
        ~Tool()                                 { delete tool_; }
        CorrelMaxSearchBase & operator()()      { return *tool_; }
    };
    //-------------------------------------------------------------------------
};


#endif  // DisplcmFinderCorrel__hpp

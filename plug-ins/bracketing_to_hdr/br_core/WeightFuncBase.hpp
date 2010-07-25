/*
 * WeightFuncBase.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file  WeightFuncBase.hpp
  
  Contents:
   - WeightFuncBase        --  base class (non-template)
   - WeightFunc<>          --  template for weight function objects
   - WeightFunc_Triangle<> --  template of triangle functions (unused)
  
  FRAGE: Brauchen wir ueberhaupt den Template-Parameter `Unsign'? Kommt zum
   Einsatz nur im Ctor als Typ des `zmax'-Arguments und in den Op-Funktionen
   <pre>
      Tweight operator() (Unsign z) {...}
      Tweight operator[] (Unsign z) {...} </pre>
   Nur letzteres echter Grund. Alternative waere, ein festes `int' oder 
   `uint'-Argument. Bietet das Unsign irgendeinen Vorteil? Unsere
   TNT::Array1D-Tabelle nimmt int als Argumet der []-Operatoren, dh. hier
   findet sowieso Konvertierung Unsign->int statt. Aber bei Typmischmasch
   wird gewarnt, vor allem wichtig fuer die Array-Aufrufe `[i]´ mit einem 
   Typ(i) > T. Aber ist dieses Konvertieren numerisch nicht ganz schlecht?
   Bietet es einen Nachteil? 
   
  FRAGE: Wozu die Basisklassen vor den eigentlichen U8/U16-Klassen? Eine abstrakte,
   U8-U16-uebergreifende Verwendung mit virtuellen Funktionen gibt es bisher nicht
   und der wiederverwendbare Code ist nicht der Rede Wert. Es gibt allein 
   uebergreifenden Gebrauch des Shape-Enums in Menues. Reicht das? (Enum koennte auch
   ausserhalb definiert werden.)
  
  @todo FRAGE: Laesst sich die virtuelle Funktion vermeiden durch statische
    Templates?
*/
#ifndef WeightFunc_hpp
#define WeightFunc_hpp


#include "TNT/tnt_array1d.hpp"


/**
  Data type of the weight values...
   
  Welchen Typ soll `Tweight' haben?
   Massiv gebraucht wird Wichtungstabelle in merge_HDR(). Dort wird zunaechst
   in double summiert und erst dann auf float (HDR) reduziert. Legte double
   nahe, float ginge aber auch, wuerde eben zuvor in double konvertiert. Ist
   noch die Platzfrage, die bei 256 Werten unkritisch, bei 65536 Werten 
   (16 Bit) aber bedenkenswert (256 KB vs. 512 KB).
*/
typedef double Tweight;  


/**===========================================================================
  
  @class WeightFuncBase 
   
  Base class (non-template) of the weight template(s). Contains common things
   which are independent of the data type, in particular the `Shape' enums, the
   min-max-values useful for plotting, and the description string. "Abstract" 
   class - instances prevented.
  
=============================================================================*/
class WeightFuncBase 
{
public:
    enum Shape 
    {   /* non-parametric functions: */
        IDENTITY,
        IDENTITY_0,
        TRIANGLE,
        SINUS,
        /* parametric functions: */
        IDENTITY_0_PARAM,
        TRIANGLE_PARAM
    };

private:
    Shape  shape_;
    Tweight  minval_;                   // maximal weight value
    Tweight  maxval_;                   // minimal weight value
    std::string  what_;                 // description string
        
public:    
    Shape shape() const                 {return shape_;}  
    Tweight minval() const              {return minval_;}
    Tweight maxval() const              {return maxval_;}
    const char* what() const            {return what_.c_str();}

protected:
    /*  Ctor protected to avoid attempted instances */    
    WeightFuncBase (Shape s) : shape_(s)  {}   // ?? minval, maxval undefined
    
    /*  set() methods only for heirs */
    void set_minval (Tweight w)         {minval_ = w;}
    void set_maxval (Tweight w)         {maxval_ = w;}
    void set_what (const char* s)       {what_ = s;}
};


/**===========================================================================
  
  @class WeightFunc  -  template
   
  Template der Wichtungsfunktionsobjekte mit (virt.) Funktionssyntax. Erben haben
   diese virtuelle Funktion zu definieren. Bereitgestellt ist dann zusaetzlich
   die Moeglichkeit, eine Tabelle mit diesen Werten zu initialisieren und via
   []-Op auszulesen, was schneller als der Funktionsaufruf. Da fuer Unsign==U16
   die Tabelle 65535 double- oder float-Werte umfasst, kann dieser Speicher bei
   zeitkritischen Aktivitaeten genutzt, bei speicherkritischen aber auch gespart
   werden.
   
  Die Funktionssyntax also immer moeglich, der []-Op nur nach init_table(). Bei
   U8 waere zu ueberlegen, die Tabellen stets schon im Ctor anzulegen.
   
  Zur Verwaltung der Tabelle nutzen wir die referenzzaehlende Haendelklasse 
   TNT::Array1D<> (Element, keine Ableitung).  
    
  NOTE: Auch wenn die virt. Funktion im Basistemplate nicht abstrakt und
   Instanzen auch von Weight_U8 moeglich waeren, koennten wir trotzdem keine
   Basis-*Objekte* als allg. Weight-Variablen benutzen, d.h. Dinge machen wie:
   <pre>
       Weight_U8  w;                 // allg. Variable
       Weight_U8_Triangle  tri;      // konkretes Funktionsobjekt
       w = tri;                      // Kopiere `tri´ auf `w´?  FEHLER!
   </pre>
   Denn bei physischen Kopien bleiben die Funktionen des Zielobjekts unveraendert,
   die virt. Fkt. "w(z)" waere nach wie vor die der Basis, nicht die von Triangle,
   wenngleich die Parameter ueberschrieben. Als allgemeine Variablen funktionieren
   nur Zeiger der Basisklasse:
   <pre>
       Weight_U8*  w;                // Basisklassen-Zeiger als allg. Variable
       Weight_U8_Triangle  tri;      // konkretes Funktionsobjekt
       w = &tri;                     // `w' referenziert jetzt `tri'. OK.
   </pre>
   
=============================================================================*/
template <typename Unsign>
class WeightFunc : public WeightFuncBase
{
    TNT::Array1D<Tweight>  table_;
    Unsign  zmax_;                              // maximal z-argument

public:
    /*  virt. Dtor to avoid warnings, not realy needed till now. */
    virtual ~WeightFunc()               {/*std::cout << "Dtor WeightFunc\n";*/}
      
    /*  Create table and fill it with function values */
    void init_table();     
    
    /*  Remove the table */
    void remove_table()                         {table_ = TNT::Array1D<Tweight>();}
    
    /*  Have we a table created? */
    bool have_table() const                     {return ! table_.is_null();}
    
    /*  Reference of the table. Allows table().dim() etc. */
    const TNT::Array1D<Tweight> & table() const {return table_;}
    
    /*  The maximal z-argument */
    Unsign  zmax() const                        {return zmax_;}
    
    /*  []-syntax: table value to the z-value `z' */
    const Tweight & operator[] (Unsign z) const {return table_[z];}
    Tweight &       operator[] (Unsign z)       {return table_[z];}
    
    /*  Function syntax - abstract virtual function */
    virtual Tweight operator() (Unsign z) const = 0;    // ABSTRACT !!

protected:        
    /*  Ctor protected to avoid attempted instances of abstract class */
    WeightFunc (Shape s, Unsign zmax)
      : WeightFuncBase (s),
        zmax_(zmax)                     // `table_' init as Null-Array
      {}
};


/**+***************************************************************************
  Allocate the table and fill it using the virtual function.
******************************************************************************/
template <typename Unsign>
void WeightFunc<Unsign>::init_table()
{
    table_ = TNT::Array1D<Tweight> (zmax_ + 1); // kein Ueberlauf, da bei numer. Ops
                                                //  stets vorher in int gewandelt
    for (int z=0; z < table_.dim(); z++)  
      table_[z] = (*this)(z);                   // uses the virt. function
}


#if 1
/**===========================================================================
  
  @class WeightFunc_Triangle  -  template   (unused, exercise)
  
  Beispiel eines von WeightFunc<> abgeleiteten Dreieck-TEMPLATES, d.h. mit 
   einer allgemeinen (template-tauglichen) Definition der virt. Funktion.
   Ein Nachteil: Im Konstruktor muss `zmax' jedesmal explizit angegeben werden.

  NOTE: Wegen der Abstammung 
    <pre>     WeightFunc_Triangle<> --> WeightFunc<> </pre>
   gilt selbiges auch fuer die Spezialisierungen: 
    <pre>     WeightFunc_Triangle<uint8> --> WeightFunc<uint8>. </pre>
   Dagegen ist die von <tt>WeightFunc<></tt> abgeleitete U8-Basisklasse 
    <pre>     WeightFunc_U8 --> WeightFunc<uint8> </pre>
   nicht mehr die Basisklasse auch von <tt>WeightFunc_Triangle<uint8></tt>.
=============================================================================*/
template <typename Unsign>
class WeightFunc_Triangle : public WeightFunc<Unsign> 
{
    Unsign zmin_;                               // minimal z-value; == 0
    Unsign zmid_;                               // middle z-value 

public:
    
    WeightFunc_Triangle (Unsign zmax)           // Ctor
      :
        WeightFunc<Unsign> (WeightFuncBase::TRIANGLE, zmax)
      { 
        zmin_ = 0;                              // before any use of (*this)()!
        zmid_ = (zmin_ + zmax) / 2;             // before any use of (*this)()!
        WeightFunc<Unsign>::set_minval (0.0);
        WeightFunc<Unsign>::set_maxval ((*this)(zmid_));  // maximum at the middle
      }

    Tweight operator() (Unsign z) const         // virtual Triangle function; could
      {                                         //  be simplified by zmin==0.
        //return (z <= zmid_) ? (z - zmin_) : (zmax_ - z);
        return ((z <= zmid_) ? (z - zmin_) : (WeightFunc<Unsign>::zmax() - z)) / Tweight(zmid_);
      }
};
#endif


#endif // WeightFuncBase_hpp

// END OF FILE

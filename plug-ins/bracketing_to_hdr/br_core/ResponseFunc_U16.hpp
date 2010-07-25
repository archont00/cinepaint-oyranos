/*
 * ResponseFunc_U16.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  ResponseFunc_U16.hpp
  
  Responsefunktion fuer U16-Werte, die aus 256 Stuetzstellen linear oder 
   spline-interpoliert.
*/
#ifndef ResponseFunc_U16_hpp
#define ResponseFunc_U16_hpp


#include "TNT/tnt_array1d.hpp"          // TNT::Array1D<>
#include "br_types.hpp"                 // uint16
#include "spline.hpp"                   // splint()

/**============================================================================
  
  @class ResponseInterpol
  
  Klasse stellt Funktionen zur linearen Interpolation und zur kubischen 
   Spline-Interplation bereits, spezialisiert sozwar fuer uint16 Argumente und
   fixe x-Stuetzstellen. 
   - value_linear() interpoliert linear;
   - value_spline_ext() nutzt die externe "Numerical Recipes in C"-Funktion
      splint(), die fuer beliebige Stuetztstellen arbeitet (zum Vergleichszwecke);
   - value_spline_fast() findet die umgebenden Stuetzstellen schneller als durch 
      Bisektion;
   - value_spline_table() gibt tabellierte Werte aus; 
   - value_spline() dient als Default-Funktion (z.Zeit value_spline_fast()).
  
  Name "Response"Interpol, weil keine allg. Interpolation, sondern schon 
   spezialisiert auf uint16-Argumente und fixe x-Stuetzstellen.
  
  Die x-Stuetzstellen liegen (zwangslaeufig) bei uint16-Werten, diese zu
   speichern reichte daher uint16-Feld. Bei der Interpolation (linear wie spline)
   dann gebraucht in der Weise <i>double_val * (z-xgrid[i])</i>, was heisst, die
   beiden uint16 \a z und \a xgrid[i] erst in int's wandeln und dann in doubles. 
   Zeitmaessig ein kleiner NACHTEIL gegen ein Rechnen gleich mit einem double-Feld.
   Daher halten wir ein double-Feld. Auch will die "NR-in-C"-Routine spline()
   zur y2-Berechnung die x-Stellen als double-Feld (welches fuer die spline-Initial.
   freilich auch nur temporaer angelegt werden koennte), und zudem erwartet auch
   die "NR-in-C"-Routine splint() doubles. Aber wie gesagt, minimalistisch
   reichte fuer eine reine lineare Interpolation oder auch unsere schliessliche
   value_spline_fast() Routine ein uint16-Feld.
   
  @note Funktion value_spline_table() mit uint16-Argument? Der []-Operator von
   \a table_ nimmt dann sowieso nur int's, d.h. Konvertierung erfolgt immer.
   Also table()-Funktion gleich mit int-Argument? Da inline, kein Zeitvorteil,
   aber der Nachteil, nicht mehr auf typgerechtes Argument geprueft zu werden.
   
=============================================================================*/
class ResponseInterpol
{
    TNT::Array1D<double>  xgrid_;       // Stuetzstellen (Abzissen)
    TNT::Array1D<double>  ygrid_;       // Stuetzstellen (Ordinaten)
    TNT::Array1D<double>  y2_;          // Helper for spline interpolation
    TNT::Array1D<double>  table_;

public:
    ResponseInterpol (const TNT::Array1D<double> & ygrid);     // Ctor
    
    double value_linear       (uint16 z) const;
    double value_spline       (uint16 z) const  {return value_spline_fast(z);}  // default
    double value_spline_ext   (uint16 z) const  {return splint (xgrid_, ygrid_, y2_, xgrid_.dim(), z);}
    double value_spline_fast  (uint16 z) const;
    double value_spline_table (uint16 z) const  {return table_[z];}
    
    void init_spline();
    void init_table();
    void remove_table()                         {table_ = TNT::Array1D<double>();}
    bool have_table()                           {return !table_.is_null();}

    //  Diagnose-Hilfen    
    const TNT::Array1D<double> & xgrid() const  {return xgrid_;}
    const TNT::Array1D<double> & ygrid() const  {return ygrid_;}
    const TNT::Array1D<double> & y2() const     {return y2_;}    
};


/**============================================================================
  
  @class ResponseFunc_U16
  
  Interpolierendes Response-Funktionsobjekt (Funktionssyntax "f(z)"). Nach 
   init_table() dient die []-Syntax zum Ausgeben der tabellierten Werte.

  @note []-Operator mit uint16 oder int-Argument? uint16 boete Warnungen bei
   Aufrufen mit Typen groesser uint16. Zeitmaessig kein Gewinn, da der aufgerufene
   TNT::Array-[]-Operator von \a table_ auch nur fuer int-Argument erklaert ist.
   Vermutlich wird sowieso jeder Feld-Aufruf \a F[i] mit kleinerem Typ \a i als
   int am Ende auf int's oder uint's zurueckgefuehrt wie in numer. Rechnungen.
    
=============================================================================*/
class ResponseFunc_U16 : public ResponseInterpol
{
public:
    /** 
    *  @param Y: Fuer das HDR-Merging ist \a Y dann ein logX-Vektor. 
    */
    ResponseFunc_U16 (const TNT::Array1D<double> & Y)
      : ResponseInterpol (Y)                 
      { init_spline(); }
      
    double operator() (uint16 z) const            {return value_spline_fast(z);}
    double operator[] (uint16 z) const            {return value_spline_table(z);}
};



#endif  // ResponseFunc_U16_hpp

// END OF FILE

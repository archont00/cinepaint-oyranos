/*
 * Rgb.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file Rgb.hpp 
   
  Rgb-Template.
  Hartmut Sbosny  <hartmut.sbosny@gmx.de>

  Helpful suggestions from the Standard C++ Library <complex> 
  (/usr/include/g++/complex).

  Wahlweise mit typreinen oder typmischenden arithm. Operatoren (Schalter
   RGB_MIXED_ARITHMETIC).

*/
#ifndef Rgb_hpp
#define Rgb_hpp


/*!  Define this to get type mixing arithm. operators */
//#define RGB_MIXED_ARITHMETIC


/**============================================================================

  @struct Rgb

  <h3>Semantik</h3>
  
  <h4>Reintypige Operationen</h4>
   
   <ul>
   <li>
     Semantik arithmetischer Vec3-Operationen <tt>( @ in {+, -, *, /} )</tt>:
     <pre>
       Rgb(r,g,b) @ Rgb(_r,_g,_b) = Rgb(r@_r, g@_g, b@_b). </pre>
        
   <li>
     Semantik von Ausdruecken der Art
     <pre>
       Rgb<T> @= T   sowie   Rgb<T> @ T   und   T @ Rgb<T>:  </pre>
    
     Das einzelne T t wird stets interpretiert als Rgb<T>-Objekt mit
     den Komponenten (t,t,t), also z.B. 
     <pre>
       2 - Rgb(r,g,b) = Rgb(2,2,2) - Rgb(r,g,b) = Rgb(2-r, 2-g, 2-b). </pre>
   </ul>
    
  <h4>Mischtypige Operationen:</h4>
   <ul>
   <li>
     Eistellige Operationen der Art 
      <pre> 
        Rgb<T> @= Rgb<U_> :  </pre>

      Diese sind wie in <complex> generell (dh auch bei ausgeschaltetem
      MIXED-Schalter) durch Templates erfasst und stets definiert durch
      elementweises<pre>
          t @= u   (mit t,u=r,g,b). </pre>
      D.h. die eingebaute Automatik wuerde mglw. zunaecht auf den groesseren der
      beiden Typen aufblaehen, das Ergebnis aber in jedem Fall auf
      den linken Typ zurechtstutzen. Wenn T der kleinere Typ,
      gibt das Verluste, und davor warnt der Compiler vermtl. nicht.

   <li>   
     Problem mischttypiger arithm. Operationen der Art 
      <pre>
        Rgb<T> @ Rgb<U_>,   Rgb<T> @ U_,  T @ Rgb<U_>:  </pre>

      Es waere sicherzustellen, dass analog zu eingebauten Typen jeweils auf
      die hoehere Genauigkeit, den groesseren Typ transformiert wird, also
      <pre>
        Rgb<float> * Rgb<int> --> Rgb<float>
        Rgb<int> / Rgb<float> --> Rgb<float>
        int(2) / Rgb<float>   --> Rgb<float> etc.  </pre>
          
      Aber kann man das abstrakt erreichen?
      Selbst wenn alles auf Operatoren der eingebauten Typen zurueckgefuehrt
      wird und die eingebaute Automatik greift, wie kann denn der eingebaute
      Rueckgabetyp zum offiziellen Rueckgabetyp werden?
      Das geht anscheinend nur durch explizte Spezialisierung aller fraglichen
      Kombination.  Auch <complex> hat keine Operatoren fuer 
      <tt> complex<int> @ complex<float> </tt>, nur fuer 
      <tt> complex<float> @ complex<double> </tt>  und die sind expliziert!
   </ul>

=============================================================================*/

template <class T>
struct Rgb {

    T   r,g,b;

    //-----------------------
    //  Type of the elements 
    //-----------------------
    typedef T   ValueType;
    
    //----------------
    //  Konstruktoren
    //----------------
    Rgb() {}                        // Default Ctor, no initialization

    explicit Rgb (T t)              // Sets all components to value t;
        : r(t), g(t), b(t) {}       // using Copy Ctor of the T's

    Rgb (T _r, T _g, T _b)          // Init-Rgb is (_r,_g,_b)
        : r(_r), g(_g), b(_b) {}    // using Copy Ctor of the T's

    template <typename U_>          // Converting Rgb<T> <-- Rgb<U_>
    Rgb (const Rgb<U_>& z)
        : r(z.r), g(z.g), b(z.b) {}

    //---------------
    //  Index access  
    //---------------
    T& operator[] (int i) 
        {   return (&r)[i]; }
    
    const T& operator[] (int i) const
        {   return (&r)[i]; }

    //-----------------------
    //  Zuweisungsoperatoren
    //-----------------------
/*    template <typename U_>            // done by )converting) Copy-Ctor
    Rgb<T>& operator= (const Rgb<U_>& z)
        {   r = z.r;  g = z.g;  b = z.b;
            return *this;
        }*/
    
    Rgb<T>& operator= (T t)             // Rgb<T> = T
        {   r=t; g=t; b=t;
            return *this;
        }
    
    //---------------------
    //  arithm. Operatoren
    //---------------------
    Rgb<T> operator- ()                 // unary "-"
        {   return Rgb<T>(-r, -g, -b);
        }
    
    template <typename U_>              // Rgb<T> += Rgb<U_>
    Rgb<T>& operator+= (const Rgb<U_>& z)
        {   r += z.r; g += z.g; b += z.b;
            return *this;
        }

    template <typename U_>
    Rgb<T>& operator-= (const Rgb<U_>& z)
        {   r -= z.r; g -= z.g; b -= z.b;
            return *this;
        }

    template <typename U_>
    Rgb<T>& operator*= (const Rgb<U_>& z)
        {   r *= z.r; g *= z.g; b *= z.b;
            return *this;
        }

    template <typename U_>
    Rgb<T>& operator/= (const Rgb<U_>& z)
        {   r /= z.r; g /= z.g; b /= z.b;
            return *this;
        }

    // Ausdruecke der Art ``Rgb<T> @= T''
    // Hier koennte 'T t' ebenfalls ein anderer Typ sein.
    // FRAGE: als Argument `T' oder `const T&'? Fuer grosse Objekte Referenz
    //  besser, fuer kleine weiss ich nicht. <complex> benutzt Referenzen.

    Rgb<T>& operator+= (T t)
        {   r += t; g += t; b += t;
            return *this;
        }

    Rgb<T>& operator-= (T t)
        {   r -= t; g -= t; b -= t;
            return *this;
        }

    Rgb<T>& operator*= (T t)
        {   r *= t; g *= t; b *= t;
            return *this;
        }

    Rgb<T>& operator/= (T t)
        {   r /= t; g /= t; b /= t;
            return *this;
        }
        
    //---------------------
    //  Extended interface 
    //---------------------    
    T trace() const
        {   return (r + g + b);  }
    
    T length2() const
        {  return (r*r + g*g + b*b);  } 

};

///////////////////////////////////
///
///  Nichtelement-Funktionen
///
///////////////////////////////////

//-------------
//  Gleichheit
//-------------
template <class T> inline
bool operator== (const Rgb<T>& a, const Rgb<T>& b)
{
    return (a.r==b.r && a.g==b.g && a.b==b.b);
}

template <class T> inline
bool operator!= (const Rgb<T>& a, const Rgb<T>& b)
{
    return (a.r!=b.r || a.g!=b.g || a.b!=b.b);
}

template <class T> inline
bool operator== (const Rgb<T>& a, const T& val)
{
    return (a.r==val && a.g==val && a.b==val);
}

template <class T> inline
bool operator!= (const Rgb<T>& a, const T& val)
{
    return (a.r!=val || a.g!=val || a.b!=val);
}


//----------------------------------
//  zweistellige arithm. Operatoren
//----------------------------------
                                //************************
#ifndef RGB_MIXED_ARITHMETIC    //  none-mixed arithmetic
                                //************************
//---------------
//  a) Rgb @ Rgb
//---------------                                
template <class T> inline
Rgb<T> operator+ (const Rgb<T>& a, const Rgb<T>& b)
{
    return Rgb<T>(a) += b;
}

template <class T> inline
Rgb<T> operator- (const Rgb<T>& a, const Rgb<T>& b)
{
    return Rgb<T>(a) -= b;
}

template <class T> inline
Rgb<T> operator* (const Rgb<T>& a, const Rgb<T>& b)
{
    return Rgb<T>(a) *= b;
}

template <class T> inline
Rgb<T> operator/ (const Rgb<T>& a, const Rgb<T>& b)
{
    return Rgb<T>(a) /= b;
}

//--------------------------------
//  b) Rgb<T> @ T  and T @ Rgb<T>
//--------------------------------
template <class T> inline
Rgb<T> operator+ (const Rgb<T>& a, T t)
{
    return Rgb<T>(a) += t;
}

template <class T> inline
Rgb<T> operator+ (T t, const Rgb<T>& a)
{
    return Rgb<T>(a) += t;
}

template <class T> inline
Rgb<T> operator- (const Rgb<T>& a, T t)
{
    return Rgb<T>(a) -= t;
}

template <class T> inline
Rgb<T> operator- (T t, const Rgb<T>& a)
{
    return Rgb<T>(t - a.r, t - a.g, t - a.b);
}

template <class T> inline
Rgb<T> operator* (const Rgb<T>& a, T t)
{
    return Rgb<T>(a) *= t;
}

template <class T> inline
Rgb<T> operator* (T t, const Rgb<T>& a)
{
    return Rgb<T>(a) *= t;
}

template <class T> inline
Rgb<T> operator/ (const Rgb<T>& a, T t)
{
    return Rgb<T>(a) /= t;
}

// "t/a" hat das Aussehen einer Division eines Skalars durch einen Vektor
// und koennte verwirrlich wirken. Bei komplexen Zahlen ist gleichwohl ein
// solcher Ausdruck im Gebrauch und z.B "2/z" wird dort gelesen als
// die komplexe Division (2, 0) / (z.re, z.im), also der Skalar "2" nur dem
// Realteil des ersten Arguments zugedacht. Hier, bei Rgb's, kann man passend
// zur Semantik der Multiplikation "t*a" analog "t/a" festsetzen als denjenigen
// Rgb mit den Komponenten (t/a.r, t/a.g, t/a.b), also als die Rgb-Division
// (t,t,t) / (a.r, a.g, a.b). So ist's hier gemacht.

template <class T> inline
Rgb<T> operator/ (T t, const Rgb<T>& a)
{
    return Rgb<T>(t) /= a;
}

//  Shift-Operatoren (nur fuer Integer-Typen)
template <typename T> inline
Rgb<T> operator>> (const Rgb<T>& a, int n)
{
    return Rgb<T>(a.r >> n, a.g >> n, a.b >> n);
}

template <typename T> inline
Rgb<T> operator<< (const Rgb<T>& a, int n)
{
    return Rgb<T>(a.r << n, a.g << n, a.b << n);
}

                //*******************************
#else           //  RGB_MIXED_ARITHMETIC defined
                //*******************************
//---------------
//  a) Rgb @ Rgb
//---------------                                
template <class T, class U_> inline
Rgb<T> operator+ (const Rgb<T>& a, const Rgb<U_>& b)
{
    return Rgb<T>(a) += b;
}

template <class T, class U_> inline
Rgb<T> operator- (const Rgb<T>& a, const Rgb<U_>& b)
{
    return Rgb<T>(a) -= b;
}

template <class T, class U_> inline
Rgb<T> operator* (const Rgb<T>& a, const Rgb<U_>& b)
{
    return Rgb<T>(a) *= b;
}

template <class T, class U_> inline
Rgb<T> operator/ (const Rgb<T>& a, const Rgb<U_>& b)
{
    return Rgb<T>(a) /= b;
}

// Spezialisierungen fuer all die Faelle, wo erster Operand ein "kleinerer"
// Typ als der zweite und Ergebnis vom zweiten "groesseren" Typ sein sollte.
// Das gibt aber einen Schwarm an Kombinationen!

// int - float

inline Rgb<float> operator+ (const Rgb<int>& a, const Rgb<float>& b)
{
    return Rgb<float>(a) += b;
}
inline Rgb<float> operator- (const Rgb<int>& a, const Rgb<float>& b)
{
    return Rgb<float>(a) -= b;
}
inline Rgb<float> operator* (const Rgb<int>& a, const Rgb<float>& b)
{
    return Rgb<float>(a) *= b;
}
inline Rgb<float> operator/ (const Rgb<int>& a, const Rgb<float>& b)
{
    return Rgb<float>(a) /= b;
}


//--------------------------------
//  b) Rgb<T> @ T  and T @ Rgb<T>
//--------------------------------
// Zur Zeit typrein, koennte ebenfalls typmischend definiert werden,
// mit der Notwendigkeit dann aber auch zu Spezialisierungen.

template <class T> inline
Rgb<T> operator+ (const Rgb<T>& a, T t)
{
    return Rgb<T>(a) += t;
}

template <class T> inline
Rgb<T> operator+ (T t, const Rgb<T>& a)
{
    return Rgb<T>(a) += t;
}

template <class T> inline
Rgb<T> operator- (const Rgb<T>& a, T t)
{
    return Rgb<T>(a) -= t;
}

template <class T> inline
Rgb<T> operator- (T t, const Rgb<T>& a)
{
    return Rgb<T>(t - a.r, t - a.g, t - a.b);
}

template <class T> inline
Rgb<T> operator* (const Rgb<T>& a, T t)
{
    return Rgb<T>(a) *= t;
}

template <class T> inline
Rgb<T> operator* (T t, const Rgb<T>& a)
{
    return Rgb<T>(a) *= t;
}

template <class T> inline
Rgb<T> operator/ (const Rgb<T>& a, T t)
{
    return Rgb<T>(a) /= t;
}

template <class T> inline
Rgb<T> operator/ (T t, const Rgb<T>& a)
{
    return Rgb<T>(t) /= a;
}

#endif  // RGB_MIXED_ARITHMETIC


//--------------
//  dot product
//--------------
template <typename T> inline
T dot (const Rgb<T>& A, const Rgb<T>& B)
{
    return (A.r * B.r + A.g * B.g + A.b * B.b);
}


#endif  // Rgb_hpp

// END OF FILE

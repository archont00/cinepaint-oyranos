/*
 * Distributor.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file  Distributor.hpp
  
  Contents:
   - DistributorBase<>
   - ValueDistributor<>
*/
#ifndef Distributor_hpp
#define Distributor_hpp

#include <vector>


 
/**===========================================================================

  @class  DistributorBase      
  
  Non-template stuff of the Distributor templates.
  
  Utility for communication between classes. Be \a _distrib a global
   ValueDistributor<int> instance. Classes - e.g. Widgets - which wants take 
   part on the distribution have to login into \a _distrib with a STATIC member
   function as callback and a userdata (usualy their \a this pointer), which is 
   deposited in \a _distrib. If now "anyone" calls \a _distrib.distribute(i), 
   then all registered callback functions are called with the address of \a i 
   as first argument and their deposited userdata (this pointer) as second one.
   The participants can do then the needed local things. If \a i exists only 
   temporarly in respect to the registred classes -- e.g. a local variable of
   the caller, then the receivers should take copies of \a i.
   
  @note Structure `Entry' contains only void pointer, so only one std::vector<> 
   template instance is needed for all ValueDistributor<> templates.

  <b>Open question:</b> Shall the Callback function be declared with constant 
   or with non-constant \a pData pointer:<pre>
      
      typedef void (Callback) (const void* pData, void* userdata)  or
      typedef void (Callback) (void* pData, void* userdata) ?</pre>
   
   Non-constant, if the pointed data shall to be changeable from inside the 
   callback functions. But I think, we don't need this. See also the discussion
   below at ValueDistributor: only constant pointer allow an `value(const T&)'.
   The constancy of the pointer `userdata' is a further question.

  @todo std::vector could be substituted by a slimmer utility, reduced to the
    essential.
      
  @bug Moegl. Mehrdeutigkeit beim Ausloggen bei statischen Elementfunktionen:
    Alle Instanzen einer Klasse A haben denselben Callback-Pointer. Wird 
    mittels `logout(cb)' statt `logout(userdata=this)' ausgelogt, waeren die
    Callbacks verschiedener A-Instanzen nicht unterscheidbar, es wuerde der
    erste Passende ausgeloggt. Es sollte wohl eine zur login()-Funktion
    symmetrische logout(fp,userdata)-Funktion angeboten werden.

*============================================================================*/
class DistributorBase
{
public:
    /**
    *   Type of the callback functions in the *base* class:
    *
    *   @param pData: void pointer to the info ("value") to distribute.
    *   @param userdata: userdata deposites with login(); common the `this'
    *      pointer of the participant.
    */
    typedef void (Callback) (const void* pData, void* userdata);

protected:
  
    /** Structure for storing the callback data: function pointer `fp' +
     *   userdata (usually the `this' pointer of the class, `fp' belongs to).
     */
    struct Entry { 
      Callback* fp;  
      void* userdata;
      Entry (Callback* f, void* u) : fp(f), userdata(u) {} 
    };

    std::vector<Entry> array_;

public:
    void login  (Callback* fp, void* userdata);
    void logout (Callback* fp);             // comment: see bug report
    void logout (void* userdata);
    void distribute (const void* pData);
    //void distribute (const void*, void* caller); // if caller would be needed
    void report (const char* label) const;  // List array on console
};



/**===========================================================================

  @class  ValueDistributor

  Type-safer interface to DistributorBase.

  @note  No "using DistributorBase::distribute;", so that the void-pointer
   version of distribute() is not available.

  <b>Offen</b> fuer mich nach wie vor, ob distribute(T), distribute(T&) oder
   distribute(const T&)?  Differenzen: 
   <ol>
    <li> 'distribute(T)' schlechter fuer grosse T's. 
    <li> Nur 'distribute(T)' und `distribute(const T&)' erlauben Dinge wie
     `distribute(5)' oder `distribute(fn())', wo fn() eine Fkt `T fn()'. 
   </ol>
   Wegen (2) sollte `distribute(T&)' vermieden werden. Schliesslich wollen wir
   innerhalb distribute(..) den T-Wert ja nicht aendern, oder?
  
  Alles klar, `value(const T&)' scheint die Wahl. Das verlangt dann aber 
   auch obige Callback Funktion mit Konstantzeiger zu deklarieren! D.h.
   'distribute(const T&)' impliziert, dass auch in den Callback-Funktionen das
   dort per Zeiger referenzierte Value-Objekt niemals veraendert werden kann.

*============================================================================*/
template <typename T> 
class ValueDistributor : public DistributorBase
{
#if 0
    // Below we redefine logout(fp), but need again Base::logout(userdata):
    using DistributorBase::logout;
#endif

public:
    /** 
    *   A callback type which reflects the type \a T in the first argument:
    * 
    *   @param pData: pointer to a T-value to distribute
    *   @param userdata: userdata deposites with login(), common the `this'
    *      pointer of the participant.
    */
    typedef void (Callback) (const T* pData, void* userdata);

   
    // Redefine Base functions for type-safer (T-reflecting) *Callback* type:
    void login (Callback* fp, void* userdata)   // blankets Base::login
      {DistributorBase::login ((DistributorBase::Callback*)fp, userdata);}
    
    void logout (Callback* fp)                  // blankets Base::logout
      {DistributorBase::logout ((DistributorBase::Callback*)fp);}  

    // Repeat logout(void*) due to visibility problems in g++ > 3.3.5 despite the `using Base::logout'
    void logout (void* userdata)                // blankets Base::logout
      {DistributorBase::logout (userdata);}  

    // Redefine Base function for type-safe distribute()-argument:
    void distribute (const T& val)              // blankets Base::distribute
      {DistributorBase::distribute ((const void*) &val);}
};



#endif  // Distributor_hpp

// END OF FILE

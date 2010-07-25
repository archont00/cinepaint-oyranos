/*
 * Scheme2D.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  Scheme2D.hpp
*/
#ifndef Scheme2D_hpp
#define Scheme2D_hpp


/**============================================================================

  @class VoidScheme2D
 
  Testweise als Versuch einer kompakten Bereitstellung der Parameter einer
   abstrakten 2D-Array-Beschreibung, wie es neuerdings in DisplcmFinderCorrel
   sich nahelegte. Primaerziel war die Sammlung der Parameter in einer Struktur
   und die Bereitstellung sprechender Namen, insbesonder von "xSize()" fuer
   dim2() und "ySize()" fuer dim1(), was immer wieder Verwechslungen gab.
   Feldzugriffe werden natuerlich nicht geboten. Ob das auch zur Basisklasse von
   Scheme2D<> werden sollte, bezweifle ich noch, machte dort einige Casts
   noetig. -- Man koennte noch eine `elementsize' Variable einfuehren, die die
   Groesse der hier abstrakt beschriebenen "T"-Feldelemente enthaelt.
   
  @internal Auch hier ein CVoidScheme2D mit "C" fuer "const" sinnvoll.
  
=============================================================================*/
class VoidScheme2D
{
    int   dim1_;            // number of rows (outer dimension)
    int   dim2_;            // number of columns (inner dimension)
    int   stride_;          // the real number of T's in memory from row to row
    void* data_;            // points to the data begin

public:        
    //---------------
    //  Constructors
    //---------------
    VoidScheme2D() : dim1_(0), dim2_(0), stride_(0), data_(0)  // NULL scheme
      {}
    VoidScheme2D (int dim1, int dim2, void* buf, int stride=0)
      : dim1_ (dim1),
        dim2_ (dim2),
        data_ (buf)
      { stride_ = stride ? stride : dim2; }
    
    VoidScheme2D (int dim1, int dim2, const void* buf, int stride=0)
      : dim1_ (dim1),
        dim2_ (dim2),
        data_ ((void*)buf)            // non-const <-- const
      { stride_ = stride ? stride : dim2; }
    
    //--------------------
    //  Dimensions & data
    //--------------------  
    int dim1() const                  {return dim1_;}
    int dim2() const                  {return dim2_;}
    int stride() const                {return stride_;}
    int size() const                  {return dim1_ * dim2_;}
    bool contiguous() const           {return dim2_ == stride_;}
    const void* data() const          {return data_;}  
    
    //------------------------------
    //  Synonyms for image contexts
    //------------------------------
    int xSize() const                 {return dim2_;}
    int ySize() const                 {return dim1_;}
    int width() const                 {return dim2_;}
    int height() const                {return dim1_;} 
};


/**============================================================================

  @class Scheme2D
 
  Template. Typecasted 2D view on a void* buffer providing the \c "[][]" array
   syntax. With <i>stride != dim2</i> also subarrays of a contiguous memory block
   of \a T elements can be described. Additionally an 1D view is provided by
   view1d(int). view1d() is not aware of non-contiguous elements, its usage is
   on own response; for contiguous()==FALSE view1d() does not iterate exclusively
   the "[][]" elements! -- Copying allowed (default Copy-Ctor and =-Op do the
   right).
   
  @note If contiguous()==TRUE, all "[][]" elements are contiguous, if !contiguous(),
   only the elements of a row form a contiguous block in memory.
  
  @todo Gelegentlich ein abgeleitetes "CScheme2D<>" mit "C" fuer "const"
   darbieten, das nur const [][]-Zugriffe (nur Lesen) bereitstellt, denn durch 
   `<tt>const Scheme2D<>&</tt>'-Argumente sind nicht alle Konstantwuensche abzudecken,
   da das hiesige Scheme2D<> fuer ein Konstantfeld ja nur zu konstruieren ist,
   indem im Ctor das const brutalst weggecastet wird (andernfalls muesste der
   Anwender es tun).
   
=============================================================================*/
template <typename T>
class Scheme2D
{
    int  dim1_;             // number of rows (outer dimension)
    int  dim2_;             // number of columns (inner dimension)
    int  stride_;           // the real number of T's in memory from row to row
    T*   data_;             // points to the data begin

public:
    //-----------------------
    //  Type of the elements
    //-----------------------
    typedef T   ValueType;

    //---------------
    //  Constructors
    //---------------
    Scheme2D() : dim1_(0), dim2_(0), stride_(0), data_(0)  // Default Ctor: NULL scheme
      {}
    Scheme2D (int dim1, int dim2, void* buf, int stride=0)
      : dim1_ (dim1),
        dim2_ (dim2),
        data_ ((T*)buf)
      { stride_ = stride ? stride : dim2; }
    
    Scheme2D (int dim1, int dim2, const void* buf, int stride=0)
      : dim1_ (dim1),
        dim2_ (dim2),
        data_ ((T*)buf)                 // non-const <-- const
      { stride_ = stride ? stride : dim2; }
    
    Scheme2D (VoidScheme2D & scheme)
      : dim1_ (scheme.dim1()),
        dim2_ (scheme.dim2()),
        stride_ (scheme.stride()),
        data_ ((T*)scheme.data())       // non-const <-- const
      {}
    
    Scheme2D (const VoidScheme2D & scheme)
      : dim1_ (scheme.dim1()),
        dim2_ (scheme.dim2()),
        stride_ (scheme.stride()),
        data_ ((T*)scheme.data())       // non-const <-- const
      {}
      
    //-------------
    //  Dimensions
    //-------------  
    int dim1() const                  {return dim1_;}
    int dim2() const                  {return dim2_;}
    int stride() const                {return stride_;}
    int size() const                  {return dim1_ * dim2_;}
    bool contiguous() const           {return dim2_ == stride_;}
    
    //------------------------------
    //  Synonyms for image contexts
    //------------------------------
    int xSize() const                 {return dim2_;}
    int ySize() const                 {return dim1_;}
    int width() const                 {return dim2_;}
    int height() const                {return dim1_;} 
    
    //---------------
    //  Index access
    //---------------
    const T* operator[] (int i) const {return & data_[ i * stride_ ];}
    T*       operator[] (int i)       {return & data_[ i * stride_ ];}
    
    //----------------------------
    //  Cast operators and data()
    //----------------------------
    operator const T*() const         {return data_;}   // Cast op
    operator T*()                     {return data_;}   // Cast op
    const T* data() const             {return data_;}
    
    //------------
    //  subscheme
    //------------
    Scheme2D<T> subscheme (int i0, int i1, int j0, int j1) const;
    
    //----------
    //  1D view
    //----------
    const T& view1d (int i) const     {return data_[i];}
    T&       view1d (int i)           {return data_[i];}  
};



/**+*************************************************************************\n
  Create a new view to a subarray defined by the boundaries [i0][i0] and
   [i1][j1].  The size of the subarray is (i1-i0) by (j1-j0).  If either
   of these lengths are zero or negative, the subarray view is null.
   
  @note The first arguments i0, i1 refer to the "outer dimension" dim1() --
   ySize(), height() -- and the last arguements j0, j1 to the "inner dimension"
   dim2() -- xSize(), width().
   
  @internal Hier findet kein Referenzzaehlen statt. Wo das gewollt ist wie 
   im Erben ImgScheme2D, muss ein neues subscheme() geschrieben werden (mit
   ImgScheme2D als Rueckgabe), das dieses hier dann ueberdeckt. Ferner ist
   dieses hier eine const-Funktion, ein subscheme(), das Referenzzaehler
   erhoeht, kann es u.U. nicht mehr sein.
******************************************************************************/
template <typename T>
Scheme2D<T> Scheme2D<T>::subscheme (int i0, int i1, int j0, int j1) const
{
    int d1 = i1 - i0 + 1;  // new dim1
    int d2 = j1 - j0 + 1;  // new dim2
    
    /* If either length is zero or negative, this is an invalide
        subscheme. Return a NULL scheme. */
    if (d1<1 || d2<1)
        return Scheme2D<T>();  // NULL scheme

    return Scheme2D<T> (d1, d2, &(*this)[i0][j0], stride_);
}

#endif  // Scheme2D_hpp

// END OF FILE

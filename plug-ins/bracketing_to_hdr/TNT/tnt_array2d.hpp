/*
*
* Template Numerical Toolkit (TNT)
*
* Mathematical and Computational Sciences Division
* National Institute of Technology,
* Gaithersburg, MD USA
*
*
* This software was developed at the National Institute of Standards and
* Technology (NIST) by employees of the Federal Government in the course
* of their official duties. Pursuant to title 17 Section 105 of the
* United States Code, this software is not subject to copyright protection
* and is in the public domain. NIST assumes no responsibility whatsoever for
* its use by other parties, and makes no guarantees, expressed or implied,
* about its quality, reliability, or any other characteristic.
*
*/
/*
  Changes && comments by Hartmut Sbosny marked by "HS". 
  
  Adds:
   o Debug output improved: type infos and ref counter output included
   o Function `is_empty()' added
   o Function `getArray1D()' added
  Bug Fixes:
   o `n' -> `n_' in subarray()
*/

#ifndef TNT_ARRAY2D_H
#define TNT_ARRAY2D_H

#include <cstdlib>
#include <iostream>

#ifdef TNT_BOUNDS_CHECK
#include <assert.h>
#endif

#include "tnt_array1d.hpp"

namespace TNT
{

template <class T>
class Array2D
{

  private:

    Array1D<T> data_;                   // HS: 1d-array of T's
    Array1D<T*> v_;                     // HS: 1d-array of T-pointer
    int m_;                             // HS: dim1, number of rows
    int n_;                             // HS: dim2, number of columns

  public:

    typedef T   value_type;

            Array2D();
            Array2D(int m, int n);
            Array2D(int m, int n,  T* a);
            Array2D(int m, int n, const T &a);
    inline  Array2D(const Array2D &A);          // HS: Copy-Ctor

    inline  operator       T**();               // HS: Cast-Op
    inline  operator const T**();               // HS: Cast-Op
    inline  Array2D& operator= (const T &a);
    inline  Array2D& operator= (const Array2D &A);
    inline  Array2D& ref(const Array2D &A);
            Array2D  copy() const;
            Array2D& inject(const Array2D &A);
    inline        T* operator[] (int i);
    inline  const T* operator[] (int i) const;
    inline  int dim1() const;
    inline  int dim2() const;
            ~Array2D();


    /* extended interface (not part of the standard) */

    inline  int     ref_count();                        
    inline  int     ref_count_data();
    inline  int     ref_count_dim1();
            Array2D subarray(int i0, int i1, int j0, int j1);
            
    inline  bool    is_empty() const {return dim1()==0;}  // Added by HS
    inline  const T* data() const    {return data_;}	  // Added by HS
    inline  Array1D<T>  getArray1D() const;		  // Added by HS
};


template <class T>
Array2D<T>::Array2D() : data_(), v_(), m_(0), n_(0) {}

template <class T>                      // HS: Copy-Ctor: no alloc
Array2D<T>::Array2D (const Array2D<T> & A)
    : data_(A.data_),                   // HS: Copy-Ctor: ref-counting
      v_(A.v_),                         // HS: Copy-Ctor: ref-counting
      m_(A.m_), n_(A.n_) 
{
#ifdef TNT_DEBUG
    std::cout << "Array2D<" << typeid(T).name() 
              << ">::Copy-Ctor(n=" << A.n_ << ") Refs: data=" 
              << ref_count_data() << ", dim1=" << ref_count_dim1() << endl;
#endif
}



template <class T>                      // HS: Init-Ctor: alloc
Array2D<T>::Array2D (int m, int n)
    : data_(m*n),               // HS: Init-Ctor: alloc for m*n T's
      v_(m),                    // HS: Init-Ctor: alloc for m T-pointers
      m_(m), n_(n)
{
    if (m>0 && n>0)
    {
        T* p = &(data_[0]);
        for (int i=0; i<m; i++)
        {
            v_[i] = p;          // HS: Assigns v_[i] the address of row i.
            p += n;
        }
    }
}



template <class T>
Array2D<T>::Array2D (int m, int n, const T & val)
    : data_(m*n),
      v_(m),
      m_(m), n_(n)
{
    if (m>0 && n>0)
    {
        data_ = val;            // HS: Sets all data to value `val'
        T* p  = &(data_[0]);
        for (int i=0; i<m; i++)
        {
            v_[i] = p;
            p += n;
        }
      }
}

template <class T>                              // HS: C-array-Ctor
Array2D<T>::Array2D (int m, int n, T* a)
    : data_(m*n, a),            // HS: C-array-Ctor: *no* alloc
      v_(m),                    // HS: Init-Ctor: *Alloc* row pointer array!
      m_(m), n_(n)
{
    if (m>0 && n>0)
    {
        T* p = &(data_[0]);

        for (int i=0; i<m; i++)
        {
            v_[i] = p;
            p += n;
        }
      }
}


template <class T>
inline T* Array2D<T>::operator[] (int i)
{
#ifdef TNT_BOUNDS_CHECK
    assert(i >= 0);
    assert(i < m_);
#endif

    return v_[i];               // HS: address of row i.
}


template <class T>
inline const T* Array2D<T>::operator[] (int i) const
{
#ifdef TNT_BOUNDS_CHECK
    assert(i >= 0);
    assert(i < m_);
#endif

    return v_[i];
}

template <class T>                          // HS: set all values to `a'
Array2D<T> & Array2D<T>::operator= (const T & a)
{
    /* non-optimzied, but will work with subarrays in future versions */

    for (int i=0; i<m_; i++)
        for (int j=0; j<n_; j++)
            v_[i][j] = a;

    return *this;
}




template <class T>                          // HS: delivers copied Array2D
Array2D<T> Array2D<T>::copy() const
{
    Array2D A(m_, n_);                      // HS: Init-Ctor: alloc

    for (int i=0; i<m_; i++)
        for (int j=0; j<n_; j++)
            A[i][j] = v_[i][j];

    return A;
}


template <class T>                          // HS: inject in representation
Array2D<T> & Array2D<T>::inject (const Array2D &A)
{
    if (A.m_ == m_ &&  A.n_ == n_)
    {
        for (int i=0; i<m_; i++)
            for (int j=0; j<n_; j++)
                v_[i][j] = A[i][j];
    }
    return *this;
}



/*!  Makes "me" to a new ref of the data referenced by \a A. */
template <class T>
Array2D<T> & Array2D<T>::ref (const Array2D<T> &A)
{
    if (this != &A)
    {
        v_ = A.v_;          // HS: handles ref counting and destroying
        data_ = A.data_;    // HS: handles ref counting and destroying
        m_ = A.m_;
        n_ = A.n_;

    }
    return *this;
}



template <class T>                              // HS: =-Op: ref-semantic
Array2D<T> & Array2D<T>::operator= (const Array2D<T> &A)
{
    return ref(A);
}

template <class T>
inline int Array2D<T>::dim1() const { return m_; }

template <class T>
inline int Array2D<T>::dim2() const { return n_; }


template <class T>
Array2D<T>::~Array2D() {}       // HS: destroying managed by the elements
                                //     `data_' und `v_'



template <class T>
inline Array2D<T>::operator T**()
{
    return &(v_[0]);
}
template <class T>
inline Array2D<T>::operator const T**()
{
    return &(v_[0]);
}

/* ............... extended interface ............... */
/**
    Create a new view to a subarray defined by the boundaries
    [i0][i0] and [i1][j1].  The size of the subarray is
    (i1-i0) by (j1-j0).  If either of these lengths are zero
    or negative, the subarray view is null.

*/
template <class T>
Array2D<T> Array2D<T>::subarray(int i0, int i1, int j0, int j1)
{
    Array2D<T> A;                           // HS: null array
    int m = i1-i0+1;
    int n = j1-j0+1;

    /* if either length is zero or negative, this is an invalide
        subarray. return a null view.
    */
    if (m<1 || n<1)
        return A;

    A.data_ = data_;                        // HS: `A' gets "my" data-Repr (ref-
    A.m_ = m;                               //   counting by =-Op of `data_')
    A.n_ = n;
    A.v_ = Array1D<T*>(m);                  // HS: `A' gets its own pointer
    T* p = &(data_[0]) + i0 * n_ + j0;      //   array, ie. with its own ref-
    for (int i=0; i<m; i++)                 //   counting
    {
        A.v_[i] = p + i*n_;                 // HS: the new row pointers
    }                                       //   Fixed: n -> n_   
    return A;
}

template <class T>
inline int Array2D<T>::ref_count()
{
    return ref_count_data();
}

template <class T>
inline int Array2D<T>::ref_count_data()
{
    return data_.ref_count();
}

template <class T>
inline int Array2D<T>::ref_count_dim1()
{
    return v_.ref_count();
}


/*!  Returns an Array1D handle of "my" contiguous data block \a data_. This
   increases the ref counter of \a data_, but not of the pointer array \a v_.
   Interesting for someone who wants to hold only the pure data block without
   the pointer array \a v_. The dim() of the Array1D is the dim of the 
   underlying contiguous data block; this will differ from dim1()*dim2() if
   "I" am a subarray. [HS] */
template <class T>
inline Array1D<T> Array2D<T>::getArray1D() const
{
    return Array1D<T>(data_);  // Ref counting copy
}

/*   HS: data_.ref_count() and v_.ref_count() can be different due to
   subarray(), in which an Array2d instance is created using the same `data_'
   presentation as `this' (==> *this.data_.(*ref)++), but with own `v_' 
   representation, i.e *this.v_.(*ref) remains invariant and 
   subarray.v_.(*ref) = 1.
*/


} /* namespace TNT */

#endif
/* TNT_ARRAY2D_H */


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

/** 
 * With comments by Hartmut.Sbosny@gmx.de, marked by "HS".
 *
 * HS: Here no more (as in Lpack++) the problem of double dereferencing to data
 *  on heap, because the representation class (i_refvec<T> v_) is hold as
 *  value (object) in the "handle class" Array1D, no more as pointer (to an
 *  object). 
 *  So    ``Array1D[i]'' <=> ``this*.v_[i]'' <=> ``this*.v_.data_[i]'',
 *  not   ``Array1D[i]'' <=> ``this*.p->data[i]''.
 *
 * HS: The class element `i_refvec<T> v_' holds the whole allocated memory, 
 *   and Array1D with its own pointer `data_' can refer to a part of it
 *   (subarrays).
 *
 * Adds by HS:
 *  - Debug output improved: type infos and ref counter output included
 *  - Function `is_empty()' added
 */
#ifndef TNT_ARRAY1D_H
#define TNT_ARRAY1D_H


#include <cstdlib>
#include <iostream>     // HS: needed for TNT_DEBUG

#ifdef TNT_BOUNDS_CHECK
#include <assert.h>
#endif

#ifdef TNT_DEBUG        
#include <typeinfo>     // added by HS to improve debug output
#endif


#include "tnt_i_refvec.hpp"

namespace TNT
{

template <class T>
class Array1D
{
  
  private:

    i_refvec<T> v_;
    int n_;
    T* data_;           /* this normally points to v_.begin(), but could
                         * also point to a portion (subvector) of v_. */
                        // HS: But not needed to avoid double dereferencing
                        // as it could look like too.
                        
    void copy_(T* p, const T* q, int len) const;
    void set_ (T* begin, T* end, const T& val);


  public:

    typedef  T   value_type;


             Array1D();                             // HS: Default-Ctor
    explicit Array1D(int n);                        // HS: Init-Ctor: ALLOC
             Array1D(int n, const T &a);            // HS: Init with setting
             Array1D(int n,  T *a);                 // HS: C-array-Ctor
    inline   Array1D(const Array1D &A);             // HS: Copy-Ctor

    inline   operator T*();                         // HS: Cast-Op
    inline   operator const T*() const;             // HS: Cast-Op, `const' added
    inline   Array1D & operator= (const T &a);      // HS: Set all elem to `a'
    inline   Array1D & operator= (const Array1D &A);  // HS: =-Op
    inline   Array1D & ref (const Array1D &A);
             Array1D   copy() const;
             Array1D & inject (const Array1D & A);
    inline         T& operator[] (int i);
    inline   const T& operator[] (int i) const;
    inline   int dim1() const;
    inline   int dim() const;
             ~Array1D();


    /* ... extended interface ... */

    inline   int     ref_count() const;
    inline   Array1D subarray(int i0, int i1);
    
    inline   bool is_empty() const  {return dim()==0;}      // Added by HS
    inline   bool is_null() const   {return v_.is_null();}  // Added by HS
    inline   const T* data() const  {return data_;}         // Added by HS
};




template <class T>                          // HS: Default-Ctor
Array1D<T>::Array1D() : v_(), n_(0), data_(0) {}


template <class T>                          // HS: Copy-Ctor
Array1D<T>::Array1D (const Array1D<T> &A)
    : v_(A.v_),                             // HS: Copy-Ctor of i_refvec,
      n_(A.n_), data_(A.data_)              //   i.e. ref-counting, no alloc
{
#ifdef TNT_DEBUG
    std::cout << "Array1D<" << typeid(T).name() 
              << ">::Copy-Ctor(n=" << A.n_ << ") refs=" << ref_count() << '\n';
#endif
}


template <class T>                          // HS: Init-Ctor: alloc,
Array1D<T>::Array1D (int n)                 //   values uninitialized
    : v_(n),                                // HS: Init-Ctor of i_refvec:
      n_(n), data_(v_.begin())              //   alloc
{
#ifdef TNT_DEBUG
    std::cout << "Array1D<" << typeid(T).name() 
              << ">::Ctor(n=" << n << "): allocated.\n";
#endif
}

template <class T>                          // HS: Init-Ctor: alloc,
Array1D<T>::Array1D (int n, const T &val)   //   values set to `val'
    : v_(n),                                // HS: alloc.
      n_(n), data_(v_.begin())
{
#ifdef TNT_DEBUG
    std::cout << "Array1D<" << typeid(T).name() 
              << ">::Ctor(n=" << n << "): allocated. Setting...\n";
#endif
    set_(data_, data_+ n, val);

}

template <class T>                          // HS: C-array-Ctor, no alloc
Array1D<T>::Array1D (int n, T *a)
    : v_(a),                                // HS: no ref-counting
      n_(n) , data_(v_.begin())
{
#ifdef TNT_DEBUG
    std::cout << "Array1D<" << typeid(T).name() 
              << ">::C-array-Ctor(n=" << n 
              << "): Only pointer allocation.\n";
#endif
}

template <class T>                          // HS: Cast to C-array
inline Array1D<T>::operator T*()
{
    return &(v_[0]);
}


template <class T>                          // HS: Cast to const C-array
inline Array1D<T>::operator const T*() const
{
    return &(v_[0]);
}



template <class T>
inline T& Array1D<T>::operator[](int i)
{
#ifdef TNT_BOUNDS_CHECK
    assert(i>= 0);
    assert(i < n_);
#endif
    return data_[i];
}

template <class T>
inline const T& Array1D<T>::operator[](int i) const
{
#ifdef TNT_BOUNDS_CHECK
    assert(i>= 0);
    assert(i < n_);
#endif
    return data_[i];
}



template <class T>
Array1D<T> & Array1D<T>::operator= (const T &a)
{
    set_(data_, data_+n_, a);
    return *this;
}

template <class T>
Array1D<T> Array1D<T>::copy() const
{
    Array1D A( n_);                         // HS: alloc a new Array1D
    copy_(A.data_, data_, n_);

    return A;
}


template <class T>
Array1D<T> & Array1D<T>::inject (const Array1D &A)
{
    if (A.n_ == n_)                         // HS: And if not??
        copy_(data_, A.data_, n_);

    return *this;
}





template <class T>
Array1D<T> & Array1D<T>::ref (const Array1D<T> &A)
{
    if (this != &A)
    {
        v_ = A.v_;        /* operator= handles the reference counting. */
        n_ = A.n_;
        data_ = A.data_;

    }
    return *this;
}

template <class T>                          // HS: =-Op: ref-semantic
Array1D<T> & Array1D<T>::operator= (const Array1D<T> &A)
{
    return ref(A);
}

template <class T>
inline int Array1D<T>::dim1() const { return n_; }

template <class T>
inline int Array1D<T>::dim() const { return n_; }

template <class T>
Array1D<T>::~Array1D() {}          // HS: destroying handled by the object `v_'

/* ............................ exented interface ......................*/

template <class T>
inline int Array1D<T>::ref_count() const
{
    return v_.ref_count();
}

template <class T>
inline Array1D<T> Array1D<T>::subarray(int i0, int i1)
{
    if ((i0 > 0) && (i1 < n_) || (i0 <= i1))
    {
        Array1D<T> X(*this);    /* create a new instance of this array. */
        X.n_ = i1-i0+1;         // HS: ie. a new *ref*-instance, no copy!
        X.data_ += i0;

        return X;
    }
    else
    {
        return Array1D<T>();        // HS: Null array, no ref to `this'
    }
}


/* private internal functions */


template <class T>
void Array1D<T>::set_(T* begin, T* end, const T& a)
{
    for (T* p=begin; p<end; p++)
        *p = a;

}

template <class T>
void Array1D<T>::copy_(T* p, const T* q, int len) const
{
    T *end = p + len;
    while (p<end )
        *p++ = *q++;

    // HS: Why not `memcpy(p, q, len*sizeof(T))'?
    //   Because the T's not of neccessisty 'fundamental' and their
    //   own, overloaded =-Op could be needed.
}


} /* namespace TNT */

#endif
/* TNT_ARRAY1D_H */


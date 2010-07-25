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

// Comments by Hartmut Sbosny marked by "HS".
//  Formated to Tab-4. Type infos included in debug ouput.


#ifndef TNT_I_REFVEC_H
#define TNT_I_REFVEC_H

#include <cstdlib>
#include <iostream>

#ifdef TNT_BOUNDS_CHECK
#include <assert.h>
#endif

#ifdef TNT_DEBUG        
#include <typeinfo>     // added by HS to improve debug output
#endif


#ifndef NULL
#define NULL 0
#endif

namespace TNT
{
/*
    Internal representation of ref-counted array.  The TNT
    arrays all use this building block.

    <p>
    If an array block is created by TNT, then every time
    an assignment is made, the left-hand-side reference
    is decreased by one, and the right-hand-side reference
    count is increased by one.  If the array block was
    external to TNT, the refernce count is a NULL pointer
    regardless of how many references are made, since the
    memory is not freed by TNT.

    <p> HS:
    Remarkable: This class maintains the data-representation on heap,
    but doesn't hold an own dimension value; this is done not until
    Array1D, Array2D etc.
*/

template <class T>
class i_refvec
{

  private:
  
    T*      data_;
    int*    ref_count_;


  public:
               
               i_refvec();                          // HS: Default-Ctor,
    explicit   i_refvec(int n);                     // HS: Init-Ctor: ALLOC
    inline     i_refvec(T* data);                   // HS: C-array-Ctor
    inline     i_refvec(const i_refvec &v);         // HS: Copy-Ctor.

    inline         T*   begin();
    inline   const T*   begin() const;
    inline         T&   operator[] (int i);
    inline   const T&   operator[] (int i) const;
    inline   i_refvec<T> & operator= (const i_refvec<T> &V);
             void   copy_(T* p, const T* q, const T* e);
             void   set_ (T* p, const T* b, const T* e);   // HS: not impl??
    inline   int    ref_count() const;
    inline   int    is_null() const;
    inline   void   destroy();
             ~i_refvec();
};

template <class T>
void i_refvec<T>::copy_(T* p, const T* q, const T* e)
{
    for (T* t=p; q<e; t++, q++)
        *t = *q;
}

template <class T>                              // HS: Default-Ctor
i_refvec<T>::i_refvec()
    : data_(NULL), ref_count_(NULL) {}

/**
    In case n is 0 or negative, it does NOT call new.
*/
template <class T>                              // HS: Init-Ctor
i_refvec<T>::i_refvec(int n)
    : data_(NULL), ref_count_(NULL)             // HS: wasted if (n>0)
{
    if (n >= 1)
    {
#ifdef TNT_DEBUG_1
    std::cout << "\ti_refvec<" << typeid(T).name() 
              << ">::Ctor(n=" << n << "): allocating...\n";
#endif
        data_ = new T[n];
        ref_count_ = new int;
        *ref_count_ = 1;
    }
}

template <class T>                              // HS: Copy-Ctor
inline     i_refvec<T>::i_refvec(const i_refvec<T> &V)
    : data_(V.data_),                           // HS: copies pointer
      ref_count_(V.ref_count_)                  // HS: copies pointer
{
    if (V.ref_count_ != NULL)
        (*(V.ref_count_))++;                    // HS: ref-counting
}


template <class T>                              // HS: C-array-Ctor
i_refvec<T>::i_refvec(T* data)
    : data_(data), ref_count_(NULL) {}          // HS: no ref-counting


template <class T>
inline T* i_refvec<T>::begin()
{
    return data_;
}

template <class T>
inline const T* i_refvec<T>::begin() const
{
    return data_;
}

template <class T>
inline const T& i_refvec<T>::operator[](int i) const
{
    return data_[i];
}

template <class T>
inline T& i_refvec<T>::operator[](int i)
{
    return data_[i];
}

template <class T>
i_refvec<T> & i_refvec<T>::operator= (const i_refvec<T> &V)
{
    if (this == &V)
        return *this;

#ifdef TNT_DEBUG
        std::cout << "i_refvec<" << typeid(T).name() 
                  << ">::=-Op: ref_counts: my old=" << ref_count()
                  << ", his old=" << V.ref_count() << '\n';
#endif
    
    if (ref_count_ != NULL)
    {
        (*ref_count_) --;
        if ((*ref_count_) == 0)
            destroy();
    }

    data_ = V.data_;
    ref_count_ = V.ref_count_;

    if (V.ref_count_ != NULL)
        (*(V.ref_count_))++;

#ifdef TNT_DEBUG_1
        std::cout << "...FINIS: i_refvec<" << typeid(T).name() 
                  << ">::=-Op: our new ref_count=" << ref_count() << '\n';
#endif
    
    return *this;
}

template <class T>
void i_refvec<T>::destroy()
{
    if (ref_count_ != NULL)
    {
#ifdef TNT_DEBUG
        std::cout << "i_refvec<" << typeid(T).name() 
                  << ">::destroy()ing data...\n";
#endif
        delete ref_count_;

#ifdef TNT_DEBUG_1
        std::cout << "\tref_count_ deleted\n";
#endif
        if (data_ != NULL)
            delete []data_;
#ifdef TNT_DEBUG_1
        std::cout << "\tdata_[] deleted\n";
#endif
        data_ = NULL;                           // HS: for is_null()
    }
}

/*
*   return 1 if vector is empty, 0 otherwise
*/
template<class T>
int i_refvec<T>::is_null() const
{
    return (data_ == NULL ? 1 : 0);
}

/*
*   returns -1 if data is external,
*   returns 0 if a is NULL array,
*   otherwise returns the positive number of vectors sharing
*       this data space.
*/
template <class T>
int i_refvec<T>::ref_count() const
{
    if (data_ == NULL)
        return 0;
    else
        return (ref_count_ != NULL ? *ref_count_ : -1) ;
}

template <class T>
i_refvec<T>::~i_refvec()
{
    if (ref_count_ != NULL)
    {
        (*ref_count_)--;

#ifdef TNT_DEBUG_1
        std::cout << "\ti_refvec<" << typeid(T).name() 
                  << ">::Dtor(): new ref_count="<<ref_count()<<'\n';
#endif
        
        if (*ref_count_ == 0)
            destroy();
    }
}


} /* namespace TNT */

#endif
/* TNT_I_REFVEC_H */


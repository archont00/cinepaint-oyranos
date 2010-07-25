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
  Adds and some changes by Hartmut Sbosny, marked by "HS".

  Changes:
   o `<<'-Operator (ostream): formating (slightly) improved and 
      typeinfos included

  Adds:
   o uchar specialization for ostream
   o min(), max()
   o dot product
   o Euclidian norm
   o sort()  (primitive)
*/
#ifndef TNT_ARRAY1D_UTILS_H
#define TNT_ARRAY1D_UTILS_H


#include <cstdlib>
#include <cassert>
#include <typeinfo>

#include "tnt_array1d.hpp"
#include "tnt_math_utils.hpp"    // dot()


namespace TNT
{


template <class T>
std::ostream& operator<<(std::ostream &s, const Array1D<T> &A)
{
    int N=A.dim1();

#ifdef TNT_DEBUG_1
    s << "addr: " << (void *) &A[0] << '\n';
#endif
    s << "Array1D<" << typeid(T).name()<< "> [ " << N << " ]:\n";
    for (int j=0; j<N; j++)
    {
        s << " [" << j << "]\t" << A[j] << '\n';
    }

    return s;
}

//-------------------------------------------------------------------
// HS: Spezialisierung fuer unsigned chars, die ueblicherweise nicht als
//  chars, sondern als Zahlen ausgegeben werden sollen; der Einfachheit
//  hier inline, da -- vollstaendige Spezialisierung! -- sonst *.cpp Datei
//  vonnoeten. Lohnt fuer so eine Funktion nicht.
//-------------------------------------------------------------------
template <class T>
std::ostream& operator<<(std::ostream &s, const Array1D<unsigned char> &A)
{
    int N=A.dim1();

#ifdef TNT_DEBUG_1
    s << "addr: " << (void *) &A[0] << '\n';
#endif
    s << "Array1D<" << typeid(unsigned char).name()<< "> [ " << N << " ]:\n";
    for (int j=0; j<N; j++)
    {
        s << " [" << j << "]\t" << (int)A[j] << '\n';
    }

    return s;
}


template <class T>
std::istream& operator>>(std::istream &s, Array1D<T> &A)
{
    int N;
    s >> N;

    Array1D<T> B(N);
    for (int i=0; i<N; i++)
        s >> B[i];
    A = B;
    return s;
}



template <class T>
Array1D<T> operator+(const Array1D<T> &A, const Array1D<T> &B)
{
    int n = A.dim1();

    if (B.dim1() != n )
        return Array1D<T>();

    else
    {
        Array1D<T> C(n);

        for (int i=0; i<n; i++)
        {
            C[i] = A[i] + B[i];
        }
        return C;
    }
}



template <class T>
Array1D<T> operator-(const Array1D<T> &A, const Array1D<T> &B)
{
    int n = A.dim1();

    if (B.dim1() != n )
        return Array1D<T>();

    else
    {
        Array1D<T> C(n);

        for (int i=0; i<n; i++)
        {
            C[i] = A[i] - B[i];
        }
        return C;
    }
}


template <class T>
Array1D<T> operator*(const Array1D<T> &A, const Array1D<T> &B)
{
    int n = A.dim1();

    if (B.dim1() != n )
        return Array1D<T>();

    else
    {
        Array1D<T> C(n);

        for (int i=0; i<n; i++)
        {
            C[i] = A[i] * B[i];
        }
        return C;
    }
}


template <class T>
Array1D<T> operator/(const Array1D<T> &A, const Array1D<T> &B)
{
    int n = A.dim1();

    if (B.dim1() != n )
        return Array1D<T>();

    else
    {
        Array1D<T> C(n);

        for (int i=0; i<n; i++)
        {
            C[i] = A[i] / B[i];
        }
        return C;
    }
}


template <class T>
Array1D<T>&  operator+=(Array1D<T> &A, const Array1D<T> &B)
{
    int n = A.dim1();

    if (B.dim1() == n)
    {
        for (int i=0; i<n; i++)
        {
                A[i] += B[i];
        }
    }
    return A;
}


template <class T>
Array1D<T>&  operator-=(Array1D<T> &A, const Array1D<T> &B)
{
    int n = A.dim1();

    if (B.dim1() == n)
    {
        for (int i=0; i<n; i++)
        {
            A[i] -= B[i];
        }
    }
    return A;
}


template <class T>
Array1D<T>&  operator*=(Array1D<T> &A, const Array1D<T> &B)
{
    int n = A.dim1();

    if (B.dim1() == n)
    {
        for (int i=0; i<n; i++)
        {
            A[i] *= B[i];
        }
    }
    return A;
}


template <class T>
Array1D<T>&  operator/=(Array1D<T> &A, const Array1D<T> &B)
{
    int n = A.dim1();

    if (B.dim1() == n)
    {
        for (int i=0; i<n; i++)
        {
            A[i] /= B[i];
        }
    }
    return A;
}




//==========
// HS adds:
//==========


// min-max...
 
template <class T>
T min (const Array1D<T>& A, int* k)
{
    T a = A[0];
    *k = 0;

    for (int i=1; i<A.dim(); i++)
        if (A[i] < a)
        {
            a=A[i]; *k=i;
        }
    return a;
}

template <class T>
T max (const Array1D<T>& A, int* k)
{
    T a = A[0];
    *k = 0;

    for (int i=1; i<A.dim(); i++)
        if (A[i] > a)
        {
            a=A[i]; *k=i;
        }
    return a;
}

template <class T>
T min_abs (const Array1D<T>& A, int* k)
{
    T a = abs(A[0]);
    *k = 0;

    for (int i=1; i<A.dim(); i++)
    {
        T aa = abs(A[i]);
        if (aa < a)
        {
            a=aa; *k=i;
        }
    }
    return a;
}

template <class T>
T max_abs (const Array1D<T>& A, int* k)
{
    T a = abs(A[0]);
    *k = 0;

    for (int i=1; i<A.dim(); i++)
    {
        T aa = abs(A[i]);
        if (aa > a)
        {
            a=aa; *k=i;
        }
    }
    return a;
}


template <class T>
bool operator==(const Array1D<T> &A, const Array1D<T> &B)
{
    int n = A.dim();
    
    if (B.dim() != n) return false;
    
    for (int i=0; i<n; i++)
    {
        if (A[i] != B[i]) return false;
    }
    return true;
}


/*
  Scalar product (dot-product, inner product) of two Array1D's.

   o Assume contiguous storing of data.
   o Primitve version without any attention to overflow/underflow or
      cascade summation.
*/
template <class T>
T dot (const Array1D<T> & A, const Array1D<T> & B)
{
    assert (A.dim() == B.dim());

    return dot (A.dim(), &A[0], &B[0]);        // chrashes for Null-Arrays

#if 0
    const T* a = &A[0];                        // chrashes for Null-Arrays
    const T* b = &B[0];
    const T* end = a + A.dim();
    T sum = 0;
    while (a < end)
        sum +=    *a++ * *b++;

    return sum;
#endif
}


template <class T>
T norm2 (const Array1D<T> & A, const Array1D<T> & B)
{
    assert (A.dim() == B.dim());

    return sqrt(dot(A,B));
}



/*
 * Primitivsortierung...
 */ 
template <class T>
void sort (Array1D<T>& A, int begin, int end)
{
    for (int i=begin; i < end-1; i++)
        for (int j=i+1; j < end; j++)
            if (A[j] < A[i])
                swap (A[j], A[i]);
}



} // namespace TNT

#endif

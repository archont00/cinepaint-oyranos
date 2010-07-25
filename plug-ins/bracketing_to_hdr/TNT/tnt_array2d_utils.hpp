/*
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
  Adds und some changes by Hartmut Sbosny, marked by "HS".

  Changes:
   o `<<'-Operator (ostream): formating (slightly) improved; 
     typeinfos included;

  Adds:
   o uchar specialization for ostream 
   o max()
   o mat_dot_vec(), transpose()...
   o list()  (out "0" for `x < eps')
*/
#ifndef TNT_ARRAY2D_UTILS_H
#define TNT_ARRAY2D_UTILS_H


#include <cstdlib>
#include <cassert>
#include <typeinfo>

#include "tnt_array2d.hpp"
#include "tnt_math_utils.hpp"    // dot()

namespace TNT
{


template <class T>
std::ostream& operator<<(std::ostream &s, const Array2D<T> &A)
{
    int M=A.dim1();
    int N=A.dim2();

    s << "Array2D<" << typeid(T).name()<< "> [ " 
      << M << " x " << N << " ]:\n";

    for (int i=0; i<M; i++)
    {
        s << " [" << i << "]\t";
        for (int j=0; j<N; j++)
        {
            s << A[i][j] << " ";
        }
        s << "\n";
    }

    return s;
}

//-------------------------------------------------------------------
// HS: Spezialisierung fuer unsigned chars, die ueblicherweise nicht als
//  chars, sondern als Zahlen ausgegeben werden sollen; der Einfachheit
//  hier inline, da -- vollstaendige Spezialisierung! -- sonst *.cpp Datei
//  vonnoeten. Lohnt fuer so eine Funktion nicht.
//-------------------------------------------------------------------
template <> inline
std::ostream& operator<<(std::ostream &s, const Array2D<unsigned char> &A)
{
    int M=A.dim1();
    int N=A.dim2();

    s << "Array2D<" << typeid(unsigned char).name() << "> [ " 
      << M << " x " << N << " ]:\n";

    for (int i=0; i<M; i++)
    {
        s << " [" << i << "]\t";
        for (int j=0; j<N; j++)
        {
            s << (int)A[i][j] << " ";
        }
        s << "\n";
    }

    return s;
}



template <class T>
std::istream& operator>>(std::istream &s, Array2D<T> &A)
{
    int M, N;

    s >> M >> N;

    Array2D<T> B(M,N);            // HS: creates new Array2D

    for (int i=0; i<M; i++)
        for (int j=0; j<N; j++)
        {
            s >>  B[i][j];
        }

    A = B;
    return s;
}


template <class T>
Array2D<T> operator+(const Array2D<T> &A, const Array2D<T> &B)
{
    int m = A.dim1();
    int n = A.dim2();

    if (B.dim1() != m ||  B.dim2() != n )
        return Array2D<T>();

    else
    {
        Array2D<T> C(m,n);

        for (int i=0; i<m; i++)
        {
            for (int j=0; j<n; j++)
                C[i][j] = A[i][j] + B[i][j];
        }
        return C;
    }
}


template <class T>
Array2D<T> operator-(const Array2D<T> &A, const Array2D<T> &B)
{
    int m = A.dim1();
    int n = A.dim2();

    if (B.dim1() != m ||  B.dim2() != n )
        return Array2D<T>();

    else
    {
        Array2D<T> C(m,n);

        for (int i=0; i<m; i++)
        {
            for (int j=0; j<n; j++)
                C[i][j] = A[i][j] - B[i][j];
        }
        return C;
    }
}


template <class T>
Array2D<T> operator*(const Array2D<T> &A, const Array2D<T> &B)
{
    int m = A.dim1();
    int n = A.dim2();

    if (B.dim1() != m ||  B.dim2() != n )
        return Array2D<T>();

    else
    {
        Array2D<T> C(m,n);

        for (int i=0; i<m; i++)
        {
            for (int j=0; j<n; j++)
                C[i][j] = A[i][j] * B[i][j];
        }
        return C;
    }
}


template <class T>
Array2D<T> operator/(const Array2D<T> &A, const Array2D<T> &B)
{
    int m = A.dim1();
    int n = A.dim2();

    if (B.dim1() != m ||  B.dim2() != n )
        return Array2D<T>();

    else
    {
        Array2D<T> C(m,n);

        for (int i=0; i<m; i++)
        {
            for (int j=0; j<n; j++)
                C[i][j] = A[i][j] / B[i][j];
        }
        return C;
    }
}



template <class T>
Array2D<T>&  operator+=(Array2D<T> &A, const Array2D<T> &B)
{
    int m = A.dim1();
    int n = A.dim2();

    if (B.dim1() == m ||  B.dim2() == n )
    {
        for (int i=0; i<m; i++)
        {
            for (int j=0; j<n; j++)
                A[i][j] += B[i][j];
        }
    }
    return A;
}


template <class T>
Array2D<T>&  operator-=(Array2D<T> &A, const Array2D<T> &B)
{
    int m = A.dim1();
    int n = A.dim2();

    if (B.dim1() == m ||  B.dim2() == n )
    {
        for (int i=0; i<m; i++)
        {
            for (int j=0; j<n; j++)
                A[i][j] -= B[i][j];
        }
    }
    return A;
}


template <class T>
Array2D<T>&  operator*=(Array2D<T> &A, const Array2D<T> &B)
{
    int m = A.dim1();
    int n = A.dim2();

    if (B.dim1() == m ||  B.dim2() == n )
    {
        for (int i=0; i<m; i++)
        {
            for (int j=0; j<n; j++)
                A[i][j] *= B[i][j];
        }
    }
    return A;
}


template <class T>
Array2D<T>&  operator/=(Array2D<T> &A, const Array2D<T> &B)
{
    int m = A.dim1();
    int n = A.dim2();

    if (B.dim1() == m ||  B.dim2() == n )
    {
        for (int i=0; i<m; i++)
        {
            for (int j=0; j<n; j++)
                A[i][j] /= B[i][j];
        }
    }
    return A;
}

/**
    Matrix Multiply:  compute C = A*B, where C[i][j]
    is the dot-product of row i of A and column j of B.

    @param A an (m x n) array
    @param B an (n x k) array
    @return the (m x k) array A*B, or a null array (0x0)
        if the matrices are non-conformant (i.e. the number
        of columns of A are different than the number of rows of B.)
*/
template <class T>
Array2D<T> matmult(const Array2D<T> &A, const Array2D<T> &B)
{
    if (A.dim2() != B.dim1())
        return Array2D<T>();

    int M = A.dim1();
    int N = A.dim2();
    int K = B.dim2();

    Array2D<T> C(M,K);                  // HS: create new

    for (int i=0; i<M; i++)
        for (int j=0; j<K; j++)
        {
            T sum = 0;                  // dot product of row i of A
                                        // and column j of B
            for (int k=0; k<N; k++)
                sum += A[i][k] * B [k][j];

            C[i][j] = sum;
        }

    return C;
}



// =======
// My Adds:
// =======

/*  Namensalternativen:
        mat_mul_mat(),     mat_mul_vec()
        mat_mlt_mat(),     mat_mlt_vec()
        mat_m_mat(),       mat_m_vec()
        mat_x_mat(),       mat_x_vec(),
        mat_dot_mat(),     mat_dot_vec()
        m_dot_m(),         m_dot_v()

    Insofern die Ergebnistypen eindeutig von den Eingangstypen abhaengen,
    koennte hier auch ein einziger Namen fuer alle Kombination stehen,
    etwa "_dot_()" mit den Varianten T_dot_(), _dot_T() fuers Transponierte
    oder "dot()" mit den Varianten T_dot(), dot_T(). Oder weil nur Matrizen
    transponiert werden koennen und um Grossbuchstaben am Anfang zu vermeiden,
    fuers Transponierte mT_dot() und dot_mT(). Naja.

        Array1D<T> dot (const Array2D<T> &A, const Array1D<T> &v)
        Array2D<T> dot (const Array2D<T> &A, const Array2D<T> &B) etc.
*/

/**
    mat_dot_mat():
    Same as matmult(), for uniform naming only.
*/
template <class T> inline
Array2D<T> mat_dot_mat(const Array2D<T> &A, const Array2D<T> &B)
{
    return matmult (A,B);
}

/**
    Matrix Multiply: C = A*B^T, where B^T the transpose of B.

    @param A: an (m x n) array
    @param B: an (k x n) array
    @return: the (m x k) array A*B^T, or null array (0x0) if the matrices
        are non-conformant (i.e. the numbers of columns are different.)
*/
template <class T>
Array2D<T> mat_dot_matT (const Array2D<T> &A, const Array2D<T> &B)
{
    if (A.dim2() != B.dim2())
        return Array2D<T>();

    int M = A.dim1();
    int N = A.dim2();
    int K = B.dim1();

    Array2D<T> C(M,K);                      // create new

    for (int i=0; i<M; i++)
        for (int j=0; j<K; j++)
        {
            C[i][j] = dot (N, A[i], B[j]);
        }

    return C;
}

/**
    Matrix-Multiply: A^T * B, where A^T the transpose of A.

    @param A: an (m x n) array
    @param B: an (m x k) array
    @return: the (n x k) array A*B, or a null array (0x0) if the matrices
        are non-conformant (i.e. the numbers of rows are different.)
*/
template <class T>
Array2D<T> matT_dot_mat (const Array2D<T> &A, const Array2D<T> &B)
{
    if (A.dim1() != B.dim1())
        return Array2D<T>();

    int M = A.dim1();
    int N = A.dim2();
    int K = B.dim1();

    Array2D<T> C(N,K);                      // create new

    for (int i=0; i<N; i++)
        for (int j=0; j<K; j++)
        {
            T sum = 0;                      // dot product of row i of A
                                            // and row j of B; equiv. to
            for (int k=0; k<M; k++)         // dot(M,&A[i],N,&B[j],K)
                sum += A[k][i] * B[k][j];

            C[i][j] = sum;
        }

    return C;
}


/**
    Matrix-mal-Vector-Multipliaktionen:

    Diese Varianten kreieren neues Array1D fuer Rueckgabe. Fuer Mehrfach-
    Operationen wie A*v+c dies nicht optimal. Dafuer koennte man
    Varianten bereitstellen, die den Zielvektor als Referenz uebergeben
    (der dann natuerlich vorher passend bereitzustellen waere):
        Array1D & mat_dot_vec(Array1D &x, const Array2D &A, const Array1D &v)
        {    //...
            return x;
        }
    Oder Drei-Argument-Funktionen mat_dot_vec_plus_vec(...).
*/
/**
    mat_dot_vec()

    Matrix-mal-Vektor-Multiplikation: x = A * v

    Die mit dem zeigerarithmetischen dot() arbeitende zweite Variante ist bei
    eingeschaltetem    TNT_BOUNDS_CHECK doppelt so schnell wie die erste
    (default), da die Ueberpruefung nur bei []-Zugriffen wirksam. Ohne
    BOUNDS_CHECK gleichschnell -- mit g++. Also g++ durch Zeigerarithmetik
    kaum zu beeindrucken. Dennoch zu Tests mit anderen Compileren beide
    Varianten aufbewahrt.
*/
template <class T>                      // creates a new Vector
Array1D<T> mat_dot_vec (const Array2D<T> & A, const Array1D<T> & V)
{
    int rows = A.dim1();
    int cols = A.dim2();

    if (cols != V.dim())
        return Array1D<T>();            // Null Array

    Array1D <T> R(rows);                // creates new

    for (int i=0; i < rows; i++)
    {
        T sum = 0;

        for (int j=0; j < cols; j++)    // dot(cols, A[i], &V[0])
            sum += A[i][j] * V[j];

        R[i] = sum;
    }
    return R;
}

template <class T>                      // creates a new Vector
Array1D<T> mat_dot_vec_1 (const Array2D<T> & A, const Array1D<T> & V)
{
    int rows = A.dim1();
    int cols = A.dim2();

    if (cols != V.dim())
        return Array1D<T>();            // Null Array

    Array1D <T> R(rows);                // creates new

    for (int i=0; i < rows; i++)
        R[i] = dot(cols, A[i], &V[0]);

    return R;
}

/**
    Matrix-by-Vector-Multiply: A^T v, where A^T the transpose of A.

    @param A: (m,n)-array
    @param v: (n)-array
    @return: the (n)-array `A^T * v' or a Null-array if the parameter are
        non-conformant, i.e. A.dim1() != v.dim().
*/
template <class T>                      // creates a new Vector
Array1D<T> matT_dot_vec (const Array2D<T> & A, const Array1D<T> & V)
{
    int rows = A.dim1();
    int cols = A.dim2();

    if (rows != V.dim())
        return Array1D<T>();            // Null Array

    Array1D <T> R(cols);                // creates new

    for (int i=0; i < cols; i++)
    {
        T sum = 0;                      // dot product of column i of A and V
                                        // dot(rows,&A[i],cols,V)
        for (int j=0; j < rows; j++)
            sum += A[j][i] * V[j];

        R[i] = sum;
    }
    return R;
}



/**
    transpose()        --    creates a new Array2D
/*/
template <class T>
Array2D<T> transpose (const Array2D<T> & A)
{
    int d1 = A.dim1();
    int d2 = A.dim2();

    Array2D <T> B(d2,d1);               // creates new

    for (int i=0; i < d1; i++)
        for (int j=0; j < d2; j++)
            B[j][i] = A[i][j];

    return B;
}


/**
    transposeIP()    --    "IP" == "in place": Transponieren in situ.

    Die Zeigerarithmetik in Variante 1 ist ohne O-Schalter zwar schneller
    (0.92 s vs 0.69 s), ab -O1 mit g++ aber nicht mehr (alle 0.64 s).
    Aber immerhin sehr nahe offenbar am g++-Optimum.
*/
template <class T>
Array2D<T> & transposeIP (Array2D<T> & A)
{
    assert (A.dim1() == A.dim2());      // for square matrices only

    int N = A.dim1();

    for (int i=1; i<N; i++)
        for (int j=0; j<i; j++)
            swap (A[i][j], A[j][i]);

    return A;
}

/** Zeigerarithmetik (spart in innerer Schleife extra j-Variable)
*/
template <class T>
Array2D<T> & transposeIP_1 (Array2D<T> & A)
{
    assert (A.dim1() == A.dim2());      // for square matrices only

    int N = A.dim1();

    for (int i=1; i<N; i++)
    {
        T* p     = A[i];                // begin of i-th row
        T* p_end = p + i;
        T* q     = &A[0][i];            // begin of i-th column
        do
        {   swap (*p,*q);
            p++;
            q += N;
        } while (p < p_end);
    }
    return A;
}


template <class T>
void list (Array2D<T> & A, T eps)
{
    int M=A.dim1();
    int N=A.dim2();

    std::cout << "Array2D [ " << M << " x " << N << " ]:\n";

    for (int i=0; i < M; i++)
    {
        std::cout << " [" << i << "]\t";
        for (int j=0; j < N; j++)
        {
            double a = A[i][j];

            std::cout.width(2);

            if (a > 0 && a < eps)       std::cout << "+0 ";
            else if (a < 0 && -a < eps) std::cout << "-0 ";
            else                        std::cout << A[i][j] << " ";
        }
        std::cout << "\n";
    }
}


/*
 * max()...
 */
template <class T>
T max (const Array2D<T>& A)
{
    T a = A[0][0];

    for (int i=0; i<A.dim1(); i++) 
    for (int k=0; k<A.dim2(); k++)
       if (A[i][k] > a)  a=A[i][k];

    return a;
}


} // namespace TNT

#endif  // TNT_ARRAY2D_UTILS_H

/**
  tnt_misc.hpp    --   Added by hartmut.sbosny@gmx.de ("HS")

  Dies und das. Wobei es sich idR. schon auf Array1D, Array2D... bezieht
   und bereits aus diesem Grund im allgemeineren "tnt_math_utils.hpp" nicht
   angebracht waere.
*/
#ifndef TNT_MISC_HPP
#define TNT_MISC_HPP

#include "tnt_array1d.hpp"
#include "tnt_array1d_utils.hpp"
#include "tnt_array2d.hpp"
#include "tnt_array2d_utils.hpp"
#include "tnt_math_utils.hpp"    // dot()


namespace TNT {

/**
    All rows are multiples of row[0]: row[i] = (i+1) row[0] and all
    columns multiples of column[0].
        [0]: 1 2 3 ...
        [1]: 2 4 6 ...
        [2]: 3 6 9 ... etc.
*/
template <class T>
void fill_rank_1 (Array2D <T> & A)
{
    int M = A.dim1();
    int N = A.dim2();

    for (int i=0; i < M; i++)
        for (int j=0; j < N; j++)
            A[i][j] = (2*i+1)*(j+1);
}

template <class T>
void fill_rank_1_sym (Array2D <T> & A)
{
    int M = A.dim1();
    int N = A.dim2();

    for (int i=0; i < M; i++)
        for (int j=0; j < N; j++)
            A[i][j] = (i+1)*(j+1);
}


// Strictly spoken for Reals only, because for integers `rand()/RAND_MAX'
//  gives always 0.
// The initialization of the random number genererator by srand() should
//  not be done inside these functions, because they produce the same 
//  numbers then in every call. Exept for debbuging mode.

template <class T>
void fill_random (Array1D <T> & A, T maxval)
{
    //srand(1);                    // init Zufallsgenerator

    for (int i=0; i<A.dim(); i++)
        A[i] = maxval * rand() / RAND_MAX;
}

template <class T>
void fill_random (Array2D <T> & A, T maxval)
{
    //srand(1);                    // init Zufallsgenerator

    for (int i=0; i<A.dim1(); i++)
        for (int j=0; j<A.dim2(); j++)
            A[i][j] = maxval * rand() / RAND_MAX;
}

template <class T>
void fill_rank_minor_1 (Array2D<T> & A)
{
    fill_random (A, 1.0);

    int m = A.dim1();
    int n = A.dim2();

    if (m <= n)         // produce two linear dependent rows
    {
        if (m < 2) return;

        // Set row 1 a multiple of row 0
        for (int i=0; i < n; i++)
            A[0][i] = A[1][i];
    }
    else                // m > n: produce two linear dependent columns
    {
        if (n < 2) return;

        // Set column 1 a multiple of column 0
        for (int i=0; i < m; i++)
            A[i][0] = A[i][1];
    }
}


/**
   check_ColOrtho(), check_RowOrtho():

   Eigentlich einfach C=A^T * A bzw. C=A*A^T, nur dass hier wegen der
    Symmetrie nur das obere Dreieck ausrechnet wird.

   Es koennte noch ein Guetewert ausgegeben werden.
*/
template <class Real>
Array2D<Real> check_ColOrtho (const Array2D<Real> A)
{
    int rows = A.dim1();
    int cols = A.dim2();

    Array2D<Real> C(cols,cols, Real(0));    // C = A^T * A

    for (int i=0; i<cols; i++)
    for (int j=i; j<cols; j++)
    {
        Real sum = 0;               // dot product of column i and j
        for (int k=0; k<rows; k++)
            sum += A[k][i] * A[k][j];

        C[i][j] = sum;
    }
    return C;
}

template <class Real>
Array2D<Real> check_RowOrtho (const Array2D<Real> A)
{
    int rows = A.dim1();
    int cols = A.dim2();

    Array2D<Real> C(rows,rows, Real(0));

    for (int i=0; i<rows; i++)
    for (int j=i; j<rows; j++)
    {
        Real sum = 0;               // dot product of row i and j
        for (int k=0; k<cols; k++)
            sum += A[i][k] * A[j][k];

        C[i][j] = sum;
    }
    return C;
}

template <class Real>
Array2D<Real> check_RowOrtho_1 (const Array2D<Real> A)
{
    int rows = A.dim1();

    Array2D<Real> C(rows,rows, Real(0));    // C = A * A^T

    for (int i=0; i<rows; i++)
    for (int j=i; j<rows; j++)
    {
        C[i][j] = dot (A.dim2(), &A[i], &A[j]);
    }
    return C;
}



/**
   proof_least_square_fit():

   Check, whether the fit solution x of Ax=b satisfies A^T A x = A^T b,
    as it should do. We calculate the Euclidian norm of A^T(Ax-b).
*/
template <class Real>
void proof_least_square_fit (
        const Array1D <Real> & x,       // the solution: (n)-vector
        const Array2D <Real> & A,       // the matrix: (m,n)-array
        const Array1D <Real> & b )      // the right side: (m)-vector
{
    if (x.dim() == 0) return;
    assert (x.dim() == A.dim2());
    assert (b.dim() == A.dim1());

    Array1D <Real> y,r;
    Real max, resid;
    int i_max;

    y = mat_dot_vec (A,x);              // y = Ax    (m)-vector
    y -= b;                             // y = Ax-b

    max = max_abs (y, &i_max);
    resid = sqrt( dot(y,y));
    std::cout << "Residium ||Ax-b|| = " << resid << "\n";
    std::cout << "\tmax Komponente: " << max << " at i=" << i_max << "\n";

    r = matT_dot_vec (A,y);             // r = A^T y    (n)-vector

    max = max_abs (r, &i_max);
    resid = sqrt( dot (r,r));
    std::cout << "Residium ||A^T(Ax-b)|| = " << resid << "\n";
    std::cout << "\tmax Komponente: " << max << " at i=" << i_max << "\n";
    //std::cout << "A^T * A = " << matT_dot_mat(A,A);
}



}  // namespace TNT
#endif  // TNT_MISC_HPP

// END OF FILE


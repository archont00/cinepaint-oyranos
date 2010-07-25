/*
 * ResponseSolver.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  @file ResponseSolver.hpp

  Contents:
   - Curve2D<>          (unused)
   - ResponseSolver<>
    
  Calculation of a response function.

*/
#ifndef ResponseSolver_hpp
#define ResponseSolver_hpp


#include <fstream>
#include <cstring>              // strlen
#include "TNT/tnt_misc.hpp"     // proof_least_square_fit()
#include "TNT/tnt_stopwatch.hpp"
#include "TNT/jama_qr.hpp"      // JAMA::QR
#include "TNT/jama_svd.hpp"     // JAMA::SVD

#include "WeightFuncBase.hpp"   // Tweight (type of weight values)
#include "FilePtr.hpp"          // FilePtr
#include "Exception.hpp"        // IF_FAIL_EXCEPTION()
#include "br_macros.hpp"
#include "ResponseSolverBase.hpp"


namespace br {

using namespace TNT;
 
/**======================================================================

  @class Curve2D  
  
  Template, currently not used.

*=======================================================================*/  
template <class Tx, class Ty>
class Curve2D 
{
public:
    
    Array1D <Tx>  X;
    Array1D <Ty>  Y;

    Curve2D()        : X(), Y()         {}
    Curve2D (int n)  : X(n), Y(n)       {}

    bool is_empty()                     {return X.dim()==0;}
    void writeFile (const char* fname);
    void list();
};

template <class Tx, class Ty>
void Curve2D<Tx,Ty>::writeFile (const char* fname)
{
    std::ofstream f(fname);              // ueberschreibt ohne Nachfrage!
    if (!f) 
    { 
      std::cerr << "Konnte »" << fname << "« nicht oeffnen\n";
      return;
    }

    for (int i=0; i < X.dim(); i++)
      f << X[i] << " " << Y[i] << "\n";
}

template <class Tx, class Ty>
void Curve2D<Tx,Ty>::list ()
{
    for (int i=0; i < X.dim(); i++)
    {
       std::cout << " [" << i << "]\tx=" << X[i] << " \tf(x)=" << Y[i] << "\n";
    }
}



/**===========================================================================

  @class ResponseSolver
  
  Template. Calculation of a response function.

  @param Unsign: Type of the z-values (typically unsigned integers)
  @param Real: Type wherein the QR-decomposition resp. SVD is solved.

  <b>Zu den Template-Parametern:</b>
  
  Wir bieten hier *keinen* Ctor mit Pack-Argument. Vermeidet unnoetige
   Abhaengigkeiten. Die (eventl.) dort entnehmbare Info \a z_max ist im 
   Ctor direkt zu uebergeben.

  Der Templateparameter \a Real dient nur internem Gebrauch (Genauigkeit
   der `Ax=b' Rechnung). Nach aussen werden nur double-Werte [oder
   \a RealCom?] praesentiert. Dadurch keine Notwendigkeit einer \a Real
   Fortfuehrung beim Nutzer. Falls <i> Real != RealCom </i>, passieren hier
   intern lediglich unkritische Konvertierungen.
   
  Der Templateparameter \a Real koennte spaeter durch statische Typdefinition
   ersetzt werden, sofern "normalerweise" nie zwei ResponseSolver Instanzen
   verschiedener \a Real Typen im Gebrauch sein werden. Allenfalls jetzt zum
   Test.

  Alle Float-Schnittstellen nach aussen derzeit auf Typ \c double gesetzt.
   Bei den Expose-Werten (`ExposeVals()' etc) kommt vielleicht mal ein 
   eigener Typ.
   
  Schliesslich natuerlich noch die Abhaengigkeit von Datentyp \a Tweight der
   Wichtungsfunktionen, definiert in WeightFuncBase.hpp.

  <b>Method:</b>
    
  The output of a photo sensor ("pixel") is called pixel-value or z-value and
   denoted by \a z. The number of possible z-values is \a n_zvals; i.e for a 
   8-bit resolution \a n_zvals=256, for 10-bit \a n_zvals=1024 etc.

  The ansatz is
     <pre>
        z = f(X) = f(E*t)  or  f^-1(z) = X = E*t  or 
        g(z) = log(X) = log(E)+log(t),  where g(z) := log[f^-1(z)]. </pre> 

   We compute the \a n_zvals values <i> g_i := g(z_i) </i> of the function 
   \a g(z), i.e. with other words, the \a n_zvals logarithmic exposure values
   \a log(X) to each z-value. We store them in the array \a logXcrv.

  QUESTION: The TNT::Array Ctor-parameters as value or as refs? - As refs!
   Value parameters would live only temporary in the Ctor, and so we have to
   take copies in every case, since we need them behind the Ctor.

  @warning Die Differenz zweier Integer muss bei signed int's nicht mehr in den
   Datentyp passen: 128 - (-127) = 255 > 128. Hier muss eigentlich immer dann
   die unsigned Variante stehen. Aber auch bei Unsigneds passt die ANZAHL
   moeglicher Werte nicht notwendig in diesen Typ, denn von 0..255 sind es 255+1
   = 256 Werte > 255. size_t (==uint) waere der vorlaeufige Behelf, der freilich 
   fuer sizeof(size_t)==sizeof(Unsign) auch versagt (32-Bit Unsign?). Aber da
   ist dieses Verfahren hier sowieso nicht mehr anwendbar.
  
*============================================================================*/
template <class Unsign, class Real>
class ResponseSolver : public ResponseSolverBase
{
    
    Unsign  z_max_;             // max. z-Wert; == 255 bei 8 Bit
    size_t  n_zvals;            // Zahl moegl. z-Werte: z_max + 1

    Array2D<Unsign>  Z_;        // Z_[i,j]: Pixelwert am Ort i in Aufname j
    Array1D<double>  B_;        // B_[j]: *Logarithm* Belichtungszeit in Aufn j

    Array1D<double>  Xcrv_;     // Xcrv[z]: exposure X to z-value z. Inverse
                                //  of the characteristic function z=f(X) and
                                //  the main effort of the procedure.
    Array1D<double>  logXcrv_;  // ==log(Xcrv); primary result of the calculation

public:
    /*  Ctor:  Inits only, doesn't execute the computation */
    ResponseSolver (const Array2D<Unsign> & Z,
                    const Array1D<double> & B, 
                    Unsign                z_max );

    
    /*  Computes the characteristic g(z_i) for given smoothing and weight function */
    void compute (double smoothing, const Array1D<Tweight> & weight, bool debug=false);

    
    /** 
    *   Returns the computed characteristic g(z_i), i.e. the log. exposure
    *    values log[X(z_i)]. If not been compute()d before, a Null-Array is
    *    returned.
    */
    const Array1D<double> & getLogExposeVals() const    {return logXcrv_;}
    
    /**
    *   Returns the unlogarithm. computed characteristic g(z_i), i.e. the
    *    exposure values X(z_i). If not been compute()d before, a Null-Array
    *    is returned.
    */
    const Array1D<double> & getExposeVals()    const    {return Xcrv_;}

    
    /*  Writes the computed exposure values (the solution) into a text file */
    void write_ExposeVals (const char* fname, bool log=false) const;

    
    /*  Exchanges the "Z-matrix" given in the Ctor */
    void exchange_Z_Matrix (const Array2D<Unsign> & Z);

    
    /*  Exchanges the vector of log. exposure times given in the Ctor. */
    void exchange_LogExposeTimes (const Array1D<double> & B);

private:

    /*  Solves the rectangular `Ax=b' problem */
    Array1D <Real> solve_ (const Array2D<Real>&, const Array1D<Real>&, bool);
      
    /*  Checks a weight function */
    bool check_weight_(const Array1D<Tweight> & weight);
    
    /*  Outputs as debug info `A' and `b' of Ax=b to stdout or into a file */
    void debug_out_ (const Array2D<Real> &A, const Array1D<Real> &b);
    
    /*  Our own weight function (old, only aufgehoben) */
    Real wicht (Unsign z);    // Gewicht des Pixelwertes z

};


/**+*************************************************************************\n
  Ctor:  Inits only, doesn't execute the computation.
  
  @param Z: "Z-Matrix"
  @param B: Array of the logarithm exposure times
  @param z_max: maximal possible z-value
******************************************************************************/
template <class Unsign, class Real>
ResponseSolver<Unsign,Real>::ResponseSolver (
        
        const Array2D<Unsign> & Z,
        const Array1D<double> & B,
        Unsign                z_max )
  :                               
    ResponseSolverBase (AUTO),          // solve_mode_ == AUTO
    z_max_        (z_max),
    Z_            (Z),
    B_            (B)
{
    IF_FAIL_EXCEPTION (Z_.dim2() == B_.dim(), Exception::RESPONSE);

    n_zvals = (size_t)z_max + 1;        // 256 for 8 bit; Problematisch fuer mehr Bits  
}


/**+*************************************************************************\n
  Internal weight function. No more used, only preserved.
******************************************************************************/
template <class Unsign, class Real>     
Real 
ResponseSolver<Unsign,Real>::wicht (Unsign z)
{
    const static Unsign zmin = 0;
    static       Unsign zmid = (zmin + z_max_) / 2;

    return (z <= zmid) ? (z - zmin) : (z_max_ - z);
}


/**+*************************************************************************\n
  Exchanges the "Z-matrix" given in the Ctor by \a Z (e.g. the Z-matrix is 
   usually different for each color channel).
  
  @note \a Z.dim2() (number of exposures) has to be identic with \a dim2() of 
   the current Z-matrix. Otherwise an exception is thrown. Whereas \a Z.dim1()
   (number of selected "spot points" ["Orte"]) can differ.
******************************************************************************/
template <class Unsign, class Real>
void 
ResponseSolver<Unsign,Real>::exchange_Z_Matrix (const Array2D<Unsign>& Z)
{ 
    IF_FAIL_EXCEPTION (Z_.dim2() == B_.dim(), Exception::RESPONSE);

    Z_ = Z; 
}

/**+*************************************************************************\n
  Exchanges the vector of log. exposure times given in the Ctor by \a B.
  @note \a B.dim() (number of times) has to match with \a Z.dim2() (number
   of exposures)! Otherwise an exception is thrown.
******************************************************************************/
template <class Unsign, class Real>
void 
ResponseSolver<Unsign,Real>::exchange_LogExposeTimes (const Array1D<double>& B)
{
    IF_FAIL_EXCEPTION (Z_.dim2() == B_.dim(), Exception::RESPONSE);

    B_ = B; 
}


/**+*************************************************************************\n
  compute() 

  Computes the characteristic <i>g(z_i)</i> (the log exposure values <i>log(X)</i>)
   for a certain smoothing coefficient \a smoothing and a given weight function
   \a weight.
  
  @param smoothing: smoothing coefficient.
  @param weight: weight function: weight[z] = weight of value z.
  @param debug: if true, give debug output.

  The number of equations is
-     n_eq  =  N*P + 1 + (n_zvals-2)  =  N*P + n_zvals - 1,

  where
-     N*P ..... for N selected pixel and P photographs
-     +1 ...... for the fixing equation g(z_mid) = 0.
-     n_zvals-2 for the smothness equations (excluding the both borders).
  
  The number of unknowns to determine is 
-     n_var = n_zvals + N,

  where
-     n_zvals: the n_zvals values g_i := g(z_i) ( ==log(X) ) corresponding
               to the n_zvals possible z-values (0..255) of an photo sensor.
-     N......: the N unknown logarithm irradiance values log(E_i) at the N
               selected pixels (assumed to be constant over the P photos
               of the same scene under different exposure times).
               
   Die Skalierung etwas undurchsichtig macht(e) Debevecs Konvention, die
   Wichtungswerte nicht in [0..1], sondern in [0..z_max] zu legen. Alles, auch
   die LogZeiten B[] und die Glaettungsgleichungen wurden also nicht mit einem
   Faktor w in [0..1], sondern mit w in [0..255] multipliziert. Da aber beide
   Seiten damit multipliziert, kann das problemlos geaendert werden (intern hier
   gehen uns die Wichtungszahlen nichts an). -- Inzwischen Gewichte draussen
   auf [0..1]-Werte umgestellt.
   
  Since this computation method is applicable ordinarily only for U8 data,
   I have gone back in type of \a weight from WeightFunc<> to TNT::Array1D<>.
   This reduces dependencies and allows to organize the WeightFunc stuff also
   in non-template form. If the caller uses a WeightFunc object he has to init
   WeightFunc's table and to take over this table.
   
  @note The weight function \a weight can have zeros, common are weight
   functions with zeros at z=0 and z=z_max -- I will call it a weight function
   with <i>"[1,1] 0-borders"</i> ("[1,1]" == one zero from each end).
   If however the weight function has wider 0-bordes than [1,1], e.g. [2,2], 
   then only the inner g_i's <i>g_1...g_254</i> are determined (form a coupled
   system) because the columns for \a g_0 and \a g_255 in the coeff. matrix `A'
   become zero-columns then and i.e. the unknowns \a g_0 and \a g_255 are undefined
   (completely decoupled from the system). They would have to be determined by
   another way, e.g. by extrapolation. The same happens for three consecutive
   zero-weights <i>w_{i-1} = w_i = w_{i+1} = 0</i> anywhere in the middle: 
   \a g_i would be undefined! In such cases the below generated coeff. matrix `A'
   has a rank defect (the QR method would fail), and although the SVD method
   returns formally here a solution, the undefined unknowns are set wherein
   only arbitrarily! Therefore only weight functions maximal with [1,1]
   0-borders are suitable. The (private) function check_weight_() gives a
   warning on stdout, if an unsuitable weight function is used.
******************************************************************************/
template <class Unsign,class Real>
void
ResponseSolver<Unsign,Real>::compute (double                 smoothing,  
                                      const Array1D<Tweight> & weight, 
                                      bool                   debug)
{
    //debug = true;
    IF_FAIL_EXCEPTION (Z_.dim2() == B_.dim(), Exception::RESPONSE);
    
    check_weight_ (weight);
    
    int N = Z_.dim1();                  // Zahl ausgewaehlter Orte (Kamerapixel)
    int P = Z_.dim2();                  // Zahl der Aufnahmen
    int n_eq  = N * P + n_zvals - 1;    // Zahl der Gleichungen
    int n_var = n_zvals + N;            // Zahl der Unbekannten

    std::cout << "\tN=" << N << ", P=" << P << ", n_zvals=" << n_zvals << '\n';
    std::cout << "\tn_eq=" << n_eq << ", n_var=" << n_var << '\n';

    Array2D<Real> A (n_eq, n_var, 0.);  // Koeff.-Matrix von Ax=b
    Array1D<Real> b (n_eq, 0.);         // rechte Seite von Ax=b

    //  A und b belegen...
    
    //  Include the (N*P) data-fitting equations:
    int k=0;
    for (int i=0; i < N; i++)           // ueber alle Orte
        for (int j=0; j < P; j++)       // ueber alle Aufn., dh Zeiten
        {
            Real wij = weight[ Z_[i][j] ];  // Real <-- Tweight
            if (wij == 0.0)
                continue;               // Nullzeilen ueberspringen
            A [k][ Z_[i][j]  ] =  wij;
            A [k][n_zvals + i] = -wij;
            b [k]              =  wij * B_[j];
            k++;
        }

    //  Fix the curve by setting its middle value to 0:
    A [k][n_zvals/2] = 1.0;              // dh. g_{mid} = b[k] = 0
    k++;

#if 0
    //  Fix the curve by setting its middle value to the corresponding
    //   original response value (notable leftover from test phase)
    
    double X_mid = 0.5 * (ccd.get_min_in() + ccd.get_max_in());
    Unsign z_mid = ccd.kennlinie (X_mid,false);
    std::cout << "X_mid=" << X_mid
              << ",  z_mid=" << z_mid
              << ",  log(X_mid)=" << log(X_mid) << "\n";
    A [k][z_mid] = 1.0;
    b [k] = log(X_mid);            // dh. g_{mid} = log(X_mid)
    k++;
#endif

    //  Include the (n-2) smoothness equations:
    for (size_t i=0; i < n_zvals-2; i++)
    {  
        //Real val = smoothing * wicht(i+1);    // Real = double * Real
        Real val = smoothing * weight[i+1];     // Real = double * Tweight
        if (val == 0.0)
            continue;                           // skip zero-rows    
        A [k][i  ] = val;
        A [k][i+1] = -2.0 * val;
        A [k][i+2] = val;
        k++;
    }

    //  If null-rows were skipped reduce A and b to their effective size:
    if (k < n_eq)
    {   std::cout << n_eq - k << " Nullzeilen. Effektiv ein (" << k << "," << n_var << ")-Problem\n";
        A = A.subarray (0,k-1, 0,n_var-1);
        b = b.subarray (0,k-1);
    }
    if (debug) debug_out_(A,b);

    //  Solve Ax=b (least square fit):
    Array1D<Real> x_solution = solve_ (A,b, debug);

    //  Extract the characteristic g(z_i) (==log(X)) from the solution:
    logXcrv_ = Array1D<double> (n_zvals);  // allocates
    Xcrv_    = Array1D<double> (n_zvals);  // allocates

    for (size_t i=0; i < n_zvals; i++) 
    {
        logXcrv_[i] = x_solution [i];      // double <-- Real
        Xcrv_   [i] = exp (logXcrv_[i]);
    }
}


/**+*************************************************************************\n
  solve_()  -  Solves \a Ax=b for given \a A and \a b (private helper).

  @param A: coefficient matrix
  @param b: the "right side"
  @param debug: if true, debug output
  @return  solution vector `x'
******************************************************************************/
template <class Unsign, class Real>
Array1D <Real> 
ResponseSolver<Unsign,Real>::solve_ (const Array2D<Real> & A, 
                                     const Array1D<Real> & b, bool debug )
{
    //debug = true;
    Array1D <Real>  x_qr, x_svd;        // Null vectors
    bool            qr_failed = false;
    TNT::Stopwatch  uhr;
    double          zeit;

    if (solve_mode() == AUTO  ||  solve_mode() & USE_QR) 
    {
      // QR-Decomposition...
      uhr.start();

      JAMA::QR <Real> qr(A);

      zeit = uhr.stop();
      std::cout << "Time for QR decomposition = " << zeit << " sec\n";
      std::cout << "\tQR::isFullRank = " << qr.isFullRank() << '\n';
      
      if (solve_mode() == AUTO  ||  solve_mode() & USE_SVD)  // ie. AUTO || BOTH
      {
        if (! qr.isFullRank())
        {
          std::cout << "QR decomposition inapplicable, use SVD...\n";
          qr_failed = true;
          goto SOLVE_VIA_SVD;
        }
      } 
      else                                                   // mode == USE_QR
        IF_FAIL_EXCEPTION (qr.isFullRank(), Exception::RESPONSE);

      // Quadratmittelproblem:
      
      x_qr = qr.solve (b);

      std::cout << "Proof QR...\n";
      proof_least_square_fit (x_qr, A, b);
    }

SOLVE_VIA_SVD:
    
    if (solve_mode() & USE_SVD  ||  qr_failed)
    {
      // SVD...
      uhr.start();

      JAMA::SVD <Real> svd(A);

      zeit = uhr.stop();
      std::cout << "Time for SVD = " << zeit << " sec\n";
      std::cout << "\trank = " << svd.rank() << '\n';
      std::cout << "\tcond = " << svd.cond() << '\n';
      std::cout << "\tnorm = " << svd.norm2() << '\n';

      bool debug_1 = false;
      if (debug_1) 
      {
        TNT::Array2D <Real> U,V,S, Aback;
        TNT::Array1D <Real> s;

        svd.getU (U);
        svd.getV (V);
        svd.getS (S);
        svd.getSingularValues (s);
        //std::cout << "s = " << s;

        Aback = mat_dot_mat (U, mat_dot_matT(S,V));

        std::cout << "U * S * V^T = " << Aback;
        std::cout << "U * S * V^T - A = " << Aback - A;
        list (Aback, 1e-13);
      }

      // Quadratmittelproblem:
    
      x_svd = svd.solve (b);

      std::cout << "Proof SVD...\n";
      proof_least_square_fit (x_svd, A, b);
    }


    // Debug output: If both, print SVD solution and the difference to QR.
    //  Criterion for "have SVD" or "have QR" are non-null vectors.
    if (debug)
    {   
      if (! x_svd.is_null())
      {  
        std::cout << "SVD solution = " << x_svd;
        
        if (! x_qr.is_null())
          std::cout << "SVD - QR = " << x_svd - x_qr;   // needs ident. dims
      }  
      else 
        std::cout << "QR solution = " << x_qr;
    }

    
    // Return: If SVD or both: SVD solution, else QR solution.
    
    if (! x_svd.is_null())
      return x_svd;

    return x_qr;    // never null - thrown then (USE_QR) above
}


/**+*************************************************************************\n
  Checks the weight function \a weight for usefulness in respect of 0-borders,
   i.e. whether \a weight would made some unknowns undeterminable. 
  @return False, if weight function makes some of the unknowns indeterminable.
******************************************************************************/
template <class Unsign, class Real>
bool
ResponseSolver<Unsign,Real>::check_weight_ (const Array1D<Tweight> & weight)
{
    /*  Find the smallest and the greatest z with weight[z] != 0 */
    Unsign  z_weight_min = 0;
    while ((z_weight_min < z_max_) && (weight[z_weight_min] == 0.0)) 
      z_weight_min ++;
    
    Unsign  z_weight_max = z_max_;
    while ((z_weight_max > 0) && (weight[z_weight_max] == 0.0)) 
      z_weight_max --;
    
    cout << "\tWeight function: 0-borders: [" 
         << (uint)z_weight_min << ", " 
         << (uint)z_max_ - z_weight_max << "]\n";
    
    if (z_weight_min == 0 || z_weight_max == z_max_)
    {
       cout << "ResponseSolver::INFO: Weight function without ";
       if (z_weight_min==0 && z_weight_max==z_max_) cout << "0-borders!\n";
       else if (z_weight_min == 0)                  cout << "left 0-border!\n";
       else                                         cout << "right 0-border!\n";
    }
    
    if (z_weight_min > 1 || z_weight_max < z_max_-1) 
    {
      cout << "ResponseSolver::WARNING **: Weight function makes some exposure values X_i indeterminable:\n\tX_i for i = ";

      if (z_weight_min > 1)
        if (z_weight_min < 4)  // maximal 2 values
          for (uint z=0; z <= (uint)z_weight_min-2; z++)
            cout << z << ", ";
        else 
          cout << "0..." << (uint)z_weight_min-2 << ", ";

      if (z_weight_max < z_max_-1)
        if (z_weight_max > z_max_-4)  // maximal 2 values
          for (uint z=z_weight_max+2; z <= (uint)z_max_; z++)
            cout << z << ", ";
        else
          cout << (uint)z_weight_max+2 << "..." << (uint)z_max_;  
      
      cout << ".\n";  
      return false;  
    }  
    
    return true;
}


/**+*************************************************************************\n
  Outputs the coeff. matrix \a A and the right side \a b of \a Ax=b to a file
   or to stdout. Devel and debugging helper.
******************************************************************************/
template <class Unsign, class Real>
void
ResponseSolver<Unsign,Real>::debug_out_ (const Array2D<Real> &A, const Array1D<Real> &b)
{
    bool to_file = true;
    if (to_file) 
    {
      /* We open the files in append mode to avoid immediate overwriting, because
          mostly ResponseSolver is called for the three channels three times
          consecutively. Disadvantage of this: After many debug runs could accrue
          a big "A_" file. */
      char fname[64];
      sprintf(fname, "A_%dx%d.txt", A.dim1(), A.dim2()); 
      std::ofstream  ofs_A (fname, std::ios::app);
      sprintf(fname, "b_%d.txt", b.dim());
      std::ofstream  ofs_b (fname, std::ios::app);
      ofs_A << A;
      ofs_b << b;
    }
    else 
    {
      std::cout << "A = " << A;
      std::cout << "b = " << b;
    }
}


/**+*************************************************************************\n
  write_ExposeVals()
    
  Writes the computed exposure values (the solution) into a text file in gnuplot
   readable form.

  @param fname: file name (an existing file gets overwritten without warning!)
  @param log: if true, logarithm. exposure values are written, and the prefix
     "log_" is added automatically to \a fname.
******************************************************************************/
template <class Unsign, class Real>
void
ResponseSolver<Unsign,Real>::write_ExposeVals (const char* fname, 
                                               bool        log  ) const
{
    char* name;

    if (log) {
      name = (char*)malloc (strlen(fname)+5);    // +5 == "log_" + '\0'
      sprintf (name, "log_%s", fname);
    }
    else
      name = (char*)fname;

    printf ("Schreibe Datei »%s«\n", name);
    FilePtr f(name, "wt");                  // ueberschreibt ohne Warnung!

    if (log) {
      for (size_t i=0; i < n_zvals; i++)
        fprintf (f, "%d  %f\n", i, logXcrv_[i]);
      
      free (name);
    }
    else 
      for (size_t i=0; i < n_zvals; i++)
        fprintf (f, "%d  %f\n", i, Xcrv_[i]);
}


}  // namespace

#endif  // ResponseSolver_hpp

// END OF FILE

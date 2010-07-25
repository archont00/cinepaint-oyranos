/*
   tnt_math.h with adds by H.S.
*/
#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <cmath>       // sqrt()


namespace TNT
{

/**
    @returns the absolute value of a real (no-complex) scalar.
    
    HS: In <cstdlib> `abs()' for ints (no namespace) and in <cmath> in 
        namespace "std"(!) abs() for floats.
*/
template <class Real>
Real abs (const Real &a)
{
    return  (a >= 0 ? a : -a);
}


/**
    @returns hypotenuse of real (non-complex) scalars a and b by
    avoiding underflow/overflow
    using (a * sqrt( 1 + (b/a) * (b/a))), rather than
    sqrt(a*a + b*b).
*/
template <class Real>
Real hypot(const Real &a, const Real &b)
{
    if (a == 0)
        return abs(b);
    else
    {
        Real c = b/a;
        return abs(a) * sqrt(1 + c*c);
    }
}

#if 0
// HS: We have min() && max() in <algorithm>.
// Btw: in <algorith> defined as `const T& min(..)' instead of `T min(..)'.

/**
    @returns the minimum of scalars a and b.
*/
template <class Scalar>
Scalar min(const Scalar &a, const Scalar &b)
{
    return  a < b ? a : b;
}

/**
    @returns the maximum of scalars a and b.
*/
template <class Scalar>
Scalar max(const Scalar &a, const Scalar &b)
{
    return  a > b ? a : b;
}

#endif 



//=========
// HS adds:
//=========

/**
  Formal sind die numerisch intendierten Template-Funktionen aufrufbar
  (zu instanziieren) fuer alle Typen, fuer die die Operationen
    a+b, a-b, a*b, a/b,
    a+=b, a-=b, ...
    a++, a--
  definiert sind, soweit sie jedenfalls verwendet werden.
  Allerdings ist nicht jede Instanziierung sinnvoll.
  Ist eine Template-Funktion sinnvoll nur fuer reelle Zahlen, heissen die
  Template-Argumente "Real", wenn nur fuer Skalare -- oder sagen wir:
  Elemente einer Menge mit einer Ordnungsrelation, dann heissen sie "Skalar".
  Soweit kaum Einschraenkungen bestehen, heissen sie "T".

  Diese Namensgebungen sind natuerlich nur Hinweise an den Nutzer ohne
  jede Compiler-Verbindlichkeit.
*/

/**
*   swap()  -   Not only a math-template.
*               And does it belong realy under the namespace TNT?
*   Always 3 =-operations take place (copies, if `=' not overwritten).
*/
template <class T> inline
void swap (T& a, T& b)
{
    T tmp = a;
    a = b;
    b = tmp;
}

#define SWAP(a,b,tmp)    tmp=a; a=b; b=tmp;


/**
   dot() and scalarprod():
   ======================
    Mathematisch ist ein Skalarprodukt (Punktprodukt) auf einem Vektorraum
    (V,|K) -- also einem Vektorraum ueber dem Koerper |K -- definiert als die
    Abbildung V x V -> |K. So gesehen wuerde man ein Skalarprodukt zunaechst
    als ein Zweiertemplate ansetzen, wo T der Typ der vektoriellen Objekt und
    K der "skalare" Rueckgabetyp (reell, komplex, ...).

    Allerdings ist der math. Begriff eines Vektorraumes (V,IK) nicht
    deckungsgleich mit dem computertechnischen `Vektor'-Begriff.

    Die math.-logische Unterscheidung zwischen der Menge V der Vektorobjekte
    und der Menge |K der "skalaren" Koerperelemente ("Zahlen") beruecksichtigt
    eine Verschiedenheit im Dingcharakter der Objekte (ein Vektor darf
    noch mehr als eine "blosse Zahl" sein). In der Computerei aber ist von
    vornherein alles auf die Zahlrepraesentation reduziert, ein Vektor also auf
    seine Komponentendarstellung. Ist also |K der "skalare" Koerper eines
    Vektorraumes (V,|K), stellt sich ein Skalarprodukt computerintern von
    vornherein nur noch als die Abbildung |K x |K -> |K dar.

    Ganz abgesehen davon, dass in der Computerrei schlicht jedes x-beliebige
    n-Tupel "Vektor" genannt wird ohne jeden Rekurs auf die Vektorraumaxiome.

    Aber jedenfalls in der Projektion des mathematischen Begriffs auf seine
    Computerrealisierung stellt sich ein "inneres Produkt" in der Tat dar
    nur noch als Abbildung |K x |K -> |K und kann als Einertemplate
    beschrieben werden.
*/

/**
    Primitive (and fast) versions without any attention to overflow/underflow
    or cascade summation.

    Keep in mind: pointer arithmetic discards BOUNDS_CHECK.
    Checking, whether such routines give a notable speed up compared to
    normal index acess, must be done with undefined TNT_BOUND_CHECK.
    Otherwise we measure the range check overhead more than anything else.
*/
/**
    The following routines assume that `a' resp. `b' denote start adresses
    of vectors of T's stored *contiguously*.
*/
template <class T, class Scalar>                    // old
Scalar scalarprod (int N, const T* a, const T* b)
{
    const T* end = a + N;
    Scalar sum = 0;
    while (a < end)
        sum += *a++ * *b++;

    return sum;
}

template <class T>
T dot (int N, const T* a, const T* b)
{
    const T* end = a + N;
    T sum = 0;
    while (a < end)
        sum += *a++ * *b++;

    return sum;
}

/**
    Column-like dot products, where the elements are stored in a distance
    `stride'. More accurate were the declaration of the `stride'-parameters
    as ptr_diff than as int, but then probaly the multiply `N * stride' makes
    problems.
*/
template <class T>
T dot (int N, const T* a, int stride_a, const T* b, int stride_b)
{
    const T* end = a + N * stride_a;
    T sum = 0;
    while (a < end)
    {
        sum += *a * *b;
        a += stride_b;
        b += stride_b;
    }

    return sum;
}

template <class T> inline
T dot (int N, const T* a, int stride_a, const T* b)
{
    return dot (N, a, stride_a, b, 1);
}

template <class T> inline
T dot (int N, const T* a, const T* b, int stride_b)
{
    return dot (N, a, 1, b, stride_b);
}


}  // namespace TNT

#endif
/* MATH_UTILS_H */

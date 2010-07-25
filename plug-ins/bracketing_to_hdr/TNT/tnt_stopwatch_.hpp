/*
  Meine Ergaenzung der Klasse Stopwatch aus "stopwatch.h[pp]" um die
  Funktion `result()', die stoppt und das Ergebnis ausgibt. Spart beim
  Anwender zwei Zeilen: "double zeit;" und "std::cout...".
  In Extra-Datei, da hier <ostream> bzw. <cstdio> notwendig wird.
  Vielleicht ueberfluessige Sorge, aber wer weiss.
*/
#ifndef TNT_STOPWATCH__HPP
#define TNT_STOPWATCH__HPP

#include <iostream>
#include "tnt_stopwatch.hpp"

namespace TNT {


class StopWatch : public TNT::Stopwatch {

  public:
    
    double result()
    {
        double zeit = stop();
        std::cout << "Zeit = " << zeit << " sec\n";
        return zeit;
    }
};


}  // namespace
#endif  // TNT_STOPWATCH__HPP

// END OF FILE

#include "cinepaint_half.h"
#include "half.h"

ImfHalf	ImfFloat2Half (float f)
{
  return half(f).bits();
}

float	ImfHalf2Float (ImfHalf h)
{
  return float (*((half *)&h));
}



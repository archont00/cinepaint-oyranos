#ifndef CINEPAINT_HALF_H
#define CINEPAINT_HALF_H


typedef unsigned short ImfHalf;

#ifdef __cplusplus
extern "C" {
#endif

ImfHalf	ImfFloat2Half (float f);
float	ImfHalf2Float (ImfHalf h);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CINEPAINT_HALF_H */

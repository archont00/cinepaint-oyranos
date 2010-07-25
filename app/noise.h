#ifndef __NOISE_H__
#define __NOISE_H__

float bias(float a, float b);
float gain(float a, float b);
float turbulence(float *v, float freq);
float noise (float x, float y);
double noise_smoothstep(double a, double b, double x);

#endif  /*  __NOISE_H__  */

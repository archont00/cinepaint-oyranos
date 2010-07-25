/*
 * testtools.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
   testtools.hpp
   
   @todo Aufraeumen, trennen nach Abhaengigkeiten! 
*/
#ifndef testtools_hpp
#define testtools_hpp

#include <cstdlib>              // rand(), RAND_MAX
#include "Br2HdrManager.hpp"

namespace br {

/*!
  Structure holds the parameters for the three simulated linear response functions.
*/
struct ParamResponseLinear {
    double Xmin_R, Xmax_R;
    double Xmin_G, Xmax_G;
    double Xmin_B, Xmax_B;
};
/*!
  Structure holds the parameters for the three simulated "half cosinus" response functions.
*/
struct ParamResponseHalfCos {
    double Xmin_R, Xmax_R;
    double Xmin_G, Xmax_G;
    double Xmin_B, Xmax_B;
};

/*!
  Enum constants for the available shapes of simulated response functions.
*/
enum ResponseShape {
    RESP_LINEAR,
    RESP_HALFCOS
};

/*!
  Enum constants for the available shapes of radiance scenes.
*/
enum RadianceSceneShape {
    SCENE_SLOPE_XY,
    SCENE_RANDOM
};



/**+*************************************************************************\n
  @class RadianceScene      
  
  Abstract base class of simulated radiance scenes.
  A "radiance scene" simulates a radiance distribution of a rectangular area.
  It is assumed:<pre>  radiance * exposure time == exposure X, </pre> 
  and that \a X determines the output \a z of a camera pixel via the response
  function \a z=f(X). Goal of the bracketing_to_hdr game is to reconstruct
  from given z-values and given exposure times the underlying radiance values. 
  So, in the best case, the resulting HDR image should be identic with the input
  radiance scene, in particular independently of the chosen response function.
  (Apart from a constant factor, which is free in the algorithm due to a missing
  absolute calibration of the HDR values, and which factor is set so, that each
  response function satifies the condition f(X=1) = z_mid, i.e. all response curves
  go in the z-X-diagram at the middle z-value \a z_mid through the point \a X=1.)
   
  - radiance_rgb() returns a radiance RGB at the pixel coords (ix,iy).
  - radiance_mono() returns a single radiance value at the pixel coords (ix,iy).

******************************************************************************/
class RadianceScene 
{
public:
    virtual ~RadianceScene() {}  // virt. DTOR needed for pedantic in SuSE 10.1?
    virtual double      radiance_mono (int ix, int iy) const = 0;
    virtual Rgb<double> radiance_rgb (int ix, int iy) const = 0;

protected:
    //  Ctor protected to avoid attempted instances of abstract class
    RadianceScene()  {}
};

/**+*************************************************************************\n
  @class RadianceSceneSlopeXY       
  
  Simulates a radiance scene with the shape of a "xy-slope". 
  
  @warning Keine Vorkehrungen in radiance_rgb() and radiance_mono(), falls 
   (ix,iy) out of range.
******************************************************************************/
class RadianceSceneSlopeXY : public RadianceScene
{
    int    xdim_, ydim_; 
    double Vmin_, Vmax_;
    double f_;
public:
    /*! \a xdim and \a ydim are the dimensions of the pixel area to simulate.
        \a Vmin is the minimal radiance value, \a Vmax the maximal radiance value.
    */
    RadianceSceneSlopeXY (int xdim, int ydim, double Vmin=0.0, double Vmax=1.0) 
      : xdim_(xdim), ydim_(ydim),
        Vmin_(Vmin), Vmax_(Vmax)
      {
        if (xdim<2 && ydim<2)
          f_ = (Vmax - Vmin);  // sinnlos, da nur Punkt(0,0) moegl. mit rad.=Vmin
        else if (xdim<2)
          f_ = (Vmax - Vmin) / (ydim - 1);
        else if (ydim<2)
          f_ = (Vmax - Vmin) / (xdim - 1);
        else
          f_ = (Vmax - Vmin) / ((xdim - 1) * (ydim - 1));
      }
    
    double      radiance_mono (int ix, int iy) const {return Vmin_ + f_ * ix * iy;}
    Rgb<double> radiance_rgb (int ix, int iy) const  {return Rgb<double>(Vmin_ + f_ * ix * iy);}
};

/**+*************************************************************************\n
  @class RadianceSceneRandom       
  
  Simulates a radiance scene with random values using stdlib's rand() function.
   In the Ctor we set a seed by `srand(1)' as a weak attempt to get reproducible
   values. To secure reproducibility you should set a seed before using the
   radiance() functions. The three channels at a pixel get different random
   values (random colors).
  
  @warning Keine Vorkehrungen in radiance_rgb() and radiance_mono(), falls 
   (ix,iy) out of range.
   
  @warning rand() is not thread-safe, so the radiance() functions as well.
******************************************************************************/
class RadianceSceneRandom : public RadianceScene
{
    int    xdim_, ydim_; 
    double Vmin_, Vmax_;
    double f_;
public:
    /*! \a xdim and \a ydim are the dimensions of the pixel area to simulate.
        \a Vmin is the minimal radiance value, \a Vmax the maximal radiance value.
    */
    RadianceSceneRandom (int xdim, int ydim, double Vmin=0.0, double Vmax=1.0) 
      : xdim_(xdim), ydim_(ydim),
        Vmin_(Vmin), Vmax_(Vmax)
      { f_ = (Vmax - Vmin) / RAND_MAX;
        srand(1);
      }
    
    double      radiance_mono (int ix, int iy) const {
        return Vmin_ + f_ * rand();
      }
    Rgb<double> radiance_rgb (int ix, int iy) const  {
        return Rgb<double>(Vmin_ + f_ * rand(),
                           Vmin_ + f_ * rand(),
                           Vmin_ + f_ * rand());
      }
};



/**+*************************************************************************\n
  @class SimulResponse     
  Abstract base class of the simulated response functions.
******************************************************************************/
class SimulResponse 
{
public:
    virtual ~SimulResponse() {}  // virt. DTOR needed for pedantic in SuSE 10.1?
    virtual unsigned response (double X) const = 0;

protected:
    //  Ctor protected to avoid attempted instances of abstract class
    SimulResponse()  {}
};

/**+*************************************************************************\n
  @class SimulResponseLinear     
  A simulated linear response function.
******************************************************************************/
class SimulResponseLinear : public SimulResponse
{
    double   Xmin_, Xmax_;      // exposure values to z=0 and z=zmax.
    unsigned zmax_;             // 255 for 8-bit, 1024 for 10-bit etc.
    double   f_;                // slope of the curve
public:
    SimulResponseLinear (double Xmin, double Xmax, unsigned zmax) 
      : Xmin_(Xmin), Xmax_(Xmax), zmax_(zmax) 
      {
        f_ = zmax / (Xmax - Xmin);
      }
    
    unsigned response (double X)  const 
      { if (X <= Xmin_) return 0;
        if (X >= Xmax_) return zmax_;
        return unsigned (f_ * (X - Xmin_) + 0.5); 
      }
};

/**+*************************************************************************\n
  @class SimulResponseHalfCos     
  A simulated "half cosinus" response function (the shifted second half wave).
******************************************************************************/
class SimulResponseHalfCos : public SimulResponse
{
    double   Xmin_, Xmax_;      // exposure values to z=0 and z=zmax.
    unsigned zmax_;             // 255 for 8-bit, 1024 for 10-bit etc.
    double   fa_, fb_;          // two internal factors
public:
    SimulResponseHalfCos (double Xmin, double Xmax, unsigned zmax) 
      : 
        Xmin_(Xmin), Xmax_(Xmax), zmax_(zmax) 
      {
        fa_ = M_PI / (Xmax - Xmin);
        fb_ = 0.5 * zmax_;
      }
    
    unsigned response (double X)  const
      { if (X <= Xmin_) return 0;
        if (X >= Xmax_) return zmax_;
        return unsigned (0.5 + fb_ * (cos(fa_*(X - Xmin_) + M_PI) + 1)); 
        //                            ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ in [0..2]
      }
};



/**+*************************************************************************\n
  @class SimulPixel   
  A simulated camera RGB pixel with three response functions.
  
  @warning  Uebereinstimmender Wertebereich \a 0..zmax der Antwortfunktionen
   nicht erzwungen, haengt ganz an uebergebenen Antwortfunktionen.
******************************************************************************/
class SimulPixel
{
    SimulResponse  *R_, *G_, *B_;

public:
    SimulPixel (SimulResponse& R, SimulResponse& G, SimulResponse& B)
      : R_(&R), G_(&G), B_(&B)  {}
    
    SimulPixel (SimulResponse* R, SimulResponse* G, SimulResponse* B)
      : R_(R), G_(G), B_(B)  {}

    void set_response_R (SimulResponse* R)      {R_ = R;}   
    void set_response_G (SimulResponse* G)      {G_ = G;}   
    void set_response_B (SimulResponse* B)      {B_ = B;}   
      
    /** @return pixel values (RGB) for a given exposure RGB */
    Rgb<unsigned> response (Rgb<double> exposure)  const
      {return Rgb<unsigned> (R_->response (exposure.r), 
                             G_->response (exposure.g), 
                             B_->response (exposure.b));}
};


void add_simul_images_RGB_U8 ( Br2HdrManager & manager, 
                               int xdim, int ydim, int N,
                               const RadianceScene & scene,
                               const SimulResponse & response_R,
                               const SimulResponse & response_G,
                               const SimulResponse & response_B,
                               const char* basename,  // basename of the images
                               bool before = false );

void add_simul_images_RGB_U16 (Br2HdrManager & manager, 
                               int xdim, int ydim, int N,
                               const RadianceScene & scene,
                               const SimulResponse & response_R,
                               const SimulResponse & response_G,
                               const SimulResponse & response_B,
                               const char* basename,  // basename of the images
                               bool before = false );


void add_TestImages            (Br2HdrManager &, int xdim, int ydim, int N, ImageType);
void add_TestImages_RGB_U8     (Br2HdrManager &, int xdim, int ydim, int N);
void add_TestImages_RGB_U16    (Br2HdrManager &, int xdim, int ydim, int N);

void add_BadImages             (Br2HdrManager &, int xdim, int ydim, int N, ImageType);
void add_BadImages_RGB_U8      (Br2HdrManager &, int xdim, int ydim, int N);
void add_BadImages_RGB_U16     (Br2HdrManager &, int xdim, int ydim, int N);


}  // namespace

#endif  // testtools_hpp

// END OF FILE

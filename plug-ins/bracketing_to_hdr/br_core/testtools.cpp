/*
 * testtools.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  testtools.cpp
*/
#include <cmath>                // M_PI
#include "testtools.hpp"
#include "Br2HdrManager.hpp"
#include "br_Image.hpp"         // Image
#include "br_Image_utils.hpp"   // fillBuffer_...()
#include "DynArray2D.hpp"       // DynArray2D<>
#include "br_macros.hpp"        // NOT_IMPL_IMAGE_TYPE()
#include "TheProgressInfo.hpp"  // TheProgressInfo
#include "i18n.hpp"             // macros N_(), _()


namespace br {

//  progress info text
static const char* str_computing_simul_ = N_("Computing simulated images...");


/**+*************************************************************************\n
  Adds to \a manager's image container a set of test images with values 0,1,2...
   Wrapped by beginAddImages() - endAddImages().
******************************************************************************/
void add_TestImages (Br2HdrManager & manager, int xdim, int ydim, int N, ImageType type)
{
    manager.beginAddImages();    
    switch (type)
    {
      case IMAGE_RGB_U8:  add_TestImages_RGB_U8  (manager, xdim,ydim, N); break;
      case IMAGE_RGB_U16: add_TestImages_RGB_U16 (manager, xdim,ydim, N); break;
      default:            NOT_IMPL_IMAGE_TYPE(type); return;
    }
    manager.timesSetting (Br2HdrManager::BY_NONE);  // use our times!
    manager.endAddImages();
}

void add_TestImages_RGB_U8 (Br2HdrManager & manager, int xdim, int ydim, int N)
{
    char s[80]; 

    TheProgressInfo::start (1.0, _(str_computing_simul_));
    for (int i=0; i < N; i++)
    {
      sprintf (s, "img#%d - RGB U8", i);
      
      BrImage img (xdim, ydim, IMAGE_RGB_U8, s, pow(2., i));
      img.exposure.shutter = 1.0f / img.time();  // spasseshalber
      fillBuffer_RGB_U8 (img, i);
      
      manager.add_Image (img);
      TheProgressInfo::forward ( 1.0 / N );
    }
    TheProgressInfo::finish();
}

void add_TestImages_RGB_U16 (Br2HdrManager & manager, int xdim, int ydim, int N)
{
    char s[80]; 
    
    TheProgressInfo::start (1.0, _(str_computing_simul_));
    for (int i=0; i < N; i++)
    {
      sprintf (s, "img#%d - RGB U16", i);
      
      BrImage img (xdim, ydim, IMAGE_RGB_U16, s, pow(2., i));
      img.exposure.shutter = 1.0f / img.time();  // spasseshalber 
      fillBuffer_RGB_U16 (img, i);
      
      manager.add_Image (img);
      TheProgressInfo::forward ( 1.0 / N );
    }
    TheProgressInfo::finish();
}


/**+*************************************************************************\n
  Adds to \a manager's image container a set of images, which should produce
   errors while response curve computation. Wrapped by beginAddImages() and
   endAddImages().
******************************************************************************/
void add_BadImages (Br2HdrManager & manager, int xdim, int ydim, int N, ImageType type)
{
    manager.beginAddImages();    
    switch (type)
    {
      case IMAGE_RGB_U8:  add_BadImages_RGB_U8  (manager, xdim,ydim, N); break;
      case IMAGE_RGB_U16: add_BadImages_RGB_U16 (manager, xdim,ydim, N); break;
      default:            NOT_IMPL_IMAGE_TYPE(type); return;
    }
    manager.timesSetting (Br2HdrManager::BY_NONE);  // use our times!
    manager.endAddImages (false);
}

void add_BadImages_RGB_U8 (Br2HdrManager & manager, int xdim, int ydim, int N)
{
    char s[80]; 
    
    TheProgressInfo::start (1.0, _(str_computing_simul_));
    for (int n=0; n < N; n++)
    {
      sprintf (s, "bad img#%d - RGB U8", n);
      
      BrImage img (xdim, ydim, IMAGE_RGB_U8, s, pow(2., n));
      ImgScheme2DView< Rgb<uint8> >  A(img);
      
      for (int i=0; i < A.dim1(); i++)
        for (int j=0; j < A.dim2(); j++) {
          Rgb<uint8> rgb(0,0,0);
          rgb [n % 3] = 50;  // jedes 3. Bild R-Kanal etc.
          A[i][j] = rgb;
        }
      manager.add_Image (img);
      TheProgressInfo::forward ( 1.0 / N );
    }
    TheProgressInfo::finish();
}

void add_BadImages_RGB_U16 (Br2HdrManager & manager, int xdim, int ydim, int N)
{
    char s[80]; 
    
    TheProgressInfo::start (1.0, _(str_computing_simul_));
    for (int n=0; n < N; n++)
    {
      sprintf (s, "bad img#%d - RGB U16", n);
      
      BrImage img (xdim, ydim, IMAGE_RGB_U16, s, pow(2., n));
      ImgScheme2DView< Rgb<uint16> >  A(img);
      
      for (int i=0; i < A.dim1(); i++)
        for (int j=0; j < A.dim2(); j++) {
          Rgb<uint16> rgb(0,0,0);
          rgb [n % 3] = 50*256;  // jedes 3. Bild R-Kanal etc.
          A[i][j] = rgb;
        }
      manager.add_Image (img);
      TheProgressInfo::forward ( 1.0 / N );
    }
    TheProgressInfo::finish();
}



/**+*************************************************************************\n
  Adds to \a manager's image container a set of simulated RGB-U8 images of the
   given radiance \a scene and using the given response functions. Exposure
   times generated currently for stop=1. \a basename is the basename of the
   images.
   
  @param before: Because the radiance values are identic for all images, we could
   compute them before - outside the image loop - in an array. For \a before=TRUE
   we do it; it needs extra memory of <i>3 * sizeof(double) * xdim * ydim</i>
   bytes (for 3000 x 3000 = 216 MB). If the radiance computation is slow, this
   can accelerate, if the system has to swap, this will decelerate. The essential
   reason however was, that in the case of a randomizing radiance \a scene we get
   only then identic radiance values for all images! (Or we would call `srand()'
   inside the loop before each image, but we work here with an abstract \a scene
   parameter and have no information whether \a scene is randomizing.) A side
   effect: if we use a randomizing scene and set \a before=FALSE, we get
   completely randomized image series.

  @internal Sollten die Einzelbilder einmal in verschiedenen Threads berechnet 
   werden, waere relevant, ob Strahlungsszene thread-sicher, z.B. rand() ist es
   nicht! - Auch da waere Vorabberechnung dienlich.
******************************************************************************/
void add_simul_images_RGB_U8 (Br2HdrManager & manager, 
                              int xdim, int ydim, int N,
                              const RadianceScene & scene,
                              const SimulResponse & response_R,
                              const SimulResponse & response_G,
                              const SimulResponse & response_B,
                              const char* basename,   // basename of the images
                              bool before )
{
    TheProgressInfo::start (1.0, _(str_computing_simul_));
    if (before) {
      /*  compute radiance values before... */
      DynArray2D< Rgb<double> >  radiance_array (ydim, xdim);
      
      for (int i=0; i < radiance_array.dim1(); i++)
        for (int j=0; j < radiance_array.dim2(); j++)
          radiance_array[i][j] = scene.radiance_rgb(j,i);  // j==xIndex, i==yIndex!
      
      TheProgressInfo::forward ( 1.0 / (N+1) );
        
      for (int n=0; n < N; n++)
      {
        char s[80];  
        snprintf (s, 80, "%s#%d - RGB U8", basename, n);
        double time = pow (2., n);        // 1,2,4,...
        
        BrImage  img (xdim, ydim, IMAGE_RGB_U8, s, time);
        ImgScheme2DView< Rgb<uint8> >  A(img);
      
        for (int i=0; i < A.dim1(); i++)
          for (int j=0; j < A.dim2(); j++) {
            Rgb<double> exposure = radiance_array[i][j] * time;
            Rgb<uint8> & rgb = A[i][j];
            rgb[0] = response_R.response (exposure.r);
            rgb[1] = response_G.response (exposure.g);
            rgb[2] = response_B.response (exposure.b);
          }
        manager.add_Image (img);
        TheProgressInfo::forward ( 1.0 / (N+1) );
      }
    } else {  
      /*  compute radiance values on place... */
      for (int n=0; n < N; n++)
      {
        char s[80];  
        snprintf (s, 80, "%s#%d - RGB U8", basename, n);
        double time = pow (2., n);        // 1,2,4,...

        BrImage  img (xdim, ydim, IMAGE_RGB_U8, s, time);
        ImgScheme2DView< Rgb<uint8> >  A(img);
      
        for (int i=0; i < A.dim1(); i++)
          for (int j=0; j < A.dim2(); j++) {
            Rgb<double> exposure = scene.radiance_rgb(j,i) * time;  // j==xIndex, i==yIndex!
            Rgb<uint8> & rgb = A[i][j];
            rgb[0] = response_R.response (exposure.r);
            rgb[1] = response_G.response (exposure.g);
            rgb[2] = response_B.response (exposure.b);
          }
        manager.add_Image (img);
        TheProgressInfo::forward ( 1.0 / N );
      }
    }  
    TheProgressInfo::finish();
}

/**+*************************************************************************\n
  Adds to \a manager's image container a set of simulated RGB-U16 images of the
   given radiance \a scene and using the given response functions. Exposure
   times generated currently for stop=1. \a basename is the basename of the
   images.
  
  @param before: Because the radiance values are identic for all images, we could
   compute them before - outside the image loop - in an array. For \a before=TRUE
   we do it; it needs extra memory of <i>3 * sizeof(double) * xdim * ydim</i>
   bytes (for 3000 x 3000 = 216 MB). If the radiance computation is slow, this
   can accelerate, if the system has to swap, this will decelerate. The essential
   reason however was, that in the case of a randomizing radiance \a scene we get
   only then identic radiance values for all images! (Or we would call `srand()'
   inside the loop before each image, but we work here with an abstract \a scene
   parameter and have no information whether \a scene is randomizing.) A side
   effect: if we use a randomizing scene and set \a before=FALSE, we get
   completely randomized image series.

  @internal Sollten die Einzelbilder einmal in verschiedenen Threads berechnet
   werden, waere relevant, ob Strahlungsszene thread-sicher, z.B. rand() ist es
   nicht! - Auch da waere Vorabberechnung dienlich.
******************************************************************************/
void add_simul_images_RGB_U16 (Br2HdrManager & manager, 
                               int xdim, int ydim, int N,
                               const RadianceScene & scene,
                               const SimulResponse & response_R,
                               const SimulResponse & response_G,
                               const SimulResponse & response_B,
                               const char* basename,  // basename of the images
                               bool before )
{
    TheProgressInfo::start (1.0, _(str_computing_simul_));
    if (before) {
      /*  compute radiance values before... */
      DynArray2D< Rgb<double> >  radiance_array (ydim, xdim);
      
      for (int i=0; i < radiance_array.dim1(); i++)
        for (int j=0; j < radiance_array.dim2(); j++)
          radiance_array[i][j] = scene.radiance_rgb(j,i);  // j==xIndex, i==yIndex!
      
      TheProgressInfo::forward ( 1.0 / (N+1) );
        
      for (int n=0; n < N; n++)
      {
        char s[80];  
        snprintf (s, 80, "%s#%d - RGB U16", basename, n);
        double time = pow (2., n);        // 1,2,4,...

        BrImage  img (xdim, ydim, IMAGE_RGB_U16, s, time);
        ImgScheme2DView< Rgb<uint16> >  A(img);
      
        for (int i=0; i < A.dim1(); i++)
          for (int j=0; j < A.dim2(); j++) {
            Rgb<double> exposure = radiance_array[j][i] * time;
            Rgb<uint16> & rgb = A[i][j];
            rgb[0] = response_R.response (exposure.r);
            rgb[1] = response_G.response (exposure.g);
            rgb[2] = response_B.response (exposure.b);
          }
        manager.add_Image (img);
        TheProgressInfo::forward ( 1.0 / (N+1) );
      }
    } else {
      /*  compute radiance values on place... */
      for (int n=0; n < N; n++)
      {
        char s[80];  
        snprintf (s, 80, "%s#%d - RGB U16", basename, n);
        double time = pow (2., n);        // 1,2,4,...

        BrImage  img (xdim, ydim, IMAGE_RGB_U16, s, time);
        ImgScheme2DView< Rgb<uint16> >  A(img);
      
        for (int i=0; i < A.dim1(); i++)
          for (int j=0; j < A.dim2(); j++) {
            Rgb<double> exposure = scene.radiance_rgb(j,i) * time;  // j==xIndex, i==yIndex!
            Rgb<uint16> & rgb = A[i][j];
            rgb[0] = response_R.response (exposure.r);
            rgb[1] = response_G.response (exposure.g);
            rgb[2] = response_B.response (exposure.b);
          }
        manager.add_Image (img);
        TheProgressInfo::forward ( 1.0 / N );
      }
    }
    TheProgressInfo::finish();
}


}  // namespace

// END OF FILE

/*
BSD style license, see: <http://www.opensource.org/licenses/bsd-license.php>

Copyright (c) 2005, Kai-Uwe Behrmann
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of the <ORGANIZATION> nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/**
   ICC <-> OpenEXR colour space notation converter

   chroma_icc.cxx takes eighter Imf::Chromaticities or an lcms generated
   v4 ICC profile including the chrm tag
   and converts each in the others direction.
 */

#include <ImfStandardAttributes.h>
#include <lcms.h>

char*
writeICCprims (Imf::Chromaticities & image_prim, size_t & size)
{
  char* buf = NULL;
  // colour space description -> ICC profile
  {
     LPGAMMATABLE gamma[3];
     gamma[0] = gamma[1] = gamma[2] = cmsBuildGamma(1, 1.0);
     cmsCIExyY wp;
     wp.x = image_prim.white[0];
     wp.y = image_prim.white[1];
     wp.Y = 1.0;

     cmsCIExyYTRIPLE primaer;
     primaer.Red.x = image_prim.red[0];
     primaer.Red.y = image_prim.red[1];
     primaer.Green.x = image_prim.green[0];
     primaer.Green.y = image_prim.green[1];
     primaer.Blue.x = image_prim.blue[0];
     primaer.Blue.y = image_prim.blue[1];
     primaer.Red.Y = primaer.Green.Y = primaer.Blue.Y = 1.0;

     cmsHPROFILE p = cmsCreateRGBProfile( &wp, &primaer, gamma);

     _cmsSaveProfileToMem (p, NULL, &size);
     buf = (char*) calloc (sizeof(char), size);
     _cmsSaveProfileToMem (p, buf, &size);

     cmsCloseProfile( p );
  }
  return buf;
}

icUInt32Number
icValue (icUInt32Number val)
{
#if BYTE_ORDER == LITTLE_ENDIAN
  unsigned char        *temp = (unsigned char*) &val;

  static unsigned char  uint32[4];

  uint32[0] = temp[3];
  uint32[1] = temp[2];
  uint32[2] = temp[1];
  uint32[3] = temp[0];

  unsigned int *erg = (unsigned int*) &uint32[0];


  return (int) *erg;
#else
  return (int)val;
#endif
}

icUInt16Number
icValue (icUInt16Number val)
{
#if BYTE_ORDER == LITTLE_ENDIAN
# define BYTES 2
# define KORB  4
  unsigned char        *temp  = (unsigned char*) &val;
  static unsigned char  korb[KORB];
  for (int i = 0; i < KORB ; i++ )
    korb[i] = (int) 0;  // leeren

  int klein = 0,
      gross = BYTES - 1;
  for (; klein < BYTES ; klein++ ) {
    korb[klein] = temp[gross--];
#   ifdef DEBUG_ICCFUNKT
    cout << klein << " "; DBG_PROG
#   endif
  }

  unsigned int *erg = (unsigned int*) &korb[0];

# ifdef DEBUG_ICCFUNKT
# if 0
  cout << *erg << " Größe nach Wandlung " << (int)korb[0] << " "
       << (int)korb[1] << " " << (int)korb[2] << " " <<(int)korb[3]
       << " "; DBG_PROG
# else
  cout << *erg << " Größe nach Wandlung " << (int)temp[0] << " " << (int)temp[1]
       << " "; DBG_PROG
# endif
# endif
# undef BYTES
# undef KORB
  return (long)*erg;
#else
  return (long)val;
#endif
}


double
icUFValue (icU16Fixed16Number val)
{
  return icValue(val) / 65536.0;
}


struct icChromaticity {
  char sig[4];
  char dummy[4];
  icUInt16Number n_channels;
  icUInt16Number code;
  icU16Fixed16Number r[2];
  icU16Fixed16Number g[2];
  icU16Fixed16Number b[2];
};

Imf::Chromaticities
writeEXRprims (char *buf, size_t size, double *g)
{
  Imf::Chromaticities image_prim;
    {
      char *mem_profile=NULL;

      mem_profile = buf;
      cmsCIEXYZTRIPLE icc_XYZ;
      cmsHPROFILE p = cmsOpenProfileFromMem( mem_profile, size);
      if(p)
      if(cmsTakeColorants(&icc_XYZ, p))
      {
        cmsCIEXYZ icc_XYZwhite;
        cmsCIExyY icc_xyYwhite;
        if(!cmsTakeMediaWhitePoint (&icc_XYZwhite, p))
        { printf("no MediaWhitePoint found in attached image profile");
        }
        cmsXYZ2xyY(&icc_xyYwhite, &icc_XYZwhite);

        cmsCIExyYTRIPLE icc_xyY;
        cmsXYZ2xyY(&icc_xyY.Red, &icc_XYZ.Red);
        cmsXYZ2xyY(&icc_xyY.Green, &icc_XYZ.Green);
        cmsXYZ2xyY(&icc_xyY.Blue, &icc_XYZ.Blue);

        // first try: D50 Prims -> Wtpt Prims
        MAT3 MColorants;
        if (!cmsBuildRGB2XYZtransferMatrix(&MColorants, &icc_xyYwhite, &icc_xyY))
        cmsAdaptMatrixFromD50( &MColorants, &icc_xyYwhite);
        icc_XYZ.Red.X = MColorants.v[0].n[0];
        icc_XYZ.Red.Y = MColorants.v[1].n[0];
        icc_XYZ.Red.Z = MColorants.v[2].n[0];

        icc_XYZ.Green.X = MColorants.v[0].n[1];
        icc_XYZ.Green.Y = MColorants.v[1].n[1];
        icc_XYZ.Green.Z = MColorants.v[2].n[1];

        icc_XYZ.Blue.X = MColorants.v[0].n[2];
        icc_XYZ.Blue.Y = MColorants.v[1].n[2];
        icc_XYZ.Blue.Z = MColorants.v[2].n[2];

        cmsXYZ2xyY(&icc_xyY.Red, &icc_XYZ.Red);
        cmsXYZ2xyY(&icc_xyY.Green, &icc_XYZ.Green);
        cmsXYZ2xyY(&icc_xyY.Blue, &icc_XYZ.Blue);

        // second try: chrm direkt - better
        icProfile *mem_p = (icProfile*) mem_profile;
        int n_tags = icValue(mem_p->count);
        icTag *tl = (icTag*) &((char*)mem_profile)[132];
        for(int i = 0; i < n_tags; ++i) {
          icTag *tag = &tl[i];
          char* sig = (char*)&tag->sig;
          if (sig[0] == 'c' && 
              sig[1] == 'h' &&
              sig[2] == 'r' &&
              sig[3] == 'm' ) {
            icUInt32Number offset = icValue(tag->offset);
            icUInt32Number size = icValue(tag->size);
            icChromaticity *chrm = (icChromaticity*) &mem_profile[offset];

            if(icValue(chrm->n_channels) == 3) {
              icc_xyY.Red.x = icUFValue(chrm->r[0]);
              icc_xyY.Red.y = icUFValue(chrm->r[1]);
              icc_xyY.Green.x = icUFValue(chrm->g[0]);
              icc_xyY.Green.y = icUFValue(chrm->g[1]);
              icc_xyY.Blue.x = icUFValue(chrm->b[0]);
              icc_xyY.Blue.y = icUFValue(chrm->b[1]);
            }
          }
        }

        Imf::Chromaticities xyY(Imath::V2f (icc_xyY.Red.x, icc_xyY.Red.y),
                                Imath::V2f (icc_xyY.Green.x, icc_xyY.Green.y),
                                Imath::V2f (icc_xyY.Blue.x, icc_xyY.Blue.y),
                                Imath::V2f (icc_xyYwhite.x, icc_xyYwhite.y));
        image_prim = xyY;
      }
      if(g)
      {
        LPGAMMATABLE gamma[3];
        gamma[0] = cmsReadICCGamma(p, icSigRedTRCTag);
        gamma[1] = cmsReadICCGamma(p, icSigGreenTRCTag);
        gamma[2] = cmsReadICCGamma(p, icSigBlueTRCTag);
        g[0] = cmsEstimateGamma( gamma[0] );
        g[1] = cmsEstimateGamma( gamma[1] );
        g[2] = cmsEstimateGamma( gamma[2] );
        cmsFreeGammaTriple(gamma);
      }
    }
#if DEBUG
  printf("%s:%d\nred xy: %f %f\ngreen xy: %f %f\nblue xy: %f %f\nwhite xy: %f %f \ngamma : %.02f %.02f %.02f\n",__FILE__,__LINE__,
                 image_prim.red[0],image_prim.red[1],
                 image_prim.green[0],image_prim.green[1],
                 image_prim.blue[0],image_prim.blue[1],
                 image_prim.white[0],image_prim.white[1],
                 g[0], g[1], g[2]);
#endif
  return image_prim;
}


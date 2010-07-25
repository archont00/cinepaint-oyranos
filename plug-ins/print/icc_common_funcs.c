/*
 *   ICC profile handling for CinePaint
 *
 *   Copyright 2004 Kai-Uwe Behrmann
 *
 *   separate icc stuff / tiff - print
 *    Kai-Uwe Behrman (ku.b@gmx.de) --  March 2004
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "lcms.h"

#ifdef HAVE_CONFIG_H
//#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

//#include <gdk/gdkkeysyms.h>
//#include <gtk/gtk.h>

#include "print_gimp.h"
//#include "tiff_plug_in.h"
#include "icc_common_funcs.h"

#ifndef DEBUG
// #define DEBUG
#endif

#ifdef DEBUG
 #define DBG(text) printf("%s:%d %s() "text"\n",__FILE__,__LINE__,__func__);
#else
 #define DBG(text) 
#endif

/*** variables ***/


/*** function declarations ***/

  // --- variable function ---

  // --- cms functions ---

void
lcms_error (int ErrorCode, const char* ErrorText)
{
   g_message ("LCMS error:%d %s", ErrorCode, ErrorText);
}

static char* cp_char (char* text)
{
  char* string;

  string = malloc(sizeof(char)*strlen(text)+1);
  sprintf(string, text);

  return string;
}

char* getPCSName (cmsHPROFILE hProfile)
{
  char* name;

  switch (cmsGetPCS (hProfile))
  {
    case icSigXYZData: name = cp_char (_("XYZ")); break;
    case icSigLabData: name = cp_char (_("Lab")); break;
    default: name = cp_char (_("")); break;
  }
  return name;
}

char* getColorSpaceName (cmsHPROFILE hProfile)
{
  char* name;

  switch (cmsGetColorSpace (hProfile))
  {
    case icSigXYZData: name = cp_char (_("XYZ")); break;
    case icSigLabData: name = cp_char (_("Lab")); break;
    case icSigLuvData: name = cp_char (_("Luv")); break;
    case icSigYCbCrData: name = cp_char (_("YCbCr")); break;
    case icSigYxyData: name = cp_char (_("Yxy")); break;
    case icSigRgbData: name = cp_char (_("Rgb")); break;
    case icSigGrayData: name = cp_char (_("Gray")); break;
    case icSigHsvData: name = cp_char (_("Hsv")); break;
    case icSigHlsData: name = cp_char (_("Hls")); break;
    case icSigCmykData: name = cp_char (_("Cmyk")); break;
    case icSigCmyData: name = cp_char (_("Cmy")); break;
    case icSig2colorData: name = cp_char (_("2color")); break;
    case icSig3colorData: name = cp_char (_("3color")); break;
    case icSig4colorData: name = cp_char (_("4color")); break;
    case icSig5colorData: name = cp_char (_("5color")); break;
    case icSig6colorData: name = cp_char (_("6color")); break;
    case icSig7colorData: name = cp_char (_("7color")); break;
    case icSig8colorData: name = cp_char (_("8color")); break;
    case icSig9colorData: name = cp_char (_("9color")); break;
    case icSig10colorData: name = cp_char (_("10color")); break;
    case icSig11colorData: name = cp_char (_("11color")); break;
    case icSig12colorData: name = cp_char (_("12color")); break;
    case icSig13colorData: name = cp_char (_("13color")); break;
    case icSig14colorData: name = cp_char (_("14color")); break;
    case icSig15colorData: name = cp_char (_("15color")); break;
    default: name = cp_char (_("")); break;
  }
  return name;
}

char* getDeviceClassName (cmsHPROFILE hProfile)
{
  char* name;

  switch (cmsGetDeviceClass (hProfile))
  {
    case icSigInputClass: name = cp_char (_("Input")); break;
    case icSigDisplayClass: name = cp_char (_("Display")); break;
    case icSigOutputClass: name = cp_char (_("Output")); break;
    case icSigLinkClass: name = cp_char (_("Link")); break;
    case icSigAbstractClass: name = cp_char (_("Abstract")); break;
    case icSigColorSpaceClass: name = cp_char (_("ColorSpace")); break;
    case icSigNamedColorClass: name = cp_char (_("NamedColor")); break;
    default: name = cp_char (_("")); break;
  }
  return name;
}

char*
getSigTagName               (   icTagSignature  sig )
{
  char* name;

  switch (sig) {
    case icSigAToB0Tag: name = cp_char (_("A2B0")); break;
    case icSigAToB1Tag: name = cp_char (_("A2B1")); break;
    case icSigAToB2Tag: name = cp_char (_("A2B2")); break;
    case icSigBlueColorantTag: name = cp_char (_("bXYZ")); break;
    case icSigBlueTRCTag: name = cp_char (_("bTRC")); break;
    case icSigBToA0Tag: name = cp_char (_("B2A0")); break;
    case icSigBToA1Tag: name = cp_char (_("B2A1")); break;
    case icSigBToA2Tag: name = cp_char (_("B2A2")); break;
    case icSigCalibrationDateTimeTag: name = cp_char (_("calt")); break;
    case icSigCharTargetTag: name = cp_char (_("targ")); break;
    case icSigCopyrightTag: name = cp_char (_("cprt")); break;
    case icSigCrdInfoTag: name = cp_char (_("crdi")); break;
    case icSigDeviceMfgDescTag: name = cp_char (_("dmnd")); break;
    case icSigDeviceModelDescTag: name = cp_char (_("dmdd")); break;
    case icSigGamutTag: name = cp_char (_("gamt")); break;
    case icSigGrayTRCTag: name = cp_char (_("kTRC")); break;
    case icSigGreenColorantTag: name = cp_char (_("gXYZ")); break;
    case icSigGreenTRCTag: name = cp_char (_("gTRC")); break;
    case icSigLuminanceTag: name = cp_char (_("lumi")); break;
    case icSigMeasurementTag: name = cp_char (_("meas")); break;
    case icSigMediaBlackPointTag: name = cp_char (_("bkpt")); break;
    case icSigMediaWhitePointTag: name = cp_char (_("wtpt")); break;
    case icSigNamedColorTag: name = cp_char (_("'ncol")); break;
    case icSigNamedColor2Tag: name = cp_char (_("ncl2")); break;
    case icSigPreview0Tag: name = cp_char (_("pre0")); break;
    case icSigPreview1Tag: name = cp_char (_("pre1")); break;
    case icSigPreview2Tag: name = cp_char (_("pre2")); break;
    case icSigProfileDescriptionTag: name = cp_char (_("desc")); break;
    case icSigProfileSequenceDescTag: name = cp_char (_("pseq")); break;
    case icSigPs2CRD0Tag: name = cp_char (_("psd0")); break;
    case icSigPs2CRD1Tag: name = cp_char (_("psd1")); break;
    case icSigPs2CRD2Tag: name = cp_char (_("psd2")); break;
    case icSigPs2CRD3Tag: name = cp_char (_("psd3")); break;
    case icSigPs2CSATag: name = cp_char (_("ps2s")); break;
    case icSigPs2RenderingIntentTag: name = cp_char (_("ps2i")); break;
    case icSigRedColorantTag: name = cp_char (_("rXYZ")); break;
    case icSigRedTRCTag: name = cp_char (_("rTRC")); break;
    case icSigScreeningDescTag: name = cp_char (_("scrd")); break;
    case icSigScreeningTag: name = cp_char (_("scrn")); break;
    case icSigTechnologyTag: name = cp_char (_("tech")); break;
    case icSigUcrBgTag: name = cp_char (_("bfd")); break;
    case icSigViewingCondDescTag: name = cp_char (_("vued")); break;
    case icSigViewingConditionsTag: name = cp_char (_("view")); break;
    default: name = cp_char (_("")); break;
  }
  return name;
}

int getLcmsGetColorSpace ( cmsHPROFILE   hProfile)
{
  int lcmsColorSpace;

  #if (LCMS_VERSION >= 113)
    lcmsColorSpace = _cmsLCMScolorSpace ( cmsGetColorSpace (hProfile) );
  #else
  switch (cmsGetColorSpace (hProfile))
  {
    case icSigXYZData: lcmsColorSpace = PT_XYZ; break;
    case icSigLabData: lcmsColorSpace = PT_Lab; break;
    case icSigLuvData: lcmsColorSpace = PT_YUV; break;
    case icSigLuvKData: lcmsColorSpace = PT_YUVK; break;
    case icSigYCbCrData: lcmsColorSpace = PT_YCbCr; break;
    case icSigYxyData: lcmsColorSpace = PT_Yxy; break;
    case icSigRgbData: lcmsColorSpace = PT_RGB; break;
    case icSigGrayData: lcmsColorSpace = PT_GRAY; break;
    case icSigHsvData: lcmsColorSpace = PT_HSV; break;
    case icSigHlsData: lcmsColorSpace = PT_HLS; break;
    case icSigCmykData: lcmsColorSpace = PT_CMYK; break;
    case icSigCmyData: lcmsColorSpace = PT_CMY; break;
    case icSig2colorData: lcmsColorSpace = PT_ANY; break;
    case icSig3colorData: lcmsColorSpace = PT_ANY; break;
    case icSig4colorData: lcmsColorSpace = PT_ANY; break;
    case icSig5colorData: lcmsColorSpace = PT_ANY; break;
    case icSig6colorData: lcmsColorSpace = PT_ANY; break;
    case icSig7colorData: lcmsColorSpace = PT_ANY; break;
    case icSig8colorData: lcmsColorSpace = PT_ANY; break;
    case icSig9colorData: lcmsColorSpace = PT_ANY; break;
    case icSig10colorData: lcmsColorSpace = PT_ANY; break;
    case icSig11colorData: lcmsColorSpace = PT_ANY; break;
    case icSig12colorData: lcmsColorSpace = PT_ANY; break;
    case icSig13colorData: lcmsColorSpace = PT_ANY; break;
    case icSig14colorData: lcmsColorSpace = PT_ANY; break;
    case icSig15colorData: lcmsColorSpace = PT_ANY; break;
    case icSigHexachromeData: lcmsColorSpace = PT_HiFi; break; // speculation
    default: lcmsColorSpace = PT_ANY; break;
  }
  #endif
  return lcmsColorSpace;
}

char*
getProfileInfo                   ( cmsHPROFILE  hProfile)
{
  gchar *text;
  gchar *profile_info;
  cmsCIEXYZ WhitePt;
  int       first = FALSE,
            min_len = 24,
            len, i;

  profile_info = calloc( sizeof(char), 2048 );
  text = malloc(128);

  {  
#if LCMS_VERSION > 112
    if (cmsIsTag(hProfile, icSigCopyrightTag)) {
      len = strlen (cmsTakeCopyright(hProfile)) + 16;
      if (len > min_len)
        min_len = len + 1;
    }
#endif
    profile_info[0] = '\000';
    //sprintf (text, _("Product Info"));
    //sprintf (profile_info, "%s\n%s ", profile_info, text);
    if (cmsIsTag(hProfile, icSigProfileDescriptionTag)) {
      sprintf (text,_("Description:    "));
      sprintf (profile_info,"%s%s %s",profile_info,text,
                              cmsTakeProductDesc(hProfile));
      if (!first) {
        len = min_len - strlen (profile_info);

        for (i=0; i < len * 2.2; i++) {
          sprintf (profile_info,"%s ",profile_info);
        }
      }
      sprintf (profile_info,"%s\n",profile_info);
    }
    if (cmsIsTag(hProfile, icSigDeviceMfgDescTag)) {
      sprintf (text,_("Product:        "));
      sprintf (profile_info,"%s%s %s\n",profile_info,text,
                              cmsTakeProductName(hProfile));
    }
#if LCMS_VERSION >= 112
    if (cmsIsTag(hProfile, icSigDeviceMfgDescTag)) {
      sprintf (text,_("Manufacturer:   "));
      sprintf (profile_info,"%s%s %s\n",profile_info,text,
                              cmsTakeManufacturer(hProfile));
    }
    if (cmsIsTag(hProfile, icSigDeviceModelDescTag)) {
      sprintf (text,_("Model:          "));
      sprintf (profile_info,"%s%s %s\n",profile_info,text,
                              cmsTakeModel(hProfile));
    }
#endif
#if LCMS_VERSION >= 113
    if (cmsIsTag(hProfile, icSigCopyrightTag)) {
      sprintf (text,_("Copyright:      "));
      sprintf (profile_info,"%s%s %s\n",profile_info,text,
                              cmsTakeCopyright(hProfile));
    }
#endif
    sprintf (profile_info,"%s\n",profile_info);

    cmsTakeMediaWhitePoint (&WhitePt, hProfile);
    _cmsIdentifyWhitePoint (text, &WhitePt);
    sprintf (profile_info, "%s%s\n", profile_info, text);

      sprintf (text,_("Device Class:   "));
      sprintf (profile_info,"%s%s %s\n",profile_info,text,
                              getDeviceClassName(hProfile));
      sprintf (text,_("Color Space:    "));
      sprintf (profile_info,"%s%s %s\n",profile_info,text,
                              getColorSpaceName(hProfile));
      sprintf (text,_("PCS Space:      "));
      sprintf (profile_info,"%s%s %s",profile_info,text,
                              getPCSName(hProfile));
  }

#ifdef __unix__  // reformatting the \n\r s of lcms
  if (0)
        while (strchr(profile_info, '\r')) {
          len = (int)strchr (profile_info, '\r') - (int)profile_info;
          profile_info[len] = 32;
          profile_info[len+1] = 32;
          profile_info[len+2] = 32;
          profile_info[len+3] = '\n';
        }
#endif
  free (text);
  
  return profile_info;
}

int checkProfileDevice (char* type, cmsHPROFILE hProfile)
{
  int check = TRUE;

  if ((strcmp(type, _("Work Space"))) == 0) {
      // test for device class
      switch (cmsGetDeviceClass (hProfile))
        {
        case icSigInputClass:
        case icSigDisplayClass:
        case icSigOutputClass:
        case icSigLinkClass:
        case icSigAbstractClass:
        case icSigColorSpaceClass:
        case icSigNamedColorClass:
          // should be good
          break;
        default:
          g_message ("%s \"%s %s: %s",_("Your"), type,
                     _("Profile\" is designed for an other device"),
                     getDeviceClassName(hProfile));
          check = FALSE;
          break;
        }
  } else if ((strcmp(type, _("Separation"))) == 0) {
      switch (cmsGetDeviceClass (hProfile))
        {
//        case icSigInputClass:
//        case icSigDisplayClass:
        case icSigOutputClass:
        case icSigLinkClass:
        case icSigAbstractClass:
        case icSigColorSpaceClass:
        case icSigNamedColorClass:
          // should be good
          break;
        default:
          g_message ("%s - %s - %s \"%s %s",_("Device class"),
                     getDeviceClassName(hProfile),
                     _("is not valid for an"),
                     type,
                     _("Profile\"."));
          check = FALSE;
          break;
        if (icSigCmykData   != cmsGetColorSpace(hProfile))
          check = FALSE;
        }
  } else if ((strcmp(type, _("Linearisation"))) == 0) {
      switch (cmsGetDeviceClass (hProfile))
        {
//        case icSigInputClass:
//        case icSigDisplayClass:
//        case icSigOutputClass:
        case icSigLinkClass:
//        case icSigAbstractClass:
//        case icSigColorSpaceClass:
//        case icSigNamedColorClass:
          // should be good
          break;
        default:
          g_message ("%s - %s - %s \"%s %s",_("Device class"),
                     getDeviceClassName(hProfile),
                     _("is not valid for an"),
                     type,
                     _("Profile\"."));
          check = FALSE;
          break;
        if (icSigCmykData   != cmsGetColorSpace(hProfile))
          g_message ("%s - %s - %s \"%s %s",_("Color space"),
                     getColorSpaceName(hProfile),
                     _("is not valid for an"),
                     type,
                     _("Profile at the moment\"."));
          check = FALSE;
        }
  }
  return check;
}


void
printProfileTags            (   cmsHPROFILE     hProfile )
{
#ifdef DEBUG
  #if LCMS_VERSION >= 113
  LPLCMSICCPROFILE p = (LPLCMSICCPROFILE) hProfile;
  int              i;

  printf ("TagCount = %d \n",p->TagCount);
  for (i=0; i < p->TagCount ; i++) {
    char *name = getSigTagName(p->TagNames[i]);
    long   len = strlen (name);

    if (len <= 1) {
      name = calloc (sizeof (char), 256);
      sprintf (name, "%x", p->TagNames[i]);
    }
    printf ("%d: TagNames %s TagSizes %d TagOffsets %d TagPtrs %d\n",
             i, name, p->TagSizes[i], p->TagOffsets[i], (int)p->TagPtrs[i]);
    free (name);
  }
  #endif
#endif
}

void
printLut                    (   LPLUT           Lut,
                                int             sig)
{
  int               i,
                    channels=0;

  printf ("Channels In/Out = %d/%d Nr: %d/%d\n",Lut->InputChan, Lut->OutputChan,
                                        Lut->InputEntries, Lut->OutputEntries);

  switch (sig)
  {
  case icSigAToB0Tag:
  case icSigAToB1Tag:
  case icSigAToB2Tag:
    channels = Lut ->InputChan;
    break;
  case icSigBToA0Tag:
  case icSigBToA1Tag:
  case icSigBToA2Tag:
    channels = Lut ->OutputChan;
    break;
  case icSigGamutTag:
    channels = 1;
    break;
  }
  channels = Lut->OutputChan;

  for (i=0; i < Lut ->InputEntries; i++)  {
    if (channels > 0)
      printf ("C=%d ", Lut ->L2[0][i]);
    if (channels > 1)
      printf ("M=%d ", Lut ->L2[1][i]);
    if (channels > 2)
      printf ("Y=%d ", Lut ->L2[2][i]);
    if (channels > 3)
      printf ("K=%d ", Lut ->L2[3][i]);
  }
  printf ("\n");
}


cmsHPROFILE
copyProfile                 (   cmsHPROFILE     hIn,
                                cmsHPROFILE     hOut)
{
  LPLUT             Lut;
  #if LCMS_VERSION > 112
  char              text[1048576];
  #endif
  LPcmsCIEXYZ       XYZ;
  LPcmsCIEXYZTRIPLE Colorants=NULL;
  LPMAT3            r=NULL;

  DBG ("cpLut:")

  //printProfileTags (hOut);

  #define cpLut(sig) \
  if (cmsIsTag (hIn,icSigAToB0Tag)) { \
    DBG (#sig " ...") \
    Lut =  cmsReadICCLut( hIn, icSigAToB0Tag); \
    if (!Lut) \
      printf ("%s:%d %s() error reading "#sig"\n",__FILE__,__LINE__,__func__); \
    else \
      if (!_cmsAddLUTTag ( hOut, icSigAToB0Tag, Lut )) \
        printf ("%s:%d %s() error writing tag\n",__FILE__,__LINE__,__func__); \
      else \
        /*printLut (Lut, sig)*/; \
  }
//  cpLut ( icSigAToB0Tag )
//  cpLut ( icSigAToB1Tag )
//  cpLut ( icSigAToB2Tag )
  cpLut ( icSigBToA0Tag )
  cpLut ( icSigBToA1Tag )
  cpLut ( icSigBToA2Tag )
  cpLut ( icSigGamutTag )
#if 0
//  cpLut ( icSigPreview0Tag )
//  cpLut ( icSigPreview1Tag )
//  cpLut ( icSigPreview2Tag )

  DBG ("cpText:")
  #if LCMS_VERSION > 112
  #define cpText(sig) \
  if (cmsIsTag (hIn,sig)) { \
    DBG (#sig " ...") \
    if (cmsReadICCText ( hIn, sig, text)) \
      cmsAddTag(hOut, icSigDeviceMfgDescTag,(void*)text); \
    else \
      printf ("%s:%d %s() error reading "#sig"\n",__FILE__,__LINE__,__func__); \
  }
  cpText ( icSigProfileDescriptionTag )
  cpText ( icSigDeviceMfgDescTag )
  cpText ( icSigDeviceModelDescTag )
  cpText ( icSigCopyrightTag )
  cpText ( icSigCharTargetTag )
  cpText ( icSigCrdInfoTag )
  #else
  #define cpText(sig, function) \
  if (cmsIsTag (hIn,sig)) { \
    DBG (#sig " ...") \
    cmsAddTag(hOut, icSigDeviceMfgDescTag, (void*) function ( hIn )); \
  }
  cpText ( icSigProfileDescriptionTag , cmsTakeProductDesc )
  cpText ( icSigDeviceMfgDescTag ,      cmsTakeManufacturer)
  cpText ( icSigDeviceModelDescTag ,    cmsTakeModel )
//  cpText ( icSigCopyrightTag ,          cmsTakeCopyright )
//  cpText ( icSigCharTargetTag ,         cmsTakeCharTargetData )
//  cpText ( icSigCrdInfoTag ,            )
  #endif

/*  if (cmsIsTag (hIn, icSigCharTargetTag)) {
    cmsReadICCText (hIn, icSigCharTargetTag, text);
    cmsAddTag(hOut, icSigDeviceMfgDescTag, (void*) text);
  }*/
  if (cmsIsTag (hIn, icSigProfileSequenceDescTag))
    cmsAddTag(hOut, icSigProfileSequenceDescTag,
                    cmsReadProfileSequenceDescription ( hIn ));

  DBG ("cpXYZ:")
  #define cpXYZ( sig, function ) \
  if (cmsIsTag (hIn, sig)) { \
    DBG (#sig " ...") \
    if (function ( (LPcmsCIEXYZ) &XYZ, hIn )) \
      cmsAddTag(hIn, sig,  &XYZ); \
    else \
      printf ("%s:%d %s() error reading "#sig"\n",__FILE__,__LINE__,__func__); \
  }
  cpXYZ ( icSigMediaWhitePointTag , cmsTakeMediaWhitePoint )
  cpXYZ ( icSigMediaBlackPointTag , cmsTakeMediaBlackPoint )
  cpXYZ ( icSigLuminanceTag ,       cmsTakeIluminant )
  if (cmsIsTag (hIn, icSigChromaticAdaptationTag)) {
    cmsReadChromaticAdaptationMatrix ( r, hIn );  // chad
    printf ("What shall to do with tag 'chad' - ChromaticAdaptation ?\n");
  } //cmsSetMatrixLUT ?

  if (cmsIsTag (hIn, icSigRedColorantTag)) {
    cmsTakeColorants (Colorants, hIn);
    cmsAddTag(hOut, icSigRedColorantTag,   (LPcmsCIEXYZ) &Colorants->Red);
    cmsAddTag(hOut, icSigBlueColorantTag,  (LPcmsCIEXYZ) &Colorants->Blue);
    cmsAddTag(hOut, icSigGreenColorantTag, (LPcmsCIEXYZ) &Colorants->Green);
  }

  DBG ("cpGamma:")
  #define cpGamma( sig ) \
  if (cmsIsTag (hIn, sig)) { \
    DBG (#sig " ...") \
    if (_cmsAddGammaTag (hOut, sig, cmsReadICCGamma ( hIn, sig))) \
      ; \
    else \
      printf ("%s:%d %s() error reading "#sig"\n",__FILE__,__LINE__,__func__); \
  }
  cpGamma ( icSigRedTRCTag )
  cpGamma ( icSigGreenTRCTag )
  cpGamma ( icSigBlueTRCTag )
  cpGamma ( icSigGrayTRCTag )

  DBG ("NamedColor2: ..")
  if (cmsIsTag (hIn, icSigNamedColor2Tag))
    printf ("What shall to do with tag 'ncl2' - NamedColor2 ?\n");
#endif

  g_print ("%s:%d %s() ", __FILE__,__LINE__,__func__);
  printProfileTags (hOut);

  DBG ("Schluﬂ")
  return hOut;
}


void
saveProfileToFile (char* filename, char *profile, int size)
{
  FILE *fp=NULL;
  int   pt = 0;

  if ((fp=fopen(filename, "w")) != NULL) {
    do {
      fputc ( profile[pt++] , fp);
    } while (--size);
  }

  fclose (fp);
}


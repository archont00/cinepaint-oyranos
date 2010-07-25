/*
 * br_enums.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  br_enums.hpp
*/
#ifndef br_enums_hpp
#define br_enums_hpp


namespace br {

enum DataType {
        DATA_NONE,              /**<  not set */
        DATA_U8,                /**<  8-bit unsigned integer */
        DATA_U16,               /**<  16-bit unsigned integer */
        DATA_F16,               /**<  16-bit floating point (future) */
        DATA_F32,               /**<  32-bit IEEE-floats (`float') */
        DATA_F64,               /**<  64-bit IEEE-floats (`double') */
        DATA_UNKNOWN            /**<  marks also the end */
};

enum StoringScheme {
        STORING_NONE,           /**<  not set */
        STORING_INTERLEAVE,     /**<  RGBRGB... */
        STORING_CHNNLWISE,      /**<  RRR...GGG...BBB... */
        STORING_UNKNOWN         /**<  marks also the end */
};

/**  Attempt to map DataType + number of channels + StoringScheme on a linear
      scheme -- at least for the most used combinations: */
enum ImageType {
        IMAGE_NONE,
        IMAGE_RGB_U8,
        IMAGE_RGB_U16,
        IMAGE_RGB_F32,
        IMAGE_RGB_F64,
        IMAGE_UNKNOWN           /**<  marks also the end */
};

enum ReportWhat {
        REPORT_NONE        = 0,             /**<  not set */
        REPORT_BASE        = 1,             /**<  base information */
        REPORT_EXPOSE_INFO = 2,             /**<  exposure data */
        REPORT_STATISTICS  = 4,             /**<  statistics data */
        REPORT_ALL         = 1 | 2 | 4,     /**<  report all */
        REPORT_DEFAULT     = REPORT_ALL     /**<  report a default volume */
};


}       // namespace

#endif  // br_enums_hpp

// END OF FILE


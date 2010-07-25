/*
 * br_eventnames.hpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
#ifndef br_eventnames_hpp
#define br_eventnames_hpp


namespace br {

/// Names of the Br2Hdr::Event enums for debug output...
const char* const EVENT_NAMES[] =
{   "NO_EVENT",
    "IMG_VECTOR_SIZE",
    "SIZE_ACTIVE",
    "CALCTOR_INIT",
    "CALCTOR_REDATED",
    "CALCTOR_DELETED",
    "CCD_UPDATED",
    "CCD_OUTDATED",
    "CCD_DELETED",
    "HDR_UPDATED",
    "TIMES_CHANGED",
    "FOLLOWUP_UPDATED",
    "FOLLOWUP_OUTDATED",
    "EXTERN_RESPONSE",
    "WEIGHT_CHANGED",
    "OFFSET_COMPUTED",
    "ACTIVE_OFFSETS",
    "UNKNOW_EVENT"
};

}  // namespace "br"

#endif  // br_eventnames_hpp

// END OF FILE



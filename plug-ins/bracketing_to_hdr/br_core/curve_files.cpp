/*
 * curve_files.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  curve_files.cpp
*/
#include <cstdio>
#include <cstring>
#include "curve_files.hpp"
#include "FilePtr.hpp"
#include "Exception.hpp"


/******************************************
  DECLARATION of local (static) functions 
*******************************************/
static int      words_in_line       (const char* line);
static bool     is_empty_line       (const char* line);
static char*    remove_lf           (char* line);
static void     format_error_msg    (const char* format, int lineno, 
                                     const char* line, const char* fname);
                                         
/****************************************************
  @return  Number of ' '-separated words in `line'
*****************************************************/
int
words_in_line (const char* line)
{
    bool in_word = false;
    int  n = 0;
    
    while (*line != '\0' && *line != '\n')
    {
      if (*line != ' ') {
        if (! in_word) {
          n++; 
          in_word = true;
        }
      }
      else
        in_word = false;
      
      line ++;
    }  
    return n;
}

/**********************************************************
  @return true if `line' consists only of ' ' chars
***********************************************************/
bool
is_empty_line (const char* line)
{
    while (*line) 
    {
      if (*line != ' ') return false;
      line ++;
    }
    return true;
}

/**********************************************************
  Remove a final '\n' from string `line'.
***********************************************************/
char* 
remove_lf (char* line)
{
    if (!line || !line[0]) return line;
    int n = strlen (line) - 1;
    if (line[n] == '\n') line[n] = 0;
    return line; 
}

/**********************************************************
  Helper for open_curve_file().
***********************************************************/
void 
format_error_msg (const char* format, int lineno, const char* line, const char* fname)
{
    printf("\nBr2Hdr-CRITICAL ***: wrong line in curve file \"%s\":\n", fname);
    printf("\tNo `%s' format in line %d: \"%s\"\n", format, lineno, line);
}


/**+*************************************************************************\n
  read_curve_file()  
  
  @param fname: name of the curve file to read
  @param npoints: number of points to read
  @param buf: buffer of npoints doubles the read values shall be written to.
  @return  0 for ok, -1 for error, +1 for warning (incomplete read)
  
  Can read files with two values per line as well as with one value ("x y" and 
   "y" syntax). But currently only the y-values are written to `buf'.
  Intended as auxillary routine without TNT dependency. In future curve classes
   consist probably of (x,y)-pairs instead of y-values allone (more general).
   Then we should take `buf' as "xyxyxy..." array.
   
  Zur Zeit muss Puffer passend geliefert werden. Koennte auch hier allokiert werden. 
******************************************************************************/
int 
read_curve_file (const char* fname, int npoints, double* buf)
{
  try {
    FilePtr file (fname, "rt");         // throws, if file doesn't exist
    char  line [256];
    float x,y;
    int   cols = 0;
    int   n_line = 0;
    int   n_scanned = 0;
    bool  first_line = true;

    while (fgets (line, 256, file))
    {  
      n_line ++;        // info for error messages
      //printf("%2d: \"%s\"\n", n_line, remove_lf(line));
      /*  Skip comments and empty lines */
      if (*line == '#' || *line == '\n' || is_empty_line(line)) 
        continue;
      /*  At first regular line find out number of columns */
      if (first_line) {
        cols = words_in_line(line);
        if (!cols)               // line of ' ' chars: skip it 
          continue;
        else {
          first_line = false;
          if (cols > 2) cols = 2;
        }
      }
      if (n_scanned >= npoints) {
        printf("\nBr2Hdr-WARNING **: More than %d points in curve file \"%s\"\n", npoints, fname);
        printf("\t\tCut to %d points.\n", npoints);
        return 0;       
      }
      /*  Scan the line... */
      if (cols == 1) {  
        if (sscanf (line, "%f", &y) < 1) {
          format_error_msg ("%%f", n_line, remove_lf(line), fname);
          return -1;
        }
        //printf("%2d: y=%f\n", n_scanned, y); 
        buf [n_scanned] = y;
      }
      else {
        if (sscanf (line, "%f %f", &x, &y) < 2) {
          format_error_msg ("%%f %%f", n_line, remove_lf(line), fname);
          return -1;
        }
        //printf("%2d: x=%f  y=%f\n", n_scanned, x,y); 
        buf [n_scanned] = y;
      }
      n_scanned ++;
    }
    
    if (n_scanned < npoints) {
      printf("\nBr2Hdr-WARNING **: Only %d of %d points found in curve file \"%s\"\n", 
          n_scanned, npoints, fname);
      printf("\t\tFilled up with zeros.\n");    
      
      for (int i=n_scanned; i < npoints; i++)  buf[i] = 0.0;    
      return 1;
    }
  }
  catch (br::Exception e) {
    return -1;
  }  
  
  return 0;
}


/**+*************************************************************************\n
  read_curve_file()  
  
  @param fname: (I) Name of the curve file to read.
  @param A: (O) Curve object TNT::Array1D to fill. If \c A.is_null(), we allocate
    it here for 256 values. If \c !A.is_null(), we try to read \c A.dim() values
    from the file. (Could be A.dim()==0 - a problem?) If \a A was given as
    Null-Array and a read error occured, we leave behind again a Null-Array.
  @return  0 for ok, -1 for error, +1 for warning (incomplete read)
******************************************************************************/
int
read_curve_file (const char* fname, TNT::Array1D<double> & A)
{
    bool alloc_here = false;
    
    if (A.is_null()) {
      printf("%s(): Array is null: allocate\n", __func__);
      A = TNT::Array1D<double> (256);
      alloc_here = true;
    }  
    
    int retcode = read_curve_file (fname, A.dim(), &A[0]);
    
    /*  If a read error occured, we leave behind a Null-Array if we got one */
    if (retcode == -1  &&  alloc_here)
      A = TNT::Array1D<double>();
      
    return retcode;  
}


#if 0
#include "FilePtr.cpp"
#include "Exception.cpp"
#include "../TNT/tnt_array1d_utils.hpp"

int main (int argc, char** argv)
{
    printf("argc=%d  argv[0]=%s\n", argc, argv[0]);
    if (argc<2) return -1;
    
    double buf[256];
    TNT::Array1D<double> A;
    
    //read_curve_file (argv[1], 256, buf);
    read_curve_file (argv[1], A);
    //std::cout << A;
    return 0;
}
#endif

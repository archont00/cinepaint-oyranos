#ifndef __MARCHING_ANTS_H__
#define __MARCHING_ANTS_H__

/* static variable definitions */
static unsigned char ant_data[8][8] =
{
  {
    0xF0,    /*  ####----  */
    0xE1,    /*  ###----#  */
    0xC3,    /*  ##----##  */
    0x87,    /*  #----###  */
    0x0F,    /*  ----####  */
    0x1E,    /*  ---####-  */
    0x3C,    /*  --####--  */
    0x78,    /*  -####---  */
  },
  {
    0xE1,    /*  ###----#  */
    0xC3,    /*  ##----##  */
    0x87,    /*  #----###  */
    0x0F,    /*  ----####  */
    0x1E,    /*  ---####-  */
    0x3C,    /*  --####--  */
    0x78,    /*  -####---  */
    0xF0,    /*  ####----  */
  },
  {
    0xC3,    /*  ##----##  */
    0x87,    /*  #----###  */
    0x0F,    /*  ----####  */
    0x1E,    /*  ---####-  */
    0x3C,    /*  --####--  */
    0x78,    /*  -####---  */
    0xF0,    /*  ####----  */
    0xE1,    /*  ###----#  */
  },
  {
    0x87,    /*  #----###  */
    0x0F,    /*  ----####  */
    0x1E,    /*  ---####-  */
    0x3C,    /*  --####--  */
    0x78,    /*  -####---  */
    0xF0,    /*  ####----  */
    0xE1,    /*  ###----#  */
    0xC3,    /*  ##----##  */
  },
  {
    0x0F,    /*  ----####  */
    0x1E,    /*  ---####-  */
    0x3C,    /*  --####--  */
    0x78,    /*  -####---  */
    0xF0,    /*  ####----  */
    0xE1,    /*  ###----#  */
    0xC3,    /*  ##----##  */
    0x87,    /*  #----###  */
  },
  {
    0x1E,    /*  ---####-  */
    0x3C,    /*  --####--  */
    0x78,    /*  -####---  */
    0xF0,    /*  ####----  */
    0xE1,    /*  ###----#  */
    0xC3,    /*  ##----##  */
    0x87,    /*  #----###  */
    0x0F,    /*  ----####  */
  },
  {
    0x3C,    /*  --####--  */
    0x78,    /*  -####---  */
    0xF0,    /*  ####----  */
    0xE1,    /*  ###----#  */
    0xC3,    /*  ##----##  */
    0x87,    /*  #----###  */
    0x0F,    /*  ----####  */
    0x1E,    /*  ---####-  */
  },
  {
    0x78,    /*  -####---  */
    0xF0,    /*  ####----  */
    0xE1,    /*  ###----#  */
    0xC3,    /*  ##----##  */
    0x87,    /*  #----###  */
    0x0F,    /*  ----####  */
    0x1E,    /*  ---####-  */
    0x3C,    /*  --####--  */
  },
};


#endif  /*  __MARCHING_ANTS_H__  */

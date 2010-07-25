#ifndef __DISPLAYLUT_H__
#define __DIPSLAYLUT_H__

#include <gdk/gdktypes.h>

#define IM_OLDFILM_LOOKUP_START 14656
#define IM_OLDFILM_LOOKUP_END 16581

extern unsigned char IM_OldFilm8bitLookup[];

#define IM_FILM_LOOKUP_START 14862
#define IM_FILM_LOOKUP_END 16726

extern unsigned char IM_Film8bitLookup[];

#define IM_GAMMA22LOOKUP_START  13717
#define IM_GAMMA22LOOKUP_END    16254

extern unsigned char IM_Gamma22Lookup[];

void display_u8_init(void);

// fast lookup tables from 16bit float to unsigned 8-bit color or alpha.
extern guint8 LUT_u8_from_float16[65535];
extern guint8 LUT_u8a_from_float16[65535];

guint8 display_u8_from_float(gfloat code);
guint8 display_u8_alpha_from_float(gfloat code);
guint8 display_u8_from_u8(guint8 code);
guint8 display_u8_from_float16(guint16 code);
guint8 display_u8_alpha_from_float16(guint16 code);

#endif

/*
 *	fromrad.c -
 *		convert a Radiance floating point image to Iris format.
 *
 *			Greg Ward - 1992
 */




/* The following was taken from color.h in ray/src/common of Radiance 2.2 */


#ifndef NULL
#define NULL 0
#endif

#define  RED		0
#define  GRN		1
#define  BLU		2
#define  ALPHA		3
#define  EXP		3
#define  COLXS		128	/* excess used for exponent */

#define  MAXGSHIFT      31
#define  MINELEN        8
#define  MAXELEN 0x7fff 
#define  MINRUN          4    

static  unsigned char  (*g_bval)[256] = NULL;

typedef unsigned char  COLR[4];		/* red, green, blue, exponent */

#define  copycolr(c1,c2)	(c1[0]=c2[0],c1[1]=c2[1], \
				c1[2]=c2[2],c1[3]=c2[3])

typedef float  COLOR[3];	/* red, green, blue */

#define  colval(col,pri)	((col)[pri])
#define  setcolor(col,r,g,b)	((col)[RED]=(r),(col)[GRN]=(g),(col)[BLU]=(b))
#define  copycolor(c1,c2)	((c1)[0]=(c2)[0],(c1)[1]=(c2)[1],(c1)[2]=(c2)[2])
#define  scalecolor(col,sf)	((col)[0]*=(sf),(col)[1]*=(sf),(col)[2]*=(sf))
#define  addcolor(c1,c2)	((c1)[0]+=(c2)[0],(c1)[1]+=(c2)[1],(c1)[2]+=(c2)[2])
#define  multcolor(c1,c2)	((c1)[0]*=(c2)[0],(c1)[1]*=(c2)[1],(c1)[2]*=(c2)[2])
#define  COLRFMT		"32-bit_rle_rgbe"

#define  bmalloc      malloc
extern char	*tempbuffer(unsigned int len);
extern int	fwritecolrs(COLR *scanline, int len, FILE *fp);
extern int	freadcolrs(COLR *scanline, int len, FILE *fp);
extern int	fwritescan_a(float *scanline, int len, FILE *fp, FILE *afp);
extern int	freadscan(COLOR *scanline, int len, FILE *fp);
extern void	setcolr(COLR clr, double r, double g, double b);
extern void	colr_color(COLOR col, COLR clr);
extern int	bigdiff(COLOR c1, COLOR c2, double md);

/* End of color.h defs */

/* The following was taken from resolu.h in ray/src/common of Radiance 2.2 */

/*
 * Definitions for resolution line in image file.
 *
 * True image orientation is defined by an xy coordinate system
 * whose origin is at the lower left corner of the image, with
 * x increasing to the right and y increasing in the upward direction.
 * This true orientation is independent of how the pixels are actually
 * ordered in the file, which is indicated by the resolution line.
 * This line is of the form "{+-}{XY} xyres {+-}{YX} yxres\n".
 * A typical line for a 1024x600 image might be "-Y 600 +X 1024\n",
 * indicating that the scanlines are in English text order (PIXSTANDARD).
 */

			/* flags for scanline ordering */
#define  XDECR			1
#define  YDECR			2
#define  YMAJOR			4

			/* standard scanline ordering */
#define  PIXSTANDARD		(YMAJOR|YDECR)
#define  PIXSTDFMT		"-Y %d +X %d\n"

			/* structure for image dimensions */
typedef struct {
	int	or;		/* orientation (from flags above) */
	int	xr, yr;		/* x and y resolution */
} RESOLU;

			/* macros to get scanline length and number */
#define  scanlen(rs)		((rs)->or & YMAJOR ? (rs)->xr : (rs)->yr)
#define  numscans(rs)		((rs)->or & YMAJOR ? (rs)->yr : (rs)->xr)

			/* resolution string buffer and its size */
#define  RESOLU_BUFLEN		32
extern char  resolu_buf[RESOLU_BUFLEN];

			/* macros for reading/writing resolution struct */
#define  fputsresolu(rs,fp)	fputs(resolu2str(resolu_buf,rs),fp)
#define  fgetsresolu(rs,fp)	str2resolu(rs, \
					fgets(resolu_buf,RESOLU_BUFLEN,fp))

			/* reading/writing of standard ordering */
#define  fprtresolu(sl,ns,fp)	fprintf(fp,PIXSTDFMT,ns,sl)
#define  fscnresolu(sl,ns,fp)	(fscanf(fp,PIXSTDFMT,ns,sl)==2)

extern char  *fgets(), *resolu2str();

/* End of resolu.h defs */

extern int expadj;

#define MAXWIDTH	8192

//COLR cbuf[MAXWIDTH];
//unsigned short rbuf[MAXWIDTH];
//unsigned short gbuf[MAXWIDTH];
//unsigned short bbuf[MAXWIDTH];


/* End of colrops.c includes */

/* The following was imported from header.c in ray/src/common of Radiance 2.2 */

/*
 *  header.c - routines for reading and writing information headers.
 *
 *	8/19/88
 *
 *  isformat(s)		returns true if s is of the form "FORMAT=*"
 *  formatval(r,s)	copy the format value in s to r
 *  getheader(fp,f,p)	read header from fp, calling f(s,p) on each line
 *  checkheader(i,p,o)	check header format from i against p and copy to o
 *
 *  To copy header from input to output, use getheader(fin, fputs, fout)
 */

#include  <ctype.h>

#define	 MAXLINE	512

#if 0
#ifndef BSD
#define	 index	strchr
#endif
#endif

extern char  *index();

extern char  FMTSTR[];
#define FMTSTRL	7

struct check {
        FILE    *fp;
        char    fs[64];
};


/*
 * Copymatch(pat,str) checks pat for wildcards, and
 * copies str into pat if there is a match (returning true).
 */

#define copymatch(pat, s)	(!strcmp(pat, s))

/*
 * Checkheader(fin,fmt,fout) returns a value of 1 if the input format
 * matches the specification in fmt, 0 if no input format was found,
 * and -1 if the input format does not match or there is an
 * error reading the header.  If fmt is empty, then -1 is returned
 * if any input format is found (or there is an error), and 0 otherwise.
 * If fmt contains any '*' or '?' characters, then checkheader
 * does wildcard expansion and copies a matching result into fmt.
 * Be sure that fmt is big enough to hold the match in such cases!
 * The input header (minus any format lines) is copied to fout
 * if fout is not NULL.
 */


/* End of header.c includes */

/* The following was imported from resolu.c in ray/src/common of Radiance 2.2 */

/*
 * Read and write image resolutions.
 */

char  resolu_buf[RESOLU_BUFLEN];	/* resolution line buffer */

//----------------------------------------------

void unswizzleColr(register unsigned char *cptr,register unsigned short *rptr,register unsigned short *gptr,register unsigned short *bptr,register int n);

int oldreadcolrs(register COLR  *scanline, int len, register FILE  *fp);		
int checkheader(FILE  *fin, char  *fmt, FILE  *fout);
int fgetresolu(int *sl, int  *ns, FILE  *fp);
int freadcolrs(register COLR  *scanline, int  len, register FILE  *fp);
int setcolrcor(double a2);
int setcolrgam(double g);
int colrs_gambs(register COLR *scan, int len);
int str2resolu(register RESOLU *rp, char  *buf);
void shiftcolrs(register COLR* scan, register int len, register int adjust);
void normcolrs(register COLR* scan, int len, int adjust);
void formatval(register char* r, register char*  s);



/* End of header.c includes */

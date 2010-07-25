/*
 *	fromrad.c -
 *		convert a Radiance floating point image to Iris format.
 *
 *			Greg Ward - 1992
 */

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#ifdef WIN32

#else
#include <sys/fcntl.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "fromrad.h"

int expadj = 0;
char  FMTSTR[] = "FORMAT=";


#if 0
void ignoremain(int argc,char **argv)
{
    IMAGE *image;
    FILE *inf;
    int xsize, ysize;
    int y;
    int  dogamma = 0;

    if(argc<3) {
        fprintf(stderr,"usage: fromrad [-g gamma][-e +/-stops] radimage irisimage\n");
        exit(1);
    }
    while (argc > 4)
	    if (!strcmp(argv[1], "-g")) {
            setcolrgam(atof(argv[2]));
            dogamma++;
            argv += 2; argc -= 2;
	    } else if (!strcmp(argv[1], "-e")) {
            expadj = atoi(argv[2]);
            argv += 2; argc -= 2;
	    }
    inf = fopen(argv[1],"rb");
    if(!inf) {
        fprintf(stderr,"fromrad: can't open %s\n",argv[1]);
        exit(1);
    }
    if (checkheader(inf,COLRFMT,(FILE *)NULL) < 0 ||
		fgetresolu(&xsize, &ysize, inf) < 0) {
        fprintf(stderr,"fromrad: input not in Radiance format\n");
        exit(1);
    }
    image = iopen(argv[2],"w",RLE(1),3,xsize,ysize,3);
    for(y=0; y<ysize; y++) {
        freadcolrs(cbuf,xsize,inf);
        if (dogamma) {
            if (expadj)
            shiftcolrs(cbuf,xsize,expadj);
            colrs_gambs(cbuf,xsize);
        } else
            normcolrs(cbuf,xsize,expadj);
        unswizzleColr((unsigned char*)cbuf,rbuf,gbuf,bbuf,xsize);
        putrow(image,rbuf,(ysize-1)-y,0);
        putrow(image,gbuf,(ysize-1)-y,1);
        putrow(image,bbuf,(ysize-1)-y,2);
    }
    iclose(image);

    exit(0);
}
#endif

void unswizzleColr(register unsigned char *cptr,register unsigned short *rptr,register unsigned short *gptr,register unsigned short *bptr,register int n)
{
    while(n--) {
        *rptr++ = *cptr++;
        *gptr++ = *cptr++;
        *bptr++ = *cptr++;
        cptr++; // Skip the alpha channel
    }
}

//********************* IMPORTED CODE **************************?

// The following was imported from color.c in ray/src/common of Radiance 2.2

char *
tempbuffer(len)			/* get a temporary buffer */
unsigned int  len;
{
	static char  *tempbuf = NULL;
	static unsigned  tempbuflen = 0;

	if (len > tempbuflen) {
		if (tempbuflen > 0)
			tempbuf = (char *)realloc((void *)tempbuf, len);
		else
			tempbuf = (char *)malloc(len);
		tempbuflen = tempbuf==NULL ? 0 : len;
	}
	return(tempbuf);
}

char *
tempbuffer2(len)			/* get a temporary buffer */
unsigned int  len;
{
	static char  *tempbuf = NULL;
	static unsigned  tempbuflen = 0;

	if (len > tempbuflen) {
		if (tempbuflen > 0)
			tempbuf = (char *)realloc((void *)tempbuf, len);
		else
			tempbuf = (char *)malloc(len);
		tempbuflen = tempbuf==NULL ? 0 : len;
	}
	return(tempbuf);
}

int
fwritecolrs(scanline, len, fp)		/* write out a colr scanline */
register COLR  *scanline;
int  len;
register FILE  *fp;
{
	register int  i, j, beg, cnt = 1;
	int  c2;
	
	if ((len < MINELEN) | (len > MAXELEN))	/* OOBs, write out flat */
		return(fwrite((char *)scanline,sizeof(COLR),len,fp) - len);
					/* put magic header */
	putc(2, fp);
	putc(2, fp);
	putc(len>>8, fp);
	putc(len&255, fp);
					/* put components seperately */
	for (i = 0; i < 4; i++) {
	    for (j = 0; j < len; j += cnt) {	/* find next run */
		for (beg = j; beg < len; beg += cnt) {
		    for (cnt = 1; cnt < 127 && beg+cnt < len &&
			    scanline[beg+cnt][i] == scanline[beg][i]; cnt++)
			;
		    if (cnt >= MINRUN)
			break;			/* long enough */
		}
		if (beg-j > 1 && beg-j < MINRUN) {
		    c2 = j+1;
		    while (scanline[c2++][i] == scanline[j][i])
			if (c2 == beg) {	/* short run */
			    putc(128+beg-j, fp);
			    putc(scanline[j][i], fp);
			    j = beg;
			    break;
			}
		}
		while (j < beg) {		/* write out non-run */
		    if ((c2 = beg-j) > 128) c2 = 128;
		    putc(c2, fp);
		    while (c2--)
			putc(scanline[j++][i], fp);
		}
		if (cnt >= MINRUN) {		/* write out run */
		    putc(128+cnt, fp);
		    putc(scanline[beg][i], fp);
		} else
		    cnt = 0;
	    }
	}
	return(ferror(fp) ? -1 : 0);
}


int
oldreadcolrs(scanline, len, fp)		/* read in an old colr scanline */
register COLR  *scanline;
int  len;
register FILE  *fp;
{
	int  rshift;
	register int  i;
	
	rshift = 0;
	
	while (len > 0) {
		scanline[0][RED] = getc(fp);
		scanline[0][GRN] = getc(fp);
		scanline[0][BLU] = getc(fp);
		scanline[0][EXP] = getc(fp);
		if (feof(fp) || ferror(fp))
			return(-1);
		if (scanline[0][RED] == 1 &&
				scanline[0][GRN] == 1 &&
				scanline[0][BLU] == 1) {
			for (i = scanline[0][EXP] << rshift; i > 0; i--) {
				copycolr(scanline[0], scanline[-1]);
				scanline++;
				len--;
			}
			rshift += 8;
		} else {
			scanline++;
			len--;
			rshift = 0;
		}
	}
	return(0);
}


int
freadcolrs(scanline, len, fp)		/* read in an encoded colr scanline */
register COLR  *scanline;
int  len;
register FILE  *fp;
{
	register int  i, j;
	int  code, val;
					/* determine scanline type */
	if ((len < MINELEN) | (len > MAXELEN))
		return(oldreadcolrs(scanline, len, fp));
	if ((i = getc(fp)) == EOF)
		return(-1);
	if (i != 2) {
		ungetc(i, fp);
		return(oldreadcolrs(scanline, len, fp));
	}
	scanline[0][GRN] = getc(fp);
	scanline[0][BLU] = getc(fp);
	if ((i = getc(fp)) == EOF)
		return(-1);
	if (scanline[0][GRN] != 2 || scanline[0][BLU] & 128) {
		scanline[0][RED] = 2;
		scanline[0][EXP] = i;
		return(oldreadcolrs(scanline+1, len-1, fp));
	}
	if ((scanline[0][BLU]<<8 | i) != len)
		return(-1);		/* length mismatch! */
					/* read each component */
	for (i = 0; i < 4; i++)
	    for (j = 0; j < len; ) {
		if ((code = getc(fp)) == EOF)
		    return(-1);
		if (code > 128) {	/* run */
		    code &= 127;
		    if ((val = getc(fp)) == EOF)
			return -1;
		    while (code--)
			scanline[j++][i] = val;
		} else			/* non-run */
		    while (code--) {
			if ((val = getc(fp)) == EOF)
			    return -1;
			scanline[j++][i] = val;
		    }
	    }
	return(0);
}


int
fwritescan_a(scanline, len, fp, afp)		/* write out a scanline */
float *scanline;
int  len;
FILE  *fp;
FILE  *afp;
{
	COLR  *clrscan;
        COLR  *alphascan;
	int  n;

	register COLR  *sp;
	register COLR  *asp;
					/* get scanline buffer */
	if ((sp = (COLR *)tempbuffer(len*sizeof(COLR))) == NULL)
		return(-1);
	clrscan = sp;

	if (afp!=NULL) {
	  if ((asp = (COLR *)tempbuffer2(len*sizeof(COLR))) == NULL)
	    return(-1);
	  alphascan = asp; 
	}


					/* convert scanline */
	n = len;
        
        if (afp==NULL) {
	    while (n-- > 0) {
		    setcolr(sp[0], scanline[RED],
				      scanline[GRN],
				      scanline[BLU]);
		    scanline+=3;
		    sp++;
	    }
	    return(fwritecolrs(clrscan, len, fp));
    
        } else {
    
	    while (n-- > 0) {
		    setcolr(sp[0],  scanline[RED],
				    scanline[GRN],
				    scanline[BLU]);
		    sp++;
	    
		    setcolr(asp[0], scanline[ALPHA],
				    scanline[ALPHA],
				    scanline[ALPHA]);
		    scanline+=4;
		    asp++;
	    }
	    if (fwritecolrs(clrscan, len, fp)!=0) return (-1);
	    return(fwritecolrs(alphascan, len, afp));
        }
}


int
freadscan(scanline, len, fp)		/* read in a scanline */
register COLOR  *scanline;
int  len;
FILE  *fp;
{
	register COLR  *clrscan;

	if ((clrscan = (COLR *)tempbuffer(len*sizeof(COLR))) == NULL)
		return(-1);
	if (freadcolrs(clrscan, len, fp) < 0)
		return(-1);
					/* convert scanline */
	colr_color(scanline[0], clrscan[0]);
	while (--len > 0) {
		scanline++; clrscan++;
		if (clrscan[0][RED] == clrscan[-1][RED] &&
			    clrscan[0][GRN] == clrscan[-1][GRN] &&
			    clrscan[0][BLU] == clrscan[-1][BLU] &&
			    clrscan[0][EXP] == clrscan[-1][EXP])
			copycolor(scanline[0], scanline[-1]);
		else
			colr_color(scanline[0], clrscan[0]);
	}
	return(0);
}


void
setcolr(clr, r, g, b)		/* assign a short color value */
register COLR  clr;
double  r, g, b;
{
	double  d;
	int  e;
	
	d = r > g ? r : g;
	if (b > d) d = b;

	if (d <= 1e-32) {
		clr[RED] = clr[GRN] = clr[BLU] = 0;
		clr[EXP] = 0;
		return;
	}

	d = frexp(d, &e) * 256 / d;

	if (r > 0.0)
		clr[RED] = r * d;
	else
		clr[RED] = 0;
	if (g > 0.0)
		clr[GRN] = g * d;
	else
		clr[GRN] = 0;
	if (b > 0.0)
		clr[BLU] = b * d;
	else
		clr[BLU] = 0;

	clr[EXP] = e + COLXS;
}


void
colr_color(col, clr)		/* convert short to float color */
register COLOR  col;
register COLR  clr;
{
	double  f;
	
	if (clr[EXP] == 0)
		col[RED] = col[GRN] = col[BLU] = 0.0;
	else {
		f = ldexp(1.0, (int)clr[EXP]-(COLXS+8));
		col[RED] = (clr[RED] + 0.5)*f;
		col[GRN] = (clr[GRN] + 0.5)*f;
		col[BLU] = (clr[BLU] + 0.5)*f;
	}
}


int
bigdiff(c1, c2, md)			/* c1 delta c2 > md? */
register COLOR  c1, c2;
double  md;
{
	register int  i;

	for (i = 0; i < 3; i++)
		if (colval(c1,i)-colval(c2,i) > md*colval(c2,i) ||
			colval(c2,i)-colval(c1,i) > md*colval(c1,i))
			return(1);
	return(0);
}


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



void
formatval(register char* r, register char*  s) /* return format value */
{
	s += FMTSTRL;
	while (isspace(*s)) s++;
	if (!*s) { *r = '\0'; return; }
	while(*s) *r++ = *s++;
	while (isspace(r[-1])) r--;
	*r = '\0';
}


int
getheader(FILE* fp, void (*f)(char *, struct check *), struct check* p)		/* get header from file */
{
	char  buf[MAXLINE];

	for ( ; ; ) {
		buf[MAXLINE-2] = '\n';
		if (fgets(buf, MAXLINE, fp) == NULL)
			return(-1);
		if (buf[0] == '\n')
			return(0);
		if (buf[MAXLINE-2] != '\n') {
			ungetc(buf[MAXLINE-2], fp);	/* prevent false end */
			buf[MAXLINE-2] = '\0';
		}
		if (f != NULL)
			(*f)(buf, p);
	}
}



static void
mycheck(char* s, register struct check* cp)			/* check a header line for format info. */
{
	if (!strncmp(s,FMTSTR,FMTSTRL))
		formatval(cp->fs, s);
	else if (cp->fp != NULL)	/* don't copy format info. */
		fputs(s, cp->fp);
}


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

int checkheader(FILE  *fin, char  *fmt, FILE  *fout)
{
	struct check	cdat;

	cdat.fp = fout;
	cdat.fs[0] = '\0';
	if (getheader(fin, mycheck, &cdat) < 0)
		return(-1);
	if (cdat.fs[0] != '\0')
		return(copymatch(fmt, cdat.fs) ? 1 : -1);
	return(0);
}

/* End of header.c includes */

/* The following was imported from resolu.c in ray/src/common of Radiance 2.2 */

/*
 * Read and write image resolutions.
 */


int
fgetresolu(int *sl, int  *ns, FILE  *fp)  /* get picture dimensions */
                                          /* scanline length and number */
{
	RESOLU  rs;

	if (!fgetsresolu(&rs, fp))
		return(-1);
	if (rs.or & YMAJOR) {
		*sl = rs.xr;
		*ns = rs.yr;
	} else {
		*sl = rs.yr;
		*ns = rs.xr;
	}
	return(rs.or);
}


int str2resolu(register RESOLU *rp, char  *buf)		/* convert resolution line to struct */
{
	register char  *xndx, *yndx;
	register char  *cp;

	if (buf == NULL)
		return(0);
	xndx = yndx = NULL;
	for (cp = buf; *cp; cp++)
		if (*cp == 'X')
			xndx = cp;
		else if (*cp == 'Y')
			yndx = cp;
	if (xndx == NULL || yndx == NULL)
		return(0);
	rp->or = 0;
	if (xndx > yndx) rp->or |= YMAJOR;
	if (xndx[-1] == '-') rp->or |= XDECR;
	if (yndx[-1] == '-') rp->or |= YDECR;
	if ((rp->xr = atoi(xndx+1)) <= 0)
		return(0);
	if ((rp->yr = atoi(yndx+1)) <= 0)
		return(0);
	return(1);
}

/* End of header.c includes */

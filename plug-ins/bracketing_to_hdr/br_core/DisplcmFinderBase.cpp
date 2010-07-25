/*
 * DisplcmFinderBase.cpp  --  Part of the CinePaint plug-in "Bracketing_to_HDR"
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
  DisplcmFinderBase.cpp
*/

#include "DisplcmFinderBase.hpp"
#include "DynArray1D.hpp"               // DynArray1D<>


#if 0
bool DisplcmFinderBase :: 
check_area_dims_      ( const Vec2_int & sizeA,
                        const Vec2_int & sizeB, 
                        int nx, int ny, int mx, int my )  const
{
    bool ok = false;
    if (2*nx + 1 > sizeA.x)
      printf("PARDON: '2*nx + 1' groesser x-Dim von A.\n");
    else
    if (2*ny + 1 > sizeA.y)
      printf("PARDON: '2*ny + 1' groesser y-Dim von A.\n");
    else
    if (2*(nx+mx) + 1 > sizeB.x)
      printf("PARDON: '2*(nx+mx)+1' groesser x-Dim von B.\n");
    else 
    if (2*(ny+my) + 1 > sizeB.y)
      printf("PARDON: '2*(ny+my)+1' groesser y-Dim von B.\n");
    else
      ok = true;  
    
    return ok;
}

bool DisplcmFinderBase :: 
check_center_coords_  ( const Vec2_int & sizeA,
                        const Vec2_int & sizeB, 
                        int n_center, 
                        const Vec2_int* centerA,
                        const Vec2_int* centerB,
                        int nx, int ny, int mx, int my )  const
{
    bool ok = true;
    for (int i=0; i < n_center; i++) {
      printf("Center #%d: (x,y) = %d, %d\n", i, centerB[i].x, centerB[i].y);
      
      if (centerA[i].x < nx) {
        printf("\txcA=%d zu klein!\n", centerA[i].x); ok=0;}
      if (centerA[i].y < ny) {
        printf("\tycA=%d zu klein!\n", centerA[i].y); ok=0;}
      if (centerA[i].x + nx >= sizeA.x) {
        printf("\txcA=%d zu weit!\n", centerA[i].x); ok=0;}
      if (centerA[i].y + ny >= sizeA.y) {
        printf("\tycA=%d zu weit!\n", centerA[i].y); ok=0;}
      
      if (centerB[i].x < nx+mx) {
        printf("\txcB=%d zu klein!\n", centerB[i].x); ok=0;}
      if (centerB[i].y < ny+my) {
        printf("\tycB=%d zu klein!\n", centerB[i].y); ok=0;}
      if (centerB[i].x + mx+nx >= sizeB.x) {
        printf("\txcB=%d zu weit!\n", centerB[i].x); ok=0;}
      if (centerB[i].y + my+ny >= sizeB.y) {
        printf("\tycB=%d zu weit!\n", centerB[i].y); ok=0;}
    }
    return ok;
}
#endif

/*!
  Verteilt \a num zentralsymmetrische Strecken (Suchgebiete) der "halben" (Pixel)Laenge
   \a hw gleichmaessig auf einer Breite \a width (Bilddimension) und liefert im Feld
   \a cntr die Mittelpunktskoordinaten der Strecken zurueck. Es wird erwartet, dass
   das Feld \a cntr Platz fuer \a num Integer bietet. Hinweis: Die Gesamtpixelzahl
   einer zentralsymmetr. Strecke ergibt sich aus der "halben" Laenge gemaess \a 2*hw+1.
   
  FRAGE: Sollte kenntlich gemacht werden z.B. durch einen Rueckgabewert, wenn
   Suchgebiete exakt uebereinander liegen? Oder sowas besser in check_center_coords_()?
*/
void DisplcmFinderBase :: 
justify_1d_centers ( int width, int hw, int num, int* cntr )
{
    int W = 2*hw + 1;
    if (W > width || num <= 0) return;
    /*  Gebiete so verteilen, dass ihr Abstand `space' (auch zum Rand) gleich */
    float space = (width - num * W) / (float)(num+1); 
    //printf("space= %f\n", space);
    /*  Der gebrochene Rest von space wird durch Akkumul. gleichmaessig verteilt!*/
    if (space >= 0.0f) {
      for (int i=0; i < num; i++)
        cntr[i] = (int)((i+1)*space) + hw + i*W;
    }else { 
      /*  Links und recht auf Bildrand (num>1 fuer space<0 garantiert) */
      cntr[0] = hw;                 // Gebietsrand = linker Bildrand
      cntr[num-1] = width-1 - hw;   // Gebietsrand = rechter Bildrand
      space = (width - W) / (float)(num-1); // space der verbleibenden
      for (int i=1; i < num-1; i++)
        cntr[i] = hw + (int)(i*space);
      /*  Oder so: Der Rest stellt ein (num-2)-Problem dar, die (num-1) Koordinaten
           sind zu schreiben ab cntr[1], da cntr[0] (wie cntr[num-1]) schon fertig */
      //justify_1d_centers ( width, hw, num-2, &cntr[1] );  // rekursiv 
    }
}

/*!
  Verteilt \a num (Suchgebiete) der (Pixel)Laenge \a W gleichmaessig auf einer Breite
   \a width (Bilddimension) und liefert im Feld \a orig die LO-Koordinaten (Anfangspunkte)
   der Strecken zurueck. Es wird erwartet, dass das Feld \a orig Platz fuer \a num Integer
   bietet.
*/
void DisplcmFinderBase :: 
justify_1d_origins ( int width, int W, int num, int* orig )
{
    if (W > width || num <= 0) return;
    /*  Gebiete so verteilen, dass ihr Abstand `space' (auch zum Rand) gleich */
    float space = (width - num * W) / (float)(num+1);
    //printf("space= %f\n", space);
    /*  Der gebrochene Rest von space wird durch Akkumul. gleichmaessig verteilt!*/
    if (space >= 0.0f) {
      for (int i=0; i < num; i++)
        orig[i] = (int)((i+1)*space) + i*W;
    }else { 
      /*  Links und rechts auf Bildrand (num>1 fuer space<0 garantiert) */
      orig[0] = 0;                  // Gebietsrand = linker Bildrand
      orig[num-1] = width - W;      // Gebietsrand = rechter Bildrand
      space = (width - W) / (float)(num-1); // space der verbleibenden
      for (int i=1; i < num-1; i++)
        orig[i] = (int)(i*space);
      /*  Oder so: Der Rest stellt ein (num-2)-Problem dar, die (num-1) Koordinaten
           sind zu schreiben ab orig[1], da orig[0] (wie orig[num-1]) schon fertig */
      //justify_1d_origins ( width, W, num-2, &orig[1] );  // rekursiv
    }
}


/*!
  Bestimmt fuer ein "P x Q"-Muster von Suchgebieten (d.h. \a P in x-Richtung und 
   \a Q in y-Richtung) deren Mittelpunktskoordinaten so, dass sie gleichabstaendig verteilt
   sind. Die zentralsymmetrischen Suchgebiete werden beschrieben durch ihre "halben" 
   Dimensionen \a hwx und \a hwy (== nx+mx bzw. ny+my). Die Mittelpunktskoordinaten
   werden zurueckgeliefert in den Feldern \a centerA und \a centerB, von denen angenommen
   wird, dass sie Platz fuer \a P*Q Vec2_int Elemente bieten.
  
  Indizierung der (Such)Gebiete: <pre>
      0   1   2 ... P-1
      P  P+1 ..... 2P-1
      .................
   (Q-1)P ........ QP-1 </pre>
*/
void DisplcmFinderBase :: 
justify_centers_for_PxQ ( const Vec2_int & sizeB, 
                          int hwx, int hwy, int P, int Q,
                          Vec2_int* centerB )
{
    DynArray1D <int>  xcntr(P);  // ISO C++ forbids variable-size arrays
    DynArray1D <int>  ycntr(Q);  // dito
    
    justify_1d_centers (sizeB.x,  hwx, P, xcntr);  // anhand von B
    justify_1d_centers (sizeB.y, hwy, Q, ycntr);
    
    /*  Jeder P-te x-Wert ist identisch */
    for (int i=0; i < P; i++)
      for (int j=0; j < Q; j++)
        centerB[i + j*P].x = xcntr[i];
        
    /*  Je P aufeinanderfolgende y-Werte sind identisch */
    for (int i=0; i < Q; i++)
      for (int j=0; j < P; j++)
        centerB[i*P + j].y = ycntr[i];
#if 0
    /*  Debug output */
    printf("xcntr: "); for (int i=0; i<P; i++) printf(" %d", xcntr[i]); printf("\n");
    printf("ycntr: "); for (int i=0; i<Q; i++) printf(" %d", ycntr[i]); printf("\n");
    for (int i=0; i < Q; i++) {
      for (int j=0; j < P; j++) 
        printf(" (%3d,%3d)", centerB[i*P+j].x, centerB[i*P+j].y);
      printf("\n");
    }
#endif    
}

/*!
  Bestimmt fuer ein "P x Q"-Muster von Suchgebieten (d.h. \a P in x-Richtung und 
   \a Q in y-Richtung) deren LO-Koordinaten so, dass sie gleichabstaendig verteilt
   sind. Die Suchgebiete werden beschrieben durch ihre Dimensionen \a wx und \a wy 
   (== Mx bzw. My). Die LO-Koordinaten werden zurueckgeliefert in den Feldern \a origA
   und \a origB, von denen angenommen wird, dass sie Platz fuer \a P*Q Vec2_int Elemente
   bieten.
  
  Indizierung der (Such)Gebiete: <pre>
      0   1   2 ... P-1
      P  P+1 ..... 2P-1
      .................
   (Q-1)P ........ QP-1 </pre>
*/
void DisplcmFinderBase :: 
justify_origins_for_PxQ ( const Vec2_int & sizeB, 
                          int wx, int wy, int P, int Q,
                          Vec2_int* origB )
{
    DynArray1D <int>  xorig(P);  // ISO C++ forbids variable-size arrays
    DynArray1D <int>  yorig(Q);  // dito
    
    justify_1d_origins (sizeB.x, wx, P, xorig);  // anhand von B
    justify_1d_origins (sizeB.y, wy, Q, yorig);
    
    /*  Jeder P-te x-Wert ist identisch */
    for (int i=0; i < P; i++)
      for (int j=0; j < Q; j++)
        origB[i + j*P].x = xorig[i];
        
    /*  Je P aufeinanderfolgende y-Werte sind identisch */
    for (int i=0; i < Q; i++)
      for (int j=0; j < P; j++)
        origB[i*P + j].y = yorig[i];
#if 0
    /*  Debug output */
    printf("xLO: "); for (int i=0; i<P; i++) printf(" %d", xorig[i]); printf("\n");
    printf("yLO: "); for (int i=0; i<Q; i++) printf(" %d", yorig[i]); printf("\n");
    for (int i=0; i < Q; i++) {
      for (int j=0; j < P; j++) 
        printf(" (%3d,%3d)", origB[i*P+j].x, origB[i*P+j].y);
      printf("\n");
    }
#endif    
}

// END OF FILE

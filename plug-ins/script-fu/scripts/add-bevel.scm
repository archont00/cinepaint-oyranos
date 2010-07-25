; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; add-bevel.scm version 1.03
; Time-stamp: <1997-12-16 09:17:24 ard>
; 
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
; 
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
; 
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
;
; Copyright (C) 1997 Andrew Donkin  (ard@cs.waikato.ac.nz)
; Contains code from add-shadow.scm by Sven Neumann
; (neumanns@uni-duesseldorf.de) (thanks Sven).
;  
; Adds a bevel to an image.  See http://www.cs.waikato.ac.nz/~ard/gimp/
;
; If there is a selection, it is bevelled.
; Otherwise if there is an alpha channel, the selection is taken from it
; and bevelled.
; Otherwise the whole image is bevelled.
;
; The selection is set on exit, so Select->Invert then Edit->Clear will
; leave a cut-out.  Then use Sven's add-shadow for that
; floating-bumpmapped-texture cliche.

;
; 1.01: now works on offset layers.
; 1.02: has crop-pixel-border option to trim one pixel off each edge of the
;       bevelled image.  Bumpmapping leaves edge pixels unchanged, which
;       looks bad.  Oddly, this is not apparant in the GIMP - you have to
;       save the image and load it into another viewer.  First noticed in
;       Nutscrape.
;       Changed path (removed "filters/").
; 1.03: adds one-pixel border before bumpmapping, and removes it after.
;	Got rid of the crop-pixel-border option (no longer reqd).
;

;
; BUMPMAP NOTES:
;
; Bumpmap changed arguments from version 2.02 to 2.03.  If you use the
; wrong bumpmap.c this script will either bomb (good) or produce a
; naff image (bad).
;
; As distributed this script expects bumpmap 2.03 (shipped with Gimp 0.99.11)
; or later.

;
; BUGS
;
; Doesn't allow undoing the operation.  Why not?
;
; Sometimes (and that's the scary bit) gives bogloads of GTK warnings.
;

  
(define (script-fu-add-bevel img 
			     drawable 
			     thickness
			     work-on-copy
			     keep-bump-layer)

  (let* ((index 0)
	 (bevelling-whole-image FALSE)
	 (greyness 0)
	 (thickness (abs thickness))
	 (type (car (gimp-drawable-type-with-alpha drawable)))
	 (image (if (= work-on-copy TRUE) (car (gimp-channel-ops-duplicate img)) img))
	 (pic-layer (car (gimp-image-active-drawable image)))
	 (width (car (gimp-drawable-width pic-layer)))
	 (height (car (gimp-drawable-height pic-layer)))
	 (old-bg (car (gimp-palette-get-background)))
	 (bump-layer (car (gimp-layer-new image 
					  width 
					  height 
					  GRAY
					  "Bumpmap" 
					  100
					  NORMAL)))
	 )
    ;
    ; If the layer we're bevelling is offset from the image's origin, we
    ; have to do the same to the bumpmap
    (let ((offsets (gimp-drawable-offsets pic-layer)))
      (gimp-layer-set-offsets bump-layer (car offsets) (cadr offsets))
      )

    (gimp-image-disable-undo image)

    ;------------------------------------------------------------
    ;
    ; Set the selection to the area we want to bevel.
    ;
    (if (eq? 0 (car (gimp-selection-bounds image)))
	(begin
	  (set! bevelling-whole-image TRUE) ; ...so we can restore things properly, and crop.
	  (gimp-image-resize image (+ width 2) (+ height 2) 1 1)
	  (if (not (eq? 0 (car (gimp-drawable-has-alpha pic-layer))))	; Wish I knew Scheme
	      (gimp-selection-layer-alpha image pic-layer)
	      (begin
		(gimp-selection-all image)
		)
	      )
	  )
	)

    ; Store it for later.
    (set! select (car (gimp-selection-save image)))
    ; Try to lose the jaggies
    (gimp-selection-feather image 2)

    ;------------------------------------------------------------
    ;
    ; Initialise our bumpmap
    ;
    (gimp-palette-set-background '(0 0 0))
    (gimp-drawable-fill bump-layer BG-IMAGE-FILL)

    (set! index 1)
    (while (< index thickness)
	   (set! greyness (/ (* index 255) thickness))
	   (gimp-palette-set-background (list greyness greyness greyness))
	   ;(gimp-selection-feather image 1) ;Stop the slopey jaggies?
	   (gimp-bucket-fill image bump-layer BG-BUCKET-FILL NORMAL 100 0 FALSE 0 0)
	   (gimp-selection-shrink image 1)
	   (set! index (+ index 1))
	   )
    ; Now the white interior
    (gimp-palette-set-background '(255 255 255))
    (gimp-bucket-fill image bump-layer BG-BUCKET-FILL NORMAL 100 0 FALSE 0 0)

    ;------------------------------------------------------------
    ;
    ; Do the bump.
    ;
    (gimp-selection-none image)

    ; To further lessen jaggies?
    ;(plug-in-gauss-rle 1 image bump-layer thickness TRUE TRUE)


    ; If they're working on the original, let them undo the filter's effects.
    ; This doesn't work - ideas why not?
    (if (= work-on-copy FALSE) (gimp-image-enable-undo image))

    ;
    ; BUMPMAP INVOCATION:
    ;
    ; Use the former with version 2.02 or earlier of bumpmap:
    ; Use the latter with version 2.03 or later (ambient and waterlevel params)
;   (plug-in-bump-map 1 image pic-layer bump-layer 125 45 3 0 0     TRUE FALSE 1)
    (plug-in-bump-map 1 image pic-layer bump-layer 125 45 3 0 0 0 0 TRUE FALSE 1)

    ;
    ; Shave one pixel off each edge
    ;
    (if (= bevelling-whole-image TRUE)
	  (gimp-crop image width height 1 1)
	  )

    (if (= work-on-copy FALSE) (gimp-image-disable-undo image))

    ;------------------------------------------------------------
    ;
    ; Restore things
    ;
    (gimp-palette-set-background old-bg)
    (if (= bevelling-whole-image TRUE)
	(gimp-selection-none image)	; No selection to start with
	(gimp-selection-load image select)
	)
    ; If they started with a selection, they can Select->Invert then
    ; Edit->Clear for a cutout.

    ; clean up
    (gimp-image-remove-channel image select)
    (if (= keep-bump-layer TRUE)
	(begin
	  (gimp-image-add-layer image bump-layer 1)
	  (gimp-layer-set-visible bump-layer 0))
	(gimp-layer-delete bump-layer)
	)

    (gimp-image-set-active-layer image pic-layer)

    (if (= work-on-copy TRUE) (gimp-display-new image))

    (gimp-image-enable-undo image)
    (gimp-displays-flush)

    )
  )

(script-fu-register "script-fu-add-bevel"
		    "<Image>/Script-Fu/Decor/Add Bevel"
		    "Add a bevel to an image."
		    "Andrew Donkin (ard@cs.waikato.ac.nz)"
		    "Andrew Donkin"
		    "1997/11/06"
		    "RGB RGBA GRAY GRAYA"
		    SF-IMAGE "Image" 0
		    SF-DRAWABLE "Drawable" 0
		    SF-VALUE "Thickness" "5"
		    SF-TOGGLE "Work on copy" TRUE
		    SF-TOGGLE "Keep bump layer" FALSE
		    ) 

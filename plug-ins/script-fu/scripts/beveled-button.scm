; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
; 
; Button00 --- create a simple beveled Web button
; Copyright (C) 1997 Federico Mena Quintero
; federico@nuclecu.unam.mx
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


(define (text-width extents)
  (car extents))

(define (text-height extents)
  (cadr extents))

(define (text-ascent extents)
  (caddr extents))

(define (text-descent extents)
  (cadr (cddr extents)))

(define (blend-bumpmap img drawable x1 y1 x2 y2)
  (gimp-blend img
	      drawable
	      FG-BG-RGB
	      DARKEN-ONLY
	      LINEAR
	      100
	      0
	      REPEAT-NONE
	      FALSE
	      0
	      0
	      x1
	      y1
	      x2
	      y2))

(define (script-fu-button00 text
			    size
			    foundry
			    family
			    weight
			    slant
			    set-width
			    spacing
			    ul-color
			    lr-color
			    text-color
			    padding
			    bevel-width
			    pressed)
  (let* ((old-fg-color (car (gimp-palette-get-foreground)))
	 (old-bg-color (car (gimp-palette-get-background)))
	 
	 (text-extents (gimp-text-get-extents text
					      size
					      PIXELS
					      foundry
					      family
					      weight
					      slant
					      set-width
					      spacing))
	 (ascent (text-ascent text-extents))
	 (descent (text-descent text-extents))
	 
	 (img-width (+ (* 2 (+ padding bevel-width))
		       (- (text-width text-extents)
			  (text-width (gimp-text-get-extents " "
							     size
							     PIXELS
							     foundry
							     family
							     weight
							     slant
							     set-width
							     spacing)))))
	 (img-height (+ (* 2 (+ padding bevel-width))
			(+ ascent descent)))

	 (img (car (gimp-image-new img-width img-height RGB)))

	 (bumpmap (car (gimp-layer-new img img-width img-height RGBA_IMAGE "Bumpmap" 100 NORMAL)))
	 (gradient (car (gimp-layer-new img img-width img-height RGBA_IMAGE "Gradient" 100 NORMAL))))

    (gimp-image-disable-undo img)

    ; Create bumpmap layer
    
    (gimp-image-add-layer img bumpmap -1)
    (gimp-palette-set-foreground '(0 0 0))
    (gimp-palette-set-background '(255 255 255))
    (gimp-edit-fill img bumpmap)

    (gimp-rect-select img 0 0 bevel-width img-height REPLACE FALSE 0)
    (blend-bumpmap img bumpmap 0 0 (- bevel-width 1) 0)

    (gimp-rect-select img 0 0 img-width bevel-width REPLACE FALSE 0)
    (blend-bumpmap img bumpmap 0 0 0 (- bevel-width 1))

    (gimp-rect-select img (- img-width bevel-width) 0 bevel-width img-height REPLACE FALSE 0)
    (blend-bumpmap img bumpmap (- img-width 1) 0 (- img-width bevel-width) 0)

    (gimp-rect-select img 0 (- img-height bevel-width) img-width bevel-width REPLACE FALSE 0)
    (blend-bumpmap img bumpmap 0 (- img-height 1) 0 (- img-height bevel-width))

    (gimp-selection-none img)

    ; Create gradient layer

    (gimp-image-add-layer img gradient -1)
    (gimp-palette-set-foreground ul-color)
    (gimp-palette-set-background lr-color)
    (gimp-blend img
		gradient
		FG-BG-RGB
		NORMAL
		LINEAR
		100
		0
		REPEAT-NONE
		FALSE
		0
		0
		0
		0
		(- img-width 1)
		(- img-height 1))

    (plug-in-bump-map 1 img gradient bumpmap 135 45 bevel-width 0 0 0 0 TRUE pressed 0)

    ; Create text layer

    (gimp-palette-set-foreground text-color)
    (let ((textl (car (gimp-text
		       img -1 0 0 text 0 TRUE size PIXELS foundry family weight slant set-width spacing))))
      (gimp-layer-set-offsets textl
			      (+ bevel-width padding)
			      (+ bevel-width padding descent)))

    ; Done

    (gimp-selection-none img)
    (gimp-palette-set-foreground old-fg-color)
    (gimp-palette-set-background old-bg-color)
    (gimp-image-enable-undo img)
    (gimp-display-new img)))

; Register!

(script-fu-register "script-fu-button00"
		    "<Toolbox>/Xtns/Script-Fu/Buttons/Simple beveled button"
		    "Simple beveled button"
		    "Federico Mena Quintero"
		    "Federico Mena Quintero"
		    "June 1997"
		    ""
		    SF-VALUE "Text" "\"Hello world!\""
		    SF-VALUE "Size" "16"
		    SF-VALUE "Foundry" "\"adobe\""
		    SF-VALUE "Family" "\"helvetica\""
		    SF-VALUE "Weight" "\"bold\""
		    SF-VALUE "Slant" "\"r\""
		    SF-VALUE "Set width" "\"normal\""
		    SF-VALUE "Spacing" "\"p\""
		    SF-COLOR "Upper-left color" '(0 255 127)
		    SF-COLOR "Lower-right color" '(0 127 255)
		    SF-COLOR "Text color" '(0 0 0)
		    SF-VALUE "Padding" "2"
		    SF-VALUE "Bevel width" "4"
		    SF-VALUE "Pressed?" "FALSE")



; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
; 
; xach effect script
; Copyright (c) 1997 Adrian Likins
; aklikins@eos.ncsu.edu
;
; based on a idea by Xach Beane <xach@mint.net>
;
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


(define (script-fu-xach-effect image
			       drawable
			       hl-offset-x
			       hl-offset-y
			       hl-color
			       hl-opacity
			       ds-color
			       ds-opacity
			       ds-blur
			       ds-offset-x
			       ds-offset-y
			       keep-selection)
  (let* ((ds-blur (max ds-blur 0))
	 (ds-opacity (min ds-opacity 100))
	 (ds-opacity (max ds-opacity 0))
	 (type (car (gimp-drawable-type-with-alpha drawable)))
	 (image-width (car (gimp-image-width image)))
	 (image-height (car (gimp-image-height image)))
	 (old-bg (car (gimp-palette-get-background))))
    
    (gimp-image-disable-undo image)
    (gimp-layer-add-alpha drawable)
    
    (if (= (car (gimp-selection-is-empty image)) TRUE)
	(begin
	  (gimp-selection-layer-alpha image drawable)
	  (set! active-selection (car (gimp-selection-save image)))
	  (set! from-selection FALSE))
	(begin 
	  (set! from-selection TRUE)
	  (set! active-selection (car (gimp-selection-save image)))))
    
    (set! hl-layer (car (gimp-layer-new image image-width image-height type "Highlight" 100 NORMAL)))
    (gimp-image-add-layer image hl-layer -1)
    

    (gimp-selection-none image)
    (gimp-edit-clear image hl-layer)
    (gimp-selection-load image active-selection)
    
    (gimp-palette-set-background hl-color)
    (gimp-edit-fill image hl-layer)
    (gimp-selection-translate image hl-offset-x hl-offset-y)
    (gimp-edit-fill image hl-layer)
    (gimp-selection-none image)
    (gimp-selection-load image active-selection)
    
    (set! mask (car (gimp-layer-create-mask hl-layer WHITE-MASK)))
    (gimp-image-add-layer-mask image hl-layer mask)
    
    (gimp-palette-set-background hl-opacity)
    (gimp-edit-fill image mask)

    (set! shadow-layer (car (gimp-layer-new image
					    image-width
					    image-height
					    type
					    "Shadow"
					    ds-opacity
					    NORMAL)))
    (gimp-image-add-layer image shadow-layer -1)
    (gimp-selection-none image)
    (gimp-edit-clear image shadow-layer)
    (gimp-selection-load image active-selection)
    (gimp-selection-translate image ds-offset-x ds-offset-y)
    (gimp-palette-set-background ds-color)
    (gimp-edit-fill image shadow-layer)
    (gimp-selection-none image)
    (plug-in-gauss-rle 1 image shadow-layer ds-blur TRUE TRUE)
    (gimp-selection-load image active-selection)
    (gimp-edit-clear image shadow-layer)
    (gimp-image-lower-layer image shadow-layer)


    (gimp-palette-set-background old-bg)

    (if (= keep-selection FALSE)
	(gimp-selection-none image))

    (gimp-image-set-active-layer image drawable)
    (gimp-image-remove-channel image active-selection)
    (gimp-image-enable-undo image)
    (gimp-displays-flush)))

(script-fu-register "script-fu-xach-effect"
		    "<Image>/Script-Fu/Decor/Xach-Effect"
		    "Add a subtle translucent 3-d effect to the current selection or alpha channel"
		    "Adrian Likins <adrian@gimp.org>"
		    "Adrian Likins"
		    "9/28/97"
		    "RGB RGBA GRAY GRAYA"
		    SF-IMAGE "Image" 0
		    SF-DRAWABLE "Drawable" 0
		    SF-VALUE "highlight X offset" "-1"
		    SF-VALUE "highlight Y offset" "-1"
		    SF-COLOR "Highlight Color" '(255 255 255)
		    SF-COLOR "Opacity" '(66 66 66)
		    SF-COLOR "Drop Shadow Color" '(0 0 0)
		    SF-VALUE "Drop shadow Opacity" "100"
		    SF-VALUE "Drop shadow Blur Radius" "12"
		    SF-VALUE "Drop shadow X offset" "5"
                    SF-VALUE "Drop shadow Y offset" "5"
		    SF-TOGGLE "Keep Selection?" TRUE)




; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
; 
; Lava effect
; Copyright (c) 1997 Adrian Likins
; aklikins@eos.ncsu.edu
;
; based on a idea by Sven Riedel <lynx@heim8.tu-clausthal.de>
; tweaked a little by Sven Neumann <neumanns@uni-duesseldorf.de>
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


(define (script-fu-lava image
			drawable
			seed
			tile_size
			mask_size
			gradient
			keep-selection
			seperate-layer
			current-grad)
  (let* (
	 (type (car (gimp-drawable-type-with-alpha drawable)))
	 (image-width (car (gimp-image-width image)))
	 (image-height (car (gimp-image-height image)))
	 (old-gradient (car (gimp-gradients-get-active)))
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
    
    (set! selection-bounds (gimp-selection-bounds image))
    (set! select-offset-x (cadr selection-bounds))
    (set! select-offset-y (caddr selection-bounds))
    (set! select-width (- (cadr (cddr selection-bounds)) select-offset-x))
    (set! select-height (- (caddr (cddr selection-bounds)) select-offset-y))
    
    (if (= seperate-layer TRUE)
	(begin
	  (set! lava-layer (car (gimp-layer-new image 
						select-width 
						select-height 
						type 
						"Lava Layer" 
						100 
						NORMAL)))
	  
	  (gimp-layer-set-offsets lava-layer select-offset-x select-offset-y)
	  (gimp-image-add-layer image lava-layer -1)
	  (gimp-selection-none image)
	  (gimp-edit-clear image lava-layer)
	  
	  (gimp-selection-load image active-selection)
	  (gimp-image-set-active-layer image lava-layer))) 
    
    (set! active-layer (car (gimp-image-get-active-layer image)))
    
    (if (= current-grad FALSE)
	(gimp-gradients-set-active gradient))
    
    (plug-in-solid-noise 1 image active-layer FALSE TRUE seed 2 2 2)
    (plug-in-cubism 1 image active-layer tile_size 2.5 0)
    (plug-in-oilify 1 image active-layer mask_size)
    (plug-in-edge 1 image active-layer 2 0)
    (plug-in-gauss-rle 1 image active-layer 2 TRUE TRUE)
    (plug-in-gradmap 1 image active-layer)

    (gimp-gradients-set-active old-gradient)
    (gimp-palette-set-background old-bg)

    (if (= keep-selection FALSE)
	(gimp-selection-none image))
   
    (gimp-image-set-active-layer image drawable)
    (gimp-image-remove-channel image active-selection)
    (gimp-image-enable-undo image)
    (gimp-displays-flush)))

(script-fu-register "script-fu-lava"
		    "<Image>/Script-Fu/Decor/Lava"
		    "Fills the current selection with lava."
		    "Adrian Likins <adrian@gimp.org>"
		    "Adrian Likins"
		    "10/12/97"
		    "RGB RGBA GRAY GRAYA"
		    SF-IMAGE "Image" 0
		    SF-DRAWABLE "Drawable" 0
		    SF-VALUE "Seed" "2"
		    SF-VALUE "Size" "10"
		    SF-VALUE "Roughness" "7"
		    SF-VALUE "Gradient" "\"German_flag_smooth\""
		    SF-TOGGLE "Keep Selection?" TRUE
		    SF-TOGGLE "Seperate Layer?" TRUE
		    SF-TOGGLE "Use current Gradient?" FALSE)







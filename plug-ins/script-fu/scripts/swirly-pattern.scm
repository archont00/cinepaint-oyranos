; The GIMP -- an image manipulation program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Pattern00 --- create a swirly tileable pattern
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


(define (script-fu-swirly-pattern qsize angle times)
  (define (whirl-it img drawable angle times)
    (if (> times 0)
	(begin
	  (plug-in-whirl-pinch 1 img drawable angle 0.0 1.0)
	  (whirl-it img drawable angle (- times 1)))))

  (let* ((hsize (* qsize 2))
	 (img-size (* qsize 4))
	 (img (car (gimp-image-new img-size img-size RGB)))
	 (drawable (car (gimp-layer-new img img-size img-size RGB "Swirly pattern" 100 NORMAL)))

	 ; Save old foregound and background colors

	 (old-fg-color (car (gimp-palette-get-foreground)))
	 (old-bg-color (car (gimp-palette-get-background))))

    (gimp-image-disable-undo img)
    (gimp-image-add-layer img drawable 0)

    ; Render checkerboard

    (gimp-palette-set-foreground '(0 0 0))
    (gimp-palette-set-background '(255 255 255))

    (plug-in-checkerboard 1 img drawable 0 qsize)

    ; Whirl upper left

    (gimp-rect-select img 0 0 hsize hsize REPLACE 0 0)
    (whirl-it img drawable angle times)
    (gimp-invert img drawable)

    ; Whirl upper right

    (gimp-rect-select img hsize 0 hsize hsize REPLACE 0 0)
    (whirl-it img drawable (- angle) times)

    ; Whirl lower left

    (gimp-rect-select img 0 hsize hsize hsize REPLACE 0 0)
    (whirl-it img drawable (- angle) times)

    ; Whirl lower right

    (gimp-rect-select img hsize hsize hsize hsize REPLACE 0 0)
    (whirl-it img drawable angle times)
    (gimp-invert img drawable)

    ; Terminate

    (gimp-selection-none img)
    (gimp-palette-set-foreground old-fg-color)
    (gimp-palette-set-background old-bg-color)
    (gimp-image-enable-undo img)
    (gimp-display-new img)))

(script-fu-register "script-fu-swirly-pattern"
		    "<Toolbox>/Xtns/Script-Fu/Patterns/Swirly (tileable)"
		    "Create a swirly pattern"
		    "Federico Mena Quintero"
		    "Federico Mena Quintero"
		    "June 1997"
		    ""
		    SF-VALUE "Quarter size" "20"
		    SF-VALUE "Whirl angle" "90"
		    SF-VALUE "Number of times to whirl" "4")

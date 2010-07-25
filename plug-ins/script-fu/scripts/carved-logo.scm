;  CARVE-TEXT
;   Carving, embossing, & stamping
;   Process taken from "The Photoshop 3 WOW! Book"
;   http://www.peachpit.com


(define (carve-brush brush-size)
  (cond ((<= brush-size 5) "Circle (05)")
	((<= brush-size 7) "Circle (07)")
	((<= brush-size 9) "Circle (09)")
	((<= brush-size 11) "Circle (11)")
	((<= brush-size 13) "Circle (13)")
	((<= brush-size 15) "Circle (15)")
	((<= brush-size 17) "Circle (17)")
	((> brush-size 17) "Circle (19)")))

(define (carve-scale val scale)
  (* (sqrt val) scale))

(define (calculate-inset-gamma img layer)
  (let* ((stats (gimp-histogram img layer 0 0 255))
	 (mean (car stats)))
    (cond ((< mean 127) (+ 1.0 (* 0.5 (/ (- 127 mean) 127.0))))
	  ((>= mean 127) (- 1.0 (* 0.5 (/ (- mean 127) 127.0)))))))

(define (script-fu-carved-logo text size font bg-img carve-raised)
  (let* ((img (car (gimp-file-load 1 bg-img bg-img)))
	 (offx (carve-scale size 0.33))
	 (offy (carve-scale size 0.25))
	 (feather (carve-scale size 0.3))
	 (brush-size (carve-scale size 0.3))
	 (b-size (carve-scale size 1.5))
	 (layer1 (car (gimp-image-active-drawable img)))
	 (mask-layer (car (gimp-text img -1 0 0 text b-size TRUE size PIXELS "*" font "*" "*" "*" "*")))
	 (width (car (gimp-drawable-width mask-layer)))
	 (height (car (gimp-drawable-height mask-layer)))
	 (mask-fs 0)
	 (mask (car (gimp-channel-new img width height "Engraving Mask" 50 '(0 0 0))))
	 (inset-gamma (calculate-inset-gamma img layer1))
	 (mask-fat 0)
	 (mask-emboss 0)
	 (mask-highlight 0)
	 (mask-shadow 0)
	 (shadow-layer 0)
	 (highlight-layer 0)
	 (cast-shadow-layer 0)
	 (csl-mask 0)
	 (inset-layer 0)
	 (il-mask 0)
	 (old-fg (car (gimp-palette-get-foreground)))
	 (old-bg (car (gimp-palette-get-background)))
	 (old-brush (car (gimp-brushes-get-brush))))
    (gimp-image-disable-undo img)

    (gimp-layer-set-preserve-trans mask-layer TRUE)
    (gimp-palette-set-background '(255 255 255))
    (gimp-edit-fill img mask-layer)
    (gimp-palette-set-background '(0 0 0))
    (gimp-edit-fill img mask)

    (plug-in-tile 1 img layer1 width height FALSE)

    (gimp-image-add-channel img mask 0)
    (gimp-edit-copy img mask-layer)
    (set! mask-fs (car (gimp-edit-paste img mask FALSE)))
    (gimp-floating-sel-anchor mask-fs)
    (if (= carve-raised TRUE)
	(gimp-invert img mask))

    (gimp-image-remove-layer img mask-layer)

    (set! mask-fat (car (gimp-channel-copy mask)))
    (gimp-image-add-channel img mask-fat 0)
    (gimp-selection-load img mask-fat)
    (gimp-brushes-set-brush (carve-brush brush-size))
    (gimp-palette-set-foreground '(255 255 255))
    (gimp-edit-stroke img mask-fat)
    (gimp-selection-none img)

    (set! mask-emboss (car (gimp-channel-copy mask-fat)))
    (gimp-image-add-channel img mask-emboss 0)
    (plug-in-gauss-rle 1 img mask-emboss feather TRUE TRUE)
    (plug-in-emboss 1 img mask-emboss 315.0 45.0 7 TRUE)

    (gimp-palette-set-background '(180 180 180))
    (gimp-selection-load img mask-fat)
    (gimp-selection-invert img)
    (gimp-edit-fill img mask-emboss)
    (gimp-selection-load img mask)
    (gimp-edit-fill img mask-emboss)
    (gimp-selection-none img)

    (set! mask-highlight (car (gimp-channel-copy mask-emboss)))
    (gimp-image-add-channel img mask-highlight 0)
    (gimp-levels img mask-highlight 0 180 255 1.0 0 255)

    (set! mask-shadow mask-emboss)
    (gimp-levels img mask-shadow 0 0 180 1.0 0 255)

    (gimp-edit-copy img mask-shadow)
    (set! shadow-layer (car (gimp-edit-paste img layer1 FALSE)))
    (gimp-floating-sel-to-layer shadow-layer)
    (gimp-layer-set-mode shadow-layer MULTIPLY)

    (gimp-edit-copy img mask-highlight)
    (set! highlight-layer (car (gimp-edit-paste img shadow-layer FALSE)))
    (gimp-floating-sel-to-layer highlight-layer)
    (gimp-layer-set-mode highlight-layer SCREEN)

    (gimp-edit-copy img mask)
    (set! cast-shadow-layer (car (gimp-edit-paste img highlight-layer FALSE)))
    (gimp-floating-sel-to-layer cast-shadow-layer)
    (gimp-layer-set-mode cast-shadow-layer MULTIPLY)
    (gimp-layer-set-opacity cast-shadow-layer 75)
    (plug-in-gauss-rle 1 img cast-shadow-layer feather TRUE TRUE)
    (gimp-layer-translate cast-shadow-layer offx offy)

    (set! csl-mask (car (gimp-layer-create-mask cast-shadow-layer BLACK-MASK)))
    (gimp-image-add-layer-mask img cast-shadow-layer csl-mask)
    (gimp-selection-load img mask)
    (gimp-palette-set-background '(255 255 255))
    (gimp-edit-fill img csl-mask)

    (set! inset-layer (car (gimp-layer-copy layer1 TRUE)))
    (gimp-image-add-layer img inset-layer 1)

    (set! il-mask (car (gimp-layer-create-mask inset-layer BLACK-MASK)))
    (gimp-image-add-layer-mask img inset-layer il-mask)
    (gimp-selection-load img mask)
    (gimp-palette-set-background '(255 255 255))
    (gimp-edit-fill img il-mask)
    (gimp-selection-none img)

    (gimp-levels img inset-layer 0 0 255 inset-gamma 0 255)

    (gimp-image-remove-channel img mask)
    (gimp-image-remove-channel img mask-fat)
    (gimp-image-remove-channel img mask-highlight)
    (gimp-image-remove-channel img mask-shadow)

    (gimp-layer-set-name layer1 "Carved Surface")
    (gimp-layer-set-name shadow-layer "Bevel Shadow")
    (gimp-layer-set-name highlight-layer "Bevel Highlight")
    (gimp-layer-set-name cast-shadow-layer "Cast Shadow")
    (gimp-layer-set-name inset-layer "Inset")

    (gimp-palette-set-foreground old-fg)
    (gimp-palette-set-background old-bg)
    (gimp-brushes-set-brush old-brush)
    (gimp-display-new img)
    (gimp-image-enable-undo img)))

(script-fu-register "script-fu-carved-logo"
		    "<Toolbox>/Xtns/Script-Fu/Logos/Carved"
		    "Carve the text from the specified image.  The image will be automatically tiled to accomodate the rendered text string.  The \"Carve Raised Text\" parameter determines whether to carve the text itself, or around the text."
		    "Spencer Kimball"
		    "Spencer Kimball"
		    "1997"
		    ""
		    SF-VALUE "Text String" "\"Marble\""
		    SF-VALUE "Font Size (in pixels)" "100"
		    SF-VALUE "Font" "\"Engraver\""
		    SF-VALUE "Background Img" (string-append "\"" gimp-data-dir "/scripts/texture3.jpg\"")
		    SF-TOGGLE "Carve Raised Text" FALSE)

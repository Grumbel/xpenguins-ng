; resize-frames - resize an xpenguins image with no bleeding between frames
; Copyright (C) 2001 Robin Hogan
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

(define (script-fu-resize-frames img
				 drawable
				 originalWidth
				 originalHeight
				 newWidth
				 newHeight)
  (let* ((imageWidth (car (gimp-image-width img)))
	 (imageHeight (car (gimp-image-height img)))
	 (xframes (/ imageWidth originalWidth))
	 (yframes (/ imageHeight originalHeight))
	 (x 0)
	 (y 0)
	 (newx 0)
	 (newy 0)
	 (newImageWidth (* newWidth xframes))
	 (newImageHeight (* newHeight yframes))
	 (current (car (gimp-image-get-active-layer img))))
       
    (gimp-image-undo-disable img)
    (gimp-layer-set-visible current 0)

    (while (< y yframes)
	   (while (< x xframes)
		  (set! newLayer (car (gimp-layer-copy current TRUE)))
		  (gimp-layer-set-visible newLayer 1)
		  (gimp-image-add-layer img newLayer (+ 1 x (* y xframes)))
		  (gimp-layer-resize newLayer originalWidth originalHeight
				     ( - (* x originalWidth)) ( - (* y originalHeight)))
		  (gimp-layer-scale newLayer newWidth newHeight FALSE)
		  (gimp-layer-set-offsets newLayer (* x newWidth) (* y newHeight))
		  (set! x (+ x 1)))
	   (set! y (+ y 1))
	   (set! x 0))
    (gimp-layer-delete current)
    (gimp-image-resize img (* xframes newWidth) (* yframes newHeight) 0 0)
    (gimp-image-merge-visible-layers img 1)

    (gimp-image-undo-enable img)
    (gimp-displays-flush)))

(script-fu-register "script-fu-resize-frames"
		    _"<Image>/Script-Fu/Utils/Resize framed image..."
		    "Resize an image frame by frame to prevent bleeding between frames. This is useful for creating themes for XPenguins."
		    "Robin Hogan <R.J.Hogan@reading.ac.uk>"
		    "Robin Hogan"
		    "September 2001"
		    "RGB*"
		    SF-IMAGE "Image" 0
		    SF-DRAWABLE "Drawable" 0
		    SF-ADJUSTMENT _"Original frame width" '(32 1 10000 1 10 1 1)
		    SF-ADJUSTMENT _"Original frame height" '(32 1 10000 1 10 1 1)
		    SF-ADJUSTMENT _"New frame width" '(32 1 10000 1 10 1 1)
		    SF-ADJUSTMENT _"New frame height" '(32 1 10000 1 10 1 1))

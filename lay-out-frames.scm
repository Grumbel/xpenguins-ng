;; SPDX-FileCopyrightText: 1999-2001 Robin Hogan
;; SPDX-License-Identifier: GPL-2.0-or-later

; lay-out-frames - lay out frames of animated gif to make xpenguins theme
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

(define (script-fu-lay-out-frames img
				  drawable
				  reversed
				  swap
				  flatten
				  index)
  (let* ((frameWidth (car (gimp-image-width img)))
	 (frameHeight (car (gimp-image-height img)))
	 (layers (gimp-image-get-layers img))
	 (nFrames (car layers))
	 (frameList (cadr layers))
	 (width (* frameWidth nFrames))
	 (height frameHeight)
	 (frame 0)
	 (normalYoffset 0)
	 (reversedYoffset frameHeight)
	 (flattenedFrame #f))
    
    (gimp-image-undo-disable img)
    (if (= reversed TRUE)
	(begin
	  (set! height (* frameHeight 2))
	  (if (= swap TRUE)
	      (begin
		(set! normalYoffset frameHeight)
		(set! reversedYoffset 0)))))

    (gimp-image-resize img width height 0 0)

    (while (< frame nFrames)
	   (let* ((currentFrame (aref frameList (- (- nFrames 1) frame)))
		  (offsets (gimp-drawable-offsets currentFrame))
		  (xoffset (car offsets))
		  (yoffset (cadr offsets))
		  (newFrame #f))
	     (gimp-layer-resize currentFrame
				frameWidth
				frameHeight
				xoffset
				yoffset)
	     (gimp-layer-set-offsets currentFrame
				     (* frame frameWidth)
				     normalYoffset)
	     (if (= reversed TRUE)
		 (begin
		   (set! newFrame (car (gimp-layer-copy currentFrame TRUE)))
		   (gimp-image-add-layer img newFrame -1)
		   (gimp-layer-set-offsets newFrame
					   (* frame frameWidth)
					   reversedYoffset)
		   (gimp-flip newFrame 0)
		 ))
	     (set! frame (+ frame 1))))

    (if (= flatten TRUE)
	(gimp-image-merge-visible-layers img 0))
    (if (= index TRUE)
	(gimp-convert-indexed img 2 0 63 0 1 ""))
    (gimp-image-undo-enable img)
    (gimp-displays-flush)))

(script-fu-register "script-fu-lay-out-frames"
		    _"<Image>/Script-Fu/Utils/Lay out frames..."
		    "Lay out the frames of an animation side by side. This is useful for creating themes for the XPenguins program."
		    "Robin Hogan <R.J.Hogan@reading.ac.uk>"
		    "Robin Hogan"
		    "April 2001"
		    ""
		    SF-IMAGE "Image" 0
		    SF-DRAWABLE "Drawable" 0
		    SF-TOGGLE _"Include reversed frames" FALSE
		    SF-TOGGLE _"Swap normal & reversed frames" FALSE
		    SF-TOGGLE _"Merge down frames" FALSE
		    SF-TOGGLE _"Convert to an indexed image" FALSE)

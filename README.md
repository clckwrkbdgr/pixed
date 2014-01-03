pixed
=====

Simple pixel editor

Usage
-----
	pixed [-w WIDTH -h HEIGHT] FILE

FILE must be of one of the image formats Qt is supporting.
By default, Qt can read and write following formats: BMP, JPG, JPEG, PNG, PPM, TIFF, XBM, XPM.
If FILE does not exist yet, it will be created upon start of the editor as 32x32 TrueColor image.
FILE will be saved upon exiting.
WIDTH and HEIGHT must be greater than zero and must be present together. When width and height are supplied, image is created anew.

Interface
---------
Image will be displayed in the center of the screen.
On the left side of the screen there is a color information present: palette for images that has one and current color view attached to the right, pointing at current choice of color.
At the corner of the current color view there is a small rectangle which displays color that are right under cursor on the image.

Controls
--------
**Q** - quit (and save).  
**Shift+S** - save.  
**Arrow keys or 'hjklyubn' (vim keys)** - move cursor.  
**Shift + Arrow keys** - shift image in view area.  
**Home** - center image back.  
**+/-** - zoom in/out. Minimal (and default) zoom is 2.  
**Ctrl+G** - switch drawing grid on/off (off by default).  
**D, I or Space** - put current color at current position.  
**P** - floodfill area under cursor with current color.  
**.** - pick color at current position as current color.  
**PgUp/PgDown** - scroll through palette colors.  
**\#** - start color input mode (see below).  

Color input mode
----------------
This allows to change current color value.
After pressing '#' key this symbol will be displayed to the right of the current color view. It acts like a prompt: one can enter color value like #fff, #ff00ff etc.
Editing finished when either Enter or Esc keys are pressed. In latter case, color input is cancelled.
If entered value is '-', color is considered to be fully transparent ("None" color).
Entered color will replace current color of palette, resulting in immediate repainting that color everywhere on the image.

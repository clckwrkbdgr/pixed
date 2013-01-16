pixed
=====

Simple pixel editor

Usage
-----
	pixed FILE

FILE must be of one of the image formats Qt is supporting.
By default, Qt can read and write following formats: BMP, JPG, JPEG, PNG, PPM, TIFF, XBM, XPM.
If FILE does not exist yet, it will be created upon start of the editor as 32x32 TrueColor image.
FILE will be saved upon exiting.

Interface
---------
Image will be displayed in the center of the screen.
On the left side of the screen there is a color information present: palette for images that has one or simple current color view.
If images has palette there also will be current color view attached to the right of palette, pointing at current choice of color.
At the corner of the current color view there is a small rectangle which displays color that are right under cursor on the image.

Controls
--------
Q - quit (and save).
Arrow keys - move cursor.
Shift + Arrow keys - shift image in view area.
Home - center image back.
P - toggle on/off extended cursor.
+/- - zoom in/out. Minimal zoom is 2.
Space - put current color at current position.
. - pick color at current position as current color.
PgUp/PgDown - scroll through palette colors.
# - start color input mode (see below).

Color input mode
----------------
Mode which allows to change current color for TrueColor images or change current color of palette.
After pressing '#' key that sign will be displayed to the right of the current color view. It acts like a prompt: one can enter color value like #fff, #ff00ff etc.
Editing finished when either Enter or Esc keys are pressed. In latter case, color input is cancelled.
Entered color will be immediately set for TrueColor images as current color, and for paletted ones it will replace current color of palette, resulting in immediate replacing that color everywhere on the image.

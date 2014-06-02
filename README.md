pixed
=====

Simple pixel editor for XPM files.
Editor designed specifically to preserve XPM file formatting upon rewriting inlcuding whitspaces and comments.

Installation
------------

Requirements:

* SDL2 library.
* [libchton](https://github.com/umi0451/libchthon)

Simply run `make` and put created `pixed` file to wherever you want.

Usage
-----
	pixed [-w WIDTH -h HEIGHT] FILE.xpm

FILE must be of of XPM format (XPM v1).
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
**A** - add new color to palette (starts color input mode immediately).
**C** - start selection mode - Copy step (see below).
**V** - start selection mode - Paste step (see below).
**Esc** - breaks color input or selection mode and returns to drawing.

Color input mode
----------------
This allows to change current color value.
After pressing '#' key this symbol will be displayed to the right of the current color view. It acts like a prompt: one can enter color value like #fff, #ff00ff etc.
Editing finished when either Enter or Esc keys are pressed. In latter case, color input is cancelled.
If entered value is '-', color is considered to be fully transparent ("None" color).
Entered color will replace current color of palette, resulting in immediate repainting that color everywhere on the image.

Selection mode
--------------

This mode allows to copy and paste rectangular part of the canvas. Mode divides into two step: Copy and Paste. In Copy mode user selects area to copy. Area lays between two point - one is the cursor position where **C** key was pressed, and the other one is the current cursor position. After area needed is selected, pressing **V** begins a Paste mode. In Paste mode user chooses position where to paste selected pixels and presses **Enter**. Pixels are pasted as-is, i.e. no alpha blending is done.

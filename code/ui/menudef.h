/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (“RTCW SP Source Code”).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning the license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/* Constants the ported idTech 3 menu framework shares between the .menu
   parser and the window code. The .menu scripts reference the item and
   window values by number, so the numbering is part of the file format.
   The unused RTCW leftovers (hud flags, cgame and ui ownerdraw ids,
   feeders, voice chats, wolf multiplayer classes) are purged; this port
   has no cgame hud and the Duke ownerdraws and feeders have their own
   id spaces in the ui module. */

#define ITEM_TYPE_TEXT              0       // simple text
#define ITEM_TYPE_BUTTON            1       // button, basically text with a border
#define ITEM_TYPE_RADIOBUTTON       2       // toggle button, may be grouped
#define ITEM_TYPE_CHECKBOX          3       // check box
#define ITEM_TYPE_EDITFIELD         4       // editable text, associated with a cvar
#define ITEM_TYPE_COMBO             5       // drop down list
#define ITEM_TYPE_LISTBOX           6       // scrollable list
#define ITEM_TYPE_MODEL             7       // model
#define ITEM_TYPE_OWNERDRAW         8       // owner draw, name specs what it is
#define ITEM_TYPE_NUMERICFIELD      9       // editable text, associated with a cvar
#define ITEM_TYPE_SLIDER            10      // mouse speed, volume, etc.
#define ITEM_TYPE_YESNO             11      // yes no cvar setting
#define ITEM_TYPE_MULTI             12      // multiple list setting, enumerated
#define ITEM_TYPE_BIND              13      // key binding field
#define ITEM_TYPE_MENUMODEL         14      // special menu model
#define ITEM_TYPE_VALIDFILEFIELD    15      // text must be valid for use in a dos filename

#define ITEM_ALIGN_LEFT             0       // left alignment
#define ITEM_ALIGN_CENTER           1       // center alignment
#define ITEM_ALIGN_RIGHT            2       // right alignment

#define ITEM_TEXTSTYLE_NORMAL           0   // normal text
#define ITEM_TEXTSTYLE_BLINK            1   // fast blinking
#define ITEM_TEXTSTYLE_PULSE            2   // slow pulsing
#define ITEM_TEXTSTYLE_SHADOWED         3   // drop shadow ( need a color for this )
#define ITEM_TEXTSTYLE_OUTLINED         4   // drop shadow ( need a color for this )
#define ITEM_TEXTSTYLE_OUTLINESHADOWED  5   // drop shadow ( need a color for this )
#define ITEM_TEXTSTYLE_SHADOWEDMORE     6   // drop shadow ( need a color for this )

#define WINDOW_BORDER_NONE          0       // no border
#define WINDOW_BORDER_FULL          1       // full border based on border color ( single pixel )
#define WINDOW_BORDER_HORZ          2       // horizontal borders only
#define WINDOW_BORDER_VERT          3       // vertical borders only
#define WINDOW_BORDER_KCGRADIENT    4       // horizontal border using the gradient bars

#define WINDOW_STYLE_EMPTY          0       // no background
#define WINDOW_STYLE_FILLED         1       // filled with background color
#define WINDOW_STYLE_GRADIENT       2       // gradient bar based on background color
#define WINDOW_STYLE_SHADER         3       // gradient bar based on background color
#define WINDOW_STYLE_TEAMCOLOR      4       // team color
#define WINDOW_STYLE_CINEMATIC      5       // cinematic

/* color range type, addColorRange versus addColorRangeRel */
#define RANGETYPE_ABSOLUTE  0
#define RANGETYPE_RELATIVE  1

/* list box element types */
#define LISTBOX_TEXT        0x00
#define LISTBOX_IMAGE       0x01

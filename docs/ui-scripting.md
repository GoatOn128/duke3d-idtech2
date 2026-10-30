# UI scripting

This is a hands-on guide to the menu system in the ui module (ui.dll). It covers how the menus load, how a .menu file is put together, what every item type and keyword does, how the script commands work, and where the data files fit in. After reading this you should be able to open the ui folder, follow any existing screen, and build a new one from scratch without touching C++.

The menu code is a port of the idTech 3 (Return to Castle Wolfenstein) menu framework. The original framework parsed text files, built menus and items from them, and painted through a draw interface. That structure is intact here. What changed is the look: instead of idTech 3 fonts and shaders, the module draws through the engine's Duke helpers, so menus get the real Duke3D glyphs, tiles and the spinning nuke cursor. The GPL source this comes from lives in the RTCW SP source release; the original menu files from that game were the template for ours.

## How the menus load

When the game starts, the module reads ui/menus.txt. That file is a plain list that names the menu files to load:

```
// the Duke menus, one load line per screen
load ui/main.menu
load ui/options.menu
load ui/keys.menu
```

A load line takes a path relative to the game's base directory. The listed files are read one after another, and every menu block inside them joins the live menu set.

menus.txt also accepts include lines, which load another list file and then keep going:

```
#include ui/multiplayer.lst
```

An include works exactly like pasting the other file's lines into this one. Includes can nest, up to eight levels deep, and a deeper level guards against runaway loops (a list that includes itself just stops at the depth limit with a console warning). This is handy for grouping: one file for the single player screens, one for the multiplayer screens, and a small menus.txt that includes both.

If menus.txt is missing, no menus load at all, and if a load line names a file that does not exist you get a console warning and the rest of the list still runs.

## The .menu file format

A .menu file holds one or more menu blocks. A menu block starts with menuDef and a set of braces:

```
menuDef {
    name "myoptions"
    rect 0 0 640 480
    focuscolor 1 1 1 1
    onOpen { setfocus opt_list }
    onESC { play item15.wav ; close myoptions ; open main }

    itemDef {
        name "opt_list"
        type ITEM_TYPE_TEXT
        text "OPTIONS"
        rect 0 16 640 16
        textalign 1
        textscale 1
        style 0
        decoration
    }
}
```

The same shape repeats everywhere: a keyword, then its values. Text strings go in double quotes. Numbers and colors are plain values, a color being four numbers (red green blue alpha, each 0 to 1). Comments are the usual // line comments and /* block comments */. The parser is forgiving about whitespace and line breaks, and it warns on the console when it sees a keyword it does not know, so typos surface fast.

### menuDef keywords

These are the keywords a menuDef block understands:

- name: the menu's name, used by open, close, setfocus and the module's own lookups.
- fullscreen: 1 makes the menu cover the whole screen and own the keyboard.
- rect: the menu rectangle in virtual screen coordinates (see the coordinates section below).
- style, border, bordersize, visible: window look basics.
- backcolor, forecolor, bordercolor, focuscolor, disablecolor, outlinecolor: the color set. focuscolor is the color the focused item's text pulses toward; the pulse speed comes from ui/config.txt.
- background: a background asset name.
- ownerdraw, ownerdrawFlag: hand a menu-level draw to the module (rare).
- onOpen: script that runs when the menu opens. onClose: when it closes. onESC: when the player presses escape. onROQDone: when a menu cinematic finishes.
- execKey, execKeyInt: bind a script to a raw key press.
- onNamedEvent: named handler blocks, run by the namedEvent script command.
- soundLoop: a sound that loops while the menu is up.
- popup: 1 keeps the menu clear of the focus chain, for popups that float over others.
- fadeClamp, fadeCycle, fadeAmount: fade tuning for the show and hide script commands.
- outOfBoundsClick: close the menu when the player clicks outside its rect.
- cinematic: a cinematic to play inside the menu rect.

### The item blocks

Each item is one row, button or list. The def name picks the item type:

- itemDef: the all-purpose item. With just text and a rect it is a label; with an action it is clickable; with type ITEM_TYPE_* set it can be a checkbox, radio button, text or more.
- windowDef: a container. Items inside a nested windowDef are placed relative to the container, and the parser bakes the container's position into each child, so children move with the container. A container with scrollable also lets its rows scroll under the mouse wheel.
- buttonDef: a clickable text row.
- editDef: a text entry field (type ITEM_TYPE_EDITFIELD when used with a cvar).
- choiceDef: a multi choice setting, the left/right arrows on a value. The choices come from cvarStrList (text picks) or cvarFloatList (number picks).
- sliderDef: a slider bound to a cvar between min and max values.
- bindDef: a key binding row, waiting for the player to press a new key.
- listDef: a scrolling list fed by a feeder (see the feeders section).

### Item keywords

These are the keywords an item block understands:

Layout and look: name, text, textfile (text from a file), textsavegame (text from the savegame info line), group (for setitemcolor and friends), rect, style, border, bordersize, decoration (no mouse, no focus), visible, background, ownerdraw, ownerdrawFlag, align, textalign (0 left, 1 center, 2 right), textalignx, textaligny, textscale, textstyle (0 plain, 1 with shadow, 3 blinking), textfont, backcolor, forecolor, bordercolor, outlinecolor, addColorRange, addColorRangeRel (recolor text by value).

Behavior: notselectable (list rows the cursor skips), wrapped and autowrapped (long text folding), horizontalscroll, scrollable, mouseEnter, mouseExit, mouseEnterText, mouseExitText, onFocus, leaveFocus, action (runs when the item is picked), accept (runs when enter is pressed on the row), doubleclick, onNamedEvent, special, focusSound.

Settings and cvars: cvar (the cvar the item reads and writes), cvarTest with enableCvar, disableCvar, showCvar, hideCvar (switch an item on, off, visible or hidden based on a cvar's value), maxChars, maxPaintChars, cvarFloat, cvarStrList, cvarFloatList.

Lists: type, elementwidth, elementheight, feeder, elementtype, columns (covered with the listDef below), cinematic.

### Nested defs and containers

An item block can hold further item blocks. Those nested items belong to the enclosing item as their container, and the parser adds the container's origin to each child's rect while parsing. The result: move the container, and everything inside follows. The options screens use this to lay out rows inside a fixed frame.

## Coordinates

Menu script coordinates live in a 640x480 virtual space, the idTech 3 convention the framework was written for. The module maps that space onto the Duke 320x200 menu grid when it draws: an x of 320 in script is the Duke center at 160, and the y values compress roughly by 2.4 with a small offset for the glyph baseline.

Two rules of thumb cover most cases:

- To center something horizontally, give it a full-width rect (x 0, width 640) and textalign 1, or place it at x 320 with the item centered.
- A Duke y position multiplies up by about 2.4 in script space; the code tables in ui_duke_menus.cpp and ui_duke_menus2.cpp store their positions in Duke coordinates directly.

One important exception: ownerdraw items do not use the script rect for their position. The module's ownerdraw code paints those rows at the Duke positions baked into its line tables (dukeLines and duke2Lines), so moving or resizing the item in the .menu file changes nothing on screen. The rect still matters for mouse hit testing and focus order; the drawn pixels come from the code table. This is why editing the position of a header or a special row in a .menu file appears to do nothing. Making these rows honor their script rects is planned work; until then, positions of ownerdraw rows are changed in the code tables.

## Script commands

Scripts are semicolon-separated commands, and they run in order:

```
onESC { play item15.wav ; close keys ; open options }
```

The framework commands:

- open / close: open or close a menu by name. close with no name closes the current menu.
- show / hide: show or hide a group or a named item.
- fadein / fadeout: fade a group or item in or out using the menu's fade settings.
- setfocus: move focus to a named item.
- setitemcolor: recolor a group or item, for example setitemcolor mygroup forecolor 1 0 0 1.
- setcolor: recolor the item the script runs on.
- setbackground / setasset: swap an item's background or asset.
- setcvar: write a cvar.
- setteamcolor: paint with the team color.
- setplayermodel / setplayerhead: player setup helpers.
- transition: animate a group or item from one rect to another over time.
- orbit: spin a group or item around a point.
- play: play a sound by name once. playlooped: keep it looping.
- exec: run a config file.
- clipboard / showpage: notebook helpers kept from the framework.
- namedEvent: run every handler registered under a name, on menus and items alike.
- addlistitem: append a row to a list.

The module adds one more: duke. It takes a subcommand and forwards it to the module code, which is how script files reach the engine side without new framework plumbing:

- duke readsave: refresh the save and load slot rows.
- duke readvideo: rebuild the video mode lists for the current renderer.
- duke refreshservers: ping the LAN server list again.
- duke startserver: start a server with the current settings.
- duke save N / duke load N: write or read save slot N (0 to 10).
- duke connect N / duke connectselected: join server N, or the one the server list cursor is on.
- duke resetcontrols: write every default binding back, the reset row on the keys screen.

Unknown commands warn on the console, and an unknown duke subcommand eats its argument so the rest of the script is not misread.

## Ownerdraw ids

Some rows are drawn by the module instead of by the framework, and the script picks the drawing code with the ownerdraw number:

- 0: the spinning Duke3D logo on the main menu.
- 1: the menu header bar, the metal plate with rivets (the MENUBAR tile, drawn centered).
- 2: the high header bar, the same plate drawn higher, for the screens whose headers ride the top of the screen (options and customize controls, matching the original game's own placement for those two screens).
- 16 and up: the Duke line tables. Each number indexes a row in the code tables (dukeLines in ui_duke_menus.cpp, duke2Lines in ui_duke_menus2.cpp) that carries the text, the Duke position, brightness and cursor behavior.
- 64 and up (DUKE2_OD_BASE): the second menu set's ids. 64 to 74 are the save slot rows (slot 0 to 10), 80 to 87 the LAN server rows, 88 the credits scroller, 96 to 110 the deathmatch flag rows, 111 the deathmatch flag value readout, and 112 and up the static lines of duke2Lines.

The keys screen uses ownerdraw 40 for one of its footer helpers and ownerdraw 2 for its header, for example.

## Feeders and lists

A listDef is a scrolling list whose rows come from a feeder, a number that names a data source the module serves:

- feeder 1: the bind list. The rows come from ui/binds.lst (next section), the label column shows the label text, and the second column shows the bound keys.
- feeder 2: the LAN server list, filled from the engine's server queries.

The list's look comes from its layout keywords:

```
listDef {
    name "bind_list"
    feeder 1
    rect 48 40 544 344
    elementheight 38
    columns 2 0 220 13 260 300 12
    type ITEM_TYPE_LISTBOX
    textscale 1
}
```

elementheight is the row pitch in virtual pixels. columns takes the column count first, then a triplet per column: the column's x offset inside the row, its width, and the most characters it paints. The example above gives the label column 220 virtual pixels and the key column 300, capped at 13 and 12 characters.

A bindDef row waits for a key press and writes it to the engine binding for its command; the listDef version of that flow is the keys screen, where pressing enter on a row starts the capture, the next key becomes the binding, escape cancels, and backspace clears it. While the capture is live, mouse button 2 counts as a regular bindable key; outside a capture it acts like escape.

## The data files

Everything the module reads at startup lives in ui/ as plain text. None of these values are baked into the module anymore: the tables start empty and the files are the only source. Comments are // lines, and unknown keywords warn on the console.

ui/resources.txt: the named tiles. Lines are a name and a tile number. The names follow names.h from the original Duke3D 1.5 source, and the numbers are tile ids in the converted art file:

```
MENUSCREEN          2456
MENUBAR             2457
SPINNINGNUKEICON    2813
LOADSCREEN          3281
```

A name that already exists is retargeted, a new name joins the table. Without this file no tiles draw, so the menu screens fall back to text only.

ui/animations.txt: the motion values, one keyword and value per line. slideTime and slideDist drive the screen-change slide; nukeFrames and nukeFrameMs drive the spinning cursor cycle; nukeLeftX and nukeRightX park the cursor pair's columns on centered menus. Without the file the slide stays off and the spinning cursor hands over to the plain cursor.

ui/config.txt: the module behavior values. soundChannel is the engine sound channel menu sounds play on. focusPulse is the pulse rate of focused text (smaller is faster; zero or less freezes the pulse). focusBlink is the blink switch time in milliseconds for blinking text style. saveInfoLength caps the savegame info line in the load and save feeders, with 0 meaning unlimited. All four warn on unknown keywords and degrade safely when missing.

ui/binds.lst: the customize controls rows. Lines are a quoted label, a quoted command and a quoted default key:

```
"FIRE"              "+attack"       "CTRL"
"SIDE STEP LEFT"    "+moveleft"     ","
"DROP ITEM"         "invdrop"       ""
```

The label is what the list shows. The command is what the engine binds. The default key is what the reset row writes back; it must match the engine's own key name strings (the same names the bind command prints in the console), an empty name leaves the default unbound, and a name the engine does not know warns on the console and stays unbound. The commands are idTech 2 engine commands, and the game code can add more. The file is the only source for the list: the module ships no built-in rows, and without the file the keys screen simply has nothing to show.

## A screen from scratch

Here is a small but complete screen that uses the pieces above: a container with two rows, a header on the high bar, and an escape that plays the original menu sound.

```
menuDef {
    name "example"
    fullscreen 1
    focuscolor 1 1 1 1
    onOpen { setfocus ex_first }
    onESC { play item15.wav ; close example ; open main }

    // the header rides the high bar like the options screen
    itemDef {
        name "ex_header"
        ownerdraw 2
        rect 0 0 640 48
        decoration
    }

    windowDef {
        name "ex_frame"
        rect 80 64 480 160

        itemDef {
            name "ex_first"
            text "FIRST ROW"
            type ITEM_TYPE_BUTTON
            rect 0 0 480 24
            textalign 1
            textscale 1
            action { play target00.wav ; close example ; open options }
        }

        itemDef {
            name "ex_second"
            text "SECOND ROW"
            type ITEM_TYPE_BUTTON
            rect 0 32 480 24
            textalign 1
            textscale 1
            action { exec "quit" }
        }
    }
}
```

Add a load line for its file in ui/menus.txt and it shows up in game. Nothing in it needs a recompile.

## Gotchas, collected

- Ownerdraw rows ignore their script rect for drawing; their positions live in the code line tables. The rect still drives focus and mouse hit tests.
- The bind list shows real labels now, but the key column still reads the engine bindings by command name, so a command in binds.lst that nothing binds to shows as unbound rather than disappearing.
- mouse button 2 behaves like escape in the menus, except while a bind capture is waiting, where it can be bound like any other key.
- Text style 3 blinks with the focusBlink rate from ui/config.txt; focused text pulses with focusPulse. Setting either to zero freezes the effect instead of dividing by zero.
- All four data files are optional, but the module assumes them: without resources.txt the screens lose their art, without binds.lst the keys screen is empty, and so on. The console lists every file it could not find.
- Unknown keywords, malformed values and missing files warn on the console and parsing continues, so one typo never blanks the whole menu set.

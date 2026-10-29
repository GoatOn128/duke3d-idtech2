/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// console.cpp

#include "client.h"
#include "../qcommon/dneditfield.h"
#include "../qcommon/dnconsolehistory.h"
#include "dndebuggraph.h"

console_t	con;

cvar_t		*con_notifytime;

// BFG console debug graphs: rolling frame times (client/dndebuggraph.h)
static cvar_t *con_frameTimes;

// the BFG consoleField: edit line with cycling tab completion
DN_EditField	consoleField;

// BFG style key repeat bookkeeping for the console scroll keys
#define CONSOLE_FIRSTREPEAT	200
#define CONSOLE_REPEAT		100
static int	con_lastKeyEvent = -1;
static int	con_nextKeyEvent = CONSOLE_FIRSTREPEAT;

extern	qboolean	keydown[256];


#define		MAXCMDLINE	256
		

void DrawString (int x, int y, char *s)
{
	while (*s)
	{
		re.DrawChar (x, y, *s);
		x+=8;
		s++;
	}
}

void DrawAltString (int x, int y, char *s)
{
	while (*s)
	{
		re.DrawChar (x, y, *s ^ 0x80);
		x+=8;
		s++;
	}
}


void Key_ClearTyping (void)
{
	consoleField.ClearAutoComplete ();	// clear any typing
	consoleField.Clear ();
}

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f (void)
{
	SCR_EndLoadingPlaque ();	// get rid of loading plaque

	if (cl.attractloop)
	{
		Cbuf_AddText ("killserver\n");
		return;
	}

	if (cls.state == ca_disconnected)
	{	// this engine has no demo loop, so the console key swaps
		// between the console and the main menu, like the idTech3
		// and idTech4 front ends
		Key_ClearTyping ();
		Con_ClearNotify ();

		if (cls.key_dest == key_console)
			M_Menu_Main_f ();
		else
		{
			M_ForceMenuOff ();
			cls.key_dest = key_console;
		}
		return;
	}

	Key_ClearTyping ();
	Con_ClearNotify ();

	if (cls.key_dest == key_console)
	{
		M_ForceMenuOff ();
		Cvar_Set ("paused", "0");
	}
	else
	{
		M_ForceMenuOff ();
		cls.key_dest = key_console;	

		if (Cvar_VariableValue ("maxclients") == 1 
			&& Com_ServerState ())
			Cvar_Set ("paused", "1");
	}
}

/*
================
Con_ToggleChat_f
================
*/
void Con_ToggleChat_f (void)
{
	Key_ClearTyping ();

	if (cls.key_dest == key_console)
	{
		if (cls.state == ca_active)
		{
			M_ForceMenuOff ();
			cls.key_dest = key_game;
		}
	}
	else
		cls.key_dest = key_console;
	
	Con_ClearNotify ();
}

/*
================
Con_Clear_f
================
*/
void Con_Clear_f (void)
{
	memset (con.text, ' ', CON_TEXTSIZE);
	con.display = con.current;	// BFG Clear() goes to the end
}

						
/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
void Con_Dump_f (void)
{
	int		l, x;
	char	*line;
	FILE	*f;
	char	buffer[1024];
	char	name[MAX_OSPATH];

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("usage: condump <filename>\n");
		return;
	}

	Com_sprintf (name, sizeof(name), "%s/%s.txt", FS_Gamedir(), Cmd_Argv(1));

	Com_Printf ("Dumped console text to %s.\n", name);
	FS_CreatePath (name);
	f = fopen (name, "w");
	if (!f)
	{
		Com_Printf ("ERROR: couldn't open.\n");
		return;
	}

	// skip empty lines
	for (l = con.current - con.totallines + 1 ; l <= con.current ; l++)
	{
		line = con.text + (l%con.totallines)*con.linewidth;
		for (x=0 ; x<con.linewidth ; x++)
			if (line[x] != ' ')
				break;
		if (x != con.linewidth)
			break;
	}

	// write the remaining lines
	buffer[con.linewidth] = 0;
	for ( ; l <= con.current ; l++)
	{
		line = con.text + (l%con.totallines)*con.linewidth;
		strncpy (buffer, line, con.linewidth);
		for (x=con.linewidth-1 ; x>=0 ; x--)
		{
			if (buffer[x] == ' ')
				buffer[x] = 0;
			else
				break;
		}
		for (x=0; buffer[x]; x++)
			buffer[x] &= 0x7f;

		fprintf (f, "%s\n", buffer);
	}

	fclose (f);
}

						
/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify (void)
{
	int		i;
	
	for (i=0 ; i<NUM_CON_TIMES ; i++)
		con.times[i] = 0;
}

						
/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f (void)
{
	chat_team = false;
	cls.key_dest = key_message;
}

/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f (void)
{
	chat_team = true;
	cls.key_dest = key_message;
}

/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize (void)
{
	int		i, j, width, oldwidth, oldtotallines, numlines, numchars;
	char	tbuf[CON_TEXTSIZE];

	width = (viddef.width >> 3) - 2;

	if (width == con.linewidth)
		return;

	if (width < 1)			// video hasn't been initialized yet
	{
		width = 38;
		con.linewidth = width;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		memset (con.text, ' ', CON_TEXTSIZE);
	}
	else
	{
		oldwidth = con.linewidth;
		con.linewidth = width;
		oldtotallines = con.totallines;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		numlines = oldtotallines;

		if (con.totallines < numlines)
			numlines = con.totallines;

		numchars = oldwidth;
	
		if (con.linewidth < numchars)
			numchars = con.linewidth;

		memcpy (tbuf, con.text, CON_TEXTSIZE);
		memset (con.text, ' ', CON_TEXTSIZE);

		for (i=0 ; i<numlines ; i++)
		{
			for (j=0 ; j<numchars ; j++)
			{
				con.text[(con.totallines - 1 - i) * con.linewidth + j] =
						tbuf[((con.current - i + oldtotallines) %
							  oldtotallines) * oldwidth + j];
			}
		}

		Con_ClearNotify ();
	}

	consoleField.SetWidthInChars (con.linewidth);

	con.current = con.totallines - 1;
	con.display = con.current;
}


/*
================
Con_Init
================
*/
void Con_Init (void)
{
	con.linewidth = -1;

	Con_CheckResize ();
	
	Com_Printf ("Console initialized.\n");

//
// register our commands
//
	con_notifytime = Cvar_Get ("con_notifytime", "3", 0);

	Cmd_AddCommand ("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand ("togglechat", Con_ToggleChat_f);
	Cmd_AddCommand ("messagemode", Con_MessageMode_f);
	Cmd_AddCommand ("messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand ("clear", Con_Clear_f);
	Cmd_AddCommand ("condump", Con_Dump_f);

	consoleField.SetWidthInChars (con.linewidth);
	DN_ConsoleHistoryInit ();

	// the BFG console debug graphs (client/dndebuggraph.h)
	con_frameTimes = Cvar_Get ("con_frameTimes", "0", 0);
	con.initialized = true;
}


/*
===============
Con_Linefeed
===============
*/
void Con_Linefeed (void)
{
	con.x = 0;
	if (con.display == con.current)
		con.display++;
	con.current++;
	memset (&con.text[(con.current%con.totallines)*con.linewidth]
	, ' ', con.linewidth);
}

/*
================
Con_Print

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
void Con_Print (char *txt)
{
	int		y;
	int		c, l;
	static int	cr;
	int		mask;

	if (!con.initialized)
		return;

	if (txt[0] == 1 || txt[0] == 2)
	{
		mask = 128;		// go to colored text
		txt++;
	}
	else
		mask = 0;


	while ( (c = *txt) )
	{
	// count word length
		for (l=0 ; l< con.linewidth ; l++)
			if ( txt[l] <= ' ')
				break;

	// word wrap
		if (l != con.linewidth && (con.x + l > con.linewidth) )
			con.x = 0;

		txt++;

		if (cr)
		{
			con.current--;
			cr = false;
		}

		
		if (!con.x)
		{
			Con_Linefeed ();
		// mark time for transparent overlay
			if (con.current >= 0)
				con.times[con.current % NUM_CON_TIMES] = cls.realtime;
		}

		switch (c)
		{
		case '\n':
			con.x = 0;
			break;

		case '\r':
			con.x = 0;
			cr = 1;
			break;

		default:	// display character and advance
			y = con.current % con.totallines;
			con.text[y*con.linewidth+con.x] = c | mask | con.ormask;
			con.x++;
			if (con.x >= con.linewidth)
				con.x = 0;
			break;
		}
		
	}
}


/*
==============
Con_CenteredPrint
==============
*/
void Con_CenteredPrint (char *text)
{
	int		l;
	char	buffer[1024];

	l = strlen(text);
	l = (con.linewidth-l)/2;
	if (l < 0)
		l = 0;
	memset (buffer, ' ', l);
	strcpy (buffer+l, text);
	strcat (buffer, "\n");
	Con_Print (buffer);
}

/*

==============================================================================

CONSOLE LINE EDITING (BFG idConsoleLocal::KeyDownEvent port)

==============================================================================
*/

/*
====================
Con_PageUp
====================
*/
static void Con_PageUp (void)
{
	con.display -= 2;
	if (con.current - con.display >= con.totallines)
		con.display = con.current - con.totallines + 1;
}

/*
====================
Con_PageDown
====================
*/
static void Con_PageDown (void)
{
	con.display += 2;
	if (con.display > con.current)
		con.display = con.current;
}

/*
====================
Con_Scroll

BFG idConsoleLocal::Scroll: deals with scrolling text because the SDL
layer filters key repeats. The first repeat is delayed by
CONSOLE_FIRSTREPEAT, the following ones by CONSOLE_REPEAT.
====================
*/
static void Con_Scroll (void)
{
	if (con_lastKeyEvent == -1 || (con_lastKeyEvent + con_nextKeyEvent) > Sys_Milliseconds())
		return;

	if (keydown[K_PGUP] || keydown[K_KP_PGUP])
	{
		Con_PageUp ();
		con_lastKeyEvent = Sys_Milliseconds ();
		con_nextKeyEvent = CONSOLE_REPEAT;
		return;
	}

	if (keydown[K_PGDN] || keydown[K_KP_PGDN])
	{
		Con_PageDown ();
		con_lastKeyEvent = Sys_Milliseconds ();
		con_nextKeyEvent = CONSOLE_REPEAT;
		return;
	}
}

/*
====================
Con_KeyDownEvent

Port of the BFG console key handling. The Q2 Key_Event merges key and
char events and applies keyshift itself, so printable keys are routed
to the field CharEvent and the BFG control codes became keydown checks.
====================
*/
void Con_KeyDownEvent (int key)
{
	char		*cmd;
	const char	*hist;

// keypad keys type their digit, like the Q2 console always did
	switch (key)
	{
	case K_KP_SLASH:		key = '/'; break;
	case K_KP_MINUS:		key = '-'; break;
	case K_KP_PLUS:			key = '+'; break;
	case K_KP_HOME:			key = '7'; break;
	case K_KP_UPARROW:		key = '8'; break;
	case K_KP_PGUP:			key = '9'; break;
	case K_KP_LEFTARROW:	key = '4'; break;
	case K_KP_5:			key = '5'; break;
	case K_KP_RIGHTARROW:	key = '6'; break;
	case K_KP_END:			key = '1'; break;
	case K_KP_DOWNARROW:	key = '2'; break;
	case K_KP_PGDN:			key = '3'; break;
	case K_KP_INS:			key = '0'; break;
	case K_KP_DEL:			key = '.'; break;
	}

// ctrl-l clears the console
	if (key == 'l' && keydown[K_CTRL])
	{
		Con_Clear_f ();
		return;
	}

// ctrl-v pastes, shift-insert pastes
	if ((toupper(key) == 'V' && keydown[K_CTRL]) ||
		((key == K_INS || key == K_KP_INS) && keydown[K_SHIFT]))
	{
		consoleField.Paste ();
		return;
	}

// enter finishes the line
	if (key == K_ENTER || key == K_KP_ENTER)
	{
		cmd = consoleField.GetBuffer ();

		Com_Printf ("]%s\n", cmd);

	// backslash text are commands, else chat
		if (cmd[0] == '\\' || cmd[0] == '/')
			Cbuf_AddText (cmd+1);	// skip the slash
		else
			Cbuf_AddText (cmd);		// valid command
		Cbuf_AddText ("\n");

	// copy line to history buffer
		if (cmd[0])
			consoleHistory.AddToHistory (cmd);

		consoleField.Clear ();
		consoleField.SetWidthInChars (con.linewidth);

		if (cls.state == ca_disconnected)
			SCR_UpdateScreen ();	// force an update, because the command
						// may take some time
		return;
	}

// command completion
	if (key == K_TAB)
	{
		consoleField.AutoComplete ();
		return;
	}

// command history, ctrl-p and ctrl-n unix style
	if ((key == K_UPARROW) ||
		(key == 'p' && keydown[K_CTRL]))
	{
		hist = consoleHistory.RetrieveFromHistory (true);
		if (hist[0])
			consoleField.SetBuffer (hist);
		return;
	}

	if ((key == K_DOWNARROW) ||
		(key == 'n' && keydown[K_CTRL]))
	{
		hist = consoleHistory.RetrieveFromHistory (false);
		if (hist[0])
			consoleField.SetBuffer (hist);
		else
			consoleField.Clear ();	// no more history, blank line again
		return;
	}

// console scrolling
	if (key == K_PGUP || key == K_KP_PGUP)
	{
		Con_PageUp ();
		con_lastKeyEvent = Sys_Milliseconds ();
		con_nextKeyEvent = CONSOLE_FIRSTREPEAT;
		return;
	}

	if (key == K_PGDN || key == K_KP_PGDN)
	{
		Con_PageDown ();
		con_lastKeyEvent = Sys_Milliseconds ();
		con_nextKeyEvent = CONSOLE_FIRSTREPEAT;
		return;
	}

	if (key == K_MWHEELUP)
	{
		Con_PageUp ();
		return;
	}

	if (key == K_MWHEELDOWN)
	{
		Con_PageDown ();
		return;
	}

// ctrl-home = top of console, ctrl-end = bottom of console
	if ((key == K_HOME || key == K_KP_HOME) && keydown[K_CTRL])
	{
		con.display = 0;
		return;
	}

	if ((key == K_END || key == K_KP_END) && keydown[K_CTRL])
	{
		con.display = con.current;
		return;
	}

// backspace deletes through the field CharEvent, which treats it
// as ctrl-h like the BFG char events do
	if (key == K_BACKSPACE)
	{
		consoleField.CharEvent (K_BACKSPACE);
		return;
	}

// BFG delivers ctrl+letter as control codes through SE_CHAR, the Q2
// event pump never does, so the remaining console field combos are
// sent to the field as control codes here
	if (keydown[K_CTRL] && key >= 'a' && key <= 'z')
	{
		if (key == 'c')
			consoleField.Clear ();	// ctrl-c clears the field
		else
			consoleField.CharEvent (key - 'a' + 1);
		return;
	}

	if (key >= 32 && key <= 127)
		consoleField.CharEvent (key);
	else
	// pass to the normal editline routine
		consoleField.KeyDownEvent (key, keydown[K_CTRL], keydown[K_SHIFT]);
}

/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
Con_DrawInput

The input line scrolls horizontally if typing goes beyond the right edge
================
*/
static void Con_DrawFieldChar (int x, int y, int c)
{
	re.DrawChar (x, y, c);
}

void Con_DrawInput (void)
{
	if (cls.key_dest == key_menu)
		return;
	if (cls.key_dest != key_console && cls.state == ca_active)
		return;	// don't draw anything (always draw if not active)

// the ] prompt, then the field with its blinking cursor
	re.DrawChar (8, con.vislines - 22, ']');

	consoleField.Draw (16, con.vislines - 22,
		!((int)(cls.realtime>>8)&1), Con_DrawFieldChar);
}

/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify (void)
{
	int		x, v;
	char	*text;
	int		i;
	int		time;
	char	*s;
	int		skip;

	v = 0;
	for (i= con.current-NUM_CON_TIMES+1 ; i<=con.current ; i++)
	{
		if (i < 0)
			continue;
		time = con.times[i % NUM_CON_TIMES];
		if (time == 0)
			continue;
		time = cls.realtime - time;
		if (time > con_notifytime->value*1000)
			continue;
		text = con.text + (i % con.totallines)*con.linewidth;
		
		for (x = 0 ; x < con.linewidth ; x++)
			re.DrawChar ( (x+1)<<3, v, text[x]);

		v += 8;
	}


	if (cls.key_dest == key_message)
	{
		if (chat_team)
		{
			DrawString (8, v, "say_team:");
			skip = 11;
		}
		else
		{
			DrawString (8, v, "say:");
			skip = 5;
		}

		s = chat_buffer;
		if (chat_bufferlen > (viddef.width>>3)-(skip+1))
			s += chat_bufferlen - ((viddef.width>>3)-(skip+1));
		x = 0;
		while(s[x])
		{
			re.DrawChar ( (x+skip)<<3, v, s[x]);
			x++;
		}
		re.DrawChar ( (x+skip)<<3, v, 10+((cls.realtime>>8)&1));
		v += 8;
	}
	
	if (v)
	{
		SCR_AddDirtyPoint (0,0);
		SCR_AddDirtyPoint (viddef.width-1, v);
	}
}

/*
================
Con_DrawConsole

Draws the console with the solid background
================
*/
void Con_DrawConsole (float frac)
{
	int				i, j, x, y, n;
	int				rows;
	char			*text;
	int				row;
	int				lines;
	char			version[64];
	char			dlbar[1024];

	Con_Scroll ();

	lines = viddef.height * frac;
	if (lines <= 0)
		return;

	if (lines > viddef.height)
		lines = viddef.height;

// draw the background
	re.DrawStretchPic (0, -viddef.height+lines, viddef.width, viddef.height, "conback");
	SCR_AddDirtyPoint (0,0);
	SCR_AddDirtyPoint (viddef.width-1,lines-1);

	Com_sprintf (version, sizeof(version), "v%4.2f", VERSION);
	for (x=0 ; x<5 ; x++)
		re.DrawChar (viddef.width-44+x*8, lines-12, 128 + version[x] );

// draw the text
	con.vislines = lines;
	
#if 0
	rows = (lines-8)>>3;		// rows of text to draw

	y = lines - 24;
#else
	rows = (lines-22)>>3;		// rows of text to draw

	y = lines - 30;
#endif

// draw from the bottom up
	if (con.display != con.current)
	{
	// draw arrows to show the buffer is backscrolled
		for (x=0 ; x<con.linewidth ; x+=4)
			re.DrawChar ( (x+1)<<3, y, '^');
	
		y -= 8;
		rows--;
	}
	
	row = con.display;
	for (i=0 ; i<rows ; i++, y-=8, row--)
	{
		if (row < 0)
			break;
		if (con.current - row >= con.totallines)
			break;		// past scrollback wrap point
			
		text = con.text + (row % con.totallines)*con.linewidth;

		for (x=0 ; x<con.linewidth ; x++)
			re.DrawChar ( (x+1)<<3, y, text[x]);
	}

//ZOID
	// draw the download bar
	// figure out width
	if (cls.download) {
		if ((text = strrchr(cls.downloadname, '/')) != NULL)
			text++;
		else
			text = cls.downloadname;

		x = con.linewidth - ((con.linewidth * 7) / 40);
		y = x - strlen(text) - 8;
		i = con.linewidth/3;
		if (strlen(text) > i) {
			y = x - i - 11;
			strncpy(dlbar, text, i);
			dlbar[i] = 0;
			strcat(dlbar, "...");
		} else
			strcpy(dlbar, text);
		strcat(dlbar, ": ");
		i = strlen(dlbar);
		dlbar[i++] = '\x80';
		// where's the dot go?
		if (cls.downloadpercent == 0)
			n = 0;
		else
			n = y * cls.downloadpercent / 100;
			
		for (j = 0; j < y; j++)
			if (j == n)
				dlbar[i++] = '\x83';
			else
				dlbar[i++] = '\x81';
		dlbar[i++] = '\x82';
		dlbar[i] = 0;

		sprintf(dlbar + strlen(dlbar), " %02d%%", cls.downloadpercent);

		// draw it
		y = con.vislines-12;
		for (i = 0; i < strlen(dlbar); i++)
			re.DrawChar ( (i+1)<<3, y, dlbar[i]);
	}
//ZOID

// draw the input prompt, user text, and cursor if desired
	Con_DrawInput ();
}

/*
==============================================================================

DEBUG GRAPHS

==============================================================================
*/

// Port of the BFG console debug graphs (idConsoleLocal::CreateGraph /
// DestroyGraph and its debugGraphs list): graphs created here render on
// top of the screen every frame, like the BFG console overlays.

#define CON_MAX_DEBUG_GRAPHS 4

static DN_DebugGraph *con_debugGraphs[CON_MAX_DEBUG_GRAPHS];
static qboolean con_debugGraphUsed[CON_MAX_DEBUG_GRAPHS];

// the built-in consumer: rolling frame times in milliseconds
static DN_DebugGraph *con_frameTimesGraph;

// 100 pixels of graph height cover 100 milliseconds
#define CON_FRAMETIME_SCALE 100.0f

DN_DebugGraph *Con_CreateGraph (void)
{
	int i;

	for (i = 0; i < CON_MAX_DEBUG_GRAPHS; i++)
	{
		if (!con_debugGraphUsed[i])
		{
			con_debugGraphUsed[i] = true;
			con_debugGraphs[i] = new DN_DebugGraph (0);
			return con_debugGraphs[i];
		}
	}
	return NULL;
}

void Con_DestroyGraph (DN_DebugGraph *graph)
{
	int i;

	for (i = 0; i < CON_MAX_DEBUG_GRAPHS; i++)
	{
		if (con_debugGraphUsed[i] && con_debugGraphs[i] == graph)
		{
			delete con_debugGraphs[i];
			con_debugGraphs[i] = NULL;
			con_debugGraphUsed[i] = false;
			return;
		}
	}
}

void Con_DrawDebugGraphs (void)
{
	int i;

	if (con_frameTimes->value)
	{
		if (!con_frameTimesGraph)
		{
			con_frameTimesGraph = Con_CreateGraph ();
			if (con_frameTimesGraph)
			{
				con_frameTimesGraph->SetPosition (viddef.width - 261, 100, 250, 100);
				con_frameTimesGraph->SetBorder (1);
				con_frameTimesGraph->SetBackgroundColor (8);
				con_frameTimesGraph->SetLabelColor (0);
				// one grid line at the frame delta clamp
				con_frameTimesGraph->AddGridLine (com_deltaTimeClamp->value / CON_FRAMETIME_SCALE, 0x40);
				con_frameTimesGraph->SetFillMode (DN_DebugGraph::GRAPH_FILL);
			}
		}
		if (con_frameTimesGraph)
		{
			// cls.frametime is seconds; the bar is the covered fraction of
			// the height, over-budget frames turn bright
			float msec = cls.frametime * 1000.0f;
			con_frameTimesGraph->SetValue (-1, msec / CON_FRAMETIME_SCALE,
				msec > com_deltaTimeClamp->value ? 0xdf : 0xd0);
		}
	}
	else if (con_frameTimesGraph)
	{
		Con_DestroyGraph (con_frameTimesGraph);
		con_frameTimesGraph = NULL;
	}

	for (i = 0; i < CON_MAX_DEBUG_GRAPHS; i++)
	{
		if (con_debugGraphUsed[i])
		{
			con_debugGraphs[i]->Render ();
		}
	}
}



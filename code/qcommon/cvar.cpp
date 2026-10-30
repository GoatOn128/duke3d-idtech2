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
// cvar.cpp -- dynamic variable tracking

#include "qcommon.h"
#include "CVar.h"

cvar_t	*cvar_vars;

/*
============
Cvar_InfoValidate
============
*/
static qboolean Cvar_InfoValidate (char *s)
{
	if (strstr (s, "\\"))
		return false;
	if (strstr (s, "\""))
		return false;
	if (strstr (s, ";"))
		return false;
	return true;
}

/*
============
Cvar_FindVar
============
*/
static cvar_t *Cvar_FindVar (char *var_name)
{
	cvar_t	*var;
	
	for (var=cvar_vars ; var ; var=var->next)
		if (!strcmp (var_name, var->name))
			return var;

	return NULL;
}

/*
============
Cvar_VariableValue
============
*/
float Cvar_VariableValue (char *var_name)
{
	cvar_t	*var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
		return 0;
	return atof (var->string);
}


/*
============
Cvar_VariableString
============
*/
char *Cvar_VariableString (char *var_name)
{
	cvar_t *var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
		return "";
	return var->string;
}


/*
============
Cvar_CompleteVariable
============
*/
char *Cvar_CompleteVariable (char *partial)
{
	cvar_t		*cvar;
	int			len;
	
	len = strlen(partial);
	
	if (!len)
		return NULL;
		
	// check exact match
	for (cvar=cvar_vars ; cvar ; cvar=cvar->next)
		if (!strcmp (partial,cvar->name))
			return cvar->name;

	// check partial match
	for (cvar=cvar_vars ; cvar ; cvar=cvar->next)
		if (!strncmp (partial,cvar->name, len))
			return cvar->name;

	return NULL;
}


/*
============
Cvar_CommandCompletion

BFG style completion support: every cvar name is passed to the
callback, which does its own prefix matching.
============
*/
void Cvar_CommandCompletion(void (*callback)(const char *command))
{
	cvar_t		*cvar;

	for (cvar=cvar_vars ; cvar ; cvar=cvar->next)
		callback(cvar->name);
}

/*
============
CVar_NormalizeValue

idTech4 style post processing: clamps to the declared range and
reformats typed (CVAR_BOOL / CVAR_INTEGER) values. Legacy cvars
declared without a range or a type pass through unchanged.
============
*/
static void CVar_NormalizeValue (cvar_t *var)
{
	float	v;
	char	val[32];

	v = var->value;
	if (var->value_min != -1.0f && v < var->value_min)
		v = var->value_min;
	if (var->value_max != -1.0f && v > var->value_max)
		v = var->value_max;

	// typed cvars keep an integer form; clamped values are
	// reformatted so the stored string matches what is stored
	if (var->flags & (CVAR_BOOL | CVAR_INTEGER))
		Com_sprintf (val, sizeof(val), "%i", (int)v);
	else if (v != var->value)
	{
		if (v == (int)v)
			Com_sprintf (val, sizeof(val), "%i", (int)v);
		else
			Com_sprintf (val, sizeof(val), "%f", v);
	}
	else
		return;

	if (!strcmp (val, var->string))
		return;

	Z_Free (var->string);
	var->string = CopyString (val);
	var->value = atof (val);
}


/*
============
Cvar_Get

If the variable already exists, the value will not be set
The flags will be or'ed in if the variable exists.
============
*/
cvar_t *Cvar_Get (char *var_name, char *var_value, int flags)
{
	cvar_t	*var;
	
	if (flags & (CVAR_USERINFO | CVAR_SERVERINFO))
	{
		if (!Cvar_InfoValidate (var_name))
		{
			Com_Printf("invalid info cvar name\n");
			return NULL;
		}
	}

	var = Cvar_FindVar (var_name);
	if (var)
	{
		var->flags |= flags;
		return var;
	}

	if (!var_value)
		return NULL;

	if (flags & (CVAR_USERINFO | CVAR_SERVERINFO))
	{
		if (!Cvar_InfoValidate (var_value))
		{
			Com_Printf("invalid info cvar value\n");
			return NULL;
		}
	}

	var = (cvar_t *)(Z_Malloc (sizeof(*var)));
	var->name = CopyString (var_name);
	var->string = CopyString (var_value);
	// idTech4: remember the default for reset and cvar_restart
	var->reset_string = CopyString (var_value);
	var->value_min = var->value_max = -1.0f;
	var->modified = true;
	var->value = atof (var->string);

	// link the variable in
	var->next = cvar_vars;
	cvar_vars = var;

	var->flags = flags;

	return var;
}

/*
============
Cvar_Set2
============
*/
cvar_t *Cvar_Set2 (char *var_name, char *value, qboolean force)
{
	cvar_t	*var;

	var = Cvar_FindVar (var_name);
	if (!var)
	{	// create it
		return Cvar_Get (var_name, value, 0);
	}

	if (var->flags & (CVAR_USERINFO | CVAR_SERVERINFO))
	{
		if (!Cvar_InfoValidate (value))
		{
			Com_Printf("invalid info cvar value\n");
			return var;
		}
	}

	if (!force)
	{
		if ((var->flags & CVAR_CHEAT) && !CVar_CheatsAllowed())
		{
			Com_Printf ("%s is cheat protected. Set allow_cheats 1 to change it.\n", var_name);
			return var;
		}

		if (var->flags & CVAR_NOSET)
		{
			Com_Printf ("%s is write protected.\n", var_name);
			return var;
		}

		if (var->flags & CVAR_LATCH)
		{
			if (var->latched_string)
			{
				if (strcmp(value, var->latched_string) == 0)
					return var;
				Z_Free (var->latched_string);
			}
			else
			{
				if (strcmp(value, var->string) == 0)
					return var;
			}

			if (Com_ServerState())
			{
				Com_Printf ("%s will be changed for next game.\n", var_name);
				var->latched_string = CopyString(value);
			}
			else
			{
				var->string = CopyString(value);
				var->value = atof (var->string);
				// idTech4: enforce the declared range and type
				CVar_NormalizeValue (var);
				if (!strcmp(var->name, "game"))
				{
					FS_SetGamedir (var->string);
					FS_ExecAutoexec ();
				}
			}
			return var;
		}
	}
	else
	{
		if (var->latched_string)
		{
			Z_Free (var->latched_string);
			var->latched_string = NULL;
		}
	}

	if (!strcmp(value, var->string))
		return var;		// not changed

	var->modified = true;

	if (var->flags & CVAR_USERINFO)
		userinfo_modified = true;	// transmit at next oportunity
	
	Z_Free (var->string);	// free the old value string
	
	var->string = CopyString(value);
	var->value = atof (var->string);

	// idTech4: enforce the declared range and type
	CVar_NormalizeValue (var);

	return var;
}

/*
============
Cvar_ForceSet
============
*/
cvar_t *Cvar_ForceSet (char *var_name, char *value)
{
	return Cvar_Set2 (var_name, value, true);
}

/*
============
Cvar_Set
============
*/
cvar_t *Cvar_Set (char *var_name, char *value)
{
	return Cvar_Set2 (var_name, value, false);
}

/*
============
Cvar_FullSet
============
*/
cvar_t *Cvar_FullSet (char *var_name, char *value, int flags)
{
	cvar_t	*var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
	{	// create it
		return Cvar_Get (var_name, value, flags);
	}

	var->modified = true;

	if (var->flags & CVAR_USERINFO)
		userinfo_modified = true;	// transmit at next oportunity
	
	Z_Free (var->string);	// free the old value string
	
	var->string = CopyString(value);
	var->value = atof (var->string);
	var->flags = flags;

	// idTech4: enforce the declared range and type
	CVar_NormalizeValue (var);

	return var;
}

/*
============
Cvar_SetValue
============
*/
void Cvar_SetValue (char *var_name, float value)
{
	char	val[32];

	if (value == (int)value)
		Com_sprintf (val, sizeof(val), "%i",(int)value);
	else
		Com_sprintf (val, sizeof(val), "%f",value);
	Cvar_Set (var_name, val);
}


/*
============
Cvar_GetLatchedVars

Any variables with latched values will now be updated
============
*/
void Cvar_GetLatchedVars (void)
{
	cvar_t	*var;

	for (var = cvar_vars ; var ; var = var->next)
	{
		if (!var->latched_string)
			continue;
		Z_Free (var->string);
		var->string = var->latched_string;
		var->latched_string = NULL;
		var->value = atof(var->string);
		// idTech4: enforce the declared range and type
		CVar_NormalizeValue (var);
		if (!strcmp(var->name, "game"))
		{
			FS_SetGamedir (var->string);
			FS_ExecAutoexec ();
		}
	}
}

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
qboolean Cvar_Command (void)
{
	cvar_t			*v;

// check variables
	v = Cvar_FindVar (Cmd_Argv(0));
	if (!v)
		return false;
		
// perform a variable print or set
	if (Cmd_Argc() == 1)
	{
		Com_Printf ("\"%s\" is \"%s\"\n", v->name, v->string);
		return true;
	}

	Cvar_Set (v->name, Cmd_Argv(1));
	return true;
}


/*
============
Cvar_Set_f

Allows setting and defining of arbitrary cvars from console
============
*/
void Cvar_Set_f (void)
{
	int		c;
	int		flags;

	c = Cmd_Argc();
	if (c != 3 && c != 4)
	{
		Com_Printf ("usage: set <variable> <value> [u / s]\n");
		return;
	}

	if (c == 4)
	{
		if (!strcmp(Cmd_Argv(3), "u"))
			flags = CVAR_USERINFO;
		else if (!strcmp(Cmd_Argv(3), "s"))
			flags = CVAR_SERVERINFO;
		else
		{
			Com_Printf ("flags can only be 'u' or 's'\n");
			return;
		}
		Cvar_FullSet (Cmd_Argv(1), Cmd_Argv(2), flags);
	}
	else
		Cvar_Set (Cmd_Argv(1), Cmd_Argv(2));
}


/*
============
Cvar_SetFlagged_f

The idTech4 convenience setters: seta also archives the cvar, setu
and sets also flag it as user resp. server info
============
*/
static void Cvar_SetFlagged_f (int flags)
{
	cvar_t	*var;

	if (Cmd_Argc() != 3)
	{
		Com_Printf ("usage: %s <variable> <value>\n", Cmd_Argv(0));
		return;
	}

	// keep the flags the cvar was registered with
	var = Cvar_FindVar (Cmd_Argv(1));
	if (var)
		flags |= var->flags;

	Cvar_FullSet (Cmd_Argv(1), Cmd_Argv(2), flags);
}

static void Cvar_Seta_f (void)
{
	Cvar_SetFlagged_f (CVAR_ARCHIVE);
}

static void Cvar_Setu_f (void)
{
	Cvar_SetFlagged_f (CVAR_USERINFO);
}

static void Cvar_Sets_f (void)
{
	Cvar_SetFlagged_f (CVAR_SERVERINFO);
}

/*
============
Cvar_Reset_f

Restores a cvar to its default value (idTech4 reset)
============
*/
static void Cvar_Reset_f (void)
{
	cvar_t	*var;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("usage: reset <variable>\n");
		return;
	}

	var = Cvar_FindVar (Cmd_Argv(1));
	if (!var)
	{
		Com_Printf ("reset: %s is not a cvar\n", Cmd_Argv(1));
		return;
	}
	if (!var->reset_string)
	{
		Com_Printf ("reset: %s has no default\n", var->name);
		return;
	}

	Cvar_ForceSet (var->name, var->reset_string);
}

/*
============
Cvar_Toggle_f

idTech4 toggle: cycles a cvar through the given values, or flips
between 0 and 1 when no values are given
============
*/
static void Cvar_Toggle_f (void)
{
	cvar_t	*var;
	int		c, i;

	c = Cmd_Argc();
	if (c < 2)
	{
		Com_Printf ("usage: toggle <variable> [value ...]\n");
		return;
	}

	var = Cvar_FindVar (Cmd_Argv(1));
	if (!var)
	{
		Com_Printf ("toggle: %s is not a cvar\n", Cmd_Argv(1));
		return;
	}

	if (c == 2)
	{
		Cvar_Set (var->name, var->value != 0.0f ? "0" : "1");
		return;
	}

	// find the current value in the list, then set the next one
	for (i = 2; i < c; i++)
		if (!strcmp (Cmd_Argv(i), var->string))
			break;
	i++;
	if (i >= c)
		i = 2;
	Cvar_Set (var->name, Cmd_Argv(i));
}

/*
============
Cvar_Restart_f

Resets every cvar to its default value (idTech4 cvar_restart)
============
*/
static void Cvar_Restart_f (void)
{
	cvar_t	*var;

	for (var = cvar_vars; var; var = var->next)
	{
		if (var->reset_string && strcmp (var->reset_string, var->string))
			Cvar_ForceSet (var->name, var->reset_string);
	}
	Com_Printf ("all cvars reset to their defaults\n");
}

/*
============
Cvar_MatchFilter

Case insensitive wildcard match (* and ?) for the cvarlist filter
============
*/
static qboolean Cvar_MatchFilter (const char *filter, const char *name)
{
	const char	*f = filter;
	const char	*n = name;
	const char	*starFilter = NULL;
	const char	*starName = NULL;

	while (*n)
	{
		if (*f == '?' || tolower((unsigned char)*f) == tolower((unsigned char)*n))
		{
			f++;
			n++;
		}
		else if (*f == '*')
		{
			starFilter = ++f;
			starName = n;
		}
		else if (starFilter)
		{
			f = starFilter;
			n = ++starName;
		}
		else
			return false;
	}
	while (*f == '*')
		f++;
	return *f == 0;
}

/*
============
Cvar_WriteVariables

Appends lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
void Cvar_WriteVariables (char *path)
{
	cvar_t	*var;
	char	buffer[1024];
	FILE	*f;

	f = fopen (path, "a");
	for (var = cvar_vars ; var ; var = var->next)
	{
		if (var->flags & CVAR_ARCHIVE)
		{
			Com_sprintf (buffer, sizeof(buffer), "seta %s \"%s\"\n", var->name, var->string);
			fprintf (f, "%s", buffer);
		}
	}
	fclose (f);
}

/*
============
Cvar_List_f

============
*/
void Cvar_List_f (void)
{
	cvar_t	*var;
	int		i, total;
	char	*filter;

	if (Cmd_Argc() > 2)
	{
		Com_Printf ("usage: cvarlist [filter]\n");
		return;
	}
	filter = (Cmd_Argc() == 2) ? Cmd_Argv(1) : NULL;

	Com_Printf ("* archive  U userinfo  S serverinfo  C cheat  L latch  - noset\n");
	Com_Printf ("I integer  B boolean\n\n");

	i = 0;
	total = 0;
	for (var = cvar_vars ; var ; var = var->next)
	{
		total++;
		if (filter && !Cvar_MatchFilter (filter, var->name))
			continue;

		Com_Printf ("%s", (var->flags & CVAR_ARCHIVE) ? "*" : " ");
		Com_Printf ("%s", (var->flags & CVAR_USERINFO) ? "U" : " ");
		Com_Printf ("%s", (var->flags & CVAR_SERVERINFO) ? "S" : " ");
		Com_Printf ("%s", (var->flags & CVAR_CHEAT) ? "C" : " ");
		if (var->flags & CVAR_NOSET)
			Com_Printf ("-");
		else
			Com_Printf ("%s", (var->flags & CVAR_LATCH) ? "L" : " ");
		Com_Printf ("%s", (var->flags & CVAR_INTEGER) ? "I" : " ");
		Com_Printf ("%s", (var->flags & CVAR_BOOL) ? "B" : " ");
		Com_Printf (" %s \"%s\"", var->name, var->string);
		if (var->value_min != -1.0f || var->value_max != -1.0f)
			Com_Printf (" [%g, %g]", var->value_min, var->value_max);
		if (var->reset_string && strcmp (var->reset_string, var->string))
			Com_Printf ("  default: \"%s\"", var->reset_string);
		Com_Printf ("\n");
		i++;
	}
	Com_Printf ("%i of %i cvars\n", i, total);
}


qboolean userinfo_modified;


char	*Cvar_BitInfo (int bit)
{
	static char	info[MAX_INFO_STRING];
	cvar_t	*var;

	info[0] = 0;

	for (var = cvar_vars ; var ; var = var->next)
	{
		if (var->flags & bit)
			Info_SetValueForKey (info, var->name, var->string);
	}
	return info;
}

// returns an info string containing all the CVAR_USERINFO cvars
char	*Cvar_Userinfo (void)
{
	return Cvar_BitInfo (CVAR_USERINFO);
}

// returns an info string containing all the CVAR_SERVERINFO cvars
char	*Cvar_Serverinfo (void)
{
	return Cvar_BitInfo (CVAR_SERVERINFO);
}

/*
============
Cvar_Init

Reads in all archived cvars
============
*/
void Cvar_Init (void)
{
	Cmd_AddCommand ("set", Cvar_Set_f);
	Cmd_AddCommand ("seta", Cvar_Seta_f);
	Cmd_AddCommand ("setu", Cvar_Setu_f);
	Cmd_AddCommand ("sets", Cvar_Sets_f);
	Cmd_AddCommand ("cvarlist", Cvar_List_f);
	Cmd_AddCommand ("reset", Cvar_Reset_f);
	Cmd_AddCommand ("toggle", Cvar_Toggle_f);
	Cmd_AddCommand ("cvar_restart", Cvar_Restart_f);

	// the statically declared idCVar objects register here
	idCVar::RegisterStaticVars();
}

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

// CVar.cpp -- idTech4 style console variable objects

#include "qcommon.h"
#include "CVar.h"

idCVar *idCVar::staticVars = NULL;

// the cheat gate for CVAR_CHEAT variables: the idTech4 cvar system
// refuses to change cheat cvars unless cheats are allowed, and here
// the switch is itself a cvar, declared through the class it guards
static idCVar allow_cheats("allow_cheats", "0", CVAR_BOOL, "allow changing CVAR_CHEAT cvars");

idCVar::idCVar(const char *cvarName, const char *cvarValue, int cvarFlags,
	const char *cvarDescription, float min, float max)
{
	name = cvarName;
	value = cvarValue;
	flags = cvarFlags;
	description = cvarDescription;
	valueMin = min;
	valueMax = max;
	var = NULL;

	// link into the per-module list; registration happens in Cvar_Init
	next = staticVars;
	staticVars = this;
}

void idCVar::RegisterStaticVars(void)
{
	cvar_t	*v;

	for (idCVar *c = staticVars; c; c = c->next)
	{
		v = Cvar_Get((char *)c->name, (char *)c->value, c->flags);
		if (!v)
			continue;
		c->var = v;
		v->value_min = c->valueMin;
		v->value_max = c->valueMax;
	}
}

const char *idCVar::GetString(void) const
{
	if (var)
		return var->string;
	return value;
}

float idCVar::GetFloat(void) const
{
	if (var)
		return var->value;
	return atof(value);
}

int idCVar::GetInteger(void) const
{
	if (var)
		return (int)var->value;
	return (int)atof(value);
}

qboolean idCVar::GetBool(void) const
{
	return GetInteger() != 0;
}

void idCVar::SetString(const char *text) const
{
	Cvar_Set((char *)name, (char *)text);
}

void idCVar::SetFloat(float f) const
{
	char	buf[32];

	// whole numbers stay clean, like Cvar_SetValue
	if (f == (int)f)
		Com_sprintf(buf, sizeof(buf), "%i", (int)f);
	else
		Com_sprintf(buf, sizeof(buf), "%f", f);
	Cvar_Set((char *)name, buf);
}

void idCVar::SetInteger(int i) const
{
	char	buf[32];

	Com_sprintf(buf, sizeof(buf), "%i", i);
	Cvar_Set((char *)name, buf);
}

void idCVar::SetBool(qboolean b) const
{
	Cvar_Set((char *)name, b ? (char *)"1" : (char *)"0");
}

const char *idCVar::GetResetString(void) const
{
	if (var && var->reset_string)
		return var->reset_string;
	return value;
}

int idCVar::GetFlags(void) const
{
	if (var)
		return var->flags;
	return flags;
}

void idCVar::SetFlags(int newFlags) const
{
	if (var)
		var->flags = newFlags;
}

void idCVar::Reset(void) const
{
	if (var && var->reset_string)
		Cvar_ForceSet(var->name, var->reset_string);
}

// the central CVAR_CHEAT gate, shared with Cvar_Set2 in cvar.cpp
qboolean CVar_CheatsAllowed(void)
{
	if (allow_cheats.GetVar())
		return allow_cheats.GetVar()->value != 0.0f;
	return false;
}

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

// CVar.h -- idTech4 style console variable objects.
//
// A cvar may be declared as a statically constructed idCVar instead of
// fetched through Cvar_Get at init: the constructor links the instance
// into a per-module list, Cvar_Init registers every one of them with
// the cvar registry, and the typed accessors run through the same
// Cvar_*() paths the rest of the engine uses. This follows the idTech4
// cvar system layout (typed values, declared ranges, cheat protection);
// it is written from scratch for this codebase, no idTech4 code is
// included.
//
// include qcommon.h first: this header uses cvar_t and qboolean.

#ifndef DN_CVAR_H
#define DN_CVAR_H

class idCVar
{
public:
	idCVar(const char *name, const char *value, int flags,
		const char *description = "",
		float valueMin = -1.0f, float valueMax = -1.0f);

	// typed read access
	const char *GetString(void) const;
	float GetFloat(void) const;
	int GetInteger(void) const;
	qboolean GetBool(void) const;

	// typed write access; range clamps and the cheat gate run inside
	// the central Cvar_Set2 path
	void SetString(const char *value) const;
	void SetFloat(float value) const;
	void SetInteger(int value) const;
	void SetBool(qboolean value) const;

	const char *GetName(void) const { return name; }
	const char *GetDescription(void) const { return description; }
	const char *GetResetString(void) const;
	int GetFlags(void) const;
	void SetFlags(int flags) const;
	float GetMinValue(void) const { return valueMin; }
	float GetMaxValue(void) const { return valueMax; }

	// restore the default value
	void Reset(void) const;

	// the backing registry entry, NULL until RegisterStaticVars ran
	cvar_t *GetVar(void) const { return var; }

	// registers every statically constructed idCVar with the cvar
	// registry; called once from Cvar_Init
	static void RegisterStaticVars(void);

private:
	const char	*name;
	const char	*value;
	int			flags;
	const char	*description;
	float		valueMin;
	float		valueMax;
	cvar_t		*var;
	idCVar		*next;		// the per-module static list

	static idCVar *staticVars;
};

#endif // DN_CVAR_H

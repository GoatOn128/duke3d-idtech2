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
// sdl_input.cpp -- SDL3 mouse and joystick code, ported from in_win.cpp.
// Mouse look uses SDL's relative mouse mode instead of cursor clipping.

#include "../client/client.h"
#include "sdl_sys.h"

#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_joystick.h>

// joystick defines and variables
#define JOY_ABSOLUTE_AXIS 0x00000000 // control like a joystick
#define JOY_RELATIVE_AXIS 0x00000010 // control like a mouse, spinner, trackball

#define JOY_AXIS_X 0
#define JOY_AXIS_Y 1
#define JOY_AXIS_Z 2
#define JOY_AXIS_R 3
#define JOY_AXIS_U 4
#define JOY_AXIS_V 5
#define JOY_MAX_AXES 6               // X, Y, Z, R, U, V

enum _ControlList
{
    AxisNada = 0,
    AxisForward,
    AxisLook,
    AxisSide,
    AxisTurn,
    AxisUp
};

int dwAxisMap[JOY_MAX_AXES];
int dwControlMap[JOY_MAX_AXES];
static float fRawValue[JOY_MAX_AXES];

cvar_t *in_mouse;
cvar_t *in_joystick;

// none of these cvars are saved over a session
// this means that advanced controller configuration needs to be executed
// each time.  this avoids any problems with getting back to a default usage
// or when changing from one controller to another.  this way at least something
// works.
cvar_t *joy_name;
cvar_t *joy_advanced;
cvar_t *joy_advaxisx;
cvar_t *joy_advaxisy;
cvar_t *joy_advaxisz;
cvar_t *joy_advaxisr;
cvar_t *joy_advaxisu;
cvar_t *joy_advaxisv;
cvar_t *joy_forwardthreshold;
cvar_t *joy_sidethreshold;
cvar_t *joy_pitchthreshold;
cvar_t *joy_yawthreshold;
cvar_t *joy_forwardsensitivity;
cvar_t *joy_sidesensitivity;
cvar_t *joy_pitchsensitivity;
cvar_t *joy_yawsensitivity;
cvar_t *joy_upthreshold;
cvar_t *joy_upsensitivity;

qboolean joy_avail, joy_advancedinit, joy_haspov;
unsigned joy_oldbuttonstate, joy_oldpovstate;

int joy_numbuttons;

static SDL_Joystick *joystick;

qboolean in_appactive;

// forward-referenced functions
void IN_StartupJoystick(void);
void Joy_AdvancedUpdate_f(void);
void IN_JoyMove(usercmd_t *cmd);

/*
============================================================

  MOUSE CONTROL

============================================================
*/

// mouse variables
cvar_t *m_filter;

qboolean mlooking;

void IN_MLookDown(void)
{
    mlooking = true;
}

void IN_MLookUp(void)
{
    mlooking = false;
    if (!freelook->value && lookspring->value)
    {
        IN_CenterView();
    }
}

int mouse_buttons;
int mouse_oldbuttonstate;
int mouse_x, mouse_y, old_mouse_x, old_mouse_y;
int mx_accum, my_accum;

qboolean mouseactive; // false when not focus app

qboolean mouseinitialized;

/*
===========
IN_ActivateMouse

Called when the window gains focus or changes in some way
===========
*/
void IN_ActivateMouse(void)
{
    SDL_Window *window;

    if (!mouseinitialized)
    {
        return;
    }
    if (!in_mouse->value)
    {
        mouseactive = false;
        return;
    }
    if (mouseactive)
    {
        return;
    }

    mouseactive = true;

    window = Sys_GameWindow();
    if (window)
    {
        SDL_SetWindowRelativeMouseMode(window, true);
    }
    SDL_HideCursor();
}

/*
===========
IN_DeactivateMouse

Called when the window loses focus
===========
*/
void IN_DeactivateMouse(void)
{
    SDL_Window *window;

    if (!mouseinitialized)
    {
        return;
    }
    if (!mouseactive)
    {
        return;
    }

    mouseactive = false;

    window = Sys_GameWindow();
    if (window)
    {
        SDL_SetWindowRelativeMouseMode(window, false);
    }
    SDL_ShowCursor();
}

/*
===========
IN_StartupMouse
===========
*/
void IN_StartupMouse(void)
{
    cvar_t *cv;

    cv = Cvar_Get("in_initmouse", "1", CVAR_NOSET);
    if (!cv->value)
    {
        return;
    }

    mouseinitialized = true;
    mouse_buttons = 3;
}

/*
===========
IN_MouseEvent
===========
*/
void IN_MouseEvent(int mstate)
{
    int i;

    if (!mouseinitialized)
    {
        return;
    }

    // perform button actions
    for (i = 0; i < mouse_buttons; i++)
    {
        if ((mstate & (1 << i)) && !(mouse_oldbuttonstate & (1 << i)))
        {
            Key_Event(K_MOUSE1 + i, true, Sys_Milliseconds());
        }

        if (!(mstate & (1 << i)) && (mouse_oldbuttonstate & (1 << i)))
        {
            Key_Event(K_MOUSE1 + i, false, Sys_Milliseconds());
        }
    }

    mouse_oldbuttonstate = mstate;
}

/*
===========
IN_MouseMove
===========
*/
void IN_MouseMove(usercmd_t *cmd)
{
    int mx, my;

    if (!mouseactive)
    {
        return;
    }

    // collect the relative deltas accumulated by the event pump
    mx = mx_accum;
    my = my_accum;
    mx_accum = 0;
    my_accum = 0;

    if (m_filter->value)
    {
        mouse_x = (mx + old_mouse_x) * 0.5;
        mouse_y = (my + old_mouse_y) * 0.5;
    }
    else
    {
        mouse_x = mx;
        mouse_y = my;
    }

    old_mouse_x = mx;
    old_mouse_y = my;

    mouse_x *= sensitivity->value;
    mouse_y *= sensitivity->value;

    IN_ApplyMouseDelta(cmd, mouse_x, mouse_y);
}

/* Apply relative motion separately from cursor capture so input direction
   can be verified through the real command/prediction path. */
void IN_ApplyMouseDelta(usercmd_t *cmd, float x, float y)
{
    if ((in_strafe.state & 1) || (lookstrafe->value && mlooking))
    {
        cmd->sidemove += m_side->value * x;
    }
    else
    {
        cl.viewangles[YAW] -= m_yaw->value * x;
    }

    if ((mlooking || freelook->value) && !(in_strafe.state & 1))
    {
        cl.viewangles[PITCH] += m_pitch->value * y;
    }
    else
    {
        cmd->forwardmove -= m_forward->value * y;
    }
}

/*
=========================================================================

VIEW CENTERING

=========================================================================
*/

cvar_t *v_centermove;
cvar_t *v_centerspeed;

/*
===========
IN_Init
===========
*/
void IN_Init(void)
{
    // mouse variables
    m_filter = Cvar_Get("m_filter", "0", 0);
    in_mouse = Cvar_Get("in_mouse", "1", CVAR_ARCHIVE);

    // joystick variables
    in_joystick = Cvar_Get("in_joystick", "0", CVAR_ARCHIVE);
    joy_name = Cvar_Get("joy_name", "joystick", 0);
    joy_advanced = Cvar_Get("joy_advanced", "0", 0);
    joy_advaxisx = Cvar_Get("joy_advaxisx", "0", 0);
    joy_advaxisy = Cvar_Get("joy_advaxisy", "0", 0);
    joy_advaxisz = Cvar_Get("joy_advaxisz", "0", 0);
    joy_advaxisr = Cvar_Get("joy_advaxisr", "0", 0);
    joy_advaxisu = Cvar_Get("joy_advaxisu", "0", 0);
    joy_advaxisv = Cvar_Get("joy_advaxisv", "0", 0);
    joy_forwardthreshold = Cvar_Get("joy_forwardthreshold", "0.15", 0);
    joy_sidethreshold = Cvar_Get("joy_sidethreshold", "0.15", 0);
    joy_upthreshold = Cvar_Get("joy_upthreshold", "0.15", 0);
    joy_pitchthreshold = Cvar_Get("joy_pitchthreshold", "0.15", 0);
    joy_yawthreshold = Cvar_Get("joy_yawthreshold", "0.15", 0);
    joy_forwardsensitivity = Cvar_Get("joy_forwardsensitivity", "-1", 0);
    joy_sidesensitivity = Cvar_Get("joy_sidesensitivity", "-1", 0);
    joy_upsensitivity = Cvar_Get("joy_upsensitivity", "-1", 0);
    joy_pitchsensitivity = Cvar_Get("joy_pitchsensitivity", "1", 0);
    joy_yawsensitivity = Cvar_Get("joy_yawsensitivity", "-1", 0);

    // centering
    v_centermove = Cvar_Get("v_centermove", "0.15", 0);
    v_centerspeed = Cvar_Get("v_centerspeed", "500", 0);

    Cmd_AddCommand("+mlook", IN_MLookDown);
    Cmd_AddCommand("-mlook", IN_MLookUp);

    Cmd_AddCommand("joy_advancedupdate", Joy_AdvancedUpdate_f);

    IN_StartupMouse();
    IN_StartupJoystick();
}

/*
===========
IN_Shutdown
===========
*/
void IN_Shutdown(void)
{
    IN_DeactivateMouse();

    if (joystick)
    {
        SDL_CloseJoystick(joystick);
        joystick = NULL;
    }
}

/*
===========
IN_Activate

Called when the main window gains or loses focus.
The window may have been destroyed and recreated
between a deactivate and an activate.
===========
*/
void IN_Activate(qboolean active)
{
    in_appactive = active;
    mouseactive = !active; // force a new window check or turn off
}

/*
==================
IN_Frame

Called every frame, even if not generating commands
==================
*/
void IN_Frame(void)
{
    if (!mouseinitialized)
    {
        return;
    }

    if (!in_mouse || !in_appactive)
    {
        IN_DeactivateMouse();
        return;
    }

    if (!cl.refresh_prepped || cls.key_dest == key_console || cls.key_dest == key_menu)
    {
        // temporarily deactivate if in windowed mode
        if (Cvar_VariableValue("vid_fullscreen") == 0)
        {
            IN_DeactivateMouse();
            return;
        }
    }

    IN_ActivateMouse();
}

/*
===========
IN_Move
===========
*/
void IN_Move(usercmd_t *cmd)
{
    IN_MouseMove(cmd);

    if (ActiveApp)
    {
        IN_JoyMove(cmd);
    }
}

/*
===================
IN_ClearStates
===================
*/
void IN_ClearStates(void)
{
    int i;

    // Menus call this while the mouse is out of use, and the client command
    // loop calls it every frame outside gameplay. Release buttons that are
    // still down so their up events are not lost; Key_Event would otherwise
    // count every later click as an autorepeat and swallow it.
    for (i = 0; i < mouse_buttons; i++)
    {
        if (mouse_oldbuttonstate & (1 << i))
        {
            Key_Event(K_MOUSE1 + i, false, 0);
        }
    }
    mx_accum = 0;
    my_accum = 0;
    mouse_oldbuttonstate = 0;
}

/*
=========================================================================

JOYSTICK

=========================================================================
*/

/*
===============
IN_StartupJoystick
===============
*/
void IN_StartupJoystick(void)
{
    int count = 0;
    SDL_JoystickID *ids;
    cvar_t *cv;

    // assume no joystick
    joy_avail = false;

    // abort startup if user requests no joystick
    cv = Cvar_Get("in_initjoy", "1", CVAR_NOSET);
    if (!cv->value)
    {
        return;
    }

    ids = SDL_GetJoysticks(&count);
    if (!ids || count == 0)
    {
        if (ids)
        {
            SDL_free(ids);
        }
        return;
    }

    joystick = SDL_OpenJoystick(ids[0]);
    SDL_free(ids);
    if (!joystick)
    {
        Com_Printf("\njoystick not found -- no valid joysticks (%s)\n\n", SDL_GetError());
        return;
    }

    // save the joystick's number of buttons and POV status
    joy_numbuttons = SDL_GetNumJoystickButtons(joystick);
    if (joy_numbuttons > 32)
    {
        joy_numbuttons = 32; // K_AUX1+i must stay inside the keynum range
    }
    joy_haspov = SDL_GetNumJoystickHats(joystick) > 0;

    // old button and POV states default to no buttons pressed
    joy_oldbuttonstate = joy_oldpovstate = 0;

    // mark the joystick as available and advanced initialization not completed
    // this is needed as cvars are not available during initialization
    joy_avail = true;
    joy_advancedinit = false;

    Com_Printf("\njoystick detected\n\n");
}

/*
===========
Joy_AdvancedUpdate_f
===========
*/
void Joy_AdvancedUpdate_f(void)
{
    // called once by IN_JoyMove and by user whenever an update is needed
    // cvars are now available
    int i;
    int dwTemp;

    // initialize all the maps
    for (i = 0; i < JOY_MAX_AXES; i++)
    {
        dwAxisMap[i] = AxisNada;
        dwControlMap[i] = JOY_ABSOLUTE_AXIS;
    }

    if (joy_advanced->value == 0.0)
    {
        // default joystick initialization
        // 2 axes only with joystick control
        dwAxisMap[JOY_AXIS_X] = AxisTurn;
        dwAxisMap[JOY_AXIS_Y] = AxisForward;
    }
    else
    {
        if (strcmp(joy_name->string, "joystick") != 0)
        {
            // notify user of advanced controller
            Com_Printf("\n%s configured\n\n", joy_name->string);
        }

        // advanced initialization here
        // data supplied by user via joy_axisn cvars
        dwTemp = (int)joy_advaxisx->value;
        dwAxisMap[JOY_AXIS_X] = dwTemp & 0x0000000f;
        dwControlMap[JOY_AXIS_X] = dwTemp & JOY_RELATIVE_AXIS;
        dwTemp = (int)joy_advaxisy->value;
        dwAxisMap[JOY_AXIS_Y] = dwTemp & 0x0000000f;
        dwControlMap[JOY_AXIS_Y] = dwTemp & JOY_RELATIVE_AXIS;
        dwTemp = (int)joy_advaxisz->value;
        dwAxisMap[JOY_AXIS_Z] = dwTemp & 0x0000000f;
        dwControlMap[JOY_AXIS_Z] = dwTemp & JOY_RELATIVE_AXIS;
        dwTemp = (int)joy_advaxisr->value;
        dwAxisMap[JOY_AXIS_R] = dwTemp & 0x0000000f;
        dwControlMap[JOY_AXIS_R] = dwTemp & JOY_RELATIVE_AXIS;
        dwTemp = (int)joy_advaxisu->value;
        dwAxisMap[JOY_AXIS_U] = dwTemp & 0x0000000f;
        dwControlMap[JOY_AXIS_U] = dwTemp & JOY_RELATIVE_AXIS;
        dwTemp = (int)joy_advaxisv->value;
        dwAxisMap[JOY_AXIS_V] = dwTemp & 0x0000000f;
        dwControlMap[JOY_AXIS_V] = dwTemp & JOY_RELATIVE_AXIS;
    }
}

/*
===========
IN_Commands
===========
*/
void IN_Commands(void)
{
    int i, key_index;
    unsigned buttonstate, povstate;
    Uint8 hat;

    if (!joy_avail)
    {
        return;
    }

    // loop through the joystick buttons
    // key a joystick event or auxillary event for higher number buttons for each state change
    buttonstate = 0;
    for (i = 0; i < joy_numbuttons; i++)
    {
        if (SDL_GetJoystickButton(joystick, i))
        {
            buttonstate |= 1 << i;
        }
    }
    for (i = 0; i < joy_numbuttons; i++)
    {
        if ((buttonstate & (1 << i)) && !(joy_oldbuttonstate & (1 << i)))
        {
            key_index = (i < 4) ? K_JOY1 : K_AUX1;
            Key_Event(key_index + i, true, 0);
        }

        if (!(buttonstate & (1 << i)) && (joy_oldbuttonstate & (1 << i)))
        {
            key_index = (i < 4) ? K_JOY1 : K_AUX1;
            Key_Event(key_index + i, false, 0);
        }
    }
    joy_oldbuttonstate = buttonstate;

    if (joy_haspov)
    {
        // SDL hats report bits for each direction, including diagonals
        hat = SDL_GetJoystickHat(joystick, 0);

        povstate = 0;
        if (hat & SDL_HAT_UP)
        {
            povstate |= 0x01;
        }
        if (hat & SDL_HAT_RIGHT)
        {
            povstate |= 0x02;
        }
        if (hat & SDL_HAT_DOWN)
        {
            povstate |= 0x04;
        }
        if (hat & SDL_HAT_LEFT)
        {
            povstate |= 0x08;
        }

        // determine which bits have changed and key an auxillary event for each change
        for (i = 0; i < 4; i++)
        {
            if ((povstate & (1 << i)) && !(joy_oldpovstate & (1 << i)))
            {
                Key_Event(K_AUX29 + i, true, 0);
            }

            if (!(povstate & (1 << i)) && (joy_oldpovstate & (1 << i)))
            {
                Key_Event(K_AUX29 + i, false, 0);
            }
        }
        joy_oldpovstate = povstate;
    }
}

/*
===============
IN_ReadJoystick
===============
*/
static qboolean IN_ReadJoystick(void)
{
    int i;

    if (!joystick)
    {
        return false;
    }

    // SDL axes are zero-centered; normalize to the same -1..1 range the
    // DirectInput code used
    for (i = 0; i < JOY_MAX_AXES; i++)
    {
        fRawValue[i] = SDL_GetJoystickAxis(joystick, i) / 32768.0f;
    }

    return true;
}

/*
===========
IN_JoyMove
===========
*/
void IN_JoyMove(usercmd_t *cmd)
{
    float speed, aspeed;
    float fAxisValue;
    int i;

    // complete initialization if first time in
    // this is needed as cvars are not available at initialization time
    if (joy_advancedinit != true)
    {
        Joy_AdvancedUpdate_f();
        joy_advancedinit = true;
    }

    // verify joystick is available and that the user wants to use it
    if (!joy_avail || !in_joystick->value)
    {
        return;
    }

    // collect the joystick data, if possible
    if (IN_ReadJoystick() != true)
    {
        return;
    }

    if ((in_speed.state & 1) ^ (int)cl_run->value)
    {
        speed = 2;
    }
    else
    {
        speed = 1;
    }
    aspeed = speed * cls.frametime;

    // loop through the axes
    for (i = 0; i < JOY_MAX_AXES; i++)
    {
        // get the floating point zero-centered, potentially-inverted data for the current axis
        fAxisValue = fRawValue[i];

        switch (dwAxisMap[i])
        {
        case AxisForward:
            if ((joy_advanced->value == 0.0) && mlooking)
            {
                // user wants forward control to become look control
                if (fabs(fAxisValue) > joy_pitchthreshold->value)
                {
                    // if mouse invert is on, invert the joystick pitch value
                    // only absolute control support here (joy_advanced is false)
                    if (m_pitch->value < 0.0)
                    {
                        cl.viewangles[PITCH] -= (fAxisValue * joy_pitchsensitivity->value) *
                                                aspeed * cl_pitchspeed->value;
                    }
                    else
                    {
                        cl.viewangles[PITCH] += (fAxisValue * joy_pitchsensitivity->value) *
                                                aspeed * cl_pitchspeed->value;
                    }
                }
            }
            else
            {
                // user wants forward control to be forward control
                if (fabs(fAxisValue) > joy_forwardthreshold->value)
                {
                    cmd->forwardmove += (fAxisValue * joy_forwardsensitivity->value) * speed *
                                        cl_forwardspeed->value;
                }
            }
            break;

        case AxisSide:
            if (fabs(fAxisValue) > joy_sidethreshold->value)
            {
                cmd->sidemove +=
                    (fAxisValue * joy_sidesensitivity->value) * speed * cl_sidespeed->value;
            }
            break;

        case AxisUp:
            if (fabs(fAxisValue) > joy_upthreshold->value)
            {
                cmd->upmove += (fAxisValue * joy_upsensitivity->value) * speed * cl_upspeed->value;
            }
            break;

        case AxisTurn:
            if ((in_strafe.state & 1) || (lookstrafe->value && mlooking))
            {
                // user wants turn control to become side control
                if (fabs(fAxisValue) > joy_sidethreshold->value)
                {
                    cmd->sidemove -=
                        (fAxisValue * joy_sidesensitivity->value) * speed * cl_sidespeed->value;
                }
            }
            else
            {
                // user wants turn control to be turn control
                if (fabs(fAxisValue) > joy_yawthreshold->value)
                {
                    if (dwControlMap[i] == JOY_ABSOLUTE_AXIS)
                    {
                        cl.viewangles[YAW] +=
                            (fAxisValue * joy_yawsensitivity->value) * aspeed * cl_yawspeed->value;
                    }
                    else
                    {
                        cl.viewangles[YAW] +=
                            (fAxisValue * joy_yawsensitivity->value) * speed * 180.0;
                    }
                }
            }
            break;

        case AxisLook:
            if (mlooking)
            {
                if (fabs(fAxisValue) > joy_pitchthreshold->value)
                {
                    // pitch movement detected and pitch movement desired by user
                    if (dwControlMap[i] == JOY_ABSOLUTE_AXIS)
                    {
                        cl.viewangles[PITCH] += (fAxisValue * joy_pitchsensitivity->value) *
                                                aspeed * cl_pitchspeed->value;
                    }
                    else
                    {
                        cl.viewangles[PITCH] +=
                            (fAxisValue * joy_pitchsensitivity->value) * speed * 180.0;
                    }
                }
            }
            break;

        default:
            break;
        }
    }
}

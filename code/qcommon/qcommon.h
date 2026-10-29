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
// qcommon.h -- definitions common to client and server
#include "q_shared.h"
#include <setjmp.h> // Com_SetThreadUnwind takes a jmp_buf

#define VERSION 3.21
#define BASEDIRNAME "basedn"

#define MAX_MSGLEN 1400 // max length of a message
#define MAX_DATAGRAM 16384 // max length of unreliable message (with UDP header)

#define PORT_MASTER 27900
#define PORT_CLIENT 27901
#define PORT_SERVER 27910

#define VERSION 3.21

// master server protocol
#define M2C_MSG_CHALLENGE ('c' << 24 | 'h' << 16 | 'a' << 8 | 'l')
#define M2C_MSG_ECHO ('e' << 24 | 'c' << 16 | 'h' << 8 | 'o')

// DD: from duke3d winglob.h
#define MAXCHAR ((char)0x7f)
#define MINCHAR ((char)0x80)
#define MAXSHORT ((short)0x7fff)
#define MINSHORT ((short)0x8000)
#define MAXINT ((int)0x7fffffff)
#define MININT ((int)0x80000000)
#define MAXLONG ((long)0x7fffffff)
#define MINLONG ((long)0x80000000)

//
// sys_viewlog system
//
void Sys_Viewlog_f(void);

// server/query protocol
// note: the duplex pending message is used by the client to build its
// packet list, but the server only sends one message per frame
enum
{
    // NETCHAN stuff
    svc_bad,
    svc_nop,
    // unused
    svc_muzzleflash,
    svc_temp_entity,
    // unused
    svc_layout,
    svc_inventory,
    svc_disconnect,
    svc_reconnect,
    svc_sound,
    svc_print,
    svc_stufftext,
    // unused
    svc_serverinfo,
    svc_precache,
    svc_centerprint,
    svc_download,
    svc_playerinfo,
    svc_packetentities,
    svc_deltapacketentities,
    svc_setangle,
    svc_setview,
    svc_usercmd,
    svc_clientinfo,
    svc_sound_f,
    svc_sound_p,
} svc_ops_t;

// client to server
enum
{
    clc_bad,
    clc_nop,
    clc_move, // [] usercmd_t
    clc_userinfo, // [] userinfo string
    clc_stringcmd, // [string] message
} clc_ops_t;

// server to client
enum
{
    svc_setangle, // [short] [short] [short] view angles
} svc2_ops_t;

#define LL(x) x = LittleLong(x)

// header for each message packet
typedef struct
{
    int netadr;
    // int  gamePort;

    // sequencing variables
    int incoming_sequence;
    int incoming_acknowledged;
    int reliable_sequence; // outgoing only
    int reliable_acknowledged; // outgoing only
    int qport;
    int last_sent_time;
    int last_reliable_sent;
    int reliable_size;
    int reliable_snd_size;
    int reliable_cleared;
    int drop_count;
    int goodack;
} netadr_t;

/*
==============================================================

NET

==============================================================
*/

#define PACKET_BACKUP 32 // number of old messages that must be kept on client
#define PACKET_MASK (PACKET_BACKUP - 1)

#define CMD_BACKUP 64 // allow a lot of command backup for very slow systems
#define CMD_MASK (CMD_BACKUP - 1)

#define MAX_PACKETENTITIES 256

typedef struct netchan_s
{
    qboolean fatal_error;
    netadr_t remote_address;
    int qport; // qport value to write (protection)

    int last_sent_time;
    int last_received;
    int reliable_sent;
    int reliable_ack;
    int reliable_length;
    byte reliable_buf[MAX_MSGLEN];

    int incoming_ack;
    int outgoing_sequence;

    int incoming_sequence;
    int incoming_acknowledged;
    int dropped;
    int qport_[2];
} netchan_t;

#define PROTOCOL_VERSION 33

typedef struct
{
    unsigned char adr;
    int port;
    int ip[4];
} netadr_lite_t;

// server.h

typedef enum
{
    ss_dead, // no map loaded
    ss_loading, // spawning level entities
    ss_game, // actively running
    ss_cinematic, // playing a cinematic
} server_state_t;

#include "qcommon_prot.h"

//
// per-level limits
//
#define MAX_CLIENTS 256 // absolute limit
#define MAX_EDICTS 1024 // must change protocol to increase more
#define MAX_LIGHTSTYLES 256
#define MAX_MODELS 256 // these are sent out as small integers
#define MAX_SOUNDS 256 // so they cannot exceed 256
#define MAX_IMAGES 256

typedef enum
{
    ERR_FATAL,
    ERR_DROP,
    ERR_QUIT
} errorcode_t;

/*
==============================================================

CMD

Command text buffering and command execution

==============================================================
*/

/*

Any number of commands can be added in a frame, from all different sources.
Most commands come from either keybindings or console line input, but
entire text files can be exec'ed as commands.

*/

void Cbuf_Init(void);
// performs a Cbuf_AddText on the command line

void Cbuf_AddText(char *text);
// as new commands are generated from the console or keybindings,
// the text is added to this buffer

void Cbuf_AddEarlyCommands(qboolean clear);
// used by the init functions, which need to execute early commands

int Cbuf_AddLateCommands(void);
// used by the init functions, which need to execute late commands
// the command line is empty when this returns

void Cbuf_Execute(void);
// pull commands from the buffer and execute them

/*
==============================================================

CMD

Command execution

==============================================================
*/

typedef void (*xcommand_t)(void);

void Cmd_Init(void);
void Cmd_AddCommand(char *name, xcommand_t cmd);

int Cmd_Argc(void);
char *Cmd_Argv(int arg);
void Cmd_QuoteString(char *string, int stringlength);
void Cmd_TokenizeString(char *text, qboolean macroExpand);
void Cmd_ExecuteString(char *text);
// Parses a single line of text into argv/argc.
// handles macro expansion and quoting

void Cmd_ForwardToServer(void);
// sends the entire command line over to the server

// used to enumerate commands for command completion (BFG console port)
void Cmd_CommandCompletion(void (*callback)(const char *command));

/*
==============================================================

CVAR

==============================================================
*/

/*

cvar_t variables are used to hold static or semistatic variables
*/

cvar_t *Cvar_Get(char *var_name, char *value, int flags);

void Cvar_Set(char *var_name, char *value);
// equivelant to <name> <value> in the main console

void Cvar_Forceset(char *var_name, char *value);

void Cvar_SetValue(char *var_name, float value);
// expands value to a character string and calls Cvar_Set

float Cvar_VariableValue(char *var_name);
// returns 0.0 if not defined

const char *Cvar_VariableString(char *var_name);
// returns "" if not defined

// used to enumerate cvars for command completion (BFG console port)
void Cvar_CommandCompletion(void (*callback)(const char *cvar));

typedef void (*cvarChanged_t)(char *name, char *string, int flags);

void Cvar_Register(cvar_t *variable, char *name, char *value, int flags, cvarChanged_t callback);
// called by the init functions

void Cvar_ForceSet(char *var_name, char *value);

/*
==============================================================

MISC

==============================================================
*/

#define ERR_FATAL 0 // exit the entire game with a popup window
#define ERR_DROP 1 // print to console and disconnect from game
#define ERR_DISCONNECT 2 // don't kill server

char *CopyString(char *in);
void Info_Print(char *s);

char *va(char *format, ...);

int Com_ServerState(void); // this should have just been a cvar...
void Com_SetServerState(int state);

unsigned Com_BlockChecksum(void *buffer, int length);
byte COM_BlockSequenceCRCByte(byte *base, int length, int sequence);

int argc;
char *argv[MAX_NUM_ARGVS + 1];

void Com_Init(void);
void Com_Shutdown(void);

void Com_Printf(char *fmt, ...);
void Com_DPrintf(char *fmt, ...);
void Com_Error(int code, char *fmt, ...);
void Com_Quit(void);

// BFG idLib thread-safe printing (implemented in qcommon/common.cpp):
// the main thread owns the output, other threads queue to it
qboolean Com_IsMainThread(void);
void Com_FlushPrintQueue(void);
void Com_SetThreadUnwind(jmp_buf *unwind);
qboolean Com_TakePendingError(int *code, char *msg, int msgSize);

int Mem_Report(void);

void Com_BeginRedirect(int target, char *buffer, int buffersize, void (*flush));
void Com_EndRedirect(void);

int Parse(char *data);

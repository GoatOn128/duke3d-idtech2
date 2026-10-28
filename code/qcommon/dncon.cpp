/* Shared CON compiler and bytecode executor. Hosts supply file, logging and
   operation bindings; compiled script data belongs to each loaded module. */
#include "dncon.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DN_CON_TOKENS 131072
#define DN_CON_SYMBOLS 16384
#define DN_CON_CODE 65536

typedef struct
{
    char text[192];
    int line;
} token_t;

typedef struct
{
    char name[96];
    int value, alias, defined;
} symbol_t;

typedef struct
{
    int op, args[16], argc, jump;
} instruction_t;

static dn_con_import_t con_import;
static uint32_t script_checksum;

void DN_ConConfigure(const dn_con_import_t *imports)
{
    con_import = *imports;
}

uint32_t DN_ConChecksum(void)
{
    return script_checksum;
}

static token_t *tokens;
static symbol_t *symbols;
static instruction_t *code;
static int ntokens, nsymbols, ncode, pos, nactions, nmoves, nais, nactors, parse_failed,
    pending_else;
static dn_actor_t actors[1024];
dn_action_t dn_actions[2048];
dn_move_t dn_moves[1024];
dn_ai_t dn_ais[1024];
dn_sound_t dn_sounds[1024];
char dn_quotes[256][192];
static dn_level_info_t levels[4][11];
static char music[5][11][64];

const char *DN_ConMusic(int episode, int level)
{
    return episode >= 0 && episode <= 4 && level >= 1 && level <= 11
               ? music[episode][level - 1]
               : "";
}

const dn_level_info_t *DN_ConLevel(int episode, int level)
{
    return episode >= 1 && episode <= 4 && level >= 1 && level <= 11
               ? &levels[episode - 1][level - 1]
               : NULL;
}

static const char *keywords[] = {
#define DN_KEYWORD(name, text) text,
#include "dnkeywords.inc"
#undef DN_KEYWORD
};

static const char *startup_names[] = {
#define DN_STARTUP(name, value) #name,
#include "dnstartup.inc"
#undef DN_STARTUP
};
static const int startup_defaults[] = {
#define DN_STARTUP(name, value) value,
#include "dnstartup.inc"
#undef DN_STARTUP
};
static int startup_values[sizeof(startup_defaults) / sizeof(startup_defaults[0])];

int DN_ConSetting(const char *name, int fallback)
{
    for (int i = 0; i < sizeof(startup_names) / sizeof(startup_names[0]); i++)
    {
        if (!strcmp(name, startup_names[i]))
        {
            return startup_values[i];
        }
    }
    return fallback;
}

static int Keyword(const char *text)
{
    int i;
    for (i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++)
    {
        if (!strcmp(text, keywords[i]))
        {
            return i;
        }
    }
    return -1;
}

static void ParseError(const char *message)
{
    con_import.Print("CON line %d near '%s': %s\n", pos < ntokens ? tokens[pos].line : 0,
                     pos < ntokens ? tokens[pos].text : "EOF", message);
    parse_failed = 1;
}

static int Intern(const char *name)
{
    int i;
    char *end;
    long value;
    for (i = 0; i < nsymbols; i++)
    {
        if (!strcmp(symbols[i].name, name))
        {
            return i;
        }
    }
    if (nsymbols == DN_CON_SYMBOLS)
    {
        ParseError("Too many CON symbols");
        return 0;
    }
    i = nsymbols++;
    snprintf(symbols[i].name, sizeof(symbols[i].name), "%s", name);
    symbols[i].alias = -1;
    value = strtol(name, &end, 10);
    if (*name && !*end)
    {
        symbols[i].value = (int)value;
        symbols[i].defined = 1;
    }
    return i;
}

static int Value(int id)
{
    int depth = 0;
    while (id >= 0 && id < nsymbols && symbols[id].alias >= 0 && depth++ < 32)
    {
        id = symbols[id].alias;
    }
    return id >= 0 && id < nsymbols && symbols[id].defined ? symbols[id].value : 0;
}

int DN_ConSymbol(const char *name, int fallback)
{
    int i;
    for (i = 0; i < nsymbols; i++)
    {
        if (!strcmp(symbols[i].name, name))
        {
            return symbols[i].defined ? Value(i) : fallback;
        }
    }
    return fallback;
}

static void Define(const char *name, int value)
{
    int id = Intern(name);
    symbols[id].defined = 1;
    symbols[id].alias = -1;
    symbols[id].value = value;
}

static int LexFile(const char *name, int depth)
{
    size_t length;
    char path[128], *source, *p;
    int line = 1;
    if (depth > 16)
    {
        con_import.Print("CON include nesting exceeds 16\n");
        return 0;
    }
    snprintf(path, sizeof(path), "scripts/%s", name);
    for (int i = 0; path[i]; i++)
    {
        path[i] = (char)tolower((unsigned char)path[i]);
    }
    if (strstr(name, "..") || strchr(name, '/') || strchr(name, '\\'))
    {
        return 0;
    }
    source = (char *)(con_import.ReadFile(path, &length));
    if (!source)
    {
        con_import.Print("Cannot read %s\n", path);
        return 0;
    }
    for (size_t byte = 0; byte < length; byte++)
    {
        script_checksum = (script_checksum ^ (unsigned char)source[byte]) * 16777619u;
    }
    p = source;
    while (*p)
    {
        token_t t;
        int count = 0;
        if (isspace((unsigned char)*p))
        {
            if (*p++ == '\n')
            {
                line++;
            }
            continue;
        }
        if (p[0] == '/' && p[1] == '/')
        {
            while (*p && *p != '\n')
            {
                p++;
            }
            continue;
        }
        if (p[0] == '/' && p[1] == '*')
        {
            p += 2;
            while (*p && !(p[0] == '*' && p[1] == '/'))
            {
                if (*p++ == '\n')
                {
                    line++;
                }
            }
            if (*p)
            {
                p += 2;
            }
            continue;
        }
        memset(&t, 0, sizeof(t));
        t.line = line;
        if (*p == '{' || *p == '}')
        {
            t.text[count++] = *p++;
        }
        else if (*p == '"')
        {
            p++;
            while (*p && *p != '"')
            {
                if (count < sizeof(t.text) - 1)
                {
                    t.text[count++] = *p;
                }
                p++;
            }
            if (*p)
            {
                p++;
            }
        }
        else
        {
            while (*p && !isspace((unsigned char)*p) && *p != '{' && *p != '}')
            {
                if (count < sizeof(t.text) - 1)
                {
                    t.text[count++] = *p;
                }
                p++;
            }
        }
        t.text[count] = 0;
        if (!strcmp(t.text, "include"))
        {
            char include[96];
            int n = 0;
            while (*p && isspace((unsigned char)*p))
            {
                if (*p++ == '\n')
                {
                    line++;
                }
            }
            while (*p && !isspace((unsigned char)*p))
            {
                if (n < sizeof(include) - 1)
                {
                    include[n++] = *p;
                }
                p++;
            }
            include[n] = 0;
            if (!LexFile(include, depth + 1))
            {
                free(source);
                return 0;
            }
            continue;
        }
        if (ntokens == DN_CON_TOKENS)
        {
            free(source);
            con_import.Print("Too many CON tokens\n");
            return 0;
        }
        tokens[ntokens++] = t;
    }
    free(source);
    return 1;
}

static int Emit(int op)
{
    int i;
    if (ncode >= DN_CON_CODE)
    {
        ParseError("CON bytecode overflow");
        return 0;
    }
    i = ncode++;
    code[i].op = op;
    return i;
}

static int Operand(void)
{
    if (pos >= ntokens)
    {
        ParseError("Missing operand");
        return Intern("0");
    }
    return Intern(tokens[pos++].text);
}

static int Values(int *out, int maximum)
{
    int n = 0;
    while (pos < ntokens && Keyword(tokens[pos].text) < 0)
    {
        if (n >= maximum)
        {
            ParseError("Too many operands");
            break;
        }
        out[n++] = Operand();
    }
    return n;
}

static int Arity(int op)
{
    switch (op)
    {
    case DN_OP_ADDAMMO:
    case DN_OP_ADDWEAPON:
    case DN_OP_SIZETO:
    case DN_OP_DEBRIS:
    case DN_OP_ADDINVENTORY:
    case DN_OP_GUTS:
    case DN_OP_IFPINVENTORY:
    case DN_OP_SIZEAT:
        return 2;
    case DN_OP_HITRADIUS:
        return 5;
    case DN_OP_IFCANSEE:
    case DN_OP_IFHITWEAPON:
    case DN_OP_BREAK:
    case DN_OP_FALL:
    case DN_OP_KILLIT:
    case DN_OP_IFDEAD:
    case DN_OP_IFSQUISHED:
    case DN_OP_RESETACTIONCOUNT:
    case DN_OP_PSTOMP:
    case DN_OP_RESETPLAYER:
    case DN_OP_IFONWATER:
    case DN_OP_IFINWATER:
    case DN_OP_IFCANSHOOTTARGET:
    case DN_OP_RESETCOUNT:
    case DN_OP_IFACTORNOTSTAYPUT:
    case DN_OP_WACKPLAYER:
    case DN_OP_IFHITSPACE:
    case DN_OP_IFOUTSIDE:
    case DN_OP_IFMULTIPLAYER:
    case DN_OP_OPERATE:
    case DN_OP_IFINSPACE:
    case DN_OP_IFBULLETNEAR:
    case DN_OP_IFRESPAWN:
    case DN_OP_IFINOUTERSPACE:
    case DN_OP_IFNOTMOVING:
    case DN_OP_RESPAWNHITAG:
    case DN_OP_TIP:
    case DN_OP_IFAWAYFROMWALL:
    case DN_OP_IFCANSEETARGET:
    case DN_OP_GETLASTPAL:
    case DN_OP_PKICK:
    case DN_OP_MIKESND:
    case DN_OP_TOSSWEAPON:
    case DN_OP_NULLOP:
    case DN_OP_IFNOSOUNDS:
        return 0;
    case DN_OP_MOVE:
    case DN_OP_IFP:
    case DN_OP_PALFROM:
        return -1;
    default:
        return 1;
    }
}

static void Statement(int depth)
{
    int op, i, n, branch, other;
    if (depth > 128)
    {
        ParseError("CON nesting exceeds 128");
        return;
    }
    if (pos >= ntokens || parse_failed)
    {
        return;
    }
    op = Keyword(tokens[pos++].text);
    if (op < 0)
    {
        ParseError("Expected a CON keyword");
        return;
    }
    if (op == DN_OP_LBRACE)
    {
        while (pos < ntokens && Keyword(tokens[pos].text) != DN_OP_RBRACE && !parse_failed)
        {
            Statement(depth + 1);
        }
        if (pos == ntokens)
        {
            ParseError("Unclosed CON block");
        }
        else
        {
            pos++;
        }
        /* Keep the closing brace in the control-flow stream. An inner false
           condition must stop here rather than mistake a following outer
           else for its own. Pig Cop firing/recovery relies on this boundary. */
        Emit(DN_OP_NULLOP);
        return;
    }
    if (op == DN_OP_ELSE)
    {
        if (!pending_else)
        {
            ParseError("Else without if");
            return;
        }
        pending_else--;
        i = Emit(DN_OP_JUMP);
        Statement(depth + 1);
        code[i].jump = ncode;
        return;
    }
    if (op == DN_OP_RBRACE || op == DN_OP_ENDS || op == DN_OP_ENDA)
    {
        ParseError("Unexpected block terminator");
        return;
    }
    i = Emit(op);
    n = Arity(op);
    if (n < 0)
    {
        code[i].argc = Values(code[i].args, op == DN_OP_PALFROM ? 4 : 16);
    }
    else
    {
        code[i].argc = n;
        for (int j = 0; j < n; j++)
        {
            code[i].args[j] = Operand();
        }
    }
    if (!strncmp(keywords[op], "if", 2))
    {
        Statement(depth + 1);
        code[i].jump = ncode;
        pending_else++;
    }
}

int DN_ConLoad(void)
{
    int i, startup_refs[30] = {0}, startup_count = 0;
    DN_ConFree();
    tokens = (token_t *)(calloc(DN_CON_TOKENS, sizeof(*tokens)));
    symbols = (symbol_t *)(calloc(DN_CON_SYMBOLS, sizeof(*symbols)));
    code = (instruction_t *)(calloc(DN_CON_CODE, sizeof(*code)));
    if (!tokens || !symbols || !code)
    {
        con_import.Error("Cannot allocate CON runtime");
    }
    script_checksum = 2166136261u;
    nactions = nmoves = nais = 1;
    Define("0", 0);
    if (!LexFile("game.con", 0))
    {
        return 0;
    }
    pos = 0;
    while (pos < ntokens && !parse_failed)
    {
        int op = Keyword(tokens[pos++].text), line = tokens[pos - 1].line;
        if (op == DN_OP_DEFINE)
        {
            int id = Operand(), alias = Operand();
            symbols[id].alias = alias;
            symbols[id].defined = 1;
        }
        else if (op == DN_OP_ACTION || op == DN_OP_MOVE || op == DN_OP_AI)
        {
            char name[96];
            int v[16] = {0}, n;
            if (pos >= ntokens)
            {
                ParseError("Missing definition name");
                break;
            }
            snprintf(name, sizeof(name), "%s", tokens[pos++].text);
            n = Values(v, op == DN_OP_ACTION ? 5 : 16);
            if (op == DN_OP_ACTION)
            {
                if (nactions >= 2048)
                {
                    ParseError("Too many actions");
                    break;
                }
                Define(name, nactions);
                for (i = 0; i < n; i++)
                {
                    dn_actions[nactions].value[i] = v[i];
                }
                nactions++;
            }
            else if (op == DN_OP_MOVE)
            {
                if (nmoves >= 1024)
                {
                    ParseError("Too many moves");
                    break;
                }
                Define(name, nmoves);
                dn_moves[nmoves].speed = n ? v[0] : 0;
                dn_moves[nmoves].zspeed = n > 1 ? v[1] : 0;
                nmoves++;
            }
            else
            {
                if (nais >= 1024)
                {
                    ParseError("Too many AIs");
                    break;
                }
                Define(name, nais);
                dn_ais[nais].action = n ? v[0] : 0;
                dn_ais[nais].move = n > 1 ? v[1] : 0;
                int flags = 0;
                for (i = 2; i < n; i++)
                {
                    flags |= Value(v[i]);
                }
                dn_ais[nais].flags = flags;
                nais++;
            }
        }
        else if (op == DN_OP_STATE)
        {
            pending_else = 0;
            int id = Operand();
            symbols[id].value = ncode;
            symbols[id].defined = 1;
            while (pos < ntokens && Keyword(tokens[pos].text) != DN_OP_ENDS && !parse_failed)
            {
                Statement(0);
            }
            if (pos >= ntokens)
            {
                ParseError("Unterminated state");
            }
            else
            {
                pos++;
            }
            Emit(DN_OP_RETURN);
        }
        else if (op == DN_OP_ACTOR || op == DN_OP_USERACTOR)
        {
            pending_else = 0;
            int enemy = 0, tileref, v[16] = {0}, n;
            dn_actor_t *actor;
            if (nactors >= 1024)
            {
                ParseError("Too many actors");
                break;
            }
            if (op == DN_OP_USERACTOR)
            {
                enemy = Value(Operand());
            }
            tileref = Operand();
            n = Values(v, op == DN_OP_ACTION ? 5 : 16);
            actor = &actors[nactors++];
            actor->tile = tileref;
            actor->code = ncode;
            actor->health = n ? v[0] : 0;
            actor->action = n > 1 ? v[1] : 0;
            actor->move = n > 2 ? v[2] : 0;
            for (i = 3; i < n; i++)
            {
                actor->flags |= Value(v[i]);
            }
            actor->enemy = enemy;
            while (pos < ntokens && Keyword(tokens[pos].text) != DN_OP_ENDA && !parse_failed)
            {
                Statement(0);
            }
            if (pos >= ntokens)
            {
                ParseError("Unterminated actor");
            }
            else
            {
                pos++;
            }
            Emit(DN_OP_RETURN);
        }
        else if (op == DN_OP_DEFINESOUND)
        {
            int id = Value(Operand()), v[5];
            char filename[64];
            if (pos >= ntokens)
            {
                ParseError("Missing sound filename");
                break;
            }
            snprintf(filename, sizeof(filename), "%s", tokens[pos++].text);
            for (i = 0; i < 5; i++)
            {
                v[i] = Value(Operand());
            }
            if (id >= 0 && id < 1024)
            {
                snprintf(dn_sounds[id].file, sizeof(dn_sounds[id].file), "%s", filename);
                dn_sounds[id].pitchlow = v[0];
                dn_sounds[id].pitchhigh = v[1];
                dn_sounds[id].priority = v[2];
                dn_sounds[id].flags = v[3];
                dn_sounds[id].distance = v[4];
            }
        }
        else if (op == DN_OP_DEFINEQUOTE)
        {
            int id = Value(Operand());
            char quote[192] = "";
            size_t used = 0;
            while (pos < ntokens && tokens[pos].line == line)
            {
                used += snprintf(quote + used, sizeof(quote) - used, "%s%s", used ? " " : "",
                                 tokens[pos++].text);
                if (used >= sizeof(quote) - 1)
                {
                    while (pos < ntokens && tokens[pos].line == line)
                    {
                        pos++;
                    }
                    break;
                }
            }
            if (id >= 0 && id < 256)
            {
                snprintf(dn_quotes[id], sizeof(dn_quotes[id]), "%s", quote);
            }
        }
        else if (op == DN_OP_GAMESTARTUP)
        {
            startup_count = Values(startup_refs, 30);
            if (startup_count != 30)
            {
                ParseError("Atomic gamestartup requires 30 operands");
            }
        }
        else if (op == DN_OP_MUSIC)
        {
            int episode = Value(Operand());
            int track = 0;
            while (pos < ntokens && Keyword(tokens[pos].text) < 0)
            {
                const char *filename = tokens[pos++].text;
                if (episode >= 0 && episode <= 4 && track < 11)
                {
                    snprintf(music[episode][track], sizeof(music[episode][track]), "%s", filename);
                }
                track++;
            }
        }
        else if (op == DN_OP_DEFINELEVELNAME)
        {
            if (pos + 4 >= ntokens || tokens[pos + 4].line != line)
            {
                ParseError("Incomplete level definition");
                break;
            }
            int episode = atoi(tokens[pos++].text), level = atoi(tokens[pos++].text);
            pos++; /* The map filename is separate from its display name. */
            dn_level_info_t info = {};
            int minutes = 0, seconds = 0;
            if (sscanf(tokens[pos++].text, "%d:%d", &minutes, &seconds) == 2)
            {
                info.par_seconds = minutes * 60 + seconds;
            }
            if (sscanf(tokens[pos++].text, "%d:%d", &minutes, &seconds) == 2)
            {
                info.designer_seconds = minutes * 60 + seconds;
            }
            size_t used = 0;
            while (pos < ntokens && tokens[pos].line == line)
            {
                if (used < sizeof(info.name) - 1)
                {
                    int written = snprintf(info.name + used, sizeof(info.name) - used, "%s%s",
                                           used ? " " : "", tokens[pos].text);
                    used += written > 0 ? (size_t)written : 0;
                }
                pos++;
            }
            if (episode >= 0 && episode < 4 && level >= 0 && level < 11)
            {
                levels[episode][level] = info;
            }
        }
        else if (op == DN_OP_DEFINEVOLUMENAME || op == DN_OP_DEFINESKILLNAME || op == DN_OP_MUSIC ||
                 op == DN_OP_BETANAME)
        {
            while (pos < ntokens && tokens[pos].line == line)
            {
                pos++;
            }
        }
        else
        {
            ParseError("Unexpected top-level keyword");
            break;
        }
    }
    if (parse_failed)
    {
        return 0;
    }
    for (i = 1; i < nactions; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            dn_actions[i].value[j] = Value(dn_actions[i].value[j]);
        }
    }
    for (i = 0; i < startup_count; i++)
    {
        startup_values[i] = Value(startup_refs[i]);
    }
    for (i = 1; i < nmoves; i++)
    {
        dn_moves[i].speed = Value(dn_moves[i].speed);
        dn_moves[i].zspeed = Value(dn_moves[i].zspeed);
    }
    for (i = 1; i < nais; i++)
    {
        dn_ais[i].action = Value(dn_ais[i].action);
        dn_ais[i].move = Value(dn_ais[i].move);
    }
    for (i = 0; i < nactors; i++)
    {
        actors[i].tile = Value(actors[i].tile);
        actors[i].health = Value(actors[i].health);
        actors[i].action = Value(actors[i].action);
        actors[i].move = Value(actors[i].move);
    }
    for (i = 0; i < ncode; i++)
    {
        for (int j = 0; j < code[i].argc; j++)
        {
            code[i].args[j] = Value(code[i].args[j]);
        }
    }
    for (i = 0; i < nsymbols; i++)
    {
        if (!symbols[i].defined)
        {
            con_import.Print("CON undefined symbol '%s'\n", symbols[i].name);
            parse_failed = 1;
        }
    }
    con_import.Print(
        "Runtime CON: %d actors, %d states/instructions, %d actions, %d moves, %d AIs\n", nactors,
        ncode, nactions - 1, nmoves - 1, nais - 1);
    free(tokens);
    tokens = NULL;
    return !parse_failed;
}

void DN_ConFree(void)
{
    free(tokens);
    free(symbols);
    free(code);
    tokens = NULL;
    symbols = NULL;
    code = NULL;
    ntokens = nsymbols = ncode = pos = nactors = parse_failed = pending_else = 0;
    memset(actors, 0, sizeof(actors));
    memset(dn_actions, 0, sizeof(dn_actions));
    memset(dn_moves, 0, sizeof(dn_moves));
    memset(dn_ais, 0, sizeof(dn_ais));
    memset(dn_sounds, 0, sizeof(dn_sounds));
    memset(dn_quotes, 0, sizeof(dn_quotes));
    memset(levels, 0, sizeof(levels));
    memset(music, 0, sizeof(music));
    memcpy(startup_values, startup_defaults, sizeof(startup_values));
}

const dn_actor_t *DN_ConActor(int tile)
{
    for (int i = 0; i < nactors; i++)
    {
        if (actors[i].tile == tile)
        {
            return &actors[i];
        }
    }
    return NULL;
}

static void Execute(const dn_con_vm_t *vm, int pc, int depth, int *budget, int *halted)
{
    while (pc >= 0 && pc < ncode && vm->Alive(vm->state) && !*halted && (*budget)-- > 0)
    {
        instruction_t *in = &code[pc++];
        if (in->op == DN_OP_RETURN || in->op == DN_OP_BREAK)
        {
            return;
        }
        if (in->op == DN_OP_JUMP)
        {
            pc = in->jump;
            continue;
        }
        if (in->op == DN_OP_STATE)
        {
            if (depth >= 32)
            {
                con_import.Print("CON recursion limit, tile %d\n", vm->tile);
                *halted = 1;
                return;
            }
            Execute(vm, in->args[0], depth + 1, budget, halted);
            continue;
        }
        if (!strncmp(keywords[in->op], "if", 2))
        {
            if (!vm->Condition(vm->state, in->op, in->args, in->argc))
            {
                pc = in->jump;
                if (pc < ncode && code[pc].op == DN_OP_JUMP)
                {
                    pc++;
                }
            }
        }
        else if (!vm->Command(vm->state, in->op, in->args, in->argc))
        {
            *halted = 1;
            return;
        }
    }
}

int DN_ConExecute(const dn_con_vm_t *vm)
{
    int budget = 4096, halted = 0;
    if (!vm || vm->entry < 0 || !vm->Alive || !vm->Condition || !vm->Command)
    {
        return 0;
    }
    Execute(vm, vm->entry, 0, &budget, &halted);
    if (budget <= 0)
    {
        con_import.Print("CON instruction budget exceeded, tile %d\n", vm->tile);
        return 0;
    }
    return !halted;
}

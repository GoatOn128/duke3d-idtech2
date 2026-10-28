/* Original Atomic Edition Logo() sequence and timing from GAME.cpp/MENUES.cpp. */
#include "client.h"

extern void M_PushMenu(void (*draw)(void), const char *(*key)(int));
extern void M_ForceMenuOff(void);
extern void M_Menu_Main_f(void);
extern void (*m_drawfunc)(void);

static int intro_stage, intro_start, intro_frames, intro_sound;
static qboolean intro_active, music_wanted;

static void IntroDraw(void);

void DN_IntroShutdown(void)
{
    if (music_wanted)
    {
        S_MusicStop();
    }
    music_wanted = false;
    intro_active = false;
}

static void MenuMusic(void)
{
    if (music_wanted)
    {
        S_MusicPlay("grabbag.mp3", true);
    }
}

static void IntroStage(int stage)
{
    intro_stage = stage;
    intro_start = cls.realtime;
    intro_sound = 0;
    if (stage == 1)
    {
        music_wanted = true;
        MenuMusic();
    }
    Com_Printf("Duke intro: %s\n", stage == 0 ? "logo" : stage == 1 ? "3D Realms" : "title");
}

static void IntroFinish(void)
{
    intro_active = false;
    music_wanted = true;
    S_StopAllSounds();
    M_ForceMenuOff();
    M_Menu_Main_f();
    Com_Printf("Duke intro: main menu\n");
}

static const char *IntroKey(int key)
{
    if (key == K_ESCAPE || intro_stage == 2)
    {
        IntroFinish();
    }
    else if (key != K_MWHEELUP && key != K_MWHEELDOWN)
    {
        S_StopAllSounds();
        IntroStage(intro_stage + 1);
    }
    return NULL;
}

static void IntroPic(const char *name)
{
    re.DrawStretchPic(0, 0, viddef.width, viddef.height, (char *)name);
}

static void TitleTile(int tile, float y, float scale)
{
    char path[MAX_QPATH];
    int width, height;
    if (scale <= 0)
    {
        return;
    }
    snprintf(path, sizeof(path), "/anm/dnintro/tile%d.tga", tile);
    re.DrawGetPicSize(&width, &height, path);
    re.DrawStretchPic((int)((160 - width * scale / 2) * viddef.width / 320),
                      (int)((y - height * scale / 2) * viddef.height / 200),
                      (int)(width * scale * viddef.width / 320),
                      (int)(height * scale * viddef.height / 200), path);
}

static void IntroDraw(void)
{
    int elapsed = cls.realtime - intro_start;
    char path[MAX_QPATH];
    re.DrawFill(0, 0, viddef.width, viddef.height, 0);
    if (intro_stage == 0)
    {
        int frame = DN_Min(intro_frames, elapsed / 75 + 1);
        snprintf(path, sizeof(path), "/anm/dnintro/logo%03d.tga", frame);
        IntroPic(path);
        if (!intro_sound)
        {
            S_StartLocalSound("flyby.wav");
            intro_sound = 1;
        }
        if (frame >= 19 && intro_sound == 1)
        {
            S_StartLocalSound("bombexpl.wav");
            intro_sound = 2;
        }
    }
    else if (intro_stage == 1)
    {
        IntroPic("/anm/dnintro/realms.tga");
    }
    else
    {
        int tick = elapsed * 120 / 1000;
        IntroPic("/anm/dnintro/title.tga");
        if (tick > 120)
        {
            TitleTile(2497, 104, DN_Min(tick - 120, 60) / 64.0f);
        }
        if (tick > 220)
        {
            TitleTile(2498, 129, DN_Min(tick - 220, 30) / 32.0f);
        }
        if (tick >= 280)
        {
            TitleTile(2502, 151, tick < 395 ? (410 - tick) / 16.0f : 30 / 32.0f);
        }
        static const int ticks[] = {120, 220, 280, 395};
        while (intro_sound < 4 && tick >= ticks[intro_sound])
        {
            S_StartLocalSound(intro_sound == 2 ? "flyby.wav" : "bombexpl.wav");
            intro_sound++;
        }
    }
    if (intro_stage != 0 && elapsed < 300)
    {
        int fade = 15 - elapsed * 15 / 300;
        snprintf(path, sizeof(path), "/anm/dnintro/fade%02d.tga", fade);
        IntroPic(path);
    }
}

void DN_IntroFrame(void)
{
    if (cls.state == ca_active || Com_ServerState())
    {
        if (music_wanted || intro_active)
        {
            music_wanted = false;
            intro_active = false;
        }
        return;
    }
    if (intro_active && m_drawfunc != IntroDraw)
    {
        intro_active = false;
    }
    if (intro_active && cls.key_dest == key_menu)
    {
        int duration = intro_stage == 0 ? intro_frames * 75 : intro_stage == 1 ? 7000 : 8167;
        if (cls.realtime - intro_start >= duration)
        {
            if (intro_stage == 2)
            {
                IntroFinish();
            }
            else
            {
                IntroStage(intro_stage + 1);
            }
        }
    }
    MenuMusic();
}

void DN_IntroStart(void)
{
    void *data = NULL;
    int size;
    if (Com_ServerState() || cls.state == ca_active)
    {
        M_Menu_Main_f();
        return;
    }
    DN_IntroShutdown();
    SCR_EndLoadingPlaque();
    size = FS_LoadFile("anm/dnintro/frames.txt", &data);
    intro_frames = size > 0 && size < 16 ? atoi(static_cast<const char *>(data)) : 0;
    if (data)
    {
        FS_FreeFile(data);
    }
    intro_frames = DN_Max(0, DN_Min(intro_frames, 1024));
    M_ForceMenuOff();
    M_PushMenu(IntroDraw, IntroKey);
    intro_active = true;
    IntroStage(intro_frames ? 0 : 1);
}

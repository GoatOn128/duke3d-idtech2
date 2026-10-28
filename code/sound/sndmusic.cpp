/* MP3 music streaming: four reusable PCM buffers, independent effect/music gain. */
#include <algorithm>
#include "../client/client.h"
#include "sndlocal.h"
#define DR_MP3_NO_STDIO
#include "../mp3/dr_mp3.h"

namespace
{
constexpr int BufferCount = 4;
constexpr int BufferFrames = 8192;
drmp3 decoder;
void *encoded;
ALuint source, buffers[BufferCount];
char current[MAX_QPATH], requested[MAX_QPATH];
bool initialized, looping, focused = true, manualPause, ended;
uint64_t decodedFrames, loopCount;
cvar_t *musicVolume, *menuMusic, *enabled;

bool Paused(void)
{
    return !focused || manualPause || !enabled->value ||
           (cl_paused && cl_paused->value) ||
           (!strcmp(current, "grabbag.mp3") && cls.state != ca_active && !menuMusic->value);
}

bool Fill(ALuint buffer)
{
    drmp3_int16 pcm[BufferFrames * 2];
    drmp3_uint64 frames = drmp3_read_pcm_frames_s16(&decoder, BufferFrames, pcm);
    if (frames < BufferFrames && looping)
    {
        if (drmp3_seek_to_pcm_frame(&decoder, 0))
        {
            loopCount++;
            frames += drmp3_read_pcm_frames_s16(&decoder, BufferFrames - frames,
                                               pcm + frames * decoder.channels);
        }
    }
    if (!frames)
    {
        return false;
    }
    decodedFrames += frames;
    alBufferData(buffer, decoder.channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16,
                 pcm, ALsizei(frames * decoder.channels * sizeof(pcm[0])), decoder.sampleRate);
    return alGetError() == AL_NO_ERROR;
}

void MusicCommand(void)
{
    const char *command = Cmd_Argv(1);
    if (!Q_strcasecmp(command, "stop"))
    {
        S_MusicStop();
    }
    else if (!Q_strcasecmp(command, "pause"))
    {
        manualPause = true;
    }
    else if (!Q_strcasecmp(command, "resume"))
    {
        manualPause = false;
    }
    else if ((!Q_strcasecmp(command, "play") || !Q_strcasecmp(command, "loop")) &&
             Cmd_Argc() == 3)
    {
        S_MusicPlay(Cmd_Argv(2), !Q_strcasecmp(command, "loop"));
    }
    else
    {
        ALint state = 0, queued = 0;
        if (source)
        {
            alGetSourcei(source, AL_SOURCE_STATE, &state);
            alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
        }
        Com_Printf("MP3 music: %s, %s, %u Hz, %u channels, %d queued buffers\n",
                   current[0] ? current : "stopped",
                   state == AL_PLAYING ? "playing" : state == AL_PAUSED ? "paused" : "stopped",
                   decoder.sampleRate, decoder.channels, queued);
        Com_Printf("music play|loop <name.mp3>; music stop|pause|resume|info\n");
        Com_Printf("Music stream: %llu decoded frames, %llu loops, gain %.2f\n",
                   (unsigned long long)decodedFrames, (unsigned long long)loopCount,
                   musicVolume ? musicVolume->value : 0);
    }
}
} // namespace

void S_MusicInit(void)
{
    if (initialized)
    {
        return;
    }
    musicVolume = Cvar_Get("s_musicvolume", "0.5", CVAR_ARCHIVE);
    menuMusic = Cvar_Get("s_menu_music", "1", CVAR_ARCHIVE);
    enabled = Cvar_Get("s_music", "1", CVAR_ARCHIVE);
    Cmd_AddCommand("music", MusicCommand);
    initialized = true;
}

void S_MusicStop(void)
{
    if (source)
    {
        alSourceStop(source);
        alDeleteSources(1, &source);
        alDeleteBuffers(BufferCount, buffers);
    }
    source = 0;
    if (encoded)
    {
        drmp3_uninit(&decoder);
        FS_FreeFile(encoded);
        encoded = nullptr;
    }
    decoder = {};
    decodedFrames = loopCount = 0;
    current[0] = requested[0] = 0;
    manualPause = ended = false;
}

void S_MusicShutdown(void)
{
    S_MusicStop();
    if (initialized)
    {
        Cmd_RemoveCommand("music");
        initialized = false;
    }
}

void S_MusicPlay(const char *name, qboolean repeat)
{
    if (!initialized)
    {
        S_MusicInit();
    }
    if (!name || !name[0] || !strcmp(name, "0"))
    {
        S_MusicStop();
        return;
    }
    if (!S_Ready())
    {
        return;
    }
    // Only a filename in the game's music directory is accepted.
    if (strlen(name) >= MAX_QPATH || strstr(name, "..") || strpbrk(name, "/\\:"))
    {
        Com_Printf("Music: invalid filename\n");
        return;
    }
    char normalized[MAX_QPATH];
    snprintf(normalized, sizeof(normalized), "%s", name);
    for (char *p = normalized; *p; p++)
    {
        *p = char(tolower(static_cast<unsigned char>(*p)));
    }
    const char *dot = strrchr(normalized, '.');
    if (!dot || strcmp(dot, ".mp3"))
    {
        Com_Printf("Music: expected an MP3 filename\n");
        return;
    }
    if (!strcmp(requested, normalized))
    {
        return; // Avoid reopening a missing or completed track every frame.
    }
    S_MusicStop();
    snprintf(requested, sizeof(requested), "%s", normalized);
    char path[MAX_QPATH + 8];
    snprintf(path, sizeof(path), "music/%s", normalized);
    int size = FS_LoadFile(path, &encoded);
    if (size <= 0 || !drmp3_init_memory(&decoder, encoded, size_t(size), nullptr))
    {
        if (encoded)
        {
            FS_FreeFile(encoded);
            encoded = nullptr;
        }
        Com_Printf("Music: missing or invalid MP3 %s\n", path);
        return;
    }
    if ((decoder.channels != 1 && decoder.channels != 2) || !decoder.sampleRate ||
        decoder.sampleRate > 192000)
    {
        S_MusicStop();
        return;
    }
    snprintf(current, sizeof(current), "%s", normalized);
    looping = repeat != 0;
    alGetError();
    alGenSources(1, &source);
    alGenBuffers(BufferCount, buffers);
    if (alGetError() != AL_NO_ERROR)
    {
        S_MusicStop();
        return;
    }
    alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcef(source, AL_ROLLOFF_FACTOR, 0);
    alSourcef(source, AL_GAIN, std::clamp(musicVolume->value, 0.0f, 1.0f));
    for (ALuint buffer : buffers)
    {
        if (!Fill(buffer))
        {
            break;
        }
        alSourceQueueBuffers(source, 1, &buffer);
    }
    if (!Paused())
    {
        alSourcePlay(source);
    }
    Com_Printf("MP3 music: %s (%u Hz, %u channels)\n", current, decoder.sampleRate,
               decoder.channels);
}

void S_MusicUpdate(void)
{
    if (!source || !S_Ready())
    {
        return;
    }
    ALint processed, state, queued;
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    alSourcef(source, AL_GAIN, std::clamp(musicVolume->value, 0.0f, 1.0f));
    if (Paused())
    {
        if (state == AL_PLAYING)
        {
            alSourcePause(source);
        }
        return;
    }
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
    while (processed-- > 0)
    {
        ALuint buffer;
        alSourceUnqueueBuffers(source, 1, &buffer);
        if (Fill(buffer))
        {
            alSourceQueueBuffers(source, 1, &buffer);
        }
    }
    alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
    if (queued && state != AL_PLAYING)
    {
        alSourcePlay(source); // Also recovers after an underrun.
    }
    if (!queued && !ended)
    {
        ended = true;
        Com_Printf("MP3 music finished: %s\n", current);
    }
}

void S_MusicActivate(qboolean enabled)
{
    focused = enabled != 0;
    S_MusicUpdate();
}

/* OpenAL sound effects, entity loops and cinematic PCM streaming. */
#include <algorithm>
#include <vector>
#include "../client/client.h"
#include "sndlocal.h"
#include "dnambience.h"

struct sfx_s
{
    char name[MAX_QPATH];
    ALuint buffer;
    int registration;
    bool attempted;
};

namespace
{
constexpr int VoiceCount = 64;
constexpr int RawBuffers = 16;
sfx_s sounds[MAX_SOUNDS * 2];
struct Voice
{
    ALuint source;
    sfx_s *sound;
    int entity, channel, begin, serial;
    float volume, attenuation;
    vec3_t origin;
    bool fixed, loop, seen, started;
} voices[VoiceCount];
ALCdevice *device;
ALCcontext *context;
ALuint rawSource, rawBuffers[RawBuffers];
bool rawUsed[RawBuffers];
int registration = 1, serial;
bool registering, active = true;
cvar_t *volume;

void StopVoice(Voice &voice)
{
    if (voice.source)
    {
        alSourceStop(voice.source);
        alSourcei(voice.source, AL_BUFFER, 0);
    }
    ALuint source = voice.source;
    voice = {};
    voice.source = source;
}

bool Load(sfx_s *sound)
{
    if (sound->buffer || sound->attempted)
    {
        return sound->buffer != 0;
    }
    sound->attempted = true;
    if (sound->name[0] == '*')
    {
        return false;
    }
    char path[MAX_QPATH + 8];
    snprintf(path, sizeof(path), "%s%s", sound->name[0] == '#' ? "" : "sound/",
             sound->name[0] == '#' ? sound->name + 1 : sound->name);
    void *data = nullptr;
    int size = FS_LoadFile(path, &data);
    dn_wave_t wave;
    if (size <= 0 || !S_ParseWave(data, size_t(size), &wave))
    {
        Com_Printf("Sound: missing or invalid PCM WAV %s\n", path);
        if (data)
        {
            FS_FreeFile(data);
        }
        return false;
    }
    // Positional effects must be mono for OpenAL to spatialize them.
    std::vector<uint8_t> mono;
    const uint8_t *pcm = wave.pcm;
    size_t bytes = wave.bytes;
    if (wave.channels == 2)
    {
        bytes /= 2;
        mono.resize(bytes);
        for (size_t i = 0; i < bytes / wave.width; i++)
        {
            if (wave.width == 1)
            {
                mono[i] = (unsigned(pcm[i * 2]) + pcm[i * 2 + 1]) / 2;
            }
            else
            {
                int16_t pair[2], sample;
                memcpy(pair, pcm + i * 4, 4);
                sample = int16_t((int(pair[0]) + pair[1]) / 2);
                memcpy(mono.data() + i * 2, &sample, 2);
            }
        }
        pcm = mono.data();
    }
    alGetError();
    alGenBuffers(1, &sound->buffer);
    alBufferData(sound->buffer, wave.width == 1 ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16,
                 pcm, ALsizei(bytes), wave.rate);
    FS_FreeFile(data);
    if (alGetError() != AL_NO_ERROR)
    {
        if (sound->buffer)
        {
            alDeleteBuffers(1, &sound->buffer);
        }
        sound->buffer = 0;
        return false;
    }
    return true;
}

Voice *PickVoice(int entity, int channel)
{
    Voice *oldest = nullptr;
    for (auto &voice : voices)
    {
        if (channel && voice.sound && voice.entity == entity && voice.channel == channel)
        {
            return &voice;
        }
    }
    for (auto &voice : voices)
    {
        if (!voice.sound)
        {
            return &voice;
        }
        if (voice.entity == cl.playernum + 1 && entity != cl.playernum + 1)
        {
            continue;
        }
        if (!oldest || voice.serial < oldest->serial)
        {
            oldest = &voice;
        }
    }
    return oldest;
}

void Spatialize(Voice &voice)
{
    bool local = voice.entity == cl.playernum + 1 || voice.attenuation == ATTN_NONE ||
                 cls.state != ca_active;
    vec3_t position = {0, 0, 0};
    if (!local)
    {
        if (voice.fixed)
        {
            VectorCopy(voice.origin, position);
        }
        else if (voice.entity >= 0 && voice.entity < MAX_EDICTS)
        {
            CL_GetEntitySoundOrigin(voice.entity, position);
        }
    }
    alSourcei(voice.source, AL_SOURCE_RELATIVE, local ? AL_TRUE : AL_FALSE);
    // Quake's x-forward/y-left/z-up becomes OpenAL's x-right/y-up/z-back.
    alSource3f(voice.source, AL_POSITION, -position[1], position[2], -position[0]);
    alSourcef(voice.source, AL_GAIN, voice.volume * std::clamp(volume->value, 0.0f, 1.0f));
    alSourcef(voice.source, AL_REFERENCE_DISTANCE, 80.0f);
    float distance = voice.loop ? 1000.0f / 3.0f :
                     1.0f / (voice.attenuation == ATTN_STATIC ? 0.001f : 0.0005f);
    alSourcef(voice.source, AL_MAX_DISTANCE, 80.0f + distance);
    alSourcef(voice.source, AL_ROLLOFF_FACTOR, local ? 0.0f : voice.attenuation);
}

void ReapRaw(void)
{
    ALint processed = 0;
    alGetSourcei(rawSource, AL_BUFFERS_PROCESSED, &processed);
    while (processed-- > 0)
    {
        ALuint buffer;
        alSourceUnqueueBuffers(rawSource, 1, &buffer);
        for (int i = 0; i < RawBuffers; i++)
        {
            if (rawBuffers[i] == buffer)
            {
                rawUsed[i] = false;
            }
        }
    }
}

void PlayCommand(void)
{
    for (int i = 1; i < Cmd_Argc(); i++)
    {
        S_StartLocalSound(Cmd_Argv(i));
    }
}

void SoundInfo(void)
{
    int buffers = 0, playing = 0;
    for (const auto &sound : sounds)
    {
        buffers += sound.buffer != 0;
    }
    for (const auto &voice : voices)
    {
        playing += voice.sound != nullptr;
    }
    Com_Printf("OpenAL: %s, %d WAV buffers, %d/%d voices\n",
               S_Ready() ? alGetString(AL_RENDERER) : "disabled", buffers, playing, VoiceCount);
    for (const auto &voice : voices)
    {
        if (voice.sound && S_IsSoundPlaying(voice.sound))
        {
            Com_Printf("  entity %d channel %d: %s gain %.2f loop %d\n", voice.entity,
                       voice.channel, voice.sound->name, voice.volume, voice.loop);
        }
    }
}

void SoundList(void)
{
    for (const auto &sound : sounds)
    {
        if (sound.name[0])
        {
            Com_Printf("%s %s\n", sound.buffer ? "loaded" : "pending", sound.name);
        }
    }
}
} // namespace

bool S_Ready(void)
{
    return context != nullptr;
}

void S_Init(void)
{
    volume = Cvar_Get("s_volume", "0.7", CVAR_ARCHIVE);
    if (!Cvar_Get("s_initsound", "1", 0)->value || S_Ready())
    {
        return;
    }
    device = alcOpenDevice(nullptr);
    if (device)
    {
        context = alcCreateContext(device, nullptr);
    }
    if (!context || !alcMakeContextCurrent(context))
    {
        S_Shutdown();
        Com_Printf("OpenAL: audio device unavailable\n");
        return;
    }
    alGetError();
    for (auto &voice : voices)
    {
        alGenSources(1, &voice.source);
    }
    alGenSources(1, &rawSource);
    alGenBuffers(RawBuffers, rawBuffers);
    if (alGetError() != AL_NO_ERROR)
    {
        S_Shutdown();
        Com_Printf("OpenAL: unable to allocate voices\n");
        return;
    }
    alSourcei(rawSource, AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcef(rawSource, AL_ROLLOFF_FACTOR, 0);
    alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
    alDopplerFactor(0);
    Cmd_AddCommand("play", PlayCommand);
    Cmd_AddCommand("stopsound", S_StopAllSounds);
    Cmd_AddCommand("soundlist", SoundList);
    Cmd_AddCommand("soundinfo", SoundInfo);
    Com_Printf("OpenAL: %s (%s)\n", alGetString(AL_RENDERER), alGetString(AL_VERSION));
}

void S_Shutdown(void)
{
    S_MusicShutdown();
    if (context)
    {
        S_StopAllSounds();
        for (auto &voice : voices)
        {
            if (voice.source)
            {
                alDeleteSources(1, &voice.source);
            }
            voice = {};
        }
        if (rawSource)
        {
            alDeleteSources(1, &rawSource);
            alDeleteBuffers(RawBuffers, rawBuffers);
        }
        rawSource = 0;
        for (auto &sound : sounds)
        {
            if (sound.buffer)
            {
                alDeleteBuffers(1, &sound.buffer);
            }
            sound = {};
        }
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(context);
        context = nullptr;
    }
    if (device)
    {
        alcCloseDevice(device);
        device = nullptr;
    }
    Cmd_RemoveCommand("play");
    Cmd_RemoveCommand("stopsound");
    Cmd_RemoveCommand("soundlist");
    Cmd_RemoveCommand("soundinfo");
}

sfx_s *S_FindName(char *name, qboolean create)
{
    if (!name || !name[0] || strlen(name) >= MAX_QPATH)
    {
        return nullptr;
    }
    char normalized[MAX_QPATH];
    snprintf(normalized, sizeof(normalized), "%s", name);
    for (char *p = normalized; *p; p++)
    {
        *p = char(tolower(static_cast<unsigned char>(*p)));
        if (*p == '\\')
        {
            *p = '/';
        }
    }
    char *dot = strrchr(normalized, '.');
    if (!dot)
    {
        if (strlen(normalized) + 4 >= sizeof(normalized))
        {
            return nullptr;
        }
        strcat(normalized, ".wav");
    }
    else if (strcmp(dot, ".wav"))
    {
        return nullptr;
    }
    sfx_s *empty = nullptr;
    for (auto &sound : sounds)
    {
        if (!strcmp(sound.name, normalized))
        {
            return &sound;
        }
        if (!sound.name[0] && !empty)
        {
            empty = &sound;
        }
    }
    if (!create || !empty)
    {
        return nullptr;
    }
    snprintf(empty->name, sizeof(empty->name), "%s", normalized);
    return empty;
}

sfx_s *S_RegisterSound(char *name)
{
    if (!S_Ready())
    {
        return nullptr;
    }
    sfx_s *sound = S_FindName(name, true);
    if (sound)
    {
        sound->registration = registration;
        if (!registering)
        {
            Load(sound);
        }
    }
    return sound;
}

void S_BeginRegistration(void)
{
    S_StopAllSounds();
    registration++;
    registering = true;
}

void S_EndRegistration(void)
{
    registering = false;
    if (!S_Ready())
    {
        return;
    }
    for (auto &sound : sounds)
    {
        if (!sound.name[0])
        {
            continue;
        }
        if (sound.registration == registration)
        {
            Load(&sound);
        }
        else
        {
            if (sound.buffer)
            {
                alDeleteBuffers(1, &sound.buffer);
            }
            sound = {};
        }
    }
}

void S_StartSound(vec3_t origin, int entnum, int entchannel, sfx_s *sound, float gain,
                  float attenuation, float timeofs)
{
    if (!S_Ready() || !sound || entchannel < 0 || !Load(sound))
    {
        return;
    }
    Voice *voice = PickVoice(entnum, entchannel);
    if (!voice)
    {
        return;
    }
    StopVoice(*voice);
    voice->sound = sound;
    voice->entity = entnum;
    voice->channel = entchannel;
    voice->volume = std::clamp(gain, 0.0f, 1.0f);
    voice->attenuation = std::clamp(attenuation, 0.0f, 4.0f);
    voice->begin = cls.realtime + int(std::clamp(timeofs, 0.0f, 60.0f) * 1000);
    voice->serial = ++serial;
    voice->fixed = origin != nullptr;
    if (origin)
    {
        VectorCopy(origin, voice->origin);
    }
    alSourcei(voice->source, AL_BUFFER, sound->buffer);
    alSourcei(voice->source, AL_LOOPING, AL_FALSE);
    Spatialize(*voice);
    if (voice->begin <= cls.realtime)
    {
        alSourcePlay(voice->source);
        voice->started = true;
    }
}

void S_StartLocalSound(const char *name)
{
    S_StartSound(nullptr, cl.playernum + 1, CHAN_AUTO, S_RegisterSound(const_cast<char *>(name)), 1,
                 ATTN_NONE, 0);
}

void S_AmbientLoop(int marker, const char *name, float gain)
{
    int entity = -marker - 1;
    sfx_s *sound = S_RegisterSound(const_cast<char *>(name));
    Voice *loop = nullptr;
    for (auto &voice : voices)
    {
        if (voice.loop && voice.entity == entity && voice.sound == sound)
        {
            loop = &voice;
            break;
        }
    }
    if (!loop)
    {
        S_StartSound(nullptr, entity, CHAN_AUTO, sound, gain, ATTN_NONE, 0);
        for (auto &voice : voices)
        {
            if (voice.sound == sound && voice.entity == entity && voice.serial == serial)
            {
                loop = &voice;
                loop->loop = true;
                alSourcei(loop->source, AL_LOOPING, AL_TRUE);
                break;
            }
        }
    }
    if (loop)
    {
        loop->volume = gain;
        loop->seen = true;
    }
}

qboolean S_IsSoundPlaying(const sfx_s *sound)
{
    if (!S_Ready() || !sound)
    {
        return false;
    }
    for (const auto &voice : voices)
    {
        if (voice.sound == sound)
        {
            ALint state;
            alGetSourcei(voice.source, AL_SOURCE_STATE, &state);
            if (!voice.started || state == AL_PLAYING || state == AL_PAUSED)
            {
                return true;
            }
        }
    }
    return false;
}

void S_StopEntitySounds(int entity, int channel)
{
    if (!S_Ready())
    {
        return;
    }
    for (auto &voice : voices)
    {
        if (voice.entity == entity && (channel == CHAN_AUTO || voice.channel == channel))
        {
            StopVoice(voice);
        }
    }
}

void S_StopAllSounds(void)
{
    S_ResetAmbience();
    if (!S_Ready())
    {
        return;
    }
    for (auto &voice : voices)
    {
        StopVoice(voice);
    }
    if (rawSource)
    {
        alSourceStop(rawSource);
        ReapRaw();
    }
}

void S_RawSamples(int samples, int rate, int width, int channels, byte *data)
{
    if (!S_Ready() || !data || samples <= 0 || samples > 1048576 || rate < 1 ||
        rate > 192000 || (width != 1 && width != 2) || (channels != 1 && channels != 2))
    {
        return;
    }
    ReapRaw();
    for (int i = 0; i < RawBuffers; i++)
    {
        if (rawUsed[i])
        {
            continue;
        }
        ALenum format = channels == 1 ? (width == 1 ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16) :
                                       (width == 1 ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16);
        alBufferData(rawBuffers[i], format, data, samples * width * channels, rate);
        alSourceQueueBuffers(rawSource, 1, &rawBuffers[i]);
        rawUsed[i] = true;
        ALint state;
        alGetSourcei(rawSource, AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING)
        {
            alSourcePlay(rawSource);
        }
        break;
    }
}

void S_Activate(qboolean enabled)
{
    active = enabled != 0;
    if (S_Ready())
    {
        alListenerf(AL_GAIN, active ? 1.0f : 0.0f);
    }
}

void S_Update(vec3_t origin, vec3_t forward, vec3_t right, vec3_t up)
{
    if (!S_Ready())
    {
        return;
    }
    ALfloat orientation[] = {-forward[1], forward[2], -forward[0], -up[1], up[2], -up[0]};
    alListener3f(AL_POSITION, -origin[1], origin[2], -origin[0]);
    alListenerfv(AL_ORIENTATION, orientation);
    alListenerf(AL_GAIN, active ? 1.0f : 0.0f);
    for (auto &voice : voices)
    {
        voice.seen = false;
        if (voice.sound && voice.started)
        {
            ALint state;
            alGetSourcei(voice.source, AL_SOURCE_STATE, &state);
            if (state == AL_STOPPED)
            {
                StopVoice(voice);
            }
        }
    }
    if (cls.state == ca_active && cl.sound_prepped && !cl_paused->value)
    {
        if (!cl.frame.playerstate.stats[DN_STAT_INTERMISSION])
        {
            S_UpdateAmbience(origin);
        }
        for (int i = 0; i < cl.frame.num_entities; i++)
        {
            entity_state_t *entity = &cl_parse_entities[(cl.frame.parse_entities + i) &
                                                       (MAX_PARSE_ENTITIES - 1)];
            if (entity->sound <= 0 || entity->sound >= MAX_SOUNDS)
            {
                continue;
            }
            sfx_s *sound = cl.sound_precache[entity->sound];
            if (!sound || !Load(sound))
            {
                continue;
            }
            Voice *loop = nullptr;
            for (auto &voice : voices)
            {
                if (voice.loop && voice.entity == entity->number && voice.sound == sound)
                {
                    loop = &voice;
                    break;
                }
            }
            if (!loop)
            {
                S_StartSound(entity->origin, entity->number, 0, sound, 1, ATTN_STATIC, 0);
                for (auto &voice : voices)
                {
                    if (voice.serial == serial && voice.sound == sound)
                    {
                        loop = &voice;
                        loop->loop = true;
                        alSourcei(loop->source, AL_LOOPING, AL_TRUE);
                        break;
                    }
                }
            }
            if (loop)
            {
                loop->seen = true;
                VectorCopy(entity->origin, loop->origin);
            }
        }
    }
    for (auto &voice : voices)
    {
        if (voice.loop && !voice.seen)
        {
            StopVoice(voice);
        }
        if (voice.sound)
        {
            Spatialize(voice);
            if (!voice.started && voice.begin <= cls.realtime)
            {
                alSourcePlay(voice.source);
                voice.started = true;
            }
        }
    }
    alSourcef(rawSource, AL_GAIN, std::clamp(volume->value, 0.0f, 1.0f));
    ReapRaw();
}

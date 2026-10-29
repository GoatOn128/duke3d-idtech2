// XAudio2.cpp
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xaudio2.h>
#include <mmsystem.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <mutex>
#include <algorithm>
#include <string>

#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ole32.lib")

#ifndef XAUDIO2_DLL
#define XAUDIO2_DLL L"xaudio2_9.dll"
#endif

// -----------------------------------------------------------------------------
// Original Duke3D-compatible declarations
// -----------------------------------------------------------------------------

typedef struct
{
    long  MaxVoices;
    long  MaxSampleBits;
    long  MaxChannels;
} fx_device;

#define MonoFx   1
#define StereoFx 2

typedef struct
{
    unsigned long Address;
    unsigned long Type;
    unsigned long Interrupt;
    unsigned long Dma8;
    unsigned long Dma16;
    unsigned long Midi;
    unsigned long Emu;
} fx_blaster_config;

enum FX_ERRORS
{
    FX_Warning = -2,
    FX_Error = -1,
    FX_Ok = 0,
    FX_ASSVersion,
    FX_BlasterError,
    FX_SoundCardError,
    FX_InvalidCard,
    FX_MultiVocError,
    FX_DPMI_Error
};

enum fx_BLASTER_Types
{
    fx_SB = 1,
    fx_SBPro = 2,
    fx_SB20 = 3,
    fx_SBPro2 = 4,
    fx_SB16 = 6
};

typedef struct
{
    unsigned long tickposition;
    unsigned long milliseconds;
    unsigned int  measure;
    unsigned int  beat;
    unsigned int  tick;
} songposition;

#define MUSIC_LoopSong ( 1 == 1 )
#define MUSIC_PlayOnce ( !MUSIC_LoopSong )

enum MUSIC_ERRORS
{
    MUSIC_Warning = -2,
    MUSIC_Error = -1,
    MUSIC_Ok = 0,
    MUSIC_ASSVersion,
    MUSIC_SoundCardError,
    MUSIC_MPU401Error,
    MUSIC_InvalidCard,
    MUSIC_MidiError,
    MUSIC_TaskManError,
    MUSIC_FMNotDetected,
    MUSIC_DPMI_Error
};

int MUSIC_ErrorCode = MUSIC_Ok;

// -----------------------------------------------------------------------------
// Globals
// -----------------------------------------------------------------------------

static IXAudio2* g_xaudio = NULL;
static IXAudio2MasteringVoice* g_masterVoice = NULL;
static int g_fxError = FX_Ok;
static int g_fxInitialized = 0;
static int g_fxMaxVoices = 32;
static int g_fxVolume = 255;
static int g_fxReverseStereo = 0;
static int g_fxReverb = 0;
static int g_fxReverbDelay = 0;
static unsigned g_fxMixRate = 44100;
static int g_nextHandle = 1;
static void (*g_fxCallback)(unsigned long) = NULL;
static std::mutex g_fxMutex;

static HANDLE g_fxThread = NULL;
static volatile LONG g_fxThreadQuit = 0;
static volatile LONG g_fxThreadRunning = 0;

static int g_musicVolume = 255;
static int g_musicLoop = 0;
static int g_musicContext = 0;
static int g_musicPaused = 0;
static int g_musicPlaying = 0;
static std::string g_musicAlias = "duke3d_music_mci";
static char g_musicTempPath[MAX_PATH] = { 0 };

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

static float clampf(float v, float a, float b)
{
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

static int clampi(int v, int a, int b)
{
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

static uint32_t read_u32le(const unsigned char* p)
{
    return ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t read_u16le(const unsigned char* p)
{
    return (uint16_t)(((uint16_t)p[0]) | ((uint16_t)p[1] << 8));
}

static uint32_t read_u24le(const unsigned char* p)
{
    return ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
}

static uint16_t read_u16be(const unsigned char* p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

static uint32_t read_u32be(const unsigned char* p)
{
    return ((uint32_t)p[0] << 24) |
        ((uint32_t)p[1] << 16) |
        ((uint32_t)p[2] << 8) |
        ((uint32_t)p[3]);
}

static float volume_to_float(int v)
{
    return clampf((float)clampi(v, 0, 255) / 255.0f, 0.0f, 1.0f);
}

static float pitch_to_ratio(int pitchoffset)
{
    return (float)pow(2.0, (double)pitchoffset / 1200.0);
}

static void make_pcm_format(WAVEFORMATEX* fmt, int channels, int bits, int rate)
{
    memset(fmt, 0, sizeof(*fmt));
    fmt->wFormatTag = WAVE_FORMAT_PCM;
    fmt->nChannels = (WORD)channels;
    fmt->nSamplesPerSec = (DWORD)rate;
    fmt->wBitsPerSample = (WORD)bits;
    fmt->nBlockAlign = (WORD)((channels * bits) / 8);
    fmt->nAvgBytesPerSec = fmt->nSamplesPerSec * fmt->nBlockAlign;
    fmt->cbSize = 0;
}

static bool is_probably_riff_wav(const char* ptr)
{
    if (!ptr) return false;
    const unsigned char* p = (const unsigned char*)ptr;
    return memcmp(p, "RIFF", 4) == 0 && memcmp(p + 8, "WAVE", 4) == 0;
}

static bool is_probably_voc(const char* ptr)
{
    if (!ptr) return false;
    return memcmp(ptr, "Creative Voice File", 19) == 0;
}

static int MUSIC_GetMidiLengthSafe(unsigned char* song, DWORD* outLen)
{
    if (!song || !outLen)
        return 0;

    *outLen = 0;

    __try
    {
        if (memcmp(song, "MThd", 4) != 0)
            return 0;

        DWORD headerLen = read_u32be(song + 4);

        if (headerLen < 6 || headerLen > 1024)
            return 0;

        unsigned short numTracks = read_u16be(song + 10);

        if (numTracks == 0 || numTracks > 256)
            return 0;

        const DWORD MAX_MIDI_SIZE = 16 * 1024 * 1024;

        const unsigned char* p = song + 8 + headerLen;
        DWORD midiLen = 8 + headerLen;

        for (unsigned short i = 0; i < numTracks; i++)
        {
            if (memcmp(p, "MTrk", 4) != 0)
                return 0;

            DWORD trkLen = read_u32be(p + 4);

            if (trkLen > MAX_MIDI_SIZE)
                return 0;

            if (midiLen > MAX_MIDI_SIZE - 8)
                return 0;

            if (trkLen > MAX_MIDI_SIZE - midiLen - 8)
                return 0;

            midiLen += 8 + trkLen;
            p += 8 + trkLen;
        }

        *outLen = midiLen;
        return 1;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

struct ParsedSound
{
    WAVEFORMATEX fmt;
    std::vector<unsigned char> data;
    unsigned int loopBeginBytes;
    unsigned int loopLengthBytes;
    bool loop;

    ParsedSound()
    {
        memset(&fmt, 0, sizeof(fmt));
        loopBeginBytes = 0;
        loopLengthBytes = 0;
        loop = false;
    }
};

static bool parse_wav(const char* ptr, ParsedSound& out)
{
    if (!ptr || !is_probably_riff_wav(ptr)) return false;

    const unsigned char* base = (const unsigned char*)ptr;
    uint32_t riffSize = read_u32le(base + 4);
    const unsigned char* end = base + 8 + riffSize;
    const unsigned char* p = base + 12;

    bool gotFmt = false;
    bool gotData = false;
    WAVEFORMATEX fmt;
    memset(&fmt, 0, sizeof(fmt));
    const unsigned char* dataPtr = NULL;
    uint32_t dataSize = 0;

    while (p + 8 <= end)
    {
        uint32_t id = read_u32le(p);
        uint32_t size = read_u32le(p + 4);
        const unsigned char* chunk = p + 8;

        if (chunk + size > end) break;

        if (id == mmioFOURCC('f', 'm', 't', ' '))
        {
            if (size >= 16)
            {
                fmt.wFormatTag = read_u16le(chunk + 0);
                fmt.nChannels = read_u16le(chunk + 2);
                fmt.nSamplesPerSec = read_u32le(chunk + 4);
                fmt.nAvgBytesPerSec = read_u32le(chunk + 8);
                fmt.nBlockAlign = read_u16le(chunk + 12);
                fmt.wBitsPerSample = read_u16le(chunk + 14);
                fmt.cbSize = 0;
                gotFmt = true;
            }
        }
        else if (id == mmioFOURCC('d', 'a', 't', 'a'))
        {
            dataPtr = chunk;
            dataSize = size;
            gotData = true;
        }

        p = chunk + size + (size & 1);
    }

    if (!gotFmt || !gotData) return false;
    if (fmt.wFormatTag != WAVE_FORMAT_PCM) return false;
    if (fmt.nChannels != 1 && fmt.nChannels != 2) return false;
    if (fmt.wBitsPerSample != 8 && fmt.wBitsPerSample != 16) return false;
    if (fmt.nBlockAlign == 0) return false;

    out.fmt = fmt;
    out.data.assign(dataPtr, dataPtr + dataSize);
    return true;
}

static bool parse_voc(const char* ptr, ParsedSound& out)
{
    if (!ptr || !is_probably_voc(ptr)) return false;

    const unsigned char* base = (const unsigned char*)ptr;
    uint16_t dataOffset = read_u16le(base + 20);
    const unsigned char* p = base + dataOffset;

    std::vector<unsigned char> pcm;
    int rate = 11025;
    int bits = 8;
    int channels = 1;


    {
        for (;;)
        {
            unsigned char blockType = *p++;
            if (blockType == 0) break;

            uint32_t blockSize = read_u24le(p);
            p += 3;

            if (blockType == 1)
            {
                if (blockSize < 2) break;
                unsigned char timeConstant = p[0];
                unsigned char packMethod = p[1];
                if (packMethod == 0)
                {
                    rate = 1000000 / (256 - timeConstant);
                    bits = 8;
                    channels = 1;
                    pcm.insert(pcm.end(), p + 2, p + blockSize);
                }
            }
            else if (blockType == 2)
            {
                pcm.insert(pcm.end(), p, p + blockSize);
            }
            else if (blockType == 8)
            {
                if (blockSize >= 4)
                {
                    uint16_t tc = read_u16le(p + 0);
                    unsigned char pack = p[2];
                    unsigned char mode = p[3];
                    if (pack == 0)
                    {
                        channels = mode + 1;
                        rate = 256000000 / ((65536 - tc) * channels);
                        bits = 8;
                    }
                }
            }
            else if (blockType == 9)
            {
                if (blockSize >= 12)
                {
                    rate = (int)read_u32le(p + 0);
                    bits = p[4];
                    channels = p[5];
                    uint16_t format = read_u16le(p + 6);
                    if (format == 0 && (bits == 8 || bits == 16) && (channels == 1 || channels == 2))
                    {
                        pcm.insert(pcm.end(), p + 12, p + blockSize);
                    }
                }
            }

            p += blockSize;
        }
    }


    if (pcm.empty()) return false;
    make_pcm_format(&out.fmt, channels, bits, rate);
    out.data.swap(pcm);
    return true;
}

class DukeVoiceCallback : public IXAudio2VoiceCallback
{
public:
    int handle;

    DukeVoiceCallback()
    {
        handle = 0;
    }

    STDMETHOD_(void, OnVoiceProcessingPassStart)(UINT32) {}
    STDMETHOD_(void, OnVoiceProcessingPassEnd)() {}
    STDMETHOD_(void, OnStreamEnd)();
    STDMETHOD_(void, OnBufferStart)(void*) {}
    STDMETHOD_(void, OnBufferEnd)(void*) {}
    STDMETHOD_(void, OnLoopEnd)(void*) {}
    STDMETHOD_(void, OnVoiceError)(void*, HRESULT) {}
};

struct FxVoice
{
    int handle;
    int priority;
    int active;
    int finished;
    int vol;
    int left;
    int right;
    int pitchoffset;
    unsigned long callbackval;
    float baseFrequency;
    WAVEFORMATEX fmt;
    std::vector<unsigned char> data;
    IXAudio2SourceVoice* voice;
    DukeVoiceCallback* callback;

    FxVoice()
    {
        handle = 0;
        priority = 0;
        active = 0;
        finished = 0;
        vol = 255;
        left = 255;
        right = 255;
        pitchoffset = 0;
        callbackval = 0;
        baseFrequency = 44100.0f;
        memset(&fmt, 0, sizeof(fmt));
        voice = NULL;
        callback = NULL;
    }
};

static std::vector<FxVoice*> g_voices;

static FxVoice* find_voice_locked(int handle)
{
    for (size_t i = 0; i < g_voices.size(); ++i)
    {
        if (g_voices[i] && g_voices[i]->handle == handle)
            return g_voices[i];
    }
    return NULL;
}

static void destroy_voice(FxVoice* v, int callCallback)
{
    if (!v) return;

    if (v->voice)
    {
        v->voice->Stop(0);
        v->voice->FlushSourceBuffers();
        v->voice->DestroyVoice();
        v->voice = NULL;
    }

    if (callCallback && g_fxCallback)
        g_fxCallback(v->callbackval);

    if (v->callback)
    {
        delete v->callback;
        v->callback = NULL;
    }

    delete v;
}

static void cleanup_finished_locked()
{
    for (size_t i = 0; i < g_voices.size(); )
    {
        FxVoice* v = g_voices[i];
        if (v && v->finished)
        {
            if (g_fxCallback)
                g_fxCallback(v->callbackval);

            if (v->voice)
            {
                v->voice->DestroyVoice();
                v->voice = NULL;
            }

            if (v->callback)
                delete v->callback;

            delete v;
            g_voices.erase(g_voices.begin() + i);
        }
        else
        {
            ++i;
        }
    }
}

void DukeVoiceCallback::OnStreamEnd()
{
    std::lock_guard<std::mutex> lock(g_fxMutex);
    FxVoice* v = find_voice_locked(handle);
    if (v)
        v->finished = 1;
}

static void apply_voice_matrix_locked(FxVoice* v)
{
    if (!v || !v->voice || !g_masterVoice) return;

    float master = volume_to_float(g_fxVolume);
    float vol = volume_to_float(v->vol);
    float l = volume_to_float(v->left);
    float r = volume_to_float(v->right);

    if (g_fxReverseStereo)
    {
        float t = l;
        l = r;
        r = t;
    }

    float left = master * vol * l;
    float right = master * vol * r;

    if (v->fmt.nChannels == 1)
    {
        float matrix[2] = { left, right };
        v->voice->SetOutputMatrix(g_masterVoice, 1, 2, matrix);
    }
    else if (v->fmt.nChannels == 2)
    {
        float matrix[4] =
        {
            left, 0.0f,
            0.0f, right
        };
        v->voice->SetOutputMatrix(g_masterVoice, 2, 2, matrix);
    }
}

static int steal_or_check_voice_locked(int priority)
{
    cleanup_finished_locked();

    if ((int)g_voices.size() < g_fxMaxVoices)
        return 1;

    FxVoice* lowest = NULL;
    for (size_t i = 0; i < g_voices.size(); ++i)
    {
        FxVoice* v = g_voices[i];
        if (!lowest || v->priority < lowest->priority)
            lowest = v;
    }

    if (lowest && lowest->priority <= priority)
    {
        int oldHandle = lowest->handle;
        for (size_t i = 0; i < g_voices.size(); ++i)
        {
            if (g_voices[i]->handle == oldHandle)
            {
                destroy_voice(g_voices[i], 1);
                g_voices.erase(g_voices.begin() + i);
                break;
            }
        }
        return 1;
    }

    return 0;
}

static int submit_sound(const ParsedSound& snd, int pitchoffset, int vol, int left, int right, int priority, unsigned long callbackval)
{
    if (!g_fxInitialized || !g_xaudio)
        return FX_Error;

    std::lock_guard<std::mutex> lock(g_fxMutex);

    if (!steal_or_check_voice_locked(priority))
        return FX_Warning;

    FxVoice* v = new FxVoice;
    v->handle = g_nextHandle++;
    if (g_nextHandle <= 0) g_nextHandle = 1;
    v->priority = priority;
    v->vol = clampi(vol, 0, 255);
    v->left = clampi(left, 0, 255);
    v->right = clampi(right, 0, 255);
    v->pitchoffset = pitchoffset;
    v->callbackval = callbackval;
    v->fmt = snd.fmt;
    v->data = snd.data;
    v->baseFrequency = (float)snd.fmt.nSamplesPerSec;
    v->callback = new DukeVoiceCallback;
    v->callback->handle = v->handle;

    HRESULT hr = g_xaudio->CreateSourceVoice(&v->voice, &v->fmt, 0, XAUDIO2_MAX_FREQ_RATIO, v->callback);
    if (FAILED(hr))
    {
        delete v->callback;
        delete v;
        g_fxError = FX_SoundCardError;
        return FX_Error;
    }

    XAUDIO2_BUFFER buf;
    memset(&buf, 0, sizeof(buf));
    buf.AudioBytes = (UINT32)v->data.size();
    buf.pAudioData = v->data.data();
    buf.Flags = XAUDIO2_END_OF_STREAM;

    if (snd.loop)
    {
        unsigned int blockAlign = v->fmt.nBlockAlign ? v->fmt.nBlockAlign : 1;
        buf.LoopBegin = snd.loopBeginBytes / blockAlign;
        buf.LoopLength = snd.loopLengthBytes / blockAlign;
        if (buf.LoopLength == 0)
            buf.LoopLength = (UINT32)(v->data.size() / blockAlign) - buf.LoopBegin;
        buf.LoopCount = XAUDIO2_LOOP_INFINITE;
        buf.Flags = 0;
    }

    hr = v->voice->SubmitSourceBuffer(&buf);
    if (FAILED(hr))
    {
        v->voice->DestroyVoice();
        delete v->callback;
        delete v;
        g_fxError = FX_MultiVocError;
        return FX_Error;
    }

    float ratio = pitch_to_ratio(pitchoffset);
    ratio = clampf(ratio, XAUDIO2_MIN_FREQ_RATIO, XAUDIO2_MAX_FREQ_RATIO);
    v->voice->SetFrequencyRatio(ratio);

    apply_voice_matrix_locked(v);

    hr = v->voice->Start(0);
    if (FAILED(hr))
    {
        v->voice->DestroyVoice();
        delete v->callback;
        delete v;
        g_fxError = FX_MultiVocError;
        return FX_Error;
    }

    v->active = 1;
    g_voices.push_back(v);
    return v->handle;
}

static void pan_from_3d(int angle, int distance, int* vol, int* left, int* right)
{
    distance = clampi(distance, 0, 255);

    float distVol = 1.0f - ((float)distance / 255.0f);
    distVol = clampf(distVol, 0.0f, 1.0f);

    float a = ((float)(angle & 2047) / 2048.0f) * 6.28318530718f;
    float pan = sinf(a);

    float l = (pan <= 0.0f) ? 1.0f : 1.0f - pan;
    float r = (pan >= 0.0f) ? 1.0f : 1.0f + pan;

    *vol = (int)(255.0f * distVol);
    *left = clampi((int)(255.0f * l), 0, 255);
    *right = clampi((int)(255.0f * r), 0, 255);
}

// -----------------------------------------------------------------------------
// FX update/thread pump
// -----------------------------------------------------------------------------
//
// XAudio2 does its own mixing on its own engine thread, but this wrapper still
// needs a small maintenance pump for Duke-style callback cleanup and to keep
// dead/finished voices from piling up if the game never calls FX_SoundsPlaying()
// or FX_SoundActive(). Call FX_Update() once per frame if you want, but the
// thread started by FX_Init() also calls it automatically.

void FX_Update(void)
{
    if (!g_fxInitialized)
        return;

    std::lock_guard<std::mutex> lock(g_fxMutex);
    cleanup_finished_locked();
}

static DWORD WINAPI FX_SoundThreadProc(LPVOID)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    InterlockedExchange(&g_fxThreadRunning, 1);

    while (InterlockedCompareExchange(&g_fxThreadQuit, 0, 0) == 0)
    {
        FX_Update();
        Sleep(10);
    }

    InterlockedExchange(&g_fxThreadRunning, 0);
    CoUninitialize();
    return 0;
}

static void FX_StartSoundThread(void)
{
    if (g_fxThread)
        return;

    InterlockedExchange(&g_fxThreadQuit, 0);
    g_fxThread = CreateThread(NULL, 0, FX_SoundThreadProc, NULL, 0, NULL);
}

static void FX_StopSoundThread(void)
{
    HANDLE h = g_fxThread;
    if (!h)
        return;

    InterlockedExchange(&g_fxThreadQuit, 1);
    WaitForSingleObject(h, INFINITE);
    CloseHandle(h);
    g_fxThread = NULL;
}

// -----------------------------------------------------------------------------
// FX API
// -----------------------------------------------------------------------------

char* FX_ErrorString(int ErrorNumber)
{
    switch (ErrorNumber)
    {
    case FX_Warning: return (char*)"FX warning";
    case FX_Error: return (char*)"FX error";
    case FX_Ok: return (char*)"FX ok";
    case FX_ASSVersion: return (char*)"FX ASS version mismatch";
    case FX_BlasterError: return (char*)"FX Sound Blaster config error";
    case FX_SoundCardError: return (char*)"FX sound card error";
    case FX_InvalidCard: return (char*)"FX invalid card";
    case FX_MultiVocError: return (char*)"FX MultiVoc/XAudio2 error";
    case FX_DPMI_Error: return (char*)"FX DPMI error";
    default: return (char*)"FX unknown error";
    }
}

int FX_SetupCard(int SoundCard, fx_device* device)
{
    (void)SoundCard;
    if (device)
    {
        device->MaxVoices = 64;
        device->MaxSampleBits = 16;
        device->MaxChannels = 2;
    }
    return FX_Ok;
}

int FX_GetBlasterSettings(fx_blaster_config* blaster)
{
    if (blaster)
    {
        memset(blaster, 0, sizeof(*blaster));
        blaster->Type = fx_SB16;
        blaster->Address = 0x220;
        blaster->Interrupt = 5;
        blaster->Dma8 = 1;
        blaster->Dma16 = 5;
        blaster->Midi = 0x330;
    }
    return FX_Ok;
}

int FX_SetupSoundBlaster(fx_blaster_config blaster, int* MaxVoices, int* MaxSampleBits, int* MaxChannels)
{
    (void)blaster;
    if (MaxVoices) *MaxVoices = 64;
    if (MaxSampleBits) *MaxSampleBits = 16;
    if (MaxChannels) *MaxChannels = 2;
    return FX_Ok;
}

int FX_Init(int SoundCard, int numvoices, int numchannels, int samplebits, unsigned mixrate)
{
    (void)SoundCard;
    (void)numchannels;
    (void)samplebits;

    if (g_fxInitialized)
        return FX_Ok;

    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    (void)hr;

    hr = XAudio2Create(&g_xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr))
    {
        g_fxError = FX_SoundCardError;
        return FX_Error;
    }

    hr = g_xaudio->CreateMasteringVoice(&g_masterVoice, 2, mixrate ? mixrate : XAUDIO2_DEFAULT_SAMPLERATE);
    if (FAILED(hr))
    {
        g_xaudio->Release();
        g_xaudio = NULL;
        g_fxError = FX_SoundCardError;
        return FX_Error;
    }

    hr = g_xaudio->StartEngine();
    if (FAILED(hr))
    {
        g_masterVoice->DestroyVoice();
        g_masterVoice = NULL;
        g_xaudio->Release();
        g_xaudio = NULL;
        g_fxError = FX_SoundCardError;
        return FX_Error;
    }

    g_fxMaxVoices = numvoices > 0 ? numvoices : 32;
    g_fxMixRate = mixrate ? mixrate : 44100;
    g_fxInitialized = 1;
    g_fxError = FX_Ok;

    FX_StartSoundThread();

    return FX_Ok;
}

int FX_Shutdown(void)
{
    FX_StopSoundThread();

    std::lock_guard<std::mutex> lock(g_fxMutex);

    for (size_t i = 0; i < g_voices.size(); ++i)
        destroy_voice(g_voices[i], 0);
    g_voices.clear();

    if (g_masterVoice)
    {
        g_masterVoice->DestroyVoice();
        g_masterVoice = NULL;
    }

    if (g_xaudio)
    {
        g_xaudio->StopEngine();
        g_xaudio->Release();
        g_xaudio = NULL;
    }

    g_fxInitialized = 0;
    return FX_Ok;
}

int FX_SetCallBack(void (*function)(unsigned long))
{
    g_fxCallback = function;
    return FX_Ok;
}

void FX_SetVolume(int volume)
{
    std::lock_guard<std::mutex> lock(g_fxMutex);
    g_fxVolume = clampi(volume, 0, 255);
    for (size_t i = 0; i < g_voices.size(); ++i)
        apply_voice_matrix_locked(g_voices[i]);
}

int FX_GetVolume(void)
{
    return g_fxVolume;
}

void FX_SetReverseStereo(int setting)
{
    std::lock_guard<std::mutex> lock(g_fxMutex);
    g_fxReverseStereo = setting ? 1 : 0;
    for (size_t i = 0; i < g_voices.size(); ++i)
        apply_voice_matrix_locked(g_voices[i]);
}

int FX_GetReverseStereo(void)
{
    return g_fxReverseStereo;
}

void FX_SetReverb(int reverb)
{
    g_fxReverb = reverb;
}

void FX_SetFastReverb(int reverb)
{
    g_fxReverb = reverb;
}

int FX_GetMaxReverbDelay(void)
{
    return 255;
}

int FX_GetReverbDelay(void)
{
    return g_fxReverbDelay;
}

void FX_SetReverbDelay(int delay)
{
    g_fxReverbDelay = clampi(delay, 0, 255);
}

int FX_VoiceAvailable(int priority)
{
    std::lock_guard<std::mutex> lock(g_fxMutex);
    return steal_or_check_voice_locked(priority);
}

int FX_SetPan(int handle, int vol, int left, int right)
{
    std::lock_guard<std::mutex> lock(g_fxMutex);
    FxVoice* v = find_voice_locked(handle);
    if (!v) return FX_Error;
    v->vol = clampi(vol, 0, 255);
    v->left = clampi(left, 0, 255);
    v->right = clampi(right, 0, 255);
    apply_voice_matrix_locked(v);
    return FX_Ok;
}

int FX_SetPitch(int handle, int pitchoffset)
{
    std::lock_guard<std::mutex> lock(g_fxMutex);
    FxVoice* v = find_voice_locked(handle);
    if (!v || !v->voice) return FX_Error;
    v->pitchoffset = pitchoffset;
    v->voice->SetFrequencyRatio(clampf(pitch_to_ratio(pitchoffset), XAUDIO2_MIN_FREQ_RATIO, XAUDIO2_MAX_FREQ_RATIO));
    return FX_Ok;
}

int FX_SetFrequency(int handle, int frequency)
{
    std::lock_guard<std::mutex> lock(g_fxMutex);
    FxVoice* v = find_voice_locked(handle);
    if (!v || !v->voice || frequency <= 0) return FX_Error;
    float ratio = (float)frequency / v->baseFrequency;
    v->voice->SetFrequencyRatio(clampf(ratio, XAUDIO2_MIN_FREQ_RATIO, XAUDIO2_MAX_FREQ_RATIO));
    return FX_Ok;
}

int FX_PlayWAV(char* ptr, int pitchoffset, int vol, int left, int right, int priority, unsigned long callbackval)
{
    ParsedSound snd;
    if (!parse_wav(ptr, snd)) return FX_Error;
    return submit_sound(snd, pitchoffset, vol, left, right, priority, callbackval);
}

int FX_PlayLoopedWAV(char* ptr, long loopstart, long loopend, int pitchoffset, int vol, int left, int right, int priority, unsigned long callbackval)
{
    ParsedSound snd;
    if (!parse_wav(ptr, snd)) return FX_Error;

    snd.loop = true;
    if (loopstart < 0) loopstart = 0;
    if (loopend <= loopstart || loopend > (long)snd.data.size()) loopend = (long)snd.data.size();
    snd.loopBeginBytes = (unsigned int)loopstart;
    snd.loopLengthBytes = (unsigned int)(loopend - loopstart);

    return submit_sound(snd, pitchoffset, vol, left, right, priority, callbackval);
}

int FX_PlayVOC(char* ptr, int pitchoffset, int vol, int left, int right, int priority, unsigned long callbackval)
{
    ParsedSound snd;
    if (!parse_voc(ptr, snd)) return FX_Error;
    return submit_sound(snd, pitchoffset, vol, left, right, priority, callbackval);
}

int FX_PlayLoopedVOC(char* ptr, long loopstart, long loopend, int pitchoffset, int vol, int left, int right, int priority, unsigned long callbackval)
{
    ParsedSound snd;
    if (!parse_voc(ptr, snd)) return FX_Error;

    snd.loop = true;
    if (loopstart < 0) loopstart = 0;
    if (loopend <= loopstart || loopend > (long)snd.data.size()) loopend = (long)snd.data.size();
    snd.loopBeginBytes = (unsigned int)loopstart;
    snd.loopLengthBytes = (unsigned int)(loopend - loopstart);

    return submit_sound(snd, pitchoffset, vol, left, right, priority, callbackval);
}

int FX_PlayVOC3D(char* ptr, int pitchoffset, int angle, int distance, int priority, unsigned long callbackval)
{
    int vol, left, right;
    pan_from_3d(angle, distance, &vol, &left, &right);
    return FX_PlayVOC(ptr, pitchoffset, vol, left, right, priority, callbackval);
}

int FX_PlayWAV3D(char* ptr, int pitchoffset, int angle, int distance, int priority, unsigned long callbackval)
{
    int vol, left, right;
    pan_from_3d(angle, distance, &vol, &left, &right);
    return FX_PlayWAV(ptr, pitchoffset, vol, left, right, priority, callbackval);
}

int FX_PlayRaw(char* ptr, unsigned long length, unsigned rate, int pitchoffset, int vol, int left, int right, int priority, unsigned long callbackval)
{
    if (!ptr || length == 0) return FX_Error;

    ParsedSound snd;
    make_pcm_format(&snd.fmt, 1, 8, rate ? rate : 11025);
    snd.data.assign((unsigned char*)ptr, (unsigned char*)ptr + length);

    return submit_sound(snd, pitchoffset, vol, left, right, priority, callbackval);
}

int FX_PlayLoopedRaw(char* ptr, unsigned long length, char* loopstart, char* loopend, unsigned rate, int pitchoffset, int vol, int left, int right, int priority, unsigned long callbackval)
{
    if (!ptr || length == 0) return FX_Error;

    ParsedSound snd;
    make_pcm_format(&snd.fmt, 1, 8, rate ? rate : 11025);
    snd.data.assign((unsigned char*)ptr, (unsigned char*)ptr + length);
    snd.loop = true;

    unsigned long ls = 0;
    unsigned long le = length;

    if (loopstart && loopstart >= ptr && loopstart < ptr + length)
        ls = (unsigned long)(loopstart - ptr);
    if (loopend && loopend > ptr && loopend <= ptr + length)
        le = (unsigned long)(loopend - ptr);

    if (le <= ls) le = length;

    snd.loopBeginBytes = ls;
    snd.loopLengthBytes = le - ls;

    return submit_sound(snd, pitchoffset, vol, left, right, priority, callbackval);
}

int FX_Pan3D(int handle, int angle, int distance)
{
    int vol, left, right;
    pan_from_3d(angle, distance, &vol, &left, &right);
    return FX_SetPan(handle, vol, left, right);
}

int FX_SoundActive(int handle)
{
    std::lock_guard<std::mutex> lock(g_fxMutex);
    cleanup_finished_locked();
    FxVoice* v = find_voice_locked(handle);
    return v ? 1 : 0;
}

int FX_SoundsPlaying(void)
{
    std::lock_guard<std::mutex> lock(g_fxMutex);
    cleanup_finished_locked();
    return (int)g_voices.size();
}

int FX_StopSound(int handle)
{
    std::lock_guard<std::mutex> lock(g_fxMutex);

    for (size_t i = 0; i < g_voices.size(); ++i)
    {
        if (g_voices[i]->handle == handle)
        {
            destroy_voice(g_voices[i], 1);
            g_voices.erase(g_voices.begin() + i);
            return FX_Ok;
        }
    }

    return FX_Error;
}

int FX_StopAllSounds(void)
{
    std::lock_guard<std::mutex> lock(g_fxMutex);

    for (size_t i = 0; i < g_voices.size(); ++i)
        destroy_voice(g_voices[i], 1);

    g_voices.clear();
    return FX_Ok;
}

int FX_StartDemandFeedPlayback(void (*function)(char** ptr, unsigned long* length), int rate, int pitchoffset, int vol, int left, int right, int priority, unsigned long callbackval)
{
    if (!function) return FX_Error;

    char* ptr = NULL;
    unsigned long length = 0;
    function(&ptr, &length);

    if (!ptr || length == 0)
        return FX_Error;

    return FX_PlayRaw(ptr, length, rate, pitchoffset, vol, left, right, priority, callbackval);
}

int FX_StartRecording(int MixRate, void (*function)(char* ptr, int length))
{
    (void)MixRate;
    (void)function;
    return FX_Warning;
}

void FX_StopRecord(void)
{
}

// -----------------------------------------------------------------------------
// MUSIC API - WinMM midiOut sequencer, no MCI
// -----------------------------------------------------------------------------

struct MidiEvent
{
    DWORD tick;
    DWORD msg;
    DWORD tempo;
    int isTempo;

    MidiEvent()
    {
        tick = 0;
        msg = 0;
        tempo = 0;
        isTempo = 0;
    }
};

static HMIDIOUT g_midiOut = NULL;
static HANDLE g_musicThread = NULL;
static volatile LONG g_musicThreadQuit = 0;
static volatile LONG g_musicThreadRunning = 0;
static volatile LONG g_musicPausedFlag = 0;
static volatile LONG g_musicPlayingFlag = 0;

static std::vector<MidiEvent> g_midiEvents;
static CRITICAL_SECTION g_musicCS;
static int g_musicCSInit = 0;

static WORD g_midiDivision = 0;
static DWORD g_musicPositionTicks = 0;
static DWORD g_musicPositionMs = 0;

static uint16_t midi_read_u16be(const unsigned char* p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

static uint32_t midi_read_u32be(const unsigned char* p)
{
    return ((uint32_t)p[0] << 24) |
        ((uint32_t)p[1] << 16) |
        ((uint32_t)p[2] << 8) |
        ((uint32_t)p[3]);
}

static void MUSIC_InitLock(void)
{
    if (!g_musicCSInit)
    {
        InitializeCriticalSection(&g_musicCS);
        g_musicCSInit = 1;
    }
}

static int MUSIC_ReadVarLen(const unsigned char*& p, const unsigned char* end, DWORD* out)
{
    DWORD value = 0;

    for (int i = 0; i < 4; i++)
    {
        if (p >= end)
            return 0;

        unsigned char c = *p++;
        value = (value << 7) | (c & 0x7f);

        if ((c & 0x80) == 0)
        {
            *out = value;
            return 1;
        }
    }

    return 0;
}

static int MUSIC_MidiEventDataLen(unsigned char status)
{
    switch (status & 0xf0)
    {
    case 0x80:
    case 0x90:
    case 0xa0:
    case 0xb0:
    case 0xe0:
        return 2;

    case 0xc0:
    case 0xd0:
        return 1;

    default:
        return 0;
    }
}

static void MUSIC_SendShort(DWORD msg)
{
    if (g_midiOut)
        midiOutShortMsg(g_midiOut, msg);
}

static void MUSIC_AllNotesOff(void)
{
    if (!g_midiOut)
        return;

    for (int ch = 0; ch < 16; ch++)
    {
        MUSIC_SendShort((DWORD)(0xb0 | ch) | ((DWORD)123 << 8)); // All notes off
        MUSIC_SendShort((DWORD)(0xb0 | ch) | ((DWORD)120 << 8)); // All sound off
        MUSIC_SendShort((DWORD)(0xb0 | ch) | ((DWORD)64 << 8));  // Sustain pedal off
    }

    midiOutReset(g_midiOut);
}

static void MUSIC_ApplyVolume(void)
{
    int vol7 = (g_musicVolume * 127) / 255;
    vol7 = clampi(vol7, 0, 127);

    if (g_midiOut)
    {
        DWORD winVol = (DWORD)((g_musicVolume * 0xffff) / 255);
        midiOutSetVolume((HMIDIOUT)g_midiOut, winVol | (winVol << 16));

        for (int ch = 0; ch < 16; ch++)
        {
            DWORD msg = (DWORD)(0xb0 | ch) | ((DWORD)7 << 8) | ((DWORD)vol7 << 16);
            MUSIC_SendShort(msg);
        }
    }
}

static int MUSIC_ParseTrack(const unsigned char* track, DWORD trackLen, std::vector<MidiEvent>& events)
{
    const unsigned char* p = track;
    const unsigned char* end = track + trackLen;

    DWORD tick = 0;
    unsigned char runningStatus = 0;

    __try
    {
        while (p < end)
        {
            DWORD delta = 0;
            if (!MUSIC_ReadVarLen(p, end, &delta))
                return 0;

            tick += delta;

            if (p >= end)
                return 0;

            unsigned char status = *p;

            if (status & 0x80)
            {
                p++;
                if (status < 0xf0)
                    runningStatus = status;
            }
            else
            {
                if (!runningStatus)
                    return 0;

                status = runningStatus;
            }

            if (status == 0xff)
            {
                if (p >= end)
                    return 0;

                unsigned char metaType = *p++;

                DWORD metaLen = 0;
                if (!MUSIC_ReadVarLen(p, end, &metaLen))
                    return 0;

                if (p + metaLen > end)
                    return 0;

                if (metaType == 0x2f)
                {
                    p += metaLen;
                    break;
                }

                if (metaType == 0x51 && metaLen == 3)
                {
                    MidiEvent ev;
                    ev.tick = tick;
                    ev.isTempo = 1;
                    ev.tempo = ((DWORD)p[0] << 16) | ((DWORD)p[1] << 8) | (DWORD)p[2];
                    events.push_back(ev);
                }

                p += metaLen;
            }
            else if (status == 0xf0 || status == 0xf7)
            {
                DWORD sysexLen = 0;
                if (!MUSIC_ReadVarLen(p, end, &sysexLen))
                    return 0;

                if (p + sysexLen > end)
                    return 0;

                p += sysexLen;
            }
            else
            {
                int dataLen = MUSIC_MidiEventDataLen(status);
                if (dataLen <= 0)
                    return 0;

                if (p + dataLen > end)
                    return 0;

                unsigned char d1 = p[0];
                unsigned char d2 = 0;

                if (dataLen == 2)
                    d2 = p[1];

                p += dataLen;

                MidiEvent ev;
                ev.tick = tick;
                ev.isTempo = 0;
                ev.tempo = 0;
                ev.msg = (DWORD)status | ((DWORD)d1 << 8) | ((DWORD)d2 << 16);
                events.push_back(ev);
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }

    return 1;
}

static int MUSIC_ParseMidi(unsigned char* song, std::vector<MidiEvent>& events, WORD* outDivision)
{
    if (!song || !outDivision)
        return 0;

    events.clear();
    *outDivision = 0;

    {
        if (memcmp(song, "MThd", 4) != 0)
            return 0;

        DWORD headerLen = midi_read_u32be(song + 4);

        if (headerLen < 6 || headerLen > 1024)
            return 0;

        WORD format = midi_read_u16be(song + 8);
        WORD numTracks = midi_read_u16be(song + 10);
        WORD division = midi_read_u16be(song + 12);

        if (numTracks == 0 || numTracks > 256)
            return 0;

        // This simple sequencer supports normal PPQ MIDI timing.
        // SMPTE timing has the high bit set and is rare for Duke-style MIDI.
        if (division & 0x8000)
            return 0;

        if (division == 0)
            return 0;

        if (format > 1)
            return 0;

        const unsigned char* p = song + 8 + headerLen;
        const DWORD MAX_MIDI_SIZE = 16 * 1024 * 1024;
        DWORD walked = 8 + headerLen;

        for (WORD i = 0; i < numTracks; i++)
        {
            if (walked > MAX_MIDI_SIZE)
                return 0;

            if (memcmp(p, "MTrk", 4) != 0)
                return 0;

            DWORD trackLen = midi_read_u32be(p + 4);

            if (trackLen > MAX_MIDI_SIZE)
                return 0;

            p += 8;
            walked += 8;

            if (walked + trackLen > MAX_MIDI_SIZE)
                return 0;

            if (!MUSIC_ParseTrack(p, trackLen, events))
                return 0;

            p += trackLen;
            walked += trackLen;
        }

        std::stable_sort(events.begin(), events.end(),
            [](const MidiEvent& a, const MidiEvent& b)
            {
                if (a.tick != b.tick)
                    return a.tick < b.tick;

                return a.isTempo > b.isTempo;
            });

        *outDivision = division;
        return !events.empty();
    }

}

static int MUSIC_WaitMs(double ms)
{
    DWORD last = timeGetTime();
    double elapsed = 0.0;

    while (elapsed < ms)
    {
        if (InterlockedCompareExchange(&g_musicThreadQuit, 0, 0))
            return 0;

        if (InterlockedCompareExchange(&g_musicPausedFlag, 0, 0))
        {
            Sleep(1);
            last = timeGetTime();
            continue;
        }

        Sleep(1);

        DWORD now = timeGetTime();
        elapsed += (double)(now - last);
        last = now;
    }

    return 1;
}

static DWORD WINAPI MUSIC_ThreadProc(LPVOID)
{
    InterlockedExchange(&g_musicThreadRunning, 1);
    InterlockedExchange(&g_musicPlayingFlag, 1);

    timeBeginPeriod(1);

    for (;;)
    {
        MUSIC_AllNotesOff();
        MUSIC_ApplyVolume();

        DWORD lastTick = 0;
        DWORD tempo = 500000; // default MIDI tempo: 120 BPM
        g_musicPositionTicks = 0;
        g_musicPositionMs = 0;

        size_t count = 0;

        EnterCriticalSection(&g_musicCS);
        count = g_midiEvents.size();
        LeaveCriticalSection(&g_musicCS);

        for (size_t i = 0; i < count; i++)
        {
            if (InterlockedCompareExchange(&g_musicThreadQuit, 0, 0))
                break;

            MidiEvent ev;

            EnterCriticalSection(&g_musicCS);
            if (i >= g_midiEvents.size())
            {
                LeaveCriticalSection(&g_musicCS);
                break;
            }
            ev = g_midiEvents[i];
            LeaveCriticalSection(&g_musicCS);

            DWORD deltaTicks = ev.tick - lastTick;
            lastTick = ev.tick;

            if (deltaTicks > 0)
            {
                double deltaMs = ((double)deltaTicks * (double)tempo) / ((double)g_midiDivision * 1000.0);
                if (!MUSIC_WaitMs(deltaMs))
                    break;

                g_musicPositionTicks = ev.tick;
                g_musicPositionMs += (DWORD)deltaMs;
            }

            if (InterlockedCompareExchange(&g_musicThreadQuit, 0, 0))
                break;

            if (ev.isTempo)
            {
                if (ev.tempo > 0)
                    tempo = ev.tempo;
            }
            else
            {
                MUSIC_SendShort(ev.msg);
            }
        }

        MUSIC_AllNotesOff();

        if (InterlockedCompareExchange(&g_musicThreadQuit, 0, 0))
            break;

        if (!g_musicLoop)
            break;
    }

    MUSIC_AllNotesOff();

    timeEndPeriod(1);

    InterlockedExchange(&g_musicPlayingFlag, 0);
    InterlockedExchange(&g_musicThreadRunning, 0);

    g_musicPlaying = 0;
    g_musicPaused = 0;

    return 0;
}

static void MUSIC_StopThread(void)
{
    HANDLE h = g_musicThread;

    if (h)
    {
        InterlockedExchange(&g_musicThreadQuit, 1);
        WaitForSingleObject(h, INFINITE);
        CloseHandle(h);
        g_musicThread = NULL;
    }

    InterlockedExchange(&g_musicThreadQuit, 0);
    InterlockedExchange(&g_musicThreadRunning, 0);
    InterlockedExchange(&g_musicPlayingFlag, 0);
}

static void MUSIC_CloseMidi(void)
{
    MUSIC_StopThread();

    if (g_midiOut)
    {
        MUSIC_AllNotesOff();
        midiOutClose(g_midiOut);
        g_midiOut = NULL;
    }

    g_musicPlaying = 0;
    g_musicPaused = 0;
    InterlockedExchange(&g_musicPausedFlag, 0);
    InterlockedExchange(&g_musicPlayingFlag, 0);
}

char* MUSIC_ErrorString(int ErrorNumber)
{
    switch (ErrorNumber)
    {
    case MUSIC_Warning: return (char*)"MUSIC warning";
    case MUSIC_Error: return (char*)"MUSIC error";
    case MUSIC_Ok: return (char*)"MUSIC ok";
    case MUSIC_ASSVersion: return (char*)"MUSIC ASS version mismatch";
    case MUSIC_SoundCardError: return (char*)"MUSIC sound card error";
    case MUSIC_MPU401Error: return (char*)"MUSIC MPU401 error";
    case MUSIC_InvalidCard: return (char*)"MUSIC invalid card";
    case MUSIC_MidiError: return (char*)"MUSIC midi error";
    case MUSIC_TaskManError: return (char*)"MUSIC task manager error";
    case MUSIC_FMNotDetected: return (char*)"MUSIC FM not detected";
    case MUSIC_DPMI_Error: return (char*)"MUSIC DPMI error";
    default: return (char*)"MUSIC unknown error";
    }
}

int MUSIC_Init(int SoundCard, int Address)
{
    (void)SoundCard;
    (void)Address;

    MUSIC_InitLock();

    MUSIC_CloseMidi();

    MMRESULT mm = midiOutOpen(&g_midiOut, MIDI_MAPPER, 0, 0, CALLBACK_NULL);
    if (mm != MMSYSERR_NOERROR)
    {
        g_midiOut = NULL;
        MUSIC_ErrorCode = MUSIC_MPU401Error;
        return MUSIC_Error;
    }

    MUSIC_ApplyVolume();

    MUSIC_ErrorCode = MUSIC_Ok;
    return MUSIC_Ok;
}

int MUSIC_Shutdown(void)
{
    MUSIC_CloseMidi();

    EnterCriticalSection(&g_musicCS);
    g_midiEvents.clear();
    LeaveCriticalSection(&g_musicCS);

    MUSIC_ErrorCode = MUSIC_Ok;
    return MUSIC_Ok;
}

void MUSIC_SetMaxFMMidiChannel(int channel)
{
    (void)channel;
}

void MUSIC_SetVolume(int volume)
{
    g_musicVolume = clampi(volume, 0, 255);
    MUSIC_ApplyVolume();
}

void MUSIC_SetMidiChannelVolume(int channel, int volume)
{
    if (!g_midiOut)
        return;

    channel = clampi(channel, 0, 15);
    volume = clampi(volume, 0, 127);

    DWORD msg = (DWORD)(0xb0 | channel) | ((DWORD)7 << 8) | ((DWORD)volume << 16);
    MUSIC_SendShort(msg);
}

void MUSIC_ResetMidiChannelVolumes(void)
{
    MUSIC_ApplyVolume();
}

int MUSIC_GetVolume(void)
{
    return g_musicVolume;
}

void MUSIC_SetLoopFlag(int loopflag)
{
    g_musicLoop = loopflag ? 1 : 0;
}

int MUSIC_SongPlaying(void)
{
    return InterlockedCompareExchange(&g_musicPlayingFlag, 0, 0) ? 1 : 0;
}

void MUSIC_Continue(void)
{
    g_musicPaused = 0;
    InterlockedExchange(&g_musicPausedFlag, 0);
}

void MUSIC_Pause(void)
{
    g_musicPaused = 1;
    InterlockedExchange(&g_musicPausedFlag, 1);
    MUSIC_AllNotesOff();
}

int MUSIC_StopSong(void)
{
    MUSIC_StopThread();
    MUSIC_AllNotesOff();

    g_musicPlaying = 0;
    g_musicPaused = 0;
    InterlockedExchange(&g_musicPausedFlag, 0);
    InterlockedExchange(&g_musicPlayingFlag, 0);

    return MUSIC_Ok;
}

int MUSIC_PlaySong(unsigned char* song, int loopflag)
{
    if (!song)
    {
        MUSIC_ErrorCode = MUSIC_MidiError;
        return MUSIC_Error;
    }

    MUSIC_InitLock();

    if (!g_midiOut)
    {
        MMRESULT mm = midiOutOpen(&g_midiOut, MIDI_MAPPER, 0, 0, CALLBACK_NULL);
        if (mm != MMSYSERR_NOERROR)
        {
            g_midiOut = NULL;
            MUSIC_ErrorCode = MUSIC_MPU401Error;
            return MUSIC_Error;
        }
    }

    MUSIC_StopThread();
    MUSIC_AllNotesOff();

    std::vector<MidiEvent> parsedEvents;
    WORD division = 0;

    if (!MUSIC_ParseMidi(song, parsedEvents, &division))
    {
        MUSIC_ErrorCode = MUSIC_MidiError;
        return MUSIC_Error;
    }

    EnterCriticalSection(&g_musicCS);
    g_midiEvents.swap(parsedEvents);
    g_midiDivision = division;
    LeaveCriticalSection(&g_musicCS);

    g_musicLoop = loopflag ? 1 : 0;
    g_musicPaused = 0;
    g_musicPlaying = 1;
    g_musicPositionTicks = 0;
    g_musicPositionMs = 0;

    InterlockedExchange(&g_musicThreadQuit, 0);
    InterlockedExchange(&g_musicPausedFlag, 0);
    InterlockedExchange(&g_musicPlayingFlag, 1);

    MUSIC_ApplyVolume();

    g_musicThread = CreateThread(NULL, 0, MUSIC_ThreadProc, NULL, 0, NULL);
    if (!g_musicThread)
    {
        g_musicPlaying = 0;
        InterlockedExchange(&g_musicPlayingFlag, 0);
        MUSIC_ErrorCode = MUSIC_TaskManError;
        return MUSIC_Error;
    }

    MUSIC_ErrorCode = MUSIC_Ok;
    return MUSIC_Ok;
}

void MUSIC_SetContext(int context)
{
    g_musicContext = context;
}

int MUSIC_GetContext(void)
{
    return g_musicContext;
}

void MUSIC_SetSongTick(unsigned long PositionInTicks)
{
    // Simple seek: restart and fast-forwarding would require storing the original song pointer.
    // Kept as a no-op for Duke3D compatibility.
    (void)PositionInTicks;
}

void MUSIC_SetSongTime(unsigned long milliseconds)
{
    // Simple seek: restart and fast-forwarding would require storing the original song pointer.
    // Kept as a no-op for Duke3D compatibility.
    (void)milliseconds;
}

void MUSIC_SetSongPosition(int measure, int beat, int tick)
{
    (void)measure;
    (void)beat;
    (void)tick;
}

void MUSIC_GetSongPosition(songposition* pos)
{
    if (!pos) return;

    memset(pos, 0, sizeof(*pos));

    pos->tickposition = g_musicPositionTicks;
    pos->milliseconds = g_musicPositionMs;
    pos->measure = 0;
    pos->beat = 0;
    pos->tick = 0;
}

void MUSIC_GetSongLength(songposition* pos)
{
    if (!pos) return;

    memset(pos, 0, sizeof(*pos));

    DWORD totalTicks = 0;

    EnterCriticalSection(&g_musicCS);
    if (!g_midiEvents.empty())
        totalTicks = g_midiEvents.back().tick;
    LeaveCriticalSection(&g_musicCS);

    pos->tickposition = totalTicks;

    if (g_midiDivision)
    {
        // Approximate using default tempo. Exact length would need replaying tempo map.
        pos->milliseconds = (unsigned long)(((double)totalTicks * 500000.0) / ((double)g_midiDivision * 1000.0));
    }
}

int MUSIC_FadeVolume(int tovolume, int milliseconds)
{
    (void)milliseconds;
    MUSIC_SetVolume(tovolume);
    return MUSIC_Ok;
}

int MUSIC_FadeActive(void)
{
    return 0;
}

void MUSIC_StopFade(void)
{
}

void MUSIC_RerouteMidiChannel(int channel, int (*function)(int event, int c1, int c2))
{
    (void)channel;
    (void)function;
}

void MUSIC_RegisterTimbreBank(unsigned char* timbres)
{
    (void)timbres;
}
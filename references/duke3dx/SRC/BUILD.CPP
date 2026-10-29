// Build.cpp
//

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <chrono>
#include <deque>
#include <limits>
#include <memory>
#include <string>
#include <vector>
#if !defined(_WIN32)
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#if !defined(_WIN32)
#define _stricmp strcasecmp
#endif

template <class A, class B>
static inline auto min(A a, B b) -> decltype((a < b) ? a : b) { return (a < b) ? a : b; }
template <class A, class B>
static inline auto max(A a, B b) -> decltype((a > b) ? a : b) { return (a > b) ? a : b; }

#ifndef MAXXDIM
#define MAXXDIM 2048
#endif
#ifndef MAXYDIM
#define MAXYDIM 1536
#endif
#ifndef MAXSECTORS
#define MAXSECTORS 1024
#endif
#ifndef MAXWALLS
#define MAXWALLS 8192
#endif
#ifndef MAXSPRITES
#define MAXSPRITES 4096
#endif
#ifndef MAXSTATUS
#define MAXSTATUS 1024
#endif
#ifndef MAXTILES
#define MAXTILES 4096
#endif
#ifndef MAXPALOOKUPS
#define MAXPALOOKUPS 256
#endif
#ifndef MAXSPRITESONSCREEN
#define MAXSPRITESONSCREEN 4096
#endif
#ifndef MAXPSKYTILES
#define MAXPSKYTILES 256
#endif

#ifndef MAXVOXELS
#define MAXVOXELS 512
#endif
#ifndef MAXVOXMIPS
#define MAXVOXMIPS 5
#endif

#define MAXCLIPNUM 512
#define MAXPERMS 512
#define MAXWALLSB 2048
#define MAXCLIPDIST 1024

static_assert(sizeof(long) == 4, "This classic BUILD drop-in assumes 32-bit long.");
#if INTPTR_MAX > INT32_MAX
#pragma message("Warning: classic BUILD drop-in mode is intended for Win32/x86. x64 requires changing old external waloff/frameplace pointer ABI.")
#endif


#include "BUILD.H"

static_assert(sizeof(sectortype) == 40, "sectortype must match BUILD v7 map layout");
static_assert(sizeof(walltype) == 32, "walltype must match BUILD v7 map layout");
static_assert(sizeof(spritetype) == 44, "spritetype must match BUILD v7 map layout");


#if INTPTR_MAX > INT32_MAX
#define BUILDGL_PTR_TO_LONG(p) static_cast<long>(reinterpret_cast<std::intptr_t>(p) & 0xffffffff)
#else
#define BUILDGL_PTR_TO_LONG(p) reinterpret_cast<long>(p)
#endif

// -----------------------------------------------------------------------------
// Public BUILD globals.
// -----------------------------------------------------------------------------
// These intentionally use normal C++ linkage to match the classic Duke/BUILD
// Visual Studio projects, whose .CPP files include BUILD.H without extern "C".
short numsectors = 0, numwalls = 0;
long numtiles = MAXTILES;
sectortype sector[MAXSECTORS] = {};
walltype wall[MAXWALLS] = {};
spritetype sprite[MAXSPRITES] = {};
spritetype tsprite[MAXSPRITESONSCREEN] = {};

short headspritesect[MAXSECTORS + 1] = {};
short prevspritesect[MAXSPRITES] = {};
short nextspritesect[MAXSPRITES] = {};
short headspritestat[MAXSTATUS + 1] = {};
short prevspritestat[MAXSPRITES] = {};
short nextspritestat[MAXSPRITES] = {};

long xdim = 640, ydim = 480, bytesperline = 640, imageSize = 640 * 480;
long windowx1 = 0, windowy1 = 0, windowx2 = 639, windowy2 = 479;
short startumost[MAXXDIM] = {}, startdmost[MAXXDIM] = {};
long ylookup[MAXYDIM + 1] = {};
long frameplace = 0, pageoffset = 0, qsetmode = 0;
char vidoption = 1;
long numpages = 1, maxpages = 1;
long activepage = 0, visualpage = 0, numframes = 0;
volatile long totalclock = 0;
long randomseed = 17;
long beforedrawrooms = 1, totalarea = 0, transarea = 0;
char automapping = 0, showinvisibility = 0;
long visibility = 512, parallaxvisibility = 512;
char parallaxtype = 2;
long parallaxyoffs = 0, parallaxyscale = 65536;
short pskyoff[MAXPSKYTILES] = {};
short pskybits = 0;

short tilesizx[MAXTILES] = {}, tilesizy[MAXTILES] = {};
long picanm[MAXTILES] = {};
long waloff[MAXTILES] = {};
char walock[MAXTILES] = {};
char gotpic[(MAXTILES + 7) >> 3] = {};
char picsiz[MAXTILES] = {};
char tilefilenum[MAXTILES] = {};
long tilefileoffs[MAXTILES] = {};
long artversion = 1, mapversion = 7, artsize = 0, cachesize = 0;

unsigned char palette[768] = {};
static unsigned char basepalette[768] = {};
static bool basepalettevalid = false;
char* palookup[MAXPALOOKUPS] = {};
short numpalookups = 32;
char* transluc = nullptr;
char paletteloaded = 0;
unsigned char britable[16][64] = {};
char textfont[1024] = {};
char smalltextfont[1024] = {};

short sintable[2048] = {};
short radarang[1280 * 2] = { };
char show2dsector[(MAXSECTORS + 7) >> 3] = {};
char show2dwall[(MAXWALLS + 7) >> 3] = {};
char show2dsprite[(MAXSPRITES + 7) >> 3] = {};
char gotsector[(MAXSECTORS + 7) >> 3] = {};
long spritesortcnt = 0;

short searchit = 0;
long searchx = -1, searchy = 0;
short searchsector = -1, searchwall = -1, searchstat = -1;

long globalposx = 0, globalposy = 0, globalposz = 0, globalhoriz = 100;
short globalang = 0, globalcursectnum = 0;
long cosglobalang = 0, singlobalang = 0;
long cosviewingrangeglobalang = 0, sinviewingrangeglobalang = 0;
long xyaspect = 65536, yxaspect = 65536, viewingrange = 65536, viewingrangerecip = 65536;
long xdimen = 640, ydimen = 480, halfxdimen = 320, xdimenrecip = 0, xdimenscale = 65536, xdimscale = 65536;
long globalpicnum = 0, globalpal = 0, globalshade = 0, globalorientation = 0;
long globalxpanning = 0, globalypanning = 0, globalyscale = 0, globalzd = 0, globalshiftval = 0;
long globvis = 0, globalvisibility = 512, globalhisibility = 512, globalpisibility = 512, globalcisibility = 512;
long wx1 = 0, wy1 = 0, wx2 = 0, wy2 = 0, viewoffset = 0;

long stereowidth = 23040, stereopixelwidth = 28, ostereopixelwidth = -1;
volatile long stereomode = 0, whiteband = 0, blackband = 0;
volatile char oa1 = 0, o3c2 = 0, ortca = 0, ortcb = 0, overtbits = 0, laststereoint = 0;
long voxoff[MAXVOXELS][MAXVOXMIPS] = {};
long voxlock[MAXVOXELS][MAXVOXMIPS] = {};

// Extra Duke3D/game-side globals that older modules often expect from the
// engine/platform layer.
char numenvsnds = 0;
char inputloc = 0;
char vgacompatible = 1;
char cachedebug = 0;


// Forward declarations for original engine entry points used before their bodies.
extern "C" {
    void loadtile(short tilenume);
    int animateoffs(short tilenum, short fakevar);
    void initspritelists();
    short insertsprite(short sectnum, short statnum);
    short insertspritesect(short sectnum);
    short insertspritestat(short statnum);
    int deletesprite(short spritenum);
    int deletespritesect(short deleteme);
    int deletespritestat(short deleteme);
    void updatesector(long x, long y, short* sectnum);
    int inside(long x, long y, short sectnum);
    void setview(long x1, long y1, long x2, long y2);
    void setaspect(long daxrange, long daaspect);
    void scansector(short sectnum);
    void drawalls(long bunch);
    void drawmasks();
    void nextpage();
    long getceilzofslope(short sectnum, long dax, long day);
    long getflorzofslope(short sectnum, long dax, long day);
    void getzsofslope(short sectnum, long dax, long day, long* ceilz, long* florz);
    int getangle(long xvect, long yvect);
    void dorotatesprite(long sx, long sy, long z, short a, short picnum, signed char dashade, char dapalnum, char dastat, long cx1, long cy1, long cx2, long cy2);
    int lintersect(long x1, long y1, long z1, long x2, long y2, long z2, long x3, long y3, long x4, long y4, long* intx, long* inty, long* intz);
    int rintersect(long x1, long y1, long z1, long vx, long vy, long vz, long x3, long y3, long x4, long y4, long* intx, long* inty, long* intz);
    int clipinsidebox(long x, long y, short wallnum, long walldist);
    void agecache();
}
void faketimerhandler();

// Internal renderer globals retained from the original names where useful.
static short umost[MAXXDIM] = {}, dmost[MAXXDIM] = {};
static short uplc[MAXXDIM] = {}, dplc[MAXXDIM] = {};
static short uwall[MAXXDIM] = {}, dwall[MAXXDIM] = {};
static long swall[MAXXDIM] = {}, lwall[MAXXDIM + 4] = {};
static short maskwall[MAXWALLSB] = {};
static long maskwallcnt = 0;
static spritetype* tspriteptr[MAXSPRITESONSCREEN] = {};
static long spritesx[MAXSPRITESONSCREEN] = {}, spritesy[MAXSPRITESONSCREEN + 1] = {}, spritesz[MAXSPRITESONSCREEN] = {};
static short sectorborder[256] = {};
static short sectorbordercnt = 0;
static linetype clipit[MAXCLIPNUM] = {};
static short clipobjectval[MAXCLIPNUM] = {};
static short clipsectorlist[MAXCLIPNUM] = {};
static short clipnum = 0, clipsectnum = 0;
static short hitwalls[4] = {};
long clipmoveboxtracenum = 3;
static long hitscangoalx = (1 << 29) - 1, hitscangoaly = (1 << 29) - 1;
static long curbrightness = 0;
static long lastageclock = 0;
static long capturecount = 0;
static bool g_mouseInit = false;
static POINT g_lastMousePoint = { 0, 0 };
static char artfilename[260] = "tiles000.art";
static long numtilefiles = 0;
static short artfil = -1;
static long inpreparemirror = 0, mirrorsx1 = 0, mirrorsx2 = 0, mirrorsy1 = 0, mirrorsy2 = 0;
static long pointhighlight = -1, linehighlight = -1, highlightcnt = 0, editstatus = 0;
static long permanentupdate = 0, buffermode = 0, origbuffermode = 0, validmodecnt = 0;

// -----------------------------------------------------------------------------
// Fixed-point and utility replacements for old pragmas/Watcom asm.
// -----------------------------------------------------------------------------
static inline long klabs(long v) { return v < 0 ? -v : v; }
static inline long ksgn(long v) { return (v > 0) - (v < 0); }

static inline long scale(long a, long b, long c)
{
    if (c == 0) return 0;
    return static_cast<long>((static_cast<std::int64_t>(a) * static_cast<std::int64_t>(b)) / c);
}
static inline long mulscale(long a, long b, long shift)
{
    return static_cast<long>((static_cast<std::int64_t>(a) * static_cast<std::int64_t>(b)) >> shift);
}
static inline long divscale(long a, long b, long shift)
{
    if (b == 0) return 0;
    return static_cast<long>((static_cast<std::int64_t>(a) << shift) / b);
}
#define DEF_MULSCALE(N) static inline long mulscale##N(long a, long b){ return static_cast<long>((static_cast<std::int64_t>(a)*static_cast<std::int64_t>(b)) >> (N)); }
#define DEF_DMULSCALE(N) static inline long dmulscale##N(long a,long b,long c,long d){ return static_cast<long>(((static_cast<std::int64_t>(a)*b)+(static_cast<std::int64_t>(c)*d)) >> (N)); }
#define DEF_DIVSCALE(N) static inline long divscale##N(long a,long b){ if(!b) return 0; return static_cast<long>((static_cast<std::int64_t>(a) << (N)) / b); }
DEF_MULSCALE(1) DEF_MULSCALE(2) DEF_MULSCALE(3) DEF_MULSCALE(4) DEF_MULSCALE(5) DEF_MULSCALE(6) DEF_MULSCALE(7) DEF_MULSCALE(8)
DEF_MULSCALE(9) DEF_MULSCALE(10) DEF_MULSCALE(11) DEF_MULSCALE(12) DEF_MULSCALE(13) DEF_MULSCALE(14) DEF_MULSCALE(15) DEF_MULSCALE(16)
DEF_MULSCALE(17) DEF_MULSCALE(18) DEF_MULSCALE(19) DEF_MULSCALE(20) DEF_MULSCALE(21) DEF_MULSCALE(22) DEF_MULSCALE(23) DEF_MULSCALE(24)
DEF_MULSCALE(25) DEF_MULSCALE(26) DEF_MULSCALE(27) DEF_MULSCALE(28) DEF_MULSCALE(29) DEF_MULSCALE(30) DEF_MULSCALE(31) DEF_MULSCALE(32)
DEF_DMULSCALE(2) DEF_DMULSCALE(3) DEF_DMULSCALE(4) DEF_DMULSCALE(5) DEF_DMULSCALE(6) DEF_DMULSCALE(8) DEF_DMULSCALE(9) DEF_DMULSCALE(10)
DEF_DMULSCALE(12) DEF_DMULSCALE(14) DEF_DMULSCALE(15) DEF_DMULSCALE(16) DEF_DMULSCALE(17) DEF_DMULSCALE(24) DEF_DMULSCALE(25) DEF_DMULSCALE(28) DEF_DMULSCALE(32)
DEF_DIVSCALE(12) DEF_DIVSCALE(14) DEF_DIVSCALE(15) DEF_DIVSCALE(16) DEF_DIVSCALE(18) DEF_DIVSCALE(19) DEF_DIVSCALE(20) DEF_DIVSCALE(24) DEF_DIVSCALE(28) DEF_DIVSCALE(30) DEF_DIVSCALE(32)
#undef DEF_MULSCALE
#undef DEF_DMULSCALE
#undef DEF_DIVSCALE

char pow2char[8] = { 1,2,4,8,16,32,64,128 };
long pow2long[32] =
{
    1L,2L,4L,8L,16L,32L,64L,128L,256L,512L,1024L,2048L,4096L,8192L,16384L,32768L,
    65536L,131072L,262144L,524288L,1048576L,2097152L,4194304L,8388608L,
    16777216L,33554432L,67108864L,134217728L,268435456L,536870912L,1073741824L,2147483647L
};

static void buildgl_clearbufbyte(void* dst, long count, long value)
{
    if (!dst || count <= 0) return;
    std::memset(dst, static_cast<unsigned char>(value & 255), static_cast<size_t>(count));
}
static void buildgl_clearbuf(void* dst, long count, long value)
{
    if (!dst || count <= 0) return;
    auto* p = static_cast<long*>(dst);
    for (long i = 0; i < count; ++i) p[i] = value;
}
static void buildgl_copybufbyte(const void* src, void* dst, long count)
{
    if (!src || !dst || count <= 0) return;
    std::memcpy(dst, src, static_cast<size_t>(count));
}
static void buildgl_copybuf(const void* src, void* dst, long count)
{
    if (!src || !dst || count <= 0) return;
    std::memcpy(dst, src, static_cast<size_t>(count) * sizeof(long));
}

extern "C" void clearbufbyte(long dst, long count, long value)
{
    buildgl_clearbufbyte(reinterpret_cast<void*>(static_cast<std::intptr_t>(dst)), count, value);
}
extern "C" void clearbuf(long dst, long count, long value)
{
    buildgl_clearbuf(reinterpret_cast<void*>(static_cast<std::intptr_t>(dst)), count, value);
}
extern "C" void copybufbyte(long src, long dst, long count)
{
    buildgl_copybufbyte(reinterpret_cast<const void*>(static_cast<std::intptr_t>(src)),
        reinterpret_cast<void*>(static_cast<std::intptr_t>(dst)), count);
}
extern "C" void copybuf(long src, long dst, long count)
{
    buildgl_copybuf(reinterpret_cast<const void*>(static_cast<std::intptr_t>(src)),
        reinterpret_cast<void*>(static_cast<std::intptr_t>(dst)), count);
}
static void swaplong(long* a, long* b) { long t = *a; *a = *b; *b = t; }
static void swapshort(short* a, short* b) { short t = *a; *a = *b; *b = t; }

extern "C" void* kmalloc(size_t size) { return std::malloc(size); }
extern "C" void kfree(void* buffer) { std::free(buffer); }
extern "C" void* kkmalloc(size_t size) { return std::malloc(size); }
extern "C" void kkfree(void* buffer) { std::free(buffer); }

// GRP-aware file table compatible with old kopen/kread style calls.
//
// BUILD GRP archives use this layout:
//   12 bytes: "KenSilverman"
//    4 bytes: little-endian number of files
//   repeated directory entries: 12-byte filename + 4-byte size
//   then each file payload concatenated in directory order.
//
// kopen4load() searches loose files first, then all mounted GRP files from newest
// to oldest. This makes local files override packaged assets while still letting
// palette.dat, tables.dat, ART files, and maps be loaded from a GRP.
namespace buildfile
{
    struct GroupEntry
    {
        std::string name;
        long offset = 0;
        long size = 0;
    };
    struct GroupFile
    {
        FILE* fp = nullptr;
        std::string path;
        std::vector<GroupEntry> entries;
    };
    struct KFile
    {
        FILE* fp = nullptr;
        bool fromGroup = false;
        long base = 0;
        long length = 0;
        long pos = 0;
    };

    static std::vector<GroupFile> g_groups;
    static KFile g_kfiles[256];

    static std::string basenameOnly(const char* filename)
    {
        if (!filename) return {};
        std::string s(filename);
        size_t p = s.find_last_of("/\\");
        if (p != std::string::npos) s = s.substr(p + 1);
        return s;
    }
    static std::string normalizeName(const char* filename)
    {
        std::string s = basenameOnly(filename);
        while (!s.empty() && (s.back() == '\0' || s.back() == ' ')) s.pop_back();
        for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return s;
    }
    static FILE* tryOpenDisk(const char* filename)
    {
        if (!filename || !filename[0]) return nullptr;
        FILE* f = std::fopen(filename, "rb");
        if (f) return f;

        std::string upper(filename);
        for (char& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        f = std::fopen(upper.c_str(), "rb");
        if (f) return f;

        std::string lower(filename);
        for (char& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return std::fopen(lower.c_str(), "rb");
    }


#if defined(_WIN32)
    static int osOpenReadPath(const char* path)
    {
        return _open(path, _O_RDONLY | _O_BINARY);
    }
    static int osOpenWritePath(const char* path)
    {
        return _open(path, _O_CREAT | _O_TRUNC | _O_WRONLY | _O_BINARY, _S_IREAD | _S_IWRITE);
    }
    static int osOpenAppendPath(const char* path)
    {
        return _open(path, _O_CREAT | _O_APPEND | _O_WRONLY | _O_BINARY, _S_IREAD | _S_IWRITE);
    }
    static int osReadHandle(int fd, void* buf, long len)
    {
        return _read(fd, buf, static_cast<unsigned int>(len));
    }
    static int osWriteHandle(int fd, const void* buf, long len)
    {
        return _write(fd, buf, static_cast<unsigned int>(len));
    }
    static long osSeekHandle(int fd, long off, int whence)
    {
        return static_cast<long>(_lseek(fd, off, whence));
    }
    static int osCloseHandle(int fd)
    {
        return _close(fd);
    }
#else
    static int osOpenReadPath(const char* path)
    {
        return open(path, O_RDONLY);
    }
    static int osOpenWritePath(const char* path)
    {
        return open(path, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    }
    static int osOpenAppendPath(const char* path)
    {
        return open(path, O_CREAT | O_APPEND | O_WRONLY, 0644);
    }
    static int osReadHandle(int fd, void* buf, long len)
    {
        return static_cast<int>(read(fd, buf, static_cast<size_t>(len)));
    }
    static int osWriteHandle(int fd, const void* buf, long len)
    {
        return static_cast<int>(write(fd, buf, static_cast<size_t>(len)));
    }
    static long osSeekHandle(int fd, long off, int whence)
    {
        return static_cast<long>(lseek(fd, off, whence));
    }
    static int osCloseHandle(int fd)
    {
        return close(fd);
    }
#endif

    static int tryOpenDiskReadHandle(const char* filename)
    {
        if (!filename || !filename[0]) return -1;

        int fd = osOpenReadPath(filename);
        if (fd >= 0) return fd;

        std::string upper(filename);
        for (char& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        fd = osOpenReadPath(upper.c_str());
        if (fd >= 0) return fd;

        std::string lower(filename);
        for (char& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return osOpenReadPath(lower.c_str());
    }

    static bool groupEntryExists(const char* filename)
    {
        const std::string want = normalizeName(filename);
        if (want.empty()) return false;

        for (auto git = g_groups.rbegin(); git != g_groups.rend(); ++git)
            for (const GroupEntry& e : git->entries)
                if (e.name == want) return true;
        return false;
    }

    static int createTempHandleForGroupEntry(GroupFile& grp, const GroupEntry& entry)
    {
#if defined(_WIN32)
        char tempDir[MAX_PATH + 1] = {};
        char tempPath[MAX_PATH + 1] = {};
        DWORD n = GetTempPathA(MAX_PATH, tempDir);
        if (n == 0 || n > MAX_PATH) std::strcpy(tempDir, ".\\");
        if (!GetTempFileNameA(tempDir, "bgl", 0, tempPath)) return -1;
        int fd = _open(tempPath, _O_CREAT | _O_TRUNC | _O_RDWR | _O_BINARY | _O_TEMPORARY, _S_IREAD | _S_IWRITE);
#else
        char tempPath[] = "/tmp/buildgl_grp_XXXXXX";
        int fd = mkstemp(tempPath);
        if (fd >= 0) unlink(tempPath);
#endif
        if (fd < 0) return -1;

        std::fseek(grp.fp, entry.offset, SEEK_SET);
        long remaining = entry.size;
        unsigned char buffer[16384];
        while (remaining > 0)
        {
            const long want = min<long>(remaining, static_cast<long>(sizeof(buffer)));
            const long got = static_cast<long>(std::fread(buffer, 1, static_cast<size_t>(want), grp.fp));
            if (got <= 0)
            {
                osCloseHandle(fd);
                return -1;
            }
            long written = 0;
            while (written < got)
            {
                int did = osWriteHandle(fd, buffer + written, got - written);
                if (did <= 0)
                {
                    osCloseHandle(fd);
                    return -1;
                }
                written += did;
            }
            remaining -= got;
        }

        osSeekHandle(fd, 0, SEEK_SET);
        return fd;
    }

    static int openGroupAsDiskCompatibleHandle(const char* filename)
    {
        const std::string want = normalizeName(filename);
        if (want.empty()) return -1;

        for (auto git = g_groups.rbegin(); git != g_groups.rend(); ++git)
        {
            GroupFile& grp = *git;
            for (const GroupEntry& e : grp.entries)
                if (e.name == want)
                    return createTempHandleForGroupEntry(grp, e);
        }
        return -1;
    }
    static short allocHandle()
    {
        for (int i = 1; i < 256; ++i)
            if (!g_kfiles[i].fp) return static_cast<short>(i);
        return -1;
    }
    static short openDiskHandle(const char* filename)
    {
        FILE* f = tryOpenDisk(filename);
        if (!f) return -1;
        short h = allocHandle();
        if (h < 0) { std::fclose(f); return -1; }
        std::fseek(f, 0, SEEK_END);
        long len = std::ftell(f);
        std::fseek(f, 0, SEEK_SET);
        g_kfiles[h] = KFile{};
        g_kfiles[h].fp = f;
        g_kfiles[h].length = len;
        return h;
    }
    static short openGroupHandle(const char* filename)
    {
        std::string want = normalizeName(filename);
        if (want.empty()) return -1;

        for (auto git = g_groups.rbegin(); git != g_groups.rend(); ++git)
        {
            GroupFile& grp = *git;
            for (const GroupEntry& e : grp.entries)
            {
                if (e.name != want) continue;
                short h = allocHandle();
                if (h < 0) return -1;
                g_kfiles[h] = KFile{};
                g_kfiles[h].fp = grp.fp;
                g_kfiles[h].fromGroup = true;
                g_kfiles[h].base = e.offset;
                g_kfiles[h].length = e.size;
                g_kfiles[h].pos = 0;
                return h;
            }
        }
        return -1;
    }
}

extern "C" long initgroupfile(char* filename)
{
    FILE* f = buildfile::tryOpenDisk(filename);
    if (!f) return -1;

    char magic[12] = {};
    long count = 0;
    if (std::fread(magic, 1, 12, f) != 12 || std::memcmp(magic, "KenSilverman", 12) != 0 ||
        std::fread(&count, 4, 1, f) != 1 || count < 0 || count > 65536)
    {
        std::fclose(f);
        return -1;
    }

    buildfile::GroupFile grp;
    grp.fp = f;
    grp.path = filename ? filename : "";
    grp.entries.reserve(static_cast<size_t>(count));

    long dataOffset = 16 + count * 16;
    for (long i = 0; i < count; ++i)
    {
        char rawname[13] = {};
        long size = 0;
        if (std::fread(rawname, 1, 12, f) != 12 || std::fread(&size, 4, 1, f) != 1 || size < 0)
        {
            std::fclose(f);
            return -1;
        }
        buildfile::GroupEntry e;
        e.name = buildfile::normalizeName(rawname);
        e.offset = dataOffset;
        e.size = size;
        dataOffset += size;
        grp.entries.push_back(e);
    }

    buildfile::g_groups.push_back(std::move(grp));
    return count;
}

extern "C" void uninitgroupfile()
{
    for (auto& kf : buildfile::g_kfiles)
    {
        if (kf.fp && !kf.fromGroup) std::fclose(kf.fp);
        kf = buildfile::KFile{};
    }
    for (auto& grp : buildfile::g_groups)
        if (grp.fp) std::fclose(grp.fp);
    buildfile::g_groups.clear();
}

extern "C" short kopen4load(char* filename, char searchfirst)
{
    // BUILD's searchfirst flag is used by loadboard() to force archive lookup
    // for names ending with 255. 0 = loose-file first, nonzero = GRP first.
    short h = -1;
    if (searchfirst == 0)
    {
        h = buildfile::openDiskHandle(filename);
        if (h >= 0) return h;
        return buildfile::openGroupHandle(filename);
    }
    h = buildfile::openGroupHandle(filename);
    if (h >= 0) return h;
    return buildfile::openDiskHandle(filename);
}

extern "C" short kopen4loadfrommod(char* filename, char searchfirst)
{
    return kopen4load(filename, searchfirst);
}

extern "C" long kread(short fil, void* buf, long len)
{
    if (fil <= 0 || fil >= 256 || !buildfile::g_kfiles[fil].fp || !buf || len < 0) return -1;
    buildfile::KFile& kf = buildfile::g_kfiles[fil];
    if (kf.fromGroup)
    {
        long avail = max(0L, kf.length - kf.pos);
        long todo = min(len, avail);
        std::fseek(kf.fp, kf.base + kf.pos, SEEK_SET);
        long got = static_cast<long>(std::fread(buf, 1, static_cast<size_t>(todo), kf.fp));
        kf.pos += got;
        return got;
    }
    long got = static_cast<long>(std::fread(buf, 1, static_cast<size_t>(len), kf.fp));
    kf.pos = std::ftell(kf.fp);
    return got;
}

extern "C" long uncompress(char* lzwinbuf, long compleng, char* lzwoutbuf);
extern "C" long compress(char* lzwinbuf, long uncompleng, char* lzwoutbuf);

namespace buildlzw
{
    static constexpr long LZWSIZE = 16384;
    static constexpr long LZWBUFSIZE = LZWSIZE + (LZWSIZE >> 4) + 16;
    static std::vector<unsigned char> lzwbuf1;
    static std::vector<short> lzwbuf2;
    static std::vector<short> lzwbuf3;
    static std::vector<unsigned char> lzwbuf4;
    static std::vector<unsigned char> lzwbuf5;

    static void ensure()
    {
        if (lzwbuf1.empty()) lzwbuf1.assign(static_cast<size_t>(LZWBUFSIZE), 0);
        if (lzwbuf2.empty()) lzwbuf2.assign(static_cast<size_t>(LZWBUFSIZE), 0);
        if (lzwbuf3.empty()) lzwbuf3.assign(static_cast<size_t>(LZWBUFSIZE), 0);
        if (lzwbuf4.empty()) lzwbuf4.assign(static_cast<size_t>(LZWBUFSIZE), 0);
        if (lzwbuf5.empty()) lzwbuf5.assign(static_cast<size_t>(LZWBUFSIZE), 0);
    }
    static unsigned short read16le(const void* p)
    {
        const auto* b = static_cast<const unsigned char*>(p);
        return static_cast<unsigned short>(b[0] | (b[1] << 8));
    }
    static void write16le(void* p, long v)
    {
        auto* b = static_cast<unsigned char*>(p);
        b[0] = static_cast<unsigned char>(v & 255);
        b[1] = static_cast<unsigned char>((v >> 8) & 255);
    }
    static long getbits(const unsigned char* in, long compleng, long bitcnt, long numbits)
    {
        unsigned long v = 0;
        for (long b = 0; b < numbits; ++b)
        {
            long bit = bitcnt + b;
            long byteIndex = bit >> 3;
            if (byteIndex >= compleng) break;
            if (in[byteIndex] & (1u << (bit & 7))) v |= (1UL << b);
        }
        return static_cast<long>(v);
    }
    static bool readBlockK(short fil, long& outLen)
    {
        unsigned char lenbuf[2] = {};
        if (kread(fil, lenbuf, 2) != 2) return false;
        long clen = static_cast<long>(read16le(lenbuf));
        if (clen < 0 || clen > LZWBUFSIZE) return false;
        if (kread(fil, lzwbuf5.data(), clen) != clen) return false;
        outLen = ::uncompress(reinterpret_cast<char*>(lzwbuf5.data()), clen, reinterpret_cast<char*>(lzwbuf4.data()));
        if (outLen < 0 || outLen > LZWSIZE) return false;
        return true;
    }
    static bool readBlockFile(FILE* fil, long& outLen)
    {
        unsigned char lenbuf[2] = {};
        if (!fil || std::fread(lenbuf, 1, 2, fil) != 2) return false;
        long clen = static_cast<long>(read16le(lenbuf));
        if (clen < 0 || clen > LZWBUFSIZE) return false;
        if (std::fread(lzwbuf5.data(), 1, static_cast<size_t>(clen), fil) != static_cast<size_t>(clen)) return false;
        outLen = ::uncompress(reinterpret_cast<char*>(lzwbuf5.data()), clen, reinterpret_cast<char*>(lzwbuf4.data()));
        if (outLen < 0 || outLen > LZWSIZE) return false;
        return true;
    }
}

extern "C" long uncompress(char* lzwinbuf, long compleng, char* lzwoutbuf)
{
    buildlzw::ensure();
    if (!lzwinbuf || !lzwoutbuf || compleng < 4) return 0;

    const auto* in = reinterpret_cast<const unsigned char*>(lzwinbuf);
    auto* out = reinterpret_cast<unsigned char*>(lzwoutbuf);
    long uncompleng = static_cast<long>(buildlzw::read16le(in));
    long strtot = static_cast<long>(buildlzw::read16le(in + 2));
    if (uncompleng < 0 || uncompleng > buildlzw::LZWSIZE) return 0;

    if (strtot == 0)
    {
        long n = min(uncompleng, max(0L, compleng - 4));
        if (n > 0) std::memcpy(out, in + 4, static_cast<size_t>(n));
        if (n < uncompleng) std::memset(out + n, 0, static_cast<size_t>(uncompleng - n));
        return uncompleng;
    }

    for (long i = 255; i >= 0; --i)
    {
        buildlzw::lzwbuf2[static_cast<size_t>(i)] = static_cast<short>(i);
        buildlzw::lzwbuf3[static_cast<size_t>(i)] = static_cast<short>(i);
    }

    long currstr = 256;
    long bitcnt = 32;
    long outbytecnt = 0;
    long numbits = 8;
    long oneupnumbits = 256;

    if (strtot > buildlzw::LZWBUFSIZE) strtot = buildlzw::LZWBUFSIZE;
    while (currstr < strtot && outbytecnt < uncompleng)
    {
        long dat = buildlzw::getbits(in, compleng, bitcnt, numbits) & (oneupnumbits - 1);
        bitcnt += numbits;
        if ((dat & ((oneupnumbits >> 1) - 1)) > ((currstr - 1) & ((oneupnumbits >> 1) - 1)))
        {
            dat &= ((oneupnumbits >> 1) - 1);
            bitcnt--;
        }

        buildlzw::lzwbuf3[static_cast<size_t>(currstr)] = static_cast<short>(dat);

        long leng = 0;
        long walk = dat;
        while (walk >= 256 && walk < currstr && leng < buildlzw::LZWBUFSIZE)
        {
            buildlzw::lzwbuf1[static_cast<size_t>(leng++)] = static_cast<unsigned char>(buildlzw::lzwbuf2[static_cast<size_t>(walk)] & 255);
            walk = buildlzw::lzwbuf3[static_cast<size_t>(walk)];
        }
        if (walk < 0 || walk > 255) walk = 0;

        out[outbytecnt++] = static_cast<unsigned char>(walk);
        for (long i = leng - 1; i >= 0 && outbytecnt < uncompleng; --i)
            out[outbytecnt++] = buildlzw::lzwbuf1[static_cast<size_t>(i)];

        buildlzw::lzwbuf2[static_cast<size_t>(currstr - 1)] = static_cast<short>(walk);
        buildlzw::lzwbuf2[static_cast<size_t>(currstr)] = static_cast<short>(walk);
        currstr++;
        if (currstr > oneupnumbits)
        {
            numbits++;
            oneupnumbits <<= 1;
            if (numbits > 15) numbits = 15;
        }
    }

    if (outbytecnt < uncompleng) std::memset(out + outbytecnt, 0, static_cast<size_t>(uncompleng - outbytecnt));
    return uncompleng;
}

extern "C" long compress(char* lzwinbuf, long uncompleng, char* lzwoutbuf)
{
    // Valid BUILD LZW block in the uncompressed fallback form. This is smaller-code,
    // deterministic, and remains readable by the original uncompress() path.
    if (!lzwinbuf || !lzwoutbuf || uncompleng < 0) return 0;
    if (uncompleng > buildlzw::LZWSIZE) uncompleng = buildlzw::LZWSIZE;
    buildlzw::write16le(lzwoutbuf, uncompleng);
    buildlzw::write16le(lzwoutbuf + 2, 0);
    if (uncompleng > 0) std::memcpy(lzwoutbuf + 4, lzwinbuf, static_cast<size_t>(uncompleng));
    return uncompleng + 4;
}

extern "C" long kdfread(void* buffer, long dasizeof, long count, short fil)
{
    buildlzw::ensure();
    if (!buffer || dasizeof <= 0 || count <= 0) return 0;
    if (dasizeof > buildlzw::LZWSIZE) { count *= dasizeof; dasizeof = 1; }

    auto* ptr = static_cast<unsigned char*>(buffer);
    long k = 0, kgoal = 0;
    if (!buildlzw::readBlockK(fil, kgoal)) return -1;
    if (dasizeof > kgoal) return -1;

    std::memcpy(ptr, buildlzw::lzwbuf4.data(), static_cast<size_t>(dasizeof));
    k += dasizeof;

    for (long i = 1; i < count; ++i)
    {
        if (k >= kgoal)
        {
            if (!buildlzw::readBlockK(fil, kgoal)) return -1;
            k = 0;
        }
        for (long j = 0; j < dasizeof; ++j)
        {
            unsigned char delta = (k + j < kgoal) ? buildlzw::lzwbuf4[static_cast<size_t>(k + j)] : 0;
            ptr[j + dasizeof] = static_cast<unsigned char>((ptr[j] + delta) & 255);
        }
        k += dasizeof;
        ptr += dasizeof;
    }
    return count;
}

extern "C" long dfread(void* buffer, long dasizeof, long count, FILE* fil)
{
    buildlzw::ensure();
    if (!buffer || !fil || dasizeof <= 0 || count <= 0) return 0;
    if (dasizeof > buildlzw::LZWSIZE) { count *= dasizeof; dasizeof = 1; }

    auto* ptr = static_cast<unsigned char*>(buffer);
    long k = 0, kgoal = 0;
    if (!buildlzw::readBlockFile(fil, kgoal)) return -1;
    if (dasizeof > kgoal) return -1;

    std::memcpy(ptr, buildlzw::lzwbuf4.data(), static_cast<size_t>(dasizeof));
    k += dasizeof;

    for (long i = 1; i < count; ++i)
    {
        if (k >= kgoal)
        {
            if (!buildlzw::readBlockFile(fil, kgoal)) return -1;
            k = 0;
        }
        for (long j = 0; j < dasizeof; ++j)
        {
            unsigned char delta = (k + j < kgoal) ? buildlzw::lzwbuf4[static_cast<size_t>(k + j)] : 0;
            ptr[j + dasizeof] = static_cast<unsigned char>((ptr[j] + delta) & 255);
        }
        k += dasizeof;
        ptr += dasizeof;
    }
    return count;
}

extern "C" long dfwrite(void* buffer, long dasizeof, long count, FILE* fil)
{
    buildlzw::ensure();
    if (!buffer || !fil || dasizeof <= 0 || count <= 0) return 0;
    if (dasizeof > buildlzw::LZWSIZE) { count *= dasizeof; dasizeof = 1; }

    auto* ptr = static_cast<unsigned char*>(buffer);
    long k = 0;
    std::memcpy(buildlzw::lzwbuf4.data(), ptr, static_cast<size_t>(dasizeof));
    k = dasizeof;

    auto flushBlock = [&](long bytes) -> bool
        {
            long clen = ::compress(reinterpret_cast<char*>(buildlzw::lzwbuf4.data()), bytes, reinterpret_cast<char*>(buildlzw::lzwbuf5.data()));
            unsigned char lenbuf[2];
            buildlzw::write16le(lenbuf, clen);
            return std::fwrite(lenbuf, 1, 2, fil) == 2 &&
                std::fwrite(buildlzw::lzwbuf5.data(), 1, static_cast<size_t>(clen), fil) == static_cast<size_t>(clen);
        };

    if (k > buildlzw::LZWSIZE - dasizeof)
    {
        if (!flushBlock(k)) return -1;
        k = 0;
    }

    for (long i = 1; i < count; ++i)
    {
        for (long j = 0; j < dasizeof; ++j)
            buildlzw::lzwbuf4[static_cast<size_t>(k + j)] = static_cast<unsigned char>((ptr[j + dasizeof] - ptr[j]) & 255);
        k += dasizeof;
        if (k > buildlzw::LZWSIZE - dasizeof)
        {
            if (!flushBlock(k)) return -1;
            k = 0;
        }
        ptr += dasizeof;
    }

    if (k > 0 && !flushBlock(k)) return -1;
    return count;
}


extern "C" void kclose(short fil)
{
    if (fil > 0 && fil < 256 && buildfile::g_kfiles[fil].fp)
    {
        if (!buildfile::g_kfiles[fil].fromGroup) std::fclose(buildfile::g_kfiles[fil].fp);
        buildfile::g_kfiles[fil] = buildfile::KFile{};
    }
}

extern "C" long klseek(short fil, long offset, long whence)
{
    if (fil <= 0 || fil >= 256 || !buildfile::g_kfiles[fil].fp) return -1;
    buildfile::KFile& kf = buildfile::g_kfiles[fil];
    if (kf.fromGroup)
    {
        long newpos = 0;
        if (whence == SEEK_SET) newpos = offset;
        else if (whence == SEEK_CUR) newpos = kf.pos + offset;
        else if (whence == SEEK_END) newpos = kf.length + offset;
        else return -1;
        kf.pos = min(max(newpos, 0L), kf.length);
        return kf.pos;
    }
    if (std::fseek(kf.fp, offset, whence) != 0) return -1;
    kf.pos = std::ftell(kf.fp);
    return kf.pos;
}

extern "C" long kfilelength(short fil)
{
    if (fil <= 0 || fil >= 256 || !buildfile::g_kfiles[fil].fp) return -1;
    buildfile::KFile& kf = buildfile::g_kfiles[fil];
    if (kf.fromGroup) return kf.length;
    long cur = std::ftell(kf.fp);
    std::fseek(kf.fp, 0, SEEK_END);
    long len = std::ftell(kf.fp);
    std::fseek(kf.fp, cur, SEEK_SET);
    return len;
}

extern "C" long ksqrt(long num)
{
    if (num <= 0) return 0;
    return static_cast<long>(std::sqrt(static_cast<double>(num)));
}
extern "C" long krecip(long num)
{
    if (num == 0) return 0x7fffffff;
    return divscale30(1, num);
}

LRESULT CALLBACK Engine_KeyboardWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// -----------------------------------------------------------------------------
// OpenGL 1.x renderer backend.
// -----------------------------------------------------------------------------
namespace buildgl
{
    static constexpr double PI = 3.1415926535897932384626433832795;

#ifndef BUILDGL_XY_SCALE_DIVISOR
#define BUILDGL_XY_SCALE_DIVISOR 1024.0f
#endif
#ifndef BUILDGL_Z_SCALE_DIVISOR
    // BUILD stores Z with finer granularity than X/Y. The previous 8192 divisor
    // made vertical distances about 2x too tall compared with sprites and classic
    // Duke3D room proportions. 16384 keeps a 64px, yrepeat-64 sprite near the same
    // GL height as its BUILD Z extent (64*64*4).
#define BUILDGL_Z_SCALE_DIVISOR 16384.0f
#endif
#ifndef BUILDGL_CLOSE_MAP_UNITS
// Keep the renderer close plane in BUILD map units, then convert it to GL
// units for gluPerspective(). A fixed GL near value of 0.05 clips at about
// 51 BUILD XY units when BUILDGL_XY_SCALE_DIVISOR is 1024.
#define BUILDGL_CLOSE_MAP_UNITS 4.0
#endif
#ifndef BUILDGL_FAR_PLANE
#define BUILDGL_FAR_PLANE 10000.0
#endif
#ifndef BUILDGL_HORIZONTAL_FOV_DEGREES
// gluPerspective() wants vertical FOV. Classic BUILD/Duke-style tuning is much
// closer to a ~90 degree horizontal FOV; using 90 as vertical FOV made the view
// show too much ceiling/floor and exaggerated Z.
#define BUILDGL_HORIZONTAL_FOV_DEGREES 90.0
#endif
#ifndef BUILDGL_VERTICAL_FOV_DEGREES
// Set this > 1.0 to override the horizontal-FOV-derived value directly.
#define BUILDGL_VERTICAL_FOV_DEGREES 0.0
#endif
#ifndef BUILDGL_SKY_SCREENSPACE
// Classic BUILD draws parallax skies as a screen-space angular backdrop.
// Keep this enabled unless you explicitly want the old GL cylinder fallback.
#define BUILDGL_SKY_SCREENSPACE 1
#endif
#ifndef BUILDGL_SKY_VERTICAL_PIXELS
// BUILD's original view is 320x200. Use virtual 200px vertical sky math so
// changing the Win32/OpenGL resolution does not stretch the sky texture.
#define BUILDGL_SKY_VERTICAL_PIXELS 200.0f
#endif
#ifndef BUILDGL_FP_OFF_WIDTH
// Duke3D's classic software frame buffer is 320x200.  Override these at
// compile time if your fork really uses a 320x220 compatibility surface.
#define BUILDGL_FP_OFF_WIDTH 320
#endif
#ifndef BUILDGL_FP_OFF_HEIGHT
#define BUILDGL_FP_OFF_HEIGHT 200
#endif
#ifndef BUILDGL_FP_OFF_POT_BACKING
// Keep the visible FP_OFF surface fixed at BUILDGL_FP_OFF_WIDTH x
// BUILDGL_FP_OFF_HEIGHT.  Set this to 1 only for ancient GL drivers that reject
// non-power-of-two GL_TEXTURE_2D uploads; texture coordinates will still crop
// back to the fixed virtual surface.
#define BUILDGL_FP_OFF_POT_BACKING 0
#endif

    static constexpr float XY_SCALE = 1.0f / BUILDGL_XY_SCALE_DIVISOR;
    static constexpr int FP_OFF_WIDTH = (BUILDGL_FP_OFF_WIDTH > 0) ? BUILDGL_FP_OFF_WIDTH : 320;
    static constexpr int FP_OFF_HEIGHT = (BUILDGL_FP_OFF_HEIGHT > 0) ? BUILDGL_FP_OFF_HEIGHT : 200;
    static constexpr float Z_SCALE = 1.0f / BUILDGL_Z_SCALE_DIVISOR;

    struct Vec2 { double x, y; };
    struct Vec3 { float x, y, z; };

    struct GLState
    {
        HWND hwnd = nullptr;
        HDC hdc = nullptr;
        HGLRC hglrc = nullptr;
        int w = 640, h = 480;
        bool ready = false;
        bool quit = false;
        bool frameActive = false;
        bool opaqueDrawn = false;
        bool maskedDrawn = false;
        // Textures are cached per BUILD lookup palette. A tile's source ART
        // data is still loaded once, but palette remaps such as access-card
        // colors, enemy variants, sector tints, and HUD palettes need distinct
        // GL textures because this fixed-function path uploads true RGBA.
        std::array<std::array<GLuint, MAXPALOOKUPS>, MAXTILES> tex{};
        std::array<std::array<unsigned char, MAXPALOOKUPS>, MAXTILES> texReady{};
        std::array<std::array<unsigned char, MAXPALOOKUPS>, MAXTILES> texDirty{};
        std::array<unsigned char, MAXTILES> fallbackTile{};
        std::vector<unsigned char> tileData[MAXTILES];

        // Compatibility path for old code that asks for Duke3D's classic
        // 320x200 linear 8-bit framebuffer through FP_OFF().  When FP_OFF() is
        // called, that fixed virtual surface is considered dirty for this frame,
        // converted through the current BUILD palette, uploaded, and drawn as
        // the last full-screen pass immediately before SwapBuffers().
        std::vector<unsigned char> fpOffBuffer;
        std::vector<unsigned char> fpOffRgba;
        unsigned char* fpOffSource = nullptr;
        GLuint fpOffTex = 0;
        bool fpOffTexReady = false;
        bool fpOffDirtyThisFrame = false;
        int fpOffTexW = 0;
        int fpOffTexH = 0;
    } g;

    static void syncClientSize(int width, int height, bool resetView)
    {
        g.w = max(1, width);
        g.h = max(1, height);

        xdim = g.w;
        ydim = g.h;
        bytesperline = xdim;
        imageSize = xdim * ydim;

        for (long y = 0, p = 0; y <= ydim && y <= MAXYDIM; ++y, p += bytesperline)
            ylookup[y] = p;

        if (resetView)
            ::setview(0, 0, xdim - 1, ydim - 1);
    }

    static Vec3 buildToGl(long x, long y, long z)
    {
        return Vec3{ static_cast<float>(x) * XY_SCALE, static_cast<float>(-z) * Z_SCALE, static_cast<float>(y) * XY_SCALE };
    }
    static double radFromBuildAngle(short ang)
    {
        return (static_cast<double>(ang & 2047) * 2.0 * PI) / 2048.0;
    }
    static float shadeFactor(long shade)
    {
        float f = 1.0f - static_cast<float>(shade) / 64.0f;
        if (f < 0.18f) f = 0.18f;
        if (f > 1.25f) f = 1.25f;
        return f;
    }
    static void glColorShade(long shade, float alpha = 1.0f)
    {
        const float f = shadeFactor(shade);
        glColor4f(f, f, f, alpha);
    }

    static double clampDouble(double v, double lo, double hi)
    {
        return (v < lo) ? lo : ((v > hi) ? hi : v);
    }

    static double computeVerticalFovDegrees(double aspect)
    {
        aspect = clampDouble(aspect, 0.10, 10.0);

        const double directVerticalFov = static_cast<double>(BUILDGL_VERTICAL_FOV_DEGREES);
        if (directVerticalFov > 1.0)
            return clampDouble(directVerticalFov, 30.0, 120.0);

        const double horizontalFov = clampDouble(static_cast<double>(BUILDGL_HORIZONTAL_FOV_DEGREES), 45.0, 140.0) * PI / 180.0;
        const double verticalFov = 2.0 * std::atan(std::tan(horizontalFov * 0.5) / aspect);
        return clampDouble(verticalFov * 180.0 / PI, 30.0, 120.0);
    }

    static double horizToPitchSlope(long horiz, double verticalFovDegrees)
    {
        // globalhoriz is centered on 100 in the classic 320x200 view. Convert it
        // to a view slope using the actual vertical FOV instead of assuming a
        // 90-degree vertical lens.
        const double halfFovSlope = std::tan((verticalFovDegrees * PI / 180.0) * 0.5);
        return (100.0 - static_cast<double>(horiz)) * (halfFovSlope / 100.0);
    }

    static float clampFloat(float v, float lo, float hi)
    {
        return (v < lo) ? lo : ((v > hi) ? hi : v);
    }

    static signed char signedPicanmByte(short pic, int shift)
    {
        if ((unsigned short)pic >= MAXTILES) return 0;
        return static_cast<signed char>((picanm[pic] >> shift) & 255);
    }

#ifndef BUILDGL_FOG_DENSITY_DIVISOR
    // The renderer scales BUILD X/Y map units down by 1024 before OpenGL sees them.
    // Fog therefore needs a much stronger world-space density than the previous
    // 262144 divisor. 65536 makes sector visibility changes visible at normal
    // Duke-sized room distances without instantly blanketing default sectors.
#define BUILDGL_FOG_DENSITY_DIVISOR 65536.0f
#endif
#ifndef BUILDGL_FOG_STRENGTH
// Extra user-facing multiplier for quick tuning. Raise to 1.5f/2.0f if you
// want heavier fog globally, or lower it if stock maps become too hazy.
#define BUILDGL_FOG_STRENGTH 1.0f
#endif
#ifndef BUILDGL_FOG_MAX_DENSITY
#define BUILDGL_FOG_MAX_DENSITY 0.45f
#endif
#ifndef BUILDGL_FOG_MODE
// GL_EXP gives visible room-scale falloff; GL_EXP2 was too weak up close with
// this port's scaled coordinates.
#define BUILDGL_FOG_MODE GL_EXP
#endif

    static long combinedVisibilityForSector(short sectnum)
    {
        // sector[].visibility is an 8-bit BUILD visibility modifier, not an
        // absolute fog-density override. A sector value of 0 means normal
        // visibility because the classic factor is (visibility + 16).
        const long baseVisibility = max(0L, visibility);
        unsigned char sectorFactor = 16;

        if (sectnum >= 0 && sectnum < numsectors)
        {
            const int sectorVis = static_cast<signed char>(sector[sectnum].visibility);
            sectorFactor = static_cast<unsigned char>(sectorVis + 16);
        }

        return baseVisibility * static_cast<long>(sectorFactor);
    }

    static float fogDensityForSector(short sectnum)
    {
        const long combinedVisibility = combinedVisibilityForSector(sectnum);
        if (combinedVisibility <= 0) return 0.0f;

        // BUILD map coordinates are scaled down before reaching OpenGL, so the
        // old palette-lookup visibility value must be converted to GL-world
        // density. This default keeps normal Duke visibility visible at room
        // distance while letting sector visibility values noticeably thicken or
        // clear the fog.
        const float density = (static_cast<float>(combinedVisibility) / BUILDGL_FOG_DENSITY_DIVISOR) *
            BUILDGL_FOG_STRENGTH;
        return clampFloat(density, 0.0f, BUILDGL_FOG_MAX_DENSITY);
    }

    static void applySectorFog(short sectnum)
    {
        const float density = fogDensityForSector(sectnum);
        if (density <= 0.0f)
        {
            glDisable(GL_FOG);
            return;
        }

        const GLfloat fogColor[4] =
        {
            static_cast<GLfloat>((static_cast<unsigned char>(palette[0]) << 2) / 255.0f),
            static_cast<GLfloat>((static_cast<unsigned char>(palette[1]) << 2) / 255.0f),
            static_cast<GLfloat>((static_cast<unsigned char>(palette[2]) << 2) / 255.0f),
            1.0f
        };
        glEnable(GL_FOG);
        glFogi(GL_FOG_MODE, BUILDGL_FOG_MODE);
        glFogfv(GL_FOG_COLOR, fogColor);
        glFogf(GL_FOG_DENSITY, density);
        glHint(GL_FOG_HINT, GL_DONT_CARE);
    }

#ifndef BUILDGL_AUTOMAP_VIRTUAL_WIDTH
#define BUILDGL_AUTOMAP_VIRTUAL_WIDTH 320.0
#endif
#ifndef BUILDGL_AUTOMAP_VIRTUAL_HEIGHT
#define BUILDGL_AUTOMAP_VIRTUAL_HEIGHT 200.0
#endif

    static long scaleAutomapDeltaX(long x)
    {
        return static_cast<long>(std::lround(
            static_cast<double>(x) * static_cast<double>(max(1, g.w)) / BUILDGL_AUTOMAP_VIRTUAL_WIDTH));
    }

    static long scaleAutomapDeltaY(long y)
    {
        return static_cast<long>(std::lround(
            static_cast<double>(y) * static_cast<double>(max(1, g.h)) / BUILDGL_AUTOMAP_VIRTUAL_HEIGHT));
    }



    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
    {
        switch (msg)
        {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        case WM_CHAR:
        case WM_ACTIVATEAPP:
        case WM_KILLFOCUS:
            return Engine_KeyboardWndProc(hwnd, msg, wp, lp);

        case WM_CLOSE: g.quit = true; DestroyWindow(hwnd); return 0;
        case WM_DESTROY: g.quit = true; PostQuitMessage(0); return 0;
        case WM_SIZE:
            syncClientSize(LOWORD(lp), HIWORD(lp), true);
            return 0;
        default: return DefWindowProc(hwnd, msg, wp, lp);
        }
    }

    static bool createWindow(int width, int height, const char* title)
    {
        if (g.ready)
        {
            if (g.hwnd)
            {
                DWORD style = static_cast<DWORD>(GetWindowLongA(g.hwnd, GWL_STYLE));
                RECT wr{ 0, 0, width, height };
                AdjustWindowRect(&wr, style, FALSE);
                SetWindowPos(g.hwnd, nullptr, 0, 0, wr.right - wr.left, wr.bottom - wr.top, SWP_NOMOVE | SWP_NOZORDER);

                RECT cr{};
                GetClientRect(g.hwnd, &cr);
                syncClientSize(cr.right - cr.left, cr.bottom - cr.top, true);
            }
            else
            {
                syncClientSize(width, height, true);
            }
            return true;
        }

        HINSTANCE inst = GetModuleHandle(nullptr);
        WNDCLASSA wc{};
        wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = wndProc;
        wc.hInstance = inst;
        wc.lpszClassName = "BuildGL1DropInWindow";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        RegisterClassA(&wc);

        DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
        RECT rc{ 0, 0, width, height };
        AdjustWindowRect(&rc, style, FALSE);

        g.hwnd = CreateWindowA(wc.lpszClassName, title ? title : "Duke Nukem 3D",
            style, CW_USEDEFAULT, CW_USEDEFAULT,
            rc.right - rc.left, rc.bottom - rc.top,
            nullptr, nullptr, inst, nullptr);
        if (!g.hwnd) return false;

        g.hdc = GetDC(g.hwnd);
        PIXELFORMATDESCRIPTOR pfd{};
        pfd.nSize = sizeof(pfd);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cDepthBits = 24;
        pfd.cAlphaBits = 8;
        pfd.iLayerType = PFD_MAIN_PLANE;
        int pf = ChoosePixelFormat(g.hdc, &pfd);
        if (pf <= 0 || !SetPixelFormat(g.hdc, pf, &pfd)) return false;

        g.hglrc = wglCreateContext(g.hdc);
        if (!g.hglrc || !wglMakeCurrent(g.hdc, g.hglrc)) return false;

        g.ready = true;
        RECT cr{};
        GetClientRect(g.hwnd, &cr);
        syncClientSize(cr.right - cr.left, cr.bottom - cr.top, true);

        glEnable(GL_TEXTURE_2D);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.05f);
        glDisable(GL_CULL_FACE);
        glClearDepth(1.0);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        return true;
    }

    static void destroy()
    {
        if (g.hglrc)
        {
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(g.hglrc);
            g.hglrc = nullptr;
        }
        if (g.hwnd && g.hdc)
        {
            ReleaseDC(g.hwnd, g.hdc);
            g.hdc = nullptr;
        }
        if (g.hwnd)
        {
            DestroyWindow(g.hwnd);
            g.hwnd = nullptr;
        }
        g.ready = false;
    }

    static void pump()
    {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    static long slopeZ(short sectnum, long x, long y, bool ceiling)
    {
        if (sectnum < 0 || sectnum >= numsectors) return 0;
        sectortype& sec = sector[sectnum];
        long z = ceiling ? sec.ceilingz : sec.floorz;
        short stat = ceiling ? sec.ceilingstat : sec.floorstat;
        if ((stat & 2) == 0) return z;
        if (sec.wallptr < 0 || sec.wallptr >= numwalls) return z;
        walltype& wal = wall[sec.wallptr];
        if (wal.point2 < 0 || wal.point2 >= numwalls) return z;
        walltype& wal2 = wall[wal.point2];
        long dx = wal2.x - wal.x;
        long dy = wal2.y - wal.y;
        long len = ksqrt(dx * dx + dy * dy) << 5;
        if (len == 0) return z;
        long j = dmulscale3(dx, y - wal.y, -dy, x - wal.x);
        long heinum = ceiling ? sec.ceilingheinum : sec.floorheinum;
        return z + scale(heinum, j, len);
    }

    static std::vector<std::vector<int>> collectLoops(short sectnum)
    {
        std::vector<std::vector<int>> loops;
        if (sectnum < 0 || sectnum >= numsectors) return loops;
        int start = sector[sectnum].wallptr;
        int end = start + sector[sectnum].wallnum;
        if (start < 0 || end > numwalls) return loops;

        std::vector<char> seen(static_cast<size_t>(end - start), 0);
        for (int wi = start; wi < end; ++wi)
        {
            if (seen[static_cast<size_t>(wi - start)]) continue;
            std::vector<int> loop;
            int cur = wi;
            for (int guard = 0; guard < MAXWALLS; ++guard)
            {
                if (cur < start || cur >= end) break;
                if (seen[static_cast<size_t>(cur - start)] && cur != wi) break;
                seen[static_cast<size_t>(cur - start)] = 1;
                loop.push_back(cur);
                int next = wall[cur].point2;
                if (next == wi) break;
                cur = next;
            }
            if (loop.size() >= 3) loops.push_back(loop);
        }
        return loops;
    }

    static double area2(const std::vector<Vec2>& p)
    {
        double a = 0.0;
        for (size_t i = 0, j = p.size() - 1; i < p.size(); j = i++)
            a += (p[j].x * p[i].y - p[i].x * p[j].y);
        return a * 0.5;
    }
    static double cross(const Vec2& a, const Vec2& b, const Vec2& c)
    {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    }
    static bool pointInTri(const Vec2& p, const Vec2& a, const Vec2& b, const Vec2& c)
    {
        const double c1 = cross(a, b, p);
        const double c2 = cross(b, c, p);
        const double c3 = cross(c, a, p);
        return (c1 >= -1e-7 && c2 >= -1e-7 && c3 >= -1e-7);
    }
    static std::vector<int> triangulate(const std::vector<Vec2>& points)
    {
        std::vector<int> out;
        const int n = static_cast<int>(points.size());
        if (n < 3) return out;

        std::vector<int> idx(n);
        for (int i = 0; i < n; ++i) idx[i] = i;
        if (area2(points) < 0.0) std::reverse(idx.begin(), idx.end());

        int guard = 0;
        while (idx.size() > 3 && guard++ < n * n)
        {
            bool clipped = false;
            int m = static_cast<int>(idx.size());
            for (int i = 0; i < m; ++i)
            {
                int i0 = idx[(i + m - 1) % m], i1 = idx[i], i2 = idx[(i + 1) % m];
                const Vec2& a = points[i0]; const Vec2& b = points[i1]; const Vec2& c = points[i2];
                if (cross(a, b, c) <= 1e-7) continue;
                bool contains = false;
                for (int j = 0; j < m; ++j)
                {
                    int q = idx[j];
                    if (q == i0 || q == i1 || q == i2) continue;
                    if (pointInTri(points[q], a, b, c)) { contains = true; break; }
                }
                if (contains) continue;
                out.push_back(i0); out.push_back(i1); out.push_back(i2);
                idx.erase(idx.begin() + i);
                clipped = true;
                break;
            }
            if (!clipped) break;
        }
        if (idx.size() == 3) { out.push_back(idx[0]); out.push_back(idx[1]); out.push_back(idx[2]); }
        return out;
    }

    static std::string makeArtName(int fileNum)
    {
        std::string path = artfilename[0] ? artfilename : "tiles000.art";

        char digits[4];
        std::snprintf(digits, sizeof(digits), "%03d", fileNum % 1000);

        // Match the original BUILD convention while also supporting callers
        // that pass a directory-qualified ART path. The old code always edited
        // buf[5..7], which only works for exactly "tiles000.art" and makes
        // higher ART files fail to load when the name is "C:\...\tiles000.art".
        const size_t slash = path.find_last_of("/\\");
        const size_t nameStart = (slash == std::string::npos) ? 0 : slash + 1;
        size_t dot = path.find_last_of('.');
        if (dot == std::string::npos || dot < nameStart) dot = path.size();

        size_t digitRun = std::string::npos;
        if (dot >= nameStart + 3)
        {
            for (size_t i = dot; i-- > nameStart + 2; )
            {
                if (std::isdigit(static_cast<unsigned char>(path[i])) &&
                    std::isdigit(static_cast<unsigned char>(path[i - 1])) &&
                    std::isdigit(static_cast<unsigned char>(path[i - 2])))
                {
                    digitRun = i - 2;
                    break;
                }
            }
        }

        if (digitRun != std::string::npos)
        {
            path[digitRun + 0] = digits[0];
            path[digitRun + 1] = digits[1];
            path[digitRun + 2] = digits[2];
            return path;
        }

        const std::string dir = (nameStart == 0) ? std::string() : path.substr(0, nameStart);
        char leaf[32];
        std::snprintf(leaf, sizeof(leaf), "tiles%03d.art", fileNum % 1000);
        return dir + leaf;
    }

    static void markTileDirty(short tilenum)
    {
        if ((unsigned short)tilenum >= MAXTILES) return;
        for (int pal = 0; pal < MAXPALOOKUPS; ++pal)
            g.texDirty[tilenum][pal] = 1;
    }

    static void markPaletteDirty(long palnum)
    {
        if (palnum < 0 || palnum >= MAXPALOOKUPS)
        {
            for (short tile = 0; tile < MAXTILES; ++tile)
                markTileDirty(tile);
            return;
        }

        for (int tile = 0; tile < MAXTILES; ++tile)
            g.texDirty[tile][palnum] = 1;
    }

    static void markAllTexturesDirty()
    {
        for (short tile = 0; tile < MAXTILES; ++tile)
            markTileDirty(tile);
    }

    static void purgeTextureCache()
    {
        if (g.ready && g.hdc && g.hglrc)
            wglMakeCurrent(g.hdc, g.hglrc);

        for (int tile = 0; tile < MAXTILES; ++tile)
        {
            for (int pal = 0; pal < MAXPALOOKUPS; ++pal)
            {
                if (g.texReady[tile][pal])
                {
                    GLuint t = g.tex[tile][pal];
                    if (g.ready && t != 0) glDeleteTextures(1, &t);
                    g.tex[tile][pal] = 0;
                    g.texReady[tile][pal] = 0;
                }
                g.texDirty[tile][pal] = 1;
            }
        }

        if (g.fpOffTexReady)
        {
            GLuint t = g.fpOffTex;
            if (g.ready && t != 0) glDeleteTextures(1, &t);
            g.fpOffTex = 0;
            g.fpOffTexReady = false;
            g.fpOffTexW = 0;
            g.fpOffTexH = 0;
        }
    }

    // Duke3D palette swaps live in LOOKUP.DAT, not in PALETTE.DAT's shade
    // ramps. Keep the renderer's true-color texture cache keyed by these 256
    // entry remap tables so sprite/wall/sector pal values pick the intended
    // color variant instead of accidentally indexing a darkness table.
    static std::array<std::array<unsigned char, 256>, MAXPALOOKUPS> lookupPal{};
    static std::array<unsigned char, MAXPALOOKUPS> lookupPalReady{};

    static void resetLookupPalettes()
    {
        for (int pal = 0; pal < MAXPALOOKUPS; ++pal)
        {
            for (int i = 0; i < 256; ++i)
                lookupPal[pal][i] = static_cast<unsigned char>(i);
            lookupPalReady[pal] = (pal == 0) ? 1 : 0;
        }
    }

    static void installLookupPalette(unsigned char palnum, const unsigned char* table)
    {
        const int pal = static_cast<int>(palnum);
        if (pal < 0 || pal >= MAXPALOOKUPS || !table) return;
        for (int i = 0; i < 256; ++i)
            lookupPal[pal][i] = table[i];
        lookupPalReady[pal] = 1;
        markPaletteDirty(pal);
    }

    static void installIdentityLookupPalette(long palnum)
    {
        if (palnum < 0 || palnum >= MAXPALOOKUPS) return;
        for (int i = 0; i < 256; ++i)
            lookupPal[palnum][i] = static_cast<unsigned char>(i);
        lookupPalReady[palnum] = (palnum == 0) ? 1 : 0;
        markPaletteDirty(palnum);
    }

    static void loadLookupDatPalettes()
    {
        short fil = ::kopen4load((char*)"lookup.dat", 0);
        if (fil < 0) fil = ::kopen4load((char*)"LOOKUP.DAT", 0);
        if (fil < 0) return;

        unsigned char count = 0;
        if (::kread(fil, &count, 1) != 1)
        {
            ::kclose(fil);
            return;
        }

        for (int n = 0; n < static_cast<int>(count); ++n)
        {
            unsigned char palnum = 0;
            unsigned char table[256] = {};
            if (::kread(fil, &palnum, 1) != 1) break;
            if (::kread(fil, table, 256) != 256) break;
            installLookupPalette(palnum, table);
        }
        ::kclose(fil);
    }

    static unsigned char remapPaletteIndex(unsigned char idx, unsigned char palnum)
    {
        if (idx == 255) return idx;

        const int palSlot = static_cast<int>(palnum);
        if (palSlot > 0 && palSlot < MAXPALOOKUPS && lookupPalReady[palSlot])
            return lookupPal[palSlot][idx];

        // makepalookup() can create runtime remaps after LOOKUP.DAT has been
        // read. Use row 0 of those generated tables as a last-resort remap, but
        // do not fall back to palookup[0]: PALETTE.DAT palookup[0] is a shade
        // ramp, not a sprite palette swap.
        if (palSlot > 0 && palSlot < MAXPALOOKUPS && palookup[palSlot])
            return static_cast<unsigned char>(palookup[palSlot][idx]);

        return idx;
    }

    static bool tileHasArt(short tilenum)
    {
        return (unsigned short)tilenum < MAXTILES &&
            tilesizx[tilenum] > 0 && tilesizy[tilenum] > 0;
    }

    static void ensureFallbackTile(short tilenum)
    {
        if (tilenum < 0 || tilenum >= MAXTILES) return;
        if (tilesizx[tilenum] > 0 && tilesizy[tilenum] > 0 && waloff[tilenum]) return;
        tilesizx[tilenum] = 64; tilesizy[tilenum] = 64;
        g.tileData[tilenum].resize(64 * 64);
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x)
                g.tileData[tilenum][x * 64 + y] = static_cast<unsigned char>(((x >> 3) ^ (y >> 3)) & 1 ? 180 : 96);
        waloff[tilenum] = BUILDGL_PTR_TO_LONG(g.tileData[tilenum].data());
        g.fallbackTile[tilenum] = 1;
        picsiz[tilenum] = (6 | (6 << 4));
        markTileDirty(tilenum);
    }

    static void uploadTexture(short tilenum, unsigned char palnum)
    {
        if (tilenum < 0 || tilenum >= MAXTILES) tilenum = 0;
        const int palSlot = static_cast<int>(palnum) & (MAXPALOOKUPS - 1);

        if (!waloff[tilenum]) ensureFallbackTile(tilenum);
        if (g.texReady[tilenum][palSlot] && !g.texDirty[tilenum][palSlot]) return;

        const int w = max(1L, tilesizx[tilenum]);
        const int h = max(1L, tilesizy[tilenum]);
        const unsigned char* src = reinterpret_cast<const unsigned char*>(waloff[tilenum]);
        std::vector<unsigned char> rgba(static_cast<size_t>(w) * h * 4);

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                unsigned char idx = src[x * h + y]; // ART is column-major.
                unsigned char* d = &rgba[(static_cast<size_t>(y) * w + x) * 4];
                if (idx == 255)
                {
                    d[0] = d[1] = d[2] = 0; d[3] = 0;
                }
                else
                {
                    idx = remapPaletteIndex(idx, static_cast<unsigned char>(palSlot));
                    d[0] = static_cast<unsigned char>(static_cast<unsigned char>(palette[idx * 3 + 0]) << 2);
                    d[1] = static_cast<unsigned char>(static_cast<unsigned char>(palette[idx * 3 + 1]) << 2);
                    d[2] = static_cast<unsigned char>(static_cast<unsigned char>(palette[idx * 3 + 2]) << 2);
                    d[3] = 255;
                }
            }
        }

        if (!g.texReady[tilenum][palSlot])
        {
            glGenTextures(1, &g.tex[tilenum][palSlot]);
            g.texReady[tilenum][palSlot] = 1;
        }
        glBindTexture(GL_TEXTURE_2D, g.tex[tilenum][palSlot]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
        g.texDirty[tilenum][palSlot] = 0;
    }

    static bool bindTilePal(short tilenum, unsigned char palnum, bool allowFallback = true)
    {
        if (tilenum < 0 || tilenum >= MAXTILES)
        {
            if (!allowFallback) return false;
            tilenum = 0;
        }

        if (!allowFallback && g.fallbackTile[tilenum]) return false;

        const int palSlot = static_cast<int>(palnum) & (MAXPALOOKUPS - 1);
        if (!waloff[tilenum]) loadtile(tilenum);
        if (!waloff[tilenum])
        {
            if (!allowFallback) return false;
            ensureFallbackTile(tilenum);
        }

        if (!allowFallback && g.fallbackTile[tilenum]) return false;
        if (!waloff[tilenum] || tilesizx[tilenum] <= 0 || tilesizy[tilenum] <= 0)
            return false;

        uploadTexture(tilenum, static_cast<unsigned char>(palSlot));
        glBindTexture(GL_TEXTURE_2D, g.tex[tilenum][palSlot]);
        return true;
    }

    static bool bindTile(short tilenum, bool allowFallback = true)
    {
        return bindTilePal(tilenum, 0, allowFallback);
    }

    static float tilePanningScale(short tilenum, bool yAxis)
    {
        if ((unsigned short)tilenum >= MAXTILES) return 1.0f;
        const int shift = yAxis ? ((picsiz[tilenum] >> 4) & 15) : (picsiz[tilenum] & 15);
        long powSize = 1L << min(max(shift, 0), 15);
        const long actual = yAxis ? tilesizy[tilenum] : tilesizx[tilenum];

        // BUILD stores panning in 0..255 over the power-of-two tile span used
        // by the software renderer, not in raw texels. For 64px art, one pan
        // unit is 64/256 = 0.25 texels. Using the raw byte made offsets 4x too
        // large on common Duke3D textures and looked randomly wrong whenever a
        // map relied on panning instead of a zero offset.
        if (powSize < max(1L, actual)) powSize <<= 1;
        return static_cast<float>(max(1L, powSize));
    }

    static float panningToTexels(short tilenum, unsigned char pan, bool yAxis)
    {
        return static_cast<float>(pan) * tilePanningScale(tilenum, yAxis) / 256.0f;
    }

    static void calcFloorTex(short tilenum, const sectortype& sec, bool ceiling, long x, long y, float& u, float& v)
    {
        const float tw = static_cast<float>(max(1L, tilesizx[tilenum]));
        const float th = static_cast<float>(max(1L, tilesizy[tilenum]));
        const short stat = ceiling ? sec.ceilingstat : sec.floorstat;

        // BUILD floors/ceilings use roughly one texture pixel per 16 map units
        // by default. Bit 3 ("E" / texture expansion) doubles that texel rate.
        const float mapToTexel = (stat & 8) ? (1.0f / 8.0f) : (1.0f / 16.0f);

        float uPix = 0.0f;
        float vPix = 0.0f;

        if ((stat & 64) && sec.wallptr >= 0 && sec.wallptr < numwalls)
        {
            const walltype& w0 = wall[sec.wallptr];
            if (w0.point2 >= 0 && w0.point2 < numwalls)
            {
                const walltype& w1 = wall[w0.point2];
                const double dx = static_cast<double>(w1.x - w0.x);
                const double dy = static_cast<double>(w1.y - w0.y);
                double len = std::sqrt(dx * dx + dy * dy);
                if (len < 1.0) len = 1.0;

                const double rx = static_cast<double>(x - w0.x);
                const double ry = static_cast<double>(y - w0.y);

                // Relative floor/ceiling alignment follows the sector's first
                // wall. This is critical for nested sectors such as bins,
                // ledges, and moving platforms whose flats should not inherit
                // nearby wall-looking coordinates.
                uPix = static_cast<float>((rx * dx + ry * dy) / len) * mapToTexel;
                vPix = static_cast<float>((ry * dx - rx * dy) / len) * mapToTexel;
            }
        }
        else
        {
            uPix = static_cast<float>(x) * mapToTexel;
            vPix = static_cast<float>(ceiling ? -y : y) * mapToTexel;
        }

        if (stat & 4) std::swap(uPix, vPix);
        if (stat & 16) uPix = -uPix;
        if (stat & 32) vPix = -vPix;

        uPix += panningToTexels(tilenum, static_cast<unsigned char>(ceiling ? sec.ceilingxpanning : sec.floorxpanning), false);
        vPix += panningToTexels(tilenum, static_cast<unsigned char>(ceiling ? sec.ceilingypanning : sec.floorypanning), true);

        u = uPix / tw;
        v = vPix / th;
    }

    static void texFloor(short tilenum, const sectortype& sec, bool ceiling, long x, long y)
    {
        float u = 0.0f, v = 0.0f;
        calcFloorTex(tilenum, sec, ceiling, x, y, u, v);
        glTexCoord2f(u, v);
    }

    static void vertexBuild(long x, long y, long z)
    {
        Vec3 v = buildToGl(x, y, z);
        glVertex3f(v.x, v.y, v.z);
    }

    struct FlatTessVertex
    {
        GLdouble pos[3] = {};
        GLfloat uv[2] = {};
    };

    struct FlatTessContext
    {
        std::vector<std::unique_ptr<FlatTessVertex>> owned;
    };

    static FlatTessContext* g_flatTessContext = nullptr;

    static void CALLBACK flatTessBeginCB(GLenum type)
    {
        glBegin(type);
    }

    static void CALLBACK flatTessEndCB()
    {
        glEnd();
    }

    static void CALLBACK flatTessVertexCB(void* data)
    {
        const FlatTessVertex* v = static_cast<const FlatTessVertex*>(data);
        if (!v) return;
        glTexCoord2f(v->uv[0], v->uv[1]);
        glVertex3dv(v->pos);
    }

    static void CALLBACK flatTessCombineCB(GLdouble coords[3], void* vertexData[4], GLfloat weight[4], void** outData)
    {
        if (!g_flatTessContext || !outData) return;

        std::unique_ptr<FlatTessVertex> v(new FlatTessVertex());
        v->pos[0] = coords[0];
        v->pos[1] = coords[1];
        v->pos[2] = coords[2];
        v->uv[0] = 0.0f;
        v->uv[1] = 0.0f;

        for (int i = 0; i < 4; ++i)
        {
            const FlatTessVertex* in = static_cast<const FlatTessVertex*>(vertexData[i]);
            if (!in) continue;
            v->uv[0] += in->uv[0] * weight[i];
            v->uv[1] += in->uv[1] * weight[i];
        }

        *outData = v.get();
        g_flatTessContext->owned.push_back(std::move(v));
    }

    struct SkySelection
    {
        bool valid = false;
        short pic = 0;
        signed char shade = 0;
        char pal = 0;
        short stat = 0;
        unsigned char xpanning = 0;
        unsigned char ypanning = 0;
        bool fromCeiling = true;
    };

    static bool sectorBitIsCurrentlyVisible(short sectnum)
    {
        if (sectnum < 0 || sectnum >= numsectors) return false;

        bool haveMask = false;
        const int nbytes = (numsectors + 7) >> 3;
        for (int i = 0; i < nbytes; ++i)
        {
            if (gotsector[i] != 0)
            {
                haveMask = true;
                break;
            }
        }

        if (!haveMask) return true;
        return (gotsector[sectnum >> 3] & pow2char[sectnum & 7]) != 0;
    }

    static SkySelection selectParallaxSky()
    {
        auto testSector = [](short si, bool ceiling) -> SkySelection
            {
                SkySelection out;
                if (si < 0 || si >= numsectors || !sectorBitIsCurrentlyVisible(si)) return out;
                const sectortype& sec = sector[si];
                const short stat = ceiling ? sec.ceilingstat : sec.floorstat;
                if ((stat & 1) == 0) return out;

                out.valid = true;
                out.pic = ceiling ? sec.ceilingpicnum : sec.floorpicnum;
                out.shade = ceiling ? sec.ceilingshade : sec.floorshade;
                out.pal = ceiling ? sec.ceilingpal : sec.floorpal;
                out.stat = stat;
                out.xpanning = static_cast<unsigned char>(ceiling ? sec.ceilingxpanning : sec.floorxpanning);
                out.ypanning = static_cast<unsigned char>(ceiling ? sec.ceilingypanning : sec.floorypanning);
                out.fromCeiling = ceiling;
                if ((unsigned short)out.pic >= MAXTILES) out.pic = 0;
                if (picanm[out.pic] & 192)
                    out.pic = static_cast<short>(out.pic + animateoffs(out.pic, si));
                return out;
            };

        // Prefer the player's current sector, then fall back to any visible
        // parallaxed surface. This avoids using an unrelated sky from a sector
        // that was loaded but not reached by scansector().
        SkySelection sky = testSector(globalcursectnum, true);
        if (sky.valid) return sky;
        sky = testSector(globalcursectnum, false);
        if (sky.valid) return sky;

        for (short si = 0; si < numsectors; ++si)
        {
            sky = testSector(si, true);
            if (sky.valid) return sky;
            sky = testSector(si, false);
            if (sky.valid) return sky;
        }
        return SkySelection{};
    }

    static short parallaxSkyTile(const SkySelection& sky, int tileSlot)
    {
        const int skyBits = static_cast<int>(min(max(static_cast<long>(pskybits), 0L), 8L));
        const int tileCount = 1 << skyBits;
        if (tileCount <= 1) tileSlot = 0;
        else tileSlot &= (tileCount - 1);

        short pic = static_cast<short>(sky.pic + pskyoff[tileSlot & (MAXPSKYTILES - 1)]);
        if ((unsigned short)pic >= MAXTILES) pic = sky.pic;
        return pic;
    }

    static double positiveMod(double v, double m)
    {
        if (m <= 0.0) return 0.0;
        v = std::fmod(v, m);
        if (v < 0.0) v += m;
        return v;
    }

    static void emitScreenSpaceSkyColumn(const SkySelection& sky, int x0, int x1,
        float u0, float u1, float v0, float v1, short pic)
    {
        if (x1 <= x0) return;
        (void)pic;

        if (sky.stat & 16)
        {
            u0 = 1.0f - u0;
            u1 = 1.0f - u1;
        }
        if (sky.stat & 32)
        {
            v0 = -v0;
            v1 = -v1;
        }

        glBegin(GL_QUADS);
        glTexCoord2f(u0, v0); glVertex2f(static_cast<float>(x0), 0.0f);
        glTexCoord2f(u1, v0); glVertex2f(static_cast<float>(x1), 0.0f);
        glTexCoord2f(u1, v1); glVertex2f(static_cast<float>(x1), static_cast<float>(max(1, g.h)));
        glTexCoord2f(u0, v1); glVertex2f(static_cast<float>(x0), static_cast<float>(max(1, g.h)));
        glEnd();
    }

    static void renderParallaxSky()
    {
        const SkySelection sky = selectParallaxSky();
        if (!sky.valid) return;

        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glDisable(GL_FOG);
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDisable(GL_ALPHA_TEST);
        glDisable(GL_BLEND);
        glEnable(GL_TEXTURE_2D);
        glColorShade(sky.shade, 1.0f);

#if BUILDGL_SKY_SCREENSPACE
        // BUILD parallax skies are effectively screen-space: each column samples
        // the sky strip from its viewing angle. A 3D cylinder produces visible UV
        // warping and gives multi-tile psky skies the wrong apparent width.
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0.0, static_cast<double>(max(1, g.w)), static_cast<double>(max(1, g.h)), 0.0, -1.0, 1.0);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        const int screenW = max(1, g.w);
        const int screenH = max(1, g.h);
        const double aspect = static_cast<double>(screenW) / static_cast<double>(screenH);
        (void)aspect;
        const double horizontalFov = clampDouble(static_cast<double>(BUILDGL_HORIZONTAL_FOV_DEGREES), 45.0, 140.0) * PI / 180.0;
        const double tanHalfFov = std::tan(horizontalFov * 0.5);
        const double yaw = radFromBuildAngle(globalang);

        const int skyBits = static_cast<int>(min(max(static_cast<long>(pskybits), 0L), 8L));
        const int tileCount = 1 << skyBits;

        short firstPic = parallaxSkyTile(sky, 0);
        bindTilePal(firstPic, static_cast<unsigned char>(sky.pal));
        const float firstTileW = static_cast<float>(max(1L, tilesizx[firstPic]));
        const float firstTileH = static_cast<float>(max(1L, tilesizy[firstPic]));

        // Horizontal panning is in texels. The sky position below is measured in
        // whole-sky tiles, so convert texels to a fractional tile offset.
        const double xPanTiles = static_cast<double>(sky.xpanning) / static_cast<double>(max(1.0f, firstTileW));

        auto skyPosForX = [&](double sx) -> double
            {
                const double ndc = (sx / static_cast<double>(screenW)) * 2.0 - 1.0;
                const double colAngle = yaw + std::atan(ndc * tanHalfFov);
                return positiveMod((colAngle / (2.0 * PI)) * static_cast<double>(tileCount) + xPanTiles,
                    static_cast<double>(tileCount));
            };

        const float virtualScaleY = BUILDGL_SKY_VERTICAL_PIXELS / static_cast<float>(screenH);
        const float yScale = clampFloat(static_cast<float>(parallaxyscale) / 65536.0f, 0.125f, 8.0f);
        const float horizonY = static_cast<float>(globalhoriz) * static_cast<float>(screenH) / BUILDGL_SKY_VERTICAL_PIXELS;
        const float yPanPix = static_cast<float>(sky.ypanning) + static_cast<float>(parallaxyoffs) / 65536.0f * firstTileH;
        const float v0 = (((0.0f - horizonY) * virtualScaleY) * yScale + firstTileH * 0.5f + yPanPix) / firstTileH;
        const float v1 = (((static_cast<float>(screenH) - horizonY) * virtualScaleY) * yScale + firstTileH * 0.5f + yPanPix) / firstTileH;

        // One-pixel columns avoid smearing across psky tile boundaries. The cost
        // is tiny for this immediate-mode renderer and matches the old column
        // renderer more closely than a curved sky cylinder.
        short boundSkyPic = -1;
        for (int x = 0; x < screenW; ++x)
        {
            const double p0 = skyPosForX(static_cast<double>(x));
            const double p1 = skyPosForX(static_cast<double>(x + 1));
            const double pm = skyPosForX(static_cast<double>(x) + 0.5);
            const int baseTile = static_cast<int>(std::floor(pm));
            const int tileSlot = (tileCount > 1) ? (baseTile & (tileCount - 1)) : 0;
            short pic = parallaxSkyTile(sky, tileSlot);
            if (pic != boundSkyPic)
            {
                bindTilePal(pic, static_cast<unsigned char>(sky.pal));
                boundSkyPic = pic;
            }

            float u0 = static_cast<float>(p0 - static_cast<double>(baseTile));
            float u1 = static_cast<float>(p1 - static_cast<double>(baseTile));

            // If this 1px strip crosses a tile seam, clamp to the seam instead
            // of letting GL_REPEAT interpolate through the wrong psky tile.
            if (u0 < 0.0f) u0 = 0.0f;
            if (u0 > 1.0f) u0 = 1.0f;
            if (u1 < 0.0f) u1 = 0.0f;
            if (u1 > 1.0f) u1 = 1.0f;

            emitScreenSpaceSkyColumn(sky, x, x + 1, u0, u1, v0, v1, pic);
        }

        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
#else
        const Vec3 eye = buildToGl(globalposx, globalposy, globalposz);
        const float radius = 7000.0f;
        const float yTop = eye.y + 3500.0f;
        const float yBot = eye.y - 3500.0f;
        const int skyBits = static_cast<int>(min(max(static_cast<long>(pskybits), 0L), 8L));
        const int tileCount = 1 << skyBits;
        const int segmentsPerTile = 4;

        const float baseVScale = clampFloat(static_cast<float>(parallaxyscale) / 65536.0f, 0.25f, 4.0f);
        const float vPan = static_cast<float>(parallaxyoffs) / 65536.0f +
            static_cast<float>(globalhoriz - 100) * 0.006f;
        const float v0 = vPan - 0.25f * baseVScale;
        const float v1 = vPan + 1.25f * baseVScale;

        for (int t = 0; t < tileCount; ++t)
        {
            short pic = parallaxSkyTile(sky, t);
            bindTilePal(pic, static_cast<unsigned char>(sky.pal));

            glBegin(GL_QUADS);
            for (int s = 0; s < segmentsPerTile; ++s)
            {
                const double f0 = static_cast<double>(t * segmentsPerTile + s) / static_cast<double>(tileCount * segmentsPerTile);
                const double f1 = static_cast<double>(t * segmentsPerTile + s + 1) / static_cast<double>(tileCount * segmentsPerTile);
                const double a0 = f0 * 2.0 * PI;
                const double a1 = f1 * 2.0 * PI;
                const float u0 = static_cast<float>(s) / static_cast<float>(segmentsPerTile);
                const float u1 = static_cast<float>(s + 1) / static_cast<float>(segmentsPerTile);

                const float x0 = eye.x + static_cast<float>(std::cos(a0)) * radius;
                const float z0 = eye.z + static_cast<float>(std::sin(a0)) * radius;
                const float x1 = eye.x + static_cast<float>(std::cos(a1)) * radius;
                const float z1 = eye.z + static_cast<float>(std::sin(a1)) * radius;

                glTexCoord2f(u0, v0); glVertex3f(x0, yTop, z0);
                glTexCoord2f(u1, v0); glVertex3f(x1, yTop, z1);
                glTexCoord2f(u1, v1); glVertex3f(x1, yBot, z1);
                glTexCoord2f(u0, v1); glVertex3f(x0, yBot, z0);
            }
            glEnd();
        }
#endif

        glDepthMask(GL_TRUE);
        glPopAttrib();
    }

    struct TessLoop
    {
        std::vector<int> walls;
        std::vector<Vec2> pts;
        double area = 0.0;
        bool hole = false;
    };

    static bool samePoint2(const Vec2& a, const Vec2& b)
    {
        return std::fabs(a.x - b.x) < 0.5 && std::fabs(a.y - b.y) < 0.5;
    }

    static bool pointInPoly2D(const Vec2& p, const std::vector<Vec2>& poly)
    {
        bool inside = false;
        const size_t n = poly.size();
        if (n < 3) return false;
        for (size_t i = 0, j = n - 1; i < n; j = i++)
        {
            const Vec2& a = poly[i];
            const Vec2& b = poly[j];
            const bool crosses = ((a.y > p.y) != (b.y > p.y));
            if (!crosses) continue;
            const double xAtY = (b.x - a.x) * (p.y - a.y) / ((b.y - a.y) + 1e-30) + a.x;
            if (p.x < xAtY) inside = !inside;
        }
        return inside;
    }

    static Vec2 loopCentroidApprox(const std::vector<Vec2>& pts)
    {
        Vec2 c{ 0.0, 0.0 };
        if (pts.empty()) return c;
        for (const Vec2& p : pts)
        {
            c.x += p.x;
            c.y += p.y;
        }
        c.x /= static_cast<double>(pts.size());
        c.y /= static_cast<double>(pts.size());
        return c;
    }

    static std::vector<TessLoop> prepareTessLoops(const std::vector<std::vector<int>>& rawLoops)
    {
        std::vector<TessLoop> out;
        out.reserve(rawLoops.size());

        for (const auto& raw : rawLoops)
        {
            TessLoop loop;
            loop.walls.reserve(raw.size());
            loop.pts.reserve(raw.size());

            for (int wi : raw)
            {
                if (wi < 0 || wi >= numwalls) continue;
                Vec2 p{ static_cast<double>(wall[wi].x), static_cast<double>(wall[wi].y) };
                if (!loop.pts.empty() && samePoint2(loop.pts.back(), p))
                    continue;
                loop.walls.push_back(wi);
                loop.pts.push_back(p);
            }

            if (loop.pts.size() >= 2 && samePoint2(loop.pts.front(), loop.pts.back()))
            {
                loop.pts.pop_back();
                loop.walls.pop_back();
            }

            // Drop exactly/near collinear vertices. GLU accepts them most of the
            // time, but the classic E1L1 bathroom has tiny degenerate corners
            // that can make one triangle disappear on some GLU builds.
            bool changed = true;
            while (changed && loop.pts.size() > 3)
            {
                changed = false;
                for (size_t i = 0; i < loop.pts.size(); ++i)
                {
                    const Vec2& a = loop.pts[(i + loop.pts.size() - 1) % loop.pts.size()];
                    const Vec2& b = loop.pts[i];
                    const Vec2& c = loop.pts[(i + 1) % loop.pts.size()];
                    const double cr = cross(a, b, c);
                    const double dot = (b.x - a.x) * (c.x - b.x) + (b.y - a.y) * (c.y - b.y);
                    if (std::fabs(cr) <= 0.01 && dot >= 0.0)
                    {
                        loop.pts.erase(loop.pts.begin() + static_cast<std::ptrdiff_t>(i));
                        loop.walls.erase(loop.walls.begin() + static_cast<std::ptrdiff_t>(i));
                        changed = true;
                        break;
                    }
                }
            }

            if (loop.pts.size() < 3) continue;
            loop.area = area2(loop.pts);
            if (std::fabs(loop.area) < 1.0) continue;
            out.push_back(std::move(loop));
        }

        for (size_t i = 0; i < out.size(); ++i)
        {
            const Vec2 c = loopCentroidApprox(out[i].pts);
            int depth = 0;
            for (size_t j = 0; j < out.size(); ++j)
            {
                if (i == j) continue;
                if (std::fabs(out[j].area) <= std::fabs(out[i].area)) continue;
                if (pointInPoly2D(c, out[j].pts)) ++depth;
            }

            out[i].hole = (depth & 1) != 0;
            const bool wantsPositiveWinding = !out[i].hole;
            const bool hasPositiveWinding = out[i].area > 0.0;
            if (wantsPositiveWinding != hasPositiveWinding)
            {
                std::reverse(out[i].pts.begin(), out[i].pts.end());
                std::reverse(out[i].walls.begin(), out[i].walls.end());
                out[i].area = -out[i].area;
            }
        }

        return out;
    }

    static void renderSectorSurface(short sectnum, bool ceiling)
    {
        if (sectnum < 0 || sectnum >= numsectors) return;
        sectortype& sec = sector[sectnum];
        const short stat = ceiling ? sec.ceilingstat : sec.floorstat;
        if (stat & 1) return; // Parallaxed sky is drawn once as an infinite backdrop.
        applySectorFog(sectnum);

        short pic = ceiling ? sec.ceilingpicnum : sec.floorpicnum;
        if ((unsigned short)pic >= MAXTILES) pic = 0;
        if (picanm[pic] & 192) pic = static_cast<short>(pic + animateoffs(pic, sectnum));
        if ((unsigned short)pic >= MAXTILES) pic = 0;

        const unsigned char palnum = static_cast<unsigned char>(ceiling ? sec.ceilingpal : sec.floorpal);
        bindTilePal(pic, palnum);
        glDisable(GL_ALPHA_TEST);
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glColorShade((ceiling ? sec.ceilingshade : sec.floorshade));

        auto loops = prepareTessLoops(collectLoops(sectnum));
        if (loops.empty()) return;

        if (loops.size() == 1 && !loops[0].hole)
        {
            // No holes are present, so avoid GLU's occasional dropped triangle
            // behavior on tiny concave/near-collinear BUILD sectors. Ear
            // clipping is deterministic for the common single-loop case; GLU is
            // kept below for true nested-loop sectors where holes matter.
            const std::vector<Vec2>& pts = loops[0].pts;
            std::vector<int> tris = triangulate(pts);
            if (tris.empty())
            {
                for (int i = 1; i + 1 < static_cast<int>(pts.size()); ++i)
                {
                    tris.push_back(0);
                    tris.push_back(i);
                    tris.push_back(i + 1);
                }
            }

            glBegin(GL_TRIANGLES);
            for (size_t i = 0; i + 2 < tris.size(); i += 3)
            {
                int order[3] = { tris[i], tris[i + (ceiling ? 2 : 1)], tris[i + (ceiling ? 1 : 2)] };
                for (int k = 0; k < 3; ++k)
                {
                    int pi = order[k];
                    long x = static_cast<long>(std::llround(pts[pi].x));
                    long y = static_cast<long>(std::llround(pts[pi].y));
                    texFloor(pic, sec, ceiling, x, y);
                    vertexBuild(x, y, slopeZ(sectnum, x, y, ceiling));
                }
            }
            glEnd();
            return;
        }

        GLUtesselator* tess = gluNewTess();
        if (!tess)
        {
            // Conservative fallback: keep the old per-loop ear clipping if GLU
            // tessellation is not available. The normal path below is what fixes
            // nested-loop sectors/holes such as trash bins and counters.
            for (const auto& loop : loops)
            {
                if (loop.hole) continue;
                const std::vector<Vec2>& pts = loop.pts;
                std::vector<int> tris = triangulate(pts);
                if (tris.empty())
                {
                    for (int i = 1; i + 1 < static_cast<int>(pts.size()); ++i)
                    {
                        tris.push_back(0); tris.push_back(i); tris.push_back(i + 1);
                    }
                }

                glBegin(GL_TRIANGLES);
                for (size_t i = 0; i + 2 < tris.size(); i += 3)
                {
                    int order[3] = { tris[i], tris[i + (ceiling ? 2 : 1)], tris[i + (ceiling ? 1 : 2)] };
                    for (int k = 0; k < 3; ++k)
                    {
                        int pi = order[k];
                        long x = static_cast<long>(std::llround(pts[pi].x));
                        long y = static_cast<long>(std::llround(pts[pi].y));
                        texFloor(pic, sec, ceiling, x, y);
                        vertexBuild(x, y, slopeZ(sectnum, x, y, ceiling));
                    }
                }
                glEnd();
            }
            return;
        }

        FlatTessContext ctx;
        ctx.owned.reserve(256);
        g_flatTessContext = &ctx;

        gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD);
        // Project tessellation onto the map footprint (X/Z in GL space). BUILD
        // slopes are still represented in vertex Y after tessellation.
        gluTessNormal(tess, 0.0, 1.0, 0.0);
        gluTessCallback(tess, GLU_TESS_BEGIN, reinterpret_cast<void (CALLBACK*)()>(flatTessBeginCB));
        gluTessCallback(tess, GLU_TESS_VERTEX, reinterpret_cast<void (CALLBACK*)()>(flatTessVertexCB));
        gluTessCallback(tess, GLU_TESS_END, reinterpret_cast<void (CALLBACK*)()>(flatTessEndCB));
        gluTessCallback(tess, GLU_TESS_COMBINE, reinterpret_cast<void (CALLBACK*)()>(flatTessCombineCB));

        gluTessBeginPolygon(tess, nullptr);
        for (const auto& loop : loops)
        {
            if (loop.walls.size() < 3) continue;
            gluTessBeginContour(tess);
            for (int wi : loop.walls)
            {
                if (wi < 0 || wi >= numwalls) continue;
                const long x = wall[wi].x;
                const long y = wall[wi].y;
                const long z = slopeZ(sectnum, x, y, ceiling);
                const Vec3 gv = buildToGl(x, y, z);

                std::unique_ptr<FlatTessVertex> tv(new FlatTessVertex());
                tv->pos[0] = static_cast<GLdouble>(gv.x);
                tv->pos[1] = static_cast<GLdouble>(gv.y);
                tv->pos[2] = static_cast<GLdouble>(gv.z);
                calcFloorTex(pic, sec, ceiling, x, y, tv->uv[0], tv->uv[1]);

                FlatTessVertex* raw = tv.get();
                ctx.owned.push_back(std::move(tv));
                gluTessVertex(tess, raw->pos, raw);
            }
            gluTessEndContour(tess);
        }
        gluTessEndPolygon(tess);

        g_flatTessContext = nullptr;
        gluDeleteTess(tess);
    }

    static void texWall(short tilenum, float uPix, float vPix)
    {
        const float tw = static_cast<float>(max(1L, tilesizx[tilenum]));
        const float th = static_cast<float>(max(1L, tilesizy[tilenum]));
        glTexCoord2f(uPix / tw, vPix / th);
    }

    static void drawWallSpan(const walltype& w, long x1, long y1, long x2, long y2,
        long zTop1, long zTop2, long zBot1, long zBot2,
        short pic, unsigned char palnum, long shade, bool transparent,
        long refTop1, long refTop2, long refBot1, long refBot2)
    {
        if (zBot1 <= zTop1 && zBot2 <= zTop2) return;

        // For masked walls, an invalid overpicnum means "nothing to draw". Do
        // not silently draw tile 0, because that makes failed masks look like
        // random opaque quads.
        if ((unsigned short)pic >= MAXTILES)
        {
            if (transparent) return;
            pic = 0;
        }

        if (picanm[pic] & 192)
            pic = static_cast<short>(pic + animateoffs(pic, static_cast<short>(&w - wall + 16384)));

        if ((unsigned short)pic >= MAXTILES)
        {
            if (transparent) return;
            pic = 0;
        }

        if (!bindTilePal(pic, palnum, !transparent)) return;

        // cstat 16 is a true masked/cutout wall: palette index 255 is
        // transparent. cstat 32 by itself is a one-way wall that uses
        // overpicnum too, but it is solid: index 255 should draw black.
        const bool maskedPass = transparent && ((w.cstat & 16) != 0);

        // BUILD forcefields are usually masked portal overlays: cstat 16 for
        // masked pixels plus cstat 32 for one-way/portal behavior, often without
        // cstat 128. Treat that case as translucent for GL state purposes so it
        // does not write depth over the floor/walls behind the portal.
        const bool forcefieldLikePortal = maskedPass && ((w.cstat & 32) != 0);
        const bool translucent = maskedPass && (((w.cstat & 128) != 0) || forcefieldLikePortal);

        if (maskedPass)
        {
            glEnable(GL_ALPHA_TEST);
            glAlphaFunc(GL_GREATER, 0.05f);
        }
        else
        {
            // Palette index 255 is only transparent for masked walls/sprites.
            // On normal walls/floors it should render as black, so do not leave
            // alpha testing enabled from a previous masked draw.
            glDisable(GL_ALPHA_TEST);
        }

        if (translucent)
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            glColorShade(shade, (w.cstat & 512) ? 0.33f : 0.66f);
        }
        else
        {
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
            glColorShade(shade, 1.0f);
        }

        const float xrep = static_cast<float>(std::max<int>(1, w.xrepeat));
        const float yrep = static_cast<float>(std::max<int>(1, w.yrepeat));

        // BUILD's wall U coordinate is driven by xrepeat<<3 across the wall.
        // xpanning is already in U texels for walls, but ypanning is a 0..255
        // fraction of the tile's power-of-two height, like floor/ceiling pan.
        const float xPan = static_cast<float>(w.xpanning);
        const float yPan = panningToTexels(pic, static_cast<unsigned char>(w.ypanning), true);
        float u0 = xPan;
        float u1 = u0 + xrep * 8.0f;
        if (w.cstat & 8) std::swap(u0, u1);

        const float vScale = yrep / 2048.0f;
        float vTop1 = 0.0f, vTop2 = 0.0f, vBot1 = 0.0f, vBot2 = 0.0f;

        if (w.cstat & 4)
        {
            // Bottom-aligned walls stay pinned to the full wall bottom. This is
            // especially important for masked portal walls: only the visible
            // portal opening should be clipped, not stretched.
            vTop1 = yPan - static_cast<float>(refBot1 - zTop1) * vScale;
            vTop2 = yPan - static_cast<float>(refBot2 - zTop2) * vScale;
            vBot1 = yPan - static_cast<float>(refBot1 - zBot1) * vScale;
            vBot2 = yPan - static_cast<float>(refBot2 - zBot2) * vScale;
        }
        else
        {
            vTop1 = yPan + static_cast<float>(zTop1 - refTop1) * vScale;
            vTop2 = yPan + static_cast<float>(zTop2 - refTop2) * vScale;
            vBot1 = yPan + static_cast<float>(zBot1 - refTop1) * vScale;
            vBot2 = yPan + static_cast<float>(zBot2 - refTop2) * vScale;
        }

        if (w.cstat & 256)
        {
            std::swap(vTop1, vBot1);
            std::swap(vTop2, vBot2);
        }

        glBegin(GL_QUADS);
        texWall(pic, u0, vTop1); vertexBuild(x1, y1, zTop1);
        texWall(pic, u1, vTop2); vertexBuild(x2, y2, zTop2);
        texWall(pic, u1, vBot2); vertexBuild(x2, y2, zBot2);
        texWall(pic, u0, vBot1); vertexBuild(x1, y1, zBot1);
        glEnd();

        if (translucent)
        {
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }
    }

    static bool haveScannedSectors()
    {
        const int nbytes = (numsectors + 7) >> 3;
        for (int i = 0; i < nbytes; ++i)
            if (gotsector[i] != 0)
                return true;
        return false;
    }

    static bool sectorScanned(short sectnum)
    {
        if (sectnum < 0 || sectnum >= numsectors)
            return false;

        // Direct helper entry points such as ceilscan()/florscan() can be called
        // outside drawrooms(). In that case there is no scan mask yet, so keep
        // the older permissive behavior instead of drawing nothing.
        if (!haveScannedSectors())
            return true;

        return (gotsector[sectnum >> 3] & pow2char[sectnum & 7]) != 0;
    }

    static std::int64_t wallSideCross(int wallnum, long x, long y)
    {
        if (wallnum < 0 || wallnum >= numwalls)
            return 0;

        const walltype& w = wall[wallnum];
        if (w.point2 < 0 || w.point2 >= numwalls)
            return 0;

        const walltype& w2 = wall[w.point2];
        const std::int64_t dx = static_cast<std::int64_t>(w2.x) - static_cast<std::int64_t>(w.x);
        const std::int64_t dy = static_cast<std::int64_t>(w2.y) - static_cast<std::int64_t>(w.y);
        return dx * static_cast<std::int64_t>(y - w.y) -
            dy * static_cast<std::int64_t>(x - w.x);
    }

    static bool cameraOnWallSectorSide(int wallnum, short wallsect)
    {
        if (wallnum < 0 || wallnum >= numwalls)
            return false;

        const walltype& w = wall[wallnum];

        // Prefer sector membership when available. It removes the ambiguity at
        // exact vertices/edges and picks the correct texture side for normal
        // two-sided masked walls.
        if (wallsect >= 0 && wallsect < numsectors)
        {
            if (globalcursectnum == wallsect) return true;
            if (inside(globalposx, globalposy, wallsect) == 1) return true;
        }

        if (w.nextsector >= 0 && w.nextsector < numsectors)
        {
            if (globalcursectnum == w.nextsector) return false;
            if (inside(globalposx, globalposy, w.nextsector) == 1) return false;
        }

        // Fall back to a full 64-bit side test. dmulscale32() throws away too
        // much precision for short/near walls and randomly flips one-way masks.
        return wallSideCross(wallnum, globalposx, globalposy) >= 0;
    }

    static bool wallFacesCamera(int wallnum)
    {
        return wallSideCross(wallnum, globalposx, globalposy) >= 0;
    }

    static bool hasParallaxCeiling(short sectnum)
    {
        return sectnum >= 0 && sectnum < numsectors && (sector[sectnum].ceilingstat & 1) != 0;
    }

    static bool hasParallaxFloor(short sectnum)
    {
        return sectnum >= 0 && sectnum < numsectors && (sector[sectnum].floorstat & 1) != 0;
    }

    static short ownerSectorOfWall(int wallnum)
    {
        if (wallnum < 0 || wallnum >= numwalls) return -1;
        for (short si = 0; si < numsectors; ++si)
        {
            const int start = sector[si].wallptr;
            const int end = start + sector[si].wallnum;
            if (wallnum >= start && wallnum < end)
                return si;
        }
        return -1;
    }

    static bool validMaskedPic(short pic)
    {
        return tileHasArt(pic);
    }

#ifndef BUILDGL_CULL_ONE_WAY_MASKED_BACKFACES
#define BUILDGL_CULL_ONE_WAY_MASKED_BACKFACES 0
#endif

    static void renderSingleMaskedWall(int drawWi, short drawSect)
    {
        if (drawWi < 0 || drawWi >= numwalls) return;
        if (drawSect < 0 || drawSect >= numsectors) drawSect = ownerSectorOfWall(drawWi);
        if (drawSect < 0 || drawSect >= numsectors) return;
        if (!sectorScanned(drawSect)) return;

        walltype& mw = wall[drawWi];
        if ((mw.cstat & 48) == 0) return;
        if (mw.point2 < 0 || mw.point2 >= numwalls) return;
        if (mw.nextsector < 0 || mw.nextsector >= numsectors) return;

        // Bit 32 by itself is a one-way overpicnum wall. Keep backface culling
        // configurable; default to drawing it because missing one-way/masked
        // walls are more damaging in this GL shim than an occasional visible
        // backside.
#if BUILDGL_CULL_ONE_WAY_MASKED_BACKFACES
        if ((mw.cstat & 32) && !cameraOnWallSectorSide(drawWi, drawSect))
            return;
#endif

        walltype& mw2 = wall[mw.point2];
        const short nextSect = mw.nextsector;

        const long c1 = slopeZ(drawSect, mw.x, mw.y, true);
        const long c2 = slopeZ(drawSect, mw2.x, mw2.y, true);
        const long f1 = slopeZ(drawSect, mw.x, mw.y, false);
        const long f2 = slopeZ(drawSect, mw2.x, mw2.y, false);
        const long nc1 = slopeZ(nextSect, mw.x, mw.y, true);
        const long nc2 = slopeZ(nextSect, mw2.x, mw2.y, true);
        const long nf1 = slopeZ(nextSect, mw.x, mw.y, false);
        const long nf2 = slopeZ(nextSect, mw2.x, mw2.y, false);

        const long top1 = max(c1, nc1);
        const long top2 = max(c2, nc2);
        const long bot1 = min(f1, nf1);
        const long bot2 = min(f2, nf2);
        if (bot1 <= top1 && bot2 <= top2) return;

        const walltype* maskTexWall = &mw;
        short maskPic = mw.overpicnum;
        unsigned char maskPal = static_cast<unsigned char>(mw.pal);

        short pairedCstat = mw.cstat;

        if (mw.nextwall >= 0 && mw.nextwall < numwalls)
        {
            const walltype& other = wall[mw.nextwall];
            pairedCstat |= other.cstat;

            if ((other.cstat & 48) && validMaskedPic(other.overpicnum) &&
                (!validMaskedPic(maskPic) || (maskPic == 0 && other.overpicnum != 0)))
            {
                maskTexWall = &other;
                maskPic = other.overpicnum;
                maskPal = static_cast<unsigned char>(other.pal);
            }
        }

        if (!validMaskedPic(maskPic))
            return;

        walltype maskDrawWall = *maskTexWall;
        maskDrawWall.cstat = static_cast<short>(maskDrawWall.cstat | (pairedCstat & (16 | 32 | 128 | 512)));

        applySectorFog(drawSect);
        const long maskShade = static_cast<long>(sector[drawSect].floorshade) + static_cast<long>(maskTexWall->shade);

        drawWallSpan(maskDrawWall, mw.x, mw.y, mw2.x, mw2.y,
            top1, top2, bot1, bot2,
            maskPic, maskPal, maskShade, true,
            c1, c2, f1, f2);
    }

    static void renderMaskedWalls()
    {
        std::vector<unsigned char> done(static_cast<size_t>(max<long>(numwalls, 0L)), 0);

        auto considerWall = [&](int wi)
            {
                if (wi < 0 || wi >= numwalls) return;
                if (!done.empty() && done[static_cast<size_t>(wi)]) return;

                walltype& w = wall[wi];
                if ((w.cstat & 48) == 0) return;
                if (w.nextsector < 0 || w.nextsector >= numsectors) return;

                short sect = ownerSectorOfWall(wi);
                if (sect < 0 || sect >= numsectors || !sectorScanned(sect)) return;

                int drawWi = wi;
                short drawSect = sect;

                const bool pairedMasked = (w.nextwall >= 0 && w.nextwall < numwalls &&
                    (wall[w.nextwall].cstat & 48) != 0);

                if (pairedMasked)
                {
                    if (!done.empty())
                    {
                        done[static_cast<size_t>(wi)] = 1;
                        done[static_cast<size_t>(w.nextwall)] = 1;
                    }

                    const short otherSect = ownerSectorOfWall(w.nextwall);
                    const bool thisSide = cameraOnWallSectorSide(wi, sect);
                    const bool otherSide = cameraOnWallSectorSide(w.nextwall, otherSect);

                    if (!thisSide && otherSide)
                    {
                        drawWi = w.nextwall;
                        drawSect = otherSect;
                    }
                    else if (thisSide == otherSide && w.nextwall < wi)
                    {
                        // Degenerate/on-line camera case: draw one copy only.
                        drawWi = w.nextwall;
                        drawSect = otherSect;
                    }
                }
                else if (!done.empty())
                {
                    done[static_cast<size_t>(wi)] = 1;
                }

                renderSingleMaskedWall(drawWi, drawSect);
            };

        if (maskwallcnt > 0)
        {
            for (long i = 0; i < maskwallcnt; ++i)
                considerWall(maskwall[i]);
        }
        else
        {
            // Compatibility for direct maskwallscan()/drawmaskwall() calls made
            // outside drawrooms(), before scansector() populated maskwall[].
            for (short si = 0; si < numsectors; ++si)
            {
                if (!sectorScanned(si)) continue;
                const int start = sector[si].wallptr;
                const int end = start + sector[si].wallnum;
                for (int wi = start; wi < end && wi < numwalls; ++wi)
                    considerWall(wi);
            }
        }
    }

    static void renderWalls(bool transparent)
    {
        if (transparent)
        {
            renderMaskedWalls();
            return;
        }


        for (short si = 0; si < numsectors; ++si)
        {
            if (!sectorScanned(si))
                continue;

            sectortype& sec = sector[si];
            int start = sec.wallptr;
            int end = start + sec.wallnum;
            if (start < 0 || end > numwalls) continue;

            applySectorFog(si);

            for (int wi = start; wi < end; ++wi)
            {
                walltype& w = wall[wi];
                if (w.point2 < 0 || w.point2 >= numwalls) continue;
                walltype& w2 = wall[w.point2];

                const bool hasNext = (w.nextsector >= 0 && w.nextsector < numsectors);
                const long c1 = slopeZ(si, w.x, w.y, true);
                const long c2 = slopeZ(si, w2.x, w2.y, true);
                const long f1 = slopeZ(si, w.x, w.y, false);
                const long f2 = slopeZ(si, w2.x, w2.y, false);
                const long shade = static_cast<long>(sec.floorshade) + static_cast<long>(w.shade);

                // A wall with no nextsector is a true solid wall. Red-line
                // cstat-32 one-way walls use overpicnum and are drawn in the
                // masked/overlay pass, not as normal wall.picnum geometry here.
                if (!hasNext)
                {
                    drawWallSpan(w, w.x, w.y, w2.x, w2.y, c1, c2, f1, f2, w.picnum, static_cast<unsigned char>(w.pal), shade, false, c1, c2, f1, f2);
                    continue;
                }

                short n = w.nextsector;
                long nc1 = slopeZ(n, w.x, w.y, true);
                long nc2 = slopeZ(n, w2.x, w2.y, true);
                long nf1 = slopeZ(n, w.x, w.y, false);
                long nf2 = slopeZ(n, w2.x, w2.y, false);

                // BUILD z grows down. Draw only the visible upper/lower portal
                // bands between this sector and the neighboring sector. When
                // both adjacent sectors have parallaxed skies, classic Duke3D
                // does not draw the vertical divider between those sky planes;
                // the sky is allowed to continue through the portal.
                const bool skyCeilingPortal = hasParallaxCeiling(si) && hasParallaxCeiling(n);
                const bool skyFloorPortal = hasParallaxFloor(si) && hasParallaxFloor(n);

                if (!skyCeilingPortal && (nc1 > c1 || nc2 > c2))
                    drawWallSpan(w, w.x, w.y, w2.x, w2.y, c1, c2, nc1, nc2, w.picnum, static_cast<unsigned char>(w.pal), shade, false, c1, c2, f1, f2);

                if (!skyFloorPortal && (nf1 < f1 || nf2 < f2))
                {
                    walltype lowerMapWall = w;
                    const walltype* lowerPicWall = &w;
                    short bottomPic = w.picnum;
                    unsigned char bottomPal = static_cast<unsigned char>(w.pal);
                    long bottomShade = shade;

                    if ((w.cstat & 2) && w.nextwall >= 0 && w.nextwall < numwalls)
                    {
                        // Bottom-swap uses the neighbor side's picture/palette,
                        // y-panning, bottom-align, and y-flip, but keeps this
                        // side's xrepeat/yrepeat and x-flip mapping. Passing
                        // nextwall wholesale made only bottom-swapped bands use
                        // the wrong repeat/orientation, which is why UVs looked
                        // fine most of the time and then broke on specific
                        // portal lower walls.
                        lowerPicWall = &wall[w.nextwall];
                        bottomPic = lowerPicWall->picnum;
                        bottomPal = static_cast<unsigned char>(lowerPicWall->pal);
                        bottomShade = static_cast<long>(sec.floorshade) + static_cast<long>(lowerPicWall->shade);
                        lowerMapWall.xpanning = lowerPicWall->xpanning;
                        lowerMapWall.ypanning = lowerPicWall->ypanning;
                        lowerMapWall.cstat = static_cast<short>((w.cstat & 8) | (lowerPicWall->cstat & (4 | 256)));
                    }

                    drawWallSpan(lowerMapWall, w.x, w.y, w2.x, w2.y, nf1, nf2, f1, f2,
                        bottomPic, bottomPal, bottomShade, false, c1, c2, f1, f2);
                }
            }
        }
    }

    static void drawSpriteQuad(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, bool xflip, bool yflip)
    {
        const float u0 = xflip ? 1.0f : 0.0f;
        const float u1 = xflip ? 0.0f : 1.0f;
        const float v0 = yflip ? 1.0f : 0.0f;
        const float v1 = yflip ? 0.0f : 1.0f;
        glBegin(GL_QUADS);
        glTexCoord2f(u0, v0); glVertex3f(a.x, a.y, a.z);
        glTexCoord2f(u1, v0); glVertex3f(b.x, b.y, b.z);
        glTexCoord2f(u1, v1); glVertex3f(c.x, c.y, c.z);
        glTexCoord2f(u0, v1); glVertex3f(d.x, d.y, d.z);
        glEnd();
    }

#ifndef BUILDGL_FLOOR_SPRITE_DEPTH_BIAS
    // Floor-aligned sprites are often placed exactly on a floor/ceiling plane. A
    // tiny upward bias prevents coplanar z fighting and keeps effects such as fire
    // decals from disappearing into the surface while remaining visually invisible.
#define BUILDGL_FLOOR_SPRITE_DEPTH_BIAS 0.0015f
#endif

    static void renderSprites()
    {
        struct Item
        {
            spritetype s;
            int listIndex = 0;
            double dist2 = 0.0;
        };

        std::vector<Item> items;
        items.reserve(MAXSPRITESONSCREEN);

        const bool frameHasScanMask = haveScannedSectors();
        const bool useTspriteList = frameHasScanMask || (spritesortcnt > 0);

        auto addSprite = [&](const spritetype& s, int listIndex)
            {
                if (s.sectnum < 0 || s.sectnum >= numsectors) return;
                if (!sectorScanned(s.sectnum)) return;
                if (s.xrepeat == 0 || s.yrepeat == 0) return;
                if ((s.cstat & 0x8000) && !showinvisibility) return;

                const double dx = static_cast<double>(s.x - globalposx);
                const double dy = static_cast<double>(s.y - globalposy);
                items.push_back(Item{ s, listIndex, dx * dx + dy * dy });
            };

        if (useTspriteList)
        {
            const long n = min<long>(spritesortcnt, MAXSPRITESONSCREEN);
            for (long i = 0; i < n; ++i)
                addSprite(tsprite[i], static_cast<int>(i));
        }
        else
        {
            // Direct compatibility entry points can call drawsprite()/drawmasks()
            // without scansector() having populated tsprite[]. In that case only,
            // fall back to raw map sprites.
            for (int i = 0; i < MAXSPRITES; ++i)
            {
                if (sprite[i].sectnum < 0 || sprite[i].sectnum >= numsectors) continue;
                spritetype raw = sprite[i];
                raw.owner = static_cast<short>(i);
                addSprite(raw, i);
            }
        }

        std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.dist2 > b.dist2; });

        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.05f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        for (const Item& it : items)
        {
            const spritetype& s = it.s;
            short pic = s.picnum;
            if ((unsigned short)pic >= MAXTILES) pic = 0;
            const short ownerForAnim = (s.owner >= 0) ? s.owner : static_cast<short>(it.listIndex);
            if (picanm[pic] & 192)
                pic = static_cast<short>(pic + animateoffs(pic, static_cast<short>(ownerForAnim + 32768)));
            if ((unsigned short)pic >= MAXTILES) pic = 0;
            if (!bindTilePal(pic, static_cast<unsigned char>(s.pal), false))
                continue;

            const float tw = static_cast<float>(max(1L, tilesizx[pic]));
            const float th = static_cast<float>(max(1L, tilesizy[pic]));
            const float width = tw * static_cast<float>(s.xrepeat) / 4096.0f;
            const float height = th * static_cast<float>(s.yrepeat) / 4096.0f;

            const bool xflip = (s.cstat & 4) != 0;
            const bool yflip = (s.cstat & 8) != 0;
            const float xoff = (static_cast<float>(s.xoffset) + static_cast<float>(signedPicanmByte(pic, 8))) *
                static_cast<float>(s.xrepeat) / 4096.0f * (xflip ? -1.0f : 1.0f);
            const float yoff = (static_cast<float>(s.yoffset) + static_cast<float>(signedPicanmByte(pic, 16))) *
                static_cast<float>(s.yrepeat) / 4096.0f * (yflip ? -1.0f : 1.0f);

            const bool trans = (s.cstat & 2) != 0;
            applySectorFog(s.sectnum);
            glColorShade(s.shade, trans ? 0.55f : 1.0f);

            Vec3 c = buildToGl(s.x, s.y, s.z);
            int mode = s.cstat & 48;

            if (mode == 0)
            {
                const double a = radFromBuildAngle(globalang);
                const Vec3 right{ static_cast<float>(-std::sin(a)), 0.0f, static_cast<float>(std::cos(a)) };
                if ((s.cstat & 128) == 0) c.y += height * 0.5f;
                c.x += right.x * xoff;
                c.z += right.z * xoff;
                // BUILD yoffset/picanm y-center is subtracted from the sprite's
                // Z in map space. Since GL Y is -Z, the visual offset must be
                // added here. The old sign pushed positive-offset effects down
                // into floors, which is why some fire sprites looked buried or
                // disappeared.
                c.y += yoff;

                const float hw = width * 0.5f, hh = height * 0.5f;
                Vec3 p0{ c.x - right.x * hw, c.y + hh, c.z - right.z * hw };
                Vec3 p1{ c.x + right.x * hw, c.y + hh, c.z + right.z * hw };
                Vec3 p2{ c.x + right.x * hw, c.y - hh, c.z + right.z * hw };
                Vec3 p3{ c.x - right.x * hw, c.y - hh, c.z - right.z * hw };
                drawSpriteQuad(p0, p1, p2, p3, xflip, yflip);
            }
            else if (mode == 16)
            {
                const double a = radFromBuildAngle(s.ang);
                // BUILD wall sprites use the angle as the facing/normal; the
                // textured span is perpendicular to that normal. The previous
                // GL path used the normal as the horizontal span, rotating wall
                // sprites 90 degrees in maps.
                const Vec3 right{ static_cast<float>(-std::sin(a)), 0.0f, static_cast<float>(std::cos(a)) };
                if ((s.cstat & 128) == 0) c.y += height * 0.5f;
                c.x += right.x * xoff;
                c.z += right.z * xoff;
                // BUILD yoffset/picanm y-center is subtracted from the sprite's
                // Z in map space. Since GL Y is -Z, the visual offset must be
                // added here. The old sign pushed positive-offset effects down
                // into floors, which is why some fire sprites looked buried or
                // disappeared.
                c.y += yoff;

                const float hw = width * 0.5f, hh = height * 0.5f;
                Vec3 p0{ c.x - right.x * hw, c.y + hh, c.z - right.z * hw };
                Vec3 p1{ c.x + right.x * hw, c.y + hh, c.z + right.z * hw };
                Vec3 p2{ c.x + right.x * hw, c.y - hh, c.z + right.z * hw };
                Vec3 p3{ c.x - right.x * hw, c.y - hh, c.z - right.z * hw };
                drawSpriteQuad(p0, p1, p2, p3, xflip, yflip);
            }
            else if (mode == 32)
            {
                const double a = radFromBuildAngle(s.ang);
                const Vec3 right{ static_cast<float>(std::cos(a)), 0.0f, static_cast<float>(std::sin(a)) };
                const Vec3 forward{ static_cast<float>(-std::sin(a)), 0.0f, static_cast<float>(std::cos(a)) };
                c.x += right.x * xoff + forward.x * yoff;
                c.z += right.z * xoff + forward.z * yoff;
                c.y += BUILDGL_FLOOR_SPRITE_DEPTH_BIAS;

                const float hw = width * 0.5f, hd = height * 0.5f;
                Vec3 p0{ c.x - right.x * hw - forward.x * hd, c.y, c.z - right.z * hw - forward.z * hd };
                Vec3 p1{ c.x + right.x * hw - forward.x * hd, c.y, c.z + right.z * hw - forward.z * hd };
                Vec3 p2{ c.x + right.x * hw + forward.x * hd, c.y, c.z + right.z * hw + forward.z * hd };
                Vec3 p3{ c.x - right.x * hw + forward.x * hd, c.y, c.z - right.z * hw + forward.z * hd };
                drawSpriteQuad(p0, p1, p2, p3, xflip, yflip);
            }
        }

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    static void begin3D()
    {
        if (!g.ready) createWindow(static_cast<int>(xdim), static_cast<int>(ydim), "Duke Nukem 3D");
        pump();
        wglMakeCurrent(g.hdc, g.hglrc);

        if (g.w <= 0 || g.h <= 0)
            syncClientSize(static_cast<int>(max(1L, xdim)), static_cast<int>(max(1L, ydim)), true);

        int vx = static_cast<int>(windowx1);
        int vy = static_cast<int>(g.h - (windowy1 + max(1L, windowy2 - windowy1 + 1)));
        int vw = static_cast<int>(max(1L, windowx2 - windowx1 + 1));
        int vh = static_cast<int>(max(1L, windowy2 - windowy1 + 1));


        glViewport(static_cast<GLint>(vx), static_cast<GLint>(vy), vw, vh);
        glClearColor(0.02f, 0.025f, 0.035f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        const double aspect = static_cast<double>(vw) / static_cast<double>(max(1, vh));
        const double verticalFov = computeVerticalFovDegrees(aspect);
        const double glNearPlane = max(0.0005, static_cast<double>(BUILDGL_CLOSE_MAP_UNITS) * static_cast<double>(XY_SCALE));
        gluPerspective(verticalFov, aspect, glNearPlane, static_cast<double>(BUILDGL_FAR_PLANE));

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        Vec3 eye = buildToGl(globalposx, globalposy, globalposz);
        double yaw = radFromBuildAngle(globalang);
        double pitch = horizToPitchSlope(globalhoriz, verticalFov);
        Vec3 dir{ static_cast<float>(std::cos(yaw)), static_cast<float>(pitch), static_cast<float>(std::sin(yaw)) };
        gluLookAt(eye.x, eye.y, eye.z,
            eye.x + dir.x, eye.y + dir.y, eye.z + dir.z,
            0.0, 1.0, 0.0);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);
        glDisable(GL_ALPHA_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_FOG);
        glDepthMask(GL_TRUE);
        g.frameActive = true;
        g.opaqueDrawn = false;
        g.maskedDrawn = false;
    }

    static void renderOpaque()
    {
        if (!g.frameActive || g.opaqueDrawn) return;

        renderParallaxSky();

        for (short si = 0; si < numsectors; ++si)
            if (sectorScanned(si))
                renderSectorSurface(si, true);

        for (short si = 0; si < numsectors; ++si)
            if (sectorScanned(si))
                renderSectorSurface(si, false);

        renderWalls(false);
        g.opaqueDrawn = true;
    }

    static void renderMasks()
    {
        if (!g.frameActive || g.maskedDrawn) return;
        renderWalls(true);
        renderSprites();
        g.maskedDrawn = true;
    }

    static void begin2D()
    {
        if (!g.ready) createWindow(static_cast<int>(xdim), static_cast<int>(ydim), "Duke Nukem 3D");
        wglMakeCurrent(g.hdc, g.hglrc);

        glPushAttrib(GL_VIEWPORT_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT |
            GL_COLOR_BUFFER_BIT | GL_SCISSOR_BIT);
        glViewport(0, 0, max(1, g.w), max(1, g.h));

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0.0, static_cast<double>(g.w), static_cast<double>(g.h), 0.0, -1.0, 1.0);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_ALPHA_TEST);
    }
    static void end2D()
    {
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();

        glPopAttrib();
    }

    static int nextPowerOfTwo(int v)
    {
        int p = 1;
        v = max(1, v);
        while (p < v && p < 16384) p <<= 1;
        return p;
    }

    static int fpOffTextureWidth()
    {
#if BUILDGL_FP_OFF_POT_BACKING
        return nextPowerOfTwo(FP_OFF_WIDTH);
#else
        return FP_OFF_WIDTH;
#endif
    }

    static int fpOffTextureHeight()
    {
#if BUILDGL_FP_OFF_POT_BACKING
        return nextPowerOfTwo(FP_OFF_HEIGHT);
#else
        return FP_OFF_HEIGHT;
#endif
    }

    static unsigned char* ensureFpOffBuffer()
    {
        const size_t bytes = static_cast<size_t>(FP_OFF_WIDTH) * static_cast<size_t>(FP_OFF_HEIGHT);

        unsigned char* oldOwned = g.fpOffBuffer.empty() ? nullptr : g.fpOffBuffer.data();
        if (g.fpOffBuffer.size() != bytes)
            g.fpOffBuffer.assign(bytes, 0);

        if (!g.fpOffSource || g.fpOffSource == oldOwned)
            g.fpOffSource = g.fpOffBuffer.data();

        return g.fpOffBuffer.data();
    }

    static long fpOffCompatibility(void* source)
    {
        unsigned char* owned = ensureFpOffBuffer();
        g.fpOffDirtyThisFrame = true;

        // Keep normal FP_OFF(ptr) semantics for callers that already own a
        // 320x200 Duke-style framebuffer, while FP_OFF(NULL) returns this
        // port's owned 320x200 compatibility framebuffer. The upload pass
        // samples FP_OFF_WIDTH bytes per row.
        if (source)
        {
            g.fpOffSource = static_cast<unsigned char*>(source);
            return BUILDGL_PTR_TO_LONG(source);
        }

        g.fpOffSource = owned;
        return BUILDGL_PTR_TO_LONG(owned);
    }

    static void uploadFpOffOverlayTexture()
    {
        if (!g.fpOffDirtyThisFrame) return;

        const int sw = FP_OFF_WIDTH;
        const int sh = FP_OFF_HEIGHT;
        const int tw = fpOffTextureWidth();
        const int th = fpOffTextureHeight();
        const size_t texPixels = static_cast<size_t>(tw) * static_cast<size_t>(th);
        const unsigned char* src = g.fpOffSource ? g.fpOffSource : ensureFpOffBuffer();

        if (g.fpOffRgba.size() != texPixels * 4)
            g.fpOffRgba.assign(texPixels * 4, 0);
        else
            std::memset(g.fpOffRgba.data(), 0, g.fpOffRgba.size());

        for (int y = 0; y < sh; ++y)
        {
            const unsigned char* srow = src + static_cast<size_t>(y) * static_cast<size_t>(sw);
            unsigned char* drow = g.fpOffRgba.data() + (static_cast<size_t>(y) * static_cast<size_t>(tw) * 4);
            for (int x = 0; x < sw; ++x)
            {
                const unsigned char idx = srow[x];
                unsigned char* d = drow + static_cast<size_t>(x) * 4;
                d[0] = static_cast<unsigned char>(static_cast<unsigned char>(palette[idx * 3 + 0]) << 2);
                d[1] = static_cast<unsigned char>(static_cast<unsigned char>(palette[idx * 3 + 1]) << 2);
                d[2] = static_cast<unsigned char>(static_cast<unsigned char>(palette[idx * 3 + 2]) << 2);
                d[3] = 255;
            }
        }

        if (!g.fpOffTexReady)
        {
            glGenTextures(1, &g.fpOffTex);
            g.fpOffTexReady = true;
            g.fpOffTexW = 0;
            g.fpOffTexH = 0;
        }

        glBindTexture(GL_TEXTURE_2D, g.fpOffTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

        if (g.fpOffTexW != tw || g.fpOffTexH != th)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tw, th, 0, GL_RGBA, GL_UNSIGNED_BYTE, g.fpOffRgba.data());
            g.fpOffTexW = tw;
            g.fpOffTexH = th;
        }
        else
        {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tw, th, GL_RGBA, GL_UNSIGNED_BYTE, g.fpOffRgba.data());
        }
    }

    static void drawFpOffOverlayIfDirty()
    {
        if (!g.fpOffDirtyThisFrame || !g.ready) return;

        wglMakeCurrent(g.hdc, g.hglrc);
        uploadFpOffOverlayTexture();
        if (!g.fpOffTexReady)
        {
            g.fpOffDirtyThisFrame = false;
            return;
        }

        const float umax = static_cast<float>(FP_OFF_WIDTH) / static_cast<float>(max(1, g.fpOffTexW));
        const float vmax = static_cast<float>(FP_OFF_HEIGHT) / static_cast<float>(max(1, g.fpOffTexH));

        begin2D();
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_ALPHA_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_FOG);
        glDepthMask(GL_FALSE);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g.fpOffTex);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
        glTexCoord2f(umax, 0.0f); glVertex2f(static_cast<float>(max(1, g.w)), 0.0f);
        glTexCoord2f(umax, vmax); glVertex2f(static_cast<float>(max(1, g.w)), static_cast<float>(max(1, g.h)));
        glTexCoord2f(0.0f, vmax); glVertex2f(0.0f, static_cast<float>(max(1, g.h)));
        glEnd();

        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glDepthMask(GL_TRUE);
        end2D();

        g.fpOffDirtyThisFrame = false;
    }

    static void draw2DLine(long x1, long y1, long x2, long y2, unsigned char col)
    {
        begin2D();
        glDisable(GL_TEXTURE_2D);
        glColor3ub(static_cast<GLubyte>(palette[col * 3 + 0] << 2),
            static_cast<GLubyte>(palette[col * 3 + 1] << 2),
            static_cast<GLubyte>(palette[col * 3 + 2] << 2));
        glBegin(GL_LINES);
        glVertex2f(static_cast<float>(x1), static_cast<float>(y1));
        glVertex2f(static_cast<float>(x2), static_cast<float>(y2));
        glEnd();
        glEnable(GL_TEXTURE_2D);
        end2D();
    }



    // -------------------------------------------------------------------------
    // BUILD-style per-column sector visibility clipping.
    //
    // The original software renderer and Polymost do not decide visibility by a
    // plain flood-fill over every nextsector. They keep an upper/lower visible
    // screen span for every column (umost/dmost, or Polymost's VSP equivalent),
    // update those spans as solid wall pieces occlude them, and only recurse
    // through a portal if that portal still has at least one open column.
    // -------------------------------------------------------------------------
    struct BuildMostSpan
    {
        bool valid = false;
        int x0 = 0;
        int x1 = 0;
        int cy0 = 0;
        int cy1 = 0;
        int fy0 = 0;
        int fy1 = 0;
        int ncy0 = 0;
        int ncy1 = 0;
        int nfy0 = 0;
        int nfy1 = 0;

        // Map-space endpoints after clipping the wall to the renderer close
        // plane. Polymost samples getzsofslope() at these clipped n0/n1
        // positions. Sampling at the original wall endpoints makes the portal
        // opening wrong when a wall crosses the near plane.
        double mx0 = 0.0;
        double my0 = 0.0;
        double mx1 = 0.0;
        double my1 = 0.0;
        double depth0 = 1.0;
        double depth1 = 1.0;
    };

    struct MostState
    {
        std::array<short, MAXXDIM> u{};
        std::array<short, MAXXDIM> d{};
    };

#ifndef BUILDGL_CLOSE_PORTAL_MAP_UNITS
    // Polymost has a "Scan close sectors" escape hatch so walls/portals right
    // next to the camera do not collapse to a bogus projected span. This value
    // is in BUILD map units. Raise slightly if very-close door/subway portals
    // still drop their neighbor sector; lower it if it admits too much overdraw.
#define BUILDGL_CLOSE_PORTAL_MAP_UNITS 64.0
#endif

    static int mostActiveWidth()
    {
        return min<int>(static_cast<int>(max(1L, xdim)), MAXXDIM);
    }

    static int mostClampY(double y)
    {
        if (y < 0.0) return 0;
        const int maxY = static_cast<int>(max(1L, ydim) - 1);
        if (y > static_cast<double>(maxY)) return maxY;
        return static_cast<int>(std::lround(y));
    }

    static int mostLerpInt(int a, int b, int x, int x0, int x1)
    {
        if (x1 == x0) return a;
        return a + static_cast<int>((static_cast<long long>(b - a) * static_cast<long long>(x - x0)) / static_cast<long long>(x1 - x0));
    }

    static bool mostProjectWallBase(short sectnum, int wi, BuildMostSpan& out)
    {
        if (sectnum < 0 || sectnum >= numsectors || wi < 0 || wi >= numwalls)
            return false;

        const walltype& w = wall[wi];
        if (w.point2 < 0 || w.point2 >= numwalls)
            return false;

        const walltype& w2 = wall[w.point2];

        const double ca = static_cast<double>(cosglobalang) / 16384.0;
        const double sa = static_cast<double>(singlobalang) / 16384.0;

        auto toView = [&](double x, double y, double& vx, double& vy)
            {
                const double dx = x - static_cast<double>(globalposx);
                const double dy = y - static_cast<double>(globalposy);
                vx = dy * ca - dx * sa;
                vy = dx * ca + dy * sa;
            };

        double mx0 = static_cast<double>(w.x);
        double my0 = static_cast<double>(w.y);
        double mx1 = static_cast<double>(w2.x);
        double my1 = static_cast<double>(w2.y);

        double vx0 = 0.0, vy0 = 0.0, vx1 = 0.0, vy1 = 0.0;
        toView(mx0, my0, vx0, vy0);
        toView(mx1, my1, vx1, vy1);

        const double nearPlane = max(1.0, static_cast<double>(BUILDGL_CLOSE_MAP_UNITS));
        if (vy0 <= nearPlane && vy1 <= nearPlane)
            return false;

        if (vy0 <= nearPlane)
        {
            const double t = (nearPlane - vy0) / (vy1 - vy0);
            vx0 += (vx1 - vx0) * t;
            vy0 = nearPlane;
            mx0 += (mx1 - mx0) * t;
            my0 += (my1 - my0) * t;
        }
        if (vy1 <= nearPlane)
        {
            const double t = (nearPlane - vy1) / (vy0 - vy1);
            vx1 += (vx0 - vx1) * t;
            vy1 = nearPlane;
            mx1 += (mx0 - mx1) * t;
            my1 += (my0 - my1) * t;
        }

        const double halfW = static_cast<double>(halfxdimen > 0 ? halfxdimen : (xdim >> 1));
        const double screenScale = halfW * (static_cast<double>(viewingrange) / 65536.0);
        const double sx0 = halfW + (vx0 * screenScale) / max(1.0, vy0);
        const double sx1 = halfW + (vx1 * screenScale) / max(1.0, vy1);

        if (sx1 <= sx0)
            return false;

        const int ix0 = static_cast<int>(std::floor(sx0));
        const int ix1 = static_cast<int>(std::ceil(sx1));
        const int width = mostActiveWidth();
        if (ix1 < 0 || ix0 >= width || ix1 <= ix0)
            return false;

        auto screenY = [&](long z, double depth) -> int
            {
                const double relz = static_cast<double>(z - globalposz);
                const double y = static_cast<double>(globalhoriz) + (relz * (static_cast<double>(yxaspect) / 65536.0)) / max(1.0, depth);
                return mostClampY(y * (static_cast<double>(ydim) / 200.0));
            };

        const long smx0 = static_cast<long>(std::llround(mx0));
        const long smy0 = static_cast<long>(std::llround(my0));
        const long smx1 = static_cast<long>(std::llround(mx1));
        const long smy1 = static_cast<long>(std::llround(my1));

        out.valid = true;
        // Keep unclamped screen X endpoints. The per-column loops clamp to the
        // framebuffer, but the y interpolation must still use the true projected
        // wall endpoints; otherwise off-screen portals get the wrong umost/dmost.
        out.x0 = ix0;
        out.x1 = ix1;
        out.mx0 = mx0;
        out.my0 = my0;
        out.mx1 = mx1;
        out.my1 = my1;
        out.depth0 = max(1.0, vy0);
        out.depth1 = max(1.0, vy1);
        out.cy0 = screenY(slopeZ(sectnum, smx0, smy0, true), out.depth0);
        out.cy1 = screenY(slopeZ(sectnum, smx1, smy1, true), out.depth1);
        out.fy0 = screenY(slopeZ(sectnum, smx0, smy0, false), out.depth0);
        out.fy1 = screenY(slopeZ(sectnum, smx1, smy1, false), out.depth1);
        out.ncy0 = out.cy0;
        out.ncy1 = out.cy1;
        out.nfy0 = out.fy0;
        out.nfy1 = out.fy1;
        return true;
    }

    static bool mostProjectPortal(short sectnum, int wi, BuildMostSpan& out)
    {
        if (!mostProjectWallBase(sectnum, wi, out))
            return false;

        const walltype& w = wall[wi];
        const short nextsect = w.nextsector;
        if (nextsect < 0 || nextsect >= numsectors)
            return false;

        auto screenY = [&](long z, double depth) -> int
            {
                const double relz = static_cast<double>(z - globalposz);
                const double y = static_cast<double>(globalhoriz) + (relz * (static_cast<double>(yxaspect) / 65536.0)) / max(1.0, depth);
                return mostClampY(y * (static_cast<double>(ydim) / 200.0));
            };

        const long mx0 = static_cast<long>(std::llround(out.mx0));
        const long my0 = static_cast<long>(std::llround(out.my0));
        const long mx1 = static_cast<long>(std::llround(out.mx1));
        const long my1 = static_cast<long>(std::llround(out.my1));

        out.ncy0 = screenY(slopeZ(nextsect, mx0, my0, true), out.depth0);
        out.ncy1 = screenY(slopeZ(nextsect, mx1, my1, true), out.depth1);
        out.nfy0 = screenY(slopeZ(nextsect, mx0, my0, false), out.depth0);
        out.nfy1 = screenY(slopeZ(nextsect, mx1, my1, false), out.depth1);
        return true;
    }

    static void mostLoadState(const MostState& state)
    {
        const int width = mostActiveWidth();
        for (int x = 0; x < width; ++x)
        {
            umost[x] = state.u[x];
            dmost[x] = state.d[x];
        }
    }

    static int mostCountOpenColumns(const MostState& state)
    {
        int count = 0;
        const int width = mostActiveWidth();
        for (int x = 0; x < width; ++x)
            if (state.u[x] < state.d[x])
                ++count;
        return count;
    }

    static bool mostRangeStillVisible(const MostState& state, const BuildMostSpan& span)
    {
        if (!span.valid)
            return false;

        const int width = mostActiveWidth();
        const int x0 = max(0, span.x0);
        const int x1 = min(width - 1, span.x1);
        if (x1 < x0)
            return false;

        for (int x = x0; x <= x1; ++x)
        {
            if (state.u[x] >= state.d[x])
                continue;

            const int cy = mostLerpInt(span.cy0, span.cy1, x, span.x0, span.x1);
            const int fy = mostLerpInt(span.fy0, span.fy1, x, span.x0, span.x1);
            const int ncy = mostLerpInt(span.ncy0, span.ncy1, x, span.x0, span.x1);
            const int nfy = mostLerpInt(span.nfy0, span.nfy1, x, span.x0, span.x1);

            const int openTop = max<int>(state.u[x], max(cy, ncy));
            const int openBot = min<int>(state.d[x], min(fy, nfy));
            if (openTop < openBot)
                return true;
        }
        return false;
    }

    static bool mostBuildChildThroughPortal(const MostState& parent, const BuildMostSpan& span, MostState& child)
    {
        const int width = mostActiveWidth();
        for (int x = 0; x < width; ++x)
        {
            child.u[x] = 0;
            child.d[x] = 0;
        }

        if (!span.valid)
            return false;

        const int x0 = max(0, span.x0);
        const int x1 = min(width - 1, span.x1);
        if (x1 < x0)
            return false;

        bool any = false;
        for (int x = x0; x <= x1; ++x)
        {
            if (parent.u[x] >= parent.d[x])
                continue;

            const int cy = mostLerpInt(span.cy0, span.cy1, x, span.x0, span.x1);
            const int fy = mostLerpInt(span.fy0, span.fy1, x, span.x0, span.x1);
            const int ncy = mostLerpInt(span.ncy0, span.ncy1, x, span.x0, span.x1);
            const int nfy = mostLerpInt(span.nfy0, span.nfy1, x, span.x0, span.x1);

            // The child sector is only visible through the overlap of the
            // parent's current span and this portal's ceiling/floor opening.
            // Closing every column outside the portal is the missing bit in the
            // original drop-in scan; otherwise a sector reached through a tiny
            // subway doorway can continue scanning/drawing as if it were visible
            // full-screen.
            const int openTop = max<int>(parent.u[x], max(cy, ncy));
            const int openBot = min<int>(parent.d[x], min(fy, nfy));

            if (openTop < openBot)
            {
                child.u[x] = static_cast<short>(openTop);
                child.d[x] = static_cast<short>(openBot);
                any = true;
            }
        }

        return any;
    }

    static void mostCopyState(const MostState& src, MostState& dst)
    {
        const int width = mostActiveWidth();
        for (int x = 0; x < width; ++x)
        {
            dst.u[x] = src.u[x];
            dst.d[x] = src.d[x];
        }
    }

    static bool mostPortalIsCloseToView(int wi)
    {
        if (wi < 0 || wi >= numwalls)
            return false;

        const walltype& w = wall[wi];
        if (w.point2 < 0 || w.point2 >= numwalls)
            return false;

        const walltype& w2 = wall[w.point2];
        const double x0 = static_cast<double>(w.x - globalposx);
        const double y0 = static_cast<double>(w.y - globalposy);
        const double x1 = static_cast<double>(w2.x - globalposx);
        const double y1 = static_cast<double>(w2.y - globalposy);
        const double dx = x1 - x0;
        const double dy = y1 - y0;
        const double len2 = dx * dx + dy * dy;
        if (len2 <= 1.0)
            return false;

        const double t = -((x0 * dx + y0 * dy) / len2);
        if (t < -0.25 || t > 1.25)
            return false;

        const double cross = x0 * y1 - x1 * y0;
        const double closeDist = static_cast<double>(BUILDGL_CLOSE_PORTAL_MAP_UNITS);
        return (cross * cross) <= (len2 * closeDist * closeDist);
    }

    static bool mostWallIsMaskedPortalPair(int wi)
    {
        if (wi < 0 || wi >= numwalls)
            return false;

        short combined = wall[wi].cstat;
        if (wall[wi].nextwall >= 0 && wall[wi].nextwall < numwalls)
            combined = static_cast<short>(combined | wall[wall[wi].nextwall].cstat);

        return (combined & 16) != 0;
    }

    static void mostClipToPortalOpening(const BuildMostSpan& span)
    {
        if (!span.valid)
            return;

        const int width = mostActiveWidth();
        const int x0 = max(0, span.x0);
        const int x1 = min(width - 1, span.x1);
        for (int x = x0; x <= x1; ++x)
        {
            const int cy = mostLerpInt(span.cy0, span.cy1, x, span.x0, span.x1);
            const int fy = mostLerpInt(span.fy0, span.fy1, x, span.x0, span.x1);
            const int ncy = mostLerpInt(span.ncy0, span.ncy1, x, span.x0, span.x1);
            const int nfy = mostLerpInt(span.nfy0, span.nfy1, x, span.x0, span.x1);
            const int openTop = max(cy, ncy);
            const int openBot = min(fy, nfy);

            if (openTop >= openBot)
            {
                umost[x] = dmost[x];
                continue;
            }

            umost[x] = static_cast<short>(max<int>(umost[x], min<int>(openTop, dmost[x])));
            dmost[x] = static_cast<short>(min<int>(dmost[x], max<int>(openBot, umost[x])));
        }
    }

    static void mostCloseSolidWall(short sectnum, int wi)
    {
        BuildMostSpan span;
        if (!mostProjectWallBase(sectnum, wi, span))
            return;

        const int width = mostActiveWidth();
        const int x0 = max(0, span.x0);
        const int x1 = min(width - 1, span.x1);
        for (int x = x0; x <= x1; ++x)
            umost[x] = dmost[x];
    }

    static void mostInitFullScreen(MostState& state)
    {
        const int width = mostActiveWidth();
        const short bottom = static_cast<short>(max(0L, ydim - 1));
        for (int x = 0; x < width; ++x)
        {
            state.u[x] = 0;
            state.d[x] = bottom;
            umost[x] = state.u[x];
            dmost[x] = state.d[x];
            startumost[x] = umost[x];
            startdmost[x] = dmost[x];
        }
    }

    static void mostInitFullScreen()
    {
        MostState state;
        mostInitFullScreen(state);
    }

} // namespace buildgl

// -----------------------------------------------------------------------------
// Engine initialization, file/data loading.
// -----------------------------------------------------------------------------
static unsigned char buildgl_to_build6(unsigned char v)
{
    // BUILD palettes are normally 0..63. Accept 0..255 input too.
    return static_cast<unsigned char>((v > 63) ? (v >> 2) : v);
}

static unsigned short buildgl_read_u16le(const unsigned char b[2])
{
    return static_cast<unsigned short>(b[0] | (static_cast<unsigned short>(b[1]) << 8));
}

static void buildgl_make_identity_palookup(char* table, long shades)
{
    if (!table || shades <= 0) return;

    for (long s = 0; s < shades; ++s)
        for (int i = 0; i < 256; ++i)
            table[(s << 8) + i] = static_cast<char>(i);
}

// -----------------------------------------------------------------------------
extern "C" void initksqrt()
{
    for (int i = 0; i < 2048; ++i)
        sintable[i] = static_cast<short>(std::lround(std::sin((i * 2.0 * buildgl::PI) / 2048.0) * 16384.0));

    //    for (int b = 0; b < 16; ++b)
    //        for (int i = 0; i < 64; ++i)
    //            britable[b][i] = static_cast<char>(min(63, (i * (b + 1)) / 16));
}

extern "C" void loadtables()
{
    initksqrt();
    short fil = kopen4load((char*)"tables.dat", 0);
    if (fil >= 0)
    {
        kread(fil, sintable, 2048 * 2);

        kread(fil, radarang, 1280);

        kread(fil, textfont, 1024);
        kread(fil, smalltextfont, 1024);
        kread(fil, britable, 16 * 64);
        kclose(fil);
    }
}

extern "C" void loadpalette()
{
    if (paletteloaded) return;

    buildgl::resetLookupPalettes();

    // Fallback palette. Keep it in basepalette so brightness never compounds.
    for (int i = 0; i < 256; ++i)
    {
        basepalette[i * 3 + 0] = static_cast<unsigned char>((i >> 0) & 63);
        basepalette[i * 3 + 1] = static_cast<unsigned char>((i >> 1) & 63);
        basepalette[i * 3 + 2] = static_cast<unsigned char>((i >> 2) & 63);
    }

    std::memcpy(palette, basepalette, sizeof(palette));
    basepalettevalid = true;
    bool loadedPaletteDat = false;

    short fil = kopen4load((char*)"palette.dat", 0);
    if (fil >= 0)
    {
        unsigned char shadeCountLE[2] = {};

        if (kread(fil, basepalette, 768) == 768 &&
            kread(fil, shadeCountLE, 2) == 2)
        {
            std::memcpy(palette, basepalette, sizeof(palette));
            basepalettevalid = true;
            loadedPaletteDat = true;

            numpalookups = static_cast<short>(buildgl_read_u16le(shadeCountLE));
            numpalookups = static_cast<short>(
                min(max(static_cast<long>(numpalookups), 1L), static_cast<long>(MAXPALOOKUPS))
                );

            if (!palookup[0])
                palookup[0] = static_cast<char*>(std::malloc(static_cast<size_t>(numpalookups) << 8));

            if (palookup[0])
            {
                const long want = static_cast<long>(numpalookups) << 8;
                if (kread(fil, palookup[0], want) != want)
                    buildgl_make_identity_palookup(palookup[0], numpalookups);
            }

            if (!transluc)
                transluc = static_cast<char*>(std::malloc(65536));

            if (transluc)
            {
                if (kread(fil, transluc, 65536) != 65536)
                    std::memset(transluc, 0, 65536);
            }
        }

        kclose(fil);
    }

    if (!loadedPaletteDat)
        numpalookups = 32;

    if (!palookup[0])
        palookup[0] = static_cast<char*>(std::malloc(static_cast<size_t>(numpalookups) << 8));

    if (palookup[0] && !loadedPaletteDat)
        buildgl_make_identity_palookup(palookup[0], numpalookups);

    if (!transluc)
        transluc = static_cast<char*>(std::malloc(65536));

    if (transluc && !loadedPaletteDat)
        std::memset(transluc, 0, 65536);

    buildgl::loadLookupDatPalettes();
    buildgl::markAllTexturesDirty();
    paletteloaded = 1;
}

extern "C" int loadpics(char* filename)
{
    if (filename && filename[0]) std::strncpy(artfilename, filename, sizeof(artfilename) - 1);
    else std::strncpy(artfilename, "tiles000.art", sizeof(artfilename) - 1);

    for (int i = 0; i < MAXTILES; ++i)
    {
        tilesizx[i] = tilesizy[i] = picanm[i] = tilefileoffs[i] = 0;
        tilefilenum[i] = 0;
        picsiz[i] = 0;
        waloff[i] = 0;
        walock[i] = 0;
        buildgl::g.tileData[i].clear();
        buildgl::g.fallbackTile[i] = 0;
        buildgl::markTileDirty(static_cast<short>(i));
    }

    numtilefiles = 0;
    artsize = 0;
    for (int fnum = 0; fnum < 256; ++fnum)
    {
        std::string name = buildgl::makeArtName(fnum);
        short fil = kopen4load(const_cast<char*>(name.c_str()), 0);
        if (fil < 0)
        {
            if (fnum == 0)
            {
                // Also try caller-provided name for the first file.
                fil = kopen4load(artfilename, 0);
            }
        }
        if (fil < 0) break;

        long localNumTiles = 0, localStart = 0, localEnd = -1;
        kread(fil, &artversion, 4);
        if (artversion != 1) { kclose(fil); return -1; }
        kread(fil, &localNumTiles, 4);
        kread(fil, &localStart, 4);
        kread(fil, &localEnd, 4);
        if (localStart < 0 || localEnd < localStart || localEnd >= MAXTILES) { kclose(fil); return -1; }

        int count = static_cast<int>(localEnd - localStart + 1);
        std::vector<short> xs(count), ys(count);
        std::vector<long> anim(count);
        kread(fil, xs.data(), count * 2);
        kread(fil, ys.data(), count * 2);
        kread(fil, anim.data(), count * 4);

        long offs = 16 + count * 8;
        for (int i = 0; i < count; ++i)
        {
            int tile = static_cast<int>(localStart) + i;
            tilesizx[tile] = xs[i];
            tilesizy[tile] = ys[i];
            picanm[tile] = anim[i];
            tilefilenum[tile] = static_cast<char>(fnum);
            tilefileoffs[tile] = offs;
            long sz = static_cast<long>(xs[i]) * static_cast<long>(ys[i]);
            offs += sz;
            artsize += ((sz + 15) & ~15);
            int px = 0; while ((1 << px) < max(1L, tilesizx[tile]) && px < 15) ++px;
            int py = 0; while ((1 << py) < max(1L, tilesizy[tile]) && py < 15) ++py;
            picsiz[tile] = static_cast<char>(px | (py << 4));
        }
        kclose(fil);
        ++numtilefiles;
    }

    if (numtilefiles == 0) buildgl::ensureFallbackTile(0);
    return 0;
}

extern "C" void loadtile(short tilenume)
{
    if ((unsigned short)tilenume >= MAXTILES) return;
    long sz = tilesizx[tilenume] * tilesizy[tilenume];
    if (sz <= 0) return;
    if (waloff[tilenume]) return;

    int fnum = static_cast<unsigned char>(tilefilenum[tilenume]);
    std::string name = buildgl::makeArtName(fnum);
    short fil = kopen4load(const_cast<char*>(name.c_str()), 0);
    if (fil < 0 && fnum == 0) fil = kopen4load(artfilename, 0);
    if (fil < 0) return;

    buildgl::g.tileData[tilenume].resize(static_cast<size_t>(sz));
    klseek(fil, tilefileoffs[tilenume], SEEK_SET);
    kread(fil, buildgl::g.tileData[tilenume].data(), sz);
    kclose(fil);

    waloff[tilenume] = BUILDGL_PTR_TO_LONG(buildgl::g.tileData[tilenume].data());
    buildgl::g.fallbackTile[tilenume] = 0;
    walock[tilenume] = 199;
    buildgl::markTileDirty(tilenume);
}

extern "C" long allocatepermanenttile(short tilenume, long xsiz, long ysiz)
{
    if ((unsigned short)tilenume >= MAXTILES || xsiz <= 0 || ysiz <= 0) return 0;
    buildgl::g.tileData[tilenume].assign(static_cast<size_t>(xsiz * ysiz), 255);
    waloff[tilenume] = BUILDGL_PTR_TO_LONG(buildgl::g.tileData[tilenume].data());
    buildgl::g.fallbackTile[tilenume] = 0;
    walock[tilenume] = 255;
    tilesizx[tilenume] = xsiz;
    tilesizy[tilenume] = ysiz;
    picanm[tilenume] = 0;
    int px = 0; while ((1 << px) < xsiz && px < 15) ++px;
    int py = 0; while ((1 << py) < ysiz && py < 15) ++py;
    picsiz[tilenume] = static_cast<char>(px | (py << 4));
    buildgl::markTileDirty(tilenume);
    return waloff[tilenume];
}

extern "C" int loadboard(char* filename, long* daposx, long* daposy, long* daposz, short* daang, short* dacursectnum)
{
    if (!filename) return -1;
    char searchfirst = 0;
    size_t namelen = std::strlen(filename);
    if (namelen > 0 && static_cast<unsigned char>(filename[namelen - 1]) == 255)
    {
        filename[namelen - 1] = 0;
        searchfirst = 1;
    }
    short fil = kopen4load(filename, searchfirst);
    if (fil < 0) { mapversion = 7; return -1; }

    kread(fil, &mapversion, 4);
    if (mapversion != 7) { kclose(fil); return -1; }

    initspritelists();
    std::memset(show2dsector, 0, sizeof(show2dsector));
    std::memset(show2dwall, 0, sizeof(show2dwall));
    std::memset(show2dsprite, 0, sizeof(show2dsprite));

    kread(fil, daposx, 4);
    kread(fil, daposy, 4);
    kread(fil, daposz, 4);
    kread(fil, daang, 2);
    kread(fil, dacursectnum, 2);

    kread(fil, &numsectors, 2);
    if (numsectors < 0 || numsectors > MAXSECTORS) { kclose(fil); return -1; }
    kread(fil, sector, sizeof(sectortype) * numsectors);

    kread(fil, &numwalls, 2);
    if (numwalls < 0 || numwalls > MAXWALLS) { kclose(fil); return -1; }
    kread(fil, wall, sizeof(walltype) * numwalls);

    short numsprites = 0;
    kread(fil, &numsprites, 2);
    if (numsprites < 0 || numsprites > MAXSPRITES) { kclose(fil); return -1; }
    for (short i = 0; i < numsprites; ++i)
    {
        kread(fil, &sprite[i], sizeof(spritetype));
        short s = insertsprite(sprite[i].sectnum, sprite[i].statnum);
        if (s >= 0 && s != i) sprite[s] = sprite[i];
    }

    if (dacursectnum) updatesector(*daposx, *daposy, dacursectnum);
    kclose(fil);
    return 0;
}

extern "C" int saveboard(char* filename, long* daposx, long* daposy, long* daposz, short* daang, short* dacursectnum)
{
    if (!filename) return -1;
    FILE* f = std::fopen(filename, "wb");
    if (!f) return -1;
    mapversion = 7;
    std::fwrite(&mapversion, 4, 1, f);
    std::fwrite(daposx, 4, 1, f);
    std::fwrite(daposy, 4, 1, f);
    std::fwrite(daposz, 4, 1, f);
    std::fwrite(daang, 2, 1, f);
    std::fwrite(dacursectnum, 2, 1, f);
    std::fwrite(&numsectors, 2, 1, f);
    std::fwrite(sector, sizeof(sectortype), numsectors, f);
    std::fwrite(&numwalls, 2, 1, f);
    std::fwrite(wall, sizeof(walltype), numwalls, f);

    short numsprites = 0;
    for (int st = 0; st < MAXSTATUS; ++st)
        for (short i = headspritestat[st]; i >= 0; i = nextspritestat[i]) ++numsprites;
    std::fwrite(&numsprites, 2, 1, f);
    for (int st = 0; st < MAXSTATUS; ++st)
        for (short i = headspritestat[st]; i >= 0; i = nextspritestat[i])
            std::fwrite(&sprite[i], sizeof(spritetype), 1, f);
    std::fclose(f);
    return 0;
}

// -----------------------------------------------------------------------------
// Rendering entry points: original names, OpenGL-injected bodies.
// -----------------------------------------------------------------------------
#ifdef FP_OFF
#undef FP_OFF
#endif
extern "C" long FP_OFF(void* source)
{
    return buildgl::fpOffCompatibility(source);
}

extern "C" long BUILDGL_FP_OFF_BUFFER()
{
    return buildgl::fpOffCompatibility(nullptr);
}

extern "C" int setgamemode(char davidoption, long daxdim, long daydim)
{
    (void)davidoption;
    if (daxdim <= 0) daxdim = 640;
    if (daydim <= 0) daydim = 480;
    xdim = daxdim; ydim = daydim; bytesperline = xdim; imageSize = xdim * ydim;
    qsetmode = 200; vidoption = 1; numpages = 1; activepage = visualpage = 0;
    for (long y = 0, p = 0; y <= ydim && y <= MAXYDIM; ++y, p += bytesperline) ylookup[y] = p;
    setview(0, 0, xdim - 1, ydim - 1);
    if (!buildgl::createWindow(static_cast<int>(xdim), static_cast<int>(ydim), "Duke Nukem 3D")) return -1;
    return 0;
}

extern "C" void initengine()
{
    loadtables();
    loadpalette();
    for (int i = 0; i < MAXPALOOKUPS; ++i) if (!palookup[i]) palookup[i] = nullptr;
    initspritelists();
    pskyoff[0] = 0; pskybits = 0;
    visibility = parallaxvisibility = 512;
    parallaxtype = 2; parallaxyscale = 65536; parallaxyoffs = 0;
    xyaspect = yxaspect = viewingrange = viewingrangerecip = 65536;
    searchit = 0; searchstat = -1;
    if (xdim <= 0 || ydim <= 0) { xdim = 640; ydim = 480; }
}

extern "C" void uninitengine()
{
    if (buildgl::g.ready)
        wglMakeCurrent(buildgl::g.hdc, buildgl::g.hglrc);

    for (int i = 0; i < MAXTILES; ++i)
    {
        for (int pal = 0; pal < MAXPALOOKUPS; ++pal)
        {
            if (buildgl::g.texReady[i][pal])
            {
                GLuint t = buildgl::g.tex[i][pal];
                glDeleteTextures(1, &t);
                buildgl::g.texReady[i][pal] = 0;
            }
            buildgl::g.texDirty[i][pal] = 1;
        }
    }

    if (buildgl::g.fpOffTexReady)
    {
        GLuint t = buildgl::g.fpOffTex;
        glDeleteTextures(1, &t);
        buildgl::g.fpOffTex = 0;
        buildgl::g.fpOffTexReady = false;
        buildgl::g.fpOffTexW = 0;
        buildgl::g.fpOffTexH = 0;
    }
    buildgl::g.fpOffBuffer.clear();
    buildgl::g.fpOffRgba.clear();
    buildgl::g.fpOffSource = nullptr;
    buildgl::g.fpOffDirtyThisFrame = false;

    buildgl::destroy();
    for (int i = 0; i < MAXPALOOKUPS; ++i)
    {
        if (palookup[i]) { std::free(palookup[i]); palookup[i] = nullptr; }
    }
    if (transluc) { std::free(transluc); transluc = nullptr; }
    for (int i = 0; i < MAXTILES; ++i)
    {
        buildgl::g.tileData[i].clear();
        buildgl::g.fallbackTile[i] = 0;
        waloff[i] = 0;
    }
}

extern "C" void drawrooms(long daposx, long daposy, long daposz, short daang, long dahoriz, short dacursectnum)
{
    beforedrawrooms = 0;
    totalarea += (windowx2 + 1 - windowx1) * (windowy2 + 1 - windowy1);

    globalposx = daposx; globalposy = daposy; globalposz = daposz;
    globalang = static_cast<short>(daang & 2047);
    globalhoriz = dahoriz;
    globalcursectnum = dacursectnum;

    cosglobalang = sintable[(globalang + 512) & 2047];
    singlobalang = sintable[globalang & 2047];
    cosviewingrangeglobalang = mulscale16(cosglobalang, viewingrange);
    sinviewingrangeglobalang = mulscale16(singlobalang, viewingrange);

    if (globalcursectnum >= 0) updatesector(globalposx, globalposy, &globalcursectnum);

    std::memset(gotsector, 0, sizeof(gotsector));
    spritesortcnt = 0; maskwallcnt = 0;

    buildgl::begin3D();

    // Keep the old sequence visible to callers: scansector -> drawalls.
    scansector(globalcursectnum);
    drawalls(0);
}

extern "C" void scansector(short sectnum)
{
    if (sectnum < 0 || sectnum >= numsectors) return;

    show2dsector[sectnum >> 3] |= pow2char[sectnum & 7];
    std::memset(gotsector, 0, sizeof(gotsector));
    spritesortcnt = 0;
    maskwallcnt = 0;

    struct SectorMostNode
    {
        short sect = -1;
        buildgl::MostState most;
    };

    std::deque<SectorMostNode> queue;
    std::vector<int> bestOpen(static_cast<size_t>(max<long>(numsectors, 0L)), -1);

    SectorMostNode first;
    first.sect = sectnum;
    buildgl::mostInitFullScreen(first.most);
    bestOpen[static_cast<size_t>(sectnum)] = buildgl::mostCountOpenColumns(first.most);
    queue.push_back(first);

    int guard = 0;
    const int guardLimit = max<int>(MAXSECTORS * 8, 1024);

    while (!queue.empty() && guard++ < guardLimit)
    {
        SectorMostNode node = queue.front();
        queue.pop_front();

        short s = node.sect;
        if (s < 0 || s >= numsectors) continue;

        buildgl::mostLoadState(node.most);

        const bool firstVisit = (gotsector[s >> 3] & pow2char[s & 7]) == 0;
        if (firstVisit)
        {
            gotsector[s >> 3] |= pow2char[s & 7];
            show2dsector[s >> 3] |= pow2char[s & 7];

            for (short spr = headspritesect[s]; spr >= 0; spr = nextspritesect[spr])
            {
                if (spritesortcnt < MAXSPRITESONSCREEN &&
                    sprite[spr].xrepeat > 0 && sprite[spr].yrepeat > 0 &&
                    (((sprite[spr].cstat & 0x8000) == 0) || showinvisibility))
                {
                    tsprite[spritesortcnt] = sprite[spr];
                    tsprite[spritesortcnt].owner = spr;
                    ++spritesortcnt;
                }
            }
        }

        int start = sector[s].wallptr;
        int end = start + sector[s].wallnum;
        if (start < 0 || end > numwalls) continue;

        for (int wi = start; wi < end && wi < numwalls; ++wi)
        {
            walltype& w = wall[wi];

            if (firstVisit && (w.cstat & 48) && maskwallcnt < MAXWALLSB)
                maskwall[maskwallcnt++] = static_cast<short>(wi);

            const short ns = w.nextsector;

            // cstat 32 by itself is a one-way/blocking portal for visibility,
            // but cstat 16|32 is the common BUILD masked/forcefield case. Some
            // maps put the masked bit on the opposite red-wall side, so check
            // the pair rather than only this wall.
            const bool maskedPortal = buildgl::mostWallIsMaskedPortalPair(wi);
            const bool oneWayBlockingPortal = ((w.cstat & 32) != 0) && !maskedPortal;

            if (ns >= 0 && ns < numsectors && !oneWayBlockingPortal)
            {
                SectorMostNode child;
                child.sect = ns;

                if (buildgl::mostPortalIsCloseToView(wi))
                {
                    // Polymost floods very-close nextsectors because their
                    // projected x-span is numerically unstable. Do the same, but
                    // keep the inherited most state path-local.
                    buildgl::mostCopyState(node.most, child.most);
                }
                else
                {
                    buildgl::BuildMostSpan span;
                    if (!buildgl::mostProjectPortal(s, wi, span))
                        continue;

                    if (!buildgl::mostRangeStillVisible(node.most, span))
                        continue;

                    if (!buildgl::mostBuildChildThroughPortal(node.most, span, child.most))
                        continue;
                }

                const int openCount = buildgl::mostCountOpenColumns(child.most);
                if (openCount <= 0)
                    continue;

                int& best = bestOpen[static_cast<size_t>(ns)];
                if (openCount > best + 2 && static_cast<int>(queue.size()) < guardLimit)
                {
                    best = openCount;
                    queue.push_back(child);
                }
            }
            else
            {
                // Actual solid wall occlusion is handled by the 3D depth pass.
                // Closing solid spans in this pre-scan would make visibility
                // depend on wall order, which Polymost avoids by drawing sorted
                // bunches and mutating its VSP during drawing.
            }
        }
    }
}

extern "C" int wallfront(long l1, long l2)
{
    if (l1 < 0 || l1 >= numwalls || l2 < 0 || l2 >= numwalls) return -1;
    walltype& a = wall[l1]; walltype& ap = wall[a.point2];
    walltype& b = wall[l2]; walltype& bp = wall[b.point2];
    long dx = ap.x - a.x, dy = ap.y - a.y;
    long t1 = dmulscale2(b.x - a.x, dy, -dx, b.y - a.y);
    long t2 = dmulscale2(bp.x - a.x, dy, -dx, bp.y - a.y);
    if (t1 == 0) t1 = t2;
    if (t2 == 0) t2 = t1;
    if ((t1 ^ t2) >= 0)
    {
        long tp = dmulscale2(globalposx - a.x, dy, -dx, globalposy - a.y);
        return ((tp ^ t1) >= 0);
    }
    return -2;
}

extern "C" int spritewallfront(spritetype* s, long w)
{
    if (!s || w < 0 || w >= numwalls) return 0;
    walltype& wal = wall[w];
    walltype& wal2 = wall[wal.point2];
    return (dmulscale32(wal2.x - wal.x, s->y - wal.y, -(s->x - wal.x), wal2.y - wal.y) >= 0);
}

extern "C" int bunchfront(long, long) { return -1; }

extern "C" void drawalls(long /*bunch*/)
{
    buildgl::renderOpaque();
}

extern "C" void drawmasks()
{
    buildgl::renderMasks();
}

extern "C" void nextpage()
{
    faketimerhandler();
    if (!buildgl::g.ready) return;
    if (buildgl::g.frameActive && !buildgl::g.maskedDrawn) buildgl::renderMasks();
    buildgl::drawFpOffOverlayIfDirty();
    SwapBuffers(buildgl::g.hdc);
    buildgl::g.frameActive = false;
    if ((totalclock >= lastageclock + 8) || (totalclock < lastageclock))
    {
        lastageclock = totalclock;
        agecache();
    }
    beforedrawrooms = 1;
    ++numframes;
}

extern "C" void prepwall(long, walltype*) {}
extern "C" void ceilscan(long, long, long sectnum) { buildgl::renderSectorSurface(static_cast<short>(sectnum), true); }
extern "C" void florscan(long, long, long sectnum) { buildgl::renderSectorSurface(static_cast<short>(sectnum), false); }
extern "C" void wallscan(long, long, short*, short*, long*, long*) { buildgl::renderWalls(false); }
extern "C" void maskwallscan(long, long, short*, short*, long*, long*) { buildgl::renderWalls(true); }
extern "C" void transmaskvline(long) {}
extern "C" void transmaskvline2(long) {}
extern "C" void transmaskwallscan(long, long) { buildgl::renderWalls(true); }
extern "C" void drawmaskwall(short) { buildgl::renderWalls(true); }
extern "C" void drawsprite(long) { buildgl::renderSprites(); }
extern "C" void hline(long, long) {}
extern "C" void slowhline(long, long) {}
extern "C" void grouscan(long, long, long sectnum, char dastat) { buildgl::renderSectorSurface(static_cast<short>(sectnum), dastat == 0); }
extern "C" void parascan(long, long, long sectnum, char dastat, long) { buildgl::renderSectorSurface(static_cast<short>(sectnum), dastat == 0); }

// -----------------------------------------------------------------------------
// Sprites, geometry, collision helpers.
// -----------------------------------------------------------------------------
extern "C" void initspritelists()
{
    for (int i = 0; i < MAXSECTORS; ++i) headspritesect[i] = -1;
    headspritesect[MAXSECTORS] = 0;
    for (int i = 0; i < MAXSPRITES; ++i)
    {
        prevspritesect[i] = static_cast<short>(i - 1);
        nextspritesect[i] = static_cast<short>(i + 1);
        sprite[i].sectnum = MAXSECTORS;
    }
    prevspritesect[0] = -1; nextspritesect[MAXSPRITES - 1] = -1;

    for (int i = 0; i < MAXSTATUS; ++i) headspritestat[i] = -1;
    headspritestat[MAXSTATUS] = 0;
    for (int i = 0; i < MAXSPRITES; ++i)
    {
        prevspritestat[i] = static_cast<short>(i - 1);
        nextspritestat[i] = static_cast<short>(i + 1);
        sprite[i].statnum = MAXSTATUS;
    }
    prevspritestat[0] = -1; nextspritestat[MAXSPRITES - 1] = -1;
}

extern "C" short insertspritesect(short sectnum)
{
    if (sectnum < 0 || sectnum >= MAXSECTORS || headspritesect[MAXSECTORS] < 0) return -1;
    short blank = headspritesect[MAXSECTORS];
    headspritesect[MAXSECTORS] = nextspritesect[blank];
    if (headspritesect[MAXSECTORS] >= 0) prevspritesect[headspritesect[MAXSECTORS]] = -1;
    prevspritesect[blank] = -1;
    nextspritesect[blank] = headspritesect[sectnum];
    if (headspritesect[sectnum] >= 0) prevspritesect[headspritesect[sectnum]] = blank;
    headspritesect[sectnum] = blank;
    sprite[blank].sectnum = sectnum;
    return blank;
}

extern "C" short insertspritestat(short statnum)
{
    if (statnum < 0 || statnum >= MAXSTATUS || headspritestat[MAXSTATUS] < 0) return -1;
    short blank = headspritestat[MAXSTATUS];
    headspritestat[MAXSTATUS] = nextspritestat[blank];
    if (headspritestat[MAXSTATUS] >= 0) prevspritestat[headspritestat[MAXSTATUS]] = -1;
    prevspritestat[blank] = -1;
    nextspritestat[blank] = headspritestat[statnum];
    if (headspritestat[statnum] >= 0) prevspritestat[headspritestat[statnum]] = blank;
    headspritestat[statnum] = blank;
    sprite[blank].statnum = statnum;
    return blank;
}

extern "C" short insertsprite(short sectnum, short statnum)
{
    // Preserve the original BUILD assumption: sector and status free lists are
    // kept in lock-step, so both insertions consume the same sprite slot.
    short st = insertspritestat(statnum);
    short ss = insertspritesect(sectnum);
    if (st < 0 || ss < 0) return -1;
    return ss;
}

extern "C" int deletespritesect(short deleteme)
{
    if (deleteme < 0 || deleteme >= MAXSPRITES || sprite[deleteme].sectnum == MAXSECTORS) return -1;
    short sectnum = sprite[deleteme].sectnum;
    if (headspritesect[sectnum] == deleteme) headspritesect[sectnum] = nextspritesect[deleteme];
    if (prevspritesect[deleteme] >= 0) nextspritesect[prevspritesect[deleteme]] = nextspritesect[deleteme];
    if (nextspritesect[deleteme] >= 0) prevspritesect[nextspritesect[deleteme]] = prevspritesect[deleteme];
    if (headspritesect[MAXSECTORS] >= 0) prevspritesect[headspritesect[MAXSECTORS]] = deleteme;
    prevspritesect[deleteme] = -1;
    nextspritesect[deleteme] = headspritesect[MAXSECTORS];
    headspritesect[MAXSECTORS] = deleteme;
    sprite[deleteme].sectnum = MAXSECTORS;
    return 0;
}

extern "C" int deletespritestat(short deleteme)
{
    if (deleteme < 0 || deleteme >= MAXSPRITES || sprite[deleteme].statnum == MAXSTATUS) return -1;
    short statnum = sprite[deleteme].statnum;
    if (headspritestat[statnum] == deleteme) headspritestat[statnum] = nextspritestat[deleteme];
    if (prevspritestat[deleteme] >= 0) nextspritestat[prevspritestat[deleteme]] = nextspritestat[deleteme];
    if (nextspritestat[deleteme] >= 0) prevspritestat[nextspritestat[deleteme]] = prevspritestat[deleteme];
    if (headspritestat[MAXSTATUS] >= 0) prevspritestat[headspritestat[MAXSTATUS]] = deleteme;
    prevspritestat[deleteme] = -1;
    nextspritestat[deleteme] = headspritestat[MAXSTATUS];
    headspritestat[MAXSTATUS] = deleteme;
    sprite[deleteme].statnum = MAXSTATUS;
    return 0;
}

extern "C" int deletesprite(short spritenum)
{
    deletespritestat(spritenum);
    return deletespritesect(spritenum);
}

extern "C" int changespritesect(short spritenum, short newsectnum)
{
    if (spritenum < 0 || spritenum >= MAXSPRITES || newsectnum < 0 || newsectnum > MAXSECTORS) return -1;
    if (sprite[spritenum].sectnum == newsectnum) return 0;
    if (sprite[spritenum].sectnum == MAXSECTORS) return -1;
    deletespritesect(spritenum);
    short oldstat = sprite[spritenum].statnum;
    short ns = insertspritesect(newsectnum);
    if (ns != spritenum && ns >= 0)
    {
        sprite[ns] = sprite[spritenum];
        sprite[ns].sectnum = newsectnum;
        sprite[ns].statnum = oldstat;
    }
    return 0;
}

extern "C" int changespritestat(short spritenum, short newstatnum)
{
    if (spritenum < 0 || spritenum >= MAXSPRITES || newstatnum < 0 || newstatnum > MAXSTATUS) return -1;
    if (sprite[spritenum].statnum == newstatnum) return 0;
    if (sprite[spritenum].statnum == MAXSTATUS) return -1;
    deletespritestat(spritenum);
    insertspritestat(newstatnum);
    return 0;
}

extern "C" int setsprite(short spritenum, long newx, long newy, long newz)
{
    if (spritenum < 0 || spritenum >= MAXSPRITES) return -1;
    sprite[spritenum].x = newx; sprite[spritenum].y = newy; sprite[spritenum].z = newz;
    short s = sprite[spritenum].sectnum;
    updatesector(newx, newy, &s);
    if (s < 0) return -1;
    if (s != sprite[spritenum].sectnum) changespritesect(spritenum, s);
    return 0;
}

extern "C" int inside(long x, long y, short sectnum)
{
    if (sectnum < 0 || sectnum >= numsectors) return -1;
    unsigned long cnt = 0;
    walltype* wal = &wall[sector[sectnum].wallptr];
    int i = sector[sectnum].wallnum;
    while (i-- > 0)
    {
        long y1 = wal->y - y;
        long y2 = wall[wal->point2].y - y;
        if ((y1 ^ y2) < 0)
        {
            long x1 = wal->x - x;
            long x2 = wall[wal->point2].x - x;
            if ((x1 ^ x2) >= 0) cnt ^= x1;
            else cnt ^= (x1 * y2 - x2 * y1) ^ y2;
        }
        ++wal;
    }
    return static_cast<int>(cnt >> 31);
}

extern "C" void updatesector(long x, long y, short* sectnum)
{
    if (!sectnum) return;
    if (*sectnum >= 0 && *sectnum < numsectors && inside(x, y, *sectnum) == 1) return;
    if (*sectnum >= 0 && *sectnum < numsectors)
    {
        int start = sector[*sectnum].wallptr, end = start + sector[*sectnum].wallnum;
        for (int i = start; i < end; ++i)
        {
            short ns = wall[i].nextsector;
            if (ns >= 0 && ns < numsectors && inside(x, y, ns) == 1) { *sectnum = ns; return; }
        }
    }
    for (short i = 0; i < numsectors; ++i)
        if (inside(x, y, i) == 1) { *sectnum = i; return; }
    *sectnum = -1;
}

extern "C" long getceilzofslope(short sectnum, long dax, long day) { return buildgl::slopeZ(sectnum, dax, day, true); }
extern "C" long getflorzofslope(short sectnum, long dax, long day) { return buildgl::slopeZ(sectnum, dax, day, false); }
extern "C" void getzsofslope(short sectnum, long dax, long day, long* ceilz, long* florz)
{
    if (ceilz) *ceilz = getceilzofslope(sectnum, dax, day);
    if (florz) *florz = getflorzofslope(sectnum, dax, day);
}

extern "C" int getangle(long xvect, long yvect)
{
    if ((xvect | yvect) == 0) return 0;
    double a = std::atan2(static_cast<double>(yvect), static_cast<double>(xvect));
    int v = static_cast<int>(std::lround(a * 2048.0 / (2.0 * buildgl::PI))) & 2047;
    return v;
}

extern "C" int animateoffs(short tilenum, short fakevar)
{
    (void)fakevar;
    if ((unsigned short)tilenum >= MAXTILES) return 0;
    long anim = picanm[tilenum];
    int count = anim & 63;
    if (count <= 0) return 0;
    long i = totalclock >> ((anim >> 24) & 15);
    switch (anim & 192)
    {
    case 64:
    {
        long k = i % (count << 1);
        return static_cast<int>(k < count ? k : ((count << 1) - k));
    }
    case 128: return static_cast<int>(i % (count + 1));
    case 192: return -static_cast<int>(i % (count + 1));
    default: return 0;
    }
}

extern "C" void rotatepoint(long xpivot, long ypivot, long x, long y, short daang, long* x2, long* y2)
{
    long dacos = sintable[(daang + 512) & 2047];
    long dasin = sintable[daang & 2047];
    x -= xpivot; y -= ypivot;
    if (x2) *x2 = dmulscale14(x, dacos, -y, dasin) + xpivot;
    if (y2) *y2 = dmulscale14(y, dacos, x, dasin) + ypivot;
}

extern "C" short lastwall(short point)
{
    if (point > 0 && wall[point - 1].point2 == point) return point - 1;
    short i = point;
    for (int cnt = MAXWALLS; cnt > 0; --cnt)
    {
        short j = wall[i].point2;
        if (j == point) return i;
        i = j;
    }
    return point;
}

extern "C" void dragpoint(short pointhighlight_, long dax, long day)
{
    if (pointhighlight_ < 0 || pointhighlight_ >= numwalls) return;
    wall[pointhighlight_].x = dax;
    wall[pointhighlight_].y = day;
    short p = pointhighlight_;
    for (int cnt = MAXWALLS; cnt > 0; --cnt)
    {
        if (wall[p].nextwall < 0) break;
        p = wall[wall[p].nextwall].point2;
        if (p == pointhighlight_) break;
        wall[p].x = dax; wall[p].y = day;
    }
}

extern "C" short sectorofwall(short theline)
{
    if (theline < 0 || theline >= numwalls) return -1;
    if (wall[theline].nextwall >= 0) return wall[wall[theline].nextwall].nextsector;
    for (short s = 0; s < numsectors; ++s)
        if (theline >= sector[s].wallptr && theline < sector[s].wallptr + sector[s].wallnum) return s;
    return -1;
}

extern "C" void alignceilslope(short dasect, long x, long y, long z)
{
    if (dasect < 0 || dasect >= numsectors) return;
    walltype& wal = wall[sector[dasect].wallptr];
    long dax = wall[wal.point2].x - wal.x;
    long day = wall[wal.point2].y - wal.y;
    long i = (y - wal.y) * dax - (x - wal.x) * day;
    if (i == 0) return;
    sector[dasect].ceilingheinum = static_cast<short>(scale((z - sector[dasect].ceilingz) << 8, ksqrt(dax * dax + day * day), i));
    if (sector[dasect].ceilingheinum == 0) sector[dasect].ceilingstat &= ~2; else sector[dasect].ceilingstat |= 2;
}
extern "C" void alignflorslope(short dasect, long x, long y, long z)
{
    if (dasect < 0 || dasect >= numsectors) return;
    walltype& wal = wall[sector[dasect].wallptr];
    long dax = wall[wal.point2].x - wal.x;
    long day = wall[wal.point2].y - wal.y;
    long i = (y - wal.y) * dax - (x - wal.x) * day;
    if (i == 0) return;
    sector[dasect].floorheinum = static_cast<short>(scale((z - sector[dasect].floorz) << 8, ksqrt(dax * dax + day * day), i));
    if (sector[dasect].floorheinum == 0) sector[dasect].floorstat &= ~2; else sector[dasect].floorstat |= 2;
}

extern "C" int clipinsidebox(long x, long y, short wallnum, long walldist)
{
    if (wallnum < 0 || wallnum >= numwalls) return 0;
    long r = (walldist << 1);
    walltype& w1 = wall[wallnum];
    walltype& w2 = wall[w1.point2];
    long x1 = w1.x + walldist - x, y1 = w1.y + walldist - y;
    long x2 = w2.x + walldist - x, y2 = w2.y + walldist - y;
    if ((x1 < 0 && x2 < 0) || (y1 < 0 && y2 < 0) || (x1 >= r && x2 >= r) || (y1 >= r && y2 >= r)) return 0;
    x2 -= x1; y2 -= y1;
    if (x2 * (walldist - y1) >= y2 * (walldist - x1))
    {
        if (x2 > 0) x2 *= (0 - y1); else x2 *= (r - y1);
        if (y2 > 0) y2 *= (r - x1); else y2 *= (0 - x1);
        return x2 < y2;
    }
    if (x2 > 0) x2 *= (r - y1); else x2 *= (0 - y1);
    if (y2 > 0) y2 *= (0 - x1); else y2 *= (r - x1);
    return (x2 >= y2) << 1;
}

extern "C" int clipinsideboxline(long x, long y, long x1, long y1, long x2, long y2, long walldist)
{
    long r = (walldist << 1);
    x1 += walldist - x; x2 += walldist - x;
    if ((x1 < 0 && x2 < 0) || (x1 >= r && x2 >= r)) return 0;
    y1 += walldist - y; y2 += walldist - y;
    if ((y1 < 0 && y2 < 0) || (y1 >= r && y2 >= r)) return 0;
    x2 -= x1; y2 -= y1;
    if (x2 * (walldist - y1) >= y2 * (walldist - x1))
    {
        if (x2 > 0) x2 *= (0 - y1); else x2 *= (r - y1);
        if (y2 > 0) y2 *= (r - x1); else y2 *= (0 - x1);
        return x2 < y2;
    }
    if (x2 > 0) x2 *= (r - y1); else x2 *= (0 - y1);
    if (y2 > 0) y2 *= (0 - x1); else y2 *= (r - x1);
    return (x2 >= y2) << 1;
}

extern "C" int lintersect(long x1, long y1, long z1, long x2, long y2, long z2, long x3, long y3, long x4, long y4, long* intx, long* inty, long* intz)
{
    // BUILD line-segment intersection. Both segments are half-open at their
    // far endpoint, matching the original engine and avoiding double hits on
    // shared wall points.
    const long x21 = x2 - x1, y21 = y2 - y1;
    const long x34 = x3 - x4, y34 = y3 - y4;
    const long bot = x21 * y34 - y21 * x34;
    if (bot == 0) return 0;

    const long x31 = x3 - x1, y31 = y3 - y1;
    const long topt = x31 * y34 - y31 * x34;
    const long topu = x21 * y31 - y21 * x31;

    if (bot > 0)
    {
        if ((topt < 0) || (topt >= bot) || (topu < 0) || (topu >= bot)) return 0;
    }
    else
    {
        if ((topt > 0) || (topt <= bot) || (topu > 0) || (topu <= bot)) return 0;
    }

    const long t = divscale24(topt, bot);
    if (intx) *intx = x1 + mulscale24(x21, t);
    if (inty) *inty = y1 + mulscale24(y21, t);
    if (intz) *intz = z1 + mulscale24(z2 - z1, t);
    return 1;
}

extern "C" int rintersect(long x1, long y1, long z1, long vx, long vy, long vz, long x3, long y3, long x4, long y4, long* intx, long* inty, long* intz)
{
    // BUILD ray-vs-wall intersection. The ray is open behind the origin and the
    // wall segment is half-open at the far endpoint.
    const long x34 = x3 - x4, y34 = y3 - y4;
    const long bot = vx * y34 - vy * x34;
    if (bot == 0) return 0;

    const long x31 = x3 - x1, y31 = y3 - y1;
    const long topt = x31 * y34 - y31 * x34;
    const long topu = vx * y31 - vy * x31;

    if (bot > 0)
    {
        if ((topt < 0) || (topu < 0) || (topu >= bot)) return 0;
    }
    else
    {
        if ((topt > 0) || (topu > 0) || (topu <= bot)) return 0;
    }

    const long t = divscale16(topt, bot);
    if (intx) *intx = x1 + mulscale16(vx, t);
    if (inty) *inty = y1 + mulscale16(vy, t);
    if (intz) *intz = z1 + mulscale16(vz, t);
    return 1;
}

extern "C" int cansee(long x1, long y1, long z1, short sect1, long x2, long y2, long z2, short sect2)
{
    if (sect1 < 0 || sect1 >= numsectors || sect2 < 0 || sect2 >= numsectors) return 0;
    if ((x1 == x2) && (y1 == y2)) return (sect1 == sect2);

    const long x21 = x2 - x1, y21 = y2 - y1, z21 = z2 - z1;

    clipsectorlist[0] = sect1;
    long danum = 1;
    for (long dacnt = 0; dacnt < danum && dacnt < MAXCLIPNUM; ++dacnt)
    {
        const short dasectnum = clipsectorlist[dacnt];
        if (dasectnum < 0 || dasectnum >= numsectors) continue;
        const sectortype* sec = &sector[dasectnum];

        for (long cnt = sec->wallnum, wi = sec->wallptr; cnt > 0; --cnt, ++wi)
        {
            if (wi < 0 || wi >= numwalls) continue;
            walltype* wal = &wall[wi];
            if (wal->point2 < 0 || wal->point2 >= numwalls) continue;
            walltype* wal2 = &wall[wal->point2];

            const long x31 = wal->x - x1;
            const long y31 = wal->y - y1;
            const long x34 = wal->x - wal2->x;
            const long y34 = wal->y - wal2->y;

            const long bot = y21 * x34 - x21 * y34;
            if (bot <= 0) continue;

            long t = y21 * x31 - x21 * y31;
            if (static_cast<unsigned long>(t) >= static_cast<unsigned long>(bot)) continue;
            t = y31 * x34 - x31 * y34;
            if (static_cast<unsigned long>(t) >= static_cast<unsigned long>(bot)) continue;

            const short nexts = wal->nextsector;
            if ((nexts < 0) || (nexts >= numsectors) || (wal->cstat & 32)) return 0;

            t = divscale24(t, bot);
            const long x = x1 + mulscale24(x21, t);
            const long y = y1 + mulscale24(y21, t);
            const long z = z1 + mulscale24(z21, t);

            long cz, fz;
            getzsofslope(dasectnum, x, y, &cz, &fz);
            if ((z <= cz) || (z >= fz)) return 0;
            getzsofslope(nexts, x, y, &cz, &fz);
            if ((z <= cz) || (z >= fz)) return 0;

            long i;
            for (i = danum - 1; i >= 0; --i)
                if (clipsectorlist[i] == nexts) break;
            if (i < 0 && danum < MAXCLIPNUM)
                clipsectorlist[danum++] = nexts;
        }
    }

    for (long i = danum - 1; i >= 0; --i)
        if (clipsectorlist[i] == sect2) return 1;
    return 0;
}

extern "C" int hitscan(long xs, long ys, long zs, short sectnum, long vx, long vy, long vz, short* hitsect, short* hitwall, short* hitsprite, long* hitx, long* hity, long* hitz, unsigned long cliptype)
{
    short outsect = -1, outwall = -1, outsprite = -1;
    long outx = hitscangoalx, outy = hitscangoaly, outz = zs;

    auto commitHit = [&](short hs, short hw, short hsp, long hx, long hy, long hz)
        {
            outsect = hs;
            outwall = hw;
            outsprite = hsp;
            outx = hx;
            outy = hy;
            outz = hz;
        };

    if (sectnum < 0 || sectnum >= numsectors)
    {
        if (hitsect) *hitsect = outsect;
        if (hitwall) *hitwall = outwall;
        if (hitsprite) *hitsprite = outsprite;
        if (hitx) *hitx = outx;
        if (hity) *hity = outy;
        if (hitz) *hitz = outz;
        return -1;
    }

    const long dawalclipmask = static_cast<long>(cliptype & 65535UL);
    const long dasprclipmask = static_cast<long>(cliptype >> 16);

    clipsectorlist[0] = sectnum;
    short tempshortcnt = 0, tempshortnum = 1;
    do
    {
        const short dasector = clipsectorlist[tempshortcnt];
        if (dasector < 0 || dasector >= numsectors) { ++tempshortcnt; continue; }
        sectortype* sec = &sector[dasector];

        long x1 = 0x7fffffff, y1 = 0, z1 = 0;
        if (sec->ceilingstat & 2)
        {
            walltype* wal = &wall[sec->wallptr];
            walltype* wal2 = &wall[wal->point2];
            long dax = wal2->x - wal->x;
            long day = wal2->y - wal->y;
            long i = ksqrt(dax * dax + day * day);
            if (i != 0)
            {
                i = divscale15(sec->ceilingheinum, i);
                dax *= i;
                day *= i;

                const long j = (vz << 8) - dmulscale15(dax, vy, -day, vx);
                if (j != 0)
                {
                    i = ((sec->ceilingz - zs) << 8) + dmulscale15(dax, ys - wal->y, -day, xs - wal->x);
                    if (((i ^ j) >= 0) && ((klabs(i) >> 1) < klabs(j)))
                    {
                        i = divscale30(i, j);
                        x1 = xs + mulscale30(vx, i);
                        y1 = ys + mulscale30(vy, i);
                        z1 = zs + mulscale30(vz, i);
                    }
                }
            }
        }
        else if ((vz < 0) && (zs >= sec->ceilingz))
        {
            z1 = sec->ceilingz;
            long i = z1 - zs;
            if ((klabs(i) >> 1) < -vz)
            {
                i = divscale30(i, vz);
                x1 = xs + mulscale30(vx, i);
                y1 = ys + mulscale30(vy, i);
            }
        }
        if ((x1 != 0x7fffffff) && (klabs(x1 - xs) + klabs(y1 - ys) < klabs(outx - xs) + klabs(outy - ys)))
            if (inside(x1, y1, dasector) != 0)
                commitHit(dasector, -1, -1, x1, y1, z1);

        x1 = 0x7fffffff;
        if (sec->floorstat & 2)
        {
            walltype* wal = &wall[sec->wallptr];
            walltype* wal2 = &wall[wal->point2];
            long dax = wal2->x - wal->x;
            long day = wal2->y - wal->y;
            long i = ksqrt(dax * dax + day * day);
            if (i != 0)
            {
                i = divscale15(sec->floorheinum, i);
                dax *= i;
                day *= i;

                const long j = (vz << 8) - dmulscale15(dax, vy, -day, vx);
                if (j != 0)
                {
                    i = ((sec->floorz - zs) << 8) + dmulscale15(dax, ys - wal->y, -day, xs - wal->x);
                    if (((i ^ j) >= 0) && ((klabs(i) >> 1) < klabs(j)))
                    {
                        i = divscale30(i, j);
                        x1 = xs + mulscale30(vx, i);
                        y1 = ys + mulscale30(vy, i);
                        z1 = zs + mulscale30(vz, i);
                    }
                }
            }
        }
        else if ((vz > 0) && (zs <= sec->floorz))
        {
            z1 = sec->floorz;
            long i = z1 - zs;
            if ((klabs(i) >> 1) < vz)
            {
                i = divscale30(i, vz);
                x1 = xs + mulscale30(vx, i);
                y1 = ys + mulscale30(vy, i);
            }
        }
        if ((x1 != 0x7fffffff) && (klabs(x1 - xs) + klabs(y1 - ys) < klabs(outx - xs) + klabs(outy - ys)))
            if (inside(x1, y1, dasector) != 0)
                commitHit(dasector, -1, -1, x1, y1, z1);

        const short startwall = sec->wallptr;
        const short endwall = static_cast<short>(startwall + sec->wallnum);
        for (short z = startwall; z < endwall; ++z)
        {
            walltype* wal = &wall[z];
            if (wal->point2 < 0 || wal->point2 >= numwalls) continue;
            walltype* wal2 = &wall[wal->point2];
            x1 = wal->x; y1 = wal->y;
            const long x2 = wal2->x, y2 = wal2->y;

            if ((x1 - xs) * (y2 - ys) < (x2 - xs) * (y1 - ys)) continue;

            long intx, inty, intz;
            if (rintersect(xs, ys, zs, vx, vy, vz, x1, y1, x2, y2, &intx, &inty, &intz) == 0) continue;
            if (klabs(intx - xs) + klabs(inty - ys) >= klabs(outx - xs) + klabs(outy - ys)) continue;

            const short nextsector = wal->nextsector;
            if ((nextsector < 0) || (nextsector >= numsectors) || (wal->cstat & dawalclipmask))
            {
                commitHit(dasector, z, -1, intx, inty, intz);
                continue;
            }

            long daz, daz2;
            getzsofslope(nextsector, intx, inty, &daz, &daz2);
            if ((intz <= daz) || (intz >= daz2))
            {
                commitHit(dasector, z, -1, intx, inty, intz);
                continue;
            }

            short zz;
            for (zz = tempshortnum - 1; zz >= 0; --zz)
                if (clipsectorlist[zz] == nextsector) break;
            if (zz < 0 && tempshortnum < MAXCLIPNUM)
                clipsectorlist[tempshortnum++] = nextsector;
        }

        for (short z = headspritesect[dasector]; z >= 0; z = nextspritesect[z])
        {
            spritetype* spr = &sprite[z];
            const long cstat = spr->cstat;
            if ((cstat & dasprclipmask) == 0) continue;
            if ((unsigned short)spr->picnum >= MAXTILES) continue;

            x1 = spr->x; y1 = spr->y; z1 = spr->z;
            switch (cstat & 48)
            {
            case 0:
            {
                long topt = vx * (x1 - xs) + vy * (y1 - ys);
                if (topt <= 0) continue;
                const long bot = vx * vx + vy * vy;
                if (bot == 0) continue;

                long intz = zs + scale(vz, topt, bot);

                long i = (tilesizy[spr->picnum] * spr->yrepeat) << 2;
                if (cstat & 128) z1 += (i >> 1);
                if (picanm[spr->picnum] & 0x00ff0000)
                    z1 -= (static_cast<long>(static_cast<signed char>((picanm[spr->picnum] >> 16) & 255)) * spr->yrepeat) << 2;
                if ((intz > z1) || (intz < z1 - i)) continue;

                const long topu = vx * (y1 - ys) - vy * (x1 - xs);
                const long offx = scale(vx, topu, bot);
                const long offy = scale(vy, topu, bot);
                const long dist = offx * offx + offy * offy;
                i = tilesizx[spr->picnum] * spr->xrepeat;
                i *= i;
                if (dist > (i >> 7)) continue;

                const long intx = xs + scale(vx, topt, bot);
                const long inty = ys + scale(vy, topt, bot);
                if (klabs(intx - xs) + klabs(inty - ys) > klabs(outx - xs) + klabs(outy - ys)) continue;
                commitHit(dasector, -1, z, intx, inty, intz);
                break;
            }
            case 16:
            {
                const short tilenum = spr->picnum;
                long xoff = static_cast<long>(static_cast<signed char>((picanm[tilenum] >> 8) & 255)) + spr->xoffset;
                if (cstat & 4) xoff = -xoff;
                long k = spr->ang;
                long l = spr->xrepeat;
                long dax = sintable[k & 2047] * l;
                long day = sintable[(k + 1536) & 2047] * l;
                l = tilesizx[tilenum];
                k = (l >> 1) + xoff;
                x1 -= mulscale16(dax, k);
                const long x2 = x1 + mulscale16(dax, l);
                y1 -= mulscale16(day, k);
                const long y2 = y1 + mulscale16(day, l);

                if ((cstat & 64) != 0)
                    if ((x1 - xs) * (y2 - ys) < (x2 - xs) * (y1 - ys)) continue;

                long intx, inty, intz;
                if (rintersect(xs, ys, zs, vx, vy, vz, x1, y1, x2, y2, &intx, &inty, &intz) == 0) continue;
                if (klabs(intx - xs) + klabs(inty - ys) > klabs(outx - xs) + klabs(outy - ys)) continue;

                k = (tilesizy[spr->picnum] * spr->yrepeat) << 2;
                long daz = (cstat & 128) ? spr->z + (k >> 1) : spr->z;
                if (picanm[spr->picnum] & 0x00ff0000)
                    daz -= (static_cast<long>(static_cast<signed char>((picanm[spr->picnum] >> 16) & 255)) * spr->yrepeat) << 2;
                if ((intz < daz) && (intz > daz - k))
                    commitHit(dasector, -1, z, intx, inty, intz);
                break;
            }
            case 32:
            {
                if (vz == 0) continue;
                long intz = z1;
                if (((intz - zs) ^ vz) < 0) continue;
                if ((cstat & 64) != 0)
                    if ((zs > intz) == ((cstat & 8) == 0)) continue;

                const long intx = xs + scale(intz - zs, vx, vz);
                const long inty = ys + scale(intz - zs, vy, vz);
                if (klabs(intx - xs) + klabs(inty - ys) > klabs(outx - xs) + klabs(outy - ys)) continue;

                const short tilenum = spr->picnum;
                long xoff = static_cast<long>(static_cast<signed char>((picanm[tilenum] >> 8) & 255)) + spr->xoffset;
                long yoff = static_cast<long>(static_cast<signed char>((picanm[tilenum] >> 16) & 255)) + spr->yoffset;
                if (cstat & 4) xoff = -xoff;
                if (cstat & 8) yoff = -yoff;

                const long ang = spr->ang;
                const long cosang = sintable[(ang + 512) & 2047];
                const long sinang = sintable[ang & 2047];
                const long xspan = tilesizx[tilenum], xrepeat = spr->xrepeat;
                const long yspan = tilesizy[tilenum], yrepeat = spr->yrepeat;

                long dax = ((xspan >> 1) + xoff) * xrepeat;
                long day = ((yspan >> 1) + yoff) * yrepeat;
                x1 += dmulscale16(sinang, dax, cosang, day) - intx;
                y1 += dmulscale16(sinang, day, -cosang, dax) - inty;
                long l = xspan * xrepeat;
                const long x2 = x1 - mulscale16(sinang, l);
                const long y2 = y1 + mulscale16(cosang, l);
                l = yspan * yrepeat;
                long k = -mulscale16(cosang, l);
                const long x3 = x2 + k;
                const long x4 = x1 + k;
                k = -mulscale16(sinang, l);
                const long y3 = y2 + k;
                const long y4 = y1 + k;

                char clipyou = 0;
                if ((y1 ^ y2) < 0)
                {
                    if ((x1 ^ x2) < 0) clipyou ^= (x1 * y2 < x2 * y1) ^ (y1 < y2);
                    else if (x1 >= 0) clipyou ^= 1;
                }
                if ((y2 ^ y3) < 0)
                {
                    if ((x2 ^ x3) < 0) clipyou ^= (x2 * y3 < x3 * y2) ^ (y2 < y3);
                    else if (x2 >= 0) clipyou ^= 1;
                }
                if ((y3 ^ y4) < 0)
                {
                    if ((x3 ^ x4) < 0) clipyou ^= (x3 * y4 < x4 * y3) ^ (y3 < y4);
                    else if (x3 >= 0) clipyou ^= 1;
                }
                if ((y4 ^ y1) < 0)
                {
                    if ((x4 ^ x1) < 0) clipyou ^= (x4 * y1 < x1 * y4) ^ (y4 < y1);
                    else if (x4 >= 0) clipyou ^= 1;
                }

                if (clipyou != 0)
                    commitHit(dasector, -1, z, intx, inty, intz);
                break;
            }
            }
        }

        ++tempshortcnt;
    } while (tempshortcnt < tempshortnum);

    if (hitsect) *hitsect = outsect;
    if (hitwall) *hitwall = outwall;
    if (hitsprite) *hitsprite = outsprite;
    if (hitx) *hitx = outx;
    if (hity) *hity = outy;
    if (hitz) *hitz = outz;
    return 0;
}

extern "C" void keepaway(long* x, long* y, long w);
extern "C" long raytrace(long x3, long y3, long* x4, long* y4);

extern "C" int clipmove(long* x, long* y, long* z, short* sectnum, long xvect, long yvect, long walldist, long ceildist, long flordist, unsigned long cliptype)
{
    if (!x || !y || !z || !sectnum) return 0;
    if (((xvect | yvect) == 0) || (*sectnum < 0) || (*sectnum >= numsectors)) return 0;

    long retval = 0;
    const long oxvect = xvect;
    const long oyvect = yvect;
    long goalx = (*x) + (xvect >> 14);
    long goaly = (*y) + (yvect >> 14);

    clipnum = 0;
    auto addclipline = [&](long dax1, long day1, long dax2, long day2, long daoval)
        {
            if (clipnum >= MAXCLIPNUM) return;
            clipit[clipnum].x1 = dax1;
            clipit[clipnum].y1 = day1;
            clipit[clipnum].x2 = dax2;
            clipit[clipnum].y2 = day2;
            clipobjectval[clipnum] = static_cast<short>(daoval);
            ++clipnum;
        };

    const long cx = (((*x) + goalx) >> 1);
    const long cy = (((*y) + goaly) >> 1);
    const long gx = goalx - (*x);
    const long gy = goaly - (*y);
    const long rad = ksqrt(gx * gx + gy * gy) + MAXCLIPDIST + walldist + 8;
    const long xmin = cx - rad, ymin = cy - rad;
    const long xmax = cx + rad, ymax = cy + rad;

    const long dawalclipmask = static_cast<long>(cliptype & 65535UL);
    const long dasprclipmask = static_cast<long>(cliptype >> 16);

    clipsectorlist[0] = *sectnum;
    long clipsectcnt = 0;
    clipsectnum = 1;
    do
    {
        const short dasect = clipsectorlist[clipsectcnt++];
        if (dasect < 0 || dasect >= numsectors) continue;
        sectortype* sec = &sector[dasect];
        const long startwall = sec->wallptr;
        const long endwall = startwall + sec->wallnum;

        for (long j = startwall; j < endwall; ++j)
        {
            walltype* wal = &wall[j];
            if (wal->point2 < 0 || wal->point2 >= numwalls) continue;
            walltype* wal2 = &wall[wal->point2];
            if ((wal->x < xmin) && (wal2->x < xmin)) continue;
            if ((wal->x > xmax) && (wal2->x > xmax)) continue;
            if ((wal->y < ymin) && (wal2->y < ymin)) continue;
            if ((wal->y > ymax) && (wal2->y > ymax)) continue;

            const long x1 = wal->x, y1 = wal->y, x2 = wal2->x, y2 = wal2->y;
            const long dx = x2 - x1, dy = y2 - y1;
            if (dx * ((*y) - y1) < ((*x) - x1) * dy) continue;

            long dax = (dx > 0) ? dx * (ymin - y1) : dx * (ymax - y1);
            long day = (dy > 0) ? dy * (xmax - x1) : dy * (xmin - x1);
            if (dax >= day) continue;

            bool clipyou = false;
            if ((wal->nextsector < 0) || (wal->nextsector >= numsectors) || (wal->cstat & dawalclipmask))
                clipyou = true;
            else if (editstatus == 0)
            {
                if (rintersect(*x, *y, 0, gx, gy, 0, x1, y1, x2, y2, &dax, &day, nullptr) == 0)
                    dax = *x, day = *y;

                long daz = getflorzofslope(dasect, dax, day);
                long daz2 = getflorzofslope(wal->nextsector, dax, day);
                sectortype* sec2 = &sector[wal->nextsector];
                if (daz2 < daz - (1 << 8))
                    if ((sec2->floorstat & 1) == 0)
                        if ((*z) >= daz2 - (flordist - 1)) clipyou = true;

                if (!clipyou)
                {
                    daz = getceilzofslope(dasect, dax, day);
                    daz2 = getceilzofslope(wal->nextsector, dax, day);
                    if (daz2 > daz + (1 << 8))
                        if ((sec2->ceilingstat & 1) == 0)
                            if ((*z) <= daz2 + (ceildist - 1)) clipyou = true;
                }
            }

            if (clipyou)
            {
                long bsz = walldist; if (gx < 0) bsz = -bsz;
                addclipline(x1 - bsz, y1 - bsz, x1 - bsz, y1 + bsz, j + 32768);
                addclipline(x2 - bsz, y2 - bsz, x2 - bsz, y2 + bsz, j + 32768);
                bsz = walldist; if (gy < 0) bsz = -bsz;
                addclipline(x1 + bsz, y1 - bsz, x1 - bsz, y1 - bsz, j + 32768);
                addclipline(x2 + bsz, y2 - bsz, x2 - bsz, y2 - bsz, j + 32768);

                dax = walldist; if (dy > 0) dax = -dax;
                day = walldist; if (dx < 0) day = -day;
                addclipline(x1 + dax, y1 + day, x2 + dax, y2 + day, j + 32768);
            }
            else
            {
                long i;
                for (i = clipsectnum - 1; i >= 0; --i)
                    if (wal->nextsector == clipsectorlist[i]) break;
                if (i < 0 && clipsectnum < MAXCLIPNUM)
                    clipsectorlist[clipsectnum++] = wal->nextsector;
            }
        }

        for (short j = headspritesect[dasect]; j >= 0; j = nextspritesect[j])
        {
            spritetype* spr = &sprite[j];
            const long cstat = spr->cstat;
            if ((cstat & dasprclipmask) == 0) continue;
            if ((unsigned short)spr->picnum >= MAXTILES) continue;

            long x1 = spr->x, y1 = spr->y, x2 = 0, y2 = 0;
            switch (cstat & 48)
            {
            case 0:
                if ((x1 >= xmin) && (x1 <= xmax) && (y1 >= ymin) && (y1 <= ymax))
                {
                    long k = (tilesizy[spr->picnum] * spr->yrepeat) << 2;
                    long daz = (cstat & 128) ? spr->z + (k >> 1) : spr->z;
                    if (picanm[spr->picnum] & 0x00ff0000)
                        daz -= (static_cast<long>(static_cast<signed char>((picanm[spr->picnum] >> 16) & 255)) * spr->yrepeat) << 2;
                    if (((*z) < daz + ceildist) && ((*z) > daz - k - flordist))
                    {
                        long bsz = (spr->clipdist << 2) + walldist; if (gx < 0) bsz = -bsz;
                        addclipline(x1 - bsz, y1 - bsz, x1 - bsz, y1 + bsz, j + 49152);
                        bsz = (spr->clipdist << 2) + walldist; if (gy < 0) bsz = -bsz;
                        addclipline(x1 + bsz, y1 - bsz, x1 - bsz, y1 - bsz, j + 49152);
                    }
                }
                break;

            case 16:
            {
                long k = (tilesizy[spr->picnum] * spr->yrepeat) << 2;
                long daz = (cstat & 128) ? spr->z + (k >> 1) : spr->z;
                if (picanm[spr->picnum] & 0x00ff0000)
                    daz -= (static_cast<long>(static_cast<signed char>((picanm[spr->picnum] >> 16) & 255)) * spr->yrepeat) << 2;
                long daz2 = daz - k;
                daz += ceildist; daz2 -= flordist;
                if (((*z) < daz) && ((*z) > daz2))
                {
                    const short tilenum = spr->picnum;
                    long xoff = static_cast<long>(static_cast<signed char>((picanm[tilenum] >> 8) & 255)) + spr->xoffset;
                    if (cstat & 4) xoff = -xoff;
                    k = spr->ang;
                    long l = spr->xrepeat;
                    long dax = sintable[k & 2047] * l;
                    long day = sintable[(k + 1536) & 2047] * l;
                    l = tilesizx[tilenum];
                    k = (l >> 1) + xoff;
                    x1 -= mulscale16(dax, k); x2 = x1 + mulscale16(dax, l);
                    y1 -= mulscale16(day, k); y2 = y1 + mulscale16(day, l);
                    if (clipinsideboxline(cx, cy, x1, y1, x2, y2, rad) != 0)
                    {
                        dax = mulscale14(sintable[(spr->ang + 256 + 512) & 2047], walldist);
                        day = mulscale14(sintable[(spr->ang + 256) & 2047], walldist);

                        if ((x1 - (*x)) * (y2 - (*y)) >= (x2 - (*x)) * (y1 - (*y)))
                            addclipline(x1 + dax, y1 + day, x2 + day, y2 - dax, j + 49152);
                        else
                        {
                            if ((cstat & 64) != 0) continue;
                            addclipline(x2 - dax, y2 - day, x1 - day, y1 + dax, j + 49152);
                        }

                        if ((x2 - x1) * ((*x) - x1) + (y2 - y1) * ((*y) - y1) < 0)
                            addclipline(x1 - day, y1 + dax, x1 + dax, y1 + day, j + 49152);
                        else if ((x1 - x2) * ((*x) - x2) + (y1 - y2) * ((*y) - y2) < 0)
                            addclipline(x2 + day, y2 - dax, x2 - dax, y2 - day, j + 49152);
                    }
                }
                break;
            }

            case 32:
            {
                long daz = spr->z + ceildist;
                long daz2 = spr->z - flordist;
                if (((*z) < daz) && ((*z) > daz2))
                {
                    if ((cstat & 64) != 0)
                        if (((*z) > spr->z) == ((cstat & 8) == 0)) continue;

                    const short tilenum = spr->picnum;
                    long xoff = static_cast<long>(static_cast<signed char>((picanm[tilenum] >> 8) & 255)) + spr->xoffset;
                    long yoff = static_cast<long>(static_cast<signed char>((picanm[tilenum] >> 16) & 255)) + spr->yoffset;
                    if (cstat & 4) xoff = -xoff;
                    if (cstat & 8) yoff = -yoff;

                    const long kangle = spr->ang;
                    const long cosang = sintable[(kangle + 512) & 2047];
                    const long sinang = sintable[kangle & 2047];
                    const long xspan = tilesizx[tilenum], xrepeat = spr->xrepeat;
                    const long yspan = tilesizy[tilenum], yrepeat = spr->yrepeat;

                    long dax = ((xspan >> 1) + xoff) * xrepeat;
                    long day = ((yspan >> 1) + yoff) * yrepeat;
                    long rxi0 = x1 + dmulscale16(sinang, dax, cosang, day);
                    long ryi0 = y1 + dmulscale16(sinang, day, -cosang, dax);
                    long l = xspan * xrepeat;
                    long rxi1 = rxi0 - mulscale16(sinang, l);
                    long ryi1 = ryi0 + mulscale16(cosang, l);
                    l = yspan * yrepeat;
                    long kk = -mulscale16(cosang, l);
                    long rxi2 = rxi1 + kk, rxi3 = rxi0 + kk;
                    kk = -mulscale16(sinang, l);
                    long ryi2 = ryi1 + kk, ryi3 = ryi0 + kk;

                    dax = mulscale14(sintable[(spr->ang - 256 + 512) & 2047], walldist);
                    day = mulscale14(sintable[(spr->ang - 256) & 2047], walldist);

                    if ((rxi0 - (*x)) * (ryi1 - (*y)) < (rxi1 - (*x)) * (ryi0 - (*y)))
                    {
                        if (clipinsideboxline(cx, cy, rxi1, ryi1, rxi0, ryi0, rad) != 0)
                            addclipline(rxi1 - day, ryi1 + dax, rxi0 + dax, ryi0 + day, j + 49152);
                    }
                    else if ((rxi2 - (*x)) * (ryi3 - (*y)) < (rxi3 - (*x)) * (ryi2 - (*y)))
                    {
                        if (clipinsideboxline(cx, cy, rxi3, ryi3, rxi2, ryi2, rad) != 0)
                            addclipline(rxi3 + day, ryi3 - dax, rxi2 - dax, ryi2 - day, j + 49152);
                    }

                    if ((rxi1 - (*x)) * (ryi2 - (*y)) < (rxi2 - (*x)) * (ryi1 - (*y)))
                    {
                        if (clipinsideboxline(cx, cy, rxi2, ryi2, rxi1, ryi1, rad) != 0)
                            addclipline(rxi2 - dax, ryi2 - day, rxi1 - day, ryi1 + dax, j + 49152);
                    }
                    else if ((rxi3 - (*x)) * (ryi0 - (*y)) < (rxi0 - (*x)) * (ryi3 - (*y)))
                    {
                        if (clipinsideboxline(cx, cy, rxi0, ryi0, rxi3, ryi3, rad) != 0)
                            addclipline(rxi0 + dax, ryi0 + day, rxi3 + day, ryi3 - dax, j + 49152);
                    }
                }
                break;
            }
            }
        }
    } while (clipsectcnt < clipsectnum);

    const long tracenum = min(max(clipmoveboxtracenum, 1L), 3L);
    long hitwall = 0;
    long cnt = tracenum;
    do
    {
        long intx = goalx, inty = goaly;
        hitwall = raytrace(*x, *y, &intx, &inty);
        if (hitwall >= 0)
        {
            const long lx = clipit[hitwall].x2 - clipit[hitwall].x1;
            const long ly = clipit[hitwall].y2 - clipit[hitwall].y1;
            const long len2 = lx * lx + ly * ly;
            if (len2 > 0)
            {
                const long dot = (goalx - intx) * lx + (goaly - inty) * ly;
                long i;
                if ((klabs(dot) >> 11) < len2) i = divscale20(dot, len2);
                else i = 0;
                goalx = mulscale20(lx, i) + intx;
                goaly = mulscale20(ly, i) + inty;
            }

            const long hitproj = dmulscale6(lx, oxvect, ly, oyvect);
            for (long i = cnt + 1; i <= tracenum; ++i)
            {
                const long j = hitwalls[i];
                const long oldproj = dmulscale6(clipit[j].x2 - clipit[j].x1, oxvect, clipit[j].y2 - clipit[j].y1, oyvect);
                if ((hitproj ^ oldproj) < 0)
                {
                    updatesector(*x, *y, sectnum);
                    return static_cast<int>(retval);
                }
            }

            keepaway(&goalx, &goaly, hitwall);
            xvect = ((goalx - intx) << 14);
            yvect = ((goaly - inty) << 14);

            if (cnt == tracenum) retval = clipobjectval[hitwall];
            hitwalls[cnt] = static_cast<short>(hitwall);
        }
        --cnt;
        *x = intx;
        *y = inty;
    } while (((xvect | yvect) != 0) && (hitwall >= 0) && (cnt > 0));

    for (long j = 0; j < clipsectnum; ++j)
        if (inside(*x, *y, clipsectorlist[j]) == 1)
        {
            *sectnum = clipsectorlist[j];
            return static_cast<int>(retval);
        }

    *sectnum = -1;
    long bestdist = 0x7fffffff;
    for (long j = numsectors - 1; j >= 0; --j)
        if (inside(*x, *y, static_cast<short>(j)) == 1)
        {
            long dist;
            if (sector[j].ceilingstat & 2) dist = getceilzofslope(static_cast<short>(j), *x, *y) - (*z);
            else dist = sector[j].ceilingz - (*z);

            if (dist > 0)
            {
                if (dist < bestdist) { *sectnum = static_cast<short>(j); bestdist = dist; }
            }
            else
            {
                if (sector[j].floorstat & 2) dist = (*z) - getflorzofslope(static_cast<short>(j), *x, *y);
                else dist = (*z) - sector[j].floorz;

                if (dist <= 0)
                {
                    *sectnum = static_cast<short>(j);
                    return static_cast<int>(retval);
                }
                if (dist < bestdist) { *sectnum = static_cast<short>(j); bestdist = dist; }
            }
        }

    return static_cast<int>(retval);
}

extern "C" void keepaway(long* x, long* y, long w)
{
    if (!x || !y || w < 0 || w >= clipnum) return;
    const long x1 = clipit[w].x1;
    const long y1 = clipit[w].y1;
    const long dx = clipit[w].x2 - x1;
    const long dy = clipit[w].y2 - y1;
    const long ox = ksgn(-dy);
    const long oy = ksgn(dx);
    char first = static_cast<char>(klabs(dx) <= klabs(dy));
    while (1)
    {
        if (dx * (*y - y1) > (*x - x1) * dy) return;
        if (first == 0) *x += ox; else *y += oy;
        first ^= 1;
    }
}

extern "C" long raytrace(long x3, long y3, long* x4, long* y4)
{
    if (!x4 || !y4) return -1;
    long hitwall = -1;
    for (long z = clipnum - 1; z >= 0; --z)
    {
        const long x1 = clipit[z].x1;
        const long y1 = clipit[z].y1;
        const long x2 = clipit[z].x2;
        const long y2 = clipit[z].y2;
        const long x21 = x2 - x1;
        const long y21 = y2 - y1;

        long topu = x21 * (y3 - y1) - (x3 - x1) * y21;
        if (topu <= 0) continue;
        if (x21 * (*y4 - y1) > (*x4 - x1) * y21) continue;
        const long x43 = *x4 - x3;
        const long y43 = *y4 - y3;
        if (x43 * (y1 - y3) > (x1 - x3) * y43) continue;
        if (x43 * (y2 - y3) <= (x2 - x3) * y43) continue;
        const long bot = x43 * y21 - x21 * y43;
        if (bot == 0) continue;

        long nintx = x3, ninty = y3;
        long cnt = 256;
        do
        {
            --cnt;
            if (cnt < 0)
            {
                *x4 = x3;
                *y4 = y3;
                return z;
            }
            nintx = x3 + scale(x43, topu, bot);
            ninty = y3 + scale(y43, topu, bot);
            --topu;
        } while (x21 * (ninty - y1) <= (nintx - x1) * y21);

        if (klabs(x3 - nintx) + klabs(y3 - ninty) < klabs(x3 - *x4) + klabs(y3 - *y4))
        {
            *x4 = nintx;
            *y4 = ninty;
            hitwall = z;
        }
    }
    return hitwall;
}

extern "C" int pushmove(long* x, long* y, long* z, short* sectnum, long walldist, long ceildist, long flordist, unsigned long cliptype)
{
    if (!x || !y || !z || !sectnum || (*sectnum < 0) || (*sectnum >= numsectors)) return -1;

    const long dawalclipmask = static_cast<long>(cliptype & 65535UL);
    const long dasprclipmask = static_cast<long>(cliptype >> 16);
    (void)dasprclipmask; // Face-sprite push was commented out in the original BUILD code.

    long k = 32;
    long dir = 1;
    long bad;
    do
    {
        bad = 0;
        clipsectorlist[0] = *sectnum;
        long clipsectcnt = 0;
        clipsectnum = 1;
        do
        {
            const short cursect = clipsectorlist[clipsectcnt];
            if (cursect < 0 || cursect >= numsectors) { ++clipsectcnt; continue; }
            sectortype* sec = &sector[cursect];

            long startwall, endwall;
            if (dir > 0)
            {
                startwall = sec->wallptr;
                endwall = startwall + sec->wallnum;
            }
            else
            {
                startwall = sec->wallptr + sec->wallnum - 1;
                endwall = sec->wallptr - 1;
            }

            for (long i = startwall; i != endwall; i += dir)
            {
                walltype* wal = &wall[i];
                if (clipinsidebox(*x, *y, static_cast<short>(i), walldist - 4) != 1) continue;

                long j = 0;
                if (wal->nextsector < 0 || wal->nextsector >= numsectors) j = 1;
                if (wal->cstat & dawalclipmask) j = 1;
                if (j == 0)
                {
                    sectortype* sec2 = &sector[wal->nextsector];

                    long dax = wall[wal->point2].x - wal->x;
                    long day = wall[wal->point2].y - wal->y;
                    long daz = dax * ((*x) - wal->x) + day * ((*y) - wal->y);
                    long t;
                    if (daz <= 0) t = 0;
                    else
                    {
                        long daz2 = dax * dax + day * day;
                        if (daz >= daz2) t = (1 << 30); else t = divscale30(daz, daz2);
                    }
                    dax = wal->x + mulscale30(dax, t);
                    day = wal->y + mulscale30(day, t);

                    daz = getflorzofslope(cursect, dax, day);
                    long daz2 = getflorzofslope(wal->nextsector, dax, day);
                    if ((daz2 < daz - (1 << 8)) && ((sec2->floorstat & 1) == 0))
                        if (*z >= daz2 - (flordist - 1)) j = 1;

                    daz = getceilzofslope(cursect, dax, day);
                    daz2 = getceilzofslope(wal->nextsector, dax, day);
                    if ((daz2 > daz + (1 << 8)) && ((sec2->ceilingstat & 1) == 0))
                        if (*z <= daz2 + (ceildist - 1)) j = 1;
                }

                if (j != 0)
                {
                    j = getangle(wall[wal->point2].x - wal->x, wall[wal->point2].y - wal->y);
                    const long dx = sintable[(j + 1024) & 2047] >> 11;
                    const long dy = sintable[(j + 512) & 2047] >> 11;
                    char bad2 = 16;
                    do
                    {
                        *x += dx;
                        *y += dy;
                        --bad2;
                        if (bad2 == 0) break;
                    } while (clipinsidebox(*x, *y, static_cast<short>(i), walldist - 4) != 0);
                    bad = -1;
                    --k;
                    if (k <= 0) return static_cast<int>(bad);
                    updatesector(*x, *y, sectnum);
                }
                else
                {
                    for (j = clipsectnum - 1; j >= 0; --j)
                        if (wal->nextsector == clipsectorlist[j]) break;
                    if (j < 0 && clipsectnum < MAXCLIPNUM)
                        clipsectorlist[clipsectnum++] = wal->nextsector;
                }
            }
            ++clipsectcnt;
        } while (clipsectcnt < clipsectnum);
        dir = -dir;
    } while (bad != 0);

    return static_cast<int>(bad);
}

extern "C" int neartag(long xs, long ys, long zs, short sectnum, short ange, short* neartagsector, short* neartagwall, short* neartagsprite, long* neartaghitdist, long neartagrange, char tagsearch)
{
    if (neartagsector) *neartagsector = -1;
    if (neartagwall) *neartagwall = -1;
    if (neartagsprite) *neartagsprite = -1;
    if (neartaghitdist) *neartaghitdist = 0;
    if (sectnum < 0) return 0;

    // Keep the compatibility shim simple: use the now-correct hitscan path for
    // near-tag selection. The collision-critical behavior is in hitscan/clipmove.
    const long vx = mulscale14(sintable[(ange + 512) & 2047], neartagrange);
    const long vy = mulscale14(sintable[ange & 2047], neartagrange);
    short hs, hw, hsp;
    long hx, hy, hz;
    hitscan(xs, ys, zs, sectnum, vx, vy, 0, &hs, &hw, &hsp, &hx, &hy, &hz, 0xffffffffUL);
    if (hs >= 0)
    {
        if (neartagsector) *neartagsector = hs;
        if (neartagwall) *neartagwall = hw;
        if (neartagsprite) *neartagsprite = hsp;
        if (neartaghitdist) *neartaghitdist = klabs(hx - xs) + klabs(hy - ys);
    }
    (void)tagsearch;
    return 0;
}

extern "C" void getzrange(long x, long y, long z, short sectnum, long* ceilz, long* ceilhit, long* florz, long* florhit, long walldist, unsigned long cliptype)
{
    long outceilz = 0x80000000;
    long outceilhit = -1;
    long outflorz = 0x7fffffff;
    long outflorhit = -1;

    auto writeOut = [&]()
        {
            if (ceilz) *ceilz = outceilz;
            if (ceilhit) *ceilhit = outceilhit;
            if (florz) *florz = outflorz;
            if (florhit) *florhit = outflorhit;
        };

    if (sectnum < 0 || sectnum >= numsectors)
    {
        writeOut();
        return;
    }

    const long extra = walldist + MAXCLIPDIST + 1;
    const long xmin = x - extra, ymin = y - extra;
    const long xmax = x + extra, ymax = y + extra;

    getzsofslope(sectnum, x, y, &outceilz, &outflorz);
    outceilhit = sectnum + 16384;
    outflorhit = sectnum + 16384;

    const long dawalclipmask = static_cast<long>(cliptype & 65535UL);
    const long dasprclipmask = static_cast<long>(cliptype >> 16);

    clipsectorlist[0] = sectnum;
    long clipsectcnt = 0;
    clipsectnum = 1;

    do
    {
        const short cursect = clipsectorlist[clipsectcnt];
        if (cursect < 0 || cursect >= numsectors) { ++clipsectcnt; continue; }
        sectortype* sec = &sector[cursect];
        const long startwall = sec->wallptr;
        const long endwall = startwall + sec->wallnum;
        for (long j = startwall; j < endwall; ++j)
        {
            walltype* wal = &wall[j];
            const short k = wal->nextsector;
            if (k < 0 || k >= numsectors) continue;
            walltype* wal2 = &wall[wal->point2];
            long x1 = wal->x, x2 = wal2->x;
            if ((x1 < xmin) && (x2 < xmin)) continue;
            if ((x1 > xmax) && (x2 > xmax)) continue;
            long y1 = wal->y, y2 = wal2->y;
            if ((y1 < ymin) && (y2 < ymin)) continue;
            if ((y1 > ymax) && (y2 > ymax)) continue;

            const long dx = x2 - x1, dy = y2 - y1;
            if (dx * (y - y1) < (x - x1) * dy) continue;
            long dax = (dx > 0) ? dx * (ymin - y1) : dx * (ymax - y1);
            long day = (dy > 0) ? dy * (xmax - x1) : dy * (xmin - x1);
            if (dax >= day) continue;

            if (wal->cstat & dawalclipmask) continue;
            sec = &sector[k];
            if (editstatus == 0)
            {
                if (((sec->ceilingstat & 1) == 0) && (z <= sec->ceilingz + (3 << 8))) continue;
                if (((sec->floorstat & 1) == 0) && (z >= sec->floorz - (3 << 8))) continue;
            }

            long i;
            for (i = clipsectnum - 1; i >= 0; --i)
                if (clipsectorlist[i] == k) break;
            if (i < 0 && clipsectnum < MAXCLIPNUM)
                clipsectorlist[clipsectnum++] = k;

            if ((x1 < xmin + MAXCLIPDIST) && (x2 < xmin + MAXCLIPDIST)) continue;
            if ((x1 > xmax - MAXCLIPDIST) && (x2 > xmax - MAXCLIPDIST)) continue;
            if ((y1 < ymin + MAXCLIPDIST) && (y2 < ymin + MAXCLIPDIST)) continue;
            if ((y1 > ymax - MAXCLIPDIST) && (y2 > ymax - MAXCLIPDIST)) continue;
            if (dx > 0) dax += dx * MAXCLIPDIST; else dax -= dx * MAXCLIPDIST;
            if (dy > 0) day -= dy * MAXCLIPDIST; else day += dy * MAXCLIPDIST;
            if (dax >= day) continue;

            long daz, daz2;
            getzsofslope(k, x, y, &daz, &daz2);
            if (daz > outceilz) { outceilz = daz; outceilhit = k + 16384; }
            if (daz2 < outflorz) { outflorz = daz2; outflorhit = k + 16384; }
        }
        ++clipsectcnt;
    } while (clipsectcnt < clipsectnum);

    for (long i = 0; i < clipsectnum; ++i)
    {
        const short cursect = clipsectorlist[i];
        if (cursect < 0 || cursect >= numsectors) continue;
        for (short j = headspritesect[cursect]; j >= 0; j = nextspritesect[j])
        {
            spritetype* spr = &sprite[j];
            const short cstat = spr->cstat;
            if ((cstat & dasprclipmask) == 0) continue;
            if ((unsigned short)spr->picnum >= MAXTILES) continue;

            long x1 = spr->x, y1 = spr->y;
            long daz = 0, daz2 = 0;
            char clipyou = 0;
            switch (cstat & 48)
            {
            case 0:
            {
                long k = walldist + (spr->clipdist << 2) + 1;
                if ((klabs(x1 - x) <= k) && (klabs(y1 - y) <= k))
                {
                    daz = spr->z;
                    k = (tilesizy[spr->picnum] * spr->yrepeat) << 1;
                    if (cstat & 128) daz += k;
                    if (picanm[spr->picnum] & 0x00ff0000)
                        daz -= (static_cast<long>(static_cast<signed char>((picanm[spr->picnum] >> 16) & 255)) * spr->yrepeat) << 2;
                    daz2 = daz - (k << 1);
                    clipyou = 1;
                }
                break;
            }
            case 16:
            {
                const short tilenum = spr->picnum;
                long xoff = static_cast<long>(static_cast<signed char>((picanm[tilenum] >> 8) & 255)) + spr->xoffset;
                if (cstat & 4) xoff = -xoff;
                long k = spr->ang;
                long l = spr->xrepeat;
                long dax = sintable[k & 2047] * l;
                long day = sintable[(k + 1536) & 2047] * l;
                l = tilesizx[tilenum];
                k = (l >> 1) + xoff;
                x1 -= mulscale16(dax, k);
                const long x2 = x1 + mulscale16(dax, l);
                y1 -= mulscale16(day, k);
                const long y2 = y1 + mulscale16(day, l);
                if (clipinsideboxline(x, y, x1, y1, x2, y2, walldist + 1) != 0)
                {
                    daz = spr->z;
                    k = (tilesizy[spr->picnum] * spr->yrepeat) << 1;
                    if (cstat & 128) daz += k;
                    if (picanm[spr->picnum] & 0x00ff0000)
                        daz -= (static_cast<long>(static_cast<signed char>((picanm[spr->picnum] >> 16) & 255)) * spr->yrepeat) << 2;
                    daz2 = daz - (k << 1);
                    clipyou = 1;
                }
                break;
            }
            case 32:
            {
                daz = spr->z;
                daz2 = daz;
                if ((cstat & 64) != 0)
                    if ((z > daz) == ((cstat & 8) == 0)) continue;

                const short tilenum = spr->picnum;
                long xoff = static_cast<long>(static_cast<signed char>((picanm[tilenum] >> 8) & 255)) + spr->xoffset;
                long yoff = static_cast<long>(static_cast<signed char>((picanm[tilenum] >> 16) & 255)) + spr->yoffset;
                if (cstat & 4) xoff = -xoff;
                if (cstat & 8) yoff = -yoff;

                const long ang = spr->ang;
                const long cosang = sintable[(ang + 512) & 2047];
                const long sinang = sintable[ang & 2047];
                const long xspan = tilesizx[tilenum], xrepeat = spr->xrepeat;
                const long yspan = tilesizy[tilenum], yrepeat = spr->yrepeat;

                long dax = ((xspan >> 1) + xoff) * xrepeat;
                long day = ((yspan >> 1) + yoff) * yrepeat;
                x1 += dmulscale16(sinang, dax, cosang, day) - x;
                y1 += dmulscale16(sinang, day, -cosang, dax) - y;
                long l = xspan * xrepeat;
                long x2 = x1 - mulscale16(sinang, l);
                long y2 = y1 + mulscale16(cosang, l);
                l = yspan * yrepeat;
                long k = -mulscale16(cosang, l); long x3 = x2 + k, x4 = x1 + k;
                k = -mulscale16(sinang, l); long y3 = y2 + k, y4 = y1 + k;

                dax = mulscale14(sintable[(spr->ang - 256 + 512) & 2047], walldist + 4);
                day = mulscale14(sintable[(spr->ang - 256) & 2047], walldist + 4);
                x1 += dax; x2 -= day; x3 -= dax; x4 += day;
                y1 += day; y2 += dax; y3 -= day; y4 -= dax;

                if ((y1 ^ y2) < 0)
                {
                    if ((x1 ^ x2) < 0) clipyou ^= (x1 * y2 < x2 * y1) ^ (y1 < y2);
                    else if (x1 >= 0) clipyou ^= 1;
                }
                if ((y2 ^ y3) < 0)
                {
                    if ((x2 ^ x3) < 0) clipyou ^= (x2 * y3 < x3 * y2) ^ (y2 < y3);
                    else if (x2 >= 0) clipyou ^= 1;
                }
                if ((y3 ^ y4) < 0)
                {
                    if ((x3 ^ x4) < 0) clipyou ^= (x3 * y4 < x4 * y3) ^ (y3 < y4);
                    else if (x3 >= 0) clipyou ^= 1;
                }
                if ((y4 ^ y1) < 0)
                {
                    if ((x4 ^ x1) < 0) clipyou ^= (x4 * y1 < x1 * y4) ^ (y4 < y1);
                    else if (x4 >= 0) clipyou ^= 1;
                }
                break;
            }
            }

            if (clipyou != 0)
            {
                if ((z > daz) && (daz > outceilz)) { outceilz = daz; outceilhit = j + 49152; }
                if ((z < daz2) && (daz2 < outflorz)) { outflorz = daz2; outflorhit = j + 49152; }
            }
        }
    }

    writeOut();
}


extern "C" short nextsectorneighborz(short sectnum, long thez, short topbottom, short direction)
{
    if (sectnum < 0 || sectnum >= numsectors) return -1;
    long best = direction ? 0x7fffffff : static_cast<long>(0x80000000);
    short bests = -1;
    int start = sector[sectnum].wallptr, end = start + sector[sectnum].wallnum;
    for (int wi = start; wi < end; ++wi)
    {
        short ns = wall[wi].nextsector;
        if (ns < 0) continue;
        long z = topbottom ? sector[ns].floorz : sector[ns].ceilingz;
        if (direction ? (z > thez && z < best) : (z < thez && z > best))
        {
            best = z; bests = ns;
        }
    }
    return bests;
}

// -----------------------------------------------------------------------------
// 2D/HUD/text compatibility.
// -----------------------------------------------------------------------------
extern "C" void setview(long x1, long y1, long x2, long y2)
{
    windowx1 = max(0L, min(x1, xdim - 1));
    windowy1 = max(0L, min(y1, ydim - 1));
    windowx2 = max(windowx1, min(x2, xdim - 1));
    windowy2 = max(windowy1, min(y2, ydim - 1));
    xdimen = windowx2 - windowx1 + 1;
    ydimen = windowy2 - windowy1 + 1;
    halfxdimen = xdimen >> 1;
    xdimenrecip = divscale32(1, max(1L, xdimen));
    wx1 = windowx1 << 12; wy1 = windowy1 << 12;
    wx2 = (windowx2 + 1) << 12; wy2 = (windowy2 + 1) << 12;
    for (long i = 0; i < xdim && i < MAXXDIM; ++i)
    {
        if (i < windowx1 || i > windowx2) { startumost[i] = 1; startdmost[i] = 0; }
        else { startumost[i] = static_cast<short>(windowy1); startdmost[i] = static_cast<short>(windowy2 + 1); }
    }
    setaspect(65536L, divscale16(ydim * 320L, max(1L, xdim * 200L)));
}

extern "C" void setaspect(long daxrange, long daaspect)
{
    viewingrange = max(1L, daxrange);
    viewingrangerecip = divscale32(1, viewingrange);
    yxaspect = max(1L, daaspect);
    xyaspect = divscale32(1, yxaspect);
    xdimenscale = scale(xdimen, yxaspect, 320);
    xdimscale = scale(320, xyaspect, max(1L, xdimen));
}
extern "C" void dosetaspect() {}

extern "C" void setactivepage(long dapage) { activepage = dapage; }
extern "C" void setvisualpage(long dapage) { visualpage = dapage; }
extern "C" int setvesa(long daxdim, long daydim) { return setgamemode(1, daxdim, daydim); }
extern "C" void uninitvesa() {}
extern "C" void setvmode(int) {}
extern "C" void limitrate() { faketimerhandler(); }
extern "C" void getvalidvesamodes() { validmodecnt = 1; }
extern "C" unsigned char readpixel(long) { return 0; }
extern "C" void drawpixel(long, unsigned char) {}
extern "C" void koutp(long, long) {}
extern "C" void koutpw(long, long) {}
extern "C" long kinp(long) { return 0; }
extern "C" long kinpw(long) { return 0; }

extern "C" void clearview(long dacol)
{
    if (!buildgl::g.ready) return;
    unsigned char c = static_cast<unsigned char>(dacol & 255);
    glClearColor((palette[c * 3 + 0] << 2) / 255.0f, (palette[c * 3 + 1] << 2) / 255.0f, (palette[c * 3 + 2] << 2) / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
extern "C" void clearallviews(long dacol) { clearview(dacol); }

extern "C" void plotpixel(long x, long y, char col)
{
    buildgl::begin2D();
    glDisable(GL_TEXTURE_2D);
    unsigned char c = static_cast<unsigned char>(col);
    glColor3ub(palette[c * 3 + 0] << 2, palette[c * 3 + 1] << 2, palette[c * 3 + 2] << 2);
    glBegin(GL_POINTS); glVertex2f(static_cast<float>(x), static_cast<float>(y)); glEnd();
    glEnable(GL_TEXTURE_2D);
    buildgl::end2D();
}
extern "C" char getpixel(long, long) { return 0; }

extern "C" void drawline256(long x1, long y1, long x2, long y2, char col)
{
    buildgl::draw2DLine(x1 >> 12, y1 >> 12, x2 >> 12, y2 >> 12, static_cast<unsigned char>(col));
}
extern "C" void drawline16(long x1, long y1, long x2, long y2, char col)
{
    buildgl::draw2DLine(x1, y1, x2, y2, static_cast<unsigned char>(col));
}
extern "C" long readpixel16(long) { return 0; }
extern "C" int screencapture(char*, char) { return -1; }

extern "C" void rotatesprite(long sx, long sy, long z, short a, short picnum, signed char dashade, char dapalnum, char dastat, long cx1, long cy1, long cx2, long cy2)
{
    (void)dapalnum;
    dorotatesprite(sx, sy, z, a, picnum, dashade, dapalnum, dastat, cx1, cy1, cx2, cy2);
}

extern "C" void dorotatesprite(long sx, long sy, long z, short a, short picnum,
    signed char dashade, char dapalnum, char dastat,
    long cx1, long cy1, long cx2, long cy2)
{
    if ((unsigned short)picnum >= MAXTILES || z <= 16) return;

    if (picanm[picnum] & 192)
        picnum = static_cast<short>(picnum + animateoffs(picnum, static_cast<short>(0xc000)));

    if ((unsigned short)picnum >= MAXTILES) return;
    if (tilesizx[picnum] <= 0 || tilesizy[picnum] <= 0) return;
    if (!buildgl::bindTilePal(picnum, static_cast<unsigned char>(dapalnum), false)) return;

    const long xsiz = tilesizx[picnum];
    const long ysiz = tilesizy[picnum];

    long xoff = 0;
    long yoff = 0;

    // Build does not simply center every rotatesprite tile.
    // It uses picanm x/y offsets unless dastat&16 says sx/sy is top-left.
    if ((dastat & 16) == 0)
    {
        xoff = static_cast<long>(static_cast<signed char>((picanm[picnum] >> 8) & 255)) + (xsiz >> 1);
        yoff = static_cast<long>(static_cast<signed char>((picanm[picnum] >> 16) & 255)) + (ysiz >> 1);
    }

    // Build y-flip changes the anchor too, not just the texture coordinates.
    if (dastat & 4)
        yoff = ysiz - yoff;

    long cosang = sintable[(a + 512) & 2047];
    long sinang = sintable[a & 2047];

    // Build auto-scale mode.
    // This is the part that fixes crosshair movement with + / - screen size.
    if (dastat & 2)
    {
        long xscale;

        if ((dastat & 8) == 0)
        {
            // Clip/window-relative mode: center around cx/cy, not full screen.
            xscale = xdimenscale;
            if (stereomode)
                xscale = scale(windowx2 - windowx1 + 1, yxaspect, 320);

            sx = ((cx1 + cx2 + 2) << 15) + scale(sx - (320 << 15), xdimen, 320);
            sy = ((cy1 + cy2 + 2) << 15) + mulscale16(sy - (200 << 15), xscale);
        }
        else
        {
            // Full-screen autoscale mode.
            xscale = scale(xdim, yxaspect, 320);
            sx = (xdim << 15) + 32768 + scale(sx - (320 << 15), xdim, 320);
            sy = (ydim << 15) + 32768 + mulscale16(sy - (200 << 15), xscale);
        }

        z = mulscale16(z, xscale);
    }

    long xv = mulscale14(cosang, z);
    long yv = mulscale14(sinang, z);
    long xv2 = xv;
    long yv2 = yv;

    // Match Build's aspect correction for autoscaled or view-clipped sprites.
    if ((dastat & 2) || ((dastat & 8) == 0))
    {
        xv2 = mulscale16(xv, xyaspect);
        yv2 = mulscale16(yv, xyaspect);
    }

    long rx[4], ry[4];

    rx[0] = sx - (xv2 * xoff - yv2 * yoff);
    ry[0] = sy - (yv * xoff + xv * yoff);

    rx[1] = rx[0] + xv2 * xsiz;
    ry[1] = ry[0] + yv * xsiz;

    rx[3] = rx[0] - yv2 * ysiz;
    ry[3] = ry[0] + xv * ysiz;

    rx[2] = rx[1] + rx[3] - rx[0];
    ry[2] = ry[1] + ry[3] - ry[0];

    auto fp16 = [](long v) -> float
        {
            return static_cast<float>(v) / 65536.0f;
        };

    buildgl::begin2D();

    // Clip to the cx/cy rectangle. This is not a full startumost/startdmost
    // clone, but it fixes most HUD/crosshair clipping behavior.
    const bool doScissor = (cx1 <= cx2 && cy1 <= cy2);
    if (doScissor)
    {
        const int scx0 = static_cast<int>(max(0L, min(cx1, xdim - 1)));
        const int scy0 = static_cast<int>(max(0L, min(cy1, ydim - 1)));
        const int scx1 = static_cast<int>(max(0L, min(cx2, xdim - 1)));
        const int scy1 = static_cast<int>(max(0L, min(cy2, ydim - 1)));

        const int scw = max(0, scx1 - scx0 + 1);
        const int sch = max(0, scy1 - scy0 + 1);

        glEnable(GL_SCISSOR_TEST);
        glScissor(scx0, max(0, buildgl::g.h - (scy1 + 1)), scw, sch);
    }

    if (dastat & 64)
        glDisable(GL_ALPHA_TEST);     // non-masked
    else
        glEnable(GL_ALPHA_TEST);      // masked, palette index 255 transparent

    if (dastat & 1)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
        glDisable(GL_BLEND);
    }

    const float shade = buildgl::shadeFactor(dashade);
    glColor4f(shade, shade, shade, (dastat & 1) ? 0.65f : 1.0f);

    const bool yflip = (dastat & 4) != 0;
    const float u0 = 0.0f, u1 = 1.0f;
    const float v0 = yflip ? 1.0f : 0.0f;
    const float v1 = yflip ? 0.0f : 1.0f;

    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0); glVertex2f(fp16(rx[0]), fp16(ry[0]));
    glTexCoord2f(u1, v0); glVertex2f(fp16(rx[1]), fp16(ry[1]));
    glTexCoord2f(u1, v1); glVertex2f(fp16(rx[2]), fp16(ry[2]));
    glTexCoord2f(u0, v1); glVertex2f(fp16(rx[3]), fp16(ry[3]));
    glEnd();

    if (doScissor)
        glDisable(GL_SCISSOR_TEST);

    if (dastat & 1)
        glDisable(GL_BLEND);

    buildgl::end2D();
}

extern "C" void printext256(long xpos, long ypos, short col, short backcol, char name[82], char fontsize)
{
    (void)backcol;
    if (!name) return;
    buildgl::begin2D();
    glDisable(GL_TEXTURE_2D);
    unsigned char c = static_cast<unsigned char>(col);
    glColor3ub(palette[c * 3 + 0] << 2, palette[c * 3 + 1] << 2, palette[c * 3 + 2] << 2);
    float x = static_cast<float>(xpos);
    float y = static_cast<float>(ypos);
    float cw = fontsize ? 4.0f : 8.0f;
    for (int i = 0; name[i]; ++i)
    {
        unsigned char ch = static_cast<unsigned char>(name[i]);
        unsigned char* glyph = reinterpret_cast<unsigned char*>((fontsize ? smalltextfont : textfont) + ch * 8);
        for (int gy = 0; gy < 8; ++gy)
        {
            unsigned char bits = glyph[gy];
            for (int gx = 0; gx < static_cast<int>(cw); ++gx)
            {
                if (bits & pow2char[7 - gx])
                {
                    glBegin(GL_QUADS);
                    glVertex2f(x + gx, y + gy); glVertex2f(x + gx + 1, y + gy);
                    glVertex2f(x + gx + 1, y + gy + 1); glVertex2f(x + gx, y + gy + 1);
                    glEnd();
                }
            }
        }
        x += cw;
    }
    glEnable(GL_TEXTURE_2D);
    buildgl::end2D();
}

extern "C" void printext16(long xpos, long ypos, short col, short backcol, char name[82], char fontsize)
{
    printext256(xpos, ypos, col, backcol, name, fontsize);
}

extern "C" void clear2dscreen() { clearallviews(0); }
extern "C" void draw2dgrid(long, long, short, long, short) {}
extern "C" void draw2dscreen(long posxe, long posye, short ange, long zoome, short gride)
{
    (void)gride;

    const long cx = buildgl::g.w >> 1;
    const long cy = buildgl::g.h >> 1;

    bool useShow2DWallMask = false;
    if (editstatus == 0 && automapping == 0)
    {
        const int nbytes = (numwalls + 7) >> 3;
        for (int i = 0; i < nbytes; ++i)
        {
            if (show2dwall[i] != 0)
            {
                useShow2DWallMask = true;
                break;
            }
        }
    }

    for (int i = 0; i < numwalls; ++i)
    {
        walltype& w = wall[i];
        if (w.point2 < 0 || w.point2 >= numwalls)
            continue;

        // Avoid drawing both halves of a two-sided portal line. This keeps the
        // map from looking like duplicate phantom walls while preserving editor
        // visibility when show2dwall[] has not been populated yet.
        if (w.nextwall >= 0 && w.nextwall < numwalls && i > w.nextwall)
            continue;

        if (useShow2DWallMask && (show2dwall[i >> 3] & pow2char[i & 7]) == 0)
            continue;

        walltype& w2 = wall[w.point2];

        const long dx1 = mulscale14(w.x - posxe, zoome);
        const long dy1 = mulscale14(w.y - posye, zoome);
        const long dx2 = mulscale14(w2.x - posxe, zoome);
        const long dy2 = mulscale14(w2.y - posye, zoome);

        // The old BUILD automap math is effectively in a 320x200 virtual
        // screen space. The GL line renderer uses real client pixels, so scale
        // only these map deltas. This keeps the 3D viewport/projection untouched.
        long x1 = cx + buildgl::scaleAutomapDeltaX(dx1);
        long y1 = cy + buildgl::scaleAutomapDeltaY(dy1);
        long x2 = cx + buildgl::scaleAutomapDeltaX(dx2);
        long y2 = cy + buildgl::scaleAutomapDeltaY(dy2);

        drawline16(x1, y1, x2, y2, w.nextsector >= 0 ? 4 : 7);
    }

    long ax = cx + buildgl::scaleAutomapDeltaX(sintable[(ange + 512) & 2047] >> 7);
    long ay = cy + buildgl::scaleAutomapDeltaY(sintable[ange & 2047] >> 7);
    drawline16(cx, cy, ax, ay, 15);
}


extern "C" void drawmapview(long dax, long day, long zoome, short ang)
{
    draw2dscreen(dax, day, ang, zoome, 0);
}

extern "C" void qsetmode640350() { qsetmode = 350; }
extern "C" void qsetmode640480() { qsetmode = 480; }

// -----------------------------------------------------------------------------
// Tile/palette utility functions.
// -----------------------------------------------------------------------------
extern "C" void copytilepiece(long tilenume1, long sx1, long sy1, long xsiz, long ysiz, long tilenume2, long sx2, long sy2)
{
    if ((unsigned long)tilenume1 >= MAXTILES || (unsigned long)tilenume2 >= MAXTILES) return;
    if (!waloff[tilenume1]) loadtile(static_cast<short>(tilenume1));
    if (!waloff[tilenume2]) loadtile(static_cast<short>(tilenume2));
    if (!waloff[tilenume1] || !waloff[tilenume2]) return;
    unsigned char* src = reinterpret_cast<unsigned char*>(waloff[tilenume1]);
    unsigned char* dst = reinterpret_cast<unsigned char*>(waloff[tilenume2]);
    long xs1 = tilesizx[tilenume1], ys1 = tilesizy[tilenume1];
    long xs2 = tilesizx[tilenume2], ys2 = tilesizy[tilenume2];
    for (long x = 0; x < xsiz; ++x)
        for (long y = 0; y < ysiz; ++y)
        {
            long ax = (sx1 + x) % xs1, ay = (sy1 + y) % ys1;
            long bx = sx2 + x, by = sy2 + y;
            if (bx >= 0 && by >= 0 && bx < xs2 && by < ys2)
            {
                unsigned char v = src[ax * ys1 + ay];
                if (v != 255) dst[bx * ys2 + by] = v;
            }
        }
    buildgl::markTileDirty(static_cast<short>(tilenume2));
}

extern "C" void squarerotatetile(short tilenume)
{
    if ((unsigned short)tilenume >= MAXTILES || !waloff[tilenume]) return;
    long xs = tilesizx[tilenume], ys = tilesizy[tilenume];
    if (xs != ys || xs <= 0) return;
    unsigned char* p = reinterpret_cast<unsigned char*>(waloff[tilenume]);
    std::vector<unsigned char> tmp(static_cast<size_t>(xs * ys));
    for (long x = 0; x < xs; ++x)
        for (long y = 0; y < ys; ++y)
            tmp[(xs - 1 - y) * ys + x] = p[x * ys + y];
    std::memcpy(p, tmp.data(), tmp.size());
    buildgl::markTileDirty(tilenume);
}

extern "C" int getclosestcol(long r, long g, long b)
{
    const unsigned char* srcpal = basepalettevalid ? basepalette : palette;

    int best = 0;
    long bestd = 0x7fffffff;

    for (int i = 0; i < 256; ++i)
    {
        long dr = static_cast<long>(srcpal[i * 3 + 0]) - r;
        long dg = static_cast<long>(srcpal[i * 3 + 1]) - g;
        long db = static_cast<long>(srcpal[i * 3 + 2]) - b;
        long d = dr * dr + dg * dg + db * db;

        if (d < bestd)
        {
            bestd = d;
            best = i;
        }
    }

    return best;
}

extern "C" void initfastcolorlookup(long, long, long) {}

extern "C" int VBE_setPalette(long start, long num, char* dapal)
{
    if (!dapal || start < 0 || num <= 0 || start >= 256) return 0;
    if (start + num > 256) num = 256 - start;

    const unsigned char* srcBase = reinterpret_cast<const unsigned char*>(dapal);

    for (long i = 0; i < num; ++i)
    {
        const long idx = start + i;
        const unsigned char* src = srcBase + (i << 2);

        // RGBQUAD/BGRx layout: byte 0 = blue, byte 1 = green,
        // byte 2 = red, byte 3 = unused.
        palette[idx * 3 + 0] = buildgl_to_build6(src[2]);
        palette[idx * 3 + 1] = buildgl_to_build6(src[1]);
        palette[idx * 3 + 2] = buildgl_to_build6(src[0]);
    }

    paletteloaded = 1;
    buildgl::purgeTextureCache();
    return 1;
}

extern "C" int VBE_getPalette(long start, long num, char* dapal)
{
    if (!dapal || start < 0 || num <= 0 || start >= 256) return 0;
    if (start + num > 256) num = 256 - start;

    unsigned char* dstBase = reinterpret_cast<unsigned char*>(dapal);

    for (long i = 0; i < num; ++i)
    {
        const long idx = start + i;
        unsigned char* dst = dstBase + (i << 2);

        // Return BGRx, matching VBE_setPalette input.
        dst[0] = static_cast<unsigned char>(palette[idx * 3 + 2]);
        dst[1] = static_cast<unsigned char>(palette[idx * 3 + 1]);
        dst[2] = static_cast<unsigned char>(palette[idx * 3 + 0]);
        dst[3] = 0;
    }

    return 1;
}

extern "C" void makepalookup(long palnum, char* remapbuf, signed char r, signed char g, signed char b, char dastat)
{
    if (palnum < 0 || palnum >= MAXPALOOKUPS) return;
    if (!palookup[palnum]) palookup[palnum] = static_cast<char*>(std::malloc(static_cast<size_t>(numpalookups) << 8));
    if (!palookup[palnum]) return;
    if (!remapbuf || dastat == 0)
    {
        if (palookup[0]) std::memcpy(palookup[palnum], palookup[0], static_cast<size_t>(numpalookups) << 8);
        buildgl::installIdentityLookupPalette(palnum);
        return;
    }
    const unsigned char* srcpal = basepalettevalid ? basepalette : palette;

    for (long s = 0; s < numpalookups; ++s)
        for (int i = 0; i < 256; ++i)
        {
            const unsigned char remapped = static_cast<unsigned char>(remapbuf[i]);
            int ri = static_cast<unsigned char>(srcpal[remapped * 3 + 0]);
            int gi = static_cast<unsigned char>(srcpal[remapped * 3 + 1]);
            int bi = static_cast<unsigned char>(srcpal[remapped * 3 + 2]);
            if (r | g | b)
            {
                ri = ri + ((r - ri) * s) / max(1L, numpalookups);
                gi = gi + ((g - gi) * s) / max(1L, numpalookups);
                bi = bi + ((b - bi) * s) / max(1L, numpalookups);
                palookup[palnum][(s << 8) + i] = static_cast<char>(getclosestcol(ri, gi, bi));
            }
            else palookup[palnum][(s << 8) + i] = remapbuf[i];
        }
    unsigned char remapRow[256];
    for (int i = 0; i < 256; ++i)
        remapRow[i] = static_cast<unsigned char>(palookup[palnum][i]);
    buildgl::installLookupPalette(static_cast<unsigned char>(palnum), remapRow);
}

extern "C" void setbrightness(char dabrightness, unsigned char* dapal)
{
    curbrightness = min(max(static_cast<long>(dabrightness), 0L), 15L);

    // Classic callers often pass palette itself. Do NOT read from palette[]
    // while also overwriting it, or brightness changes compound permanently.
    const unsigned char* src = dapal;
    if (!src || src == palette)
        src = basepalettevalid ? basepalette : palette;

    for (int i = 0; i < 256; ++i)
    {
        const unsigned char r = buildgl_to_build6(src[i * 3 + 0]);
        const unsigned char g = buildgl_to_build6(src[i * 3 + 1]);
        const unsigned char b = buildgl_to_build6(src[i * 3 + 2]);

        palette[i * 3 + 0] = britable[curbrightness][r];
        palette[i * 3 + 1] = britable[curbrightness][g];
        palette[i * 3 + 2] = britable[curbrightness][b];
    }

    paletteloaded = 1;
    buildgl::purgeTextureCache();
}

extern "C" void FlushTextureCache()
{
    buildgl::purgeTextureCache();
}

extern "C" long getpalookup(long davis, long dashade)
{
    return min(max(dashade + (davis >> 8), 0L), numpalookups - 1);
}

// -----------------------------------------------------------------------------
// View-to-tile and mirror hooks.
// -----------------------------------------------------------------------------
static long setviewcnt = 0;
static long bakwindowx1[8], bakwindowy1[8], bakwindowx2[8], bakwindowy2[8];
extern "C" void setviewtotile(short tilenume, long xsiz, long ysiz)
{
    if (setviewcnt < 8)
    {
        bakwindowx1[setviewcnt] = windowx1; bakwindowy1[setviewcnt] = windowy1;
        bakwindowx2[setviewcnt] = windowx2; bakwindowy2[setviewcnt] = windowy2;
        ++setviewcnt;
    }
    allocatepermanenttile(tilenume, xsiz, ysiz);

    // The OpenGL drop-in does not actually render into the software tile
    // framebuffer. Shrinking the global BUILD view here makes later on-screen
    // 3D/2D rendering appear in a tiny tile-sized rectangle if a caller forgets
    // to restore it before the next GL frame. Keep the saved-view stack behavior
    // for compatibility, but leave the real on-screen view unchanged.
#if defined(BUILDGL_SET_VIEW_TO_TILE_SHRINKS_SCREEN) && BUILDGL_SET_VIEW_TO_TILE_SHRINKS_SCREEN
    setview(0, 0, ysiz - 1, xsiz - 1);
#else
    (void)xsiz;
    (void)ysiz;
#endif
}
extern "C" void setviewback()
{
    if (setviewcnt <= 0) return;
    --setviewcnt;
    setview(bakwindowx1[setviewcnt], bakwindowy1[setviewcnt], bakwindowx2[setviewcnt], bakwindowy2[setviewcnt]);
}
extern "C" void preparemirror(long dax, long day, long daz, short daang, long dahoriz, short dawall, short dasector, long* tposx, long* tposy, short* tang)
{
    (void)daz; (void)dahoriz; (void)dasector;
    if (dawall < 0 || dawall >= numwalls) return;
    walltype& w = wall[dawall]; walltype& w2 = wall[w.point2];
    long dx = w2.x - w.x, dy = w2.y - w.y;
    long j = dx * dx + dy * dy; if (j == 0) return;
    long i = (((dax - w.x) * dx + (day - w.y) * dy) << 1);
    if (tposx) *tposx = (w.x << 1) + scale(dx, i, j) - dax;
    if (tposy) *tposy = (w.y << 1) + scale(dy, i, j) - day;
    if (tang) *tang = static_cast<short>(((getangle(dx, dy) << 1) - daang) & 2047);
    inpreparemirror = 1;
}
extern "C" void completemirror() { inpreparemirror = 0; }


// -----------------------------------------------------------------------------
// Duke/RTS/CONFIG compatibility shims expected by older Visual Studio projects.
// -----------------------------------------------------------------------------
#ifndef BUILDGL_NO_CRT_ARGV_STUBS
extern "C" int _argc = 0;
extern "C" char** _argv = nullptr;
#endif

namespace buildgl_shutdown
{
    static std::vector<void (*)()> funcs;
    static bool installed = false;
    static void run()
    {
        for (auto it = funcs.rbegin(); it != funcs.rend(); ++it)
            if (*it) (*it)();
    }
}

extern "C" void RegisterShutdownFunction(void (*fn)())
{
    if (!buildgl_shutdown::installed)
    {
        std::atexit(buildgl_shutdown::run);
        buildgl_shutdown::installed = true;
    }
    if (fn) buildgl_shutdown::funcs.push_back(fn);
}

extern "C" void Error(const char* fmt, ...)
{
    std::fprintf(stderr, "ERROR: ");
    if (fmt)
    {
        va_list ap;
        va_start(ap, fmt);
        std::vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
    std::fprintf(stderr, "\n");
    std::fflush(stderr);
    std::exit(1);
}

extern "C" int SafeFileExists(const char* filename)
{
    if (!filename || !filename[0]) return 0;

    int fd = buildfile::tryOpenDiskReadHandle(filename);
    if (fd >= 0)
    {
        buildfile::osCloseHandle(fd);
        return 1;
    }

    return buildfile::groupEntryExists(filename) ? 1 : 0;
}

extern "C" void* SafeMalloc(unsigned long size)
{
    void* p = std::malloc(static_cast<size_t>(size ? size : 1));
    if (!p) Error("SafeMalloc failed for %lu bytes", size);
    return p;
}

extern "C" void* SafeRealloc(void* ptr, unsigned long size)
{
    void* p = std::realloc(ptr, static_cast<size_t>(size ? size : 1));
    if (!p) Error("SafeRealloc failed for %lu bytes", size);
    return p;
}

extern "C" void SafeFree(void* ptr)
{
    std::free(ptr);
}

extern "C" int SafeOpenRead(const char* filename)
{
    if (!filename || !filename[0]) Error("SafeOpenRead called with empty filename");

    // File I/O compatibility path: return a normal OS/CRT file descriptor.
    // Loose files are opened first. If a mounted GRP contains the file, the
    // entry is copied to an auto-deleted temporary file so old code that calls
    // lseek(), filelength(), read(), SafeRead(), or close() on the handle still
    // behaves like it was reading a regular disk file.
    int h = buildfile::tryOpenDiskReadHandle(filename);
    if (h >= 0) return h;

    h = buildfile::openGroupAsDiskCompatibleHandle(filename);
    if (h >= 0) return h;

    Error("Could not open %s for reading", filename ? filename : "<null>");
    return -1;
}

extern "C" int SafeOpenWrite(const char* filename)
{
    if (!filename || !filename[0]) Error("SafeOpenWrite called with empty filename");
    int h = buildfile::osOpenWritePath(filename);
    if (h < 0) Error("Could not open %s for writing", filename ? filename : "<null>");
    return h;
}

extern "C" int SafeOpenAppend(const char* filename)
{
    if (!filename || !filename[0]) Error("SafeOpenAppend called with empty filename");
    int h = buildfile::osOpenAppendPath(filename);
    if (h < 0) Error("Could not open %s for appending", filename ? filename : "<null>");
    return h;
}

extern "C" void SafeRead(int handle, void* buffer, long count)
{
    if (count <= 0) return;
    if (!buffer) Error("SafeRead called with null buffer");
    long done = 0;
    while (done < count)
    {
        int got = buildfile::osReadHandle(handle, static_cast<char*>(buffer) + done, count - done);
        if (got <= 0) Error("SafeRead failed");
        done += got;
    }
}

extern "C" void SafeWrite(int handle, const void* buffer, long count)
{
    if (count <= 0) return;
    if (!buffer) Error("SafeWrite called with null buffer");
    long done = 0;
    while (done < count)
    {
        int did = buildfile::osWriteHandle(handle, static_cast<const char*>(buffer) + done, count - done);
        if (did <= 0) Error("SafeWrite failed");
        done += did;
    }
}

extern "C" long SafeFileLength(int handle)
{
    long cur = buildfile::osSeekHandle(handle, 0, SEEK_CUR);
    if (cur < 0) return -1;
    long end = buildfile::osSeekHandle(handle, 0, SEEK_END);
    buildfile::osSeekHandle(handle, cur, SEEK_SET);
    return end;
}

extern "C" void SafeClose(int handle)
{
    if (handle >= 0) buildfile::osCloseHandle(handle);
}

extern "C" long IntelLong(long value)
{
    return value;
}

extern "C" int CheckParm(const char* check)
{
    if (!check || !check[0]) return 0;
#ifndef BUILDGL_NO_CRT_ARGV_STUBS
    int argcLocal = _argc;
    char** argvLocal = _argv;
#else
    int argcLocal = 0;
    char** argvLocal = nullptr;
#endif
    if (!argvLocal || argcLocal <= 0) return 0;
    for (int i = 1; i < argcLocal; ++i)
        if (argvLocal[i] && _stricmp(argvLocal[i], check) == 0) return i;
    return 0;
}

//extern "C" int MOUSE_GetButtons()
//{
//#if defined(_WIN32)
//    int buttons = 0;
//    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) buttons |= 1;
//    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) buttons |= 2;
//    if (GetAsyncKeyState(VK_MBUTTON) & 0x8000) buttons |= 4;
//    return buttons;
//#else
//    return 0;
//#endif
//}
extern "C" void inittimerhandler();
void TestCallBack(unsigned long) { faketimerhandler(); }
//void inittimer() { inittimerhandler(); }
//void uninittimer() { uninittimerhandler(); }

// -----------------------------------------------------------------------------
// Misc original entry points retained/stubbed for compatibility.
// -----------------------------------------------------------------------------
extern "C" void flushperms() {}
extern "C" int clippoly4(long, long, long, long) { return 0; }
extern "C" int clippoly(long npoints, long) { return static_cast<int>(npoints); }
extern "C" void fillpolygon(long) {}
extern "C" int owallmost(short* mostbuf, long, long)
{
    if (mostbuf) std::fill(mostbuf, mostbuf + std::min<long>(xdimen, MAXXDIM), 0);
    return 0;
}
extern "C" int wallmost(short* mostbuf, long, long, char)
{
    if (mostbuf) std::fill(mostbuf, mostbuf + std::min<long>(xdimen, MAXXDIM), 0);
    return 0;
}
extern "C" int initmouse() { return 0; }
extern "C" void getmousevalues(short* mousx, short* mousy, short* bstatus)
{
    if (mousx) *mousx = 0; if (mousy) *mousy = 0; if (bstatus) *bstatus = 0;
}
extern "C" void printscreeninterrupt() {}
extern "C" int krand()
{
    randomseed = randomseed * 27584621 + 1;
    return static_cast<int>((static_cast<unsigned long>(randomseed)) >> 16);
}
extern "C" void stereonextpage() {}
extern "C" void setstereo(long dastereomode) { stereomode = dastereomode ? 0 : 0; }
extern "C" int qloadkvx(long, char*) { return 0; }
extern "C" void drawvox(long, long, long, long, long, long, char, signed char, char, long*, long*) {}
extern "C" void ceilspritescan(long, long) {}
extern "C" void ceilspritehline(long, long) {}
extern "C" void setgotpic(long tilenume)
{
    if ((unsigned long)tilenume >= MAXTILES) return;
    gotpic[tilenume >> 3] |= pow2char[tilenume & 7];
    if (walock[tilenume] < 200) walock[tilenume] = 199;
}

// Full cache1d-compatible cache, timer, and multiplayer/network compatibility.
#define BUILDCACHE_MAXOBJECTS 9216
struct cactype { long* hand; long leng; char* lock; };
long cacnum = 0;
static cactype buildgl_public_cac_storage[BUILDCACHE_MAXOBJECTS] = {};
cactype* cac = buildgl_public_cac_storage;

namespace buildcache
{
    struct CacheBlock { long* hand = nullptr; long leng = 0; char* lock = nullptr; };
    struct HeapBlock { long* hand = nullptr; void* ptr = nullptr; long leng = 0; char* lock = nullptr; };
    static long cachesize_ = 0;
    static long cachestart_ = 0;
    static long cacnum_ = 0;
    static long agecount_ = 0;
    static long cachecount_ = 0;
    static char zerochar_ = 0;
    static long lockrecip_[200] = {};
    static CacheBlock cac_[BUILDCACHE_MAXOBJECTS];
    static std::unique_ptr<unsigned char[]> ownedCache_;
    static std::vector<HeapBlock> heapBlocks_;

    static void syncPublicCache()
    {
        ::cacnum = cacnum_;
        const long n = (cacnum_ < BUILDCACHE_MAXOBJECTS) ? cacnum_ : BUILDCACHE_MAXOBJECTS;
        for (long i = 0; i < n; ++i)
        {
            ::cac[i].hand = cac_[i].hand;
            ::cac[i].leng = cac_[i].leng;
            ::cac[i].lock = cac_[i].lock;
        }
    }

    static void collapseEmpty(long i)
    {
        if (i > 0 && cac_[i - 1].lock && *cac_[i - 1].lock == 0)
        {
            cac_[i - 1].leng += cac_[i].leng;
            --cacnum_;
            std::memmove(&cac_[i], &cac_[i + 1], static_cast<size_t>(cacnum_ - i) * sizeof(CacheBlock));
            --i;
        }
        if (i >= 0 && i < cacnum_ - 1 && cac_[i + 1].lock && *cac_[i + 1].lock == 0)
        {
            cac_[i].leng += cac_[i + 1].leng;
            --cacnum_;
            std::memmove(&cac_[i + 1], &cac_[i + 2], static_cast<size_t>(cacnum_ - i - 1) * sizeof(CacheBlock));
        }
    }
}

extern "C" void reportandexit(char* errormessage)
{
    std::fprintf(stderr, "BUILDGL fatal error: %s\n", errormessage ? errormessage : "unknown error");
    std::fflush(stderr);
    std::abort();
}

extern "C" void initcache(long dacachestart, long dacachesize)
{
    using namespace buildcache;
    if (dacachesize <= 0) dacachesize = 4L << 20;
    dacachesize = (dacachesize + 15) & ~15L;

    if (dacachestart == 0)
    {
        ownedCache_.reset(new unsigned char[static_cast<size_t>(dacachesize) + 16]);
        uintptr_t p = reinterpret_cast<uintptr_t>(ownedCache_.get());
        p = (p + 15u) & ~static_cast<uintptr_t>(15u);
        dacachestart = BUILDGL_PTR_TO_LONG(reinterpret_cast<void*>(p));
    }
    else ownedCache_.reset();

    for (long i = 1; i < 200; ++i) lockrecip_[i] = (1 << 28) / (200 - i);
    cachestart_ = dacachestart;
    cachesize_ = dacachesize;
    cac_[0].hand = nullptr;
    cac_[0].leng = cachesize_;
    cac_[0].lock = &zerochar_;
    cacnum_ = 1;
    agecount_ = 0;
    syncPublicCache();
}

extern "C" void allocache(long* newhandle, long newbytes, char* newlockptr)
{
    using namespace buildcache;
    if (!newhandle) return;
    newbytes = ((newbytes + 15) & ~15L);
    if (newbytes <= 0) newbytes = 16;

    if (!newlockptr || *newlockptr == 0)
        reportandexit((char*)"ALLOCACHE CALLED WITH LOCK OF 0!");

    if (cachesize_ <= 0 || cachestart_ == 0)
    {
        void* p = std::malloc(static_cast<size_t>(newbytes));
        if (!p) reportandexit((char*)"OUT OF MEMORY IN ALLOCACHE!");
        *newhandle = BUILDGL_PTR_TO_LONG(p);
        heapBlocks_.push_back(HeapBlock{ newhandle, p, newbytes, newlockptr });
        if (*newlockptr < 200) *newlockptr = 199;
        ++cachecount_;
        syncPublicCache();
        return;
    }

    if (static_cast<unsigned long>(newbytes) > static_cast<unsigned long>(cachesize_))
        reportandexit((char*)"BUFFER TOO BIG TO FIT IN CACHE!");

    long bestval = 0x7fffffff, besto = 0, bestz = 0, o1 = cachesize_;
    for (long z = cacnum_ - 1; z >= 0; --z)
    {
        o1 -= cac_[z].leng;
        long o2 = o1 + newbytes;
        if (o2 > cachesize_) continue;

        long daval = 0;
        long zz = z;
        for (long i = o1; i < o2 && zz < cacnum_; i += cac_[zz++].leng)
        {
            if (!cac_[zz].lock || *cac_[zz].lock == 0) continue;
            if (static_cast<unsigned char>(*cac_[zz].lock) >= 200) { daval = 0x7fffffff; break; }
            daval += mulscale32(cac_[zz].leng + 65536, lockrecip_[static_cast<unsigned char>(*cac_[zz].lock)]);
            if (daval >= bestval) break;
        }
        if (daval < bestval)
        {
            bestval = daval;
            besto = o1;
            bestz = z;
            if (bestval == 0) break;
        }
    }

    if (bestval == 0x7fffffff) reportandexit((char*)"CACHE SPACE ALL LOCKED UP!");

    long sucklen = -newbytes;
    long suckz = bestz;
    while (sucklen < 0 && suckz < cacnum_)
    {
        if (cac_[suckz].lock && *cac_[suckz].lock && cac_[suckz].hand) *cac_[suckz].hand = 0;
        sucklen += cac_[suckz].leng;
        ++suckz;
    }

    long removeCount = suckz - (bestz + 1);
    cacnum_ -= removeCount;
    if (removeCount > 0)
        std::memmove(&cac_[bestz + 1], &cac_[bestz + 1 + removeCount], static_cast<size_t>(cacnum_ - bestz - 1) * sizeof(CacheBlock));

    cac_[bestz].hand = newhandle;
    *newhandle = cachestart_ + besto;
    cac_[bestz].leng = newbytes;
    cac_[bestz].lock = newlockptr;
    if (*newlockptr < 200) *newlockptr = 199;
    ++cachecount_;

    if (sucklen <= 0) { syncPublicCache(); return; }

    long insertAt = bestz + 1;
    if (insertAt == cacnum_)
    {
        if (cacnum_ >= BUILDCACHE_MAXOBJECTS) reportandexit((char*)"Too many objects in cache!");
        cac_[insertAt].hand = nullptr;
        cac_[insertAt].leng = sucklen;
        cac_[insertAt].lock = &zerochar_;
        ++cacnum_;
        syncPublicCache();
        return;
    }

    if (cac_[insertAt].lock && *cac_[insertAt].lock == 0)
    {
        cac_[insertAt].leng += sucklen;
        syncPublicCache();
        return;
    }

    if (cacnum_ >= BUILDCACHE_MAXOBJECTS) reportandexit((char*)"Too many objects in cache!");
    std::memmove(&cac_[insertAt + 1], &cac_[insertAt], static_cast<size_t>(cacnum_ - insertAt) * sizeof(CacheBlock));
    cac_[insertAt].hand = nullptr;
    cac_[insertAt].leng = sucklen;
    cac_[insertAt].lock = &zerochar_;
    ++cacnum_;
    syncPublicCache();
}

extern "C" void suckcache(long* suckptr)
{
    using namespace buildcache;
    if (!suckptr) return;

    for (auto it = heapBlocks_.begin(); it != heapBlocks_.end(); )
    {
        bool match = (it->hand == suckptr) || (it->hand && *it->hand == BUILDGL_PTR_TO_LONG(suckptr));
        if (match)
        {
            if (it->hand) *it->hand = 0;
            std::free(it->ptr);
            it = heapBlocks_.erase(it);
        }
        else ++it;
    }

    for (long i = 0; i < cacnum_; ++i)
    {
        if (!cac_[i].hand) continue;
        if ((*cac_[i].hand == BUILDGL_PTR_TO_LONG(suckptr)) || (cac_[i].hand == suckptr))
        {
            if (cac_[i].lock && *cac_[i].lock) *cac_[i].hand = 0;
            cac_[i].hand = nullptr;
            cac_[i].lock = &zerochar_;
            collapseEmpty(i);
            syncPublicCache();
            return;
        }
    }
    syncPublicCache();
}

extern "C" void agecache()
{
    using namespace buildcache;
    if (cacnum_ <= 0) return;
    if (agecount_ >= cacnum_) agecount_ = cacnum_ - 1;
    for (long cnt = (cacnum_ >> 4); cnt >= 0; --cnt)
    {
        if (cac_[agecount_].lock)
        {
            unsigned char ch = static_cast<unsigned char>(*cac_[agecount_].lock);
            if (((ch - 2) & 255) < 198) *cac_[agecount_].lock = static_cast<char>(ch - 1);
        }
        --agecount_;
        if (agecount_ < 0) agecount_ = cacnum_ - 1;
    }
    syncPublicCache();
}

extern "C" void uninitcache(long all)
{
    using namespace buildcache;
    for (long i = 0; i < cacnum_; ++i)
    {
        if (cac_[i].lock && (all || *cac_[i].lock != 0))
        {
            if (cac_[i].hand) *cac_[i].hand = 0;
            cac_[i].hand = nullptr;
            cac_[i].lock = &zerochar_;
        }
    }
    for (auto it = heapBlocks_.begin(); it != heapBlocks_.end(); )
    {
        if (all || (it->lock && *it->lock != 0))
        {
            if (it->hand) *it->hand = 0;
            std::free(it->ptr);
            it = heapBlocks_.erase(it);
        }
        else ++it;
    }
    for (long i = 0; i < cacnum_; ++i) collapseEmpty(i);
    syncPublicCache();
}

extern "C" long getcachecount() { return buildcache::cachecount_; }

namespace buildtimer
{
    static bool initialized = false;
    static std::chrono::steady_clock::time_point start;
    static long startClock = 0;
}

extern "C" void inittimerhandler()
{
    buildtimer::initialized = true;
    buildtimer::start = std::chrono::steady_clock::now();
    buildtimer::startClock = totalclock;
    lockclock = totalclock;
}

extern "C" void uninittimerhandler() {}

void _faketimerhandler(void);
void faketimerhandler()
{
    //if (!buildtimer::initialized) inittimerhandler();
    //const auto now = std::chrono::steady_clock::now();
    //const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - buildtimer::start).count();
    //long ticks = buildtimer::startClock + static_cast<long>((ms * 120) / 1000);
    //if (ticks > totalclock) totalclock = ticks;
    lockclock = totalclock;
    buildgl::pump();

    _faketimerhandler();
}

// -----------------------------------------------------------------------------
// Modern UDP multiplayer transport. The old DOS commit-driver packet reliability
// layer is preserved, but packets are carried by UDP sockets. Configure with:
//   BUILDGL_NET_PLAYERS=N
//   BUILDGL_NET_INDEX=0-based local player index
//   BUILDGL_NET_PORT=local UDP port
//   BUILDGL_NET_PEERS=host:port,host:port,-,host:port  (one slot per player)
// You may also call setplayeraddress(player, host, port) from game code.
// -----------------------------------------------------------------------------
#define BUILDNET_MAXPLAYERS 16
#define BUILDNET_BAKSIZ 16384
#define BUILDNET_MAXPACKETSIZE 2048
#define buildnet_updatecrc16(crc,dat) crc = (((crc << 8) & 65535) ^ crctable[((((unsigned short)crc) >> 8) & 65535) ^ (unsigned char)(dat)])

long crctable[256] = {};
short myconnectindex = 0, numplayers = 1;
short connecthead = 0;
static short buildgl_connectpoint2_storage[BUILDNET_MAXPLAYERS] = { -1 };
short* connectpoint2 = buildgl_connectpoint2_storage;
char syncstate = 0;

namespace buildnet
{
#if defined(_WIN32)
    using socket_t = SOCKET;
    static constexpr socket_t invalid_socket_value = INVALID_SOCKET;
    static void closesocket_compat(socket_t s) { if (s != invalid_socket_value) closesocket(s); }
    using socklen_type = int;
#else
    using socket_t = int;
    static constexpr socket_t invalid_socket_value = -1;
    static void closesocket_compat(socket_t s) { if (s != invalid_socket_value) close(s); }
    using socklen_type = socklen_t;
#endif

    struct Peer
    {
        sockaddr_storage addr{};
        socklen_type addrlen = 0;
        bool valid = false;
    };
    struct RawPacket
    {
        short from = -1;
        long len = 0;
        char data[BUILDNET_MAXPACKETSIZE]{};
    };

    static socket_t sock = invalid_socket_value;
    static bool socketStarted = false;
    static Peer peers[BUILDNET_MAXPLAYERS];
    static std::deque<RawPacket> queue;

    static void startup()
    {
#if defined(_WIN32)
        if (!socketStarted)
        {
            WSADATA wsa{};
            WSAStartup(MAKEWORD(2, 2), &wsa);
            socketStarted = true;
        }
#endif
    }

    static bool setNonBlocking(socket_t s)
    {
#if defined(_WIN32)
        u_long mode = 1;
        return ioctlsocket(s, FIONBIO, &mode) == 0;
#else
        int flags = fcntl(s, F_GETFL, 0);
        if (flags < 0) return false;
        return fcntl(s, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
    }

    static bool bindPort(unsigned short port)
    {
        startup();
        if (sock != invalid_socket_value) closesocket_compat(sock);
        sock = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock == invalid_socket_value) return false;

        sockaddr_in a{};
        a.sin_family = AF_INET;
        a.sin_addr.s_addr = htonl(INADDR_ANY);
        a.sin_port = htons(port);
        if (::bind(sock, reinterpret_cast<sockaddr*>(&a), sizeof(a)) != 0)
        {
            closesocket_compat(sock);
            sock = invalid_socket_value;
            return false;
        }
        setNonBlocking(sock);
        return true;
    }

    static bool setPeer(short player, const char* host, unsigned short port)
    {
        if (player < 0 || player >= BUILDNET_MAXPLAYERS || !host || !host[0]) return false;
        startup();

        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        char portbuf[16];
        std::snprintf(portbuf, sizeof(portbuf), "%u", static_cast<unsigned>(port));
        addrinfo* res = nullptr;
        if (getaddrinfo(host, portbuf, &hints, &res) != 0 || !res) return false;

        std::memset(&peers[player], 0, sizeof(peers[player]));
        std::memcpy(&peers[player].addr, res->ai_addr, static_cast<size_t>(res->ai_addrlen));
        peers[player].addrlen = static_cast<socklen_type>(res->ai_addrlen);
        peers[player].valid = true;
        freeaddrinfo(res);
        return true;
    }

    static void parsePeersEnv()
    {
        const char* peersEnv = std::getenv("BUILDGL_NET_PEERS");
        if (!peersEnv || !peersEnv[0]) return;
        std::string s(peersEnv);
        size_t start = 0;
        int player = 0;
        while (start <= s.size() && player < BUILDNET_MAXPLAYERS)
        {
            size_t comma = s.find(',', start);
            std::string tok = s.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
            while (!tok.empty() && std::isspace(static_cast<unsigned char>(tok.front()))) tok.erase(tok.begin());
            while (!tok.empty() && std::isspace(static_cast<unsigned char>(tok.back()))) tok.pop_back();
            if (!tok.empty() && tok != "-")
            {
                size_t colon = tok.rfind(':');
                if (colon != std::string::npos)
                {
                    std::string host = tok.substr(0, colon);
                    unsigned short port = static_cast<unsigned short>(std::strtoul(tok.c_str() + colon + 1, nullptr, 10));
                    setPeer(static_cast<short>(player), host.c_str(), port);
                }
            }
            if (comma == std::string::npos) break;
            start = comma + 1;
            ++player;
        }
    }

    static bool sameAddress(const sockaddr_storage& a, socklen_type alen, const sockaddr_storage& b, socklen_type blen)
    {
        if (alen != blen || a.ss_family != b.ss_family) return false;
        return std::memcmp(&a, &b, static_cast<size_t>(alen)) == 0;
    }

    static short peerFromAddress(const sockaddr_storage& from, socklen_type fromlen, short hinted)
    {
        if (hinted >= 0 && hinted < BUILDNET_MAXPLAYERS)
        {
            if (!peers[hinted].valid)
            {
                peers[hinted].addr = from;
                peers[hinted].addrlen = fromlen;
                peers[hinted].valid = true;
                return hinted;
            }
            if (sameAddress(from, fromlen, peers[hinted].addr, peers[hinted].addrlen)) return hinted;
        }
        for (short i = 0; i < BUILDNET_MAXPLAYERS; ++i)
            if (peers[i].valid && sameAddress(from, fromlen, peers[i].addr, peers[i].addrlen)) return i;
        return hinted;
    }

    static bool rawSend(short player, const char* data, long len)
    {
        if (player < 0 || player >= BUILDNET_MAXPLAYERS || player == myconnectindex) return false;
        if (sock == invalid_socket_value || !peers[player].valid || !data || len <= 0) return false;
        if (len > BUILDNET_MAXPACKETSIZE - 4) len = BUILDNET_MAXPACKETSIZE - 4;
        char out[BUILDNET_MAXPACKETSIZE + 4];
        out[0] = 'B'; out[1] = 'N'; out[2] = static_cast<char>(myconnectindex & 255);
        std::memcpy(out + 3, data, static_cast<size_t>(len));
        int sent = sendto(sock, out, static_cast<int>(len + 3), 0, reinterpret_cast<sockaddr*>(&peers[player].addr), peers[player].addrlen);
        return sent == len + 3;
    }

    static bool rawRecv(RawPacket& out)
    {
        if (!queue.empty()) { out = queue.front(); queue.pop_front(); return true; }
        if (sock == invalid_socket_value) return false;

        for (;;)
        {
            char buf[BUILDNET_MAXPACKETSIZE + 8];
            sockaddr_storage from{};
            socklen_type fromlen = sizeof(from);
            int got = recvfrom(sock, buf, sizeof(buf), 0, reinterpret_cast<sockaddr*>(&from), &fromlen);
            if (got <= 0) return false;

            short hinted = -1;
            long offs = 0;
            if (got >= 3 && buf[0] == 'B' && buf[1] == 'N')
            {
                hinted = static_cast<unsigned char>(buf[2]);
                offs = 3;
            }
            short fromPlayer = peerFromAddress(from, fromlen, hinted);
            if (fromPlayer < 0 || fromPlayer >= BUILDNET_MAXPLAYERS) continue;
            out.from = fromPlayer;
            out.len = min<long>(got - offs, BUILDNET_MAXPACKETSIZE);
            std::memcpy(out.data, buf + offs, static_cast<size_t>(out.len));
            return true;
        }
    }

    static void closeAll()
    {
        if (sock != invalid_socket_value)
        {
            closesocket_compat(sock);
            sock = invalid_socket_value;
        }
        queue.clear();
    }
}

static long buildnet_incnt[BUILDNET_MAXPLAYERS] = {};
static long buildnet_outcntplc[BUILDNET_MAXPLAYERS] = {};
static long buildnet_outcntend[BUILDNET_MAXPLAYERS] = {};
static char buildnet_errorgotnum[BUILDNET_MAXPLAYERS] = {};
static char buildnet_errorfixnum[BUILDNET_MAXPLAYERS] = {};
static char buildnet_errorresendnum[BUILDNET_MAXPLAYERS] = {};
static char buildnet_lastpacket[BUILDNET_MAXPACKETSIZE] = {};
static char buildnet_inlastpacket = 0;
static short buildnet_lastpacketfrom = -1, buildnet_lastpacketleng = 0;
static long buildnet_timeoutcount = 60, buildnet_resendagaincount = 4, buildnet_lastsendtime[BUILDNET_MAXPLAYERS] = {};
static short buildnet_bakpacketptr[BUILDNET_MAXPLAYERS][256] = {};
static short buildnet_bakpacketlen[BUILDNET_MAXPLAYERS][256] = {};
static char buildnet_bakpacketbuf[BUILDNET_BAKSIZ] = {};
static long buildnet_bakpacketplc = 0;

extern "C" void initcrc()
{
    long i, j, k, a;
    for (j = 0; j < 256; ++j)
    {
        k = (j << 8); a = 0;
        for (i = 7; i >= 0; --i)
        {
            if (((k ^ a) & 0x8000) > 0) a = ((a << 1) & 65535) ^ 0x1021;
            else a = ((a << 1) & 65535);
            k = ((k << 1) & 65535);
        }
        crctable[j] = (a & 65535);
    }
}

extern "C" long getcrc(char* buffer, short bufleng)
{
    long crc = 0;
    if (!buffer || bufleng <= 0) return 0;
    for (long i = bufleng - 1; i >= 0; --i) buildnet_updatecrc16(crc, buffer[i]);
    return crc & 65535;
}

static void buildnet_resetReliable()
{
    std::memset(buildnet_incnt, 0, sizeof(buildnet_incnt));
    std::memset(buildnet_outcntplc, 0, sizeof(buildnet_outcntplc));
    std::memset(buildnet_outcntend, 0, sizeof(buildnet_outcntend));
    std::memset(buildnet_errorgotnum, 0, sizeof(buildnet_errorgotnum));
    std::memset(buildnet_errorfixnum, 0, sizeof(buildnet_errorfixnum));
    std::memset(buildnet_errorresendnum, 0, sizeof(buildnet_errorresendnum));
    std::memset(buildnet_bakpacketlen, 0, sizeof(buildnet_bakpacketlen));
    for (int i = 0; i < BUILDNET_MAXPLAYERS; ++i)
    {
        buildnet_bakpacketlen[i][255] = -1;
        buildnet_lastsendtime[i] = totalclock;
    }
    buildnet_bakpacketplc = 0;
    buildnet_inlastpacket = 0;
}

extern "C" void setpackettimeout(long datimeoutcount, long daresendagaincount)
{
    buildnet_timeoutcount = datimeoutcount;
    buildnet_resendagaincount = daresendagaincount;
    for (int i = 0; i < BUILDNET_MAXPLAYERS; ++i) buildnet_lastsendtime[i] = totalclock;
}

extern "C" void setsocket(short newsocket)
{
    unsigned short port = static_cast<unsigned short>(newsocket ? newsocket : (23513 + myconnectindex));
    buildnet::bindPort(port);
}

extern "C" int setplayeraddress(short player, char* host, short port)
{
    return buildnet::setPeer(player, host, static_cast<unsigned short>(port)) ? 0 : -1;
}

extern "C" void initmultiplayers(char damultioption, char dacomrateoption, char dapriority)
{
    (void)dacomrateoption; (void)dapriority;
    initcrc();
    faketimerhandler();
    buildnet_resetReliable();

    numplayers = 1;
    myconnectindex = 0;
    const char* envPlayers = std::getenv("BUILDGL_NET_PLAYERS");
    const char* envIndex = std::getenv("BUILDGL_NET_INDEX");
    if (envPlayers && envPlayers[0]) numplayers = static_cast<short>(std::atoi(envPlayers));
    else if (static_cast<unsigned char>(damultioption) > 1) numplayers = static_cast<short>(static_cast<unsigned char>(damultioption));
    if (envIndex && envIndex[0]) myconnectindex = static_cast<short>(std::atoi(envIndex));
    numplayers = min<short>(max<short>(numplayers, 1), BUILDNET_MAXPLAYERS);
    if (myconnectindex < 0 || myconnectindex >= numplayers) myconnectindex = 0;

    connecthead = 0;
    for (short i = 0; i < numplayers - 1; ++i) connectpoint2[i] = i + 1;
    connectpoint2[numplayers - 1] = -1;

    if (numplayers > 1)
    {
        unsigned short port = static_cast<unsigned short>(23513 + myconnectindex);
        const char* envPort = std::getenv("BUILDGL_NET_PORT");
        if (envPort && envPort[0]) port = static_cast<unsigned short>(std::strtoul(envPort, nullptr, 10));
        buildnet::bindPort(port);
        buildnet::parsePeersEnv();
    }
}

extern "C" void uninitmultiplayers()
{
    buildnet::closeAll();
#if defined(_WIN32)
    if (buildnet::socketStarted)
    {
        WSACleanup();
        buildnet::socketStarted = false;
    }
#endif
}

static void buildnet_dosendpackets(long other)
{
    if (numplayers < 2 || other < 0 || other >= BUILDNET_MAXPLAYERS) return;
    if (buildnet_outcntplc[other] == buildnet_outcntend[other]) return;

    char out[BUILDNET_MAXPACKETSIZE] = {};
    long k = 0;
    if (buildnet_outcntplc[other] + 1 == buildnet_outcntend[other])
    {
        out[k++] = static_cast<char>(buildnet_outcntplc[other] & 255);
        out[k++] = static_cast<char>((buildnet_errorgotnum[other] & 7) + ((buildnet_errorresendnum[other] & 7) << 3));
        out[k++] = static_cast<char>(buildnet_incnt[other] & 255);
        long j = buildnet_bakpacketptr[other][buildnet_outcntplc[other] & 255];
        long messleng = buildnet_bakpacketlen[other][buildnet_outcntplc[other] & 255];
        messleng = min<long>(messleng, BUILDNET_MAXPACKETSIZE - 8);
        for (long i = 0; i < messleng; ++i) out[k++] = buildnet_bakpacketbuf[(i + j) & (BUILDNET_BAKSIZ - 1)];
        buildnet_outcntplc[other]++;
    }
    else
    {
        out[k++] = static_cast<char>(buildnet_outcntplc[other] & 255);
        out[k++] = static_cast<char>((buildnet_errorgotnum[other] & 7) + ((buildnet_errorresendnum[other] & 7) << 3) + 128);
        out[k++] = static_cast<char>(buildnet_incnt[other] & 255);
        long j = buildnet_bakpacketptr[other][buildnet_outcntplc[other] & 255];
        long messleng = buildnet_bakpacketlen[other][buildnet_outcntplc[other] & 255];
        messleng = min<long>(messleng, (BUILDNET_MAXPACKETSIZE - 10) / 2);
        out[k++] = static_cast<char>(messleng & 255);
        out[k++] = static_cast<char>((messleng >> 8) & 255);
        for (long i = 0; i < messleng; ++i) out[k++] = buildnet_bakpacketbuf[(i + j) & (BUILDNET_BAKSIZ - 1)];
        buildnet_outcntplc[other]++;
        j = buildnet_bakpacketptr[other][buildnet_outcntplc[other] & 255];
        messleng = buildnet_bakpacketlen[other][buildnet_outcntplc[other] & 255];
        messleng = min<long>(messleng, BUILDNET_MAXPACKETSIZE - k - 2);
        for (long i = 0; i < messleng; ++i) out[k++] = buildnet_bakpacketbuf[(i + j) & (BUILDNET_BAKSIZ - 1)];
        buildnet_outcntplc[other]++;
    }
    unsigned short crc = static_cast<unsigned short>(getcrc(out, static_cast<short>(k)));
    out[k++] = static_cast<char>(crc & 255);
    out[k++] = static_cast<char>((crc >> 8) & 255);
    buildnet::rawSend(static_cast<short>(other), out, k);
}

extern "C" void sendpacket(long other, char* bufptr, long messleng)
{
    if (numplayers < 2 || other < 0 || other >= BUILDNET_MAXPLAYERS || !bufptr || messleng <= 0) return;
    messleng = min<long>(messleng, BUILDNET_MAXPACKETSIZE - 16);
    long i = 0, j = 0;
    if (buildnet_bakpacketlen[other][(buildnet_outcntend[other] - 1) & 255] == messleng)
    {
        j = buildnet_bakpacketptr[other][(buildnet_outcntend[other] - 1) & 255];
        for (i = messleng - 1; i >= 0; --i)
            if (buildnet_bakpacketbuf[(i + j) & (BUILDNET_BAKSIZ - 1)] != bufptr[i]) break;
    }
    buildnet_bakpacketlen[other][buildnet_outcntend[other] & 255] = static_cast<short>(messleng);
    if (i < 0) buildnet_bakpacketptr[other][buildnet_outcntend[other] & 255] = static_cast<short>(j);
    else
    {
        buildnet_bakpacketptr[other][buildnet_outcntend[other] & 255] = static_cast<short>(buildnet_bakpacketplc);
        for (i = 0; i < messleng; ++i) buildnet_bakpacketbuf[(buildnet_bakpacketplc + i) & (BUILDNET_BAKSIZ - 1)] = bufptr[i];
        buildnet_bakpacketplc = ((buildnet_bakpacketplc + messleng) & (BUILDNET_BAKSIZ - 1));
    }
    buildnet_outcntend[other]++;
    buildnet_lastsendtime[other] = totalclock;
    buildnet_dosendpackets(other);
}

extern "C" short getpacket(short* other, char* bufptr)
{
    if (numplayers < 2 || !other || !bufptr) return 0;
    faketimerhandler();

    for (long i = connecthead; i >= 0; i = connectpoint2[i])
    {
        if (i == myconnectindex) continue;
        if (totalclock < buildnet_lastsendtime[i]) buildnet_lastsendtime[i] = totalclock;
        if (totalclock > buildnet_lastsendtime[i] + buildnet_timeoutcount)
        {
            buildnet_errorgotnum[i] = buildnet_errorfixnum[i] + 1;
            if ((buildnet_outcntplc[i] == buildnet_outcntend[i]) && (buildnet_outcntplc[i] > 0))
            {
                buildnet_outcntplc[i]--;
                buildnet_lastsendtime[i] = totalclock;
            }
            else buildnet_lastsendtime[i] += buildnet_resendagaincount;
            buildnet_dosendpackets(i);
        }
    }

    if (buildnet_inlastpacket)
    {
        buildnet_inlastpacket = 0;
        *other = buildnet_lastpacketfrom;
        std::memcpy(bufptr, buildnet_lastpacket, static_cast<size_t>(buildnet_lastpacketleng));
        return buildnet_lastpacketleng;
    }

    buildnet::RawPacket rp;
    if (!buildnet::rawRecv(rp)) return 0;
    if (rp.len < 5) return 0;
    *other = rp.from;
    auto* b = reinterpret_cast<unsigned char*>(rp.data);
    long messleng = rp.len;
    unsigned short dacrc = static_cast<unsigned short>(b[messleng - 2] | (b[messleng - 1] << 8));
    if (dacrc != static_cast<unsigned short>(getcrc(rp.data, static_cast<short>(messleng - 2))))
    {
        buildnet_errorgotnum[*other] = buildnet_errorfixnum[*other] + 1;
        return 0;
    }

    while ((buildnet_errorfixnum[*other] & 7) != ((b[1] >> 3) & 7)) buildnet_errorfixnum[*other]++;
    if ((b[1] & 7) != (buildnet_errorresendnum[*other] & 7))
    {
        buildnet_errorresendnum[*other]++;
        buildnet_outcntplc[*other] = (buildnet_outcntend[*other] & 0xffffff00) + b[2];
        if (buildnet_outcntplc[*other] > buildnet_outcntend[*other]) buildnet_outcntplc[*other] -= 256;
    }

    if (b[0] != (buildnet_incnt[*other] & 255))
    {
        if (((buildnet_incnt[*other] - b[0]) & 255) > 32) buildnet_errorgotnum[*other] = buildnet_errorfixnum[*other] + 1;
        return 0;
    }

    if ((b[1] & 128) == 0)
    {
        messleng = rp.len - 5;
        if (messleng <= 0) return 0;
        std::memcpy(bufptr, &rp.data[3], static_cast<size_t>(messleng));
        buildnet_incnt[*other]++;
        return static_cast<short>(messleng);
    }

    long firstLen = b[3] | (b[4] << 8);
    if (firstLen < 0 || firstLen > rp.len - 7) return 0;
    buildnet_lastpacketleng = static_cast<short>(rp.len - 7 - firstLen);
    buildnet_lastpacketfrom = *other;
    buildnet_inlastpacket = 1;
    std::memcpy(bufptr, &rp.data[5], static_cast<size_t>(firstLen));
    std::memcpy(buildnet_lastpacket, &rp.data[firstLen + 5], static_cast<size_t>(buildnet_lastpacketleng));
    buildnet_incnt[*other] += 2;
    return static_cast<short>(firstLen);
}

extern "C" void flushpackets()
{
    buildnet_resetReliable();
    buildnet::queue.clear();
}

extern "C" void sendlogon() {}
extern "C" void sendlogoff()
{
    char tempbuf[2];
    tempbuf[0] = static_cast<char>(255);
    tempbuf[1] = static_cast<char>(myconnectindex);
    for (long i = connecthead; i >= 0; i = connectpoint2[i])
        if (i != myconnectindex) sendpacket(i, tempbuf, 2);
}
extern "C" long getoutputcirclesize()
{
    long total = 0;
    for (int i = 0; i < BUILDNET_MAXPLAYERS; ++i) total += buildnet_outcntend[i] - buildnet_outcntplc[i];
    return total;
}
extern "C" void genericmultifunction(long other, char* bufptr, long messleng, long command)
{
    if (numplayers < 2 || !bufptr || messleng <= 0) return;
    char out[BUILDNET_MAXPACKETSIZE] = {};
    out[0] = static_cast<char>(command & 255);
    long n = min<long>(messleng, BUILDNET_MAXPACKETSIZE - 1);
    std::memcpy(out + 1, bufptr, static_cast<size_t>(n));
    buildnet::rawSend(static_cast<short>(other), out, n + 1);
}
extern "C" void callcommit() {}


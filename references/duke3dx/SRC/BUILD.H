#ifndef BUILD_ENGINE_GL1_H
#define BUILD_ENGINE_GL1_H

#include <stddef.h>
#include <stdio.h>

#define MAXSECTORS 1024
#define MAXWALLS 8192
#define MAXSPRITES 4096

#define MAXTILES 6144
#define MAXSTATUS 1024
#define MAXPLAYERS 16
#define MAXXDIM 1600
#define MAXYDIM 1200
#define MAXPALOOKUPS 256
#define MAXPSKYTILES 256
#define MAXSPRITESONSCREEN 1024

#define CLIPMASK0 (((1L)<<16)+1L)
#define CLIPMASK1 (((256L)<<16)+64L)

#ifndef MAXPERMS
#define MAXPERMS 512
#endif
#ifndef MAXWALLSB
#define MAXWALLSB 2048
#endif
#ifndef MAXCLIPNUM
#define MAXCLIPNUM 512
#endif
#ifndef MAXVOXELS
#define MAXVOXELS 512
#endif
#ifndef MAXVOXMIPS
#define MAXVOXMIPS 5
#endif

#ifdef ENGINE
#define EXTERN
#else
#define EXTERN extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct
    {
        short wallptr, wallnum;
        long ceilingz, floorz;
        short ceilingstat, floorstat;
        short ceilingpicnum, ceilingheinum;
        signed char ceilingshade;
        char ceilingpal, ceilingxpanning, ceilingypanning;
        short floorpicnum, floorheinum;
        signed char floorshade;
        char floorpal, floorxpanning, floorypanning;
        char visibility, filler;
        short lotag, hitag, extra;
    } sectortype;

    typedef struct
    {
        long x, y;
        short point2, nextwall, nextsector, cstat;
        short picnum, overpicnum;
        signed char shade;
        char pal, xrepeat, yrepeat, xpanning, ypanning;
        short lotag, hitag, extra;
    } walltype;

    typedef struct
    {
        long x, y, z;
        short cstat, picnum;
        signed char shade;
        char pal, clipdist, filler;
        unsigned char xrepeat, yrepeat;
        signed char xoffset, yoffset;
        short sectnum, statnum;
        short ang, owner, xvel, yvel, zvel;
        short lotag, hitag, extra;
    } spritetype;

    typedef struct { long x1, y1, x2, y2; } linetype;

    /* Original BUILD globals. */
    EXTERN sectortype sector[MAXSECTORS];
    EXTERN walltype wall[MAXWALLS];
    EXTERN spritetype sprite[MAXSPRITES];

    EXTERN long spritesortcnt;
    EXTERN spritetype tsprite[MAXSPRITESONSCREEN];

    EXTERN char vidoption;
    EXTERN long xdim, ydim, ylookup[MAXYDIM + 1], numpages;
    EXTERN long yxaspect, viewingrange;

    EXTERN short numsectors, numwalls;
    EXTERN volatile long totalclock;
    EXTERN long numframes, randomseed;
    EXTERN short sintable[2048];
    EXTERN unsigned char palette[768];
    EXTERN short numpalookups;
    EXTERN char* palookup[MAXPALOOKUPS];
    EXTERN char parallaxtype, showinvisibility;
    EXTERN long parallaxyoffs, parallaxyscale;
    EXTERN long visibility, parallaxvisibility;

    EXTERN long windowx1, windowy1, windowx2, windowy2;
    EXTERN short startumost[MAXXDIM], startdmost[MAXXDIM];

    EXTERN short pskyoff[MAXPSKYTILES], pskybits;

    EXTERN short headspritesect[MAXSECTORS + 1], headspritestat[MAXSTATUS + 1];
    EXTERN short prevspritesect[MAXSPRITES], prevspritestat[MAXSPRITES];
    EXTERN short nextspritesect[MAXSPRITES], nextspritestat[MAXSPRITES];

    EXTERN short tilesizx[MAXTILES], tilesizy[MAXTILES];
    EXTERN char walock[MAXTILES];
    EXTERN long numtiles, picanm[MAXTILES], waloff[MAXTILES];

    EXTERN char show2dsector[(MAXSECTORS + 7) >> 3];
    EXTERN char show2dwall[(MAXWALLS + 7) >> 3];
    EXTERN char show2dsprite[(MAXSPRITES + 7) >> 3];
    EXTERN char automapping;

    EXTERN char gotpic[(MAXTILES + 7) >> 3];
    EXTERN char gotsector[(MAXSECTORS + 7) >> 3];

    /* Extra public compatibility globals exposed by the drop-in .cpp. */
    EXTERN long bytesperline, imageSize;
    EXTERN long frameplace, pageoffset, qsetmode;
    EXTERN long maxpages, activepage, visualpage, lockclock;
    EXTERN long beforedrawrooms, totalarea, transarea;
    EXTERN char picsiz[MAXTILES], tilefilenum[MAXTILES];
    EXTERN long tilefileoffs[MAXTILES];
    EXTERN long artversion, mapversion, artsize, cachesize;
    EXTERN char* transluc;
    EXTERN char paletteloaded;
    EXTERN unsigned char britable[16][64];
    EXTERN char textfont[1024], smalltextfont[1024];

    EXTERN short searchit;
    EXTERN long searchx, searchy;
    EXTERN short searchsector, searchwall, searchstat;

    EXTERN long globalposx, globalposy, globalposz, globalhoriz;
    EXTERN short globalang, globalcursectnum;
    EXTERN long cosglobalang, singlobalang;
    EXTERN long cosviewingrangeglobalang, sinviewingrangeglobalang;
    EXTERN long xyaspect, viewingrangerecip;
    EXTERN long xdimen, ydimen, halfxdimen, xdimenrecip, xdimenscale, xdimscale;
    EXTERN long globalpicnum, globalpal, globalshade, globalorientation;
    EXTERN long globalxpanning, globalypanning, globalyscale, globalzd, globalshiftval;
    EXTERN long globvis, globalvisibility, globalhisibility, globalpisibility, globalcisibility;
    EXTERN long wx1, wy1, wx2, wy2, viewoffset;

    EXTERN long stereowidth, stereopixelwidth, ostereopixelwidth;
    EXTERN volatile long stereomode, whiteband, blackband;
    EXTERN volatile char oa1, o3c2, ortca, ortcb, overtbits, laststereoint;

    EXTERN long voxoff[MAXVOXELS][MAXVOXMIPS];
    EXTERN long voxlock[MAXVOXELS][MAXVOXMIPS];

    EXTERN char pow2char[8];
    EXTERN long pow2long[32];

    /* Cache, GRP, rendering, geometry, timer, and multiplayer entry points. */
    void* kmalloc(size_t size);
    void kfree(void* buffer);
    void* kkmalloc(size_t size);
    void kkfree(void* buffer);
    long initgroupfile(char* filename);
    void uninitgroupfile(void);
    short kopen4load(char* filename, char searchfirst);
    short kopen4loadfrommod(char* filename, char searchfirst);
    long kread(short fil, void* buf, long len);
    long uncompress(char* lzwinbuf, long compleng, char* lzwoutbuf);
    long compress(char* lzwinbuf, long uncompleng, char* lzwoutbuf);
    long kdfread(void* buffer, long dasizeof, long count, short fil);
    long dfread(void* buffer, long dasizeof, long count, FILE* fil);
    long dfwrite(void* buffer, long dasizeof, long count, FILE* fil);
    void kclose(short fil);
    long klseek(short fil, long offset, long whence);
    long kfilelength(short fil);
    long ksqrt(long num);
    long krecip(long num);
    void initksqrt(void);
    void loadtables(void);
    void loadpalette(void);
    int loadpics(char* filename);
    void loadtile(short tilenume);
    long allocatepermanenttile(short tilenume, long xsiz, long ysiz);
    int loadboard(char* filename, long* daposx, long* daposy, long* daposz, short* daang, short* dacursectnum);
    int saveboard(char* filename, long* daposx, long* daposy, long* daposz, short* daang, short* dacursectnum);
    int setgamemode(char davidoption, long daxdim, long daydim);
    void initengine(void);
    void uninitengine(void);
    void drawrooms(long daposx, long daposy, long daposz, short daang, long dahoriz, short dacursectnum);
    void scansector(short sectnum);
    int wallfront(long l1, long l2);
    int spritewallfront(spritetype* s, long w);
    int bunchfront(long, long);
    void drawalls(long);
    void drawmasks(void);
    void nextpage(void);
    void prepwall(long, walltype*);
    void ceilscan(long, long, long sectnum);
    void florscan(long, long, long sectnum);
    void wallscan(long, long, short*, short*, long*, long*);
    void maskwallscan(long, long, short*, short*, long*, long*);
    void transmaskvline(long);
    void transmaskvline2(long);
    void transmaskwallscan(long, long);
    void drawmaskwall(short);
    void drawsprite(long);
    void hline(long, long);
    void slowhline(long, long);
    void grouscan(long, long, long sectnum, char dastat);
    void parascan(long, long, long sectnum, char dastat, long);
    void initspritelists(void);
    short insertspritesect(short sectnum);
    short insertspritestat(short statnum);
    short insertsprite(short sectnum, short statnum);
    int deletespritesect(short deleteme);
    int deletespritestat(short deleteme);
    int deletesprite(short spritenum);
    int changespritesect(short spritenum, short newsectnum);
    int changespritestat(short spritenum, short newstatnum);
    int setsprite(short spritenum, long newx, long newy, long newz);
    int inside(long x, long y, short sectnum);
    void FlushTextureCache();
    void updatesector(long x, long y, short* sectnum);
    long getceilzofslope(short sectnum, long dax, long day);
    long getflorzofslope(short sectnum, long dax, long day);
    void getzsofslope(short sectnum, long dax, long day, long* ceilz, long* florz);
    int getangle(long xvect, long yvect);
    int animateoffs(short tilenum, short fakevar);
    void rotatepoint(long xpivot, long ypivot, long x, long y, short daang, long* x2, long* y2);
    short lastwall(short point);
    void dragpoint(short pointhighlight_, long dax, long day);
    short sectorofwall(short theline);
    void alignceilslope(short dasect, long x, long y, long z);
    void alignflorslope(short dasect, long x, long y, long z);
    int clipinsidebox(long x, long y, short wallnum, long walldist);
    int clipinsideboxline(long x, long y, long x1, long y1, long x2, long y2, long walldist);
    int lintersect(long x1, long y1, long z1, long x2, long y2, long z2, long x3, long y3, long x4, long y4, long* intx, long* inty, long* intz);
    int rintersect(long x1, long y1, long z1, long vx, long vy, long vz, long x3, long y3, long x4, long y4, long* intx, long* inty, long* intz);
    int cansee(long x1, long y1, long z1, short sect1, long x2, long y2, long z2, short sect2);
    int hitscan(long xs, long ys, long zs, short sectnum, long vx, long vy, long vz, short* hitsect, short* hitwall, short* hitsprite, long* hitx, long* hity, long* hitz, unsigned long cliptype);
    int clipmove(long* x, long* y, long* z, short* sectnum, long xvect, long yvect, long walldist, long ceildist, long flordist, unsigned long cliptype);
    int pushmove(long* x, long* y, long* z, short* sectnum, long walldist, long ceildist, long flordist, unsigned long cliptype);
    void keepaway(long* x, long* y, long w);
    long raytrace(long x3, long y3, long* x4, long* y4);
    int neartag(long xs, long ys, long zs, short sectnum, short ange, short* neartagsector, short* neartagwall, short* neartagsprite, long* neartaghitdist, long neartagrange, char tagsearch);
    void getzrange(long x, long y, long z, short sectnum, long* ceilz, long* ceilhit, long* florz, long* florhit, long walldist, unsigned long cliptype);
    short nextsectorneighborz(short sectnum, long thez, short topbottom, short direction);
    void setview(long x1, long y1, long x2, long y2);
    void setaspect(long daxrange, long daaspect);
    void dosetaspect(void);
    void setactivepage(long dapage);
    void setvisualpage(long dapage);
    int setvesa(long daxdim, long daydim);
    void uninitvesa(void);
    void setvmode(int);
    void limitrate(void);
    void getvalidvesamodes(void);
    int VBE_setPalette(long start, long num, char* dapal);
    long FP_OFF(void* source);
//    unsigned char readpixel(long);
//    void drawpixel(long, unsigned char);
//    void koutp(long, long);
//    void koutpw(long, long);
//    long kinp(long);
    long kinpw(long);
    void clearview(long dacol);
    void clearallviews(long dacol);
    void plotpixel(long x, long y, char col);
    char getpixel(long, long);
    void drawline256(long x1, long y1, long x2, long y2, char col);
    void drawline16(long x1, long y1, long x2, long y2, char col);
    long readpixel16(long);
    int screencapture(char*, char);
    void rotatesprite(long sx, long sy, long z, short a, short picnum, signed char dashade, char dapalnum, char dastat, long cx1, long cy1, long cx2, long cy2);
    void dorotatesprite(long sx, long sy, long z, short a, short picnum, signed char dashade, char dapalnum, char dastat, long cx1, long cy1, long cx2, long cy2);
    void printext256(long xpos, long ypos, short col, short backcol, char name[82], char fontsize);
    void printext16(long xpos, long ypos, short col, short backcol, char name[82], char fontsize);
    void clear2dscreen(void);
    void draw2dgrid(long, long, short, long, short);
    void draw2dscreen(long posxe, long posye, short ange, long zoome, short gride);
    void drawmapview(long dax, long day, long zoome, short ang);
    void qsetmode640350(void);
    void qsetmode640480(void);
    void copytilepiece(long tilenume1, long sx1, long sy1, long xsiz, long ysiz, long tilenume2, long sx2, long sy2);
    void squarerotatetile(short tilenume);
    int getclosestcol(long r, long g, long b);
    void initfastcolorlookup(long, long, long);
    void makepalookup(long palnum, char* remapbuf, signed char r, signed char g, signed char b, char dastat);
    void setbrightness(char dabrightness, unsigned char* dapal);
    long getpalookup(long davis, long dashade);
    void setviewtotile(short tilenume, long xsiz, long ysiz);
    void setviewback(void);
    void preparemirror(long dax, long day, long daz, short daang, long dahoriz, short dawall, short dasector, long* tposx, long* tposy, short* tang);
    void completemirror(void);
    void flushperms(void);
    int clippoly4(long, long, long, long);
    int clippoly(long npoints, long);
    void fillpolygon(long);
    int owallmost(short* mostbuf, long, long);
    int wallmost(short* mostbuf, long, long, char);
    int initmouse(void);
    void getmousevalues(short* mousx, short* mousy, short* bstatus);
    void printscreeninterrupt(void);
    int krand(void);
    void stereonextpage(void);
    void setstereo(long dastereomode);
    int qloadkvx(long, char*);
    void drawvox(long, long, long, long, long, long, char, signed char, char, long*, long*);
    void ceilspritescan(long, long);
    void ceilspritehline(long, long);
    void setgotpic(long tilenume);
    void reportandexit(char* errormessage);
    void initcache(long dacachestart, long dacachesize);
    void allocache(long* newhandle, long newbytes, char* newlockptr);
    void suckcache(long* suckptr);
    void agecache(void);
    void uninitcache(long all);
    long getcachecount(void);
    void inittimerhandler(void);
    void uninittimerhandler(void);
    void faketimerhandler(void);
    void initcrc(void);
    long getcrc(char* buffer, short bufleng);
    void setpackettimeout(long datimeoutcount, long daresendagaincount);
    void setsocket(short newsocket);
    int setplayeraddress(short player, char* host, short port);
    void initmultiplayers(char damultioption, char dacomrateoption, char dapriority);
    void uninitmultiplayers(void);
    void sendpacket(long other, char* bufptr, long messleng);
    short getpacket(short* other, char* bufptr);
    void flushpackets(void);
    void sendlogon(void);
    void sendlogoff(void);
    long getoutputcirclesize(void);
    void genericmultifunction(long other, char* bufptr, long messleng, long command);
    void callcommit(void);
    void inittimerhandler();
    void uninittimerhandler();
#ifdef __cplusplus
} /* extern "C" */
#endif

#if defined(__cplusplus) && (defined(_WIN32) || defined(_MSC_VER))
static_assert(sizeof(sectortype) == 40, "sectortype must match BUILD v7 map layout");
static_assert(sizeof(walltype) == 32, "walltype must match BUILD v7 map layout");
static_assert(sizeof(spritetype) == 44, "spritetype must match BUILD v7 map layout");
#endif

#undef EXTERN

#endif /* BUILD_ENGINE_GL1_H */

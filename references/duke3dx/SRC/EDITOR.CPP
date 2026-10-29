/*
=================================================
BuildX Editor for Duke Nukem 3D
Copyright (C) 2026 Justin Marshall

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
=================================================
*/

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <objidl.h>
#include <gdiplus.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <array>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <utility>
#include <cwchar>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "gdiplus.lib")

#include "BUILD.H"

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

// Build.cpp currently declares these but does not provide game/editor-specific bodies.
extern "C" { long lockclock = 0; }
void _faketimerhandler(void) {}

// -----------------------------------------------------------------------------
// IDs and constants.
// -----------------------------------------------------------------------------
namespace ids
{
    enum : int
    {
        TIMER_RENDER = 1,
        TIMER_2D_REFRESH = 2,
        ID_TAB = 100,
        ID_VIEW2D = 101,
        ID_HOST3D = 102,
        ID_TILEART = 103,
        ID_GRPEDIT = 104,
        ID_TEXTUREBAR = 105,
        ID_INSPECTOR = 106,

        ID_INSP_TILE_EDIT = 1100,
        ID_INSP_SHADE_EDIT,
        ID_INSP_PAL_EDIT,
        ID_INSP_VIS_EDIT,
        ID_INSP_LOTAG_EDIT,
        ID_INSP_HITAG_EDIT,
        ID_INSP_X_EDIT,
        ID_INSP_Y_EDIT,
        ID_INSP_Z_EDIT,
        ID_INSP_USE_TILE,
        ID_INSP_APPLY,
        ID_INSP_REFRESH,
        ID_INSP_TYPE_EDIT,
        ID_INSP_OBJ_HITAG_EDIT,
        ID_INSP_EXTRA_EDIT,
        ID_INSP_CSTAT_EDIT,
        ID_INSP_STATNUM_EDIT,
        ID_INSP_ANG_EDIT,
        ID_INSP_BLOOD_PRESET,
        ID_INSP_BLOOD_FIXUP,

        ID_ART_TILE_EDIT = 1300,
        ID_ART_USE_SELECTED,
        ID_ART_IMPORT_PNG,
        ID_ART_EXPORT_PNG,
        ID_ART_SAVE_ART,
        ID_ART_RELOAD_GRP_PALETTE,

        ID_GRP_LIST = 1400,
        ID_GRP_SELECT_GRP,
        ID_GRP_EXPORT_PATH_EDIT,
        ID_GRP_CHANGE_EXPORT_PATH,
        ID_GRP_EXTRACT_SELECTED,
        ID_GRP_EXTRACT_ALL,
        ID_GRP_IMPORT_FILES,
        ID_GRP_REFRESH,

        ID_CTX_MOVE_CAMERA_HERE = 1500,
        ID_CTX_PLACE_START_HERE,
        ID_CTX_ADD_POINT_HERE,

        M_FILE_NEW = 40001,
        M_FILE_OPEN,
        M_FILE_OPEN_GRP,
        M_FILE_SAVE,
        M_FILE_SAVE_AS,
        M_FILE_RUN_MAP,
        M_FILE_EXIT,

        M_VIEW_2D,
        M_VIEW_3D,
        M_VIEW_TILE_ART,
        M_VIEW_GRPEDIT,
        M_VIEW_ZOOM_IN,
        M_VIEW_ZOOM_OUT,
        M_VIEW_GRID,
        M_VIEW_GRID_LOCK,
        M_VIEW_SHOW_TAGS,

        M_EDIT_BEGIN_DRAW,
        M_EDIT_CLOSE_DRAW,
        M_EDIT_CANCEL_DRAW,
        M_EDIT_INSERT_POINT,
        M_EDIT_INSERT_SPRITE,
        M_EDIT_DELETE,
        M_EDIT_COPY_PROPS,
        M_EDIT_PASTE_PROPS,
        M_EDIT_APPLY_TEXTURE,
        M_EDIT_PROPERTIES,

        M_TOOLS_RELINK,
        M_TOOLS_SET_START,
        M_TOOLS_TOGGLE_BLOCKING,
        M_TOOLS_TOGGLE_ONEWAY,
        M_TOOLS_TOGGLE_MASKED,
        M_TOOLS_TOGGLE_TRANSLUCENT,
        M_TOOLS_TOGGLE_PARALLAX,
        M_TOOLS_SHADE_UP,
        M_TOOLS_SHADE_DOWN,
        M_TOOLS_Z_UP,
        M_TOOLS_Z_DOWN,
        M_BLOOD_SPRITE_PRESET,
        M_BLOOD_FIXUP_SELECTION,
        M_BLOOD_FIXUP_MAP,
        M_TEXTURE_GOTO,
        M_TEXTURE_TOGGLE_SIZE,
        M_HELP_KEYS,
    };
}

static constexpr int TEXTURE_BAR_DEFAULT_HEIGHT = 138;
static constexpr int TEXTURE_BAR_EXPANDED_HEIGHT = 312;
static constexpr int TEXTURE_BAR_MIN_HEIGHT = 48;
static constexpr int TEXTURE_BAR_HEIGHT = TEXTURE_BAR_DEFAULT_HEIGHT; // Kept for default creation size.
static constexpr int TAB_MARGIN = 4;
static constexpr int TILE_CELL = 72;
static constexpr int TILE_PREVIEW = 56;
static constexpr int BUILD_ANG_MAX = 2048;

// Original searchstat meanings retained where useful.
enum SearchStat : int
{
    SS_NONE = -1,
    SS_WALL = 0,
    SS_CEILING = 1,
    SS_FLOOR = 2,
    SS_SPRITE = 3,
    SS_MASKED_WALL = 4,
};


enum class GameProfile : int
{
    Duke3D = 0,
    ShadowWarrior = 1,
    Blood = 2,
};

struct Selection
{
    int stat = SS_NONE;
    short sector = -1;
    short wall = -1;       // Also stores sprite index for SS_SPRITE, matching old editor searchwall.
    bool valid() const { return stat >= 0; }
    void clear() { stat = SS_NONE; sector = -1; wall = -1; }
};

struct CopiedProps
{
    bool valid = false;
    int stat = SS_NONE;
    short picnum = 0;
    short cstat = 0;
    short lotag = 0, hitag = 0, extra = -1;
    signed char shade = 0;
    char pal = 0;
    char visibility = 0;
    unsigned char xrepeat = 8, yrepeat = 8;
    char xpanning = 0, ypanning = 0;
};

struct CameraState
{
    long x = 32768;
    long y = 32768;
    long z = 0;
    long horiz = 100;
    short ang = 1536;
    short sect = -1;
};

struct TileOverride
{
    std::vector<unsigned char> pixels; // BUILD/ART column-major 8-bit palette indexes.
    short w = 0;
    short h = 0;
    long picanmValue = 0;
    bool dirty = false;
};

struct AppState
{
    HINSTANCE inst = nullptr;
    HWND main = nullptr;
    HWND tab = nullptr;
    HWND view2d = nullptr;
    HWND host3d = nullptr;
    HWND tileArt = nullptr;
    HWND grpEdit = nullptr;
    HWND textureBar = nullptr;
    HWND inspector = nullptr;
    HWND inspSelectionGroup = nullptr;
    HWND inspSurfaceGroup = nullptr;
    HWND inspPositionGroup = nullptr;
    HWND inspSectorGroup = nullptr;
    HWND inspTitle = nullptr;
    HWND inspSurfaceTitle = nullptr;
    HWND inspTileEdit = nullptr;
    HWND inspShadeEdit = nullptr;
    HWND inspPalEdit = nullptr;
    HWND inspXEdit = nullptr;
    HWND inspYEdit = nullptr;
    HWND inspZEdit = nullptr;
    HWND inspSectorTitle = nullptr;
    HWND inspVisEdit = nullptr;
    HWND inspLotagEdit = nullptr;
    HWND inspHitagEdit = nullptr;
    HWND inspBloodGroup = nullptr;
    HWND inspBloodTitle = nullptr;
    HWND inspTypeEdit = nullptr;
    HWND inspObjHitagEdit = nullptr;
    HWND inspExtraEdit = nullptr;
    HWND inspCstatEdit = nullptr;
    HWND inspStatnumEdit = nullptr;
    HWND inspAngEdit = nullptr;
    HWND inspBloodPresetButton = nullptr;
    HWND inspBloodFixupButton = nullptr;
    HWND inspUseTileButton = nullptr;
    HWND inspApplyButton = nullptr;
    HWND inspRefreshButton = nullptr;
    HWND artTileEdit = nullptr;
    HWND artUseSelectedButton = nullptr;
    HWND artImportButton = nullptr;
    HWND artExportButton = nullptr;
    HWND artSaveButton = nullptr;
    HWND artPaletteButton = nullptr;
    HWND artStatusLabel = nullptr;
    HWND grpPathLabel = nullptr;
    HWND grpList = nullptr;
    HWND grpSelectButton = nullptr;
    HWND grpExportPathEdit = nullptr;
    HWND grpChangeExportPathButton = nullptr;
    HWND grpExtractButton = nullptr;
    HWND grpExtractAllButton = nullptr;
    HWND grpImportButton = nullptr;
    HWND grpRefreshButton = nullptr;
    HFONT inspFont = nullptr;
    HFONT inspBoldFont = nullptr;
    HWND engine = nullptr;
    WNDPROC oldEngineProc = nullptr;
    WNDPROC oldTabProc = nullptr;

    GameProfile game = GameProfile::Duke3D;
    std::string bloodDataPath;
    std::string bloodRffPath;
    std::string bloodExtractPath;
    bool bloodRffPrepared = false;
    std::string bloodTempArtPath;
    std::vector<std::string> bloodTempFiles;

    bool engineCreated = false;
    bool running = true;
    bool dirty = false;
    bool artDirty = false;
    bool showTags = true;
    bool gridLock = true;
    int grid = 3;
    int activeTab = 0; // 0 = 2D, 1 = 3D, 2 = tile art, 3 = GRP edit.

    std::string boardPath = "newboard.map";
    std::string dukeGrpPath;
    std::string grpEditPath;
    std::string grpExportPath;
    bool boardFromGrp = false;
    std::string boardGrpPath;
    std::string boardGrpEntry;
    std::string status = "Ready.";

    CameraState cam;
    long startx = 32768, starty = 32768, startz = 0;
    short startang = 1536, startsect = -1;

    long viewX = 32768;
    long viewY = 32768;
    long zoom = 768;

    Selection sel;
    CopiedProps clip;

    short selectedTile = 0;
    int tileScroll = 0;
    std::vector<short> tileList;
    std::unordered_map<int, TileOverride> tileOverrides;
    bool textureBarExpanded = false;

    bool keys[256] = {};
    bool panning2D = false;
    bool panning2DMoved = false;
    bool draggingPoint = false;
    bool draggingSprite = false;
    bool orbit3D = false;
    bool dragPointMoved = false;
    bool dragSpriteMoved = false;
    bool dragDirtySet = false;
    bool pending2DInvalidate = false;
    bool inspectorUpdating = false;
    DWORD last2DInvalidateTick = 0;
    POINT lastMouse = {};
    POINT rightClickStart = {};
    POINT rightClickMap = {};
    short dragPointWall = -1;
    short dragSprite = -1;
    long dragOldX = 0, dragOldY = 0;

    bool drawingSector = false;
    std::vector<POINT> drawPts; // map coordinates in x/y.
    POINT lastMapMouse = { 0, 0 };

    LARGE_INTEGER freq = {};
    LARGE_INTEGER lastTick = {};
    ULONG_PTR gdiplusToken = 0;
};

static AppState g;


// -----------------------------------------------------------------------------
// Game profile and Blood metadata.
// -----------------------------------------------------------------------------
static const char* gameProfileName(GameProfile profile)
{
    switch (profile)
    {
    case GameProfile::Duke3D: return "Duke Nukem 3D";
    case GameProfile::ShadowWarrior: return "Shadow Warrior";
    case GameProfile::Blood: return "Blood";
    default: return "Unknown";
    }
}

static bool isBloodMode()
{
    return g.game == GameProfile::Blood;
}

static bool isDukeMode()
{
    return g.game == GameProfile::Duke3D;
}

struct BloodName
{
    int id;
    const char* name;
};

struct BloodSpriteDefault
{
    short type;
    short picnum;
    short xrepeat;
    short yrepeat;
    signed char cstat256; // -1 leave as-is, 0 clear blocking-hitscan bit, 1 set it.
    short pal;
};

static const BloodName kBloodSpriteTypeNames[] =
{
    { 0, "Decoration" },
    { 1, "Player Start" },
    { 2, "Bloodbath Start" },
    { 3, "Off marker" },
    { 4, "On marker" },
    { 5, "Axis marker" },
    { 6, "Lower link" },
    { 7, "Upper link" },
    { 8, "Teleport target" },
    { 10, "Lower water" },
    { 9, "Upper water" },
    { 12, "Lower stack" },
    { 11, "Upper stack" },
    { 14, "Lower goo" },
    { 13, "Upper goo" },
    { 15, "Path marker" },
    { 16, "Alignable Region" },
    { 17, "Base Region" },
    { 18, "Dude Spawn" },
    { 19, "Earthquake" },
    { 20, "Toggle switch" },
    { 21, "1-Way switch" },
    { 22, "Combination switch" },
    { 23, "Padlock (1-shot)" },
    { 30, "Torch" },
    { 32, "Candle" },
    { 40, "Sprite type 40" },
    { 47, "Sprite type 47" },
    { 43, "Sprite type 43" },
    { 41, "Sprite type 41" },
    { 42, "Sprite type 42" },
    { 46, "Sprite type 46" },
    { 49, "Sprite type 49" },
    { 48, "Sprite type 48" },
    { 45, "Sprite type 45" },
    { 50, "Sprite type 50" },
    { 44, "Sprite type 44" },
    { 60, "Sprite type 60" },
    { 62, "Sprite type 62" },
    { 63, "Sprite type 63" },
    { 64, "Sprite type 64" },
    { 65, "Sprite type 65" },
    { 67, "Sprite type 67" },
    { 68, "Sprite type 68" },
    { 69, "Sprite type 69" },
    { 70, "Sprite type 70" },
    { 72, "Sprite type 72" },
    { 73, "Sprite type 73" },
    { 76, "Sprite type 76" },
    { 79, "Sprite type 79" },
    { 66, "Sprite type 66" },
    { 80, "Random Ammo" },
    { 100, "Sprite type 100" },
    { 101, "Sprite type 101" },
    { 102, "Sprite type 102" },
    { 103, "Sprite type 103" },
    { 104, "Sprite type 104" },
    { 105, "Sprite type 105" },
    { 106, "Sprite type 106" },
    { 107, "Sprite type 107" },
    { 108, "Sprite type 108" },
    { 109, "Sprite type 109" },
    { 110, "Sprite type 110" },
    { 111, "Sprite type 111" },
    { 112, "Sprite type 112" },
    { 113, "Sprite type 113" },
    { 114, "Sprite type 114" },
    { 115, "Sprite type 115" },
    { 116, "Sprite type 116" },
    { 117, "Sprite type 117" },
    { 118, "Sprite type 118" },
    { 119, "Sprite type 119" },
    { 120, "Sprite type 120" },
    { 121, "Sprite type 121" },
    { 122, "Sprite type 122" },
    { 123, "Sprite type 123" },
    { 124, "Sprite type 124" },
    { 125, "Sprite type 125" },
    { 126, "Sprite type 126" },
    { 127, "Sprite type 127" },
    { 128, "Sprite type 128" },
    { 129, "Sprite type 129" },
    { 130, "Sprite type 130" },
    { 131, "Sprite type 131" },
    { 132, "Sprite type 132" },
    { 133, "Sprite type 133" },
    { 134, "Sprite type 134" },
    { 135, "Sprite type 135" },
    { 136, "Sprite type 136" },
    { 137, "Sprite type 137" },
    { 138, "Sprite type 138" },
    { 139, "Sprite type 139" },
    { 140, "Sprite type 140" },
    { 141, "Sprite type 141" },
    { 142, "Sprite type 142" },
    { 143, "Sprite type 143" },
    { 144, "Sprite type 144" },
    { 145, "Sprite type 145" },
    { 146, "Sprite type 146" },
    { 200, "Random Creature" },
    { 201, "Cultist w/Tommy" },
    { 202, "Cultist w/Shotgun" },
    { 247, "Cultist w/Tesla" },
    { 248, "Cultist w/Dynamite" },
    { 249, "Beast Cultist" },
    { 250, "Tiny Caleb" },
    { 251, "Beast" },
    { 203, "Axe Zombie" },
    { 204, "Fat Zombie" },
    { 205, "Earth Zombie" },
    { 244, "Sleep Zombie" },
    { 245, "Innocent" },
    { 206, "Flesh Gargoyle" },
    { 207, "Stone Gargoyle" },
    { 208, "Flesh Statue" },
    { 209, "Stone Statue" },
    { 210, "Phantasm" },
    { 211, "Hound" },
    { 212, "Hand" },
    { 213, "Brown Spider" },
    { 214, "Red Spider" },
    { 216, "Mother Spider" },
    { 215, "Black Spider" },
    { 217, "GillBeast" },
    { 218, "Eel" },
    { 219, "Bat" },
    { 220, "Rat" },
    { 221, "Green Pod" },
    { 222, "Green Tentacle" },
    { 223, "Fire Pod" },
    { 224, "Fire Tentacle" },
    { 225, "Mother Pod" },
    { 226, "Mother Tentacle" },
    { 227, "Cerberus" },
    { 229, "Tchernobog" },
    { 230, "TCultist prone" },
    { 246, "SCultist prone" },
    { 400, "TNT Barrel" },
    { 401, "Armed Prox Bomb" },
    { 402, "Armed Remote" },
    { 403, "Blue Vase" },
    { 404, "Brown Vase" },
    { 405, "Crate Face" },
    { 406, "Glass Window" },
    { 407, "Fluorescent Light" },
    { 408, "Wall Crack" },
    { 409, "Wood Beam" },
    { 410, "Spider's Web" },
    { 411, "MetalGrate1" },
    { 412, "FlammableTree" },
    { 413, "Machine Gun" },
    { 414, "Falling Rock" },
    { 415, "Kickable Pail" },
    { 416, "Gib Object" },
    { 417, "Explode Object" },
    { 427, "Zombie Head" },
    { 450, "Spike Trap" },
    { 451, "Rock Trap" },
    { 452, "Flame Trap" },
    { 454, "Saw Blade" },
    { 455, "Electric Zap" },
    { 456, "Switched Zap" },
    { 457, "Pendulum" },
    { 458, "Guillotine" },
    { 459, "Hidden Exploder" },
    { 700, "Trigger Gen" },
    { 701, "WaterDrip Gen" },
    { 702, "BloodDrip Gen" },
    { 703, "Fireball Gen" },
    { 704, "EctoSkull Gen" },
    { 705, "Dart Gen" },
    { 706, "Bubble Gen" },
    { 707, "Multi-Bubble Gen" },
    { 708, "SFX Gen" },
    { 709, "Sector SFX" },
    { 710, "Ambient SFX" },
    { 711, "Player SFX" },
};

static const BloodName kBloodWallTypeNames[] =
{
    { 0, "Normal" },
    { 20, "Toggle switch" },
    { 21, "1-Way switch" },
    { 500, "Wall Link" },
    { 501, "Wall Stack (unsupp.)" },
    { 511, "Gib Wall" },
};

static const BloodName kBloodSectorTypeNames[] =
{
    { 0, "Normal" },
    { 600, "Z Motion" },
    { 602, "Z Motion SPRITE" },
    { 603, "Warp" },
    { 604, "Teleporter" },
    { 614, "Slide Marked" },
    { 615, "Rotate Marked" },
    { 616, "Slide" },
    { 617, "Rotate" },
    { 613, "Step Rotate" },
    { 612, "Path Sector" },
    { 618, "Damage Sector" },
    { 619, "Counter Sector" },
};

static const BloodName kBloodTransitionWaveNames[] =
{
    { 1, "Linear" },
    { 0, "Sine" },
    { 2, "SlowOff" },
    { 3, "SlowOn" },
};

static const BloodName kBloodWaveNames[] =
{
    { 0, "None" },
    { 1, "Square" },
    { 2, "Saw" },
    { 3, "Ramp up" },
    { 4, "Ramp down" },
    { 5, "Sine" },
    { 6, "Flicker1" },
    { 7, "Flicker2" },
    { 8, "Flicker3" },
    { 9, "Flicker4" },
    { 10, "Strobe" },
    { 11, "Search" },
};

static const BloodName kBloodCommandNames[] =
{
    { 0, "OFF" },
    { 1, "ON" },
    { 2, "State" },
    { 3, "Toggle" },
    { 4, "!State" },
    { 5, "Link" },
    { 6, "Lock" },
    { 7, "Unlock" },
    { 8, "Toggle Lock" },
    { 9, "Stop OFF" },
    { 10, "Stop ON" },
    { 11, "Stop Next" },
};

static const BloodName kBloodRespawnNames[] =
{
    { 0, "Optional" },
    { 1, "Never" },
    { 2, "Always" },
    { 3, "Permanent" },
};

static const BloodSpriteDefault kBloodSpriteDefaults[] =
{
    { 1, -1, -1, -1, 0, 0 },
    { 2, -1, -1, -1, 0, 5 },
    { 6, 2331, 64, 64, 0, 0 },
    { 7, 2332, 64, 64, 0, 0 },
    { 10, 2331, 64, 64, 0, 0 },
    { 9, 2332, 64, 64, 0, 0 },
    { 12, 2331, 64, 64, 0, 0 },
    { 11, 2332, 64, 64, 0, 0 },
    { 14, 2331, 64, 64, 0, 0 },
    { 13, 2332, 64, 64, 0, 0 },
    { 15, 2319, 64, 64, 0, 0 },
    { 18, 2077, 64, 64, 0, 0 },
    { 19, 2072, 64, 64, 0, 0 },
    { 20, -1, -1, -1, 1, -1 },
    { 21, -1, -1, -1, 1, -1 },
    { 22, -1, -1, -1, 1, -1 },
    { 23, 948, -1, -1, 1, -1 },
    { 30, 550, -1, -1, 1, -1 },
    { 30, 572, -1, -1, 1, -1 },
    { 30, 560, -1, -1, 1, -1 },
    { 30, 564, -1, -1, 1, -1 },
    { 30, 570, -1, -1, 1, -1 },
    { 30, 554, -1, -1, 1, -1 },
    { 32, 938, -1, -1, 1, -1 },
    { 40, 832, 48, 48, 0, 0 },
    { 43, 524, 48, 48, 0, 0 },
    { 41, 559, 48, 48, 0, 0 },
    { 42, 558, 48, 48, 0, 0 },
    { 46, 526, 48, 48, 0, 0 },
    { 45, 539, 48, 48, 0, 0 },
    { 50, 800, 48, 48, 0, 0 },
    { 60, 618, 40, 40, 1, 0 },
    { 62, 589, 48, 48, 1, 0 },
    { 63, 809, 48, 48, 1, 0 },
    { 64, 811, 40, 40, 0, 0 },
    { 65, 810, 40, 40, 0, 0 },
    { 66, 820, 24, 24, 0, 0 },
    { 67, 619, 48, 48, 0, 0 },
    { 68, 812, 48, 48, 0, 0 },
    { 69, 813, 48, 48, 0, 0 },
    { 70, 525, 48, 48, 0, 0 },
    { 72, 817, 48, 48, 0, 0 },
    { 73, 548, 24, 24, 0, 0 },
    { 76, 816, 48, 48, 0, 0 },
    { 79, 801, 48, 48, 0, 0 },
    { 80, 832, 40, 40, 0, 0 },
    { 100, 2552, 32, 32, 0, 0 },
    { 101, 2553, 32, 32, 0, 0 },
    { 102, 2554, 32, 32, 0, 0 },
    { 103, 2555, 32, 32, 0, 0 },
    { 104, 2556, 32, 32, 0, 0 },
    { 105, 2557, 32, 32, 0, 0 },
    { 106, -1, -1, -1, 0, 0 },
    { 107, 519, 48, 48, 0, 0 },
    { 108, 822, 40, 40, 0, 0 },
    { 109, 2169, 40, 40, 0, 0 },
    { 110, 2433, 40, 40, 0, 0 },
    { 113, 896, 40, 40, 0, 0 },
    { 114, 825, 40, 40, 0, 0 },
    { 115, 827, 40, 40, 0, 0 },
    { 117, 829, 40, 40, 0, 0 },
    { 118, 830, 80, 64, 0, 0 },
    { 121, 760, 40, 40, 0, 0 },
    { 124, 2428, 40, 40, 0, 0 },
    { 125, 839, 40, 40, 0, 0 },
    { 127, 840, 48, 48, 0, 0 },
    { 128, 841, 48, 48, 0, 0 },
    { 129, 842, 48, 48, 0, 0 },
    { 130, 843, 48, 48, 0, 0 },
    { 136, 518, 40, 40, 0, 0 },
    { 137, 522, 40, 40, 0, 0 },
    { 138, 523, 40, 40, 0, 0 },
    { 140, 2628, 64, 64, 0, 0 },
    { 141, 2586, 64, 64, 0, 0 },
    { 142, 2578, 64, 64, 0, 0 },
    { 143, 2602, 64, 64, 0, 0 },
    { 144, 2594, 64, 64, 0, 0 },
    { 144, 2594, 64, 64, 0, 0 },
    { 145, -1, 64, 64, 1, 0 },
    { 146, -1, 64, 64, 1, 0 },
    { 200, 832, 64, 64, 1, 0 },
    { 201, 2820, 40, 40, 1, 3 },
    { 202, 2825, 40, 40, 1, 0 },
    { 247, 2820, 40, 40, 1, 11 },
    { 248, 2820, 40, 40, 1, 13 },
    { 249, 2825, 48, 48, 1, 12 },
    { 250, 3870, 16, 16, 1, 12 },
    { 251, 2960, 48, 48, 1, 0 },
    { 203, 1170, 40, 40, 1, 0 },
    { 204, 1370, 48, 48, 1, 0 },
    { 205, 3054, 40, 40, 1, 0 },
    { 244, 1209, 40, 40, 1, 0 },
    { 245, 3798, 40, 40, 1, 0 },
    { 206, 1470, 40, 40, 1, 0 },
    { 207, 1470, 40, 40, 1, 5 },
    { 208, 1530, 40, 40, 1, 0 },
    { 209, 1530, 40, 40, 1, 5 },
    { 210, 3060, 40, 40, 1, 0 },
    { 211, 1270, 40, 40, 1, 0 },
    { 212, 1980, 32, 32, 1, 0 },
    { 213, 1920, 16, 16, 1, 7 },
    { 214, 1925, 24, 24, 1, 4 },
    { 216, 1930, 40, 40, 1, 0 },
    { 215, 1935, 32, 32, 1, 4 },
    { 217, 1570, 48, 48, 1, 0 },
    { 218, 1870, 32, 32, 1, 0 },
    { 219, 1948, 32, 32, 1, 0 },
    { 220, 1745, 24, 24, 1, 0 },
    { 221, 1792, 32, 32, 1, 0 },
    { 222, 1797, 32, 32, 1, 0 },
    { 223, 1792, 48, 48, 1, 2 },
    { 224, 1797, 48, 48, 1, 2 },
    { 225, 1792, 64, 64, 1, 6 },
    { 226, 1797, 64, 64, 1, 6 },
    { 227, 2680, 64, 64, 1, 0 },
    { 229, 3140, 64, 64, 1, 0 },
    { 230, 3385, 40, 40, 1, 3 },
    { 231, 2860, 40, 40, 1, 0 },
    { 232, 2860, 40, 40, 1, 0 },
    { 233, 2860, 40, 40, 1, 0 },
    { 234, 2860, 40, 40, 1, 0 },
    { 235, 2860, 40, 40, 1, 0 },
    { 236, 2860, 40, 40, 1, 0 },
    { 237, 2860, 40, 40, 1, 0 },
    { 238, 2860, 40, 40, 1, 0 },
    { 243, 2860, 40, 40, 1, 0 },
    { 246, 3385, 40, 40, 1, 0 },
    { 400, 907, 64, 64, 1, 0 },
    { 401, 3444, 40, 40, 1, 0 },
    { 402, 3457, 40, 40, 1, 0 },
    { 403, 739, -1, -1, 1, 0 },
    { 404, 642, -1, -1, 1, 0 },
    { 405, 462, -1, -1, 1, 0 },
    { 406, 266, -1, -1, 1, 0 },
    { 407, 796, -1, -1, 1, 0 },
    { 408, -1, -1, -1, 0, -1 },
    { 409, 1142, -1, -1, 1, 0 },
    { 410, 1069, -1, -1, 1, 0 },
    { 411, 483, -1, -1, 1, -1 },
    { 412, -1, -1, -1, 1, -1 },
    { 413, -1, 64, 64, 0, 0 },
    { 414, -1, -1, -1, 1, 0 },
    { 415, -1, 48, 48, 1, 0 },
    { 416, -1, -1, -1, -1, -1 },
    { 417, -1, -1, -1, -1, -1 },
    { 427, -1, 40, 40, 1, -1 },
    { 450, 968, 64, 64, 0, 0 },
    { 451, -1, 64, 64, 0, 0 },
    { 452, 2183, -1, -1, 0, 0 },
    { 454, 655, -1, -1, 0, 0 },
    { 455, 1156, -1, -1, 0, 0 },
    { 456, 1156, -1, -1, 0, 0 },
    { 457, 1080, -1, -1, 0, 0 },
    { 458, 835, -1, -1, 0, 0 },
    { 459, 908, 4, -1, 0, 0 },
    { 708, 2519, 64, 64, 0, 0 },
    { 709, 2520, 64, 64, 0, 0 },
    { 710, 2521, 64, 64, 0, 0 },
    { 711, 2519, 64, 64, 0, 5 },
};


static const char* bloodLookupName(const BloodName* table, size_t count, int id, const char* fallback = nullptr)
{
    if (table)
    {
        for (size_t i = 0; i < count; ++i)
            if (table[i].id == id) return table[i].name;
    }
    return fallback ? fallback : "Unknown";
}

#define BLOOD_COUNT_OF(a) (sizeof(a) / sizeof((a)[0]))

static const char* bloodSpriteTypeName(int type)
{
    return bloodLookupName(kBloodSpriteTypeNames, BLOOD_COUNT_OF(kBloodSpriteTypeNames), type, "Decoration");
}

static const char* bloodWallTypeName(int type)
{
    return bloodLookupName(kBloodWallTypeNames, BLOOD_COUNT_OF(kBloodWallTypeNames), type, "Normal");
}

static const char* bloodSectorTypeName(int type)
{
    return bloodLookupName(kBloodSectorTypeNames, BLOOD_COUNT_OF(kBloodSectorTypeNames), type, "Normal");
}

static const BloodSpriteDefault* bloodSpriteDefaultByTypeOrPic(short type, short picnum)
{
    const BloodSpriteDefault* byType = nullptr;
    const BloodSpriteDefault* byPic = nullptr;
    for (size_t i = 0; i < BLOOD_COUNT_OF(kBloodSpriteDefaults); ++i)
    {
        const BloodSpriteDefault& def = kBloodSpriteDefaults[i];
        if (!byType && def.type == type) byType = &def;
        if (!byPic && def.picnum >= 0 && def.picnum == picnum) byPic = &def;
        if (def.type == type && def.picnum >= 0 && def.picnum == picnum) return &def;
    }
    return byType ? byType : byPic;
}

static std::string bloodTypeLabel(const char* kind, int type, const char* name)
{
    char buf[192];
    std::snprintf(buf, sizeof(buf), "%s type %d: %s", kind ? kind : "Blood", type, name ? name : "Unknown");
    return buf;
}


// -----------------------------------------------------------------------------
// Dark theme helpers.
// -----------------------------------------------------------------------------
namespace theme
{
    static const COLORREF WindowBg = RGB(18, 18, 22);
    static const COLORREF PanelBg = RGB(28, 29, 34);
    static const COLORREF CardBg = RGB(34, 36, 42);
    static const COLORREF HeaderBg = RGB(45, 49, 58);
    static const COLORREF EditBg = RGB(22, 23, 27);
    static const COLORREF ButtonBg = RGB(48, 51, 60);
    static const COLORREF ButtonHotBg = RGB(58, 62, 72);
    static const COLORREF Border = RGB(74, 78, 88);
    static const COLORREF Text = RGB(235, 237, 242);
    static const COLORREF MutedText = RGB(168, 174, 184);
    static const COLORREF Accent = RGB(255, 210, 96);
}

static HBRUSH gThemeWindowBrush = nullptr;
static HBRUSH gThemePanelBrush = nullptr;
static HBRUSH gThemeCardBrush = nullptr;
static HBRUSH gThemeHeaderBrush = nullptr;
static HBRUSH gThemeEditBrush = nullptr;
static HBRUSH gThemeButtonBrush = nullptr;

static HBRUSH themeWindowBrush() { if (!gThemeWindowBrush) gThemeWindowBrush = CreateSolidBrush(theme::WindowBg); return gThemeWindowBrush; }
static HBRUSH themePanelBrush() { if (!gThemePanelBrush) gThemePanelBrush = CreateSolidBrush(theme::PanelBg); return gThemePanelBrush; }
static HBRUSH themeCardBrush() { if (!gThemeCardBrush) gThemeCardBrush = CreateSolidBrush(theme::CardBg); return gThemeCardBrush; }
static HBRUSH themeHeaderBrush() { if (!gThemeHeaderBrush) gThemeHeaderBrush = CreateSolidBrush(theme::HeaderBg); return gThemeHeaderBrush; }
static HBRUSH themeEditBrush() { if (!gThemeEditBrush) gThemeEditBrush = CreateSolidBrush(theme::EditBg); return gThemeEditBrush; }
static HBRUSH themeButtonBrush() { if (!gThemeButtonBrush) gThemeButtonBrush = CreateSolidBrush(theme::ButtonBg); return gThemeButtonBrush; }

static void releaseThemeBrushes()
{
    if (gThemeWindowBrush) { DeleteObject(gThemeWindowBrush); gThemeWindowBrush = nullptr; }
    if (gThemePanelBrush) { DeleteObject(gThemePanelBrush); gThemePanelBrush = nullptr; }
    if (gThemeCardBrush) { DeleteObject(gThemeCardBrush); gThemeCardBrush = nullptr; }
    if (gThemeHeaderBrush) { DeleteObject(gThemeHeaderBrush); gThemeHeaderBrush = nullptr; }
    if (gThemeEditBrush) { DeleteObject(gThemeEditBrush); gThemeEditBrush = nullptr; }
    if (gThemeButtonBrush) { DeleteObject(gThemeButtonBrush); gThemeButtonBrush = nullptr; }
}

static void fillRectColor(HDC hdc, const RECT& rc, COLORREF color)
{
    HBRUSH b = CreateSolidBrush(color);
    FillRect(hdc, &rc, b);
    DeleteObject(b);
}

static void drawRectFrame(HDC hdc, const RECT& rc, COLORREF color)
{
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HPEN oldPen = reinterpret_cast<HPEN>(SelectObject(hdc, pen));
    HBRUSH oldBrush = reinterpret_cast<HBRUSH>(SelectObject(hdc, GetStockObject(NULL_BRUSH)));
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

static LRESULT darkCtlColor(UINT msg, WPARAM wp)
{
    HDC hdc = reinterpret_cast<HDC>(wp);
    switch (msg)
    {
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
        SetTextColor(hdc, theme::Text);
        SetBkColor(hdc, theme::EditBg);
        return reinterpret_cast<LRESULT>(themeEditBrush());
    case WM_CTLCOLORBTN:
        SetTextColor(hdc, theme::Text);
        SetBkColor(hdc, theme::ButtonBg);
        return reinterpret_cast<LRESULT>(themeButtonBrush());
    case WM_CTLCOLORSTATIC:
        SetTextColor(hdc, theme::MutedText);
        SetBkMode(hdc, TRANSPARENT);
        return reinterpret_cast<LRESULT>(themePanelBrush());
    default:
        return reinterpret_cast<LRESULT>(themePanelBrush());
    }
}

static void drawDarkButton(const DRAWITEMSTRUCT* dis)
{
    if (!dis || !dis->hwndItem) return;

    RECT r = dis->rcItem;
    const bool pressed = (dis->itemState & ODS_SELECTED) != 0;
    const bool focus = (dis->itemState & ODS_FOCUS) != 0;
    const bool disabled = (dis->itemState & ODS_DISABLED) != 0;

    fillRectColor(dis->hDC, r, pressed ? theme::HeaderBg : theme::ButtonBg);
    drawRectFrame(dis->hDC, r, focus ? theme::Accent : theme::Border);

    char text[256] = {};
    GetWindowTextA(dis->hwndItem, text, sizeof(text));
    SetBkMode(dis->hDC, TRANSPARENT);
    SetTextColor(dis->hDC, disabled ? RGB(96, 101, 110) : theme::Text);
    if (pressed) OffsetRect(&r, 1, 1);
    DrawTextA(dis->hDC, text, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

static void enableProcessDarkMode()
{
    // Windows only exposes process-wide dark-menu/common-control opt-in through
    // newer uxtheme exports. Load them dynamically so the editor still runs on
    // older Windows versions.
    HMODULE uxtheme = LoadLibraryA("uxtheme.dll");
    if (!uxtheme) return;

    typedef int(WINAPI* SetPreferredAppModeFn)(int);
    typedef void(WINAPI* FlushMenuThemesFn)();

    SetPreferredAppModeFn setPreferredAppMode = reinterpret_cast<SetPreferredAppModeFn>(GetProcAddress(uxtheme, MAKEINTRESOURCEA(135)));
    FlushMenuThemesFn flushMenuThemes = reinterpret_cast<FlushMenuThemesFn>(GetProcAddress(uxtheme, MAKEINTRESOURCEA(136)));

    if (setPreferredAppMode)
        setPreferredAppMode(1); // AllowDark.
    if (flushMenuThemes)
        flushMenuThemes();

    FreeLibrary(uxtheme);
}

static void enableDarkTitleBar(HWND hwnd)
{
    if (!hwnd) return;

    // Let Explorer-themed child controls and menus know this HWND prefers dark
    // mode when the OS supports it.
    HMODULE uxtheme = LoadLibraryA("uxtheme.dll");
    if (uxtheme)
    {
        typedef BOOL(WINAPI* AllowDarkModeForWindowFn)(HWND, BOOL);
        AllowDarkModeForWindowFn allowDarkModeForWindow = reinterpret_cast<AllowDarkModeForWindowFn>(GetProcAddress(uxtheme, MAKEINTRESOURCEA(133)));
        if (allowDarkModeForWindow)
            allowDarkModeForWindow(hwnd, TRUE);
        FreeLibrary(uxtheme);
    }

    HMODULE dwm = LoadLibraryA("dwmapi.dll");
    if (!dwm) return;

    typedef HRESULT(WINAPI* DwmSetWindowAttributeFn)(HWND, DWORD, LPCVOID, DWORD);
    DwmSetWindowAttributeFn setAttribute = reinterpret_cast<DwmSetWindowAttributeFn>(GetProcAddress(dwm, "DwmSetWindowAttribute"));
    if (setAttribute)
    {
        BOOL enabled = TRUE;
        setAttribute(hwnd, 20, &enabled, sizeof(enabled)); // DWMWA_USE_IMMERSIVE_DARK_MODE on modern Windows 10/11.
        setAttribute(hwnd, 19, &enabled, sizeof(enabled)); // Older Windows 10 builds used 19.

        // Windows 11+ honors these explicit non-client colors. Older versions
        // safely ignore the attributes, and the manual border repaint below is
        // used as a fallback for the visible frame line.
        COLORREF captionColor = theme::WindowBg; // DWMWA_CAPTION_COLOR = 35.
        COLORREF borderColor = theme::Border;    // DWMWA_BORDER_COLOR = 34.
        COLORREF textColor = theme::Text;        // DWMWA_TEXT_COLOR = 36.
        setAttribute(hwnd, 34, &borderColor, sizeof(borderColor));
        setAttribute(hwnd, 35, &captionColor, sizeof(captionColor));
        setAttribute(hwnd, 36, &textColor, sizeof(textColor));
    }

    FreeLibrary(dwm);

    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

static void paintDarkWindowBorder(HWND hwnd)
{
    if (!hwnd || IsIconic(hwnd)) return;

    HDC hdc = GetWindowDC(hwnd);
    if (!hdc) return;

    RECT wr{};
    GetWindowRect(hwnd, &wr);
    RECT r{ 0, 0, max(1, wr.right - wr.left), max(1, wr.bottom - wr.top) };

    // This paints the visible frame line even on systems that do not support
    // DWMWA_BORDER_COLOR. DWM will still own the caption buttons/title text.
    HBRUSH borderBrush = CreateSolidBrush(theme::Border);
    HBRUSH darkBrush = CreateSolidBrush(theme::WindowBg);
    FrameRect(hdc, &r, borderBrush);
    InflateRect(&r, -1, -1);
    FrameRect(hdc, &r, darkBrush);
    DeleteObject(darkBrush);
    DeleteObject(borderBrush);

    ReleaseDC(hwnd, hdc);
}

struct MenuDrawData
{
    std::string text;
    bool topLevel = false;
    bool separator = false;
};

static std::vector<MenuDrawData*> gDarkMenuData;

static MenuDrawData* makeMenuDrawData(const char* text, bool topLevel, bool separator = false)
{
    MenuDrawData* data = new MenuDrawData();
    data->text = text ? text : "";
    data->topLevel = topLevel;
    data->separator = separator;
    gDarkMenuData.push_back(data);
    return data;
}

static void releaseDarkMenuData()
{
    for (size_t i = 0; i < gDarkMenuData.size(); ++i)
        delete gDarkMenuData[i];
    gDarkMenuData.clear();
}

static std::string menuTextWithoutAmpersands(const std::string& in)
{
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i)
    {
        if (in[i] == '&')
        {
            if (i + 1 < in.size() && in[i + 1] == '&')
            {
                out.push_back('&');
                ++i;
            }
            continue;
        }
        out.push_back(in[i]);
    }
    return out;
}

static void splitMenuText(const std::string& raw, std::string& left, std::string& right)
{
    const std::string clean = menuTextWithoutAmpersands(raw);
    const size_t tab = clean.find('\t');
    if (tab == std::string::npos)
    {
        left = clean;
        right.clear();
    }
    else
    {
        left = clean.substr(0, tab);
        right = clean.substr(tab + 1);
    }
}

static void applyDarkMenuInfo(HMENU menu, bool topLevel)
{
    if (!menu) return;
    MENUINFO mi{};
    mi.cbSize = sizeof(mi);
    mi.fMask = MIM_BACKGROUND | MIM_STYLE;
    mi.hbrBack = topLevel ? themeWindowBrush() : themePanelBrush();
    mi.dwStyle = MNS_CHECKORBMP;
    SetMenuInfo(menu, &mi);
}

static void appendDarkMenuItem(HMENU menu, UINT_PTR id, const char* text)
{
    AppendMenuA(menu, MF_OWNERDRAW, id, reinterpret_cast<LPCSTR>(makeMenuDrawData(text, false)));
}

static void appendDarkPopupMenu(HMENU menu, HMENU popup, const char* text)
{
    AppendMenuA(menu, MF_POPUP | MF_OWNERDRAW, reinterpret_cast<UINT_PTR>(popup), reinterpret_cast<LPCSTR>(makeMenuDrawData(text, true)));
}

static void appendDarkSeparator(HMENU menu)
{
    AppendMenuA(menu, MF_OWNERDRAW | MF_GRAYED, 0, reinterpret_cast<LPCSTR>(makeMenuDrawData("", false, true)));
}

static SIZE menuTextExtent(HDC hdc, const std::string& text)
{
    SIZE size{ 0, 0 };
    if (!text.empty())
        GetTextExtentPoint32A(hdc, text.c_str(), static_cast<int>(text.size()), &size);
    return size;
}

static void measureDarkMenuItem(MEASUREITEMSTRUCT* mis)
{
    if (!mis || mis->CtlType != ODT_MENU) return;
    MenuDrawData* data = reinterpret_cast<MenuDrawData*>(mis->itemData);
    if (!data)
    {
        mis->itemWidth = 80;
        mis->itemHeight = 24;
        return;
    }
    if (data->separator)
    {
        mis->itemWidth = 220;
        mis->itemHeight = 9;
        return;
    }

    HDC hdc = GetDC(g.main ? g.main : nullptr);
    HFONT oldFont = hdc ? reinterpret_cast<HFONT>(SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT))) : nullptr;
    std::string left, right;
    splitMenuText(data->text, left, right);
    SIZE leftSize = hdc ? menuTextExtent(hdc, left) : SIZE{ static_cast<LONG>(left.size() * 7), 16 };
    SIZE rightSize = hdc ? menuTextExtent(hdc, right) : SIZE{ static_cast<LONG>(right.size() * 7), 16 };
    if (hdc && oldFont) SelectObject(hdc, oldFont);
    if (hdc) ReleaseDC(g.main ? g.main : nullptr, hdc);

    if (data->topLevel)
    {
        mis->itemWidth = static_cast<UINT>(max(42L, leftSize.cx + 22L));
        mis->itemHeight = static_cast<UINT>(max(24L, leftSize.cy + 10L));
    }
    else
    {
        const LONG accelSpace = right.empty() ? 0 : (rightSize.cx + 30L);
        mis->itemWidth = static_cast<UINT>(max(190L, leftSize.cx + accelSpace + 52L));
        mis->itemHeight = static_cast<UINT>(max(24L, leftSize.cy + 10L));
    }
}

static void drawMenuCheckmark(HDC hdc, const RECT& r, COLORREF color)
{
    HPEN pen = CreatePen(PS_SOLID, 2, color);
    HPEN oldPen = reinterpret_cast<HPEN>(SelectObject(hdc, pen));
    const int cx = r.left + 13;
    const int cy = r.top + ((r.bottom - r.top) / 2);
    MoveToEx(hdc, cx - 4, cy, nullptr);
    LineTo(hdc, cx - 1, cy + 4);
    LineTo(hdc, cx + 7, cy - 5);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

static void drawDarkMenuItem(const DRAWITEMSTRUCT* dis)
{
    if (!dis || dis->CtlType != ODT_MENU) return;
    MenuDrawData* data = reinterpret_cast<MenuDrawData*>(dis->itemData);
    if (!data) return;

    RECT r = dis->rcItem;
    const bool selected = (dis->itemState & ODS_SELECTED) != 0;
    const bool disabled = (dis->itemState & (ODS_DISABLED | ODS_GRAYED)) != 0;
    const bool checked = (dis->itemState & ODS_CHECKED) != 0;
    const COLORREF bg = selected ? theme::HeaderBg : (data->topLevel ? theme::WindowBg : theme::PanelBg);
    const COLORREF text = disabled ? RGB(94, 98, 108) : (selected ? RGB(255, 255, 255) : theme::Text);

    fillRectColor(dis->hDC, r, bg);

    if (data->separator)
    {
        RECT line = r;
        line.left += 12;
        line.right -= 12;
        line.top += ((line.bottom - line.top) / 2);
        line.bottom = line.top + 1;
        fillRectColor(dis->hDC, line, theme::Border);
        return;
    }

    if (selected)
    {
        RECT hi = r;
        InflateRect(&hi, -2, -2);
        drawRectFrame(dis->hDC, hi, theme::Border);
    }

    std::string left, right;
    splitMenuText(data->text, left, right);

    SetBkMode(dis->hDC, TRANSPARENT);
    SetTextColor(dis->hDC, text);
    HFONT oldFont = reinterpret_cast<HFONT>(SelectObject(dis->hDC, GetStockObject(DEFAULT_GUI_FONT)));

    if (data->topLevel)
    {
        RECT textRc = r;
        InflateRect(&textRc, -10, 0);
        DrawTextA(dis->hDC, left.c_str(), -1, &textRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    }
    else
    {
        RECT checkRc = r;
        checkRc.right = checkRc.left + 28;
        if (checked) drawMenuCheckmark(dis->hDC, checkRc, disabled ? RGB(94, 98, 108) : theme::Accent);

        RECT leftRc = r;
        leftRc.left += 30;
        leftRc.right -= right.empty() ? 12 : 96;
        DrawTextA(dis->hDC, left.c_str(), -1, &leftRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

        if (!right.empty())
        {
            RECT rightRc = r;
            rightRc.left = max(rightRc.left + 90, rightRc.right - 92);
            rightRc.right -= 14;
            SetTextColor(dis->hDC, disabled ? RGB(94, 98, 108) : theme::MutedText);
            DrawTextA(dis->hDC, right.c_str(), -1, &rightRc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
        }
    }

    SelectObject(dis->hDC, oldFont);
}

// -----------------------------------------------------------------------------
// Small math helpers.
// -----------------------------------------------------------------------------
static long i64mulscale(long a, long b, int shift)
{
    return static_cast<long>((static_cast<std::int64_t>(a) * static_cast<std::int64_t>(b)) >> shift);
}

static long mulscale14_local(long a, long b) { return i64mulscale(a, b, 14); }

static long divscale14_local(long a, long b)
{
    if (b == 0) return 0;
    return static_cast<long>((static_cast<std::int64_t>(a) << 14) / b);
}

static long scale_local(long a, long b, long c)
{
    if (c == 0) return 0;
    return static_cast<long>((static_cast<std::int64_t>(a) * static_cast<std::int64_t>(b)) / c);
}

static long clamp_long(long v, long lo, long hi)
{
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

static int clamp_int(int v, int lo, int hi)
{
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

static double dist2(double ax, double ay, double bx, double by)
{
    const double dx = ax - bx, dy = ay - by;
    return dx * dx + dy * dy;
}

static double pointSegmentDistance2(double px, double py, double ax, double ay, double bx, double by, double* outT = nullptr)
{
    const double vx = bx - ax;
    const double vy = by - ay;
    const double wx = px - ax;
    const double wy = py - ay;
    const double len2 = vx * vx + vy * vy;
    double t = 0.0;
    if (len2 > 0.0) t = (wx * vx + wy * vy) / len2;
    t = max(0.0, min(1.0, t));
    if (outT) *outT = t;
    const double cx = ax + vx * t;
    const double cy = ay + vy * t;
    return dist2(px, py, cx, cy);
}

static int gridSize()
{
    if (g.grid <= 0) return 1;
    const int shift = max(1, 11 - g.grid);
    return 1 << shift;
}

static void snapPoint(long& x, long& y)
{
    if (!g.gridLock || g.grid <= 0) return;
    const int gs = gridSize();
    x = ((x + (gs >> 1)) / gs) * gs;
    y = ((y + (gs >> 1)) / gs) * gs;
}

static void mapToScreen(HWND hwnd, long mx, long my, int& sx, int& sy)
{
    RECT rc{};
    GetClientRect(hwnd, &rc);
    const double z = static_cast<double>(g.zoom) / 16384.0;
    sx = static_cast<int>((rc.right - rc.left) * 0.5 + (static_cast<double>(mx - g.viewX) * z));
    sy = static_cast<int>((rc.bottom - rc.top) * 0.5 + (static_cast<double>(my - g.viewY) * z));
}

static POINT screenToMap(HWND hwnd, int sx, int sy, bool snap)
{
    RECT rc{};
    GetClientRect(hwnd, &rc);
    const double z = static_cast<double>(g.zoom) / 16384.0;
    POINT p{};
    if (z <= 0.00001)
    {
        p.x = static_cast<LONG>(g.viewX);
        p.y = static_cast<LONG>(g.viewY);
    }
    else
    {
        p.x = static_cast<LONG>(std::llround(g.viewX + (sx - (rc.right - rc.left) * 0.5) / z));
        p.y = static_cast<LONG>(std::llround(g.viewY + (sy - (rc.bottom - rc.top) * 0.5) / z));
    }
    if (snap)
    {
        long x = p.x, y = p.y;
        snapPoint(x, y);
        p.x = x; p.y = y;
    }
    return p;
}

static long screenPixelsToMap(HWND hwnd, int pixels)
{
    RECT rc{};
    GetClientRect(hwnd, &rc);
    const double z = static_cast<double>(g.zoom) / 16384.0;
    if (z <= 0.00001) return 1024;
    return static_cast<long>(std::ceil(static_cast<double>(pixels) / z));
}

// -----------------------------------------------------------------------------
// Forward declarations.
// -----------------------------------------------------------------------------
static void updateInspector();
static void applyInspectorEdits(bool useCurrentTile);
static void syncEngineSearchGlobals(const Selection& s);
static void updateTileArtPanel();
static void updateGrpEditPanel();
static void runCurrentMap();
static void bloodNormalizeMapForBlood(bool quiet);
static bool bloodChooseAndApplyPresetToSelectedSprite();
static bool prepareBloodRffCache(const std::string& rffPath);
static std::string bloodArtPathForLoad();
static bool loadArtSetFromPathOrFolder(const std::string& artPathOrFolder, const char* statusLabel = nullptr);
static bool loadBloodArtSetFromCurrentData();
static bool reloadCurrentGamePalette();
static bool importSelectedTileFromPng();
static bool exportSelectedTileToPng();
static bool saveModifiedArtFiles();
static std::string moduleDirectory();
static bool fileExistsA(const char* path);
static std::string pathDirectory(const std::string& path);
static std::string pathJoin(const std::string& dir, const std::string& name);
static void moveCtl(HWND h, int x, int y, int w, int ht);

// -----------------------------------------------------------------------------
// UI utilities.
// -----------------------------------------------------------------------------
static void setStatus(const char* fmt, ...)
{
    char buf[512];
    va_list va;
    va_start(va, fmt);
    _vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, va);
    va_end(va);
    g.status = buf;
    if (g.textureBar) InvalidateRect(g.textureBar, nullptr, FALSE);
    if (g.tileArt) updateTileArtPanel();
}

static void updateTitle()
{
    std::string title = "BuildX (";
    title += gameProfileName(g.game);
    title += ") - ";
    title += g.boardPath.empty() ? "newboard.map" : g.boardPath;
    if (g.dirty || g.artDirty) title += " *";
    if (g.main) SetWindowTextA(g.main, title.c_str());
}

static void setDirty(bool dirty = true)
{
    g.dirty = dirty;
    updateTitle();
}

static void setArtDirty(bool dirty = true)
{
    g.artDirty = dirty;
    updateTitle();
}

static void markDirtyDuringDrag()
{
    if (g.dragDirtySet) return;
    if (!g.dirty) setDirty(true);
    else g.dirty = true;
    g.dragDirtySet = true;
}

static void request2DRefresh(HWND hwnd, bool immediate = false)
{
    if (!hwnd) return;

    const DWORD now = GetTickCount();
    const DWORD minMs = 15; // Cap high-frequency mouse redraws to about one screen refresh.
    const DWORD elapsed = now - g.last2DInvalidateTick;

    if (immediate || g.last2DInvalidateTick == 0 || elapsed >= minMs)
    {
        KillTimer(hwnd, ids::TIMER_2D_REFRESH);
        g.pending2DInvalidate = false;
        g.last2DInvalidateTick = now;
        InvalidateRect(hwnd, nullptr, FALSE);
        return;
    }

    if (!g.pending2DInvalidate)
    {
        g.pending2DInvalidate = true;
        const UINT delay = static_cast<UINT>(minMs - elapsed);
        SetTimer(hwnd, ids::TIMER_2D_REFRESH, delay ? delay : 1, nullptr);
    }
}

static bool confirmDiscardChanges()
{
    if (!g.dirty && !g.artDirty) return true;

    const int r = MessageBoxA(g.main, "Save changes to the current map or tile art?", "Unsaved changes", MB_ICONQUESTION | MB_YESNOCANCEL);
    if (r == IDCANCEL) return false;
    if (r == IDNO) return true; // Discard changes and continue with New/Open.

    if (r == IDYES)
    {
        SendMessage(g.main, WM_COMMAND, ids::M_FILE_SAVE, 0);
        return !g.dirty && !g.artDirty; // Save succeeded only if dirty flags were cleared.
    }

    return false;
}

static void refreshAllViews()
{
    if (g.view2d) InvalidateRect(g.view2d, nullptr, FALSE);
    if (g.tileArt) InvalidateRect(g.tileArt, nullptr, FALSE);
    if (g.grpEdit) InvalidateRect(g.grpEdit, nullptr, FALSE);
    if (g.textureBar) InvalidateRect(g.textureBar, nullptr, FALSE);
    if (g.inspector) InvalidateRect(g.inspector, nullptr, FALSE);
}

static HMENU makeMainMenu()
{
    releaseDarkMenuData();

    HMENU menu = CreateMenu();
    HMENU file = CreatePopupMenu();
    appendDarkMenuItem(file, ids::M_FILE_NEW, "&New\tCtrl+N");
    appendDarkMenuItem(file, ids::M_FILE_OPEN, "&Open...\tCtrl+O");
    appendDarkMenuItem(file, ids::M_FILE_OPEN_GRP, "Open From &Archive...");
    appendDarkSeparator(file);
    appendDarkMenuItem(file, ids::M_FILE_SAVE, "&Save\tCtrl+S");
    appendDarkMenuItem(file, ids::M_FILE_SAVE_AS, "Save &As...");
    appendDarkSeparator(file);
    appendDarkMenuItem(file, ids::M_FILE_RUN_MAP, "&Run Map\tF5");
    appendDarkSeparator(file);
    appendDarkMenuItem(file, ids::M_FILE_EXIT, "E&xit");
    applyDarkMenuInfo(file, false);
    appendDarkPopupMenu(menu, file, "&File");

    HMENU run = CreatePopupMenu();
    appendDarkMenuItem(run, ids::M_FILE_RUN_MAP, "&Run Current Map\tF5");
    applyDarkMenuInfo(run, false);
    appendDarkPopupMenu(menu, run, "&Run");

    HMENU view = CreatePopupMenu();
    appendDarkMenuItem(view, ids::M_VIEW_2D, "Show &2D Editor\tF2");
    appendDarkMenuItem(view, ids::M_VIEW_3D, "Show &3D Editor\tF3");
    appendDarkMenuItem(view, ids::M_VIEW_TILE_ART, "Show &Tile Art\tF4");
    appendDarkMenuItem(view, ids::M_VIEW_GRPEDIT, "Show &GRP Edit\tF6");
    appendDarkSeparator(view);
    appendDarkMenuItem(view, ids::M_VIEW_ZOOM_IN, "Zoom &In");
    appendDarkMenuItem(view, ids::M_VIEW_ZOOM_OUT, "Zoom &Out");
    appendDarkMenuItem(view, ids::M_VIEW_GRID, "Cycle &Grid\tG");
    appendDarkMenuItem(view, ids::M_VIEW_GRID_LOCK, "&Grid Lock\tL");
    appendDarkMenuItem(view, ids::M_VIEW_SHOW_TAGS, "Show &Tags\tCtrl+T");
    applyDarkMenuInfo(view, false);
    appendDarkPopupMenu(menu, view, "&View");

    HMENU edit = CreatePopupMenu();
    appendDarkMenuItem(edit, ids::M_EDIT_BEGIN_DRAW, "Begin/Add Sector Point\tSpace");
    appendDarkMenuItem(edit, ids::M_EDIT_CLOSE_DRAW, "Close Drawn Sector\tEnter");
    appendDarkMenuItem(edit, ids::M_EDIT_CANCEL_DRAW, "Cancel Sector Drawing\tEsc");
    appendDarkSeparator(edit);
    appendDarkMenuItem(edit, ids::M_EDIT_INSERT_POINT, "Insert Point\tIns");
    appendDarkMenuItem(edit, ids::M_EDIT_INSERT_SPRITE, "Insert Sprite\tS");
    appendDarkMenuItem(edit, ids::M_EDIT_DELETE, "Delete Selection\tDel");
    appendDarkSeparator(edit);
    appendDarkMenuItem(edit, ids::M_EDIT_COPY_PROPS, "Copy Properties\tTab");
    appendDarkMenuItem(edit, ids::M_EDIT_PASTE_PROPS, "Paste Properties\tEnter");
    appendDarkMenuItem(edit, ids::M_EDIT_APPLY_TEXTURE, "Apply Selected Texture\tV");
    appendDarkMenuItem(edit, ids::M_EDIT_PROPERTIES, "Properties...\tAlt+Enter");
    applyDarkMenuInfo(edit, false);
    appendDarkPopupMenu(menu, edit, "&Edit");

    HMENU tools = CreatePopupMenu();
    appendDarkMenuItem(tools, ids::M_TOOLS_RELINK, "Repair Red-Wall Links");
    appendDarkMenuItem(tools, ids::M_TOOLS_SET_START, "Set Player Start Here\tScroll Lock");
    appendDarkSeparator(tools);
    appendDarkMenuItem(tools, ids::M_TOOLS_TOGGLE_BLOCKING, "Toggle Blocking\tB");
    appendDarkMenuItem(tools, ids::M_TOOLS_TOGGLE_ONEWAY, "Toggle One-Way\t1");
    appendDarkMenuItem(tools, ids::M_TOOLS_TOGGLE_MASKED, "Toggle Masked Wall\tM");
    appendDarkMenuItem(tools, ids::M_TOOLS_TOGGLE_TRANSLUCENT, "Toggle Translucency\tT");
    appendDarkMenuItem(tools, ids::M_TOOLS_TOGGLE_PARALLAX, "Toggle Parallax\tP");
    appendDarkSeparator(tools);
    appendDarkMenuItem(tools, ids::M_TOOLS_SHADE_UP, "Shade Darker\t-");
    appendDarkMenuItem(tools, ids::M_TOOLS_SHADE_DOWN, "Shade Brighter\t+");
    appendDarkMenuItem(tools, ids::M_TOOLS_Z_UP, "Raise Z\tPgUp");
    appendDarkMenuItem(tools, ids::M_TOOLS_Z_DOWN, "Lower Z\tPgDn");
    appendDarkSeparator(tools);
    appendDarkMenuItem(tools, ids::M_BLOOD_SPRITE_PRESET, "Blood: Apply Sprite Preset...");
    appendDarkMenuItem(tools, ids::M_BLOOD_FIXUP_SELECTION, "Blood: Normalize Selection");
    appendDarkMenuItem(tools, ids::M_BLOOD_FIXUP_MAP, "Blood: Normalize All Sprites");
    applyDarkMenuInfo(tools, false);
    appendDarkPopupMenu(menu, tools, "&Tools");

    HMENU texture = CreatePopupMenu();
    appendDarkMenuItem(texture, ids::M_TEXTURE_GOTO, "Goto Tile...");
    appendDarkMenuItem(texture, ids::M_TEXTURE_TOGGLE_SIZE, "Expand/Collapse Texture Browser");
    applyDarkMenuInfo(texture, false);
    appendDarkPopupMenu(menu, texture, "&Texture");

    HMENU help = CreatePopupMenu();
    appendDarkMenuItem(help, ids::M_HELP_KEYS, "Keyboard Help");
    applyDarkMenuInfo(help, false);
    appendDarkPopupMenu(menu, help, "&Help");

    applyDarkMenuInfo(menu, true);
    return menu;
}

// -----------------------------------------------------------------------------
// Tiny modal input box, no resource file required.
// -----------------------------------------------------------------------------
struct InputBoxState
{
    std::string title;
    std::string label;
    std::string value;
    bool done = false;
    bool ok = false;
    HWND edit = nullptr;
};

static LRESULT CALLBACK InputBoxProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    InputBoxState* st = reinterpret_cast<InputBoxState*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
    switch (msg)
    {
    case WM_CREATE:
    {
        st = reinterpret_cast<InputBoxState*>(reinterpret_cast<CREATESTRUCTA*>(lp)->lpCreateParams);
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(st));
        enableDarkTitleBar(hwnd);
        CreateWindowA("STATIC", st->label.c_str(), WS_CHILD | WS_VISIBLE, 12, 12, 340, 18, hwnd, nullptr, g.inst, nullptr);
        st->edit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", st->value.c_str(), WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            12, 38, 340, 24, hwnd, reinterpret_cast<HMENU>(1), g.inst, nullptr);
        CreateWindowA("BUTTON", "OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_OWNERDRAW, 196, 74, 72, 26, hwnd, reinterpret_cast<HMENU>(IDOK), g.inst, nullptr);
        CreateWindowA("BUTTON", "Cancel", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 280, 74, 72, 26, hwnd, reinterpret_cast<HMENU>(IDCANCEL), g.inst, nullptr);
        SendMessageA(st->edit, EM_SETSEL, 0, -1);
        SetFocus(st->edit);
        return 0;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORBTN:
        return darkCtlColor(msg, wp);
    case WM_ERASEBKGND:
    {
        HDC hdc = reinterpret_cast<HDC>(wp);
        RECT rc{};
        GetClientRect(hwnd, &rc);
        fillRectColor(hdc, rc, theme::PanelBg);
        return 1;
    }
    case WM_DRAWITEM:
        drawDarkButton(reinterpret_cast<DRAWITEMSTRUCT*>(lp));
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK)
        {
            char buf[512] = {};
            GetWindowTextA(st->edit, buf, sizeof(buf));
            st->value = buf;
            st->ok = true;
            st->done = true;
            DestroyWindow(hwnd);
            return 0;
        }
        if (LOWORD(wp) == IDCANCEL)
        {
            st->ok = false;
            st->done = true;
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_CLOSE:
        if (st) { st->ok = false; st->done = true; }
        DestroyWindow(hwnd);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static bool promptString(const char* title, const char* label, std::string& value)
{
    static bool registered = false;
    if (!registered)
    {
        WNDCLASSA wc{};
        wc.lpfnWndProc = InputBoxProc;
        wc.hInstance = g.inst;
        wc.lpszClassName = "BuildEditorInputBox";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = themePanelBrush();
        RegisterClassA(&wc);
        registered = true;
    }

    InputBoxState st;
    st.title = title ? title : "Input";
    st.label = label ? label : "Value:";
    st.value = value;

    RECT pr{};
    GetWindowRect(g.main, &pr);
    const int w = 374, h = 138;
    const int x = pr.left + ((pr.right - pr.left) - w) / 2;
    const int y = pr.top + ((pr.bottom - pr.top) - h) / 2;
    HWND hwnd = CreateWindowExA(WS_EX_DLGMODALFRAME, "BuildEditorInputBox", st.title.c_str(),
        WS_CAPTION | WS_SYSMENU | WS_POPUP, x, y, w, h, g.main, nullptr, g.inst, &st);
    if (!hwnd) return false;

    EnableWindow(g.main, FALSE);
    ShowWindow(hwnd, SW_SHOW);
    MSG msg{};
    while (!st.done && GetMessageA(&msg, nullptr, 0, 0) > 0)
    {
        if (!IsDialogMessageA(hwnd, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }
    EnableWindow(g.main, TRUE);
    SetActiveWindow(g.main);

    if (st.ok) value = st.value;
    return st.ok;
}

static bool promptLong(const char* title, const char* label, long& value, long minv, long maxv)
{
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%ld", value);
    std::string s = buf;
    if (!promptString(title, label, s)) return false;
    char* end = nullptr;
    long v = std::strtol(s.c_str(), &end, 0);
    if (!end || *end != 0)
    {
        MessageBoxA(g.main, "That was not a valid number.", "Input", MB_ICONERROR);
        return false;
    }
    value = clamp_long(v, minv, maxv);
    return true;
}

struct ChoiceBoxState
{
    std::string title;
    std::string label;
    std::vector<std::string> items;
    int selected = 0;
    bool done = false;
    bool ok = false;
    HWND list = nullptr;
};

static void finishChoiceBox(HWND hwnd, ChoiceBoxState* st, bool ok)
{
    if (!st) return;
    st->ok = ok;
    if (ok && st->list)
    {
        const int sel = static_cast<int>(SendMessageA(st->list, LB_GETCURSEL, 0, 0));
        if (sel != LB_ERR) st->selected = sel;
    }
    st->done = true;
    DestroyWindow(hwnd);
}

static LRESULT CALLBACK ChoiceBoxProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    ChoiceBoxState* st = reinterpret_cast<ChoiceBoxState*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
    switch (msg)
    {
    case WM_CREATE:
    {
        st = reinterpret_cast<ChoiceBoxState*>(reinterpret_cast<CREATESTRUCTA*>(lp)->lpCreateParams);
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(st));
        enableDarkTitleBar(hwnd);
        CreateWindowA("STATIC", st->label.c_str(), WS_CHILD | WS_VISIBLE, 12, 12, 396, 18, hwnd, nullptr, g.inst, nullptr);
        st->list = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
            12, 36, 396, 232, hwnd, reinterpret_cast<HMENU>(1001), g.inst, nullptr);
        for (const std::string& item : st->items)
            SendMessageA(st->list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item.c_str()));
        SendMessageA(st->list, LB_SETCURSEL, clamp_int(st->selected, 0, static_cast<int>(st->items.size()) - 1), 0);
        CreateWindowA("BUTTON", "OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | BS_OWNERDRAW, 252, 280, 72, 26, hwnd, reinterpret_cast<HMENU>(IDOK), g.inst, nullptr);
        CreateWindowA("BUTTON", "Cancel", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 336, 280, 72, 26, hwnd, reinterpret_cast<HMENU>(IDCANCEL), g.inst, nullptr);
        SetFocus(st->list);
        return 0;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
        return darkCtlColor(msg, wp);
    case WM_ERASEBKGND:
    {
        HDC hdc = reinterpret_cast<HDC>(wp);
        RECT rc{};
        GetClientRect(hwnd, &rc);
        fillRectColor(hdc, rc, theme::PanelBg);
        return 1;
    }
    case WM_DRAWITEM:
        drawDarkButton(reinterpret_cast<DRAWITEMSTRUCT*>(lp));
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wp) == IDOK || (LOWORD(wp) == 1001 && HIWORD(wp) == LBN_DBLCLK))
        {
            finishChoiceBox(hwnd, st, true);
            return 0;
        }
        if (LOWORD(wp) == IDCANCEL)
        {
            finishChoiceBox(hwnd, st, false);
            return 0;
        }
        break;
    case WM_CLOSE:
        finishChoiceBox(hwnd, st, false);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static bool chooseStringIndex(const char* title, const char* label, const std::vector<std::string>& items, int& selected)
{
    if (items.empty()) return false;

    static bool registered = false;
    if (!registered)
    {
        WNDCLASSA wc{};
        wc.lpfnWndProc = ChoiceBoxProc;
        wc.hInstance = g.inst;
        wc.lpszClassName = "BuildEditorChoiceBox";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = themePanelBrush();
        RegisterClassA(&wc);
        registered = true;
    }

    ChoiceBoxState st;
    st.title = title ? title : "Choose";
    st.label = label ? label : "Choose an item:";
    st.items = items;
    st.selected = selected;

    RECT pr{};
    GetWindowRect(g.main, &pr);
    const int w = 432, h = 350;
    const int x = pr.left + ((pr.right - pr.left) - w) / 2;
    const int y = pr.top + ((pr.bottom - pr.top) - h) / 2;
    HWND hwnd = CreateWindowExA(WS_EX_DLGMODALFRAME, "BuildEditorChoiceBox", st.title.c_str(),
        WS_CAPTION | WS_SYSMENU | WS_POPUP, x, y, w, h, g.main, nullptr, g.inst, &st);
    if (!hwnd) return false;

    EnableWindow(g.main, FALSE);
    ShowWindow(hwnd, SW_SHOW);
    MSG msg{};
    while (!st.done && GetMessageA(&msg, nullptr, 0, 0) > 0)
    {
        if (!IsDialogMessageA(hwnd, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }
    EnableWindow(g.main, TRUE);
    SetActiveWindow(g.main);

    if (st.ok) selected = st.selected;
    return st.ok;
}


// -----------------------------------------------------------------------------
// Startup game selector.
// -----------------------------------------------------------------------------
struct GameSelectState
{
    bool done = false;
    bool ok = false;
    GameProfile selected = GameProfile::Duke3D;
    HWND dukeButton = nullptr;
    HWND shadowButton = nullptr;
    HWND bloodButton = nullptr;
};

static void finishGameSelect(HWND hwnd, GameSelectState* st, bool ok, GameProfile profile)
{
    if (!st) return;
    st->ok = ok;
    if (ok) st->selected = profile;
    st->done = true;
    DestroyWindow(hwnd);
}

static LRESULT CALLBACK GameSelectProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    GameSelectState* st = reinterpret_cast<GameSelectState*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
    switch (msg)
    {
    case WM_CREATE:
    {
        st = reinterpret_cast<GameSelectState*>(reinterpret_cast<CREATESTRUCTA*>(lp)->lpCreateParams);
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(st));
        enableDarkTitleBar(hwnd);

        CreateWindowA("STATIC", "Choose the BUILD game profile for this editing session.", WS_CHILD | WS_VISIBLE | SS_LEFT,
            18, 16, 438, 22, hwnd, nullptr, g.inst, nullptr);
        CreateWindowA("STATIC", "Shadow Warrior is visible but disabled until its tags and resource rules are added.", WS_CHILD | WS_VISIBLE | SS_LEFT,
            18, 42, 438, 22, hwnd, nullptr, g.inst, nullptr);

        st->dukeButton = CreateWindowA("BUTTON", "Duke Nukem 3D", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_DEFPUSHBUTTON,
            18, 78, 438, 42, hwnd, reinterpret_cast<HMENU>(1), g.inst, nullptr);
        st->shadowButton = CreateWindowA("BUTTON", "Shadow Warrior  (coming soon)", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            18, 132, 438, 42, hwnd, reinterpret_cast<HMENU>(2), g.inst, nullptr);
        st->bloodButton = CreateWindowA("BUTTON", "Blood", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            18, 186, 438, 42, hwnd, reinterpret_cast<HMENU>(3), g.inst, nullptr);
        EnableWindow(st->shadowButton, FALSE);
        SetFocus(st->dukeButton);
        return 0;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
        return darkCtlColor(msg, wp);
    case WM_ERASEBKGND:
    {
        HDC hdc = reinterpret_cast<HDC>(wp);
        RECT rc{};
        GetClientRect(hwnd, &rc);
        fillRectColor(hdc, rc, theme::PanelBg);
        return 1;
    }
    case WM_DRAWITEM:
        drawDarkButton(reinterpret_cast<DRAWITEMSTRUCT*>(lp));
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wp))
        {
        case 1: finishGameSelect(hwnd, st, true, GameProfile::Duke3D); return 0;
        case 3: finishGameSelect(hwnd, st, true, GameProfile::Blood); return 0;
        case IDCANCEL: finishGameSelect(hwnd, st, false, GameProfile::Duke3D); return 0;
        }
        break;
    case WM_KEYDOWN:
        if (wp == VK_ESCAPE) { finishGameSelect(hwnd, st, false, GameProfile::Duke3D); return 0; }
        if (wp == 'B') { finishGameSelect(hwnd, st, true, GameProfile::Blood); return 0; }
        if (wp == 'D') { finishGameSelect(hwnd, st, true, GameProfile::Duke3D); return 0; }
        break;
    case WM_CLOSE:
        finishGameSelect(hwnd, st, false, GameProfile::Duke3D);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static bool showGameSelectScreen()
{
    static bool registered = false;
    if (!registered)
    {
        WNDCLASSA wc{};
        wc.lpfnWndProc = GameSelectProc;
        wc.hInstance = g.inst;
        wc.lpszClassName = "BuildEditorGameSelect";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = themePanelBrush();
        RegisterClassA(&wc);
        registered = true;
    }

    GameSelectState st;
    const int w = 480;
    const int h = 280;
    const int x = max(0, (GetSystemMetrics(SM_CXSCREEN) - w) / 2);
    const int y = max(0, (GetSystemMetrics(SM_CYSCREEN) - h) / 2);
    HWND hwnd = CreateWindowExA(WS_EX_DLGMODALFRAME, "BuildEditorGameSelect", "BuildX Game Profile",
        WS_CAPTION | WS_SYSMENU | WS_POPUP, x, y, w, h, nullptr, nullptr, g.inst, &st);
    if (!hwnd) return false;

    ShowWindow(hwnd, SW_SHOW);
    MSG msg{};
    while (!st.done && GetMessageA(&msg, nullptr, 0, 0) > 0)
    {
        if (!IsDialogMessageA(hwnd, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    if (st.ok)
        g.game = st.selected;
    return st.ok;
}


// -----------------------------------------------------------------------------
// BUILD map editing helpers.
// -----------------------------------------------------------------------------
static short ownerSectorOfWall(short wallnum)
{
    if (wallnum < 0 || wallnum >= numwalls) return -1;
    for (short s = 0; s < numsectors; ++s)
    {
        const int start = sector[s].wallptr;
        const int end = start + sector[s].wallnum;
        if (wallnum >= start && wallnum < end) return s;
    }
    return -1;
}

static long wallLength(short wi)
{
    if (wi < 0 || wi >= numwalls) return 0;
    const walltype& w = wall[wi];
    if (w.point2 < 0 || w.point2 >= numwalls) return 0;
    const long dx = wall[w.point2].x - w.x;
    const long dy = wall[w.point2].y - w.y;
    return ksqrt(dx * dx + dy * dy);
}

static void fixWallRepeat(short wi)
{
    if (wi < 0 || wi >= numwalls) return;
    long len = wallLength(wi);
    wall[wi].xrepeat = static_cast<char>(clamp_long((len + 4) >> 3, 1, 255));
    if (wall[wi].yrepeat == 0) wall[wi].yrepeat = 8;
}

struct EdgeKey
{
    long x1, y1, x2, y2;

    EdgeKey() : x1(0), y1(0), x2(0), y2(0) {}
    EdgeKey(long ax, long ay, long bx, long by) : x1(ax), y1(ay), x2(bx), y2(by) {}

    bool operator==(const EdgeKey& other) const
    {
        return x1 == other.x1 && y1 == other.y1 && x2 == other.x2 && y2 == other.y2;
    }
};

struct EdgeKeyHash
{
    std::size_t operator()(const EdgeKey& k) const
    {
        std::uint64_t h = 1469598103934665603ull;
        hashMix(h, k.x1);
        hashMix(h, k.y1);
        hashMix(h, k.x2);
        hashMix(h, k.y2);
        return static_cast<std::size_t>(h);
    }

    static void hashMix(std::uint64_t& h, long v)
    {
        const std::uint64_t x = static_cast<std::uint64_t>(static_cast<std::int64_t>(v));
        h ^= x + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
        h *= 1099511628211ull;
    }
};

static void buildWallOwnerList(std::vector<short>& owners)
{
    owners.assign(numwalls > 0 ? numwalls : 0, static_cast<short>(-1));
    for (short s = 0; s < numsectors; ++s)
    {
        const int start = sector[s].wallptr;
        const int end = start + sector[s].wallnum;
        if (start < 0) continue;
        for (int i = start; i < end && i < numwalls; ++i)
        {
            if (i >= 0) owners[static_cast<size_t>(i)] = s;
        }
    }
}

static void rebuildSectorLinks()
{
    for (short i = 0; i < numwalls; ++i)
    {
        wall[i].nextwall = -1;
        wall[i].nextsector = -1;
    }

    std::vector<short> owner;
    buildWallOwnerList(owner);

    std::unordered_multimap<EdgeKey, short, EdgeKeyHash> openEdges;
    openEdges.reserve(static_cast<size_t>(max(1, numwalls * 2)));

    for (short i = 0; i < numwalls; ++i)
    {
        if (wall[i].point2 < 0 || wall[i].point2 >= numwalls) continue;
        if (i >= static_cast<short>(owner.size()) || owner[i] < 0) continue;

        const walltype& a0 = wall[i];
        const walltype& a1 = wall[a0.point2];
        const EdgeKey key(a0.x, a0.y, a1.x, a1.y);
        const EdgeKey reverse(a1.x, a1.y, a0.x, a0.y);

        auto range = openEdges.equal_range(reverse);
        auto match = range.second;
        for (auto it = range.first; it != range.second; ++it)
        {
            const short j = it->second;
            if (j >= 0 && j < numwalls && j < static_cast<short>(owner.size()) && owner[j] >= 0 && owner[j] != owner[i])
            {
                match = it;
                break;
            }
        }

        if (match != range.second)
        {
            const short j = match->second;
            wall[i].nextwall = j;
            wall[i].nextsector = owner[j];
            wall[j].nextwall = i;
            wall[j].nextsector = owner[i];
            openEdges.erase(match);
        }
        else
        {
            openEdges.emplace(key, i);
        }
    }
    setStatus("Red-wall links rebuilt.");
}

static void refreshTileList()
{
    g.tileList.clear();
    for (short i = 0; i < MAXTILES; ++i)
        if (tilesizx[i] > 0 && tilesizy[i] > 0)
            g.tileList.push_back(i);
    if (g.tileList.empty()) g.tileList.push_back(0);
    g.tileScroll = clamp_int(g.tileScroll, 0, max(0, static_cast<int>(g.tileList.size()) - 1));
}

static int tileListIndex(short tile)
{
    for (int i = 0; i < static_cast<int>(g.tileList.size()); ++i)
        if (g.tileList[i] == tile) return i;
    return 0;
}

static void centerTextureBarOnSelected()
{
    if (g.tileList.empty()) return;
    g.tileScroll = max(0, tileListIndex(g.selectedTile) - 2);
    if (g.textureBar) InvalidateRect(g.textureBar, nullptr, FALSE);
    if (g.tileArt) updateTileArtPanel();
}

static void initNewMap()
{
    std::memset(sector, 0, sizeof(sector));
    std::memset(wall, 0, sizeof(wall));
    std::memset(sprite, 0, sizeof(sprite));
    initspritelists();

    numsectors = 1;
    numwalls = 4;

    sectortype& s = sector[0];
    s.wallptr = 0;
    s.wallnum = 4;
    s.ceilingz = -32768;
    s.floorz = 32768;
    s.ceilingpicnum = 0;
    s.floorpicnum = 0;
    s.ceilingheinum = 0;
    s.floorheinum = 0;
    s.ceilingshade = 0;
    s.floorshade = 0;
    s.visibility = 0;
    s.extra = -1;

    const long x0 = 24576, y0 = 24576, x1 = 40960, y1 = 40960;
    const long xs[4] = { x0, x1, x1, x0 };
    const long ys[4] = { y0, y0, y1, y1 };
    for (int i = 0; i < 4; ++i)
    {
        wall[i].x = xs[i];
        wall[i].y = ys[i];
        wall[i].point2 = static_cast<short>((i + 1) & 3);
        wall[i].nextwall = -1;
        wall[i].nextsector = -1;
        wall[i].picnum = 0;
        wall[i].overpicnum = 0;
        wall[i].shade = 0;
        wall[i].pal = 0;
        wall[i].xrepeat = 8;
        wall[i].yrepeat = 8;
        wall[i].extra = -1;
        fixWallRepeat(static_cast<short>(i));
    }

    g.cam.x = 32768;
    g.cam.y = 32768;
    g.cam.z = 0;
    g.cam.ang = 1536;
    g.cam.horiz = 100;
    g.cam.sect = 0;
    g.viewX = g.cam.x;
    g.viewY = g.cam.y;
    g.startx = g.cam.x;
    g.starty = g.cam.y;
    g.startz = g.cam.z;
    g.startang = g.cam.ang;
    g.startsect = g.cam.sect;
    g.sel.clear();
    syncEngineSearchGlobals(g.sel);
    g.drawingSector = false;
    g.drawPts.clear();
    rebuildSectorLinks();
}


// -----------------------------------------------------------------------------
// Blood BLM map loader.
//
// Duke/BUILD maps are raw version-7 BUILD maps. Blood maps are BLM files with a
// small Blood header, optional XOR encryption, and XSECTOR/XWALL/XSPRITE data
// blocks after objects whose extra field is non-zero. This loader imports the
// base BUILD geometry/sprites so Blood maps open in the editor even before the
// full Blood dbSaveMap/dbLoadMap backend is ported.
// -----------------------------------------------------------------------------
static std::uint16_t bloodReadU16(const unsigned char* p)
{
    return static_cast<std::uint16_t>(p[0]) |
        (static_cast<std::uint16_t>(p[1]) << 8);
}

static std::int16_t bloodReadI16(const unsigned char* p)
{
    return static_cast<std::int16_t>(bloodReadU16(p));
}

static std::uint32_t bloodReadU32(const unsigned char* p)
{
    return static_cast<std::uint32_t>(p[0]) |
        (static_cast<std::uint32_t>(p[1]) << 8) |
        (static_cast<std::uint32_t>(p[2]) << 16) |
        (static_cast<std::uint32_t>(p[3]) << 24);
}

static std::int32_t bloodReadI32(const unsigned char* p)
{
    return static_cast<std::int32_t>(bloodReadU32(p));
}

static bool bloodReadWholeFile(const char* path, std::vector<unsigned char>& bytes)
{
    bytes.clear();
    FILE* f = nullptr;
    if (!path || !path[0] || fopen_s(&f, path, "rb") != 0 || !f) return false;
    std::fseek(f, 0, SEEK_END);
    const long len = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (len <= 0)
    {
        std::fclose(f);
        return false;
    }
    bytes.resize(static_cast<size_t>(len));
    const bool ok = std::fread(bytes.data(), 1, bytes.size(), f) == bytes.size();
    std::fclose(f);
    return ok;
}

static void bloodCryptBytes(unsigned char* data, size_t bytes, int key)
{
    if (!data || bytes == 0) return;
    int nKey = key;
    for (size_t i = 0; i < bytes; ++i)
        data[i] = static_cast<unsigned char>(data[i] ^ static_cast<unsigned char>(nKey++));
}

struct BloodMapHeaderLite
{
    long px = 32768;
    long py = 32768;
    long pz = 0;
    short ang = 1536;
    short sect = -1;
    short skyBits = 0;
    long vis = 0;
    long songId = 0;
    char parallaxType = 0;
    long mapRev = 0;
    std::uint16_t numSectors = 0;
    std::uint16_t numWalls = 0;
    std::uint16_t numSprites = 0;
};

static BloodMapHeaderLite bloodParseMapHeader(const std::vector<unsigned char>& raw)
{
    BloodMapHeaderLite h;
    if (raw.size() < 37) return h;
    h.px = bloodReadI32(raw.data() + 0);
    h.py = bloodReadI32(raw.data() + 4);
    h.pz = bloodReadI32(raw.data() + 8);
    h.ang = bloodReadI16(raw.data() + 12);
    h.sect = bloodReadI16(raw.data() + 14);
    h.skyBits = bloodReadI16(raw.data() + 16);
    h.vis = bloodReadI32(raw.data() + 18);
    h.songId = bloodReadI32(raw.data() + 22);
    h.parallaxType = static_cast<char>(raw[26]);
    h.mapRev = bloodReadI32(raw.data() + 27);
    h.numSectors = bloodReadU16(raw.data() + 31);
    h.numWalls = bloodReadU16(raw.data() + 33);
    h.numSprites = bloodReadU16(raw.data() + 35);
    return h;
}

static bool bloodHeaderLooksValid(const BloodMapHeaderLite& h)
{
    if (h.numSectors == 0 || h.numSectors > MAXSECTORS) return false;
    if (h.numWalls == 0 || h.numWalls > MAXWALLS) return false;
    if (h.numSprites > MAXSPRITES) return false;
    if (h.skyBits < 0 || h.skyBits > 12) return false;
    if ((1 << h.skyBits) > MAXPSKYTILES) return false;
    if ((h.ang & ~2047) != 0) return false;
    return true;
}

static int bloodSafeXDataSize(int value, int fallback)
{
    if (value < 0 || value > 4096) return fallback;
    return value;
}

static void bloodSanitizeLoadedMap()
{
    for (short i = 0; i < numsectors; ++i)
    {
        if (sector[i].wallptr < 0 || sector[i].wallptr >= numwalls)
        {
            sector[i].wallptr = 0;
            sector[i].wallnum = 0;
        }
        if (sector[i].wallnum < 0 || sector[i].wallptr + sector[i].wallnum > numwalls)
            sector[i].wallnum = static_cast<short>(max(0, numwalls - sector[i].wallptr));
    }

    for (short i = 0; i < numwalls; ++i)
    {
        if (wall[i].point2 < 0 || wall[i].point2 >= numwalls) wall[i].point2 = i;
        if (wall[i].nextwall < 0 || wall[i].nextwall >= numwalls) wall[i].nextwall = -1;
        if (wall[i].nextsector < 0 || wall[i].nextsector >= numsectors) wall[i].nextsector = -1;
        if (wall[i].picnum < 0 || wall[i].picnum >= MAXTILES) wall[i].picnum = 0;
        if (wall[i].overpicnum < 0 || wall[i].overpicnum >= MAXTILES) wall[i].overpicnum = 0;
    }

    for (short i = 0; i < MAXSPRITES; ++i)
    {
        if (sprite[i].sectnum >= numsectors && sprite[i].sectnum != MAXSECTORS) sprite[i].sectnum = MAXSECTORS;
        if (sprite[i].statnum >= MAXSTATUS && sprite[i].statnum != MAXSTATUS) sprite[i].statnum = MAXSTATUS;
        if (sprite[i].picnum < 0 || sprite[i].picnum >= MAXTILES) sprite[i].picnum = 0;
    }
}

static bool loadBloodMapAsBuild(const char* path, long* daposx, long* daposy, long* daposz, short* daang, short* dacursectnum)
{
    std::vector<unsigned char> file;
    if (!bloodReadWholeFile(path, file)) return false;
    if (file.size() < 6 + 37 || std::memcmp(file.data(), "BLM\x1a", 4) != 0) return false;

    const std::uint16_t version = bloodReadU16(file.data() + 4);
    const std::uint16_t versionMajor = static_cast<std::uint16_t>(version & 0xff00);
    if (versionMajor != 0x0600 && versionMajor != 0x0700) return false;
    const bool encryptedObjects = (versionMajor == 0x0700);

    constexpr int kBloodMapHeaderBytes = 37;
    constexpr int kBloodHeader2Bytes = 128;
    constexpr int kMattKey = 0x7474614d; // bytes: 'M','a','t','t'
    constexpr int kLegacyXSectorBytes = 60;
    constexpr int kLegacyXWallBytes = 24;
    constexpr int kLegacyXSpriteBytes = 56;

    size_t pos = 6;
    std::vector<unsigned char> header(file.begin() + pos, file.begin() + pos + kBloodMapHeaderBytes);
    pos += kBloodMapHeaderBytes;

    BloodMapHeaderLite mh = bloodParseMapHeader(header);
    if (!bloodHeaderLooksValid(mh))
    {
        std::vector<unsigned char> decoded = header;
        bloodCryptBytes(decoded.data(), decoded.size(), kMattKey);
        BloodMapHeaderLite decodedHeader = bloodParseMapHeader(decoded);
        if (!bloodHeaderLooksValid(decodedHeader)) return false;
        header.swap(decoded);
        mh = decodedHeader;
    }

    int xSpriteBytes = kLegacyXSpriteBytes;
    int xWallBytes = kLegacyXWallBytes;
    int xSectorBytes = kLegacyXSectorBytes;
    if (encryptedObjects)
    {
        if (pos + kBloodHeader2Bytes > file.size()) return false;
        std::vector<unsigned char> header2(file.begin() + pos, file.begin() + pos + kBloodHeader2Bytes);
        pos += kBloodHeader2Bytes;
        bloodCryptBytes(header2.data(), header2.size(), mh.numWalls);
        xSpriteBytes = bloodSafeXDataSize(static_cast<int>(bloodReadI32(header2.data() + 0x40)), kLegacyXSpriteBytes);
        xWallBytes = bloodSafeXDataSize(static_cast<int>(bloodReadI32(header2.data() + 0x44)), kLegacyXWallBytes);
        xSectorBytes = bloodSafeXDataSize(static_cast<int>(bloodReadI32(header2.data() + 0x48)), kLegacyXSectorBytes);
    }

    const int skyCount = 1 << mh.skyBits;
    if (skyCount < 0 || skyCount > MAXPSKYTILES) return false;
    if (pos + static_cast<size_t>(skyCount) * 2 > file.size()) return false;
    std::vector<unsigned char> skyBytes(file.begin() + pos, file.begin() + pos + static_cast<size_t>(skyCount) * 2);
    pos += static_cast<size_t>(skyCount) * 2;
    if (encryptedObjects) bloodCryptBytes(skyBytes.data(), skyBytes.size(), skyCount * 2);

    std::memset(sector, 0, sizeof(sector));
    std::memset(wall, 0, sizeof(wall));
    std::memset(sprite, 0, sizeof(sprite));
    initspritelists();

    numsectors = static_cast<short>(mh.numSectors);
    numwalls = static_cast<short>(mh.numWalls);
    pskybits = mh.skyBits;
    visibility = mh.vis;
    parallaxtype = mh.parallaxType;
    for (int i = 0; i < MAXPSKYTILES; ++i) pskyoff[i] = 0;
    for (int i = 0; i < skyCount; ++i) pskyoff[i] = bloodReadI16(skyBytes.data() + i * 2);

    const int sectorCryptKey = static_cast<int>(mh.mapRev * static_cast<long>(sizeof(sectortype)));
    const int wallCryptKey = sectorCryptKey | kMattKey;
    const int spriteCryptKey = static_cast<int>(mh.mapRev * static_cast<long>(sizeof(spritetype))) | kMattKey;

    for (short i = 0; i < numsectors; ++i)
    {
        if (pos + sizeof(sectortype) > file.size()) return false;
        std::vector<unsigned char> raw(file.begin() + pos, file.begin() + pos + sizeof(sectortype));
        pos += sizeof(sectortype);
        if (encryptedObjects) bloodCryptBytes(raw.data(), raw.size(), sectorCryptKey);
        std::memcpy(&sector[i], raw.data(), sizeof(sectortype));
        if (sector[i].extra > 0)
        {
            if (pos + static_cast<size_t>(xSectorBytes) > file.size()) return false;
            pos += static_cast<size_t>(xSectorBytes);
        }
    }

    for (short i = 0; i < numwalls; ++i)
    {
        if (pos + sizeof(walltype) > file.size()) return false;
        std::vector<unsigned char> raw(file.begin() + pos, file.begin() + pos + sizeof(walltype));
        pos += sizeof(walltype);
        if (encryptedObjects) bloodCryptBytes(raw.data(), raw.size(), wallCryptKey);
        std::memcpy(&wall[i], raw.data(), sizeof(walltype));
        if (wall[i].extra > 0)
        {
            if (pos + static_cast<size_t>(xWallBytes) > file.size()) return false;
            pos += static_cast<size_t>(xWallBytes);
        }
    }

    initspritelists();
    for (std::uint16_t i = 0; i < mh.numSprites; ++i)
    {
        if (pos + sizeof(spritetype) > file.size()) return false;
        spritetype sp{};
        std::vector<unsigned char> raw(file.begin() + pos, file.begin() + pos + sizeof(spritetype));
        pos += sizeof(spritetype);
        if (encryptedObjects) bloodCryptBytes(raw.data(), raw.size(), spriteCryptKey);
        std::memcpy(&sp, raw.data(), sizeof(spritetype));
        if (sp.extra > 0)
        {
            if (pos + static_cast<size_t>(xSpriteBytes) > file.size()) return false;
            pos += static_cast<size_t>(xSpriteBytes);
        }

        if (sp.sectnum < 0 || sp.sectnum >= numsectors || sp.statnum < 0 || sp.statnum >= MAXSTATUS)
            continue;

        const short slot = insertsprite(sp.sectnum, sp.statnum);
        if (slot >= 0) sprite[slot] = sp;
    }

    bloodSanitizeLoadedMap();

    if (daposx) *daposx = mh.px;
    if (daposy) *daposy = mh.py;
    if (daposz) *daposz = mh.pz;
    if (daang) *daang = static_cast<short>(mh.ang & 2047);
    if (dacursectnum)
    {
        *dacursectnum = mh.sect;
        if (*dacursectnum < 0 || *dacursectnum >= numsectors)
            updatesector(mh.px, mh.py, dacursectnum);
    }

    return true;
}


static void tryMountBloodResourcesNearMap(const char* mapPath)
{
    if (!isBloodMode() || !mapPath || !mapPath[0]) return;
    if (g.bloodRffPrepared && !g.bloodExtractPath.empty())
    {
        reloadCurrentGamePalette();
        return;
    }

    const std::string mapDir = pathDirectory(mapPath);
    const std::string candidates[] = {
        pathJoin(mapDir, "BLOOD.RFF"),
        pathJoin(mapDir, "blood.rff"),
        pathJoin(moduleDirectory(), "BLOOD.RFF"),
        pathJoin(moduleDirectory(), "blood.rff"),
        "BLOOD.RFF",
        "blood.rff",
    };

    for (const std::string& rff : candidates)
    {
        if (rff.empty() || !fileExistsA(rff.c_str())) continue;
        if (!prepareBloodRffCache(rff)) continue;
        reloadCurrentGamePalette();
        loadBloodArtSetFromCurrentData();
        return;
    }
}

static bool loadMapFile(const char* path)
{
    if (!path || !path[0]) return false;
    char filename[MAX_PATH];
    std::strncpy(filename, path, sizeof(filename) - 1);
    filename[sizeof(filename) - 1] = 0;

    tryMountBloodResourcesNearMap(filename);

    long px = 32768, py = 32768, pz = 0;
    short a = 1536, s = -1;
    bool loaded = false;

    if (isBloodMode())
    {
        // Blood maps are BLM files, not raw Duke/BUILD mapversion-7 files.
        // Try the Blood loader first; fall back to the classic loader so the
        // Blood profile can still open test BUILD maps.
        loaded = loadBloodMapAsBuild(filename, &px, &py, &pz, &a, &s);
    }

    if (!loaded)
        loaded = loadboard(filename, &px, &py, &pz, &a, &s) >= 0;

    if (!loaded)
    {
        MessageBoxA(g.main,
            isBloodMode()
            ? "Could not load that map. Tried Blood BLM and classic BUILD map loaders; a new square map was created instead."
            : "Could not load that map. A new square map was created instead.",
            "Open map", MB_ICONWARNING);
        initNewMap();
        return false;
    }

    g.boardPath = path;
    g.boardFromGrp = false;
    g.boardGrpPath.clear();
    g.boardGrpEntry.clear();
    g.cam.x = px; g.cam.y = py; g.cam.z = pz; g.cam.ang = a; g.cam.sect = s; g.cam.horiz = 100;
    updatesector(g.cam.x, g.cam.y, &g.cam.sect);
    g.viewX = g.cam.x;
    g.viewY = g.cam.y;
    g.startx = px; g.starty = py; g.startz = pz; g.startang = a; g.startsect = g.cam.sect;
    g.sel.clear();
    syncEngineSearchGlobals(g.sel);
    rebuildSectorLinks();
    if (isBloodMode()) bloodNormalizeMapForBlood(true);
    setDirty(false);
    setStatus(isBloodMode() ? "Loaded Blood map %s" : "Loaded %s", path);
    refreshAllViews();
    return true;
}

static bool saveMapFile(const char* path)
{
    if (!path || !path[0]) return false;
    updatesector(g.startx, g.starty, &g.startsect);
    if (isBloodMode()) bloodNormalizeMapForBlood(true);
    char filename[MAX_PATH];
    std::strncpy(filename, path, sizeof(filename) - 1);
    filename[sizeof(filename) - 1] = 0;
    if (saveboard(filename, &g.startx, &g.starty, &g.startz, &g.startang, &g.startsect) < 0)
    {
        MessageBoxA(g.main, "Could not save the map.", "Save map", MB_ICONERROR);
        return false;
    }
    g.boardPath = path;
    g.boardFromGrp = false;
    g.boardGrpPath.clear();
    g.boardGrpEntry.clear();
    setDirty(false);
    setStatus("Saved %s", path);
    return true;
}

struct GrpMapEntry
{
    std::string name;
    std::uint32_t offset = 0;
    std::uint32_t size = 0;
    std::uint32_t packedSize = 0;
    std::uint32_t id = 0;
    std::uint16_t archiveVersion = 0;
    std::uint8_t flags = 0;
    bool fromRff = false;
};

static std::uint32_t readLe32(const unsigned char b[4])
{
    return static_cast<std::uint32_t>(b[0]) |
        (static_cast<std::uint32_t>(b[1]) << 8) |
        (static_cast<std::uint32_t>(b[2]) << 16) |
        (static_cast<std::uint32_t>(b[3]) << 24);
}

static std::uint16_t readLe16(const unsigned char b[2])
{
    return static_cast<std::uint16_t>(b[0]) |
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(b[1]) << 8);
}

static bool readExact(FILE* f, void* dst, size_t bytes)
{
    return std::fread(dst, 1, bytes, f) == bytes;
}

static bool hasMapExtension(const std::string& name)
{
    const size_t n = name.size();
    if (n < 4 || name[n - 4] != '.') return false;
    const char a = name[n - 3], b = name[n - 2], c = name[n - 1];
    return (a == 'm' || a == 'M') && (b == 'a' || b == 'A') && (c == 'p' || c == 'P');
}

static std::string upperAscii(std::string s)
{
    for (char& ch : s)
        if (ch >= 'a' && ch <= 'z') ch = static_cast<char>(ch - 'a' + 'A');
    return s;
}

static std::string fullPathOf(const char* path)
{
    if (!path || !path[0]) return std::string();
    char full[MAX_PATH] = {};
    DWORD n = GetFullPathNameA(path, sizeof(full), full, nullptr);
    if (n == 0 || n >= sizeof(full)) return path;
    return full;
}

static bool fileExistsA(const char* path)
{
    if (!path || !path[0]) return false;
    DWORD attr = GetFileAttributesA(path);
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static std::string pathDirectory(const std::string& path)
{
    size_t p1 = path.find_last_of('\\');
    size_t p2 = path.find_last_of('/');
    size_t p = std::string::npos;
    if (p1 != std::string::npos && p2 != std::string::npos) p = max(p1, p2);
    else p = (p1 != std::string::npos) ? p1 : p2;
    if (p == std::string::npos) return ".";
    if (p == 0) return path.substr(0, 1);
    return path.substr(0, p);
}

static std::string pathJoin(const std::string& dir, const std::string& name)
{
    if (dir.empty() || dir == ".") return name;
    char last = dir[dir.size() - 1];
    if (last == '\\' || last == '/') return dir + name;
    return dir + "\\" + name;
}

static std::string pathBaseNameString(const std::string& path)
{
    size_t p1 = path.find_last_of('\\');
    size_t p2 = path.find_last_of('/');
    size_t p = std::string::npos;
    if (p1 != std::string::npos && p2 != std::string::npos) p = max(p1, p2);
    else p = (p1 != std::string::npos) ? p1 : p2;
    return p == std::string::npos ? path : path.substr(p + 1);
}

static std::string forceMapExtension(std::string s)
{
    const std::string up = upperAscii(s);
    if (up.size() < 4 || up.substr(up.size() - 4) != ".MAP") s += ".map";
    return s;
}

static std::string sanitizeFileName(std::string s)
{
    if (s.empty()) s = "BuildXRun.map";
    for (char& ch : s)
    {
        if (ch == '<' || ch == '>' || ch == ':' || ch == '"' || ch == '/' || ch == '\\' || ch == '|' || ch == '?' || ch == '*')
            ch = '_';
    }
    return s;
}

static std::string trimFixedArchiveString(const char* raw, size_t len)
{
    size_t n = 0;
    while (n < len && raw[n] != 0) ++n;
    while (n > 0 && (raw[n - 1] == ' ' || raw[n - 1] == '\t')) --n;
    return std::string(raw, raw + n);
}

static bool isRffSignature(const unsigned char header[4])
{
    return header && header[0] == 'R' && header[1] == 'F' && header[2] == 'F' && header[3] == 0x1a;
}

static bool isRffArchivePath(const char* path)
{
    if (!path || !path[0]) return false;
    FILE* f = nullptr;
    if (fopen_s(&f, path, "rb") != 0 || !f) return false;
    unsigned char header[4] = {};
    const bool ok = readExact(f, header, sizeof(header)) && isRffSignature(header);
    std::fclose(f);
    return ok;
}

static bool isGrpArchivePath(const char* path)
{
    if (!path || !path[0]) return false;
    FILE* f = nullptr;
    if (fopen_s(&f, path, "rb") != 0 || !f) return false;
    char signature[12] = {};
    const bool ok = readExact(f, signature, sizeof(signature)) && std::memcmp(signature, "KenSilverman", 12) == 0;
    std::fclose(f);
    return ok;
}

static bool isWritableGrpArchivePath(const std::string& path)
{
    return !path.empty() && isGrpArchivePath(path.c_str()) && !isRffArchivePath(path.c_str());
}

static bool archiveEntryNameEquals(const std::string& a, const char* b)
{
    return upperAscii(a) == upperAscii(b ? b : "");
}

static bool readRffEntries(const char* rffPath, std::vector<GrpMapEntry>& entries)
{
    entries.clear();
    if (!rffPath || !rffPath[0]) return false;

    FILE* f = nullptr;
    if (fopen_s(&f, rffPath, "rb") != 0 || !f) return false;

    unsigned char header[32] = {};
    if (!readExact(f, header, sizeof(header)) || !isRffSignature(header))
    {
        std::fclose(f);
        return false;
    }

    const std::uint16_t version = readLe16(header + 4);
    const std::uint32_t dictOffset = readLe32(header + 8);
    const std::uint32_t dictEntries = readLe32(header + 12);
    const std::uint64_t fatBytes = static_cast<std::uint64_t>(dictEntries) * 48ull;

    if (dictEntries > 1000000u || fatBytes > 0xffffffffull)
    {
        std::fclose(f);
        return false;
    }

    if (std::fseek(f, 0, SEEK_END) != 0)
    {
        std::fclose(f);
        return false;
    }
    const long fileEnd = std::ftell(f);
    if (fileEnd < 0 || static_cast<std::uint64_t>(dictOffset) + fatBytes > static_cast<std::uint64_t>(fileEnd))
    {
        std::fclose(f);
        return false;
    }

    if (std::fseek(f, static_cast<long>(dictOffset), SEEK_SET) != 0)
    {
        std::fclose(f);
        return false;
    }

    std::vector<unsigned char> fat(static_cast<size_t>(fatBytes));
    if (!fat.empty() && std::fread(fat.data(), 1, fat.size(), f) != fat.size())
    {
        std::fclose(f);
        return false;
    }
    std::fclose(f);

    if (version >= 0x0300)
    {
        std::uint8_t key = static_cast<std::uint8_t>(dictOffset & 0xff);
        for (size_t i = 0; i < fat.size(); ++i)
        {
            if (version == 0x0300)
            {
                fat[i] = static_cast<unsigned char>(fat[i] ^ (key >> 1));
                ++key;
            }
            else
            {
                fat[i] = static_cast<unsigned char>(fat[i] ^ key);
                key = static_cast<std::uint8_t>(key + (i & 1));
            }
        }
    }

    entries.reserve(dictEntries);
    for (std::uint32_t i = 0; i < dictEntries; ++i)
    {
        const unsigned char* e = fat.data() + static_cast<size_t>(i) * 48u;
        const std::uint32_t offset = readLe32(e + 16);
        const std::uint32_t size = readLe32(e + 20);
        const std::uint32_t packedSize = readLe32(e + 24);
        const std::uint8_t flags = e[32];
        const std::string ext = trimFixedArchiveString(reinterpret_cast<const char*>(e + 33), 3);
        const std::string stem = trimFixedArchiveString(reinterpret_cast<const char*>(e + 36), 8);
        const std::uint32_t id = readLe32(e + 44);

        GrpMapEntry ge;
        ge.offset = offset;
        ge.size = size;
        ge.packedSize = packedSize;
        ge.id = id;
        ge.archiveVersion = version;
        ge.flags = flags;
        ge.fromRff = true;

        if ((flags & 0x01) && stem.empty())
        {
            char idName[64] = {};
            if (!ext.empty()) std::snprintf(idName, sizeof(idName), "%u.%s", id, ext.c_str());
            else std::snprintf(idName, sizeof(idName), "%u", id);
            ge.name = idName;
        }
        else if (!ext.empty())
        {
            ge.name = stem + "." + ext;
        }
        else
        {
            ge.name = stem;
        }

        if (ge.name.empty())
        {
            char fallback[64] = {};
            std::snprintf(fallback, sizeof(fallback), "RFF_%u.BIN", i);
            ge.name = fallback;
        }

        entries.push_back(ge);
    }

    std::sort(entries.begin(), entries.end(), [](const GrpMapEntry& a, const GrpMapEntry& b)
        {
            return upperAscii(a.name) < upperAscii(b.name);
        });
    return true;
}

static bool readKenSilvermanGrpEntries(const char* grpPath, std::vector<GrpMapEntry>& entries)
{
    entries.clear();
    if (!grpPath || !grpPath[0]) return false;

    FILE* f = nullptr;
    if (fopen_s(&f, grpPath, "rb") != 0 || !f) return false;

    char signature[12] = {};
    unsigned char countBytes[4] = {};
    if (!readExact(f, signature, sizeof(signature)) || !readExact(f, countBytes, sizeof(countBytes)) ||
        std::memcmp(signature, "KenSilverman", 12) != 0)
    {
        std::fclose(f);
        return false;
    }

    const std::uint32_t count = readLe32(countBytes);
    std::uint64_t dataOffset = 16ull + static_cast<std::uint64_t>(count) * 16ull;

    for (std::uint32_t i = 0; i < count; ++i)
    {
        char rawName[12] = {};
        unsigned char sizeBytes[4] = {};
        if (!readExact(f, rawName, sizeof(rawName)) || !readExact(f, sizeBytes, sizeof(sizeBytes)))
        {
            std::fclose(f);
            return false;
        }

        size_t nameLen = 0;
        while (nameLen < sizeof(rawName) && rawName[nameLen] != 0) ++nameLen;
        const std::uint32_t size = readLe32(sizeBytes);

        if (dataOffset <= 0xffffffffull)
        {
            GrpMapEntry e;
            e.name.assign(rawName, rawName + nameLen);
            e.offset = static_cast<std::uint32_t>(dataOffset);
            e.size = size;
            e.fromRff = false;
            entries.push_back(e);
        }
        dataOffset += size;
    }

    std::fclose(f);
    return true;
}

static bool readGrpEntries(const char* grpPath, std::vector<GrpMapEntry>& entries)
{
    entries.clear();
    if (readRffEntries(grpPath, entries)) return true;
    return readKenSilvermanGrpEntries(grpPath, entries);
}

static bool findGrpEntry(const char* grpPath, const char* wantedName, GrpMapEntry& out)
{
    std::vector<GrpMapEntry> entries;
    if (!readGrpEntries(grpPath, entries)) return false;
    const std::string wanted = upperAscii(wantedName ? wantedName : "");
    for (const GrpMapEntry& e : entries)
    {
        if (upperAscii(e.name) == wanted)
        {
            out = e;
            return true;
        }
    }
    return false;
}

static bool readRffExternalEntryBytes(const char* rffPath, const GrpMapEntry& entry, std::vector<unsigned char>& data)
{
    data.clear();
    const std::string path = pathJoin(pathDirectory(rffPath ? rffPath : "."), entry.name);
    FILE* f = nullptr;
    if (fopen_s(&f, path.c_str(), "rb") != 0 || !f) return false;
    data.resize(entry.size);
    const bool ok = entry.size == 0 || std::fread(data.data(), 1, entry.size, f) == entry.size;
    std::fclose(f);
    if (!ok) data.clear();
    return ok;
}

static bool readGrpEntryBytes(const char* grpPath, const GrpMapEntry& entry, std::vector<unsigned char>& data)
{
    data.clear();
    if (!grpPath || !grpPath[0]) return false;

    if (entry.fromRff || isRffArchivePath(grpPath))
    {
        if (entry.flags & 0x02)
            return readRffExternalEntryBytes(grpPath, entry, data);
    }

    FILE* f = nullptr;
    if (fopen_s(&f, grpPath, "rb") != 0 || !f) return false;
    if (std::fseek(f, static_cast<long>(entry.offset), SEEK_SET) != 0)
    {
        std::fclose(f);
        return false;
    }
    data.resize(entry.size);
    bool ok = entry.size == 0 || std::fread(data.data(), 1, entry.size, f) == entry.size;
    std::fclose(f);

    if (ok && (entry.fromRff || isRffArchivePath(grpPath)) && (entry.flags & 0x10))
    {
        const size_t n = std::min<size_t>(256, data.size());
        for (size_t i = 0; i < n; ++i)
            data[i] = static_cast<unsigned char>(data[i] ^ (i >> 1));
    }

    if (!ok) data.clear();
    return ok;
}

static bool loadPaletteFromGrp(const char* grpPath)
{
    GrpMapEntry palEntry;
    if (!findGrpEntry(grpPath, "PALETTE.DAT", palEntry)) return false;
    std::vector<unsigned char> bytes;
    if (!readGrpEntryBytes(grpPath, palEntry, bytes) || bytes.size() < 768) return false;
    for (int i = 0; i < 768; ++i) palette[i] = static_cast<char>(bytes[static_cast<size_t>(i)]);
    FlushTextureCache();
    return true;
}


static bool loadPaletteFromLooseFile(const char* path)
{
    if (!path || !path[0]) return false;
    FILE* f = nullptr;
    if (fopen_s(&f, path, "rb") != 0 || !f) return false;
    unsigned char bytes[768] = {};
    const bool ok = std::fread(bytes, 1, sizeof(bytes), f) == sizeof(bytes);
    std::fclose(f);
    if (!ok) return false;
    for (int i = 0; i < 768; ++i) palette[i] = static_cast<char>(bytes[i]);
    FlushTextureCache();
    return true;
}

static bool loadBloodPaletteFromFolder(const std::string& folder)
{
    const std::string pal1 = pathJoin(folder.empty() ? "." : folder, "PALETTE.DAT");
    const std::string pal2 = pathJoin(folder.empty() ? "." : folder, "palette.dat");
    if (loadPaletteFromLooseFile(pal1.c_str())) return true;
    if (loadPaletteFromLooseFile(pal2.c_str())) return true;
    return false;
}

static bool reloadCurrentGamePalette()
{
    if (isBloodMode())
    {
        if (!g.bloodExtractPath.empty() && loadBloodPaletteFromFolder(g.bloodExtractPath))
            return true;
        if (!g.bloodRffPath.empty() && fileExistsA(g.bloodRffPath.c_str()) && loadPaletteFromGrp(g.bloodRffPath.c_str()))
            return true;
        if (!g.dukeGrpPath.empty() && fileExistsA(g.dukeGrpPath.c_str()))
        {
            if (isRffArchivePath(g.dukeGrpPath.c_str()) && loadPaletteFromGrp(g.dukeGrpPath.c_str()))
            {
                g.bloodRffPath = g.dukeGrpPath;
                return true;
            }
            return loadBloodPaletteFromFolder(pathDirectory(g.dukeGrpPath));
        }
        return loadBloodPaletteFromFolder(".");
    }

    if (!g.dukeGrpPath.empty() && fileExistsA(g.dukeGrpPath.c_str()))
        return loadPaletteFromGrp(g.dukeGrpPath.c_str());
    return false;
}


static bool readGrpMapEntries(const char* grpPath, std::vector<GrpMapEntry>& maps)
{
    maps.clear();
    std::vector<GrpMapEntry> entries;
    if (!readGrpEntries(grpPath, entries)) return false;
    for (const GrpMapEntry& e : entries)
        if (hasMapExtension(e.name)) maps.push_back(e);
    std::sort(maps.begin(), maps.end(), [](const GrpMapEntry& a, const GrpMapEntry& b)
        {
            return upperAscii(a.name) < upperAscii(b.name);
        });
    return true;
}

static bool extractGrpEntryToTemp(const char* grpPath, const GrpMapEntry& entry, std::string& tempPath)
{
    tempPath.clear();

    char tempDir[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    if (!GetTempPathA(sizeof(tempDir), tempDir) || !GetTempFileNameA(tempDir, "BXM", 0, tempFile))
        return false;

    std::vector<unsigned char> bytes;
    if (!readGrpEntryBytes(grpPath, entry, bytes))
    {
        DeleteFileA(tempFile);
        return false;
    }

    FILE* out = nullptr;
    if (fopen_s(&out, tempFile, "wb") != 0 || !out)
    {
        DeleteFileA(tempFile);
        return false;
    }
    const bool ok = bytes.empty() || std::fwrite(bytes.data(), 1, bytes.size(), out) == bytes.size();
    std::fclose(out);

    if (!ok)
    {
        DeleteFileA(tempFile);
        return false;
    }

    tempPath = tempFile;
    return true;
}

static const char* pathBaseName(const char* path)
{
    if (!path) return "";
    const char* slash1 = std::strrchr(path, '\\');
    const char* slash2 = std::strrchr(path, '/');
    const char* slash = nullptr;
    if (slash1 && slash2) slash = (slash1 > slash2) ? slash1 : slash2;
    else slash = slash1 ? slash1 : slash2;
    return slash ? slash + 1 : path;
}

static bool promptOpenMapFromGrp()
{
    char grpFile[MAX_PATH] = "";
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g.main;
    ofn.lpstrFilter = isBloodMode()
        ? "Blood/BUILD archives (*.rff;*.grp)\0*.rff;*.grp\0All files (*.*)\0*.*\0"
        : "BUILD group files (*.grp)\0*.grp\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = grpFile;
    ofn.nMaxFile = sizeof(grpFile);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = isBloodMode() ? "rff" : "grp";
    if (!GetOpenFileNameA(&ofn)) return false;

    std::vector<GrpMapEntry> maps;
    if (!readGrpMapEntries(grpFile, maps))
    {
        MessageBoxA(g.main, "Could not read that archive. Select a Ken Silverman .grp or Blood .rff file.", "Open from archive", MB_ICONERROR);
        return false;
    }
    if (maps.empty())
    {
        MessageBoxA(g.main, "That archive does not contain any .map entries.", "Open from archive", MB_ICONINFORMATION);
        return false;
    }

    int choice = 0;
    if (maps.size() > 1)
    {
        std::vector<std::string> names;
        names.reserve(maps.size());
        for (const GrpMapEntry& e : maps) names.push_back(e.name);
        if (!chooseStringIndex("Open map from archive", "Select a map inside the archive:", names, choice))
            return false;
    }

    std::string tempMap;
    if (!extractGrpEntryToTemp(grpFile, maps[choice], tempMap))
    {
        MessageBoxA(g.main, "Could not extract that map from the archive.", "Open from archive", MB_ICONERROR);
        return false;
    }

    const bool ok = loadMapFile(tempMap.c_str());
    DeleteFileA(tempMap.c_str());
    if (!ok) return false;

    g.boardFromGrp = true;
    g.boardGrpPath = grpFile;
    g.dukeGrpPath = fullPathOf(grpFile);
    g.grpEditPath = g.dukeGrpPath;
    if (isBloodMode() && isRffArchivePath(g.dukeGrpPath.c_str()))
    {
        g.bloodDataPath = g.dukeGrpPath;
        g.bloodRffPath = g.dukeGrpPath;
        prepareBloodRffCache(g.bloodRffPath);
        loadBloodArtSetFromCurrentData();
    }
    if (g.grpExportPath.empty()) g.grpExportPath = pathJoin(pathDirectory(g.grpEditPath), isRffArchivePath(g.grpEditPath.c_str()) ? "rff_export" : "grp_export");
    updateGrpEditPanel();
    loadPaletteFromGrp(g.dukeGrpPath.c_str());
    g.boardGrpEntry = maps[choice].name;
    g.boardPath = std::string(maps[choice].name) + " (" + pathBaseName(grpFile) + ")";
    setDirty(false);
    updateTitle();
    setStatus("Loaded %s from %s", maps[choice].name.c_str(), pathBaseName(grpFile));
    refreshAllViews();
    return true;
}

static bool promptOpenMap()
{
    char file[MAX_PATH] = "";
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g.main;
    ofn.lpstrFilter = "BUILD maps (*.map)\0*.map\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = sizeof(file);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "map";
    if (!GetOpenFileNameA(&ofn)) return false;
    return loadMapFile(file);
}

static bool promptSaveMap()
{
    char file[MAX_PATH] = "";
    if (!g.boardFromGrp)
        std::strncpy(file, g.boardPath.c_str(), sizeof(file) - 1);
    else if (!g.boardGrpEntry.empty())
        std::strncpy(file, g.boardGrpEntry.c_str(), sizeof(file) - 1);
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g.main;
    ofn.lpstrFilter = "BUILD maps (*.map)\0*.map\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = sizeof(file);
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "map";
    if (!GetSaveFileNameA(&ofn)) return false;
    return saveMapFile(file);
}

static bool insertPointOnWall(short wi, long x, long y)
{
    if (wi < 0 || wi >= numwalls || numwalls >= MAXWALLS - 1) return false;
    const short sect = ownerSectorOfWall(wi);
    if (sect < 0) return false;

    const short insertAt = static_cast<short>(wi + 1);
    const short oldPoint2 = wall[wi].point2;

    for (int i = numwalls; i > insertAt; --i)
        wall[i] = wall[i - 1];

    for (short s = 0; s < numsectors; ++s)
    {
        if (sector[s].wallptr >= insertAt) sector[s].wallptr++;
    }
    sector[sect].wallnum++;

    for (short i = 0; i <= numwalls; ++i)
    {
        if (i == insertAt) continue;
        if (wall[i].point2 >= insertAt) wall[i].point2++;
        if (wall[i].nextwall >= insertAt) wall[i].nextwall++;
    }

    wall[insertAt] = wall[wi];
    wall[insertAt].x = x;
    wall[insertAt].y = y;
    wall[insertAt].point2 = static_cast<short>(oldPoint2 >= insertAt ? oldPoint2 + 1 : oldPoint2);
    wall[insertAt].nextwall = -1;
    wall[insertAt].nextsector = -1;
    wall[wi].point2 = insertAt;
    ++numwalls;

    fixWallRepeat(wi);
    fixWallRepeat(insertAt);
    rebuildSectorLinks();
    setDirty();
    refreshAllViews();
    return true;
}

static bool deletePoint(short wi)
{
    if (wi < 0 || wi >= numwalls) return false;
    const short sect = ownerSectorOfWall(wi);
    if (sect < 0 || sector[sect].wallnum <= 3) return false;

    short prev = -1;
    const int start = sector[sect].wallptr;
    const int end = start + sector[sect].wallnum;
    for (int i = start; i < end; ++i)
    {
        if (wall[i].point2 == wi) { prev = static_cast<short>(i); break; }
    }
    if (prev < 0) return false;

    const short next = wall[wi].point2;
    wall[prev].point2 = next;

    for (int i = wi; i < numwalls - 1; ++i)
        wall[i] = wall[i + 1];

    --numwalls;
    sector[sect].wallnum--;

    for (short s = 0; s < numsectors; ++s)
        if (sector[s].wallptr > wi) sector[s].wallptr--;

    for (short i = 0; i < numwalls; ++i)
    {
        if (wall[i].point2 > wi) wall[i].point2--;
        else if (wall[i].point2 == wi) wall[i].point2 = static_cast<short>(next > wi ? next - 1 : next);
        if (wall[i].nextwall >= wi) wall[i].nextwall = -1;
        wall[i].nextsector = -1;
    }

    fixWallRepeat(prev > wi ? prev - 1 : prev);
    rebuildSectorLinks();
    g.sel.clear();
    syncEngineSearchGlobals(g.sel);
    setDirty();
    refreshAllViews();
    return true;
}


static void rebuildSpriteListsKeepingSprites()
{
    std::array<spritetype, MAXSPRITES> oldSprites;
    for (int i = 0; i < MAXSPRITES; ++i) oldSprites[i] = sprite[i];

    for (int i = 0; i <= MAXSECTORS; ++i) headspritesect[i] = -1;
    for (int i = 0; i <= MAXSTATUS; ++i) headspritestat[i] = -1;
    for (int i = 0; i < MAXSPRITES; ++i)
    {
        prevspritesect[i] = nextspritesect[i] = -1;
        prevspritestat[i] = nextspritestat[i] = -1;
        sprite[i] = oldSprites[i];
    }

    auto linkSect = [](short i, short sect)
        {
            prevspritesect[i] = -1;
            nextspritesect[i] = headspritesect[sect];
            if (headspritesect[sect] >= 0) prevspritesect[headspritesect[sect]] = i;
            headspritesect[sect] = i;
        };
    auto linkStat = [](short i, short stat)
        {
            prevspritestat[i] = -1;
            nextspritestat[i] = headspritestat[stat];
            if (headspritestat[stat] >= 0) prevspritestat[headspritestat[stat]] = i;
            headspritestat[stat] = i;
        };

    for (short i = 0; i < MAXSPRITES; ++i)
    {
        bool active = sprite[i].sectnum >= 0 && sprite[i].sectnum < numsectors &&
            sprite[i].statnum >= 0 && sprite[i].statnum < MAXSTATUS;
        if (active)
        {
            linkSect(i, sprite[i].sectnum);
            linkStat(i, sprite[i].statnum);
        }
        else
        {
            sprite[i].sectnum = MAXSECTORS;
            sprite[i].statnum = MAXSTATUS;
            linkSect(i, MAXSECTORS);
            linkStat(i, MAXSTATUS);
        }
    }
}

static bool deleteSector(short sect)
{
    if (sect < 0 || sect >= numsectors) return false;
    const int start = sector[sect].wallptr;
    const int count = sector[sect].wallnum;
    if (start < 0 || count <= 0 || start + count > numwalls) return false;

    for (short spr = headspritesect[sect]; spr >= 0; )
    {
        short next = nextspritesect[spr];
        deletesprite(spr);
        spr = next;
    }

    for (int i = start; i + count < numwalls; ++i)
        wall[i] = wall[i + count];
    numwalls = static_cast<short>(numwalls - count);

    for (short s = sect; s < numsectors - 1; ++s)
        sector[s] = sector[s + 1];
    --numsectors;

    for (short s = 0; s < numsectors; ++s)
        if (sector[s].wallptr > start) sector[s].wallptr = static_cast<short>(sector[s].wallptr - count);

    for (short i = 0; i < numwalls; ++i)
    {
        if (wall[i].point2 >= start + count) wall[i].point2 = static_cast<short>(wall[i].point2 - count);
        else if (wall[i].point2 >= start) wall[i].point2 = i;
        wall[i].nextwall = -1;
        wall[i].nextsector = -1;
    }

    for (short s = 0; s < MAXSPRITES; ++s)
    {
        if (sprite[s].sectnum == sect) deletesprite(s);
        else if (sprite[s].sectnum > sect && sprite[s].sectnum < MAXSECTORS) sprite[s].sectnum--;
    }

    rebuildSpriteListsKeepingSprites();
    rebuildSectorLinks();
    g.sel.clear();
    syncEngineSearchGlobals(g.sel);
    setDirty();
    refreshAllViews();
    return true;
}

static bool moveJoinedPoint(short wi, long nx, long ny)
{
    if (wi < 0 || wi >= numwalls) return false;
    const long ox = wall[wi].x;
    const long oy = wall[wi].y;
    snapPoint(nx, ny);
    if (nx == ox && ny == oy) return false;

    bool changed = false;
    for (short i = 0; i < numwalls; ++i)
    {
        if (wall[i].x == ox && wall[i].y == oy)
        {
            wall[i].x = nx;
            wall[i].y = ny;
            changed = true;
        }
    }

    if (changed)
    {
        g.dragPointMoved = true;
        markDirtyDuringDrag();
    }
    return changed;
}

static void finalizePointDrag()
{
    if (!g.dragPointMoved) return;

    // Wall lengths and red-wall joins only need to be recalculated once after
    // the user releases the mouse. Doing this during every WM_MOUSEMOVE made
    // large maps feel extremely slow.
    for (short i = 0; i < numwalls; ++i) fixWallRepeat(i);
    rebuildSectorLinks();
    setStatus("Point moved.");
    refreshAllViews();
}

static bool commitDrawnSector()
{
    if (!g.drawingSector || g.drawPts.size() < 3) return false;
    if (numsectors >= MAXSECTORS || numwalls + static_cast<int>(g.drawPts.size()) >= MAXWALLS)
    {
        MessageBoxA(g.main, "The map has reached the BUILD sector/wall limit.", "Sector drawing", MB_ICONERROR);
        return false;
    }

    // Remove duplicate closing point if present.
    if (g.drawPts.size() >= 2)
    {
        POINT a = g.drawPts.front();
        POINT b = g.drawPts.back();
        if (a.x == b.x && a.y == b.y) g.drawPts.pop_back();
    }
    if (g.drawPts.size() < 3) return false;

    // BUILD outer loops are expected in a consistent order. Use positive signed
    // area in the editor's y-down view, matching the default square above.
    long long area = 0;
    for (size_t i = 0, j = g.drawPts.size() - 1; i < g.drawPts.size(); j = i++)
        area += static_cast<long long>(g.drawPts[j].x) * g.drawPts[i].y - static_cast<long long>(g.drawPts[i].x) * g.drawPts[j].y;
    if (area < 0) std::reverse(g.drawPts.begin(), g.drawPts.end());

    const short newSect = numsectors;
    const short firstWall = numwalls;
    const short n = static_cast<short>(g.drawPts.size());

    sectortype templ{};
    if (g.sel.sector >= 0 && g.sel.sector < numsectors)
        templ = sector[g.sel.sector];
    else if (numsectors > 0)
        templ = sector[0];
    else
    {
        templ.ceilingz = -32768;
        templ.floorz = 32768;
        templ.ceilingpicnum = 0;
        templ.floorpicnum = 0;
        templ.extra = -1;
    }
    templ.wallptr = firstWall;
    templ.wallnum = n;
    templ.extra = -1;
    sector[newSect] = templ;

    for (short i = 0; i < n; ++i)
    {
        walltype w{};
        w.x = g.drawPts[i].x;
        w.y = g.drawPts[i].y;
        w.point2 = static_cast<short>(firstWall + ((i + 1) % n));
        w.nextwall = -1;
        w.nextsector = -1;
        w.picnum = g.selectedTile;
        w.overpicnum = g.selectedTile;
        w.shade = 0;
        w.pal = 0;
        w.xrepeat = 8;
        w.yrepeat = 8;
        w.extra = -1;
        wall[firstWall + i] = w;
    }

    ++numsectors;
    numwalls = static_cast<short>(numwalls + n);
    for (short i = firstWall; i < numwalls; ++i) fixWallRepeat(i);
    rebuildSectorLinks();
    g.drawingSector = false;
    g.drawPts.clear();
    g.sel.stat = SS_FLOOR;
    g.sel.sector = newSect;
    g.sel.wall = firstWall;
    syncEngineSearchGlobals(g.sel);
    setDirty();
    setStatus("Sector %d created.", newSect);
    refreshAllViews();
    return true;
}

static short nearestPointAtScreen(HWND hwnd, int sx, int sy)
{
    // Hit-test vertices in screen space, not snapped map space.  On large maps
    // and off-grid geometry, snapping the mouse before testing can move the
    // test point far enough away that dragging never starts.
    const double th2 = 12.0 * 12.0;
    double best = th2;
    short bestWall = -1;

    for (short i = 0; i < numwalls; ++i)
    {
        int vx = 0, vy = 0;
        mapToScreen(hwnd, wall[i].x, wall[i].y, vx, vy);
        const double d = dist2(sx, sy, vx, vy);
        if (d < best)
        {
            best = d;
            bestWall = i;
        }
    }
    return bestWall;
}

static short nearestSpriteAtScreen(HWND hwnd, int sx, int sy)
{
    // Sprites are small crosses in the 2D view; give them a generous click
    // target and let them win over nearby sector/wall hits. This fixes the
    // common editor problem where clicks on sprites fall through to floors.
    const double th2 = 22.0 * 22.0;
    double best = th2;
    short bestSprite = -1;

    for (short i = 0; i < MAXSPRITES; ++i)
    {
        if (sprite[i].statnum < 0 || sprite[i].statnum >= MAXSTATUS ||
            sprite[i].sectnum < 0 || sprite[i].sectnum >= MAXSECTORS) continue;

        int vx = 0, vy = 0;
        mapToScreen(hwnd, sprite[i].x, sprite[i].y, vx, vy);
        const double d = dist2(sx, sy, vx, vy);
        if (d < best)
        {
            best = d;
            bestSprite = i;
        }
    }
    return bestSprite;
}

static short nearestLineAtScreen(HWND hwnd, int sx, int sy, double* bestT = nullptr)
{
    const double th2 = 7.0 * 7.0;
    double best = th2;
    double t = 0.0;
    short bestWall = -1;

    for (short i = 0; i < numwalls; ++i)
    {
        if (wall[i].point2 < 0 || wall[i].point2 >= numwalls) continue;

        int ax = 0, ay = 0, bx = 0, by = 0;
        mapToScreen(hwnd, wall[i].x, wall[i].y, ax, ay);
        mapToScreen(hwnd, wall[wall[i].point2].x, wall[wall[i].point2].y, bx, by);

        double localT = 0.0;
        const double d = pointSegmentDistance2(sx, sy, ax, ay, bx, by, &localT);
        if (d < best)
        {
            best = d;
            t = localT;
            bestWall = i;
        }
    }

    if (bestT) *bestT = t;
    return bestWall;
}

static Selection pick2DAtScreen(HWND hwnd, int sx, int sy, long mx, long my)
{
    Selection s;

    short spr = nearestSpriteAtScreen(hwnd, sx, sy);
    if (spr >= 0)
    {
        s.stat = SS_SPRITE;
        s.wall = spr;
        s.sector = sprite[spr].sectnum;
        return s;
    }

    short line = nearestLineAtScreen(hwnd, sx, sy);
    if (line >= 0)
    {
        s.stat = (wall[line].cstat & 16) ? SS_MASKED_WALL : SS_WALL;
        s.wall = line;
        s.sector = ownerSectorOfWall(line);
        return s;
    }

    for (short i = 0; i < numsectors; ++i)
    {
        if (inside(mx, my, i) == 1)
        {
            s.stat = SS_FLOOR;
            s.sector = i;
            s.wall = sector[i].wallptr;
            return s;
        }
    }
    return s;
}

static void syncEngineSearchGlobals(const Selection& s)
{
    searchstat = static_cast<short>(s.stat);
    searchsector = s.sector;
    searchwall = s.wall;
    updateInspector();
}

// -----------------------------------------------------------------------------
// Selection property helpers.
// -----------------------------------------------------------------------------
static short getSelectedPic()
{
    if (!g.sel.valid()) return g.selectedTile;
    switch (g.sel.stat)
    {
    case SS_WALL: return wall[g.sel.wall].picnum;
    case SS_MASKED_WALL: return wall[g.sel.wall].overpicnum;
    case SS_CEILING: return sector[g.sel.sector].ceilingpicnum;
    case SS_FLOOR: return sector[g.sel.sector].floorpicnum;
    case SS_SPRITE: return sprite[g.sel.wall].picnum;
    default: return g.selectedTile;
    }
}

static void setSelectedPic(short pic)
{
    if (!g.sel.valid() || pic < 0 || pic >= MAXTILES) return;
    switch (g.sel.stat)
    {
    case SS_WALL:
        wall[g.sel.wall].picnum = pic;
        fixWallRepeat(g.sel.wall);
        break;
    case SS_MASKED_WALL:
        wall[g.sel.wall].overpicnum = pic;
        if (wall[g.sel.wall].nextwall >= 0) wall[wall[g.sel.wall].nextwall].overpicnum = pic;
        break;
    case SS_CEILING:
        sector[g.sel.sector].ceilingpicnum = pic;
        break;
    case SS_FLOOR:
        sector[g.sel.sector].floorpicnum = pic;
        break;
    case SS_SPRITE:
        sprite[g.sel.wall].picnum = pic;
        break;
    }
    setDirty();
    updateInspector();
    refreshAllViews();
}

static bool selectedSectorValid()
{
    return g.sel.sector >= 0 && g.sel.sector < numsectors;
}

static bool selectedSurfaceValid()
{
    if (!g.sel.valid()) return false;
    switch (g.sel.stat)
    {
    case SS_WALL:
    case SS_MASKED_WALL:
        return g.sel.wall >= 0 && g.sel.wall < numwalls;
    case SS_CEILING:
    case SS_FLOOR:
        return selectedSectorValid();
    case SS_SPRITE:
        return g.sel.wall >= 0 && g.sel.wall < MAXSPRITES &&
            sprite[g.sel.wall].statnum >= 0 && sprite[g.sel.wall].statnum < MAXSTATUS &&
            sprite[g.sel.wall].sectnum >= 0 && sprite[g.sel.wall].sectnum < MAXSECTORS;
    default:
        return false;
    }
}

static bool selectedSpriteValid()
{
    return g.sel.stat == SS_SPRITE &&
        g.sel.wall >= 0 && g.sel.wall < MAXSPRITES &&
        sprite[g.sel.wall].statnum >= 0 && sprite[g.sel.wall].statnum < MAXSTATUS &&
        sprite[g.sel.wall].sectnum >= 0 && sprite[g.sel.wall].sectnum < MAXSECTORS;
}


static bool selectedBuildObjectValid()
{
    return selectedSurfaceValid() || selectedSectorValid();
}

static int getSelectedTypeValue()
{
    if (!selectedBuildObjectValid()) return 0;
    switch (g.sel.stat)
    {
    case SS_WALL:
    case SS_MASKED_WALL: return wall[g.sel.wall].lotag;
    case SS_CEILING:
    case SS_FLOOR: return sector[g.sel.sector].lotag;
    case SS_SPRITE: return sprite[g.sel.wall].lotag;
    default: return selectedSectorValid() ? sector[g.sel.sector].lotag : 0;
    }
}

static int getSelectedHitagValue()
{
    if (!selectedBuildObjectValid()) return 0;
    switch (g.sel.stat)
    {
    case SS_WALL:
    case SS_MASKED_WALL: return wall[g.sel.wall].hitag;
    case SS_CEILING:
    case SS_FLOOR: return sector[g.sel.sector].hitag;
    case SS_SPRITE: return sprite[g.sel.wall].hitag;
    default: return selectedSectorValid() ? sector[g.sel.sector].hitag : 0;
    }
}

static int getSelectedExtraValue()
{
    if (!selectedBuildObjectValid()) return -1;
    switch (g.sel.stat)
    {
    case SS_WALL:
    case SS_MASKED_WALL: return wall[g.sel.wall].extra;
    case SS_CEILING:
    case SS_FLOOR: return sector[g.sel.sector].extra;
    case SS_SPRITE: return sprite[g.sel.wall].extra;
    default: return selectedSectorValid() ? sector[g.sel.sector].extra : -1;
    }
}

static int getSelectedCstatValue()
{
    if (!selectedSurfaceValid()) return 0;
    switch (g.sel.stat)
    {
    case SS_WALL:
    case SS_MASKED_WALL: return static_cast<unsigned short>(wall[g.sel.wall].cstat);
    case SS_CEILING: return static_cast<unsigned short>(sector[g.sel.sector].ceilingstat);
    case SS_FLOOR: return static_cast<unsigned short>(sector[g.sel.sector].floorstat);
    case SS_SPRITE: return static_cast<unsigned short>(sprite[g.sel.wall].cstat);
    default: return 0;
    }
}

static bool setSelectedTypeDirect(int value)
{
    if (!selectedBuildObjectValid()) return false;
    const short v = static_cast<short>(clamp_long(value, -32768, 32767));
    switch (g.sel.stat)
    {
    case SS_WALL:
    case SS_MASKED_WALL: if (wall[g.sel.wall].lotag == v) return false; wall[g.sel.wall].lotag = v; return true;
    case SS_CEILING:
    case SS_FLOOR: if (sector[g.sel.sector].lotag == v) return false; sector[g.sel.sector].lotag = v; return true;
    case SS_SPRITE: if (sprite[g.sel.wall].lotag == v) return false; sprite[g.sel.wall].lotag = v; return true;
    default: if (selectedSectorValid() && sector[g.sel.sector].lotag != v) { sector[g.sel.sector].lotag = v; return true; } return false;
    }
}

static bool setSelectedHitagDirect(int value)
{
    if (!selectedBuildObjectValid()) return false;
    const short v = static_cast<short>(clamp_long(value, -32768, 32767));
    switch (g.sel.stat)
    {
    case SS_WALL:
    case SS_MASKED_WALL: if (wall[g.sel.wall].hitag == v) return false; wall[g.sel.wall].hitag = v; return true;
    case SS_CEILING:
    case SS_FLOOR: if (sector[g.sel.sector].hitag == v) return false; sector[g.sel.sector].hitag = v; return true;
    case SS_SPRITE: if (sprite[g.sel.wall].hitag == v) return false; sprite[g.sel.wall].hitag = v; return true;
    default: if (selectedSectorValid() && sector[g.sel.sector].hitag != v) { sector[g.sel.sector].hitag = v; return true; } return false;
    }
}

static bool setSelectedExtraDirect(int value)
{
    if (!selectedBuildObjectValid()) return false;
    const short v = static_cast<short>(clamp_long(value, -32768, 32767));
    switch (g.sel.stat)
    {
    case SS_WALL:
    case SS_MASKED_WALL: if (wall[g.sel.wall].extra == v) return false; wall[g.sel.wall].extra = v; return true;
    case SS_CEILING:
    case SS_FLOOR: if (sector[g.sel.sector].extra == v) return false; sector[g.sel.sector].extra = v; return true;
    case SS_SPRITE: if (sprite[g.sel.wall].extra == v) return false; sprite[g.sel.wall].extra = v; return true;
    default: if (selectedSectorValid() && sector[g.sel.sector].extra != v) { sector[g.sel.sector].extra = v; return true; } return false;
    }
}

static bool setSelectedCstatDirect(int value)
{
    if (!selectedSurfaceValid()) return false;
    const short v = static_cast<short>(clamp_long(value, 0, 65535));
    switch (g.sel.stat)
    {
    case SS_WALL:
    case SS_MASKED_WALL: if (wall[g.sel.wall].cstat == v) return false; wall[g.sel.wall].cstat = v; return true;
    case SS_CEILING: if (sector[g.sel.sector].ceilingstat == v) return false; sector[g.sel.sector].ceilingstat = v; return true;
    case SS_FLOOR: if (sector[g.sel.sector].floorstat == v) return false; sector[g.sel.sector].floorstat = v; return true;
    case SS_SPRITE: if (sprite[g.sel.wall].cstat == v) return false; sprite[g.sel.wall].cstat = v; return true;
    default: return false;
    }
}

static bool setSelectedSpriteStatnumDirect(int value)
{
    if (!selectedSpriteValid()) return false;
    const short v = static_cast<short>(clamp_long(value, 0, MAXSTATUS - 1));
    if (sprite[g.sel.wall].statnum == v) return false;
    changespritestat(g.sel.wall, v);
    return true;
}

static bool setSelectedSpriteAngleDirect(int value)
{
    if (!selectedSpriteValid()) return false;
    const short v = static_cast<short>(value & 2047);
    if (sprite[g.sel.wall].ang == v) return false;
    sprite[g.sel.wall].ang = v;
    return true;
}

static std::string bloodCaptionForSelectedObject()
{
    if (!selectedBuildObjectValid()) return "No selected Blood object.";

    char buf[256];
    switch (g.sel.stat)
    {
    case SS_SPRITE:
        std::snprintf(buf, sizeof(buf), "Sprite %d | %s | stat %d | extra %d",
            g.sel.wall, bloodTypeLabel("Sprite", sprite[g.sel.wall].lotag, bloodSpriteTypeName(sprite[g.sel.wall].lotag)).c_str(),
            sprite[g.sel.wall].statnum, sprite[g.sel.wall].extra);
        break;
    case SS_WALL:
    case SS_MASKED_WALL:
        std::snprintf(buf, sizeof(buf), "Wall %d | %s | extra %d",
            g.sel.wall, bloodTypeLabel("Wall", wall[g.sel.wall].lotag, bloodWallTypeName(wall[g.sel.wall].lotag)).c_str(),
            wall[g.sel.wall].extra);
        break;
    case SS_CEILING:
    case SS_FLOOR:
    default:
        if (selectedSectorValid())
            std::snprintf(buf, sizeof(buf), "Sector %d | %s | extra %d",
                g.sel.sector, bloodTypeLabel("Sector", sector[g.sel.sector].lotag, bloodSectorTypeName(sector[g.sel.sector].lotag)).c_str(),
                sector[g.sel.sector].extra);
        else
            std::snprintf(buf, sizeof(buf), "No selected Blood object.");
        break;
    }
    return buf;
}


static const char* selectedSurfaceName()
{
    switch (g.sel.stat)
    {
    case SS_WALL: return "Wall";
    case SS_MASKED_WALL: return "Masked wall";
    case SS_CEILING: return "Ceiling";
    case SS_FLOOR: return "Floor/Sector";
    case SS_SPRITE: return "Sprite";
    default: return "No surface";
    }
}

static int getSelectedShadeValue()
{
    if (!selectedSurfaceValid()) return 0;
    switch (g.sel.stat)
    {
    case SS_WALL:
    case SS_MASKED_WALL: return wall[g.sel.wall].shade;
    case SS_CEILING: return sector[g.sel.sector].ceilingshade;
    case SS_FLOOR: return sector[g.sel.sector].floorshade;
    case SS_SPRITE: return sprite[g.sel.wall].shade;
    default: return 0;
    }
}

static int getSelectedPalValue()
{
    if (!selectedSurfaceValid()) return 0;
    switch (g.sel.stat)
    {
    case SS_WALL:
    case SS_MASKED_WALL: return static_cast<unsigned char>(wall[g.sel.wall].pal);
    case SS_CEILING: return static_cast<unsigned char>(sector[g.sel.sector].ceilingpal);
    case SS_FLOOR: return static_cast<unsigned char>(sector[g.sel.sector].floorpal);
    case SS_SPRITE: return static_cast<unsigned char>(sprite[g.sel.wall].pal);
    default: return 0;
    }
}

static bool setSelectedPicDirect(short pic)
{
    if (!selectedSurfaceValid() || pic < 0 || pic >= MAXTILES) return false;
    bool changed = false;
    switch (g.sel.stat)
    {
    case SS_WALL:
        changed = wall[g.sel.wall].picnum != pic;
        wall[g.sel.wall].picnum = pic;
        if (changed) fixWallRepeat(g.sel.wall);
        break;
    case SS_MASKED_WALL:
        changed = wall[g.sel.wall].overpicnum != pic;
        wall[g.sel.wall].overpicnum = pic;
        if (wall[g.sel.wall].nextwall >= 0)
        {
            changed = changed || wall[wall[g.sel.wall].nextwall].overpicnum != pic;
            wall[wall[g.sel.wall].nextwall].overpicnum = pic;
        }
        break;
    case SS_CEILING:
        changed = sector[g.sel.sector].ceilingpicnum != pic;
        sector[g.sel.sector].ceilingpicnum = pic;
        break;
    case SS_FLOOR:
        changed = sector[g.sel.sector].floorpicnum != pic;
        sector[g.sel.sector].floorpicnum = pic;
        break;
    case SS_SPRITE:
        changed = sprite[g.sel.wall].picnum != pic;
        sprite[g.sel.wall].picnum = pic;
        break;
    }
    return changed;
}

static bool setSelectedShadeDirect(int shade)
{
    if (!selectedSurfaceValid()) return false;
    const signed char v = static_cast<signed char>(clamp_int(shade, -128, 127));
    switch (g.sel.stat)
    {
    case SS_WALL:
    case SS_MASKED_WALL:
        if (wall[g.sel.wall].shade == v) return false;
        wall[g.sel.wall].shade = v;
        return true;
    case SS_CEILING:
        if (sector[g.sel.sector].ceilingshade == v) return false;
        sector[g.sel.sector].ceilingshade = v;
        return true;
    case SS_FLOOR:
        if (sector[g.sel.sector].floorshade == v) return false;
        sector[g.sel.sector].floorshade = v;
        return true;
    case SS_SPRITE:
        if (sprite[g.sel.wall].shade == v) return false;
        sprite[g.sel.wall].shade = v;
        return true;
    default:
        return false;
    }
}

static bool setSelectedPalDirect(int palnum)
{
    if (!selectedSurfaceValid()) return false;
    const char v = static_cast<char>(clamp_int(palnum, 0, 255));
    switch (g.sel.stat)
    {
    case SS_WALL:
    case SS_MASKED_WALL:
        if (wall[g.sel.wall].pal == v) return false;
        wall[g.sel.wall].pal = v;
        return true;
    case SS_CEILING:
        if (sector[g.sel.sector].ceilingpal == v) return false;
        sector[g.sel.sector].ceilingpal = v;
        return true;
    case SS_FLOOR:
        if (sector[g.sel.sector].floorpal == v) return false;
        sector[g.sel.sector].floorpal = v;
        return true;
    case SS_SPRITE:
        if (sprite[g.sel.wall].pal == v) return false;
        sprite[g.sel.wall].pal = v;
        return true;
    default:
        return false;
    }
}

static void copyProps()
{
    if (!g.sel.valid()) return;
    CopiedProps c;
    c.valid = true;
    c.stat = g.sel.stat;
    switch (g.sel.stat)
    {
    case SS_WALL:
    case SS_MASKED_WALL:
    {
        walltype& w = wall[g.sel.wall];
        c.picnum = (g.sel.stat == SS_MASKED_WALL) ? w.overpicnum : w.picnum;
        c.cstat = w.cstat;
        c.lotag = w.lotag;
        c.hitag = w.hitag;
        c.extra = w.extra;
        c.shade = w.shade;
        c.pal = w.pal;
        c.xrepeat = static_cast<unsigned char>(w.xrepeat);
        c.yrepeat = static_cast<unsigned char>(w.yrepeat);
        c.xpanning = w.xpanning;
        c.ypanning = w.ypanning;
        break;
    }
    case SS_CEILING:
    case SS_FLOOR:
    {
        sectortype& s = sector[g.sel.sector];
        const bool ceil = (g.sel.stat == SS_CEILING);
        c.picnum = ceil ? s.ceilingpicnum : s.floorpicnum;
        c.cstat = ceil ? s.ceilingstat : s.floorstat;
        c.lotag = s.lotag;
        c.hitag = s.hitag;
        c.extra = s.extra;
        c.shade = ceil ? s.ceilingshade : s.floorshade;
        c.pal = ceil ? s.ceilingpal : s.floorpal;
        c.visibility = s.visibility;
        c.xpanning = ceil ? s.ceilingxpanning : s.floorxpanning;
        c.ypanning = ceil ? s.ceilingypanning : s.floorypanning;
        break;
    }
    case SS_SPRITE:
    {
        spritetype& sp = sprite[g.sel.wall];
        c.picnum = sp.picnum;
        c.cstat = sp.cstat;
        c.lotag = sp.lotag;
        c.hitag = sp.hitag;
        c.extra = sp.extra;
        c.shade = sp.shade;
        c.pal = sp.pal;
        c.xrepeat = sp.xrepeat;
        c.yrepeat = sp.yrepeat;
        break;
    }
    }
    g.clip = c;
    g.selectedTile = c.picnum;
    centerTextureBarOnSelected();
    setStatus("Copied properties from selection.");
}

static void pasteProps()
{
    if (!g.clip.valid || !g.sel.valid()) return;
    const CopiedProps& c = g.clip;
    switch (g.sel.stat)
    {
    case SS_WALL:
    case SS_MASKED_WALL:
    {
        walltype& w = wall[g.sel.wall];
        if (g.sel.stat == SS_MASKED_WALL) w.overpicnum = c.picnum; else w.picnum = c.picnum;
        w.cstat = c.cstat;
        w.lotag = c.lotag;
        w.hitag = c.hitag;
        w.extra = c.extra;
        w.shade = c.shade;
        w.pal = c.pal;
        w.xrepeat = static_cast<char>(c.xrepeat);
        w.yrepeat = static_cast<char>(c.yrepeat);
        w.xpanning = c.xpanning;
        w.ypanning = c.ypanning;
        break;
    }
    case SS_CEILING:
    case SS_FLOOR:
    {
        sectortype& s = sector[g.sel.sector];
        if (g.sel.stat == SS_CEILING)
        {
            s.ceilingpicnum = c.picnum;
            s.ceilingstat = c.cstat;
            s.ceilingshade = c.shade;
            s.ceilingpal = c.pal;
            s.ceilingxpanning = c.xpanning;
            s.ceilingypanning = c.ypanning;
        }
        else
        {
            s.floorpicnum = c.picnum;
            s.floorstat = c.cstat;
            s.floorshade = c.shade;
            s.floorpal = c.pal;
            s.floorxpanning = c.xpanning;
            s.floorypanning = c.ypanning;
        }
        s.visibility = c.visibility;
        s.lotag = c.lotag;
        s.hitag = c.hitag;
        s.extra = c.extra;
        break;
    }
    case SS_SPRITE:
    {
        spritetype& sp = sprite[g.sel.wall];
        sp.picnum = c.picnum;
        sp.cstat = c.cstat;
        sp.lotag = c.lotag;
        sp.hitag = c.hitag;
        sp.extra = c.extra;
        sp.shade = c.shade;
        sp.pal = c.pal;
        sp.xrepeat = c.xrepeat;
        sp.yrepeat = c.yrepeat;
        break;
    }
    }
    setDirty();
    updateInspector();
    refreshAllViews();
    setStatus("Pasted properties to selection.");
}

static void applySelectedTexture()
{
    setSelectedPic(g.selectedTile);
    setStatus("Applied tile %d.", g.selectedTile);
}

static void shadeSelection(int delta)
{
    if (!g.sel.valid()) return;
    switch (g.sel.stat)
    {
    case SS_WALL:
    case SS_MASKED_WALL:
        wall[g.sel.wall].shade = static_cast<signed char>(clamp_int(wall[g.sel.wall].shade + delta, -128, 127));
        break;
    case SS_CEILING:
        sector[g.sel.sector].ceilingshade = static_cast<signed char>(clamp_int(sector[g.sel.sector].ceilingshade + delta, -128, 127));
        break;
    case SS_FLOOR:
        sector[g.sel.sector].floorshade = static_cast<signed char>(clamp_int(sector[g.sel.sector].floorshade + delta, -128, 127));
        break;
    case SS_SPRITE:
        sprite[g.sel.wall].shade = static_cast<signed char>(clamp_int(sprite[g.sel.wall].shade + delta, -128, 127));
        break;
    }
    setDirty(); updateInspector(); refreshAllViews();
}

static void zAdjustSelection(long dz)
{
    if (!g.sel.valid()) return;
    switch (g.sel.stat)
    {
    case SS_CEILING:
        sector[g.sel.sector].ceilingz += dz;
        break;
    case SS_FLOOR:
        sector[g.sel.sector].floorz += dz;
        break;
    case SS_SPRITE:
        sprite[g.sel.wall].z += dz;
        setsprite(g.sel.wall, sprite[g.sel.wall].x, sprite[g.sel.wall].y, sprite[g.sel.wall].z);
        g.sel.sector = sprite[g.sel.wall].sectnum;
        break;
    case SS_WALL:
    case SS_MASKED_WALL:
        sector[g.sel.sector].ceilingz += dz;
        sector[g.sel.sector].floorz += dz;
        break;
    }
    setDirty(); updateInspector(); refreshAllViews();
}

static void toggleBlocking()
{
    if (!g.sel.valid()) return;
    if (g.sel.stat == SS_SPRITE)
    {
        spritetype& sp = sprite[g.sel.wall];
        sp.cstat ^= 1;
        sp.cstat &= ~256;
        sp.cstat |= ((sp.cstat & 1) << 8);
    }
    else if (g.sel.wall >= 0)
    {
        walltype& w = wall[g.sel.wall];
        w.cstat ^= 1;
        w.cstat &= ~64;
        if (w.nextwall >= 0)
        {
            wall[w.nextwall].cstat &= ~(1 | 64);
            wall[w.nextwall].cstat |= (w.cstat & 1);
        }
    }
    setDirty(); refreshAllViews();
}

static void toggleOneWay()
{
    if (!g.sel.valid()) return;
    if (g.sel.stat == SS_SPRITE) sprite[g.sel.wall].cstat ^= 64;
    else if (g.sel.wall >= 0) wall[g.sel.wall].cstat ^= 32;
    setDirty(); refreshAllViews();
}

static void toggleMasked()
{
    if (!g.sel.valid() || g.sel.stat == SS_SPRITE || g.sel.wall < 0) return;
    walltype& w = wall[g.sel.wall];
    if (w.nextwall < 0) return;
    w.cstat ^= 16;
    if (w.cstat & 16)
    {
        w.cstat &= ~8;
        wall[w.nextwall].cstat |= 8 | 16;
        wall[w.nextwall].overpicnum = w.overpicnum;
    }
    else
    {
        w.cstat &= ~8;
        wall[w.nextwall].cstat &= ~(8 | 16);
    }
    w.cstat &= ~32;
    wall[w.nextwall].cstat &= ~32;
    setDirty(); refreshAllViews();
}

static void toggleTranslucent()
{
    if (!g.sel.valid()) return;
    if (g.sel.stat == SS_SPRITE)
    {
        spritetype& sp = sprite[g.sel.wall];
        if ((sp.cstat & 2) == 0) sp.cstat |= 2;
        else if ((sp.cstat & 512) == 0) sp.cstat |= 512;
        else sp.cstat &= ~(2 | 512);
    }
    else if (g.sel.stat == SS_MASKED_WALL || g.sel.stat == SS_WALL)
    {
        walltype& w = wall[g.sel.wall];
        if ((w.cstat & 128) == 0) w.cstat |= 128;
        else if ((w.cstat & 512) == 0) w.cstat |= 512;
        else w.cstat &= ~(128 | 512);
        if (w.nextwall >= 0)
        {
            wall[w.nextwall].cstat &= ~(128 | 512);
            wall[w.nextwall].cstat |= (w.cstat & (128 | 512));
        }
    }
    setDirty(); refreshAllViews();
}

static void toggleParallax()
{
    if (!g.sel.valid()) return;
    if (g.sel.stat == SS_CEILING || g.sel.stat == SS_WALL || g.sel.stat == SS_MASKED_WALL)
        sector[g.sel.sector].ceilingstat ^= 1;
    else if (g.sel.stat == SS_FLOOR)
        sector[g.sel.sector].floorstat ^= 1;
    setDirty(); refreshAllViews();
}

static void editProperties()
{
    if (!g.sel.valid()) return;
    if (g.sel.stat == SS_WALL || g.sel.stat == SS_MASKED_WALL)
    {
        walltype& w = wall[g.sel.wall];
        long v = (g.sel.stat == SS_MASKED_WALL) ? w.overpicnum : w.picnum;
        if (promptLong("Wall properties", "Picture tile:", v, 0, MAXTILES - 1)) { if (g.sel.stat == SS_MASKED_WALL) w.overpicnum = static_cast<short>(v); else w.picnum = static_cast<short>(v); }
        v = w.lotag; if (promptLong("Wall properties", "Lo-tag:", v, -32768, 65535)) w.lotag = static_cast<short>(v);
        v = w.hitag; if (promptLong("Wall properties", "Hi-tag:", v, -32768, 65535)) w.hitag = static_cast<short>(v);
        v = w.cstat; if (promptLong("Wall properties", "Cstat:", v, 0, 65535)) w.cstat = static_cast<short>(v);
        v = w.pal; if (promptLong("Wall properties", "Palette:", v, 0, 255)) w.pal = static_cast<char>(v);
    }
    else if (g.sel.stat == SS_CEILING || g.sel.stat == SS_FLOOR)
    {
        sectortype& s = sector[g.sel.sector];
        const bool ceil = g.sel.stat == SS_CEILING;
        long v = ceil ? s.ceilingpicnum : s.floorpicnum;
        if (promptLong("Sector properties", ceil ? "Ceiling tile:" : "Floor tile:", v, 0, MAXTILES - 1)) { if (ceil) s.ceilingpicnum = static_cast<short>(v); else s.floorpicnum = static_cast<short>(v); }
        v = s.lotag; if (promptLong("Sector properties", "Lo-tag:", v, -32768, 65535)) s.lotag = static_cast<short>(v);
        v = s.hitag; if (promptLong("Sector properties", "Hi-tag:", v, -32768, 65535)) s.hitag = static_cast<short>(v);
        v = ceil ? s.ceilingstat : s.floorstat; if (promptLong("Sector properties", ceil ? "Ceiling stat:" : "Floor stat:", v, 0, 65535)) { if (ceil) s.ceilingstat = static_cast<short>(v); else s.floorstat = static_cast<short>(v); }
        v = ceil ? s.ceilingpal : s.floorpal; if (promptLong("Sector properties", ceil ? "Ceiling palette:" : "Floor palette:", v, 0, 255)) { if (ceil) s.ceilingpal = static_cast<char>(v); else s.floorpal = static_cast<char>(v); }
    }
    else if (g.sel.stat == SS_SPRITE)
    {
        spritetype& sp = sprite[g.sel.wall];
        long v = sp.picnum; if (promptLong("Sprite properties", "Picture tile:", v, 0, MAXTILES - 1)) sp.picnum = static_cast<short>(v);
        v = sp.lotag; if (promptLong("Sprite properties", "Lo-tag:", v, -32768, 65535)) sp.lotag = static_cast<short>(v);
        v = sp.hitag; if (promptLong("Sprite properties", "Hi-tag:", v, -32768, 65535)) sp.hitag = static_cast<short>(v);
        v = sp.cstat; if (promptLong("Sprite properties", "Cstat:", v, 0, 65535)) sp.cstat = static_cast<short>(v);
        v = sp.pal; if (promptLong("Sprite properties", "Palette:", v, 0, 255)) sp.pal = static_cast<char>(v);
        v = sp.statnum; if (promptLong("Sprite properties", "Status list:", v, 0, MAXSTATUS - 1)) changespritestat(g.sel.wall, static_cast<short>(v));
    }
    setDirty(); updateInspector(); refreshAllViews();
}



static std::string bloodSpriteDefaultChoiceText(const BloodSpriteDefault& def)
{
    char buf[256];
    std::snprintf(buf, sizeof(buf), "Type %3d  %-24s  tile %4d  rep %3d,%3d  pal %d",
        def.type, bloodSpriteTypeName(def.type), def.picnum, def.xrepeat, def.yrepeat, def.pal);
    return buf;
}

static bool bloodChooseSpriteDefault(const BloodSpriteDefault*& outDef)
{
    outDef = nullptr;
    std::vector<std::string> items;
    items.reserve(BLOOD_COUNT_OF(kBloodSpriteDefaults));
    for (size_t i = 0; i < BLOOD_COUNT_OF(kBloodSpriteDefaults); ++i)
        items.push_back(bloodSpriteDefaultChoiceText(kBloodSpriteDefaults[i]));

    int selected = 0;
    const BloodSpriteDefault* byType = bloodSpriteDefaultByTypeOrPic(selectedSpriteValid() ? sprite[g.sel.wall].lotag : 0, g.selectedTile);
    if (byType)
    {
        for (size_t i = 0; i < BLOOD_COUNT_OF(kBloodSpriteDefaults); ++i)
            if (&kBloodSpriteDefaults[i] == byType) { selected = static_cast<int>(i); break; }
    }

    if (!chooseStringIndex("Blood sprite preset", "Choose a Blood sprite type/preset:", items, selected))
        return false;
    selected = clamp_int(selected, 0, static_cast<int>(BLOOD_COUNT_OF(kBloodSpriteDefaults)) - 1);
    outDef = &kBloodSpriteDefaults[selected];
    return outDef != nullptr;
}

static void bloodSetSpriteStat(short nSprite, short wantedStat)
{
    if (nSprite < 0 || nSprite >= MAXSPRITES) return;
    wantedStat = static_cast<short>(clamp_int(wantedStat, 0, MAXSTATUS - 1));
    if (sprite[nSprite].statnum != wantedStat)
        changespritestat(nSprite, wantedStat);
}

static short bloodDefaultStatForSpriteType(short type)
{
    if (type == 15) return 16;                      // Path marker.
    if (type >= 40 && type < 81) return 3;           // Weapons/ammo.
    if (type >= 100 && type < 149) return 3;         // Items.
    if (type >= 200 && type < 254) return 6;         // Dudes.
    if (type >= 400 && type < 433) return 4;         // Gib/explode/world objects.
    if (type >= 450 && type < 460) return 11;        // Hazards/traps.
    if (type == 710) return 12;                      // Ambient SFX.
    return 0;
}

static bool bloodApplySpriteDefault(short nSprite, const BloodSpriteDefault& def, bool forcePicnum)
{
    if (nSprite < 0 || nSprite >= MAXSPRITES) return false;
    spritetype& sp = sprite[nSprite];
    bool changed = false;

    if (sp.lotag != def.type) { sp.lotag = def.type; changed = true; }
    if (forcePicnum && def.picnum >= 0 && sp.picnum != def.picnum) { sp.picnum = def.picnum; changed = true; }
    if (def.xrepeat >= 0 && sp.xrepeat != def.xrepeat) { sp.xrepeat = static_cast<unsigned char>(clamp_int(def.xrepeat, 1, 255)); changed = true; }
    if (def.yrepeat >= 0 && sp.yrepeat != def.yrepeat) { sp.yrepeat = static_cast<unsigned char>(clamp_int(def.yrepeat, 1, 255)); changed = true; }
    if (def.pal >= 0 && sp.pal != def.pal) { sp.pal = static_cast<char>(clamp_int(def.pal, 0, 255)); changed = true; }

    if (def.cstat256 == 0 && (sp.cstat & 0x100)) { sp.cstat &= ~0x100; changed = true; }
    else if (def.cstat256 > 0 && !(sp.cstat & 0x100)) { sp.cstat |= 0x100; changed = true; }

    if (def.type == 18 || def.type == 19 || def.type == 709 || def.type == 710 || def.type == 711)
    {
        const short old = sp.cstat;
        sp.cstat |= static_cast<short>(0x8000);
        sp.cstat &= static_cast<short>(~0x101);
        if (sp.cstat != old) changed = true;
        if (def.type == 709 || def.type == 710 || def.type == 711)
        {
            if (sp.shade != -128) { sp.shade = -128; changed = true; }
        }
    }
    else if (def.type == 6 || def.type == 7 || def.type == 9 || def.type == 10 ||
        def.type == 11 || def.type == 12 || def.type == 13 || def.type == 14 ||
        def.type == 20 || def.type == 21 || def.type == 22 || def.type == 23)
    {
        const short old = sp.cstat;
        sp.cstat &= static_cast<short>(~0x01);
        if (sp.cstat != old) changed = true;
    }

    const short wantedStat = bloodDefaultStatForSpriteType(def.type);
    if (sp.statnum != wantedStat)
    {
        bloodSetSpriteStat(nSprite, wantedStat);
        changed = true;
    }

    return changed;
}

static bool bloodChooseAndApplyPresetToSelectedSprite()
{
    if (!isBloodMode())
    {
        setStatus("Blood sprite presets are only available in Blood mode.");
        return false;
    }
    if (!selectedSpriteValid())
    {
        setStatus("Select a sprite before choosing a Blood preset.");
        return false;
    }

    const BloodSpriteDefault* def = nullptr;
    if (!bloodChooseSpriteDefault(def) || !def) return false;
    const bool changed = bloodApplySpriteDefault(g.sel.wall, *def, true);
    g.selectedTile = sprite[g.sel.wall].picnum;
    centerTextureBarOnSelected();
    if (changed) setDirty();
    updateInspector();
    refreshAllViews();
    setStatus("Blood preset applied: type %d (%s).", def->type, bloodSpriteTypeName(def->type));
    return changed;
}

static bool bloodNormalizeSelectedObject()
{
    if (!isBloodMode()) return false;
    if (selectedSpriteValid())
    {
        spritetype& sp = sprite[g.sel.wall];
        const BloodSpriteDefault* def = bloodSpriteDefaultByTypeOrPic(sp.lotag, sp.picnum);
        if (!def)
        {
            setStatus("No Blood preset matched sprite type %d / tile %d.", sp.lotag, sp.picnum);
            return false;
        }
        const bool changed = bloodApplySpriteDefault(g.sel.wall, *def, def->picnum >= 0);
        if (changed) setDirty();
        updateInspector();
        refreshAllViews();
        setStatus("Blood sprite %d normalized as type %d (%s).", g.sel.wall, def->type, bloodSpriteTypeName(def->type));
        return changed;
    }
    setStatus("Blood normalization currently applies to sprites; X-sector/X-wall data is shown as tags.");
    return false;
}

static void bloodNormalizeMapForBlood(bool quiet)
{
    if (!isBloodMode()) return;
    int changedCount = 0;
    for (short i = 0; i < MAXSPRITES; ++i)
    {
        if (sprite[i].statnum < 0 || sprite[i].statnum >= MAXSTATUS || sprite[i].sectnum < 0 || sprite[i].sectnum >= MAXSECTORS)
            continue;
        const BloodSpriteDefault* def = bloodSpriteDefaultByTypeOrPic(sprite[i].lotag, sprite[i].picnum);
        if (!def) continue;
        if (bloodApplySpriteDefault(i, *def, def->picnum >= 0))
            ++changedCount;
    }
    if (!quiet)
        setStatus("Blood sprite normalization touched %d sprite(s).", changedCount);
    if (changedCount > 0)
    {
        setDirty();
        updateInspector();
        refreshAllViews();
    }
}

static void insertSpriteAt(long mx, long my)
{
    short sect = -1;
    for (short i = 0; i < numsectors; ++i)
    {
        if (inside(mx, my, i) == 1) { sect = i; break; }
    }
    if (sect < 0) return;
    short idx = insertsprite(sect, 0);
    if (idx < 0) return;
    spritetype& sp = sprite[idx];
    sp.x = mx;
    sp.y = my;
    sp.z = getflorzofslope(sect, mx, my);
    sp.cstat = 0;
    sp.picnum = g.selectedTile;
    sp.shade = 0;
    sp.pal = 0;
    sp.clipdist = 32;
    sp.xrepeat = 64;
    sp.yrepeat = 64;
    sp.xoffset = 0;
    sp.yoffset = 0;
    sp.ang = 1536;
    sp.owner = -1;
    sp.xvel = sp.yvel = sp.zvel = 0;
    sp.lotag = sp.hitag = 0;
    sp.extra = -1;
    if (isBloodMode())
    {
        const BloodSpriteDefault* def = nullptr;
        if (bloodChooseSpriteDefault(def) && def)
            bloodApplySpriteDefault(idx, *def, true);
        else
            sp.lotag = 0; // Decoration.
    }
    setsprite(idx, mx, my, sp.z);
    g.sel.stat = SS_SPRITE;
    g.sel.wall = idx;
    g.sel.sector = sect;
    setDirty();
    syncEngineSearchGlobals(g.sel);
    refreshAllViews();
    setStatus("Sprite %d inserted.", idx);
}

static void deleteSelection()
{
    if (!g.sel.valid()) return;
    if (g.sel.stat == SS_SPRITE)
    {
        deletesprite(g.sel.wall);
        g.sel.clear();
        syncEngineSearchGlobals(g.sel);
        setDirty(); refreshAllViews();
        return;
    }
    if ((GetKeyState(VK_CONTROL) & 0x8000) && g.sel.sector >= 0)
    {
        deleteSector(g.sel.sector);
        return;
    }
    if (g.sel.wall >= 0)
    {
        deletePoint(g.sel.wall);
    }
}

// -----------------------------------------------------------------------------
// Engine window hosting and 3D rendering.
// -----------------------------------------------------------------------------
static HWND findEngineWindow()
{
    HWND hwnd = FindWindowA("BuildGL1DropInWindow", nullptr);
    if (hwnd) return hwnd;
    // Class-name lookup may fail if the title changed; enumerate as a fallback.
    struct FindData { HWND found = nullptr; } data;
    EnumWindows([](HWND h, LPARAM lp) -> BOOL
        {
            char cls[128] = {};
            GetClassNameA(h, cls, sizeof(cls));
            if (std::strcmp(cls, "BuildGL1DropInWindow") == 0)
            {
                reinterpret_cast<FindData*>(lp)->found = h;
                return FALSE;
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&data));
    return data.found;
}

static long cameraForwardDepth(long wx, long wy)
{
    const long dx = wx - g.cam.x;
    const long dy = wy - g.cam.y;
    const long fx = sintable[(g.cam.ang + 512) & 2047];
    const long fy = sintable[g.cam.ang & 2047];
    return static_cast<long>((static_cast<std::int64_t>(dx) * fx + static_cast<std::int64_t>(dy) * fy) >> 14);
}

static bool projectWorldTo3DScreen(long wx, long wy, long wz, int viewW, int viewH, int& outX, int& outY, long& outDepth)
{
    const long dx = wx - g.cam.x;
    const long dy = wy - g.cam.y;
    const long fx = sintable[(g.cam.ang + 512) & 2047];
    const long fy = sintable[g.cam.ang & 2047];
    const long rx = -fy;
    const long ry = fx;

    const long forward = static_cast<long>((static_cast<std::int64_t>(dx) * fx + static_cast<std::int64_t>(dy) * fy) >> 14);
    if (forward <= 64) return false;

    const long side = static_cast<long>((static_cast<std::int64_t>(dx) * rx + static_cast<std::int64_t>(dy) * ry) >> 14);
    const long cx = viewW >> 1;
    outX = static_cast<int>(cx + scale_local(side, cx, forward));

    // pick3D builds a ray with dz = (screenY * 200 / h - horiz) * 2000.
    // This inverse projects a BUILD z coordinate back into that same screen space.
    const double rayDz = (static_cast<double>(wz - g.cam.z) * 16384.0) / static_cast<double>(forward);
    const double screenY = (static_cast<double>(g.cam.horiz) + (rayDz / 2000.0)) * static_cast<double>(viewH) / 200.0;
    outY = static_cast<int>(std::llround(screenY));
    outDepth = forward;
    return true;
}

static bool activeVisibleSprite(short i)
{
    if (i < 0 || i >= MAXSPRITES) return false;
    const spritetype& sp = sprite[i];
    if (sp.statnum < 0 || sp.statnum >= MAXSTATUS || sp.sectnum < 0 || sp.sectnum >= MAXSECTORS) return false;
    if (sp.picnum < 0 || sp.picnum >= MAXTILES) return false;
    if (tilesizx[sp.picnum] <= 0 || tilesizy[sp.picnum] <= 0) return false;
    if (sp.cstat & 32768) return false; // Invisible sprite flag.
    return true;
}

static bool spriteScreenBounds(short i, int viewW, int viewH, RECT& outBox, int& outCenterX, int& outCenterY, long& outDepth)
{
    if (!activeVisibleSprite(i)) return false;

    const spritetype& sp = sprite[i];
    const int tileW = max(1, static_cast<int>(tilesizx[sp.picnum]));
    const int tileH = max(1, static_cast<int>(tilesizy[sp.picnum]));
    const int xrep = max(1, static_cast<int>(static_cast<unsigned char>(sp.xrepeat)));
    const int yrep = max(1, static_cast<int>(static_cast<unsigned char>(sp.yrepeat)));

    // BUILD face sprites usually store z at the bottom unless cstat bit 128 centers them.
    // This only needs an editor pick rectangle, so use a deliberately forgiving box.
    const long zHeight = static_cast<long>(tileH) * static_cast<long>(yrep) * 4L;
    const bool centered = (sp.cstat & 128) != 0;
    const long topZ = centered ? sp.z - (zHeight >> 1) : sp.z - zHeight;
    const long bottomZ = centered ? sp.z + (zHeight >> 1) : sp.z;
    const long midZ = topZ + ((bottomZ - topZ) >> 1);

    if (!projectWorldTo3DScreen(sp.x, sp.y, midZ, viewW, viewH, outCenterX, outCenterY, outDepth))
        return false;

    int topX = outCenterX, topY = outCenterY;
    int botX = outCenterX, botY = outCenterY;
    long dummyDepth = outDepth;
    projectWorldTo3DScreen(sp.x, sp.y, topZ, viewW, viewH, topX, topY, dummyDepth);
    projectWorldTo3DScreen(sp.x, sp.y, bottomZ, viewW, viewH, botX, botY, dummyDepth);

    int y0 = min(topY, botY);
    int y1 = max(topY, botY);

    const int scaledW = static_cast<int>((static_cast<double>(tileW) * static_cast<double>(xrep) * static_cast<double>(viewW)) /
        (max(1.0, static_cast<double>(outDepth)) * 8.0));
    const int fallbackH = static_cast<int>((static_cast<double>(tileH) * static_cast<double>(yrep) * static_cast<double>(viewH)) /
        (max(1.0, static_cast<double>(outDepth)) * 8.0));

    const int halfW = clamp_int(scaledW >> 1, 8, 160);
    const int halfH = clamp_int(fallbackH >> 1, 8, 180);
    if (std::abs(y1 - y0) < halfH * 2)
    {
        y0 = outCenterY - halfH;
        y1 = outCenterY + halfH;
    }

    // Wall/floor aligned sprites can have very flat projected height from this approximation.
    // Keep a small click target around their projected center so they remain selectable.
    const int pad = 6;
    outBox.left = outCenterX - halfW - pad;
    outBox.right = outCenterX + halfW + pad;
    outBox.top = y0 - pad;
    outBox.bottom = y1 + pad;

    const int offscreenPad = 256;
    if (outBox.right < -offscreenPad || outBox.left > viewW + offscreenPad ||
        outBox.bottom < -offscreenPad || outBox.top > viewH + offscreenPad)
        return false;

    return true;
}

static double distanceToRect2(int x, int y, const RECT& r)
{
    const int cx = clamp_int(x, r.left, r.right);
    const int cy = clamp_int(y, r.top, r.bottom);
    return dist2(x, y, cx, cy);
}

static short pick3DSpriteByScreen(int sx, int sy, int viewW, int viewH, long maxDepth)
{
    short bestSprite = -1;
    long bestDepth = 0x7fffffffL;
    double bestCenterDistance = 1.0e30;

    for (short i = 0; i < MAXSPRITES; ++i)
    {
        RECT box{};
        int cx = 0, cy = 0;
        long depth = 0;
        if (!spriteScreenBounds(i, viewW, viewH, box, cx, cy, depth)) continue;

        // If the normal hitscan struck a wall/floor first, do not select sprites
        // that are clearly behind that hit point. A little tolerance helps with
        // large sprites whose visual center can be behind the click surface.
        if (maxDepth > 0 && depth > maxDepth + 1024) continue;

        const double edgeD2 = distanceToRect2(sx, sy, box);
        const double centerD2 = dist2(sx, sy, cx, cy);
        if (edgeD2 > 16.0 * 16.0 && centerD2 > 24.0 * 24.0) continue;

        // Prefer the nearest visible sprite; for equal depths, use the one the
        // user clicked closest to. This behaves better when sprites overlap.
        if (depth < bestDepth || (depth == bestDepth && centerD2 < bestCenterDistance))
        {
            bestDepth = depth;
            bestCenterDistance = centerD2;
            bestSprite = i;
        }
    }

    return bestSprite;
}

static Selection pick3D(int sx, int sy)
{
    Selection s;
    RECT rc{};
    GetClientRect(g.engine ? g.engine : g.host3d, &rc);
    const int w = max(1, rc.right - rc.left);
    const int h = max(1, rc.bottom - rc.top);

    long dax = 16384;
    long day = divscale14_local(sx - (w >> 1), (w >> 1));
    rotatepoint(0, 0, dax, day, g.cam.ang, &dax, &day);
    const long dz = (scale_local(sy, 200, h) - g.cam.horiz) * 2000;

    short hs = -1, hw = -1, hsp = -1;
    long hx = 0, hy = 0, hz = 0;

    // Most editor sprites are non-blocking, so a normal game hitscan can miss
    // them and return the wall/floor behind them. Temporarily make every active
    // sprite both blocking and hitscan-blocking, then restore cstat immediately.
    std::vector<std::pair<short, short> > forcedSpriteCstats;
    for (short i = 0; i < MAXSPRITES; ++i)
    {
        if (sprite[i].statnum < 0 || sprite[i].statnum >= MAXSTATUS ||
            sprite[i].sectnum < 0 || sprite[i].sectnum >= MAXSECTORS) continue;
        if ((sprite[i].cstat & 257) != 257)
        {
            forcedSpriteCstats.push_back(std::make_pair(i, sprite[i].cstat));
            sprite[i].cstat = static_cast<short>(sprite[i].cstat | 257);
        }
    }

    hitscan(g.cam.x, g.cam.y, g.cam.z, g.cam.sect, dax, day, dz, &hs, &hw, &hsp, &hx, &hy, &hz, CLIPMASK1);

    for (size_t i = 0; i < forcedSpriteCstats.size(); ++i)
        sprite[forcedSpriteCstats[i].first].cstat = forcedSpriteCstats[i].second;

    const long hitDepth = (hw >= 0 || hs >= 0 || hsp >= 0) ? cameraForwardDepth(hx, hy) : 0;

    if (hsp >= 0)
    {
        s.stat = SS_SPRITE;
        s.wall = hsp;
        s.sector = sprite[hsp].sectnum;
    }
    else
    {
        // Fallback editor hit-test: project all visible sprites into screen space
        // and test the mouse against those rectangles. This makes decorative and
        // non-blocking Duke sprites selectable in 3D mode.
        const short screenSprite = pick3DSpriteByScreen(sx, sy, w, h, hitDepth);
        if (screenSprite >= 0)
        {
            s.stat = SS_SPRITE;
            s.wall = screenSprite;
            s.sector = sprite[screenSprite].sectnum;
        }
    }

    if (s.valid()) return s;

    if (hw >= 0)
    {
        s.stat = (wall[hw].cstat & 16) ? SS_MASKED_WALL : SS_WALL;
        s.wall = hw;
        s.sector = ownerSectorOfWall(hw);
    }
    else if (hs >= 0)
    {
        long cz = getceilzofslope(hs, hx, hy);
        long fz = getflorzofslope(hs, hx, hy);
        s.stat = (std::labs(hz - cz) < std::labs(hz - fz)) ? SS_CEILING : SS_FLOOR;
        s.sector = hs;
        s.wall = sector[hs].wallptr;
    }

    return s;
}

static LRESULT CALLBACK EngineChildSubclassProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_LBUTTONDOWN:
    {
        SetFocus(hwnd);
        POINT p{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        g.sel = pick3D(p.x, p.y);
        syncEngineSearchGlobals(g.sel);
        if ((GetKeyState(VK_CONTROL) & 0x8000) && g.sel.valid()) applySelectedTexture();
        else if (g.sel.valid())
        {
            g.selectedTile = getSelectedPic();
            centerTextureBarOnSelected();
            setStatus("3D selection: stat %d sector %d wall/sprite %d", g.sel.stat, g.sel.sector, g.sel.wall);
        }
        refreshAllViews();
        return 0;
    }
    case WM_RBUTTONDOWN:
        g.orbit3D = true;
        g.lastMouse = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        SetCapture(hwnd);
        return 0;
    case WM_RBUTTONUP:
        g.orbit3D = false;
        ReleaseCapture();
        return 0;
    case WM_MOUSEMOVE:
        if (g.orbit3D)
        {
            POINT p{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
            const int dx = p.x - g.lastMouse.x;
            const int dy = p.y - g.lastMouse.y;
            g.cam.ang = static_cast<short>((g.cam.ang + dx * 4) & 2047);
            g.cam.horiz = clamp_long(g.cam.horiz + dy, -100, 300);
            g.lastMouse = p;
            return 0;
        }
        break;
    case WM_MOUSEWHEEL:
        g.cam.z -= (GET_WHEEL_DELTA_WPARAM(wp) / WHEEL_DELTA) * (4 << 8);
        return 0;
    }
    return CallWindowProcA(g.oldEngineProc, hwnd, msg, wp, lp);
}

static void attachEngineWindow()
{
    if (!g.host3d) return;

    if (!g.engineCreated)
    {
        RECT rc{};
        GetClientRect(g.host3d, &rc);
        const int w = max(320, rc.right - rc.left);
        const int h = max(200, rc.bottom - rc.top);
        setgamemode(1, w, h);
        g.engine = findEngineWindow();
        g.engineCreated = true;
    }

    if (!g.engine) g.engine = findEngineWindow();
    if (!g.engine) return;

    SetParent(g.engine, g.host3d);
    LONG_PTR style = GetWindowLongPtrA(g.engine, GWL_STYLE);
    style &= ~(WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
    style |= WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    SetWindowLongPtrA(g.engine, GWL_STYLE, style);

    if (!g.oldEngineProc)
        g.oldEngineProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(g.engine, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EngineChildSubclassProc)));

    RECT rc{};
    GetClientRect(g.host3d, &rc);
    SetWindowPos(g.engine, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_SHOWWINDOW | SWP_FRAMECHANGED);
}

static bool has3DInputFocus()
{
    if (!g.main) return false;

    HWND fg = GetForegroundWindow();
    if (!fg) return false;
    HWND root = GetAncestor(fg, GA_ROOT);
    if (fg != g.main && root != g.main) return false;

    HWND focus = GetFocus();
    if (!focus) return true;
    if (focus == g.main || focus == g.host3d || focus == g.engine) return true;
    return g.host3d && IsChild(g.host3d, focus);
}

static void update3DCamera(double dt)
{
    if (g.cam.sect < 0) updatesector(g.cam.x, g.cam.y, &g.cam.sect);
    if (!has3DInputFocus()) return;

    const bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    const long move = static_cast<long>((shift ? 12000.0 : 6000.0) * dt);
    const long turn = static_cast<long>(700.0 * dt);
    long dx = 0, dy = 0;

    if (GetAsyncKeyState(VK_LEFT) & 0x8000) g.cam.ang = static_cast<short>((g.cam.ang - turn) & 2047);
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000) g.cam.ang = static_cast<short>((g.cam.ang + turn) & 2047);
    if (GetAsyncKeyState(VK_UP) & 0x8000) g.cam.horiz = clamp_long(g.cam.horiz - static_cast<long>(200.0 * dt), -100, 300);
    if (GetAsyncKeyState(VK_DOWN) & 0x8000) g.cam.horiz = clamp_long(g.cam.horiz + static_cast<long>(200.0 * dt), -100, 300);

    auto addMove = [&](int angOffset, long amount)
        {
            dx += i64mulscale(sintable[(g.cam.ang + angOffset + 512) & 2047], amount, 14);
            dy += i64mulscale(sintable[(g.cam.ang + angOffset) & 2047], amount, 14);
        };

    if (GetAsyncKeyState('W') & 0x8000) addMove(0, move);
    if (GetAsyncKeyState('S') & 0x8000) addMove(1024, move);
    if (GetAsyncKeyState('A') & 0x8000) addMove(1536, move);
    if (GetAsyncKeyState('D') & 0x8000) addMove(512, move);

    if (dx || dy)
    {
        g.cam.x += dx;
        g.cam.y += dy;
        updatesector(g.cam.x, g.cam.y, &g.cam.sect);
    }
    // Vertical flight should feel closer to classic BUILD editor movement.
    // Space/C and PgUp/PgDn now climb/drop much faster than the first pass.
    const long fly = static_cast<long>((shift ? 65536.0 : 32768.0) * dt);
    if (GetAsyncKeyState(VK_SPACE) & 0x8000) g.cam.z -= fly;
    if (GetAsyncKeyState('C') & 0x8000) g.cam.z += fly;
    if (GetAsyncKeyState(VK_PRIOR) & 0x8000) g.cam.z -= fly;
    if (GetAsyncKeyState(VK_NEXT) & 0x8000) g.cam.z += fly;
}

static void render3D()
{
    if (g.activeTab != 1) return;
    attachEngineWindow();
    if (!g.engine) return;

    LARGE_INTEGER now{};
    QueryPerformanceCounter(&now);
    double dt = 1.0 / 60.0;
    if (g.lastTick.QuadPart != 0 && g.freq.QuadPart != 0)
        dt = static_cast<double>(now.QuadPart - g.lastTick.QuadPart) / static_cast<double>(g.freq.QuadPart);
    g.lastTick = now;
    dt = max(0.0, min(dt, 0.1));

    update3DCamera(dt);
    totalclock += static_cast<long>(dt * 120.0);
    drawrooms(g.cam.x, g.cam.y, g.cam.z, g.cam.ang, g.cam.horiz, g.cam.sect);
    drawmasks();
    nextpage();
}

// Called by Build.cpp's private GL window proc.
LRESULT CALLBACK Engine_KeyboardWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    (void)hWnd; (void)lParam;
    switch (msg)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (wParam < 256) g.keys[wParam] = true;
        return 0;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (wParam < 256) g.keys[wParam] = false;
        return 0;
    case WM_KILLFOCUS:
    case WM_ACTIVATEAPP:
        if (msg == WM_KILLFOCUS || wParam == FALSE) std::memset(g.keys, 0, sizeof(g.keys));
        return 0;
    default:
        return 0;
    }
}

// -----------------------------------------------------------------------------
// 2D view painting and messages.
//
// The 2D editor no longer renders its map primitives through GDI.  Everything
// in this view is rasterized into a persistent 32-bit BGRA software framebuffer,
// then WM_PAINT copies that buffer to the window with SetDIBitsToDevice.  This
// avoids per-line HPEN/HBRUSH churn, avoids erase/redraw flicker, and keeps GDI
// away from the OpenGL child window used by the 3D backend.
// -----------------------------------------------------------------------------
struct Soft2DBackBuffer
{
    int w = 0;
    int h = 0;
    std::vector<std::uint32_t> pixels;
    BITMAPINFO bmi{};

    void resize(int nw, int nh)
    {
        nw = max(1, nw);
        nh = max(1, nh);
        if (w == nw && h == nh && !pixels.empty()) return;

        w = nw;
        h = nh;
        pixels.assign(static_cast<size_t>(w) * h, 0xff000000u);

        std::memset(&bmi, 0, sizeof(bmi));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = w;
        bmi.bmiHeader.biHeight = -h; // top-down DIB
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
    }
};

static Soft2DBackBuffer g_2dBuffer;

static std::uint32_t makeColor(std::uint8_t r, std::uint8_t g_, std::uint8_t b)
{
    return 0xff000000u | (static_cast<std::uint32_t>(r) << 16) |
        (static_cast<std::uint32_t>(g_) << 8) | static_cast<std::uint32_t>(b);
}

static std::uint32_t palColor32(int idx)
{
    idx &= 255;
    return makeColor(static_cast<std::uint8_t>(static_cast<unsigned char>(palette[idx * 3 + 0]) << 2),
        static_cast<std::uint8_t>(static_cast<unsigned char>(palette[idx * 3 + 1]) << 2),
        static_cast<std::uint8_t>(static_cast<unsigned char>(palette[idx * 3 + 2]) << 2));
}

static void clear2D(std::uint32_t color)
{
    std::fill(g_2dBuffer.pixels.begin(), g_2dBuffer.pixels.end(), color);
}

static void putPixel2D(int x, int y, std::uint32_t color)
{
    if (static_cast<unsigned>(x) >= static_cast<unsigned>(g_2dBuffer.w) ||
        static_cast<unsigned>(y) >= static_cast<unsigned>(g_2dBuffer.h))
        return;
    g_2dBuffer.pixels[static_cast<size_t>(y) * g_2dBuffer.w + x] = color;
}

static void fillRect2D(int x0, int y0, int x1, int y1, std::uint32_t color)
{
    if (x0 > x1) std::swap(x0, x1);
    if (y0 > y1) std::swap(y0, y1);

    x0 = clamp_int(x0, 0, g_2dBuffer.w);
    x1 = clamp_int(x1, 0, g_2dBuffer.w);
    y0 = clamp_int(y0, 0, g_2dBuffer.h);
    y1 = clamp_int(y1, 0, g_2dBuffer.h);
    if (x0 >= x1 || y0 >= y1) return;

    for (int y = y0; y < y1; ++y)
    {
        std::uint32_t* dst = &g_2dBuffer.pixels[static_cast<size_t>(y) * g_2dBuffer.w + x0];
        std::fill(dst, dst + (x1 - x0), color);
    }
}

static void drawLineThin2D(int x0, int y0, int x1, int y1, std::uint32_t color)
{
    int dx = std::abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    for (;;)
    {
        putPixel2D(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        const int e2 = err << 1;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void drawLine2D(int x0, int y0, int x1, int y1, std::uint32_t color, int width = 1)
{
    width = max(1, width);
    if (width <= 1)
    {
        drawLineThin2D(x0, y0, x1, y1, color);
        return;
    }

    const int r = width / 2;
    const int dx = std::abs(x1 - x0);
    const int dy = std::abs(y1 - y0);

    if (dx >= dy)
    {
        for (int o = -r; o <= r; ++o)
            drawLineThin2D(x0, y0 + o, x1, y1 + o, color);
    }
    else
    {
        for (int o = -r; o <= r; ++o)
            drawLineThin2D(x0 + o, y0, x1 + o, y1, color);
    }
}

static void drawRectOutline2D(int x0, int y0, int x1, int y1, std::uint32_t color)
{
    drawLine2D(x0, y0, x1, y0, color);
    drawLine2D(x1, y0, x1, y1, color);
    drawLine2D(x1, y1, x0, y1, color);
    drawLine2D(x0, y1, x0, y0, color);
}

// Minimal editor HUD font. Lowercase input is promoted to uppercase.
static const std::uint8_t* font5x7Rows(char ch)
{
    static const std::uint8_t blank[7] = { 0,0,0,0,0,0,0 };
    static const std::uint8_t qmark[7] = { 0x0e,0x11,0x01,0x02,0x04,0x00,0x04 };

    switch (ch)
    {
    case '0': { static const std::uint8_t r[7] = { 0x0e,0x11,0x13,0x15,0x19,0x11,0x0e }; return r; }
    case '1': { static const std::uint8_t r[7] = { 0x04,0x0c,0x04,0x04,0x04,0x04,0x0e }; return r; }
    case '2': { static const std::uint8_t r[7] = { 0x0e,0x11,0x01,0x02,0x04,0x08,0x1f }; return r; }
    case '3': { static const std::uint8_t r[7] = { 0x1e,0x01,0x01,0x0e,0x01,0x01,0x1e }; return r; }
    case '4': { static const std::uint8_t r[7] = { 0x02,0x06,0x0a,0x12,0x1f,0x02,0x02 }; return r; }
    case '5': { static const std::uint8_t r[7] = { 0x1f,0x10,0x10,0x1e,0x01,0x01,0x1e }; return r; }
    case '6': { static const std::uint8_t r[7] = { 0x06,0x08,0x10,0x1e,0x11,0x11,0x0e }; return r; }
    case '7': { static const std::uint8_t r[7] = { 0x1f,0x01,0x02,0x04,0x08,0x08,0x08 }; return r; }
    case '8': { static const std::uint8_t r[7] = { 0x0e,0x11,0x11,0x0e,0x11,0x11,0x0e }; return r; }
    case '9': { static const std::uint8_t r[7] = { 0x0e,0x11,0x11,0x0f,0x01,0x02,0x0c }; return r; }

    case 'A': { static const std::uint8_t r[7] = { 0x0e,0x11,0x11,0x1f,0x11,0x11,0x11 }; return r; }
    case 'B': { static const std::uint8_t r[7] = { 0x1e,0x11,0x11,0x1e,0x11,0x11,0x1e }; return r; }
    case 'C': { static const std::uint8_t r[7] = { 0x0e,0x11,0x10,0x10,0x10,0x11,0x0e }; return r; }
    case 'D': { static const std::uint8_t r[7] = { 0x1e,0x11,0x11,0x11,0x11,0x11,0x1e }; return r; }
    case 'E': { static const std::uint8_t r[7] = { 0x1f,0x10,0x10,0x1e,0x10,0x10,0x1f }; return r; }
    case 'F': { static const std::uint8_t r[7] = { 0x1f,0x10,0x10,0x1e,0x10,0x10,0x10 }; return r; }
    case 'G': { static const std::uint8_t r[7] = { 0x0e,0x11,0x10,0x17,0x11,0x11,0x0e }; return r; }
    case 'H': { static const std::uint8_t r[7] = { 0x11,0x11,0x11,0x1f,0x11,0x11,0x11 }; return r; }
    case 'I': { static const std::uint8_t r[7] = { 0x0e,0x04,0x04,0x04,0x04,0x04,0x0e }; return r; }
    case 'J': { static const std::uint8_t r[7] = { 0x07,0x02,0x02,0x02,0x12,0x12,0x0c }; return r; }
    case 'K': { static const std::uint8_t r[7] = { 0x11,0x12,0x14,0x18,0x14,0x12,0x11 }; return r; }
    case 'L': { static const std::uint8_t r[7] = { 0x10,0x10,0x10,0x10,0x10,0x10,0x1f }; return r; }
    case 'M': { static const std::uint8_t r[7] = { 0x11,0x1b,0x15,0x15,0x11,0x11,0x11 }; return r; }
    case 'N': { static const std::uint8_t r[7] = { 0x11,0x19,0x15,0x13,0x11,0x11,0x11 }; return r; }
    case 'O': { static const std::uint8_t r[7] = { 0x0e,0x11,0x11,0x11,0x11,0x11,0x0e }; return r; }
    case 'P': { static const std::uint8_t r[7] = { 0x1e,0x11,0x11,0x1e,0x10,0x10,0x10 }; return r; }
    case 'Q': { static const std::uint8_t r[7] = { 0x0e,0x11,0x11,0x11,0x15,0x12,0x0d }; return r; }
    case 'R': { static const std::uint8_t r[7] = { 0x1e,0x11,0x11,0x1e,0x14,0x12,0x11 }; return r; }
    case 'S': { static const std::uint8_t r[7] = { 0x0f,0x10,0x10,0x0e,0x01,0x01,0x1e }; return r; }
    case 'T': { static const std::uint8_t r[7] = { 0x1f,0x04,0x04,0x04,0x04,0x04,0x04 }; return r; }
    case 'U': { static const std::uint8_t r[7] = { 0x11,0x11,0x11,0x11,0x11,0x11,0x0e }; return r; }
    case 'V': { static const std::uint8_t r[7] = { 0x11,0x11,0x11,0x11,0x11,0x0a,0x04 }; return r; }
    case 'W': { static const std::uint8_t r[7] = { 0x11,0x11,0x11,0x15,0x15,0x15,0x0a }; return r; }
    case 'X': { static const std::uint8_t r[7] = { 0x11,0x11,0x0a,0x04,0x0a,0x11,0x11 }; return r; }
    case 'Y': { static const std::uint8_t r[7] = { 0x11,0x11,0x0a,0x04,0x04,0x04,0x04 }; return r; }
    case 'Z': { static const std::uint8_t r[7] = { 0x1f,0x01,0x02,0x04,0x08,0x10,0x1f }; return r; }

    case ' ': return blank;
    case '-': { static const std::uint8_t r[7] = { 0x00,0x00,0x00,0x1f,0x00,0x00,0x00 }; return r; }
    case '+': { static const std::uint8_t r[7] = { 0x00,0x04,0x04,0x1f,0x04,0x04,0x00 }; return r; }
    case '.': { static const std::uint8_t r[7] = { 0x00,0x00,0x00,0x00,0x00,0x0c,0x0c }; return r; }
    case ',': { static const std::uint8_t r[7] = { 0x00,0x00,0x00,0x00,0x0c,0x04,0x08 }; return r; }
    case ':': { static const std::uint8_t r[7] = { 0x00,0x0c,0x0c,0x00,0x0c,0x0c,0x00 }; return r; }
    case '/': { static const std::uint8_t r[7] = { 0x01,0x01,0x02,0x04,0x08,0x10,0x10 }; return r; }
    case '\\': { static const std::uint8_t r[7] = { 0x10,0x10,0x08,0x04,0x02,0x01,0x01 }; return r; }
    case '*': { static const std::uint8_t r[7] = { 0x00,0x15,0x0e,0x1f,0x0e,0x15,0x00 }; return r; }
    case '_': { static const std::uint8_t r[7] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x1f }; return r; }
    case '%': { static const std::uint8_t r[7] = { 0x19,0x1a,0x02,0x04,0x08,0x0b,0x13 }; return r; }
    case '(': { static const std::uint8_t r[7] = { 0x02,0x04,0x08,0x08,0x08,0x04,0x02 }; return r; }
    case ')': { static const std::uint8_t r[7] = { 0x08,0x04,0x02,0x02,0x02,0x04,0x08 }; return r; }
    case '[': { static const std::uint8_t r[7] = { 0x0e,0x08,0x08,0x08,0x08,0x08,0x0e }; return r; }
    case ']': { static const std::uint8_t r[7] = { 0x0e,0x02,0x02,0x02,0x02,0x02,0x0e }; return r; }
    default: return qmark;
    }
}

static void drawChar2D(int x, int y, char ch, std::uint32_t color, int scale = 1)
{
    if (ch >= 'a' && ch <= 'z') ch = static_cast<char>(ch - 'a' + 'A');
    const std::uint8_t* rows = font5x7Rows(ch);
    scale = max(1, scale);

    for (int ry = 0; ry < 7; ++ry)
    {
        const std::uint8_t bits = rows[ry];
        for (int rx = 0; rx < 5; ++rx)
        {
            if (bits & (1 << (4 - rx)))
                fillRect2D(x + rx * scale, y + ry * scale, x + (rx + 1) * scale, y + (ry + 1) * scale, color);
        }
    }
}

static void drawText2D(int x, int y, const char* text, std::uint32_t color, int scale = 1)
{
    if (!text) return;
    const int step = 6 * max(1, scale);
    int cx = x;
    for (const char* p = text; *p; ++p)
    {
        if (*p == '\n')
        {
            cx = x;
            y += 9 * max(1, scale);
            continue;
        }
        drawChar2D(cx, y, *p, color, scale);
        cx += step;
        if (cx >= g_2dBuffer.w - step) break;
    }
}

static void draw2DGrid(HWND hwnd)
{
    if (g.grid <= 0) return;
    RECT rc{}; GetClientRect(hwnd, &rc);
    const int gs = gridSize();
    const double z = static_cast<double>(g.zoom) / 16384.0;
    if (z <= 0.00001) return;
    const long left = static_cast<long>(g.viewX - (rc.right * 0.5) / z) - gs * 2;
    const long top = static_cast<long>(g.viewY - (rc.bottom * 0.5) / z) - gs * 2;
    const long right = static_cast<long>(g.viewX + (rc.right * 0.5) / z) + gs * 2;
    const long bottom = static_cast<long>(g.viewY + (rc.bottom * 0.5) / z) + gs * 2;

    const std::uint32_t color = g.gridLock ? makeColor(32, 48, 32) : makeColor(32, 32, 48);
    long firstX = (left / gs) * gs;
    long firstY = (top / gs) * gs;
    for (long x = firstX; x <= right; x += gs)
    {
        int sx1, sy1, sx2, sy2;
        mapToScreen(hwnd, x, top, sx1, sy1);
        mapToScreen(hwnd, x, bottom, sx2, sy2);
        drawLine2D(sx1, sy1, sx2, sy2, color);
    }
    for (long y = firstY; y <= bottom; y += gs)
    {
        int sx1, sy1, sx2, sy2;
        mapToScreen(hwnd, left, y, sx1, sy1);
        mapToScreen(hwnd, right, y, sx2, sy2);
        drawLine2D(sx1, sy1, sx2, sy2, color);
    }
}

static void rasterize2DView(HWND hwnd)
{
    RECT rc{}; GetClientRect(hwnd, &rc);
    g_2dBuffer.resize(rc.right - rc.left, rc.bottom - rc.top);
    clear2D(makeColor(0, 0, 0));

    draw2DGrid(hwnd);

    // Draw sectors/walls.
    for (short i = 0; i < numwalls; ++i)
    {
        if (wall[i].point2 < 0 || wall[i].point2 >= numwalls) continue;
        if (wall[i].nextwall >= 0 && i > wall[i].nextwall) continue; // avoid duplicate red walls

        int x1, y1, x2, y2;
        mapToScreen(hwnd, wall[i].x, wall[i].y, x1, y1);
        mapToScreen(hwnd, wall[wall[i].point2].x, wall[wall[i].point2].y, x2, y2);

        std::uint32_t c = (wall[i].nextsector >= 0) ? makeColor(220, 40, 40) : makeColor(210, 210, 210);
        if (wall[i].cstat & 1) c = makeColor(255, 180, 40);
        const int width = (g.sel.valid() && g.sel.wall == i && (g.sel.stat == SS_WALL || g.sel.stat == SS_MASKED_WALL)) ? 3 : 1;
        drawLine2D(x1, y1, x2, y2, c, width);
    }

    // Highlight a selected 2D sector by outlining all of its walls.
    if (selectedSectorValid() && (g.sel.stat == SS_FLOOR || g.sel.stat == SS_CEILING))
    {
        const int start = sector[g.sel.sector].wallptr;
        const int end = start + sector[g.sel.sector].wallnum;
        for (int wi = start; wi < end && wi >= 0 && wi < numwalls; ++wi)
        {
            if (wall[wi].point2 < 0 || wall[wi].point2 >= numwalls) continue;
            int x1, y1, x2, y2;
            mapToScreen(hwnd, wall[wi].x, wall[wi].y, x1, y1);
            mapToScreen(hwnd, wall[wall[wi].point2].x, wall[wall[wi].point2].y, x2, y2);
            drawLine2D(x1, y1, x2, y2, makeColor(80, 255, 80), 3);
        }
    }

    // Draw vertices.
    for (short i = 0; i < numwalls; ++i)
    {
        int sx, sy;
        mapToScreen(hwnd, wall[i].x, wall[i].y, sx, sy);
        fillRect2D(sx - 2, sy - 2, sx + 3, sy + 3, makeColor(180, 180, 180));
    }

    // Draw sprites.
    for (short i = 0; i < MAXSPRITES; ++i)
    {
        if (sprite[i].statnum < 0 || sprite[i].statnum >= MAXSTATUS ||
            sprite[i].sectnum < 0 || sprite[i].sectnum >= MAXSECTORS) continue;
        int sx, sy;
        mapToScreen(hwnd, sprite[i].x, sprite[i].y, sx, sy);
        std::uint32_t c = (g.sel.stat == SS_SPRITE && g.sel.wall == i) ? makeColor(80, 255, 80) : makeColor(80, 180, 255);
        if (isBloodMode())
        {
            const int t = sprite[i].lotag;
            if (t >= 200 && t < 254) c = makeColor(255, 80, 80);
            else if (t >= 400 && t < 460) c = makeColor(230, 120, 255);
            else if (t >= 20 && t < 33) c = makeColor(255, 180, 64);
            else if (t >= 6 && t <= 19) c = makeColor(120, 255, 180);
        }
        drawLine2D(sx - 5, sy, sx + 5, sy, c, 2);
        drawLine2D(sx, sy - 5, sx, sy + 5, c, 2);
        const int ax = sx + ((sintable[(sprite[i].ang + 512) & 2047]) >> 12);
        const int ay = sy + ((sintable[sprite[i].ang & 2047]) >> 12);
        drawLine2D(sx, sy, ax, ay, c, 1);
        if (g.showTags && isBloodMode() && sprite[i].lotag != 0)
        {
            char sbuf[96];
            std::snprintf(sbuf, sizeof(sbuf), "%d:%s", sprite[i].lotag, bloodSpriteTypeName(sprite[i].lotag));
            drawText2D(sx + 7, sy - 4, sbuf, makeColor(210, 210, 120));
        }
    }

    auto drawArrow = [&](long x, long y, short a, std::uint32_t c)
        {
            int sx, sy;
            mapToScreen(hwnd, x, y, sx, sy);
            int dx = sintable[(a + 512) & 2047] >> 10;
            int dy = sintable[a & 2047] >> 10;
            drawLine2D(sx, sy, sx + dx, sy + dy, c, 2);
            drawLine2D(sx + dx, sy + dy, sx + dx - dy / 2, sy + dy + dx / 2, c, 1);
            drawLine2D(sx + dx, sy + dy, sx + dx + dy / 2, sy + dy - dx / 2, c, 1);
        };
    drawArrow(g.startx, g.starty, g.startang, makeColor(160, 120, 60));
    drawArrow(g.cam.x, g.cam.y, g.cam.ang, makeColor(255, 255, 255));

    if (g.showTags)
    {
        for (short s = 0; s < numsectors; ++s)
        {
            long ax = 0, ay = 0;
            const int start = sector[s].wallptr;
            const int end = start + sector[s].wallnum;
            if (start < 0 || end > numwalls || sector[s].wallnum <= 0) continue;
            for (int w = start; w < end; ++w) { ax += wall[w].x; ay += wall[w].y; }
            ax /= sector[s].wallnum;
            ay /= sector[s].wallnum;

            int sx, sy;
            mapToScreen(hwnd, ax, ay, sx, sy);
            char buf[160];
            if (isBloodMode())
            {
                std::snprintf(buf, sizeof(buf), "S%d %d:%s H%d E%d", s, sector[s].lotag,
                    bloodSectorTypeName(sector[s].lotag), sector[s].hitag, sector[s].extra);
            }
            else
            {
                std::snprintf(buf, sizeof(buf), "S%d L%d H%d", s, sector[s].lotag, sector[s].hitag);
            }
            drawText2D(sx - 20, sy - 8, buf, makeColor(180, 180, 80));
        }
    }

    if (g.drawingSector && !g.drawPts.empty())
    {
        for (size_t i = 0; i < g.drawPts.size(); ++i)
        {
            POINT a = g.drawPts[i];
            POINT b = (i + 1 < g.drawPts.size()) ? g.drawPts[i + 1] : g.lastMapMouse;
            int x1, y1, x2, y2;
            mapToScreen(hwnd, a.x, a.y, x1, y1);
            mapToScreen(hwnd, b.x, b.y, x2, y2);
            drawLine2D(x1, y1, x2, y2, makeColor(80, 255, 80), 2);
        }
    }

    char status[512];
    std::snprintf(status, sizeof(status), "2D POS %ld,%ld  ZOOM %ld  GRID %d%s  SECTORS %d WALLS %d  %s",
        g.lastMapMouse.x, g.lastMapMouse.y, g.zoom, g.grid,
        g.gridLock ? " LOCKED" : " FREE", numsectors, numwalls, g.status.c_str());

    // Small dark backing strip so the software-font HUD remains readable.
    fillRect2D(4, 4, min(g_2dBuffer.w - 4, 4 + static_cast<int>(std::strlen(status)) * 6 + 8), 18, makeColor(8, 8, 8));
    drawRectOutline2D(4, 4, min(g_2dBuffer.w - 4, 4 + static_cast<int>(std::strlen(status)) * 6 + 8), 18, makeColor(32, 32, 32));
    drawText2D(8, 8, status, makeColor(230, 230, 230));
}

static void draw2DView(HWND hwnd, HDC paintDC)
{
    rasterize2DView(hwnd);
    SetDIBitsToDevice(paintDC,
        0, 0,
        static_cast<DWORD>(g_2dBuffer.w),
        static_cast<DWORD>(g_2dBuffer.h),
        0, 0,
        0,
        static_cast<UINT>(g_2dBuffer.h),
        g_2dBuffer.pixels.data(),
        &g_2dBuffer.bmi,
        DIB_RGB_COLORS);
}

static void beginOrAddDrawPoint(HWND hwnd)
{
    POINT p = screenToMap(hwnd, g.lastMouse.x, g.lastMouse.y, true);
    if (!g.drawingSector)
    {
        g.drawingSector = true;
        g.drawPts.clear();
        g.drawPts.push_back(p);
        setStatus("Sector drawing started. Add points with Space/click; close with Enter.");
    }
    else
    {
        if (g.drawPts.size() >= 3)
        {
            POINT first = g.drawPts.front();
            const long closeMap = screenPixelsToMap(hwnd, 10);
            if (dist2(first.x, first.y, p.x, p.y) <= static_cast<double>(closeMap) * closeMap)
            {
                commitDrawnSector();
                return;
            }
        }
        if (g.drawPts.empty() || g.drawPts.back().x != p.x || g.drawPts.back().y != p.y)
            g.drawPts.push_back(p);
    }
    InvalidateRect(hwnd, nullptr, FALSE);
}

static bool insertPointAtScreen(HWND hwnd, int sx, int sy)
{
    double t = 0.0;
    short line = nearestLineAtScreen(hwnd, sx, sy, &t);
    if (line < 0)
    {
        setStatus("Right-click Add Point needs the cursor near a wall.");
        return false;
    }

    const walltype& a = wall[line];
    const walltype& b = wall[a.point2];
    long x = static_cast<long>(std::llround(a.x + (b.x - a.x) * t));
    long y = static_cast<long>(std::llround(a.y + (b.y - a.y) * t));
    snapPoint(x, y);
    if (!insertPointOnWall(line, x, y)) return false;
    setStatus("Point inserted on wall %d.", line);
    return true;
}

static void insertPointAtMouse(HWND hwnd)
{
    insertPointAtScreen(hwnd, g.lastMouse.x, g.lastMouse.y);
}

static short sectorAtMapPoint(long x, long y)
{
    short sect = g.cam.sect;
    updatesector(x, y, &sect);
    if (sect >= 0 && sect < numsectors) return sect;

    for (short i = 0; i < numsectors; ++i)
        if (inside(x, y, i) == 1) return i;

    return -1;
}

static void moveCameraToMapPoint(long x, long y)
{
    g.cam.x = x;
    g.cam.y = y;
    g.cam.sect = sectorAtMapPoint(x, y);
    g.viewX = x;
    g.viewY = y;
    setStatus("Camera moved to %ld,%ld%s", x, y, g.cam.sect >= 0 ? "." : " (outside sectors).");
    refreshAllViews();
}

static void placeStartAtMapPoint(long x, long y)
{
    g.startx = x;
    g.starty = y;
    g.startz = g.cam.z;
    g.startang = g.cam.ang;
    g.startsect = sectorAtMapPoint(x, y);
    setDirty();
    setStatus("Player start placed at %ld,%ld%s", x, y, g.startsect >= 0 ? "." : " (outside sectors).");
    refreshAllViews();
}

static void show2DContextMenu(HWND hwnd, int clientX, int clientY, long mapX, long mapY)
{
    HMENU menu = CreatePopupMenu();
    if (!menu) return;

    AppendMenuA(menu, MF_STRING, ids::ID_CTX_MOVE_CAMERA_HERE, "Move camera to here");
    AppendMenuA(menu, MF_STRING, ids::ID_CTX_PLACE_START_HERE, "Place start position here");
    AppendMenuA(menu, MF_STRING, ids::ID_CTX_ADD_POINT_HERE, "Add point here");
    if (nearestLineAtScreen(hwnd, clientX, clientY) < 0)
        EnableMenuItem(menu, ids::ID_CTX_ADD_POINT_HERE, MF_BYCOMMAND | MF_GRAYED);

    POINT screenPt{ clientX, clientY };
    ClientToScreen(hwnd, &screenPt);
    const int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, screenPt.x, screenPt.y, 0, hwnd, nullptr);
    DestroyMenu(menu);

    if (cmd == ids::ID_CTX_MOVE_CAMERA_HERE)
        moveCameraToMapPoint(mapX, mapY);
    else if (cmd == ids::ID_CTX_PLACE_START_HERE)
        placeStartAtMapPoint(mapX, mapY);
    else if (cmd == ids::ID_CTX_ADD_POINT_HERE)
        insertPointAtScreen(hwnd, clientX, clientY);
}

static LRESULT CALLBACK View2DProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_ERASEBKGND:
        return 1;
    case WM_SETFOCUS:
        return 0;
    case WM_TIMER:
        if (wp == ids::TIMER_2D_REFRESH)
        {
            KillTimer(hwnd, ids::TIMER_2D_REFRESH);
            g.pending2DInvalidate = false;
            g.last2DInvalidateTick = GetTickCount();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        draw2DView(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        POINT p{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        POINT mp = screenToMap(hwnd, p.x, p.y, false);
        g.lastMapMouse = mp;
        if (g.panning2D)
        {
            const int totalDx = p.x - g.rightClickStart.x;
            const int totalDy = p.y - g.rightClickStart.y;
            if (std::abs(totalDx) > 3 || std::abs(totalDy) > 3)
                g.panning2DMoved = true;

            if (g.panning2DMoved)
            {
                const double z = static_cast<double>(g.zoom) / 16384.0;
                if (z > 0.00001)
                {
                    g.viewX -= static_cast<long>((p.x - g.lastMouse.x) / z);
                    g.viewY -= static_cast<long>((p.y - g.lastMouse.y) / z);
                }
                request2DRefresh(hwnd);
            }
            g.lastMouse = p;
        }
        else if (g.draggingPoint)
        {
            POINT sp = screenToMap(hwnd, p.x, p.y, true);
            if (moveJoinedPoint(g.dragPointWall, sp.x, sp.y))
                request2DRefresh(hwnd);
        }
        else if (g.draggingSprite)
        {
            POINT sp = screenToMap(hwnd, p.x, p.y, true);
            spritetype& spr = sprite[g.dragSprite];
            if (spr.x != sp.x || spr.y != sp.y)
            {
                spr.x = sp.x;
                spr.y = sp.y;
                setsprite(g.dragSprite, spr.x, spr.y, spr.z);
                g.sel.sector = spr.sectnum;
                g.dragSpriteMoved = true;
                markDirtyDuringDrag();
                request2DRefresh(hwnd);
            }
        }
        else if (g.drawingSector)
        {
            POINT sp = screenToMap(hwnd, p.x, p.y, true);
            if (g.lastMapMouse.x != sp.x || g.lastMapMouse.y != sp.y)
            {
                g.lastMapMouse = sp;
                request2DRefresh(hwnd);
            }
        }
        else
        {
            request2DRefresh(hwnd);
        }

        if (!g.panning2D)
            g.lastMouse = p;
        return 0;
    }
    case WM_LBUTTONDOWN:
    {
        SetFocus(hwnd);
        SetCapture(hwnd);
        g.lastMouse = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        POINT mp = screenToMap(hwnd, g.lastMouse.x, g.lastMouse.y, false);
        g.lastMapMouse = mp;
        g.dragPointMoved = false;
        g.dragSpriteMoved = false;
        g.dragDirtySet = false;

        if (g.drawingSector)
        {
            beginOrAddDrawPoint(hwnd);
            return 0;
        }

        short spr = nearestSpriteAtScreen(hwnd, g.lastMouse.x, g.lastMouse.y);
        short pt = nearestPointAtScreen(hwnd, g.lastMouse.x, g.lastMouse.y);
        if (spr >= 0)
        {
            // Sprite picking intentionally wins over nearby wall/sector picking.
            // A Shift-click still lets mappers grab a vertex under a sprite.
            if ((GetKeyState(VK_SHIFT) & 0x8000) && pt >= 0)
            {
                g.draggingPoint = true;
                g.dragPointWall = pt;
                g.dragOldX = wall[pt].x;
                g.dragOldY = wall[pt].y;
                g.sel.stat = SS_WALL;
                g.sel.wall = pt;
                g.sel.sector = ownerSectorOfWall(pt);
            }
            else
            {
                g.draggingSprite = true;
                g.dragSprite = spr;
                g.sel.stat = SS_SPRITE;
                g.sel.wall = spr;
                g.sel.sector = sprite[spr].sectnum;
            }
        }
        else if (pt >= 0)
        {
            g.draggingPoint = true;
            g.dragPointWall = pt;
            g.dragOldX = wall[pt].x;
            g.dragOldY = wall[pt].y;
            g.sel.stat = SS_WALL;
            g.sel.wall = pt;
            g.sel.sector = ownerSectorOfWall(pt);
        }
        else
        {
            g.sel = pick2DAtScreen(hwnd, g.lastMouse.x, g.lastMouse.y, mp.x, mp.y);
            if ((GetKeyState(VK_CONTROL) & 0x8000) && g.sel.valid()) applySelectedTexture();
        }
        syncEngineSearchGlobals(g.sel);
        if (g.sel.valid())
        {
            g.selectedTile = getSelectedPic();
            centerTextureBarOnSelected();
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    case WM_LBUTTONUP:
    {
        const bool wasDraggingPoint = g.draggingPoint;
        const bool wasDraggingSprite = g.draggingSprite;
        g.draggingPoint = false;
        g.draggingSprite = false;
        ReleaseCapture();
        if (wasDraggingPoint) finalizePointDrag();
        if (wasDraggingSprite && g.dragSpriteMoved)
        {
            updateInspector();
            setStatus("Sprite %d moved.", g.dragSprite);
        }
        g.dragPointMoved = false;
        g.dragSpriteMoved = false;
        g.dragDirtySet = false;
        return 0;
    }
    case WM_CAPTURECHANGED:
        if (g.draggingPoint) finalizePointDrag();
        if (g.draggingSprite && g.dragSpriteMoved) updateInspector();
        g.panning2D = false;
        g.panning2DMoved = false;
        g.draggingPoint = false;
        g.draggingSprite = false;
        g.dragPointMoved = false;
        g.dragSpriteMoved = false;
        g.dragDirtySet = false;
        return 0;
    case WM_RBUTTONDOWN:
        SetFocus(hwnd);
        g.panning2D = true;
        g.panning2DMoved = false;
        g.lastMouse = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        g.rightClickStart = g.lastMouse;
        g.rightClickMap = screenToMap(hwnd, g.lastMouse.x, g.lastMouse.y, false);
        SetCapture(hwnd);
        return 0;
    case WM_RBUTTONUP:
    {
        POINT p{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
        POINT mp = screenToMap(hwnd, p.x, p.y, false);
        const bool showMenu = !g.panning2DMoved;
        g.panning2D = false;
        g.panning2DMoved = false;
        ReleaseCapture();
        if (showMenu) show2DContextMenu(hwnd, p.x, p.y, mp.x, mp.y);
        return 0;
    }
    case WM_MOUSEWHEEL:
    {
        int delta = GET_WHEEL_DELTA_WPARAM(wp);
        if (delta > 0) g.zoom = min(g.zoom + (g.zoom >> 3) + 1, 65536);
        else g.zoom = max(g.zoom - (g.zoom >> 3), 24);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    case WM_KEYDOWN:
        switch (wp)
        {
        case VK_SPACE: beginOrAddDrawPoint(hwnd); return 0;
        case VK_RETURN: if (g.drawingSector) commitDrawnSector(); else pasteProps(); return 0;
        case VK_ESCAPE: g.drawingSector = false; g.drawPts.clear(); InvalidateRect(hwnd, nullptr, FALSE); return 0;
        case VK_INSERT: insertPointAtMouse(hwnd); return 0;
        case VK_DELETE: deleteSelection(); return 0;
        case 'S': insertSpriteAt(g.lastMapMouse.x, g.lastMapMouse.y); return 0;
        case 'G': g.grid = (g.grid + 1) % 7; setStatus("Grid %d", g.grid); InvalidateRect(hwnd, nullptr, FALSE); return 0;
        case 'L': g.gridLock = !g.gridLock; setStatus("Grid lock %s", g.gridLock ? "ON" : "OFF"); InvalidateRect(hwnd, nullptr, FALSE); return 0;
        case 'V': applySelectedTexture(); return 0;
        case VK_TAB: copyProps(); return 0;
        case 'B': toggleBlocking(); return 0;
        case 'M': toggleMasked(); return 0;
        case 'T': if (GetKeyState(VK_CONTROL) & 0x8000) { g.showTags = !g.showTags; InvalidateRect(hwnd, nullptr, FALSE); }
                else toggleTranslucent(); return 0;
        case 'P': toggleParallax(); return 0;
        case '1': toggleOneWay(); return 0;
        case VK_OEM_MINUS: shadeSelection(+1); return 0;
        case VK_OEM_PLUS: shadeSelection(-1); return 0;
        case VK_PRIOR: zAdjustSelection(-1024); return 0;
        case VK_NEXT: zAdjustSelection(+1024); return 0;
        }
        break;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

// -----------------------------------------------------------------------------
// Texture bar.
// -----------------------------------------------------------------------------
static int currentTextureBarHeight()
{
    return g.textureBarExpanded ? TEXTURE_BAR_EXPANDED_HEIGHT : TEXTURE_BAR_DEFAULT_HEIGHT;
}

static RECT textureSizeButtonRect(HWND hwnd)
{
    RECT rc{};
    GetClientRect(hwnd, &rc);
    RECT button{};
    button.left = max(8, rc.right - 104);
    button.top = 4;
    button.right = max(button.left + 1, rc.right - 8);
    button.bottom = 24;
    return button;
}

static void toggleTextureBarSize()
{
    g.textureBarExpanded = !g.textureBarExpanded;
    setStatus("Texture browser %s.", g.textureBarExpanded ? "expanded" : "shrunk");
    if (g.main) SendMessageA(g.main, WM_SIZE, 0, 0);
    if (g.textureBar) InvalidateRect(g.textureBar, nullptr, TRUE);
}

static void drawTilePreview(HDC hdc, short tile, int x, int y, int w, int h)
{
    if (tile < 0 || tile >= MAXTILES || tilesizx[tile] <= 0 || tilesizy[tile] <= 0)
    {
        Rectangle(hdc, x, y, x + w, y + h);
        return;
    }
    if (!waloff[tile]) loadtile(tile);
    const unsigned char* src = reinterpret_cast<const unsigned char*>(static_cast<std::intptr_t>(waloff[tile]));
    if (!src) return;

    const int tw = tilesizx[tile];
    const int th = tilesizy[tile];
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    std::vector<std::uint32_t> pixels(static_cast<size_t>(w) * h, 0xff000000u);

    for (int py = 0; py < h; ++py)
    {
        int sy = py * th / max(1, h);
        for (int px = 0; px < w; ++px)
        {
            int sx = px * tw / max(1, w);
            unsigned char idx = src[sx * th + sy]; // ART is column-major.
            std::uint8_t r, gr, b;
            if (idx == 255)
            {
                const bool chk = (((px >> 3) ^ (py >> 3)) & 1) != 0;
                r = gr = b = chk ? 48 : 24;
            }
            else
            {
                r = static_cast<std::uint8_t>(static_cast<unsigned char>(palette[idx * 3 + 0]) << 2);
                gr = static_cast<std::uint8_t>(static_cast<unsigned char>(palette[idx * 3 + 1]) << 2);
                b = static_cast<std::uint8_t>(static_cast<unsigned char>(palette[idx * 3 + 2]) << 2);
            }
            pixels[static_cast<size_t>(py) * w + px] = 0xff000000u | (static_cast<std::uint32_t>(r) << 16) | (static_cast<std::uint32_t>(gr) << 8) | b;
        }
    }

    StretchDIBits(hdc, x, y, w, h, 0, 0, w, h, pixels.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
}

static void drawTextureBar(HWND hwnd, HDC hdc)
{
    RECT rc{}; GetClientRect(hwnd, &rc);
    HBRUSH bg = CreateSolidBrush(RGB(26, 26, 30));
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(235, 235, 240));
    char top[512];
    std::snprintf(top, sizeof(top), "Tile %d   %dx%d   %s", g.selectedTile,
        (g.selectedTile >= 0 && g.selectedTile < MAXTILES) ? tilesizx[g.selectedTile] : 0,
        (g.selectedTile >= 0 && g.selectedTile < MAXTILES) ? tilesizy[g.selectedTile] : 0,
        g.status.c_str());

    RECT clipText{ 8, 5, max(8, rc.right - 114), 24 };
    DrawTextA(hdc, top, -1, &clipText, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

    RECT sizeButton = textureSizeButtonRect(hwnd);
    HBRUSH buttonBg = CreateSolidBrush(RGB(54, 56, 64));
    FillRect(hdc, &sizeButton, buttonBg);
    DeleteObject(buttonBg);
    FrameRect(hdc, &sizeButton, reinterpret_cast<HBRUSH>(GetStockObject(GRAY_BRUSH)));
    SetTextColor(hdc, RGB(245, 245, 245));
    const char* sizeText = g.textureBarExpanded ? "Shrink" : "Expand";
    DrawTextA(hdc, sizeText, -1, &sizeButton, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    const int y0 = 28;
    const int rows = max(1, (rc.bottom - y0) / TILE_CELL);
    const int cols = max(1, rc.right / TILE_CELL);
    const int visible = rows * cols;
    g.tileScroll = clamp_int(g.tileScroll, 0, max(0, static_cast<int>(g.tileList.size()) - visible));

    HFONT oldFont = reinterpret_cast<HFONT>(SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT)));
    for (int i = 0; i < visible; ++i)
    {
        const int idx = g.tileScroll + i;
        if (idx < 0 || idx >= static_cast<int>(g.tileList.size())) break;
        const short tile = g.tileList[idx];
        const int col = i % cols;
        const int row = i / cols;
        const int x = col * TILE_CELL + 4;
        const int y = y0 + row * TILE_CELL + 2;
        RECT cell{ x - 2, y - 2, x + TILE_CELL - 6, y + TILE_CELL - 4 };
        HBRUSH cellBg = CreateSolidBrush(tile == g.selectedTile ? RGB(80, 80, 40) : RGB(42, 42, 48));
        FillRect(hdc, &cell, cellBg);
        DeleteObject(cellBg);
        drawTilePreview(hdc, tile, x + 4, y + 4, TILE_PREVIEW, TILE_PREVIEW);
        char label[32]; std::snprintf(label, sizeof(label), "%d", tile);
        SetTextColor(hdc, tile == g.selectedTile ? RGB(255, 255, 120) : RGB(210, 210, 210));
        TextOutA(hdc, x + 4, y + TILE_PREVIEW + 6, label, static_cast<int>(std::strlen(label)));
    }
    SelectObject(hdc, oldFont);
}

static LRESULT CALLBACK TextureBarProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        drawTextureBar(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN:
    {
        RECT rc{}; GetClientRect(hwnd, &rc);
        const int y0 = 28;
        const int x = GET_X_LPARAM(lp);
        const int y = GET_Y_LPARAM(lp);
        POINT pt{ x, y };
        RECT sizeButton = textureSizeButtonRect(hwnd);
        if (PtInRect(&sizeButton, pt))
        {
            toggleTextureBarSize();
            return 0;
        }
        if (y < y0) return 0;
        const int cols = max(1, rc.right / TILE_CELL);
        const int col = x / TILE_CELL;
        const int row = (y - y0) / TILE_CELL;
        const int idx = g.tileScroll + row * cols + col;
        if (idx >= 0 && idx < static_cast<int>(g.tileList.size()))
        {
            g.selectedTile = g.tileList[idx];

            const bool clickAppliesToSelection = selectedSurfaceValid() &&
                (g.sel.stat == SS_FLOOR || g.sel.stat == SS_CEILING || g.sel.stat == SS_SPRITE);
            if (clickAppliesToSelection)
            {
                const bool changed = setSelectedPicDirect(g.selectedTile);
                if (changed) setDirty();
                updateInspector();
                setStatus("Applied tile %d to %s.", g.selectedTile, selectedSurfaceName());
                refreshAllViews();
            }
            else
            {
                setStatus("Selected tile %d. Press V or Ctrl-click an object to apply.", g.selectedTile);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            updateTileArtPanel();
        }
        return 0;
    }
    case WM_RBUTTONDOWN:
        SendMessage(g.main, WM_COMMAND, ids::M_TEXTURE_GOTO, 0);
        return 0;
    case WM_MOUSEWHEEL:
    {
        int delta = GET_WHEEL_DELTA_WPARAM(wp);
        g.tileScroll += (delta < 0) ? 4 : -4;
        g.tileScroll = clamp_int(g.tileScroll, 0, max(0, static_cast<int>(g.tileList.size()) - 1));
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

// -----------------------------------------------------------------------------
// Tile art import/export panel.
// -----------------------------------------------------------------------------
static void setDefaultGuiFont(HWND h)
{
    if (h) SendMessageA(h, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
}

static HWND makeTileArtControl(HWND parent, const char* cls, const char* text, DWORD style, DWORD exStyle, int id)
{
    if (cls && std::strcmp(cls, "BUTTON") == 0) style |= BS_OWNERDRAW;
    HWND h = CreateWindowExA(exStyle, cls, text ? text : "", WS_CHILD | WS_VISIBLE | style,
        0, 0, 10, 10, parent, reinterpret_cast<HMENU>(id), g.inst, nullptr);
    setDefaultGuiFont(h);
    return h;
}

static std::wstring widenAnsi(const std::string& s)
{
    if (s.empty()) return std::wstring();
    int needed = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, nullptr, 0);
    if (needed <= 0) return std::wstring();
    std::wstring out(static_cast<size_t>(needed), L'\0');
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, &out[0], needed);
    if (!out.empty() && out[out.size() - 1] == L'\0') out.resize(out.size() - 1);
    return out;
}

static bool getPngEncoderClsid(CLSID& clsid)
{
    UINT num = 0, bytes = 0;
    if (Gdiplus::GetImageEncodersSize(&num, &bytes) != Gdiplus::Ok || bytes == 0) return false;
    std::vector<unsigned char> buffer(bytes);
    Gdiplus::ImageCodecInfo* info = reinterpret_cast<Gdiplus::ImageCodecInfo*>(buffer.data());
    if (Gdiplus::GetImageEncoders(num, bytes, info) != Gdiplus::Ok) return false;
    for (UINT i = 0; i < num; ++i)
    {
        if (std::wcscmp(info[i].MimeType, L"image/png") == 0)
        {
            clsid = info[i].Clsid;
            return true;
        }
    }
    return false;
}

static bool promptForDukeGrpPath()
{
    char file[MAX_PATH] = "";
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g.main;
    const char* filter = isBloodMode()
        ? "Blood data/art (BLOOD.RFF;TILES000.ART)\0BLOOD.RFF;blood.rff;TILES000.ART;tiles000.art\0Blood RFF archives (*.rff)\0*.rff\0ART files (*.art)\0*.art\0All files (*.*)\0*.*\0"
        : "Duke Nukem 3D GRP (DUKE3D.GRP)\0DUKE3D.GRP;duke3d.grp\0BUILD group files (*.grp)\0*.grp\0All files (*.*)\0*.*\0";
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = file;
    ofn.nMaxFile = sizeof(file);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = isBloodMode() ? "rff" : "grp";
    if (!GetOpenFileNameA(&ofn)) return false;

    g.dukeGrpPath = fullPathOf(file);
    g.grpEditPath = g.dukeGrpPath;
    if (g.grpExportPath.empty())
        g.grpExportPath = pathJoin(pathDirectory(g.grpEditPath), isBloodMode() && isRffArchivePath(g.grpEditPath.c_str()) ? "rff_export" : "grp_export");

    if (isBloodMode())
    {
        g.bloodDataPath = g.dukeGrpPath;
        if (isRffArchivePath(g.dukeGrpPath.c_str()))
        {
            g.bloodRffPath = g.dukeGrpPath;
            g.bloodRffPrepared = false;
            prepareBloodRffCache(g.bloodRffPath);
            if (!loadPaletteFromGrp(g.bloodRffPath.c_str()))
                MessageBoxA(g.main, "Could not read PALETTE.DAT from the selected BLOOD.RFF.", "Tile art", MB_ICONWARNING);
        }
        else if (!loadBloodPaletteFromFolder(pathDirectory(g.dukeGrpPath)))
        {
            MessageBoxA(g.main, "Could not find a readable PALETTE.DAT beside the selected Blood data/art file.", "Tile art", MB_ICONWARNING);
        }
        loadBloodArtSetFromCurrentData();
    }
    else if (!loadPaletteFromGrp(g.dukeGrpPath.c_str()))
    {
        MessageBoxA(g.main, "The selected GRP did not contain a readable PALETTE.DAT.", "Tile art", MB_ICONWARNING);
    }

    updateGrpEditPanel();
    updateTileArtPanel();
    refreshAllViews();
    return true;
}

static bool ensureDukeGrpPath()
{
    if (isBloodMode())
    {
        if (!g.dukeGrpPath.empty() && fileExistsA(g.dukeGrpPath.c_str()))
        {
            if (isRffArchivePath(g.dukeGrpPath.c_str()))
            {
                g.bloodRffPath = g.dukeGrpPath;
                g.bloodDataPath = g.dukeGrpPath;
                prepareBloodRffCache(g.bloodRffPath);
                reloadCurrentGamePalette();
                return true;
            }
            loadBloodPaletteFromFolder(pathDirectory(g.dukeGrpPath));
            return true;
        }
        if (fileExistsA("BLOOD.RFF"))
        {
            g.dukeGrpPath = fullPathOf("BLOOD.RFF");
            g.bloodDataPath = g.dukeGrpPath;
            g.bloodRffPath = g.dukeGrpPath;
            prepareBloodRffCache(g.bloodRffPath);
            reloadCurrentGamePalette();
            return true;
        }
        if (fileExistsA("blood.rff"))
        {
            g.dukeGrpPath = fullPathOf("blood.rff");
            g.bloodDataPath = g.dukeGrpPath;
            g.bloodRffPath = g.dukeGrpPath;
            prepareBloodRffCache(g.bloodRffPath);
            reloadCurrentGamePalette();
            return true;
        }
        if (fileExistsA("TILES000.ART"))
        {
            g.dukeGrpPath = fullPathOf("TILES000.ART");
            g.bloodRffPath.clear();
            g.bloodRffPrepared = false;
            loadBloodPaletteFromFolder(pathDirectory(g.dukeGrpPath));
            return true;
        }
        if (fileExistsA("tiles000.art"))
        {
            g.dukeGrpPath = fullPathOf("tiles000.art");
            g.bloodRffPath.clear();
            g.bloodRffPrepared = false;
            loadBloodPaletteFromFolder(pathDirectory(g.dukeGrpPath));
            return true;
        }
        return promptForDukeGrpPath();
    }
    if (!g.dukeGrpPath.empty() && fileExistsA(g.dukeGrpPath.c_str())) return true;
    if (!g.boardGrpPath.empty() && fileExistsA(g.boardGrpPath.c_str()))
    {
        g.dukeGrpPath = fullPathOf(g.boardGrpPath.c_str());
        loadPaletteFromGrp(g.dukeGrpPath.c_str());
        return true;
    }
    if (fileExistsA("DUKE3D.GRP"))
    {
        g.dukeGrpPath = fullPathOf("DUKE3D.GRP");
        loadPaletteFromGrp(g.dukeGrpPath.c_str());
        return true;
    }
    if (fileExistsA("duke3d.grp"))
    {
        g.dukeGrpPath = fullPathOf("duke3d.grp");
        loadPaletteFromGrp(g.dukeGrpPath.c_str());
        return true;
    }
    return promptForDukeGrpPath();
}

static std::string artSaveFolder()
{
    if (!ensureDukeGrpPath()) return std::string();
    return pathDirectory(g.dukeGrpPath);
}

static bool readTileArtTileNumber(short& tile)
{
    char buf[64] = {};
    if (!g.artTileEdit) { tile = g.selectedTile; return true; }
    GetWindowTextA(g.artTileEdit, buf, sizeof(buf));
    char* end = nullptr;
    long value = std::strtol(buf, &end, 0);
    while (end && (*end == ' ' || *end == '\t')) ++end;
    if (!buf[0] || !end || *end != 0)
    {
        MessageBoxA(g.main, "Tile number must be a valid number.", "Tile art", MB_ICONERROR);
        SetFocus(g.artTileEdit);
        return false;
    }
    value = clamp_long(value, 0, MAXTILES - 1);
    tile = static_cast<short>(value);
    return true;
}

static void updateTileArtPanel()
{
    if (!g.tileArt) return;

    if (g.artTileEdit)
    {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%d", g.selectedTile);
        SetWindowTextA(g.artTileEdit, buf);
    }

    if (g.artStatusLabel)
    {
        char text[512];
        const std::string grp = g.dukeGrpPath.empty()
            ? std::string(isBloodMode() ? "No Blood data/art selected" : "No DUKE3D.GRP selected")
            : pathBaseNameString(g.dukeGrpPath);
        const bool dirty = g.artDirty;
        std::snprintf(text, sizeof(text), "Tile %d   %dx%d   %s   Data: %s",
            g.selectedTile,
            (g.selectedTile >= 0 && g.selectedTile < MAXTILES) ? tilesizx[g.selectedTile] : 0,
            (g.selectedTile >= 0 && g.selectedTile < MAXTILES) ? tilesizy[g.selectedTile] : 0,
            dirty ? "unsaved ART changes" : "ART saved",
            grp.c_str());
        SetWindowTextA(g.artStatusLabel, text);
    }
    InvalidateRect(g.tileArt, nullptr, FALSE);
}

static unsigned char nearestDukePaletteIndex(std::uint8_t r, std::uint8_t gr, std::uint8_t b)
{
    int best = 0;
    long bestDist = 0x7fffffffL;
    // Index 255 is treated as transparent in Build art, so keep it for alpha.
    for (int i = 0; i < 255; ++i)
    {
        const int pr = static_cast<unsigned char>(palette[i * 3 + 0]) << 2;
        const int pg = static_cast<unsigned char>(palette[i * 3 + 1]) << 2;
        const int pb = static_cast<unsigned char>(palette[i * 3 + 2]) << 2;
        const long dr = static_cast<long>(r) - pr;
        const long dg = static_cast<long>(gr) - pg;
        const long db = static_cast<long>(b) - pb;
        const long d = dr * dr + dg * dg + db * db;
        if (d < bestDist)
        {
            bestDist = d;
            best = i;
            if (d == 0) break;
        }
    }
    return static_cast<unsigned char>(best);
}

static bool setTilePixelsFromIndexed(short tile, int w, int h, const std::vector<unsigned char>& pixels, bool dirty)
{
    if (tile < 0 || tile >= MAXTILES || w <= 0 || h <= 0 || w > 32767 || h > 32767) return false;
    if (pixels.size() != static_cast<size_t>(w) * static_cast<size_t>(h)) return false;

    TileOverride& ov = g.tileOverrides[tile];
    ov.pixels = pixels;
    ov.w = static_cast<short>(w);
    ov.h = static_cast<short>(h);
    ov.picanmValue = picanm[tile];
    ov.dirty = ov.dirty || dirty;

    tilesizx[tile] = ov.w;
    tilesizy[tile] = ov.h;
    picanm[tile] = ov.picanmValue;
    waloff[tile] = static_cast<long>(reinterpret_cast<std::intptr_t>(ov.pixels.data()));

    g.selectedTile = tile;
    refreshTileList();
    centerTextureBarOnSelected();
    if (dirty) setArtDirty(true);
    updateTileArtPanel();
    refreshAllViews();
    return true;
}

static bool importSelectedTileFromPng()
{
    if (!g.gdiplusToken)
    {
        MessageBoxA(g.main, "Windows GDI+ is not available, so PNG import cannot run.", "Import tile", MB_ICONERROR);
        return false;
    }
    short tile = g.selectedTile;
    if (!readTileArtTileNumber(tile)) return false;
    if (!ensureDukeGrpPath()) return false;
    reloadCurrentGamePalette();

    char file[MAX_PATH] = "";
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g.main;
    ofn.lpstrFilter = "PNG images (*.png)\0*.png\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = sizeof(file);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "png";
    if (!GetOpenFileNameA(&ofn)) return false;

    std::wstring wide = widenAnsi(file);
    Gdiplus::Bitmap bmp(wide.c_str());
    if (bmp.GetLastStatus() != Gdiplus::Ok || bmp.GetWidth() == 0 || bmp.GetHeight() == 0)
    {
        MessageBoxA(g.main, "Could not load that PNG image.", "Import tile", MB_ICONERROR);
        return false;
    }
    if (bmp.GetWidth() > 4096 || bmp.GetHeight() > 4096)
    {
        MessageBoxA(g.main, "That PNG is too large for this editor tile import path.", "Import tile", MB_ICONERROR);
        return false;
    }

    const int w = static_cast<int>(bmp.GetWidth());
    const int h = static_cast<int>(bmp.GetHeight());
    std::vector<unsigned char> indexed(static_cast<size_t>(w) * h, 255);
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            Gdiplus::Color c;
            bmp.GetPixel(x, y, &c);
            const unsigned char idx = (c.GetA() < 128) ? 255 : nearestDukePaletteIndex(c.GetR(), c.GetG(), c.GetB());
            indexed[static_cast<size_t>(x) * h + y] = idx; // ART column-major.
        }
    }

    if (!setTilePixelsFromIndexed(tile, w, h, indexed, true))
    {
        MessageBoxA(g.main, "Could not apply the PNG to that tile.", "Import tile", MB_ICONERROR);
        return false;
    }

    setStatus("Imported PNG into tile %d. Use File > Save or Save ART files to write TILES%03d.ART beside the current game data.", tile, tile / 256);
    return true;
}

static bool exportSelectedTileToPng()
{
    if (!g.gdiplusToken)
    {
        MessageBoxA(g.main, "Windows GDI+ is not available, so PNG export cannot run.", "Export tile", MB_ICONERROR);
        return false;
    }
    short tile = g.selectedTile;
    if (!readTileArtTileNumber(tile)) return false;
    g.selectedTile = tile;
    centerTextureBarOnSelected();
    if (tile < 0 || tile >= MAXTILES || tilesizx[tile] <= 0 || tilesizy[tile] <= 0)
    {
        MessageBoxA(g.main, "That tile is empty.", "Export tile", MB_ICONINFORMATION);
        return false;
    }
    if (!waloff[tile]) loadtile(tile);
    const unsigned char* src = reinterpret_cast<const unsigned char*>(static_cast<std::intptr_t>(waloff[tile]));
    if (!src)
    {
        MessageBoxA(g.main, "Could not load that tile's art data.", "Export tile", MB_ICONERROR);
        return false;
    }

    char defaultName[MAX_PATH] = {};
    std::snprintf(defaultName, sizeof(defaultName), "tile%04d.png", tile);
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g.main;
    ofn.lpstrFilter = "PNG images (*.png)\0*.png\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = defaultName;
    ofn.nMaxFile = sizeof(defaultName);
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "png";
    if (!GetSaveFileNameA(&ofn)) return false;

    const int w = tilesizx[tile];
    const int h = tilesizy[tile];
    Gdiplus::Bitmap out(w, h, PixelFormat32bppARGB);
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            const unsigned char idx = src[static_cast<size_t>(x) * h + y];
            if (idx == 255)
            {
                out.SetPixel(x, y, Gdiplus::Color(0, 0, 0, 0));
            }
            else
            {
                const BYTE r = static_cast<BYTE>(static_cast<unsigned char>(palette[idx * 3 + 0]) << 2);
                const BYTE gr = static_cast<BYTE>(static_cast<unsigned char>(palette[idx * 3 + 1]) << 2);
                const BYTE b = static_cast<BYTE>(static_cast<unsigned char>(palette[idx * 3 + 2]) << 2);
                out.SetPixel(x, y, Gdiplus::Color(255, r, gr, b));
            }
        }
    }

    CLSID pngClsid{};
    if (!getPngEncoderClsid(pngClsid))
    {
        MessageBoxA(g.main, "Could not find the Windows PNG encoder.", "Export tile", MB_ICONERROR);
        return false;
    }
    std::wstring wide = widenAnsi(defaultName);
    if (out.Save(wide.c_str(), &pngClsid, nullptr) != Gdiplus::Ok)
    {
        MessageBoxA(g.main, "Could not save the PNG file.", "Export tile", MB_ICONERROR);
        return false;
    }

    setStatus("Exported tile %d to PNG.", tile);
    return true;
}

static bool writeLe16(FILE* f, int value)
{
    unsigned char b[2] = {
        static_cast<unsigned char>(value & 255),
        static_cast<unsigned char>((value >> 8) & 255)
    };
    return std::fwrite(b, 1, 2, f) == 2;
}

static bool writeLe32(FILE* f, long value)
{
    const std::uint32_t v = static_cast<std::uint32_t>(value);
    unsigned char b[4] = {
        static_cast<unsigned char>(v & 255),
        static_cast<unsigned char>((v >> 8) & 255),
        static_cast<unsigned char>((v >> 16) & 255),
        static_cast<unsigned char>((v >> 24) & 255)
    };
    return std::fwrite(b, 1, 4, f) == 4;
}

// -----------------------------------------------------------------------------
// GRP editor panel helpers.
// -----------------------------------------------------------------------------
static bool directoryExistsA(const char* path)
{
    if (!path || !path[0]) return false;
    const DWORD attr = GetFileAttributesA(path);
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

static bool ensureDirectoryTreeA(const std::string& folder)
{
    if (folder.empty()) return false;
    if (directoryExistsA(folder.c_str())) return true;

    std::string path = folder;
    std::replace(path.begin(), path.end(), '/', '\\');
    while (!path.empty() && (path[path.size() - 1] == '\\' || path[path.size() - 1] == '/'))
        path.resize(path.size() - 1);
    if (path.empty()) return false;

    size_t start = 0;
    if (path.size() >= 2 && path[1] == ':') start = 2;
    if (path.size() >= 2 && path[0] == '\\' && path[1] == '\\')
    {
        size_t server = path.find('\\', 2);
        if (server != std::string::npos)
        {
            size_t share = path.find('\\', server + 1);
            if (share != std::string::npos) start = share;
        }
    }

    for (size_t pos = path.find('\\', start + 1); pos != std::string::npos; pos = path.find('\\', pos + 1))
    {
        const std::string part = path.substr(0, pos);
        if (!part.empty() && !directoryExistsA(part.c_str()))
            CreateDirectoryA(part.c_str(), nullptr);
    }

    if (directoryExistsA(path.c_str())) return true;
    if (CreateDirectoryA(path.c_str(), nullptr)) return true;
    return GetLastError() == ERROR_ALREADY_EXISTS && directoryExistsA(path.c_str());
}

static bool readFileBytesA(const std::string& path, std::vector<unsigned char>& bytes)
{
    bytes.clear();
    HANDLE h = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER size{};
    if (!GetFileSizeEx(h, &size) || size.QuadPart < 0 || size.QuadPart > 0xffffffffll)
    {
        CloseHandle(h);
        return false;
    }

    bytes.resize(static_cast<size_t>(size.QuadPart));
    size_t offset = 0;
    while (offset < bytes.size())
    {
        const DWORD want = static_cast<DWORD>(std::min<size_t>(64 * 1024, bytes.size() - offset));
        DWORD got = 0;
        if (!ReadFile(h, bytes.data() + offset, want, &got, nullptr) || got == 0)
        {
            CloseHandle(h);
            bytes.clear();
            return false;
        }
        offset += got;
    }

    CloseHandle(h);
    return true;
}

static bool writeFileBytesA(const std::string& path, const std::vector<unsigned char>& bytes)
{
    FILE* f = nullptr;
    if (fopen_s(&f, path.c_str(), "wb") != 0 || !f) return false;
    const bool ok = bytes.empty() || std::fwrite(bytes.data(), 1, bytes.size(), f) == bytes.size();
    std::fclose(f);
    if (!ok) DeleteFileA(path.c_str());
    return ok;
}

static bool bloodRffEntryIsArt(const std::string& name)
{
    const std::string up = upperAscii(name);
    return up.size() >= 9 && (up.find("TILES") == 0 || up.find("SHARE") == 0) && up.substr(up.size() - 4) == ".ART";
}

static bool bloodRffEntryShouldExtractForArt(const std::string& name)
{
    const std::string up = upperAscii(name);
    return bloodRffEntryIsArt(up) ||
        up == "PALETTE.DAT" ||
        up == "PALOOKUP.DAT" ||
        up == "LOOKUP.DAT" ||
        up == "TRANSLUC.TBL";
}

static std::string bloodRffCacheFolder()
{
    char tempDir[MAX_PATH] = {};
    if (!GetTempPathA(sizeof(tempDir), tempDir)) return std::string();
    char folderName[64] = {};
    std::snprintf(folderName, sizeof(folderName), "BuildX_BloodRFF_%lu", static_cast<unsigned long>(GetCurrentProcessId()));
    return pathJoin(tempDir, folderName);
}

static bool prepareBloodRffCache(const std::string& rffPath)
{
    if (rffPath.empty() || !fileExistsA(rffPath.c_str()) || !isRffArchivePath(rffPath.c_str())) return false;

    const std::string full = fullPathOf(rffPath.c_str());
    const std::string existingArt = g.bloodExtractPath.empty() ? std::string() : pathJoin(g.bloodExtractPath, "TILES000.ART");
    if (g.bloodRffPrepared && g.bloodRffPath == full && !existingArt.empty() && fileExistsA(existingArt.c_str()))
        return true;

    std::vector<GrpMapEntry> entries;
    if (!readRffEntries(full.c_str(), entries)) return false;

    const std::string cache = bloodRffCacheFolder();
    if (cache.empty() || !ensureDirectoryTreeA(cache)) return false;

    int artCount = 0;
    int supportCount = 0;
    for (const GrpMapEntry& e : entries)
    {
        if (!bloodRffEntryShouldExtractForArt(e.name)) continue;

        std::vector<unsigned char> bytes;
        if (!readGrpEntryBytes(full.c_str(), e, bytes)) continue;

        const std::string outName = sanitizeFileName(upperAscii(e.name));
        const std::string outPath = pathJoin(cache, outName);
        if (!writeFileBytesA(outPath, bytes)) continue;

        if (bloodRffEntryIsArt(e.name)) ++artCount;
        else ++supportCount;
    }

    if (artCount <= 0) return false;

    g.bloodRffPath = full;
    g.bloodDataPath = full;
    g.dukeGrpPath = full;
    if (g.grpEditPath.empty()) g.grpEditPath = full;
    if (g.grpExportPath.empty()) g.grpExportPath = pathJoin(pathDirectory(full), "rff_export");
    g.bloodExtractPath = cache;
    g.bloodRffPrepared = true;

    loadPaletteFromGrp(full.c_str());
    setStatus("Mounted BLOOD.RFF: extracted %d ART file(s)%s to a temporary cache.", artCount, supportCount ? " plus support tables" : "");
    return true;
}

static std::string bloodArtPathForLoad()
{
    if (!g.bloodRffPath.empty() && prepareBloodRffCache(g.bloodRffPath))
        return pathJoin(g.bloodExtractPath, "TILES000.ART");

    if (!g.dukeGrpPath.empty() && fileExistsA(g.dukeGrpPath.c_str()))
    {
        if (isRffArchivePath(g.dukeGrpPath.c_str()) && prepareBloodRffCache(g.dukeGrpPath))
            return pathJoin(g.bloodExtractPath, "TILES000.ART");

        const std::string base = upperAscii(pathBaseNameString(g.dukeGrpPath));
        if (bloodRffEntryIsArt(base)) return g.dukeGrpPath;

        const std::string looseArt = pathJoin(pathDirectory(g.dukeGrpPath), "TILES000.ART");
        if (fileExistsA(looseArt.c_str())) return looseArt;
        const std::string looseArtLower = pathJoin(pathDirectory(g.dukeGrpPath), "tiles000.art");
        if (fileExistsA(looseArtLower.c_str())) return looseArtLower;
    }

    if (fileExistsA("TILES000.ART")) return "TILES000.ART";
    return "tiles000.art";
}


static int countLoadedBuildTiles()
{
    int count = 0;
    for (int i = 0; i < MAXTILES; ++i)
        if (tilesizx[i] > 0 && tilesizy[i] > 0)
            ++count;
    return count;
}

static bool artSetBaseExistsInFolder(const std::string& folder, const char* baseName)
{
    if (!baseName || !baseName[0]) return false;
    return fileExistsA(pathJoin(folder, baseName).c_str());
}

static std::string chooseArtBaseNameForFolder(const std::string& folder, const std::string& requestedBase)
{
    // BUILD's loadpics() rewrites the three digits in an 8.3 ART base name
    // while scanning a sequence such as TILES000.ART, TILES001.ART, etc.
    // Passing an absolute path can corrupt the path, so this helper always
    // changes into the ART folder and passes only the 8.3 base name.
    if (!requestedBase.empty() && requestedBase.size() >= 12 && artSetBaseExistsInFolder(folder, requestedBase.c_str()))
        return requestedBase;

    static const char* kBaseNames[] = {
        "tiles000.art", "TILES000.ART",
        "share000.art", "SHARE000.ART",
    };
    for (size_t i = 0; i < BLOOD_COUNT_OF(kBaseNames); ++i)
        if (artSetBaseExistsInFolder(folder, kBaseNames[i]))
            return kBaseNames[i];

    return "tiles000.art";
}

static bool loadArtSetFromPathOrFolder(const std::string& artPathOrFolder, const char* statusLabel)
{
    std::string folder;
    std::string requestedBase;

    if (!artPathOrFolder.empty() && directoryExistsA(artPathOrFolder.c_str()))
    {
        folder = artPathOrFolder;
    }
    else if (!artPathOrFolder.empty())
    {
        folder = pathDirectory(artPathOrFolder);
        requestedBase = pathBaseNameString(artPathOrFolder);
    }
    else
    {
        folder = ".";
    }

    if (folder.empty()) folder = ".";
    if (!directoryExistsA(folder.c_str())) return false;

    const std::string baseName = chooseArtBaseNameForFolder(folder, requestedBase);
    if (!artSetBaseExistsInFolder(folder, baseName.c_str()))
    {
        setStatus("No ART base file found in %s", folder.c_str());
        return false;
    }

    char oldDir[MAX_PATH] = {};
    const DWORD oldLen = GetCurrentDirectoryA(sizeof(oldDir), oldDir);
    const bool canRestore = oldLen > 0 && oldLen < sizeof(oldDir);

    if (!SetCurrentDirectoryA(folder.c_str())) return false;

    char artName[32] = {};
    std::strncpy(artName, baseName.c_str(), sizeof(artName) - 1);
    loadpics(artName);

    if (canRestore) SetCurrentDirectoryA(oldDir);

    reloadCurrentGamePalette();
    refreshTileList();
    refreshAllViews();

    const int loaded = countLoadedBuildTiles();
    if (loaded > 0)
    {
        setStatus("Loaded %d %s tile(s) from %s", loaded, statusLabel ? statusLabel : "BUILD", pathJoin(folder, baseName).c_str());
        return true;
    }

    setStatus("No tiles loaded from %s", pathJoin(folder, baseName).c_str());
    return false;
}

static bool loadBloodArtSetFromCurrentData()
{
    if (!isBloodMode()) return false;
    return loadArtSetFromPathOrFolder(bloodArtPathForLoad(), "Blood");
}

static std::string sanitizeGrpEntryName(const std::string& sourcePath)
{
    std::string name = pathBaseNameString(sourcePath);
    if (name.empty()) name = "IMPORT.BIN";

    for (char& ch : name)
    {
        if (ch >= 'a' && ch <= 'z') ch = static_cast<char>(ch - 'a' + 'A');
        if (ch == '<' || ch == '>' || ch == ':' || ch == '"' || ch == '/' || ch == '\\' ||
            ch == '|' || ch == '?' || ch == '*' || static_cast<unsigned char>(ch) < 32)
            ch = '_';
    }

    if (name.size() > 12) name.resize(12);
    while (!name.empty() && name[name.size() - 1] == ' ') name.resize(name.size() - 1);
    if (name.empty()) name = "IMPORT.BIN";
    return name;
}

static std::string defaultGrpExportPath()
{
    std::string base;
    std::string archivePath;
    if (!g.grpEditPath.empty()) { base = pathDirectory(g.grpEditPath); archivePath = g.grpEditPath; }
    else if (!g.dukeGrpPath.empty()) { base = pathDirectory(g.dukeGrpPath); archivePath = g.dukeGrpPath; }
    else if (!g.boardGrpPath.empty()) { base = pathDirectory(g.boardGrpPath); archivePath = g.boardGrpPath; }
    else base = moduleDirectory();
    return pathJoin(base, (!archivePath.empty() && isRffArchivePath(archivePath.c_str())) ? "rff_export" : "grp_export");
}

static void syncGrpExportPathFromEdit()
{
    if (!g.grpExportPathEdit) return;
    char buf[MAX_PATH * 2] = {};
    GetWindowTextA(g.grpExportPathEdit, buf, sizeof(buf));
    if (buf[0]) g.grpExportPath = buf;
}

static bool promptSelectGrpForEditor()
{
    char file[MAX_PATH] = "";
    if (!g.grpEditPath.empty())
        std::strncpy(file, g.grpEditPath.c_str(), sizeof(file) - 1);
    else if (!g.dukeGrpPath.empty())
        std::strncpy(file, g.dukeGrpPath.c_str(), sizeof(file) - 1);

    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g.main;
    ofn.lpstrFilter = isBloodMode()
        ? "Build/Blood archives (*.grp;*.rff)\0*.grp;*.rff\0Blood RFF archives (*.rff)\0*.rff\0BUILD group files (*.grp)\0*.grp\0All files (*.*)\0*.*\0"
        : "BUILD group files (*.grp)\0*.grp\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = sizeof(file);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = isBloodMode() ? "rff" : "grp";
    if (!GetOpenFileNameA(&ofn)) return false;

    std::vector<GrpMapEntry> entries;
    if (!readGrpEntries(file, entries))
    {
        MessageBoxA(g.main, "Could not read that archive. Use a Ken Silverman BUILD .grp or Blood .rff file.", "Archive Edit", MB_ICONERROR);
        return false;
    }

    g.grpEditPath = fullPathOf(file);
    if (g.dukeGrpPath.empty()) g.dukeGrpPath = g.grpEditPath;
    if (isBloodMode() && isRffArchivePath(g.grpEditPath.c_str()))
    {
        g.bloodDataPath = g.grpEditPath;
        g.bloodRffPath = g.grpEditPath;
        prepareBloodRffCache(g.bloodRffPath);
    }
    if (g.grpExportPath.empty()) g.grpExportPath = defaultGrpExportPath();
    loadPaletteFromGrp(g.grpEditPath.c_str());
    updateGrpEditPanel();
    setStatus("Archive editor opened %s.", pathBaseNameString(g.grpEditPath).c_str());
    return true;
}

static bool ensureGrpEditorPath(bool prompt)
{
    if (!g.grpEditPath.empty() && fileExistsA(g.grpEditPath.c_str())) return true;

    if (!g.dukeGrpPath.empty() && fileExistsA(g.dukeGrpPath.c_str()))
    {
        g.grpEditPath = g.dukeGrpPath;
        if (g.grpExportPath.empty()) g.grpExportPath = defaultGrpExportPath();
        updateGrpEditPanel();
        return true;
    }

    if (!g.boardGrpPath.empty() && fileExistsA(g.boardGrpPath.c_str()))
    {
        g.grpEditPath = g.boardGrpPath;
        if (g.grpExportPath.empty()) g.grpExportPath = defaultGrpExportPath();
        updateGrpEditPanel();
        return true;
    }

    return prompt ? promptSelectGrpForEditor() : false;
}

static void updateGrpEditPanel()
{
    if (!g.grpEdit) return;

    if (g.grpExportPath.empty()) g.grpExportPath = defaultGrpExportPath();
    if (g.grpExportPathEdit) SetWindowTextA(g.grpExportPathEdit, g.grpExportPath.c_str());

    if (g.grpPathLabel)
    {
        char text[MAX_PATH * 2] = {};
        if (!g.grpEditPath.empty())
            std::snprintf(text, sizeof(text), "Archive: %s", g.grpEditPath.c_str());
        else
            std::snprintf(text, sizeof(text), "Archive: no file selected");
        SetWindowTextA(g.grpPathLabel, text);
    }

    if (g.grpList)
    {
        SendMessageA(g.grpList, LB_RESETCONTENT, 0, 0);
        std::vector<GrpMapEntry> entries;
        if (!g.grpEditPath.empty() && readGrpEntries(g.grpEditPath.c_str(), entries))
        {
            for (size_t i = 0; i < entries.size(); ++i)
            {
                char row[256] = {};
                std::snprintf(row, sizeof(row), "%3d  %-12s  %10u bytes", static_cast<int>(i), entries[i].name.c_str(), entries[i].size);
                SendMessageA(g.grpList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(row));
            }
        }
    }

    InvalidateRect(g.grpEdit, nullptr, FALSE);
}

static bool promptGrpExportPath(const char* title)
{
    syncGrpExportPathFromEdit();
    std::string folder = g.grpExportPath.empty() ? defaultGrpExportPath() : g.grpExportPath;
    if (!promptString(title ? title : "Archive export path", "Export folder:", folder)) return false;
    g.grpExportPath = folder;
    if (g.grpExportPathEdit) SetWindowTextA(g.grpExportPathEdit, g.grpExportPath.c_str());
    return true;
}

static std::vector<int> selectedGrpEntryIndices()
{
    std::vector<int> indices;
    if (!g.grpList) return indices;

    const int selCount = static_cast<int>(SendMessageA(g.grpList, LB_GETSELCOUNT, 0, 0));
    if (selCount > 0 && selCount != LB_ERR)
    {
        indices.resize(selCount);
        const int got = static_cast<int>(SendMessageA(g.grpList, LB_GETSELITEMS, selCount, reinterpret_cast<LPARAM>(indices.data())));
        if (got > 0) indices.resize(got);
        else indices.clear();
    }

    // Fallback for the listbox caret/current selection, useful on double-clicks.
    if (indices.empty())
    {
        const int sel = static_cast<int>(SendMessageA(g.grpList, LB_GETCURSEL, 0, 0));
        if (sel != LB_ERR) indices.push_back(sel);
    }

    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
    return indices;
}

static bool extractGrpEntriesToFolderByIndices(const std::vector<int>& requestedIndices)
{
    if (!ensureGrpEditorPath(true)) return false;
    syncGrpExportPathFromEdit();
    if (g.grpExportPath.empty()) g.grpExportPath = defaultGrpExportPath();
    if (!ensureDirectoryTreeA(g.grpExportPath))
    {
        MessageBoxA(g.main, "Could not create the export folder.", "Archive Extract", MB_ICONERROR);
        return false;
    }

    std::vector<GrpMapEntry> entries;
    if (!readGrpEntries(g.grpEditPath.c_str(), entries))
    {
        MessageBoxA(g.main, "Could not read the archive file.", "Archive Extract", MB_ICONERROR);
        return false;
    }

    std::vector<int> indices;
    for (int index : requestedIndices)
        if (index >= 0 && index < static_cast<int>(entries.size())) indices.push_back(index);
    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());

    if (indices.empty())
    {
        MessageBoxA(g.main, "Select one or more archive entries to extract.", "Archive Extract", MB_ICONINFORMATION);
        return false;
    }

    if (indices.size() > 1)
    {
        char msg[512] = {};
        std::snprintf(msg, sizeof(msg), "Extract %d selected GRP entr%s to:\r\n%s\r\n\r\nExisting files with the same names will ask before being overwritten.",
            static_cast<int>(indices.size()), indices.size() == 1 ? "y" : "ies", g.grpExportPath.c_str());
        if (MessageBoxA(g.main, msg, "Archive Extract", MB_ICONQUESTION | MB_YESNO) != IDYES)
            return false;
    }

    int written = 0;
    int skipped = 0;
    for (int index : indices)
    {
        std::vector<unsigned char> bytes;
        if (!readGrpEntryBytes(g.grpEditPath.c_str(), entries[index], bytes))
        {
            ++skipped;
            continue;
        }

        const std::string outPath = pathJoin(g.grpExportPath, sanitizeFileName(entries[index].name));
        if (fileExistsA(outPath.c_str()))
        {
            char msg[MAX_PATH * 2] = {};
            std::snprintf(msg, sizeof(msg), "%s already exists. Overwrite it?", outPath.c_str());
            const int r = MessageBoxA(g.main, msg, "Archive Extract", MB_ICONQUESTION | MB_YESNOCANCEL);
            if (r == IDCANCEL) break;
            if (r != IDYES)
            {
                ++skipped;
                continue;
            }
        }

        if (writeFileBytesA(outPath, bytes)) ++written;
        else ++skipped;
    }

    if (indices.size() == 1 && written == 1)
        setStatus("Extracted %s to %s.", entries[indices[0]].name.c_str(), g.grpExportPath.c_str());
    else
        setStatus("Extracted %d of %d selected GRP entr%s to %s%s.", written, static_cast<int>(indices.size()),
            indices.size() == 1 ? "y" : "ies", g.grpExportPath.c_str(), skipped ? " (some skipped)" : "");
    return written > 0 && skipped == 0;
}

static bool extractGrpEntryToFolderByIndex(int index)
{
    std::vector<int> single;
    single.push_back(index);
    return extractGrpEntriesToFolderByIndices(single);
}

static bool extractAllGrpEntries()
{
    if (!ensureGrpEditorPath(true)) return false;
    if (!promptGrpExportPath("Extract all archive files")) return false;
    if (!ensureDirectoryTreeA(g.grpExportPath))
    {
        MessageBoxA(g.main, "Could not create the export folder.", "Archive Extract All", MB_ICONERROR);
        return false;
    }

    std::vector<GrpMapEntry> entries;
    if (!readGrpEntries(g.grpEditPath.c_str(), entries))
    {
        MessageBoxA(g.main, "Could not read the archive file.", "Archive Extract All", MB_ICONERROR);
        return false;
    }
    if (entries.empty())
    {
        MessageBoxA(g.main, "That archive has no entries.", "Archive Extract All", MB_ICONINFORMATION);
        return false;
    }

    char msg[512] = {};
    std::snprintf(msg, sizeof(msg), "Extract %d file(s) to:\r\n%s\r\n\r\nExisting files with the same names will be overwritten.",
        static_cast<int>(entries.size()), g.grpExportPath.c_str());
    if (MessageBoxA(g.main, msg, "Archive Extract All", MB_ICONQUESTION | MB_YESNO) != IDYES)
        return false;

    int written = 0;
    for (const GrpMapEntry& e : entries)
    {
        std::vector<unsigned char> bytes;
        if (!readGrpEntryBytes(g.grpEditPath.c_str(), e, bytes)) continue;
        const std::string outPath = pathJoin(g.grpExportPath, sanitizeFileName(e.name));
        if (writeFileBytesA(outPath, bytes)) ++written;
    }

    setStatus("Extracted %d of %d archive file(s) to %s.", written, static_cast<int>(entries.size()), g.grpExportPath.c_str());
    return written == static_cast<int>(entries.size());
}

static std::vector<std::string> parseMultiSelectFileBuffer(const char* buffer)
{
    std::vector<std::string> paths;
    if (!buffer || !buffer[0]) return paths;

    const char* p = buffer;
    std::string first = p;
    p += first.size() + 1;
    if (!*p)
    {
        paths.push_back(first);
        return paths;
    }

    const std::string folder = first;
    while (*p)
    {
        std::string file = p;
        paths.push_back(pathJoin(folder, file));
        p += file.size() + 1;
    }
    return paths;
}

static bool writeGrpFileAtomically(const std::string& path, const std::vector<GrpMapEntry>& entries, const std::vector<std::vector<unsigned char> >& data)
{
    if (entries.size() != data.size() || entries.size() > 0xffffffffull) return false;

    const std::string tempPath = path + ".tmp";
    FILE* f = nullptr;
    if (fopen_s(&f, tempPath.c_str(), "wb") != 0 || !f) return false;

    bool ok = std::fwrite("KenSilverman", 1, 12, f) == 12;
    ok = ok && writeLe32(f, static_cast<long>(entries.size()));

    for (size_t i = 0; ok && i < entries.size(); ++i)
    {
        char rawName[12] = {};
        const size_t copy = std::min<size_t>(12, entries[i].name.size());
        if (copy > 0) std::memcpy(rawName, entries[i].name.data(), copy);
        ok = std::fwrite(rawName, 1, 12, f) == 12;
        ok = ok && writeLe32(f, static_cast<long>(data[i].size()));
    }

    for (size_t i = 0; ok && i < data.size(); ++i)
    {
        if (!data[i].empty())
            ok = std::fwrite(data[i].data(), 1, data[i].size(), f) == data[i].size();
    }

    std::fclose(f);
    if (!ok)
    {
        DeleteFileA(tempPath.c_str());
        return false;
    }

    if (!MoveFileExA(tempPath.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        DeleteFileA(tempPath.c_str());
        return false;
    }
    return true;
}

static bool importFilesIntoGrp(const std::vector<std::string>& paths)
{
    if (paths.empty()) return false;
    if (!ensureGrpEditorPath(true)) return false;

    if (isRffArchivePath(g.grpEditPath.c_str()))
    {
        MessageBoxA(g.main, "RFF archives are supported for browsing and extraction, but this editor does not rewrite/encrypt RFF files yet. Extract files and keep Blood resources loose for now.", "RFF Import", MB_ICONINFORMATION);
        return false;
    }

    char msg[512] = {};
    std::snprintf(msg, sizeof(msg), "Import %d file(s) into:\r\n%s\r\n\r\nThe GRP will be rewritten. Existing matching 12-character entry names will be replaced.",
        static_cast<int>(paths.size()), g.grpEditPath.c_str());
    if (MessageBoxA(g.main, msg, "GRP Import", MB_ICONQUESTION | MB_YESNO) != IDYES)
        return false;

    std::vector<GrpMapEntry> entries;
    if (!readGrpEntries(g.grpEditPath.c_str(), entries))
    {
        MessageBoxA(g.main, "Could not read the GRP file before importing.", "GRP Import", MB_ICONERROR);
        return false;
    }

    std::vector<std::vector<unsigned char> > data(entries.size());
    for (size_t i = 0; i < entries.size(); ++i)
    {
        if (!readGrpEntryBytes(g.grpEditPath.c_str(), entries[i], data[i]))
        {
            MessageBoxA(g.main, "Could not read every current GRP entry. Import was canceled.", "GRP Import", MB_ICONERROR);
            return false;
        }
    }

    int imported = 0;
    for (const std::string& path : paths)
    {
        std::vector<unsigned char> bytes;
        if (!readFileBytesA(path, bytes)) continue;

        const std::string entryName = sanitizeGrpEntryName(path);
        const std::string key = upperAscii(entryName);
        int existing = -1;
        for (int i = 0; i < static_cast<int>(entries.size()); ++i)
        {
            if (upperAscii(entries[i].name) == key)
            {
                existing = i;
                break;
            }
        }

        if (existing >= 0)
        {
            data[existing] = bytes;
            entries[existing].size = static_cast<std::uint32_t>(bytes.size());
        }
        else
        {
            GrpMapEntry e;
            e.name = entryName;
            e.offset = 0;
            e.size = static_cast<std::uint32_t>(bytes.size());
            entries.push_back(e);
            data.push_back(bytes);
        }
        ++imported;
    }

    if (imported <= 0)
    {
        MessageBoxA(g.main, "None of the selected files could be imported.", "GRP Import", MB_ICONERROR);
        return false;
    }

    if (!writeGrpFileAtomically(g.grpEditPath, entries, data))
    {
        MessageBoxA(g.main, "Could not rewrite the GRP file.", "GRP Import", MB_ICONERROR);
        return false;
    }

    updateGrpEditPanel();
    refreshTileList();
    setStatus("Imported %d file(s) into %s.", imported, pathBaseNameString(g.grpEditPath).c_str());
    return true;
}

static bool promptImportFilesIntoGrp()
{
    if (!ensureGrpEditorPath(true)) return false;

    std::vector<char> fileBuffer(32768, 0);
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g.main;
    ofn.lpstrFilter = "All files (*.*)\0*.*\0";
    ofn.lpstrFile = fileBuffer.data();
    ofn.nMaxFile = static_cast<DWORD>(fileBuffer.size());
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
    if (!GetOpenFileNameA(&ofn)) return false;

    return importFilesIntoGrp(parseMultiSelectFileBuffer(fileBuffer.data()));
}

static void layoutGrpEditControls(HWND hwnd)
{
    RECT rc{};
    GetClientRect(hwnd, &rc);
    const int W = max(1, rc.right - rc.left);
    const int H = max(1, rc.bottom - rc.top);
    const int m = 14;
    int y = 12;

    moveCtl(g.grpPathLabel, m, y, W - 2 * m - 230, 22);
    moveCtl(g.grpSelectButton, W - m - 220, y, 112, 25);
    moveCtl(g.grpRefreshButton, W - m - 104, y, 104, 25);
    y += 36;

    HWND exportLabel = GetDlgItem(hwnd, 1410);
    moveCtl(exportLabel, m, y + 4, 86, 20);
    moveCtl(g.grpExportPathEdit, m + 92, y, max(120, W - (m + 92) - 126 - m), 24);
    moveCtl(g.grpChangeExportPathButton, W - m - 118, y, 118, 25);
    y += 38;

    const int buttonsH = 40;
    moveCtl(g.grpList, m, y, W - 2 * m, max(80, H - y - buttonsH - m));

    const int by = H - buttonsH + 6;
    const int buttonW = max(118, (W - 5 * m) / 4);
    int bx = m;
    moveCtl(g.grpExtractButton, bx, by, buttonW, 28); bx += buttonW + m;
    moveCtl(g.grpExtractAllButton, bx, by, buttonW, 28); bx += buttonW + m;
    moveCtl(g.grpImportButton, bx, by, buttonW, 28);
}

static void drawGrpEditPanel(HWND hwnd, HDC hdc)
{
    RECT rc{};
    GetClientRect(hwnd, &rc);
    fillRectColor(hdc, rc, theme::WindowBg);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, theme::MutedText);
    HFONT oldFont = reinterpret_cast<HFONT>(SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT)));
    const char* note = "Archive Edit: select a GRP/RFF, choose an export folder, extract one/all entries; GRP import is supported, RFF import is read-only.";
    RECT textRc{ 14, max(0, rc.bottom - 20), max(15, rc.right - 14), max(1, rc.bottom - 4) };
    DrawTextA(hdc, note, -1, &textRc, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
    SelectObject(hdc, oldFont);
}

static LRESULT CALLBACK GrpEditProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_CREATE:
        g.grpPathLabel = makeTileArtControl(hwnd, "STATIC", "Archive: no file selected", SS_LEFT, 0, 1409);
        g.grpSelectButton = makeTileArtControl(hwnd, "BUTTON", "Select Archive...", BS_PUSHBUTTON, 0, ids::ID_GRP_SELECT_GRP);
        g.grpRefreshButton = makeTileArtControl(hwnd, "BUTTON", "Refresh", BS_PUSHBUTTON, 0, ids::ID_GRP_REFRESH);
        makeTileArtControl(hwnd, "STATIC", "Export folder", SS_LEFT, 0, 1410);
        g.grpExportPathEdit = makeTileArtControl(hwnd, "EDIT", "", ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ids::ID_GRP_EXPORT_PATH_EDIT);
        g.grpChangeExportPathButton = makeTileArtControl(hwnd, "BUTTON", "Change...", BS_PUSHBUTTON, 0, ids::ID_GRP_CHANGE_EXPORT_PATH);
        g.grpList = makeTileArtControl(hwnd, "LISTBOX", "", WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_EXTENDEDSEL, WS_EX_CLIENTEDGE, ids::ID_GRP_LIST);
        g.grpExtractButton = makeTileArtControl(hwnd, "BUTTON", "Extract selected...", BS_PUSHBUTTON, 0, ids::ID_GRP_EXTRACT_SELECTED);
        g.grpExtractAllButton = makeTileArtControl(hwnd, "BUTTON", "Extract all...", BS_PUSHBUTTON, 0, ids::ID_GRP_EXTRACT_ALL);
        g.grpImportButton = makeTileArtControl(hwnd, "BUTTON", "Import files...", BS_PUSHBUTTON, 0, ids::ID_GRP_IMPORT_FILES);
        layoutGrpEditControls(hwnd);
        updateGrpEditPanel();
        return 0;
    case WM_SIZE:
        layoutGrpEditControls(hwnd);
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wp))
        {
        case ids::ID_GRP_SELECT_GRP:
            promptSelectGrpForEditor();
            return 0;
        case ids::ID_GRP_CHANGE_EXPORT_PATH:
            promptGrpExportPath("Archive export path");
            return 0;
        case ids::ID_GRP_EXTRACT_SELECTED:
            extractGrpEntriesToFolderByIndices(selectedGrpEntryIndices());
            return 0;
        case ids::ID_GRP_EXTRACT_ALL:
            extractAllGrpEntries();
            return 0;
        case ids::ID_GRP_IMPORT_FILES:
            promptImportFilesIntoGrp();
            return 0;
        case ids::ID_GRP_REFRESH:
            updateGrpEditPanel();
            return 0;
        case ids::ID_GRP_LIST:
            if (HIWORD(wp) == LBN_DBLCLK)
            {
                const int sel = g.grpList ? static_cast<int>(SendMessageA(g.grpList, LB_GETCURSEL, 0, 0)) : LB_ERR;
                extractGrpEntryToFolderByIndex(sel == LB_ERR ? -1 : sel);
                return 0;
            }
            break;
        }
        break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        drawGrpEditPanel(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
        return darkCtlColor(msg, wp);
    case WM_DRAWITEM:
        drawDarkButton(reinterpret_cast<DRAWITEMSTRUCT*>(lp));
        return TRUE;
    case WM_ERASEBKGND:
        return 1;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static bool saveArtRangeToFolder(int artIndex, const std::string& folder)
{
    const int start = artIndex * 256;
    const int end = min(MAXTILES - 1, start + 255);
    if (start < 0 || start >= MAXTILES || start > end) return false;

    char name[32];
    std::snprintf(name, sizeof(name), "TILES%03d.ART", artIndex);
    const std::string path = pathJoin(folder, name);

    FILE* f = nullptr;
    if (fopen_s(&f, path.c_str(), "wb") != 0 || !f) return false;

    bool ok = true;
    ok = ok && writeLe32(f, 1);            // artversion
    ok = ok && writeLe32(f, MAXTILES);     // numtiles is historically unused, but Build writes a total count.
    ok = ok && writeLe32(f, start);
    ok = ok && writeLe32(f, end);

    for (int t = start; ok && t <= end; ++t) ok = writeLe16(f, tilesizx[t]);
    for (int t = start; ok && t <= end; ++t) ok = writeLe16(f, tilesizy[t]);
    for (int t = start; ok && t <= end; ++t) ok = writeLe32(f, picanm[t]);

    for (int t = start; ok && t <= end; ++t)
    {
        const int w = tilesizx[t];
        const int h = tilesizy[t];
        if (w <= 0 || h <= 0) continue;
        if (!waloff[t]) loadtile(static_cast<short>(t));
        const unsigned char* src = reinterpret_cast<const unsigned char*>(static_cast<std::intptr_t>(waloff[t]));
        if (!src) { ok = false; break; }
        const size_t bytes = static_cast<size_t>(w) * static_cast<size_t>(h);
        ok = std::fwrite(src, 1, bytes, f) == bytes;
    }

    std::fclose(f);
    if (!ok) DeleteFileA(path.c_str());
    return ok;
}

static bool saveModifiedArtFiles()
{
    if (!g.artDirty)
    {
        setStatus("No unsaved ART tile changes.");
        return true;
    }

    const std::string folder = artSaveFolder();
    if (folder.empty()) return false;

    std::set<int> artRanges;
    for (std::unordered_map<int, TileOverride>::const_iterator it = g.tileOverrides.begin(); it != g.tileOverrides.end(); ++it)
    {
        if (it->second.dirty) artRanges.insert(it->first / 256);
    }

    if (artRanges.empty())
    {
        setArtDirty(false);
        setStatus("No unsaved ART tile changes.");
        return true;
    }

    for (std::set<int>::const_iterator it = artRanges.begin(); it != artRanges.end(); ++it)
    {
        if (!saveArtRangeToFolder(*it, folder))
        {
            char msg[256];
            std::snprintf(msg, sizeof(msg), "Could not save TILES%03d.ART to the GRP folder.", *it);
            MessageBoxA(g.main, msg, "Save ART", MB_ICONERROR);
            return false;
        }
    }

    for (std::unordered_map<int, TileOverride>::iterator it = g.tileOverrides.begin(); it != g.tileOverrides.end(); ++it)
        it->second.dirty = false;

    setArtDirty(false);
    updateTileArtPanel();
    setStatus("Saved %d ART file(s) to %s.", static_cast<int>(artRanges.size()), folder.c_str());
    return true;
}

static void layoutTileArtControls(HWND hwnd)
{
    RECT rc{};
    GetClientRect(hwnd, &rc);
    const int W = max(1, rc.right - rc.left);
    const int m = 14;
    const int top = 12;
    const int labelW = 56;
    const int editW = 90;
    int x = m;

    HWND tileLabel = GetDlgItem(hwnd, 1310);
    moveCtl(tileLabel, x, top + 4, labelW, 20);
    x += labelW;
    moveCtl(g.artTileEdit, x, top, editW, 24);
    x += editW + 8;
    moveCtl(g.artUseSelectedButton, x, top, 126, 25);
    x += 134;
    moveCtl(g.artImportButton, x, top, 104, 25);
    x += 112;
    moveCtl(g.artExportButton, x, top, 104, 25);
    x += 112;
    moveCtl(g.artSaveButton, x, top, 112, 25);
    x += 120;
    moveCtl(g.artPaletteButton, x, top, max(120, W - x - m), 25);
    moveCtl(g.artStatusLabel, m, top + 36, W - 2 * m, 22);
}

static void drawTileArtPanel(HWND hwnd, HDC hdc)
{
    RECT rc{};
    GetClientRect(hwnd, &rc);
    fillRectColor(hdc, rc, theme::WindowBg);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, theme::Text);

    const int panelTop = 76;
    RECT card{ 14, panelTop, max(15, rc.right - 14), max(panelTop + 240, rc.bottom - 14) };
    fillRectColor(hdc, card, theme::CardBg);
    drawRectFrame(hdc, card, theme::Border);

    const int previewSize = min(256, max(96, min(card.right - card.left - 36, card.bottom - card.top - 78)));
    const int px = card.left + 18;
    const int py = card.top + 46;

    HFONT oldFont = reinterpret_cast<HFONT>(SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT)));
    const char* title = "Tile Art Import / Export";
    TextOutA(hdc, card.left + 18, card.top + 16, title, static_cast<int>(std::strlen(title)));

    drawTilePreview(hdc, g.selectedTile, px, py, previewSize, previewSize);
    { RECT previewFrame{ px, py, px + previewSize, py + previewSize }; drawRectFrame(hdc, previewFrame, theme::Border); }

    char info[512];
    std::snprintf(info, sizeof(info),
        "Import PNG maps image colors to the Duke3D PALETTE.DAT palette and keeps transparent pixels as index 255.\r\n"
        "Save writes complete TILES###.ART replacement files beside DUKE3D.GRP, so the game can load them before the GRP.");
    RECT textRc{ px + previewSize + 24, py, card.right - 18, py + previewSize };
    DrawTextA(hdc, info, -1, &textRc, DT_LEFT | DT_WORDBREAK);
    SelectObject(hdc, oldFont);
}

static LRESULT CALLBACK TileArtProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_CREATE:
        makeTileArtControl(hwnd, "STATIC", "Tile #", SS_LEFT, 0, 1310);
        g.artTileEdit = makeTileArtControl(hwnd, "EDIT", "0", ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ids::ID_ART_TILE_EDIT);
        g.artUseSelectedButton = makeTileArtControl(hwnd, "BUTTON", "Use selected tile", BS_PUSHBUTTON, 0, ids::ID_ART_USE_SELECTED);
        g.artImportButton = makeTileArtControl(hwnd, "BUTTON", "Import PNG...", BS_PUSHBUTTON, 0, ids::ID_ART_IMPORT_PNG);
        g.artExportButton = makeTileArtControl(hwnd, "BUTTON", "Export PNG...", BS_PUSHBUTTON, 0, ids::ID_ART_EXPORT_PNG);
        g.artSaveButton = makeTileArtControl(hwnd, "BUTTON", "Save ART files", BS_PUSHBUTTON, 0, ids::ID_ART_SAVE_ART);
        g.artPaletteButton = makeTileArtControl(hwnd, "BUTTON", "Reload palette from data", BS_PUSHBUTTON, 0, ids::ID_ART_RELOAD_GRP_PALETTE);
        g.artStatusLabel = makeTileArtControl(hwnd, "STATIC", "Tile art ready", SS_LEFT, 0, 1311);
        layoutTileArtControls(hwnd);
        updateTileArtPanel();
        return 0;
    case WM_SIZE:
        layoutTileArtControls(hwnd);
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wp))
        {
        case ids::ID_ART_USE_SELECTED:
            updateTileArtPanel();
            return 0;
        case ids::ID_ART_IMPORT_PNG:
            importSelectedTileFromPng();
            return 0;
        case ids::ID_ART_EXPORT_PNG:
            exportSelectedTileToPng();
            return 0;
        case ids::ID_ART_SAVE_ART:
            saveModifiedArtFiles();
            return 0;
        case ids::ID_ART_RELOAD_GRP_PALETTE:
            if (ensureDukeGrpPath() && reloadCurrentGamePalette())
            {
                if (isBloodMode())
                    loadBloodArtSetFromCurrentData();
                else
                    refreshAllViews();
                setStatus("Reloaded %s palette%s from %s.", isBloodMode() ? "Blood" : "Duke3D", isBloodMode() ? " and tiles" : "", pathBaseNameString(g.dukeGrpPath).c_str());
            }
            else
            {
                MessageBoxA(g.main, "Could not reload a palette from the current game data.", "Tile art", MB_ICONWARNING);
            }
            return 0;
        }
        break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        drawTileArtPanel(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORBTN:
        return darkCtlColor(msg, wp);
    case WM_DRAWITEM:
        drawDarkButton(reinterpret_cast<DRAWITEMSTRUCT*>(lp));
        return TRUE;
    case WM_ERASEBKGND:
        return 1;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}


// -----------------------------------------------------------------------------
// Right-side selection inspector.
// -----------------------------------------------------------------------------
static HFONT makeInspectorFont(int points, int weight)
{
    HDC hdc = GetDC(nullptr);
    const int dpiY = hdc ? GetDeviceCaps(hdc, LOGPIXELSY) : 96;
    if (hdc) ReleaseDC(nullptr, hdc);

    return CreateFontA(-MulDiv(points, dpiY, 72), 0, 0, 0, weight, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
}

static void ensureInspectorFonts()
{
    if (!g.inspFont) g.inspFont = makeInspectorFont(9, FW_NORMAL);
    if (!g.inspBoldFont) g.inspBoldFont = makeInspectorFont(9, FW_SEMIBOLD);
}

static void setControlFont(HWND h, bool bold = false)
{
    if (!h) return;
    ensureInspectorFonts();
    SendMessageA(h, WM_SETFONT, reinterpret_cast<WPARAM>(bold ? g.inspBoldFont : g.inspFont), TRUE);
}

static HWND makeInspectorControl(HWND parent, const char* cls, const char* text, DWORD style, DWORD exStyle, int id, bool bold = false)
{
    if (cls && std::strcmp(cls, "BUTTON") == 0) style |= BS_OWNERDRAW;
    // Static labels get transparent painting so the custom inspector cards show
    // through, closer to a modern C#/WinForms inspector instead of old group boxes.
    if (cls && std::strcmp(cls, "STATIC") == 0) exStyle |= WS_EX_TRANSPARENT;

    HWND h = CreateWindowExA(exStyle, cls, text ? text : "", WS_CHILD | WS_VISIBLE | style,
        0, 0, 10, 10, parent, reinterpret_cast<HMENU>(id), g.inst, nullptr);
    setControlFont(h, bold);
    return h;
}

static void moveCtl(HWND h, int x, int y, int w, int ht)
{
    if (h) MoveWindow(h, x, y, max(1, w), max(1, ht), TRUE);
}

static void setEditLong(HWND h, long value)
{
    if (!h) return;
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%ld", value);
    SetWindowTextA(h, buf);
}

static void setEditBlank(HWND h)
{
    if (h) SetWindowTextA(h, "");
}

static bool readEditLong(HWND h, const char* label, long& out)
{
    char buf[128] = {};
    GetWindowTextA(h, buf, sizeof(buf));
    char* start = buf;
    while (*start == ' ' || *start == '\t') ++start;
    char* end = nullptr;
    long v = std::strtol(start, &end, 0);
    while (end && (*end == ' ' || *end == '\t')) ++end;
    if (!start[0] || !end || *end != 0)
    {
        char msg[192];
        std::snprintf(msg, sizeof(msg), "%s must be a valid number.", label ? label : "Value");
        MessageBoxA(g.main, msg, "Selection properties", MB_ICONERROR);
        if (h) SetFocus(h);
        return false;
    }
    out = v;
    return true;
}

static void enableCtl(HWND h, bool enabled)
{
    if (h) EnableWindow(h, enabled ? TRUE : FALSE);
}

static void updateInspector()
{
    if (!g.inspector || !g.inspTitle) return;

    g.inspectorUpdating = true;
    const bool surfaceOk = selectedSurfaceValid();
    const bool sectorOk = selectedSectorValid();
    const bool spriteOk = selectedSpriteValid();

    char title[160];
    if (surfaceOk)
    {
        if (g.sel.stat == SS_SPRITE)
            std::snprintf(title, sizeof(title), "Sprite %d  |  Sector %d", g.sel.wall, sprite[g.sel.wall].sectnum);
        else if (g.sel.stat == SS_WALL || g.sel.stat == SS_MASKED_WALL)
            std::snprintf(title, sizeof(title), "%s %d  |  Sector %d", selectedSurfaceName(), g.sel.wall, g.sel.sector);
        else
            std::snprintf(title, sizeof(title), "%s  |  Sector %d", selectedSurfaceName(), g.sel.sector);
    }
    else if (sectorOk)
    {
        std::snprintf(title, sizeof(title), "Sector %d", g.sel.sector);
    }
    else
    {
        std::snprintf(title, sizeof(title), "Nothing selected");
    }
    SetWindowTextA(g.inspTitle, title);

    if (g.inspSurfaceGroup) SetWindowTextA(g.inspSurfaceGroup, "Surface / Material");
    if (g.inspPositionGroup) SetWindowTextA(g.inspPositionGroup, "Sprite Position");
    if (g.inspSectorGroup) SetWindowTextA(g.inspSectorGroup, isBloodMode() ? "Current Sector" : "Sector Data");
    if (g.inspBloodGroup) SetWindowTextA(g.inspBloodGroup, isBloodMode() ? "Blood / Extended Tags" : "Advanced Build Fields");
    HWND labelLotag = GetDlgItem(g.inspector, 1205);
    HWND labelHitag = GetDlgItem(g.inspector, 1206);
    if (labelLotag) SetWindowTextA(labelLotag, isBloodMode() ? "Type" : "Lotag");
    if (labelHitag) SetWindowTextA(labelHitag, isBloodMode() ? "Data" : "Hitag");

    SetWindowTextA(g.inspSurfaceTitle, surfaceOk ? selectedSurfaceName() : "Select a wall, floor, ceiling, or sprite");
    enableCtl(g.inspTileEdit, surfaceOk);
    enableCtl(g.inspShadeEdit, surfaceOk);
    enableCtl(g.inspPalEdit, surfaceOk);
    enableCtl(g.inspUseTileButton, surfaceOk);

    if (surfaceOk)
    {
        setEditLong(g.inspTileEdit, getSelectedPic());
        setEditLong(g.inspShadeEdit, getSelectedShadeValue());
        setEditLong(g.inspPalEdit, getSelectedPalValue());
    }
    else
    {
        setEditBlank(g.inspTileEdit);
        setEditBlank(g.inspShadeEdit);
        setEditBlank(g.inspPalEdit);
    }

    enableCtl(g.inspXEdit, spriteOk);
    enableCtl(g.inspYEdit, spriteOk);
    enableCtl(g.inspZEdit, spriteOk);
    if (spriteOk)
    {
        const spritetype& sp = sprite[g.sel.wall];
        setEditLong(g.inspXEdit, sp.x);
        setEditLong(g.inspYEdit, sp.y);
        setEditLong(g.inspZEdit, sp.z);
    }
    else
    {
        setEditBlank(g.inspXEdit);
        setEditBlank(g.inspYEdit);
        setEditBlank(g.inspZEdit);
    }

    char sectorTitle[128];
    if (sectorOk) std::snprintf(sectorTitle, sizeof(sectorTitle), "Sector %d", g.sel.sector);
    else std::snprintf(sectorTitle, sizeof(sectorTitle), "No sector selected");
    SetWindowTextA(g.inspSectorTitle, sectorTitle);

    enableCtl(g.inspVisEdit, sectorOk);
    enableCtl(g.inspLotagEdit, sectorOk);
    enableCtl(g.inspHitagEdit, sectorOk);
    enableCtl(g.inspApplyButton, surfaceOk || sectorOk || spriteOk);
    enableCtl(g.inspRefreshButton, true);

    if (sectorOk)
    {
        setEditLong(g.inspVisEdit, static_cast<unsigned char>(sector[g.sel.sector].visibility));
        setEditLong(g.inspLotagEdit, sector[g.sel.sector].lotag);
        setEditLong(g.inspHitagEdit, sector[g.sel.sector].hitag);
    }
    else
    {
        setEditBlank(g.inspVisEdit);
        setEditBlank(g.inspLotagEdit);
        setEditBlank(g.inspHitagEdit);
    }


    const bool objectOk = selectedBuildObjectValid();
    if (g.inspBloodTitle)
    {
        const std::string caption = isBloodMode() ? bloodCaptionForSelectedObject() : std::string("Edit selected object lotag/hitag/extra/cstat directly.");
        SetWindowTextA(g.inspBloodTitle, caption.c_str());
    }

    enableCtl(g.inspTypeEdit, objectOk);
    enableCtl(g.inspObjHitagEdit, objectOk);
    enableCtl(g.inspExtraEdit, objectOk);
    enableCtl(g.inspCstatEdit, surfaceOk);
    enableCtl(g.inspStatnumEdit, spriteOk);
    enableCtl(g.inspAngEdit, spriteOk);
    enableCtl(g.inspBloodPresetButton, isBloodMode() && spriteOk);
    enableCtl(g.inspBloodFixupButton, isBloodMode() && objectOk);

    if (objectOk)
    {
        setEditLong(g.inspTypeEdit, getSelectedTypeValue());
        setEditLong(g.inspObjHitagEdit, getSelectedHitagValue());
        setEditLong(g.inspExtraEdit, getSelectedExtraValue());
    }
    else
    {
        setEditBlank(g.inspTypeEdit);
        setEditBlank(g.inspObjHitagEdit);
        setEditBlank(g.inspExtraEdit);
    }

    if (surfaceOk) setEditLong(g.inspCstatEdit, getSelectedCstatValue());
    else setEditBlank(g.inspCstatEdit);

    if (spriteOk)
    {
        setEditLong(g.inspStatnumEdit, sprite[g.sel.wall].statnum);
        setEditLong(g.inspAngEdit, sprite[g.sel.wall].ang);
    }
    else
    {
        setEditBlank(g.inspStatnumEdit);
        setEditBlank(g.inspAngEdit);
    }

    g.inspectorUpdating = false;
}

static void applyInspectorEdits(bool useCurrentTile)
{
    if (g.inspectorUpdating) return;

    const bool surfaceOk = selectedSurfaceValid();
    const bool sectorOk = selectedSectorValid();
    const bool spriteOk = selectedSpriteValid();
    if (!surfaceOk && !sectorOk && !spriteOk)
    {
        setStatus("No selected wall/floor/ceiling/sprite/sector to edit.");
        return;
    }

    bool changed = false;
    if (surfaceOk)
    {
        long tileValue = g.selectedTile;
        if (!useCurrentTile && !readEditLong(g.inspTileEdit, "Texture", tileValue)) return;
        tileValue = clamp_long(tileValue, 0, MAXTILES - 1);
        changed = setSelectedPicDirect(static_cast<short>(tileValue)) || changed;
        if (useCurrentTile) setEditLong(g.inspTileEdit, tileValue);

        long shadeValue = 0;
        if (!readEditLong(g.inspShadeEdit, "Shade", shadeValue)) return;
        changed = setSelectedShadeDirect(static_cast<int>(shadeValue)) || changed;

        long palValue = 0;
        if (!readEditLong(g.inspPalEdit, "Palette", palValue)) return;
        changed = setSelectedPalDirect(static_cast<int>(palValue)) || changed;
    }

    // Sector fields are applied before sprite movement so moving a sprite into a
    // different sector does not accidentally copy the old sector tags/visibility
    // onto the new sector in the same Apply click.
    if (sectorOk)
    {
        sectortype& s = sector[g.sel.sector];
        long visValue = 0, loValue = 0, hiValue = 0;
        if (!readEditLong(g.inspVisEdit, "Sector visibility", visValue)) return;
        if (!readEditLong(g.inspLotagEdit, "Sector lotag", loValue)) return;
        if (!readEditLong(g.inspHitagEdit, "Sector hitag", hiValue)) return;

        const char vis = static_cast<char>(clamp_int(static_cast<int>(visValue), 0, 255));
        const short lotag = static_cast<short>(clamp_long(loValue, -32768, 65535));
        const short hitag = static_cast<short>(clamp_long(hiValue, -32768, 65535));

        if (s.visibility != vis) { s.visibility = vis; changed = true; }
        if (s.lotag != lotag) { s.lotag = lotag; changed = true; }
        if (s.hitag != hitag) { s.hitag = hitag; changed = true; }
    }

    if (spriteOk)
    {
        long xValue = 0, yValue = 0, zValue = 0;
        if (!readEditLong(g.inspXEdit, "Sprite X", xValue)) return;
        if (!readEditLong(g.inspYEdit, "Sprite Y", yValue)) return;
        if (!readEditLong(g.inspZEdit, "Sprite Z", zValue)) return;

        spritetype& sp = sprite[g.sel.wall];
        if (sp.x != xValue || sp.y != yValue || sp.z != zValue)
        {
            setsprite(g.sel.wall, xValue, yValue, zValue);
            g.sel.sector = sprite[g.sel.wall].sectnum;
            searchsector = g.sel.sector;
            searchwall = g.sel.wall;
            searchstat = static_cast<short>(g.sel.stat);
            changed = true;
        }
    }


    const bool objectOk = selectedBuildObjectValid();
    if (objectOk)
    {
        long typeValue = 0, hitagValue = 0, extraValue = 0;
        if (!readEditLong(g.inspTypeEdit, isBloodMode() ? "Blood type" : "Object lotag", typeValue)) return;
        if (!readEditLong(g.inspObjHitagEdit, isBloodMode() ? "Blood data/hitag" : "Object hitag", hitagValue)) return;
        if (!readEditLong(g.inspExtraEdit, "Extra", extraValue)) return;
        changed = setSelectedTypeDirect(static_cast<int>(typeValue)) || changed;
        changed = setSelectedHitagDirect(static_cast<int>(hitagValue)) || changed;
        changed = setSelectedExtraDirect(static_cast<int>(extraValue)) || changed;
    }

    if (surfaceOk)
    {
        long cstatValue = 0;
        if (!readEditLong(g.inspCstatEdit, "Cstat/stat", cstatValue)) return;
        changed = setSelectedCstatDirect(static_cast<int>(cstatValue)) || changed;
    }

    if (spriteOk)
    {
        long statValue = 0, angValue = 0;
        if (!readEditLong(g.inspStatnumEdit, "Sprite statnum", statValue)) return;
        if (!readEditLong(g.inspAngEdit, "Sprite angle", angValue)) return;
        changed = setSelectedSpriteStatnumDirect(static_cast<int>(statValue)) || changed;
        changed = setSelectedSpriteAngleDirect(static_cast<int>(angValue)) || changed;
        if (isBloodMode())
        {
            const BloodSpriteDefault* def = bloodSpriteDefaultByTypeOrPic(sprite[g.sel.wall].lotag, sprite[g.sel.wall].picnum);
            if (def)
                changed = bloodApplySpriteDefault(g.sel.wall, *def, def->picnum >= 0) || changed;
        }
    }

    if (changed)
    {
        setDirty();
        setStatus("Selection properties updated.");
        updateInspector();
        refreshAllViews();
    }
    else
    {
        setStatus("Selection properties unchanged.");
    }
}

static void layoutInspectorControls(HWND hwnd)
{
    RECT rc{};
    GetClientRect(hwnd, &rc);
    const int W = max(1, rc.right - rc.left);
    const int m = 10;
    const int cardX = m;
    const int cardW = max(1, W - 2 * m);
    const int cardPadX = 12;
    const int headerH = 26;
    const int contentX = cardX + cardPadX;
    const int contentW = max(1, cardW - 2 * cardPadX);
    const int labelW = clamp_int(contentW / 3, 58, 86);
    const int editGap = 8;
    const int editX = contentX + labelW + editGap;
    const int editW = max(56, cardX + cardW - cardPadX - editX);
    const int halfGap = 8;
    const int halfW = max(44, (editW - halfGap) / 2);
    const int halfX2 = editX + halfW + halfGap;
    const int rowH = 23;
    int y = 10;

    HWND labelTexture = GetDlgItem(hwnd, 1201);
    HWND labelShade = GetDlgItem(hwnd, 1202);
    HWND labelPal = GetDlgItem(hwnd, 1203);
    HWND labelVis = GetDlgItem(hwnd, 1204);
    HWND labelLotag = GetDlgItem(hwnd, 1205);
    HWND labelHitag = GetDlgItem(hwnd, 1206);
    HWND labelX = GetDlgItem(hwnd, 1209);
    HWND labelY = GetDlgItem(hwnd, 1210);
    HWND labelZ = GetDlgItem(hwnd, 1211);
    HWND labelType = GetDlgItem(hwnd, 1213);
    HWND labelObjHitag = GetDlgItem(hwnd, 1214);
    HWND labelExtra = GetDlgItem(hwnd, 1215);
    HWND labelCstat = GetDlgItem(hwnd, 1216);
    HWND labelStatnum = GetDlgItem(hwnd, 1217);
    HWND labelAngle = GetDlgItem(hwnd, 1218);

    const int selectionH = 70;
    moveCtl(g.inspSelectionGroup, contentX, y + 6, contentW, 18);
    moveCtl(g.inspTitle, contentX, y + headerH + 13, contentW, 22);
    y += selectionH + 10;

    const int surfaceH = 190;
    moveCtl(g.inspSurfaceGroup, contentX, y + 6, contentW, 18);
    moveCtl(g.inspSurfaceTitle, contentX, y + headerH + 9, contentW, 18);
    int rowY = y + headerH + 36;
    moveCtl(labelTexture, contentX, rowY + 4, labelW, 18);
    moveCtl(g.inspTileEdit, editX, rowY, editW, rowH); rowY += 31;
    moveCtl(g.inspUseTileButton, contentX, rowY, contentW, 25); rowY += 35;
    moveCtl(labelShade, contentX, rowY + 4, labelW, 18);
    moveCtl(g.inspShadeEdit, editX, rowY, editW, rowH); rowY += 31;
    moveCtl(labelPal, contentX, rowY + 4, labelW, 18);
    moveCtl(g.inspPalEdit, editX, rowY, editW, rowH);
    y += surfaceH + 10;

    const int positionH = 140;
    moveCtl(g.inspPositionGroup, contentX, y + 6, contentW, 18);
    rowY = y + headerH + 18;
    moveCtl(labelX, contentX, rowY + 4, labelW, 18);
    moveCtl(g.inspXEdit, editX, rowY, editW, rowH); rowY += 31;
    moveCtl(labelY, contentX, rowY + 4, labelW, 18);
    moveCtl(g.inspYEdit, editX, rowY, editW, rowH); rowY += 31;
    moveCtl(labelZ, contentX, rowY + 4, labelW, 18);
    moveCtl(g.inspZEdit, editX, rowY, editW, rowH);
    y += positionH + 10;

    const int sectorH = 166;
    moveCtl(g.inspSectorGroup, contentX, y + 6, contentW, 18);
    moveCtl(g.inspSectorTitle, contentX, y + headerH + 9, contentW, 18);
    rowY = y + headerH + 36;
    moveCtl(labelVis, contentX, rowY + 4, labelW, 18);
    moveCtl(g.inspVisEdit, editX, rowY, editW, rowH); rowY += 31;
    moveCtl(labelLotag, contentX, rowY + 4, labelW, 18);
    moveCtl(g.inspLotagEdit, editX, rowY, editW, rowH); rowY += 31;
    moveCtl(labelHitag, contentX, rowY + 4, labelW, 18);
    moveCtl(g.inspHitagEdit, editX, rowY, editW, rowH);
    y += sectorH + 10;

    const int bloodH = 278;
    moveCtl(g.inspBloodGroup, contentX, y + 6, contentW, 18);
    moveCtl(g.inspBloodTitle, contentX, y + headerH + 8, contentW, 34);
    rowY = y + headerH + 48;
    moveCtl(labelType, contentX, rowY + 4, labelW, 18);
    moveCtl(g.inspTypeEdit, editX, rowY, editW, rowH); rowY += 31;
    moveCtl(labelObjHitag, contentX, rowY + 4, labelW, 18);
    moveCtl(g.inspObjHitagEdit, editX, rowY, editW, rowH); rowY += 31;
    moveCtl(labelExtra, contentX, rowY + 4, labelW, 18);
    moveCtl(g.inspExtraEdit, editX, rowY, editW, rowH); rowY += 31;
    moveCtl(labelCstat, contentX, rowY + 4, labelW, 18);
    moveCtl(g.inspCstatEdit, editX, rowY, editW, rowH); rowY += 31;
    moveCtl(labelStatnum, contentX, rowY + 4, labelW, 18);
    moveCtl(g.inspStatnumEdit, editX, rowY, halfW, rowH);
    moveCtl(labelAngle, halfX2, rowY + 4, labelW, 18);
    moveCtl(g.inspAngEdit, halfX2 + labelW, rowY, max(40, cardX + cardW - cardPadX - (halfX2 + labelW)), rowH); rowY += 34;
    moveCtl(g.inspBloodPresetButton, contentX, rowY, contentW, 25); rowY += 31;
    moveCtl(g.inspBloodFixupButton, contentX, rowY, contentW, 25);
    y += bloodH + 14;

    const int buttonW = max(92, (W - 3 * m) / 2);
    moveCtl(g.inspApplyButton, m, y, buttonW, 30);
    moveCtl(g.inspRefreshButton, m + buttonW + m, y, W - (m + buttonW + 2 * m), 30);

    InvalidateRect(hwnd, nullptr, TRUE);
}

static void drawInspectorCard(HDC hdc, const RECT& r)
{
    fillRectColor(hdc, r, theme::CardBg);

    RECT header = r;
    header.bottom = header.top + 26;
    fillRectColor(hdc, header, theme::HeaderBg);
    drawRectFrame(hdc, r, theme::Border);
}

static void drawInspectorChrome(HWND hwnd, HDC hdc)
{
    RECT rc{};
    GetClientRect(hwnd, &rc);
    fillRectColor(hdc, rc, theme::WindowBg);

    const int W = max(1, rc.right - rc.left);
    const int m = 10;
    const int cardX = m;
    const int cardW = max(1, W - 2 * m);
    int y = 10;

    const int heights[] = { 70, 190, 140, 166, 278 };
    for (int i = 0; i < 5; ++i)
    {
        RECT card{ cardX, y, cardX + cardW, y + heights[i] };
        drawInspectorCard(hdc, card);
        y += heights[i] + 10;
    }
}

static LRESULT CALLBACK InspectorProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        g.inspSelectionGroup = makeInspectorControl(hwnd, "STATIC", "Selection", SS_LEFT, 0, 1190, true);
        g.inspSurfaceGroup = makeInspectorControl(hwnd, "STATIC", "Surface / Material", SS_LEFT, 0, 1191, true);
        g.inspPositionGroup = makeInspectorControl(hwnd, "STATIC", "Sprite Position", SS_LEFT, 0, 1192, true);
        g.inspSectorGroup = makeInspectorControl(hwnd, "STATIC", "Sector Data", SS_LEFT, 0, 1193, true);

        g.inspTitle = makeInspectorControl(hwnd, "STATIC", "Nothing selected", SS_LEFT, 0, 1200, true);
        g.inspSurfaceTitle = makeInspectorControl(hwnd, "STATIC", "Select a surface", SS_LEFT, 0, 1207);
        makeInspectorControl(hwnd, "STATIC", "Texture", SS_LEFT, 0, 1201);
        g.inspTileEdit = makeInspectorControl(hwnd, "EDIT", "", ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ids::ID_INSP_TILE_EDIT);
        g.inspUseTileButton = makeInspectorControl(hwnd, "BUTTON", "Use selected texture tile", BS_PUSHBUTTON, 0, ids::ID_INSP_USE_TILE);
        makeInspectorControl(hwnd, "STATIC", "Shade", SS_LEFT, 0, 1202);
        g.inspShadeEdit = makeInspectorControl(hwnd, "EDIT", "", ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ids::ID_INSP_SHADE_EDIT);
        makeInspectorControl(hwnd, "STATIC", "Palette", SS_LEFT, 0, 1203);
        g.inspPalEdit = makeInspectorControl(hwnd, "EDIT", "", ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ids::ID_INSP_PAL_EDIT);

        makeInspectorControl(hwnd, "STATIC", "X", SS_LEFT, 0, 1209);
        g.inspXEdit = makeInspectorControl(hwnd, "EDIT", "", ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ids::ID_INSP_X_EDIT);
        makeInspectorControl(hwnd, "STATIC", "Y", SS_LEFT, 0, 1210);
        g.inspYEdit = makeInspectorControl(hwnd, "EDIT", "", ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ids::ID_INSP_Y_EDIT);
        makeInspectorControl(hwnd, "STATIC", "Z", SS_LEFT, 0, 1211);
        g.inspZEdit = makeInspectorControl(hwnd, "EDIT", "", ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ids::ID_INSP_Z_EDIT);

        g.inspSectorTitle = makeInspectorControl(hwnd, "STATIC", "No sector selected", SS_LEFT, 0, 1208);
        makeInspectorControl(hwnd, "STATIC", "Visibility", SS_LEFT, 0, 1204);
        g.inspVisEdit = makeInspectorControl(hwnd, "EDIT", "", ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ids::ID_INSP_VIS_EDIT);
        makeInspectorControl(hwnd, "STATIC", "Lotag", SS_LEFT, 0, 1205);
        g.inspLotagEdit = makeInspectorControl(hwnd, "EDIT", "", ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ids::ID_INSP_LOTAG_EDIT);
        makeInspectorControl(hwnd, "STATIC", "Hitag", SS_LEFT, 0, 1206);
        g.inspHitagEdit = makeInspectorControl(hwnd, "EDIT", "", ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ids::ID_INSP_HITAG_EDIT);

        g.inspBloodGroup = makeInspectorControl(hwnd, "STATIC", "Blood / Extended Tags", SS_LEFT, 0, 1194, true);
        g.inspBloodTitle = makeInspectorControl(hwnd, "STATIC", "No selected Blood object", SS_LEFT, 0, 1212);
        makeInspectorControl(hwnd, "STATIC", "Type", SS_LEFT, 0, 1213);
        g.inspTypeEdit = makeInspectorControl(hwnd, "EDIT", "", ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ids::ID_INSP_TYPE_EDIT);
        makeInspectorControl(hwnd, "STATIC", "Data", SS_LEFT, 0, 1214);
        g.inspObjHitagEdit = makeInspectorControl(hwnd, "EDIT", "", ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ids::ID_INSP_OBJ_HITAG_EDIT);
        makeInspectorControl(hwnd, "STATIC", "Extra", SS_LEFT, 0, 1215);
        g.inspExtraEdit = makeInspectorControl(hwnd, "EDIT", "", ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ids::ID_INSP_EXTRA_EDIT);
        makeInspectorControl(hwnd, "STATIC", "Cstat", SS_LEFT, 0, 1216);
        g.inspCstatEdit = makeInspectorControl(hwnd, "EDIT", "", ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ids::ID_INSP_CSTAT_EDIT);
        makeInspectorControl(hwnd, "STATIC", "Statnum", SS_LEFT, 0, 1217);
        g.inspStatnumEdit = makeInspectorControl(hwnd, "EDIT", "", ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ids::ID_INSP_STATNUM_EDIT);
        makeInspectorControl(hwnd, "STATIC", "Angle", SS_LEFT, 0, 1218);
        g.inspAngEdit = makeInspectorControl(hwnd, "EDIT", "", ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, ids::ID_INSP_ANG_EDIT);
        g.inspBloodPresetButton = makeInspectorControl(hwnd, "BUTTON", "Blood sprite preset...", BS_PUSHBUTTON, 0, ids::ID_INSP_BLOOD_PRESET);
        g.inspBloodFixupButton = makeInspectorControl(hwnd, "BUTTON", "Normalize Blood tags", BS_PUSHBUTTON, 0, ids::ID_INSP_BLOOD_FIXUP);

        g.inspApplyButton = makeInspectorControl(hwnd, "BUTTON", "Apply changes", BS_DEFPUSHBUTTON, 0, ids::ID_INSP_APPLY);
        g.inspRefreshButton = makeInspectorControl(hwnd, "BUTTON", "Revert", BS_PUSHBUTTON, 0, ids::ID_INSP_REFRESH);
        layoutInspectorControls(hwnd);
        updateInspector();
        return 0;
    }
    case WM_SIZE:
        layoutInspectorControls(hwnd);
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wp))
        {
        case ids::ID_INSP_USE_TILE:
            applyInspectorEdits(true);
            return 0;
        case ids::ID_INSP_APPLY:
            applyInspectorEdits(false);
            return 0;
        case ids::ID_INSP_REFRESH:
            updateInspector();
            return 0;
        case ids::ID_INSP_BLOOD_PRESET:
            bloodChooseAndApplyPresetToSelectedSprite();
            return 0;
        case ids::ID_INSP_BLOOD_FIXUP:
            bloodNormalizeSelectedObject();
            return 0;
        }
        break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        drawInspectorChrome(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_CTLCOLORSTATIC:
    {
        HDC hdc = reinterpret_cast<HDC>(wp);
        HWND child = reinterpret_cast<HWND>(lp);
        const int id = GetDlgCtrlID(child);
        SetBkMode(hdc, TRANSPARENT);
        if (id >= 1190 && id <= 1194)
        {
            SetTextColor(hdc, theme::Text);
            return reinterpret_cast<LRESULT>(themeHeaderBrush());
        }
        if (id == 1200)
        {
            SetTextColor(hdc, theme::Text);
            return reinterpret_cast<LRESULT>(themeCardBrush());
        }
        if (id == 1207 || id == 1208 || id == 1212)
        {
            SetTextColor(hdc, theme::MutedText);
            return reinterpret_cast<LRESULT>(themeCardBrush());
        }
        SetTextColor(hdc, theme::MutedText);
        return reinterpret_cast<LRESULT>(themeCardBrush());
    }
    case WM_CTLCOLOREDIT:
        return darkCtlColor(msg, wp);
    case WM_CTLCOLORBTN:
        return darkCtlColor(msg, wp);
    case WM_DRAWITEM:
        drawDarkButton(reinterpret_cast<DRAWITEMSTRUCT*>(lp));
        return TRUE;
    case WM_ERASEBKGND:
        return 1;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

// -----------------------------------------------------------------------------
// Main window layout and commands.
// -----------------------------------------------------------------------------
static void switchTab(int tab)
{
    g.activeTab = clamp_int(tab, 0, 3);
    TabCtrl_SetCurSel(g.tab, g.activeTab);
    if (g.tab) RedrawWindow(g.tab, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME);
    ShowWindow(g.view2d, g.activeTab == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(g.host3d, g.activeTab == 1 ? SW_SHOW : SW_HIDE);
    ShowWindow(g.tileArt, g.activeTab == 2 ? SW_SHOW : SW_HIDE);
    ShowWindow(g.grpEdit, g.activeTab == 3 ? SW_SHOW : SW_HIDE);
    if (g.engine) ShowWindow(g.engine, g.activeTab == 1 ? SW_SHOW : SW_HIDE);
    if (g.activeTab == 1)
    {
        attachEngineWindow();
        SetFocus(g.engine ? g.engine : g.host3d);
    }
    if (g.activeTab == 2) updateTileArtPanel();
    if (g.activeTab == 3) updateGrpEditPanel();
    refreshAllViews();
}

static void layoutChildren(HWND hwnd)
{
    RECT rc{}; GetClientRect(hwnd, &rc);
    const int clientW = max(1, rc.right - rc.left);
    const int clientH = max(1, rc.bottom - rc.top);
    const int inspectorW = max(180, clientW / 5); // Right inspector: roughly 20% of the main window.
    const int editorW = max(1, clientW - inspectorW);

    const int texH = min(currentTextureBarHeight(), max(TEXTURE_BAR_MIN_HEIGHT, clientH - 120));
    const int tabH = max(0, clientH - texH);
    MoveWindow(g.tab, 0, 0, editorW, tabH, TRUE);
    MoveWindow(g.textureBar, 0, tabH, editorW, texH, TRUE);
    if (g.inspector) MoveWindow(g.inspector, editorW, 0, inspectorW, clientH, TRUE);

    RECT tr{}; GetClientRect(g.tab, &tr);
    TabCtrl_AdjustRect(g.tab, FALSE, &tr);
    tr.left += TAB_MARGIN;
    tr.top += TAB_MARGIN;
    tr.right -= TAB_MARGIN;
    tr.bottom -= TAB_MARGIN;
    const int w = max(1, tr.right - tr.left);
    const int h = max(1, tr.bottom - tr.top);
    MoveWindow(g.view2d, tr.left, tr.top, w, h, TRUE);
    MoveWindow(g.host3d, tr.left, tr.top, w, h, TRUE);
    MoveWindow(g.tileArt, tr.left, tr.top, w, h, TRUE);
    MoveWindow(g.grpEdit, tr.left, tr.top, w, h, TRUE);
    if (g.engine)
        SetWindowPos(g.engine, nullptr, 0, 0, w, h, SWP_NOZORDER | SWP_SHOWWINDOW);
    if (g.tab)
        RedrawWindow(g.tab, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME);
}

static void gotoTileDialog()
{
    long v = g.selectedTile;
    if (promptLong("Goto tile", "Tile number:", v, 0, MAXTILES - 1))
    {
        g.selectedTile = static_cast<short>(v);
        if (tilesizx[g.selectedTile] <= 0 || tilesizy[g.selectedTile] <= 0)
            setStatus("Tile %d is empty, but it is selected.", g.selectedTile);
        centerTextureBarOnSelected();
    }
}

static void showKeyboardHelp()
{
    const char* msg =
        "2D view:\n"
        "  Right-click: context menu; right drag: pan; mouse wheel: zoom\n"
        "  Left drag vertex/sprite: move it\n"
        "  Space/click: begin/add sector points, Enter: close sector, Esc: cancel\n"
        "  Insert or right-click > Add point here: insert point on nearest wall; S: insert sprite; Delete: delete\n"
        "  G: grid size, L: grid lock, Ctrl+T: show tags\n"
        "  Shift-click a sprite/vertex overlap to grab the vertex instead of the sprite\n"
        "\n3D view:\n"
        "  W/A/S/D: move, arrows: look/turn, Space/C or PgUp/PgDn: fly up/down\n"
        "  Right drag: turn/look, left click: select wall/floor/sprite\n"
        "  Camera movement only runs while the 3D editor has focus\n"
        "  F2/F3/F4/F6: 2D, 3D, Tile Art, Archive Edit tabs; F5: run map\n"
        "\nEditing:\n"
        "  Texture bar click: select tile, header button: expand/shrink browser\n"
        "  Tile Art tab: import/export PNG tiles and save TILES###.ART beside DUKE3D.GRP\n"
        "  Archive Edit tab: browse GRP/RFF, extract selected/all; import is GRP-only\n"
        "  Sprite selection: click a sprite, then edit X/Y/Z in the inspector\n"
        "  V/Ctrl-click: apply tile, Tab: copy properties, Enter: paste properties\n"
        "  B/1/M/T/P: blocking, one-way, masked, translucent, parallax\n"
        "  +/-: shade, PgUp/PgDn: adjust selected Z where applicable\n"
        "\nBlood mode:\n"
        "  Startup selector chooses Duke Nukem 3D or Blood; Shadow Warrior is greyed out for now\n"
        "  Blood inspector card edits selected object type/extra/cstat/statnum/angle\n"
        "  Blood sprite insertion opens the Blood preset list and applies statnum/pic/repeat/pal defaults";
    MessageBoxA(g.main, msg, "Keyboard help", MB_ICONINFORMATION);
}

static bool mountGroupIfExists(const char* name)
{
    DWORD attr = GetFileAttributesA(name);
    if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY)) return false;
    const bool ok = initgroupfile(const_cast<char*>(name)) >= 0;
    if (ok && g.dukeGrpPath.empty())
    {
        g.dukeGrpPath = fullPathOf(name);
        if (g.grpEditPath.empty()) g.grpEditPath = g.dukeGrpPath;
        if (g.grpExportPath.empty()) g.grpExportPath = pathJoin(pathDirectory(g.grpEditPath), "grp_export");
    }
    return ok;
}

static void loadBuildAssets()
{
    if (isBloodMode())
    {
        if (fileExistsA("BLOOD.RFF")) g.bloodDataPath = fullPathOf("BLOOD.RFF");
        else if (fileExistsA("blood.rff")) g.bloodDataPath = fullPathOf("blood.rff");
        if (!g.bloodDataPath.empty()) g.dukeGrpPath = g.bloodDataPath;
        else if (fileExistsA("TILES000.ART")) g.dukeGrpPath = fullPathOf("TILES000.ART");
        else if (fileExistsA("tiles000.art")) g.dukeGrpPath = fullPathOf("tiles000.art");

        if (!g.bloodDataPath.empty() && isRffArchivePath(g.bloodDataPath.c_str()))
        {
            g.bloodRffPath = g.bloodDataPath;
            g.grpEditPath = g.bloodDataPath;
            prepareBloodRffCache(g.bloodRffPath);
        }

        initengine();
        inittimerhandler();
        reloadCurrentGamePalette();
        if (!loadBloodArtSetFromCurrentData())
            setStatus("Blood mode ready, but no Blood ART set was found. Choose BLOOD.RFF or TILES000.ART from the Tile Art tab.");
        else
            setStatus("Blood mode ready. RFF archive support is enabled; Shadow Warrior is disabled for now.");
        return;
    }

    mountGroupIfExists("duke3d.grp");
    mountGroupIfExists("DUKE3D.GRP");
    initengine();
    inittimerhandler();
    if (!g.dukeGrpPath.empty()) loadPaletteFromGrp(g.dukeGrpPath.c_str());
    loadpics(const_cast<char*>("tiles000.art"));
    refreshTileList();
}

static bool writeMapSnapshotForRun(const std::string& path)
{
    if (path.empty()) return false;
    short sect = g.startsect;
    updatesector(g.startx, g.starty, &sect);

    char filename[MAX_PATH] = {};
    std::strncpy(filename, path.c_str(), sizeof(filename) - 1);
    if (saveboard(filename, &g.startx, &g.starty, &g.startz, &g.startang, &sect) < 0)
        return false;
    g.startsect = sect;
    return true;
}

static std::string moduleDirectory()
{
    char path[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, path, sizeof(path));
    return pathDirectory(path);
}

static bool findDuke3DX(std::string& exePath)
{
    std::vector<std::string> dirs;
    if (!g.dukeGrpPath.empty()) dirs.push_back(pathDirectory(g.dukeGrpPath));
    if (!g.boardGrpPath.empty()) dirs.push_back(pathDirectory(g.boardGrpPath));
    if (!g.boardPath.empty() && g.boardPath.find('(') == std::string::npos) dirs.push_back(pathDirectory(g.boardPath));
    dirs.push_back(moduleDirectory());
    dirs.push_back(".");

    for (size_t i = 0; i < dirs.size(); ++i)
    {
        const std::string candidate = pathJoin(dirs[i], "Duke3DX.exe");
        if (fileExistsA(candidate.c_str()))
        {
            exePath = fullPathOf(candidate.c_str());
            return true;
        }
    }

    char file[MAX_PATH] = "";
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g.main;
    ofn.lpstrFilter = "Duke3DX executable (Duke3DX.exe)\0Duke3DX.exe\0Executables (*.exe)\0*.exe\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = sizeof(file);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "exe";
    if (!GetOpenFileNameA(&ofn)) return false;
    exePath = fullPathOf(file);
    return true;
}


static bool findBloodRunner(std::string& exePath)
{
    std::vector<std::string> dirs;
    if (!g.bloodDataPath.empty()) dirs.push_back(pathDirectory(g.bloodDataPath));
    if (!g.dukeGrpPath.empty()) dirs.push_back(pathDirectory(g.dukeGrpPath));
    if (!g.boardPath.empty() && g.boardPath.find('(') == std::string::npos) dirs.push_back(pathDirectory(g.boardPath));
    dirs.push_back(moduleDirectory());
    dirs.push_back(".");

    const char* names[] = { "NBlood.exe", "nblood.exe", "BLOOD.EXE", "blood.exe" };
    for (size_t d = 0; d < dirs.size(); ++d)
    {
        for (size_t n = 0; n < sizeof(names) / sizeof(names[0]); ++n)
        {
            const std::string candidate = pathJoin(dirs[d], names[n]);
            if (fileExistsA(candidate.c_str()))
            {
                exePath = fullPathOf(candidate.c_str());
                return true;
            }
        }
    }

    char file[MAX_PATH] = "";
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g.main;
    ofn.lpstrFilter = "Blood executable (NBlood.exe; BLOOD.EXE)\0NBlood.exe;BLOOD.EXE;nblood.exe;blood.exe\0Executables (*.exe)\0*.exe\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = sizeof(file);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "exe";
    if (!GetOpenFileNameA(&ofn)) return false;
    exePath = fullPathOf(file);
    return true;
}

static bool findGameRunner(std::string& exePath)
{
    return isBloodMode() ? findBloodRunner(exePath) : findDuke3DX(exePath);
}

static bool saveBeforeRunIfNeeded()
{
    if (!g.dirty && !g.artDirty) return true;

    const int r = MessageBoxA(g.main,
        "The current map or tile art has unsaved changes.\r\n\r\nSave changes before running Duke3DX?",
        "Run map", MB_ICONQUESTION | MB_YESNO);
    if (r != IDYES)
    {
        setStatus("Run canceled because changes were not saved.");
        return false;
    }

    bool ok = true;
    if (g.dirty)
    {
        if (g.boardFromGrp || g.boardPath.empty() || g.boardPath == "newboard.map" || g.boardPath == "newblood.map")
            ok = promptSaveMap();
        else
            ok = saveMapFile(g.boardPath.c_str());
    }

    if (ok && g.artDirty)
        ok = saveModifiedArtFiles();

    if (!ok) setStatus("Run canceled because save failed or was canceled.");
    return ok;
}

static bool buildRunMapPath(std::string& mapPath)
{
    mapPath.clear();

    if (!g.boardFromGrp && !g.boardPath.empty() && g.boardPath != "newboard.map" && g.boardPath != "newblood.map")
    {
        if (g.dirty)
        {
            MessageBoxA(g.main, "The map still has unsaved changes. Save it before running.", "Run map", MB_ICONWARNING);
            return false;
        }
        mapPath = fullPathOf(g.boardPath.c_str());
        return true;
    }

    std::string folder;
    if (!g.boardGrpPath.empty()) folder = pathDirectory(g.boardGrpPath);
    else if (!g.dukeGrpPath.empty()) folder = pathDirectory(g.dukeGrpPath);
    else folder = moduleDirectory();

    std::string name;
    if (g.boardFromGrp && !g.boardGrpEntry.empty()) name = g.boardGrpEntry;
    else if (!g.boardPath.empty() && g.boardPath != "newboard.map" && g.boardPath != "newblood.map") name = pathBaseNameString(g.boardPath);
    else name = "BuildXRun.map";

    name = sanitizeFileName(forceMapExtension(name));
    mapPath = pathJoin(folder, name);
    if (!writeMapSnapshotForRun(mapPath))
    {
        MessageBoxA(g.main, "Could not write a temporary map file to run.", "Run map", MB_ICONERROR);
        return false;
    }
    return true;
}

static void runCurrentMap()
{
    if (!saveBeforeRunIfNeeded()) return;

    std::string mapPath;
    if (!buildRunMapPath(mapPath)) return;

    std::string exePath;
    if (!findGameRunner(exePath)) return;

    const std::string workDir = pathDirectory(exePath);
    std::string commandLine = "\"" + exePath + "\" ";
    commandLine += isBloodMode() ? "-map \"" : "/map \"";
    commandLine += mapPath + "\"";

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::vector<char> mutableCmd(commandLine.begin(), commandLine.end());
    mutableCmd.push_back('\0');

    BOOL ok = CreateProcessA(exePath.c_str(), mutableCmd.data(), nullptr, nullptr, FALSE, 0, nullptr,
        workDir.empty() ? nullptr : workDir.c_str(), &si, &pi);
    if (!ok)
    {
        MessageBoxA(g.main, isBloodMode() ? "Could not launch the selected Blood executable." : "Could not launch Duke3DX.exe.", "Run map", MB_ICONERROR);
        return;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    setStatus("Launched %s with %s", pathBaseNameString(exePath).c_str(), pathBaseNameString(mapPath).c_str());
}


static void handleCommand(HWND hwnd, int id)
{
    switch (id)
    {
    case ids::M_FILE_NEW:
        if (!confirmDiscardChanges()) break;
        initNewMap();
        g.boardPath = isBloodMode() ? "newblood.map" : "newboard.map";
        g.boardFromGrp = false;
        g.boardGrpPath.clear();
        g.boardGrpEntry.clear();
        setDirty(false);
        updateTitle();
        refreshAllViews();
        break;
    case ids::M_FILE_OPEN:
        if (confirmDiscardChanges()) promptOpenMap();
        break;
    case ids::M_FILE_OPEN_GRP:
        if (confirmDiscardChanges()) promptOpenMapFromGrp();
        break;
    case ids::M_FILE_SAVE:
    {
        bool ok = true;
        if (g.dirty)
        {
            if (g.boardFromGrp || g.boardPath.empty() || g.boardPath == "newboard.map" || g.boardPath == "newblood.map") ok = promptSaveMap();
            else ok = saveMapFile(g.boardPath.c_str());
        }
        if (ok && g.artDirty) saveModifiedArtFiles();
        break;
    }
    case ids::M_FILE_SAVE_AS:
    {
        const bool ok = promptSaveMap();
        if (ok && g.artDirty) saveModifiedArtFiles();
        break;
    }
    case ids::M_FILE_RUN_MAP:
        runCurrentMap();
        break;
    case ids::M_FILE_EXIT:
        PostMessage(hwnd, WM_CLOSE, 0, 0);
        break;
    case ids::M_VIEW_2D:
        switchTab(0); break;
    case ids::M_VIEW_3D:
        switchTab(1); break;
    case ids::M_VIEW_TILE_ART:
        switchTab(2); break;
    case ids::M_VIEW_GRPEDIT:
        switchTab(3); break;
    case ids::M_VIEW_ZOOM_IN:
        g.zoom = min(g.zoom + (g.zoom >> 3) + 1, 65536); refreshAllViews(); break;
    case ids::M_VIEW_ZOOM_OUT:
        g.zoom = max(g.zoom - (g.zoom >> 3), 24); refreshAllViews(); break;
    case ids::M_VIEW_GRID:
        g.grid = (g.grid + 1) % 7; refreshAllViews(); break;
    case ids::M_VIEW_GRID_LOCK:
        g.gridLock = !g.gridLock; refreshAllViews(); break;
    case ids::M_VIEW_SHOW_TAGS:
        g.showTags = !g.showTags; refreshAllViews(); break;
    case ids::M_EDIT_BEGIN_DRAW:
        beginOrAddDrawPoint(g.view2d); break;
    case ids::M_EDIT_CLOSE_DRAW:
        commitDrawnSector(); break;
    case ids::M_EDIT_CANCEL_DRAW:
        g.drawingSector = false; g.drawPts.clear(); refreshAllViews(); break;
    case ids::M_EDIT_INSERT_POINT:
        insertPointAtMouse(g.view2d); break;
    case ids::M_EDIT_INSERT_SPRITE:
        insertSpriteAt(g.lastMapMouse.x, g.lastMapMouse.y); break;
    case ids::M_EDIT_DELETE:
        deleteSelection(); break;
    case ids::M_EDIT_COPY_PROPS:
        copyProps(); break;
    case ids::M_EDIT_PASTE_PROPS:
        pasteProps(); break;
    case ids::M_EDIT_APPLY_TEXTURE:
        applySelectedTexture(); break;
    case ids::M_EDIT_PROPERTIES:
        editProperties(); break;
    case ids::M_TOOLS_RELINK:
        rebuildSectorLinks(); setDirty(); refreshAllViews(); break;
    case ids::M_TOOLS_SET_START:
        g.startx = g.cam.x; g.starty = g.cam.y; g.startz = g.cam.z; g.startang = g.cam.ang; g.startsect = g.cam.sect; setDirty(); refreshAllViews(); break;
    case ids::M_TOOLS_TOGGLE_BLOCKING:
        toggleBlocking(); break;
    case ids::M_TOOLS_TOGGLE_ONEWAY:
        toggleOneWay(); break;
    case ids::M_TOOLS_TOGGLE_MASKED:
        toggleMasked(); break;
    case ids::M_TOOLS_TOGGLE_TRANSLUCENT:
        toggleTranslucent(); break;
    case ids::M_TOOLS_TOGGLE_PARALLAX:
        toggleParallax(); break;
    case ids::M_TOOLS_SHADE_UP:
        shadeSelection(+1); break;
    case ids::M_TOOLS_SHADE_DOWN:
        shadeSelection(-1); break;
    case ids::M_TOOLS_Z_UP:
        zAdjustSelection(-1024); break;
    case ids::M_TOOLS_Z_DOWN:
        zAdjustSelection(+1024); break;
    case ids::M_BLOOD_SPRITE_PRESET:
        bloodChooseAndApplyPresetToSelectedSprite(); break;
    case ids::M_BLOOD_FIXUP_SELECTION:
        bloodNormalizeSelectedObject(); break;
    case ids::M_BLOOD_FIXUP_MAP:
        bloodNormalizeMapForBlood(false); break;
    case ids::M_TEXTURE_GOTO:
        gotoTileDialog(); break;
    case ids::M_TEXTURE_TOGGLE_SIZE:
        toggleTextureBarSize(); break;
    case ids::M_HELP_KEYS:
        showKeyboardHelp(); break;
    }
}

static LRESULT CALLBACK Host3DProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_ERASEBKGND:
        return 1;
    case WM_SIZE:
        if (g.engine) SetWindowPos(g.engine, nullptr, 0, 0, LOWORD(lp), HIWORD(lp), SWP_NOZORDER | SWP_SHOWWINDOW);
        return 0;
    case WM_SETFOCUS:
        if (g.engine) SetFocus(g.engine);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static void drawDarkTabItem(const DRAWITEMSTRUCT* dis)
{
    if (!dis || !dis->hwndItem) return;

    RECT r = dis->rcItem;
    const bool selected = (dis->itemState & ODS_SELECTED) != 0 ||
        TabCtrl_GetCurSel(dis->hwndItem) == static_cast<int>(dis->itemID);

    fillRectColor(dis->hDC, r, selected ? theme::PanelBg : theme::WindowBg);
    drawRectFrame(dis->hDC, r, selected ? theme::Accent : theme::Border);

    char text[80] = {};
    TCITEMA item{};
    item.mask = TCIF_TEXT;
    item.pszText = text;
    item.cchTextMax = sizeof(text);
    TabCtrl_GetItem(dis->hwndItem, static_cast<int>(dis->itemID), &item);

    SetBkMode(dis->hDC, TRANSPARENT);
    SetTextColor(dis->hDC, selected ? theme::Text : theme::MutedText);
    InflateRect(&r, -10, 0);
    DrawTextA(dis->hDC, text, -1, &r, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS);
}

static void drawDarkTabLabel(HWND tab, HDC hdc, int index, bool selected)
{
    RECT r{};
    if (!TabCtrl_GetItemRect(tab, index, &r)) return;

    // Keep selected tabs visually connected to the dark pane and non-selected
    // tabs slightly recessed, matching the rest of the dark editor chrome.
    RECT fill = r;
    if (selected)
    {
        fill.top = max(0, fill.top - 1);
        fill.bottom += 2;
    }
    else
    {
        fill.top += 2;
    }

    fillRectColor(hdc, fill, selected ? theme::PanelBg : theme::WindowBg);
    drawRectFrame(hdc, fill, selected ? theme::Accent : theme::Border);

    if (selected)
    {
        // Cover the lower border of the active tab so it blends into the page.
        RECT seam = fill;
        seam.top = seam.bottom - 2;
        fillRectColor(hdc, seam, theme::PanelBg);
    }

    char text[96] = {};
    TCITEMA item{};
    item.mask = TCIF_TEXT;
    item.pszText = text;
    item.cchTextMax = sizeof(text);
    TabCtrl_GetItem(tab, index, &item);

    RECT textRc = r;
    InflateRect(&textRc, -12, 0);
    if (selected) textRc.top = max(0, textRc.top - 1);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, selected ? theme::Text : theme::MutedText);
    HFONT oldFont = reinterpret_cast<HFONT>(SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT)));
    DrawTextA(hdc, text, -1, &textRc, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS);
    if (oldFont) SelectObject(hdc, oldFont);
}

static void drawDarkTabControl(HWND hwnd, HDC hdc)
{
    RECT rc{};
    GetClientRect(hwnd, &rc);
    fillRectColor(hdc, rc, theme::WindowBg);

    RECT pane = rc;
    TabCtrl_AdjustRect(hwnd, FALSE, &pane);
    pane.left = max(rc.left, pane.left);
    pane.top = max(rc.top, pane.top);
    pane.right = min(rc.right, pane.right);
    pane.bottom = min(rc.bottom, pane.bottom);

    if (pane.right > pane.left && pane.bottom > pane.top)
    {
        fillRectColor(hdc, pane, theme::PanelBg);
        drawRectFrame(hdc, pane, theme::Border);

        RECT inner = pane;
        InflateRect(&inner, -1, -1);
        if (inner.right > inner.left && inner.bottom > inner.top)
            drawRectFrame(hdc, inner, RGB(38, 40, 46));
    }

    const int count = TabCtrl_GetItemCount(hwnd);
    const int selected = TabCtrl_GetCurSel(hwnd);

    for (int i = 0; i < count; ++i)
    {
        if (i != selected) drawDarkTabLabel(hwnd, hdc, i, false);
    }
    if (selected >= 0 && selected < count)
        drawDarkTabLabel(hwnd, hdc, selected, true);
}

static LRESULT CALLBACK DarkTabProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_ERASEBKGND:
    {
        HDC hdc = reinterpret_cast<HDC>(wp);
        RECT rc{};
        GetClientRect(hwnd, &rc);
        fillRectColor(hdc, rc, theme::WindowBg);
        return 1;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        drawDarkTabControl(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_PRINTCLIENT:
    {
        HDC hdc = reinterpret_cast<HDC>(wp);
        drawDarkTabControl(hwnd, hdc);
        return 0;
    }
    case WM_THEMECHANGED:
    case WM_SETTINGCHANGE:
        InvalidateRect(hwnd, nullptr, TRUE);
        break;
    }

    if (g.oldTabProc)
        return CallWindowProcA(g.oldTabProc, hwnd, msg, wp, lp);
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static void subclassDarkTabControl(HWND hwnd)
{
    if (!hwnd || g.oldTabProc) return;

    // The normal themed tab control still paints a light page border even when
    // TCS_OWNERDRAWFIXED is used.  Subclass it so the whole tab frame and page
    // border are painted with our dark colors.
    g.oldTabProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(DarkTabProc)));
    RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME);
}

static LRESULT CALLBACK MainProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_ERASEBKGND:
    {
        HDC hdc = reinterpret_cast<HDC>(wp);
        RECT rc{};
        GetClientRect(hwnd, &rc);
        fillRectColor(hdc, rc, theme::WindowBg);
        return 1;
    }
    case WM_NCPAINT:
    {
        LRESULT result = DefWindowProcA(hwnd, msg, wp, lp);
        paintDarkWindowBorder(hwnd);
        return result;
    }
    case WM_NCACTIVATE:
    {
        LRESULT result = DefWindowProcA(hwnd, msg, wp, lp);
        enableDarkTitleBar(hwnd);
        paintDarkWindowBorder(hwnd);
        return result;
    }
    case WM_ACTIVATE:
    case WM_THEMECHANGED:
    case WM_SETTINGCHANGE:
        enableDarkTitleBar(hwnd);
        paintDarkWindowBorder(hwnd);
        break;
    case WM_CREATE:
    {
        g.main = hwnd;
        enableDarkTitleBar(hwnd);
        SetMenu(hwnd, makeMainMenu());
        DrawMenuBar(hwnd);

        g.tab = CreateWindowExA(0, WC_TABCONTROLA, "", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TCS_OWNERDRAWFIXED,
            0, 0, 100, 100, hwnd, reinterpret_cast<HMENU>(ids::ID_TAB), g.inst, nullptr);
        TCITEMA item{};
        item.mask = TCIF_TEXT;
        item.pszText = const_cast<char*>("2D Editor");
        TabCtrl_InsertItem(g.tab, 0, &item);
        item.pszText = const_cast<char*>("3D Editor");
        TabCtrl_InsertItem(g.tab, 1, &item);
        item.pszText = const_cast<char*>("Tile Art");
        TabCtrl_InsertItem(g.tab, 2, &item);
        item.pszText = const_cast<char*>("Archive Edit");
        TabCtrl_InsertItem(g.tab, 3, &item);
        subclassDarkTabControl(g.tab);

        g.view2d = CreateWindowExA(0, "BuildEditor2DView", "", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP,
            0, 0, 100, 100, g.tab, reinterpret_cast<HMENU>(ids::ID_VIEW2D), g.inst, nullptr);
        g.host3d = CreateWindowExA(0, "BuildEditor3DHost", "", WS_CHILD | WS_CLIPSIBLINGS | WS_TABSTOP,
            0, 0, 100, 100, g.tab, reinterpret_cast<HMENU>(ids::ID_HOST3D), g.inst, nullptr);
        g.tileArt = CreateWindowExA(0, "BuildEditorTileArt", "", WS_CHILD | WS_CLIPSIBLINGS | WS_TABSTOP,
            0, 0, 100, 100, g.tab, reinterpret_cast<HMENU>(ids::ID_TILEART), g.inst, nullptr);
        g.grpEdit = CreateWindowExA(0, "BuildEditorGrpEdit", "", WS_CHILD | WS_CLIPSIBLINGS | WS_TABSTOP,
            0, 0, 100, 100, g.tab, reinterpret_cast<HMENU>(ids::ID_GRPEDIT), g.inst, nullptr);
        g.textureBar = CreateWindowExA(0, "BuildEditorTextureBar", "", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
            0, 0, 100, TEXTURE_BAR_HEIGHT, hwnd, reinterpret_cast<HMENU>(ids::ID_TEXTUREBAR), g.inst, nullptr);
        g.inspector = CreateWindowExA(0, "BuildEditorInspector", "", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
            0, 0, 100, 100, hwnd, reinterpret_cast<HMENU>(ids::ID_INSPECTOR), g.inst, nullptr);
        updateInspector();
        updateTileArtPanel();
        updateGrpEditPanel();

        QueryPerformanceFrequency(&g.freq);
        QueryPerformanceCounter(&g.lastTick);
        SetTimer(hwnd, ids::TIMER_RENDER, 16, nullptr);
        updateTitle();
        layoutChildren(hwnd);
        return 0;
    }
    case WM_SIZE:
        layoutChildren(hwnd);
        return 0;
    case WM_MEASUREITEM:
    {
        MEASUREITEMSTRUCT* mis = reinterpret_cast<MEASUREITEMSTRUCT*>(lp);
        if (mis && mis->CtlType == ODT_MENU)
        {
            measureDarkMenuItem(mis);
            return TRUE;
        }
        break;
    }
    case WM_DRAWITEM:
    {
        DRAWITEMSTRUCT* dis = reinterpret_cast<DRAWITEMSTRUCT*>(lp);
        if (dis && dis->CtlType == ODT_MENU)
        {
            drawDarkMenuItem(dis);
            return TRUE;
        }
        if (dis && dis->CtlID == ids::ID_TAB)
        {
            drawDarkTabItem(dis);
            return TRUE;
        }
        break;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
        return darkCtlColor(msg, wp);
    case WM_NOTIFY:
        if (reinterpret_cast<NMHDR*>(lp)->hwndFrom == g.tab && reinterpret_cast<NMHDR*>(lp)->code == TCN_SELCHANGE)
            switchTab(TabCtrl_GetCurSel(g.tab));
        return 0;
    case WM_TIMER:
        if (wp == ids::TIMER_RENDER) render3D();
        return 0;
    case WM_COMMAND:
        handleCommand(hwnd, LOWORD(wp));
        return 0;
    case WM_KEYDOWN:
        if (GetKeyState(VK_CONTROL) & 0x8000)
        {
            if (wp == 'N') { handleCommand(hwnd, ids::M_FILE_NEW); return 0; }
            if (wp == 'O') { handleCommand(hwnd, ids::M_FILE_OPEN); return 0; }
            if (wp == 'S') { handleCommand(hwnd, ids::M_FILE_SAVE); return 0; }
            if (wp == 'T') { handleCommand(hwnd, ids::M_VIEW_SHOW_TAGS); return 0; }
        }
        if (wp == VK_F2) { switchTab(0); return 0; }
        if (wp == VK_F3) { switchTab(1); return 0; }
        if (wp == VK_F4) { switchTab(2); return 0; }
        if (wp == VK_F5) { runCurrentMap(); return 0; }
        if (wp == VK_F6) { switchTab(3); return 0; }
        break;
    case WM_CLOSE:
        if (!confirmDiscardChanges()) return 0;
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, ids::TIMER_RENDER);
        if (g.inspFont) { DeleteObject(g.inspFont); g.inspFont = nullptr; }
        if (g.inspBoldFont) { DeleteObject(g.inspBoldFont); g.inspBoldFont = nullptr; }
        uninitengine();
        uninitgroupfile();
        if (g.gdiplusToken)
        {
            Gdiplus::GdiplusShutdown(g.gdiplusToken);
            g.gdiplusToken = 0;
        }
        releaseDarkMenuData();
        releaseThemeBrushes();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static void registerWindowClasses(HINSTANCE inst)
{
    WNDCLASSA wc{};
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = themeWindowBrush();
    wc.style = CS_DBLCLKS;

    wc.lpfnWndProc = MainProc;
    wc.lpszClassName = "BuildWin32EditorMain";
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    RegisterClassA(&wc);

    wc.lpfnWndProc = View2DProc;
    wc.lpszClassName = "BuildEditor2DView";
    wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    RegisterClassA(&wc);

    wc.style = CS_DBLCLKS | CS_OWNDC;
    wc.lpfnWndProc = Host3DProc;
    wc.lpszClassName = "BuildEditor3DHost";
    wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    RegisterClassA(&wc);

    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = TileArtProc;
    wc.lpszClassName = "BuildEditorTileArt";
    wc.hbrBackground = themePanelBrush();
    RegisterClassA(&wc);

    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = GrpEditProc;
    wc.lpszClassName = "BuildEditorGrpEdit";
    wc.hbrBackground = themePanelBrush();
    RegisterClassA(&wc);

    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = TextureBarProc;
    wc.lpszClassName = "BuildEditorTextureBar";
    wc.hbrBackground = themePanelBrush();
    RegisterClassA(&wc);

    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = InspectorProc;
    wc.lpszClassName = "BuildEditorInspector";
    wc.hbrBackground = themePanelBrush();
    wc.hIcon = nullptr;
    RegisterClassA(&wc);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR lpCmdLine, int nCmdShow)
{
    g.inst = hInst;
    enableProcessDarkMode();
    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_TAB_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    Gdiplus::GdiplusStartupInput gdiplusInput;
    if (Gdiplus::GdiplusStartup(&g.gdiplusToken, &gdiplusInput, nullptr) != Gdiplus::Ok)
        g.gdiplusToken = 0;

    registerWindowClasses(hInst);
    if (!showGameSelectScreen())
    {
        if (g.gdiplusToken)
        {
            Gdiplus::GdiplusShutdown(g.gdiplusToken);
            g.gdiplusToken = 0;
        }
        releaseThemeBrushes();
        return 0;
    }
    loadBuildAssets();

    int argc = 0;
    LPWSTR* argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::string initialMap;
    if (argc >= 2 && argvW && argvW[1])
    {
        char mb[MAX_PATH] = {};
        WideCharToMultiByte(CP_ACP, 0, argvW[1], -1, mb, sizeof(mb), nullptr, nullptr);
        initialMap = mb;
    }
    if (argvW) LocalFree(argvW);

    bool initialLoaded = false;
    if (!initialMap.empty()) initialLoaded = loadMapFile(initialMap.c_str());
    else
    {
        initNewMap();
        if (isBloodMode()) g.boardPath = "newblood.map";
    }

    HWND hwnd = CreateWindowExA(0, "BuildWin32EditorMain", "BuildX",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 820,
        nullptr, nullptr, hInst, nullptr);
    if (!hwnd) return 1;

    g.main = hwnd;
    enableDarkTitleBar(hwnd);
    updateTitle();
    ShowWindow(hwnd, SW_SHOWMAXIMIZED);
    enableDarkTitleBar(hwnd);
    paintDarkWindowBorder(hwnd);
    DrawMenuBar(hwnd);
    UpdateWindow(hwnd);

    if (!initialMap.empty() && initialLoaded)
        updateTitle();

    MSG msg{};
    while (GetMessageA(&msg, nullptr, 0, 0) > 0)
    {
        if (msg.message == WM_KEYDOWN)
        {
            if (msg.wParam == VK_F2) { switchTab(0); continue; }
            if (msg.wParam == VK_F3) { switchTab(1); continue; }
            if (msg.wParam == VK_F4) { switchTab(2); continue; }
            if (msg.wParam == VK_F5) { runCurrentMap(); continue; }
            if (msg.wParam == VK_F6) { switchTab(3); continue; }
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return static_cast<int>(msg.wParam);
}

#!/usr/bin/env python3
"""Convert 3drealms/duke3d.grp into the basedn/ assets the engine loads.

Input (relative to --root):
  3drealms/duke3d.grp    retail Duke Nukem 3D group file

Outputs (under <root>/basedn/):
  maps/*.map             raw Build v7 maps, lowercased (cmodel.cpp loads maps/<name>.map)
  scripts/*.con          CON scripts, lowercased (dncon.cpp loads scripts/game.con)
  sound/*.wav            VOC sounds converted to 8-bit PCM WAV (sndwave.cpp)
  sprites/tiles.dat      6144 entries of {int16 width, int16 height, uint32 picanm}
  sprites/tables.dat     raw Build TABLES.DAT (dnmath.cpp reads sine/radar tables)
  pics/palookup.dat      int16 numshades + 32*256 shade tables (from PALETTE.DAT)
  pics/lookup.dat        raw LOOKUP.DAT palette remap tables
  pics/colormap.pcx      global 256-color palette (gl_image.cpp Draw_GetPalette)
  pics/16to8.dat         65536-byte RGB565 to palette-index table
  textures/duke/tile%04d.wal   raw palette-index tiles (ref_gl TileImage bakes
                               pal/shade at load time from these)
  pics/dn/tile%04d_p%d_s%d.pcx  UI tiles with pal/shade baked into the pixels
                               (client dnhud/dnmenu/dnintermission draw these)
  anm/dnintro/*          intro sequence baked from tiles (dnintro_assets.py)
  asset-manifest.json    summary of everything generated

Formats were verified against this repository's loaders: ref_gl/dnrender.cpp
(TileImage, LoadPalettes), ref_gl/gl_image.cpp (LoadPCX/DN_DecodePCX,
Draw_GetPalette, 16to8 indexing in GL_BuildPalettedTexture), qcommon/dnpcx.cpp,
qcommon/dnmath.cpp (DN_MathInit), sound/sndwave.cpp and client/dnintro.cpp.
Written from scratch for this project; no third-party code is included.
"""

import argparse
import json
import os
import struct
import sys

import dnintro_assets

MAX_TILES = 6144  # qcommon/dnmap.h DN_MAX_TILES


def fail(message):
    sys.stderr.write("prepare_assets.py: %s\n" % message)
    sys.exit(1)


# ---------------------------------------------------------------------------
# Binary helpers
# ---------------------------------------------------------------------------

def u16(data, offset):
    return struct.unpack_from("<H", data, offset)[0]


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def i16(data, offset):
    return struct.unpack_from("<h", data, offset)[0]


# ---------------------------------------------------------------------------
# GRP (Ken Silverman group file) reader
# ---------------------------------------------------------------------------

class GrpFile(object):
    """Flat uncompressed container: 12-byte magic, int32 count, 16-byte entries."""

    def __init__(self, path):
        self.data = open(path, "rb").read()
        if self.data[:12] != b"KenSilverman":
            fail("%s is not a KenSilverman GRP file" % path)
        count = u32(self.data, 12)
        self.entries = {}
        offset = 16 + count * 16
        for i in range(count):
            base = 16 + i * 16
            name = self.data[base:base + 12].split(b"\0")[0].decode("latin-1")
            size = u32(self.data, base + 12)
            self.entries[name.lower()] = (offset, size)
            offset += size

    def has(self, name):
        return name.lower() in self.entries

    def read(self, name):
        entry = self.entries.get(name.lower())
        if entry is None:
            return None
        return self.data[entry[0]:entry[0] + entry[1]]

    def names(self, suffix):
        return sorted(n for n in self.entries if n.endswith(suffix))


# ---------------------------------------------------------------------------
# ART tile library reader
# ---------------------------------------------------------------------------

class ArtLibrary(object):
    """All TILES*.ART files. Pixels are stored column-major per the ART spec."""

    def __init__(self, grp):
        self.width = {}
        self.height = {}
        self.picanm = {}
        self.pixels = {}
        for name in grp.names(".art"):
            data = grp.read(name)
            if data is None or len(data) < 16:
                continue
            start, end = u32(data, 8), u32(data, 12)
            count = end - start + 1
            pos = 16
            widths = [u16(data, pos + i * 2) for i in range(count)]
            pos += count * 2
            heights = [u16(data, pos + i * 2) for i in range(count)]
            pos += count * 2
            for i in range(count):
                tile = start + i
                self.width[tile] = widths[i]
                self.height[tile] = heights[i]
                self.picanm[tile] = u32(data, pos + i * 4)
            pos += count * 4
            for i in range(count):
                tile = start + i
                w, h = widths[i], heights[i]
                raw = data[pos:pos + w * h]
                pos += w * h
                # De-column: stored byte x*h + y becomes pixel (x, y).
                rows = bytearray(w * h)
                for x in range(w):
                    for y in range(h):
                        rows[y * w + x] = raw[x * h + y]
                self.pixels[tile] = bytes(rows)

    def tile(self, tile):
        """Row-major pixel bytes for a tile, or None when absent/empty."""
        w = self.width.get(tile)
        h = self.height.get(tile)
        if not w or not h or tile not in self.pixels:
            return None
        return self.pixels[tile]


# ---------------------------------------------------------------------------
# PALETTE.DAT / LOOKUP.DAT
# ---------------------------------------------------------------------------

def parse_palette(data):
    """Returns (rgb24[768], shade tables as list of 256-byte bytes)."""
    if data is None or len(data) < 768:
        fail("PALETTE.DAT missing or truncated")
    # VGA 0-63 component values scale to 24-bit with a factor of four.
    rgb24 = bytearray(768)
    for i in range(768):
        rgb24[i] = min(255, data[i] * 4)
    shades = []
    if len(data) >= 770:
        numshades = u16(data, 768)
        for i in range(numshades):
            table = data[770 + i * 256:770 + (i + 1) * 256]
            if len(table) < 256:
                break
            shades.append(bytes(table))
    return bytes(rgb24), shades


def parse_lookup(data):
    """LOOKUP.DAT: count byte, then {pal byte, 256 remap bytes} records."""
    remaps = {}
    if data and len(data) > 0:
        count = data[0]
        for i in range(count):
            base = 1 + i * 257
            if base + 257 > len(data):
                break
            remaps[data[base]] = data[base + 1:base + 257]
    return remaps


def shade_tables(shades):
    """Exactly 32 tables (ref_gl LoadPalettes reads a fixed 32*256 block)."""
    tables = []
    for i in range(32):
        tables.append(shades[i] if i < len(shades) else bytes(range(256)))
    return tables


# ---------------------------------------------------------------------------
# File writers (formats as consumed by this engine)
# ---------------------------------------------------------------------------

def write_wal(path, tile_index, width, height, pixels):
    """Duke tile WAL: 100-byte header then raw palette indices.

    ref_gl/dnrender.cpp TileImage only reads pixel bytes from offset 100 and
    requires length >= 100 + width*height; mip levels are never generated for
    it_duke textures, so none are stored.
    """
    header = bytearray(100)
    name = ("tile%04d" % tile_index).encode("ascii")
    header[:len(name)] = name
    struct.pack_into("<II", header, 32, width, height)
    with open(path, "wb") as handle:
        handle.write(header)
        handle.write(pixels)


def write_pcx(path, width, height, pixels, palette24):
    """8-bit single-plane RLE PCX matching qcommon/dnpcx.cpp DN_DecodePCX."""
    header = bytearray(128)
    header[0] = 10    # manufacturer
    header[1] = 5     # version (needed by DN_DecodePCX)
    header[2] = 1     # RLE encoding
    header[3] = 8     # bits per pixel
    struct.pack_into("<HHHH", header, 4, 0, 0, width - 1, height - 1)
    header[65] = 1    # one plane
    struct.pack_into("<H", header, 66, width)  # stride equals width
    out = bytearray(header)
    for y in range(height):
        row = pixels[y * width:(y + 1) * width]
        x = 0
        while x < width:
            value = row[x]
            run = 1
            while run < 63 and x + run < width and row[x + run] == value:
                run += 1
            if run > 1 or value >= 192:
                out.append(192 | run)
            out.append(value)
            x += run
    out.append(12)
    out.extend(palette24)
    with open(path, "wb") as handle:
        handle.write(out)


def write_wav(path, samples, rate):
    """8-bit unsigned mono PCM; sndwave.cpp accepts up to 192 kHz."""
    byte_rate = rate
    block_align = 1
    bits = 8
    data_size = len(samples)
    fmt = struct.pack("<HHIIHH", 1, 1, rate, byte_rate, block_align, bits)
    riff_size = 4 + 8 + len(fmt) + 8 + data_size
    with open(path, "wb") as handle:
        handle.write(b"RIFF")
        handle.write(struct.pack("<I", riff_size))
        handle.write(b"WAVE")
        handle.write(b"fmt ")
        handle.write(struct.pack("<I", len(fmt)))
        handle.write(fmt)
        handle.write(b"data")
        handle.write(struct.pack("<I", data_size))
        handle.write(samples)


def write_16to8(path, palette24):
    """Nearest palette index for each RGB565 value, indexed r | g<<5 | b<<11.

    The indexing matches ref_gl/gl_image.cpp GL_BuildPalettedTexture. Index
    255 is skipped because the engine treats it as transparent everywhere.
    """
    r8 = [(r << 3) | (r >> 2) for r in range(32)]
    g8 = [(g << 2) | (g >> 4) for g in range(64)]
    b8 = [(b << 3) | (b >> 2) for b in range(32)]
    pal_r = [palette24[i * 3] for i in range(255)]
    pal_g = [palette24[i * 3 + 1] for i in range(255)]
    pal_b = [palette24[i * 3 + 2] for i in range(255)]
    # Per-channel squared-distance tables keep the 65536x255 scan in bounds.
    dr = [[(r8[r] - pal_r[i]) ** 2 for i in range(255)] for r in range(32)]
    dg = [[(g8[g] - pal_g[i]) ** 2 for i in range(255)] for g in range(64)]
    db = [[(b8[b] - pal_b[i]) ** 2 for i in range(255)] for b in range(32)]
    table = bytearray(65536)
    for b in range(32):
        dbb = db[b]
        for g in range(64):
            dgg = dg[g]
            base = g << 5
            for r in range(32):
                drr = dr[r]
                best = 0
                best_dist = drr[0] + dgg[0] + dbb[0]
                for i in range(255):
                    dist = drr[i] + dgg[i] + dbb[i]
                    if dist < best_dist:
                        best_dist = dist
                        best = i
                table[base + r | (b << 11)] = best
    with open(path, "wb") as handle:
        handle.write(table)


# ---------------------------------------------------------------------------
# VOC to WAV conversion
# ---------------------------------------------------------------------------

def voc_to_wav(data):
    """Creative Voice File blocks to 8-bit PCM plus a sample rate."""
    if data is None or len(data) < 26 or data[:19] != b"Creative Voice File":
        return None, 0
    rate = 0
    samples = bytearray()
    pos = u16(data, 20)  # data offset field; almost always 26
    while pos + 4 <= len(data):
        block = data[pos]
        if block == 0:
            break
        size = data[pos + 1] | (data[pos + 2] << 8) | (data[pos + 3] << 16)
        body = data[pos + 4:pos + 4 + size]
        pos += 4 + size
        if block in (1, 8) and len(body) >= 2:
            if not rate and 256 - body[0] > 0:
                rate = 1000000 // (256 - body[0])
            samples.extend(body[2:])
        elif block == 9 and len(body) >= 4:
            if not rate:
                rate = u32(body, 0)
            samples.extend(body[4:])
        # Blocks 5 (silence), 6/7 (loop markers) do not carry samples.
    return bytes(samples), rate


# ---------------------------------------------------------------------------
# UI tile variants (dn/tile%04d_p%d_s%d.pcx)
# ---------------------------------------------------------------------------

def ui_tile_requests():
    """(tile, pal, shade) combinations drawn by the client, from the sources:

    client/dnmenu.cpp:    big-font glyphs p0 s0/s16, cursor 2813-2819 p0 s0,
                          background 2456 p0 s16
    client/dnsavemenu.cpp: header 2457 p0 s16, mini-font 3072.. p0 s0
    client/dnintermission.cpp: text glyphs 2822.. p0 s0/s16, panels
                          3240-3242/3245-3247 p0 s0
    client/dnhud.cpp:     status bar digits/icons listed below
    """
    requests = set()

    def add(tiles, pals, shades):
        for tile in tiles:
            for pal in pals:
                for shade in shades:
                    requests.add((tile, pal, shade))

    # Big menu alphabet: digits, A-Z and punctuation (dnmenu.cpp Glyph()).
    add([2929, 3002, 3003, 3004, 3005, 3006, 3007, 3022], [0], [0, 16])
    add(list(range(2930, 2966)), [0], [0, 16])
    add(list(range(2813, 2820)), [0], [0])  # menu cursor animation
    add([2456, 2457], [0], [16])            # menu and save-menu backgrounds
    # Intermission text alphabet ('!'..'~' mapped onto 2822+) and panels.
    add(list(range(2822, 2917)), [0], [0, 16])
    add([3240, 3241, 3242, 3245, 3246, 3247], [0], [0])
    # Mini font: p0 for save menus, pals 0..23 for deathmatch frag bar names
    # (dnhud.cpp FragBar receives the server player palette verbatim).
    add(list(range(3072, 3167)), range(24), [0])
    # Status bar (dnhud.cpp): big digits, keys, icons, ammo counters.
    add(list(range(2472, 2482)), [0], [0])
    add([2465, 2462], [0], [0, 4])
    add([2471], [0, 21, 23], [0])
    add([30, 31, 33], [21], [0])
    add([2460, 2461, 2463, 2467, 2468, 2469, 2470], [0], [0])
    # Weapon rows: shades 0/12/21 plus pal-7 labels with reduced shades.
    add(list(range(3010, 3020)), [0], [0, 12, 21])
    add(list(range(3010, 3020)), [7], [0, 2, 11])
    add([3020, 3021], [0], [0, 12, 21])
    return sorted(requests)


def bake_tile_pixels(art, tile, pal, shade, remaps, shades):
    """Same remap order as ref_gl TileImage: shades[s][remaps[p][c]]."""
    pixels = art.tile(tile)
    if pixels is None:
        return None
    remap = remaps.get(pal, bytes(range(256))) if pal else bytes(range(256))
    table = shades[shade]
    baked = bytearray(len(pixels))
    for i, c in enumerate(pixels):
        if c == 255:
            baked[i] = 255  # transparency index survives every bake
        else:
            baked[i] = table[remap[c]]
    return bytes(baked)


# ---------------------------------------------------------------------------
# Output helpers
# ---------------------------------------------------------------------------

def fresh_dir(path):
    """Create a directory the pipeline owns and clear stale files from it."""
    if os.path.isdir(path):
        for name in os.listdir(path):
            stale = os.path.join(path, name)
            if os.path.isfile(stale):
                os.remove(stale)
    else:
        os.makedirs(path)


def write_file(path, data):
    with open(path, "wb") as handle:
        handle.write(data)


def main():
    parser = argparse.ArgumentParser(description="Prepare basedn assets from duke3d.grp")
    parser.add_argument("--root", default=".", help="repository root containing 3drealms/")
    args = parser.parse_args()

    root = os.path.abspath(args.root)
    grp_path = os.path.join(root, "3drealms", "duke3d.grp")
    if not os.path.isfile(grp_path):
        fail("missing game data: %s" % grp_path)
    basedn = os.path.join(root, "basedn")
    os.makedirs(basedn, exist_ok=True)

    print("prepare_assets: reading %s" % grp_path)
    grp = GrpFile(grp_path)

    palette24, raw_shades = parse_palette(grp.read("palette.dat"))
    shades = shade_tables(raw_shades)
    remaps = parse_lookup(grp.read("lookup.dat"))
    art = ArtLibrary(grp)
    if not art.width:
        fail("no TILES*.ART libraries found in the group file")

    counts = {}

    # Raw maps: cmodel.cpp DN_LoadMap expects the untouched Build v7 bytes.
    maps_dir = os.path.join(basedn, "maps")
    os.makedirs(maps_dir, exist_ok=True)
    for name in grp.names(".map"):
        write_file(os.path.join(maps_dir, name), grp.read(name))
        counts["maps"] = counts.get("maps", 0) + 1

    # CON scripts: dncon.cpp loads them from scripts/ in lowercase.
    scripts_dir = os.path.join(basedn, "scripts")
    os.makedirs(scripts_dir, exist_ok=True)
    for name in grp.names(".con"):
        write_file(os.path.join(scripts_dir, name), grp.read(name))
        counts["scripts"] = counts.get("scripts", 0) + 1

    # Tile metadata: 6144 packed {int16 width, int16 height, uint32 picanm}.
    tiles = bytearray(MAX_TILES * 8)
    for tile in range(MAX_TILES):
        w = art.width.get(tile, 0)
        h = art.height.get(tile, 0)
        if w or h:
            struct.pack_into("<HHI", tiles, tile * 8, w, h, art.picanm.get(tile, 0))
    os.makedirs(os.path.join(basedn, "sprites"), exist_ok=True)
    write_file(os.path.join(basedn, "sprites", "tiles.dat"), tiles)

    # Build's TABLES.DAT verbatim: dnmath.cpp DN_MathInit reads sine and
    # radar arrays straight from it.
    tables = grp.read("tables.dat")
    if tables is None or len(tables) < 4096 + 1280:
        fail("TABLES.DAT missing or shorter than the sine/radar arrays")
    write_file(os.path.join(basedn, "sprites", "tables.dat"), tables)

    # Shade tables for the renderer: int16 count + 32*256 bytes.
    palookup = struct.pack("<H", len(shades)) + b"".join(shades)
    pics_dir = os.path.join(basedn, "pics")
    os.makedirs(pics_dir, exist_ok=True)
    write_file(os.path.join(pics_dir, "palookup.dat"), palookup)

    # LOOKUP.DAT verbatim: LoadPalettes reads its count-and-record layout.
    lookup = grp.read("lookup.dat")
    if lookup:
        write_file(os.path.join(pics_dir, "lookup.dat"), lookup)

    # Global palette: only the 768-byte trailer of colormap.pcx is used.
    write_pcx(os.path.join(pics_dir, "colormap.pcx"), 256, 1, bytes(range(256)), palette24)
    write_16to8(os.path.join(pics_dir, "16to8.dat"), palette24)

    # World textures: one raw-index WAL per non-empty tile.
    wal_dir = os.path.join(basedn, "textures", "duke")
    fresh_dir(wal_dir)
    for tile in sorted(art.width):
        pixels = art.tile(tile)
        if pixels is None:
            continue
        write_wal(os.path.join(wal_dir, "tile%04d.wal" % tile), tile,
                  art.width[tile], art.height[tile], pixels)
        counts["wal_textures"] = counts.get("wal_textures", 0) + 1

    # UI tiles with pal/shade baked into the pixel data.
    ui_dir = os.path.join(pics_dir, "dn")
    fresh_dir(ui_dir)
    for tile, pal, shade in ui_tile_requests():
        pixels = bake_tile_pixels(art, tile, pal, shade, remaps, shades)
        if pixels is None:
            continue
        write_pcx(os.path.join(ui_dir, "tile%04d_p%d_s%d.pcx" % (tile, pal, shade)),
                  art.width[tile], art.height[tile], pixels, palette24)
        counts["ui_tiles"] = counts.get("ui_tiles", 0) + 1

    # Sounds: every VOC becomes an 8-bit PCM WAV with its original rate.
    sound_dir = os.path.join(basedn, "sound")
    os.makedirs(sound_dir, exist_ok=True)
    skipped = []
    for name in grp.names(".voc"):
        samples, rate = voc_to_wav(grp.read(name))
        if not samples or not rate or rate > 192000:
            skipped.append(name)
            continue
        write_wav(os.path.join(sound_dir, os.path.splitext(name)[0] + ".wav"), samples, rate)
        counts["sounds"] = counts.get("sounds", 0) + 1

    # Intro sequence baked from tile artwork.
    intro_dir = os.path.join(basedn, "anm", "dnintro")
    intro_counts = dnintro_assets.write_intro(intro_dir, art, palette24, remaps, shades)
    for key, value in intro_counts.items():
        counts[key] = counts.get(key, 0) + value

    manifest = {
        "source": "3drealms/duke3d.grp",
        "tiles": len(art.width),
        "generated": counts,
        "notes": [
            "Music is not converted: the engine only plays music/<name>.mp3 "
            "and the group file holds MIDI tracks.",
            "DUKE.RTS taunt sounds are not read by the engine and are skipped.",
            "Deathmatch frag-bar names cover palettes 0..23; players reported "
            "with higher palette values simply draw no name tiles.",
        ],
        "skipped_voc": skipped,
    }
    write_file(os.path.join(basedn, "asset-manifest.json"),
               (json.dumps(manifest, indent=2) + "\n").encode("utf-8"))
    print("prepare_assets: done (%d maps, %d wal, %d ui tiles, %d sounds)" %
          (counts.get("maps", 0), counts.get("wal_textures", 0),
           counts.get("ui_tiles", 0), counts.get("sounds", 0)))


if __name__ == "__main__":
    main()

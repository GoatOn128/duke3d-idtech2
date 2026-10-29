#!/usr/bin/env python3
"""Bake the anm/dnintro assets client/dnintro.cpp plays at startup.

The engine expects (under basedn/anm/dnintro/):
  frames.txt        ASCII frame count, under 16 bytes
  logo%03d.tga      320x200 frames shown 75 ms each; flyby.wav at frame 1,
                    bombexpl.wav from frame 19
  realms.tga        320x200 still for the 3D Realms stage
  title.tga         320x200 still behind the animated title tiles
  tile%d.tga        title letter tiles 2497/2498/2502 with alpha, scaled by
                    the engine's TitleTile()
  fade%02d.tga      black overlays with alpha index*17 for the 300 ms fade

The original animation cannot be rebuilt from the group file alone, so the
logo stage is baked as the Duke title composition with a fade-in over the
first ten frames, keeping the flyby and bomb sound timing of the engine.
LoadTGA accepts uncompressed type-2 24/32-bit images in bottom-up row order.
"""

import struct

TITLE_BACKGROUND_TILE = 2456  # also the menu background (dnmenu.cpp)
TITLE_LETTER_TILES = (2497, 2498, 2502)
LOGO_FRAMES = 26               # ~2 s at 75 ms, bombexpl lands at frame 19
LOGO_FADE_FRAMES = 10
SCREEN_W, SCREEN_H = 320, 200


def write_tga32(path, width, height, pixels):
    """Type 2, 32-bit, bottom-up BGRA; pixels are (r, g, b, a) tuples."""
    header = bytearray(18)
    header[2] = 2                    # uncompressed true-color
    struct.pack_into("<HH", header, 12, width, height)
    header[16] = 32                  # bits per pixel
    header[17] = 8                   # 8 attribute (alpha) bits, bottom-up
    out = bytearray(header)
    for y in range(height - 1, -1, -1):
        for x in range(width):
            r, g, b, a = pixels[y * width + x]
            out += bytes((b, g, r, a))
    with open(path, "wb") as handle:
        handle.write(out)


def tile_rgba(art, tile, palette24, remaps, shades, pal=0, shade=0):
    """Tile as an RGBA buffer, or None when the tile is absent."""
    pixels = art.tile(tile)
    if pixels is None:
        return None
    remap = remaps.get(pal, bytes(range(256))) if pal else bytes(range(256))
    table = shades[shade]
    rgba = []
    for c in pixels:
        if c == 255:
            rgba.append((0, 0, 0, 0))
        else:
            index = table[remap[c]]
            base = index * 3
            rgba.append((palette24[base], palette24[base + 1], palette24[base + 2], 255))
    return rgba


def blit(dst, dst_w, src, src_w, src_h, x, y):
    """Alpha-composite an RGBA source onto an RGBA destination buffer."""
    for sy in range(src_h):
        dy = y + sy
        if dy < 0 or dy >= SCREEN_H:
            continue
        for sx in range(src_w):
            dx = x + sx
            if dx < 0 or dx >= dst_w:
                continue
            r, g, b, a = src[sy * src_w + sx]
            if a == 0:
                continue
            di = dy * dst_w + dx
            if a == 255:
                dst[di] = (r, g, b, 255)
            else:
                dr, dg, db, da = dst[di]
                dst[di] = ((r * a + dr * (255 - a)) // 255,
                           (g * a + dg * (255 - a)) // 255,
                           (b * a + db * (255 - a)) // 255,
                           max(da, a))


def title_letters(art, palette24, remaps, shades):
    """RGBA buffers for the three title tiles keyed by tile number."""
    return {tile: tile_rgba(art, tile, palette24, remaps, shades)
            for tile in TITLE_LETTER_TILES}


def letter_positions():
    """Anchor rows from client/dnintro.cpp TitleTile calls, centered on x."""
    return [(2497, 104), (2498, 129), (2502, 151)]


def compose_title_screen(art, palette24, remaps, shades):
    """Title background plus the three letters at their resting positions."""
    background = tile_rgba(art, TITLE_BACKGROUND_TILE, palette24, remaps, shades)
    screen = [(0, 0, 0, 255)] * (SCREEN_W * SCREEN_H)
    if background:
        blit(screen, SCREEN_W, background, art.width[TITLE_BACKGROUND_TILE],
             art.height[TITLE_BACKGROUND_TILE], 0, 0)
    letters = title_letters(art, palette24, remaps, shades)
    for tile, y in letter_positions():
        src = letters.get(tile)
        if not src:
            continue
        w, h = art.width[tile], art.height[tile]
        blit(screen, SCREEN_W, src, w, h, (SCREEN_W - w) // 2, y - h // 2)
    return screen, letters


def realms_text_glyphs():
    """Big-font tile numbers for the characters of the 3D Realms still."""
    glyphs = {}
    for i, ch in enumerate("0123456789"):
        glyphs[ch] = 2930 + i
    for i, ch in enumerate("ABCDEFGHIJKLMNOPQRSTUVWXYZ"):
        glyphs[ch] = 2940 + i
    return glyphs


def compose_realms(art, palette24, remaps, shades):
    glyphs = realms_text_glyphs()
    screen = [(0, 0, 0, 255)] * (SCREEN_W * SCREEN_H)
    text = "3D REALMS"
    width = 0
    for ch in text:
        tile = glyphs.get(ch)
        width += art.width.get(tile, 6) - 1 if tile else 6
    x = (SCREEN_W - width) // 2
    y = 92
    for ch in text:
        tile = glyphs.get(ch)
        if tile:
            src = tile_rgba(art, tile, palette24, remaps, shades)
            if src:
                blit(screen, SCREEN_W, src, art.width[tile], art.height[tile], x, y)
            x += art.width.get(tile, 6) - 1
        else:
            x += 6
    return screen


def write_intro(out_dir, art, palette24, remaps, shades):
    """Generate every anm/dnintro file; returns a counts dict for the manifest."""
    import os
    os.makedirs(out_dir, exist_ok=True)
    counts = {}
    screen, letters = compose_title_screen(art, palette24, remaps, shades)

    # Logo frames: the title composition fading in from black.
    for frame in range(1, LOGO_FADE_FRAMES + 1):
        alpha = frame * 255 // LOGO_FADE_FRAMES
        pixels = []
        for r, g, b, a in screen:
            pixels.append((r * alpha // 255, g * alpha // 255, b * alpha // 255, 255))
        write_tga32(os.path.join(out_dir, "logo%03d.tga" % frame), SCREEN_W, SCREEN_H, pixels)
    for frame in range(LOGO_FADE_FRAMES + 1, LOGO_FRAMES + 1):
        write_tga32(os.path.join(out_dir, "logo%03d.tga" % frame), SCREEN_W, SCREEN_H, screen)
    counts["intro_logo_frames"] = LOGO_FRAMES

    write_tga32(os.path.join(out_dir, "realms.tga"), SCREEN_W, SCREEN_H,
                compose_realms(art, palette24, remaps, shades))
    write_tga32(os.path.join(out_dir, "title.tga"), SCREEN_W, SCREEN_H, screen)
    counts["intro_stills"] = 2

    # Title letter tiles, drawn scaled by the engine with their own alpha.
    for tile, src in letters.items():
        if src:
            write_tga32(os.path.join(out_dir, "tile%d.tga" % tile),
                        art.width[tile], art.height[tile], src)
            counts["intro_letters"] = counts.get("intro_letters", 0) + 1

    # Fade overlays: fade00 is fully transparent, fade15 fully black.
    for i in range(16):
        pixels = [(0, 0, 0, i * 17)] * (SCREEN_W * SCREEN_H)
        write_tga32(os.path.join(out_dir, "fade%02d.tga" % i), SCREEN_W, SCREEN_H, pixels)
    counts["intro_fades"] = 16

    with open(os.path.join(out_dir, "frames.txt"), "w") as handle:
        handle.write("%d\n" % LOGO_FRAMES)
    return counts

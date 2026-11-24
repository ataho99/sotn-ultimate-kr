#!/usr/bin/env python3
import os
import struct
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ptbr_glyphs as G  # noqa: E402
try:
    import ptbr_text  # noqa: E402
except ImportError:
    ptbr_text = None

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../.."))
HDR = os.path.join(ROOT, "mods/sotn-ultimate/include/mod_ptbr_build.h")
BUS = os.path.join(ROOT, "build/us")
MON2 = os.path.join(BUS, "MONSTER2.BIN")
BIOS = "/mnt/c/Users/renan/Downloads/games/PS1/SCPH1001.BIN"
FONT_OFF = 0x30000
MAGIC = 0x31425450        # "PTB1"
BLOB_MAX = 0x7800         # InitHeap area 0x801F0000-0x801F7800
CHUNK = 0x2000            # one demo-key load (4 sectors)
DEMOKEY = 0x800ACA88
EMPTY = ("// Written by mods/sotn-ultimate/tools/build_disc.sh: MOD_PTBR=1 builds the PT-BR side\n"
         "// project disc (never a main release / RC); otherwise this stays empty.\n")


def write_if_changed(path, text):
    if os.path.exists(path) and open(path).read() == text:
        return
    open(path, "w").write(text)


HOOK_OFF = 0x10    # the Krom2RawAdd routine, right after the header
HOOK_MAX = 0x80    # its room; the compiled code follows
CODE_OFF = HOOK_OFF + HOOK_MAX
BLOB_ADDR = 0x801F0000
KCODE0, K2CODE0 = 0x8540, 0x824F
CODE_SRC = os.path.join(ROOT, "mods/sotn-ultimate/src/ptbr/ptbr_blob.c")
SEL_ADDR = 0x801E8000
SEL_BLOCK = TTL_BLOCK = b""  # the menu block (id 0), the title block (id 1)
BLOB_USED = 0


def blob_code():
    import tempfile
    import ptbr_sel
    ptbr_sel.header()
    with tempfile.TemporaryDirectory() as d:
        o, e, b, ld = (os.path.join(d, n) for n in ("c.o", "c.elf", "c.bin", "c.ld"))
        open(ld, "w").write(
            "SECTIONS {\n . = 0x%08X;\n .text : { *(.entry) *(.text*) *(.rodata*)"
            " *(.data*) *(.sdata*) *(.sbss*) *(.bss*) *(COMMON) }\n"
            " OVERLAY 0x%08X : {\n"
            "  .sel { *(.sel.hdr) *(.sel.text) *(.sel.rodata) }\n"
            "  .ttl { *(.ttl.hdr) *(.ttl.text) *(.ttl.rodata) *(.ttl.data) }\n }\n"
            " /DISCARD/ : { *(.reginfo) *(.MIPS.*) *(.pdr) *(.comment) *(.gnu.*)"
            " *(.note*) }\n}\n" % (BLOB_ADDR + CODE_OFF, SEL_ADDR))
        subprocess.run(["mipsel-linux-gnu-gcc", "-march=r3000", "-mabi=32", "-msoft-float",
                        "-mno-abicalls", "-fno-pic", "-G0", "-O2", "-ffreestanding",
                        "-fno-builtin", "-fno-tree-loop-distribute-patterns",
                        "-nostdlib", "-I", os.path.join(ROOT, "mods/sotn-ultimate/include"),
                        "-DMOD_PTBR_DEMOKEY=0x%X" % DEMOKEY,
                        "-c", CODE_SRC, "-o", o], check=True)
        subprocess.run(["mipsel-linux-gnu-ld", "-T", ld, "-o", e, o], check=True)
        subprocess.run(["mipsel-linux-gnu-objcopy", "-O", "binary", "-j", ".text", e, b],
                       check=True)
        code = open(b, "rb").read()
        subprocess.run(["mipsel-linux-gnu-objcopy", "-O", "binary", "-j", ".sel", e, b],
                       check=True)
        sel = open(b, "rb").read()
        subprocess.run(["mipsel-linux-gnu-objcopy", "-O", "binary", "-j", ".ttl", e, b],
                       check=True)
        ttl = open(b, "rb").read()
    global SEL_BLOCK, TTL_BLOCK
    assert len(sel) <= CHUNK, "PT-BR menu block %d bytes > %d" % (len(sel), CHUNK)
    assert len(ttl) <= CHUNK, "PT-BR title block %d bytes > %d" % (len(ttl), CHUNK)
    SEL_BLOCK = sel + b"\0" * (CHUNK - len(sel))
    TTL_BLOCK = ttl + b"\0" * (CHUNK - len(ttl))
    return code + b"\0" * (-len(code) % 4)


def krom_hook(n1, n2, glyphs):
    src = """
    .set noat
    .text
hook:
    lui   $8, %(blob_hi)d
    lw    $9, 4($8)
    lui   $10, %(magic_hi)d
    ori   $10, $10, %(magic_lo)d
    bne   $9, $10, 2f
    andi  $11, $4, 0xFFFF
    ori   $15, $0, %(k1)d
    subu  $12, $11, $15
    sltiu $13, $12, %(n1)d
    bnez  $13, 1f
    ori   $15, $0, %(k2)d
    subu  $12, $11, $15
    sltiu $13, $12, %(n2)d
    beqz  $13, 2f
    addiu $12, $12, %(n1)d
1:  sll   $13, $12, 5
    sll   $14, $12, 1
    subu  $12, $13, $14
    addu  $2, $8, $12
    addiu $2, $2, %(glyphs)d
    jr    $31
2:  addiu $10, $0, 0xB0
    addiu $9, $0, 0x51
    jr    $10
""" % {"blob_hi": BLOB_ADDR >> 16, "magic_hi": MAGIC >> 16,
       "magic_lo": MAGIC & 0xFFFF, "k1": KCODE0, "k2": K2CODE0,
       "n1": n1, "n2": n2, "glyphs": glyphs}
    import tempfile
    with tempfile.TemporaryDirectory() as d:
        s, o, b = (os.path.join(d, n) for n in ("h.s", "h.o", "h.bin"))
        open(s, "w").write(src)
        subprocess.run(["mipsel-linux-gnu-as", "-march=r3000", "-EL", "-o", o, s],
                       check=True)
        subprocess.run(["mipsel-linux-gnu-objcopy", "-O", "binary", "-j", ".text",
                        o, b], check=True)
        code = open(b, "rb").read()
    assert len(code) <= HOOK_MAX, "Krom2RawAdd hook %d bytes > %d" % (len(code), HOOK_MAX)
    return code + b"\0" * (HOOK_MAX - len(code))


def blob(strict=True):
    """Magic, done word (0: SEL sets it), length, text offset; the
    Krom2RawAdd hook (HOOK_OFF), the compiled code (CODE_OFF), the
    description font's glyphs (the accented letters, then the shorter
    digits / capitals), then the text section."""
    import ptbr_sel
    ptbr_sel.STRICT = strict
    rom = open(BIOS, "rb").read()

    def raw16(ch):
        """A Kanji ROM full-width letter as 15 rows of 16 "#"/"." (ROM
        index: full-width A-Z from 157, a-z from 183; Krom2RawAdd)."""
        idx = (157 + ord(ch) - ord("A") if ch.isupper() else
               183 + ord(ch) - ord("a"))
        o = 0x66000 + idx * 30
        return ["".join("#" if ((rom[o + 2 * r] << 8 | rom[o + 2 * r + 1])
                                >> (15 - b)) & 1 else "." for b in range(16))
                for r in range(15)]

    def rom16(code):
        # digits from 147 (0x824F), capitals from 157 (0x8260)
        idx = 147 + code - 0x824F if code < 0x8260 else 157 + code - 0x8260
        o = 0x66000 + idx * 30
        return ["".join("#" if ((rom[o + 2 * r] << 8 | rom[o + 2 * r + 1])
                                >> (15 - b)) & 1 else "." for b in range(16))
                for r in range(15)]

    out = bytearray()
    for rows in [G.krom(raw16, ch) for ch in G.ORDER] + G.krom2(rom16):
        for r in rows:
            v = int(r.replace("#", "1").replace(".", "0"), 2)
            out += bytes([v >> 8, v & 0xFF])
    assert len(out) == (len(G.ORDER) + G.KROM2_COUNT) * 30
    code = blob_code()
    glyphs = CODE_OFF + len(code)
    out = krom_hook(len(G.ORDER), G.KROM2_COUNT, glyphs) + code + out
    out += b"\0" * (-len(out) % 4)
    text_off = 0x10 + len(out)
    body = bytes(out) + ptbr_text.section(text_off)
    global BLOB_USED
    BLOB_USED = 0x10 + len(body)
    data = struct.pack("<IIII", MAGIC, 0, 0, text_off) + body
    bs = os.path.join(ROOT, "build/us/ptbr_script/blob_scripts.bin")
    if os.path.exists(bs):
        raw = open(bs, "rb").read()
        at = struct.unpack("<I", raw[:4])[0] - BLOB_ADDR
        if raw[4:]:
            assert len(data) <= at, "PT-BR blob %d bytes runs into its cutscene copies at +0x%X" % (
                len(data), at)
            data = data + b"\0" * (at - len(data)) + raw[4:]
    assert len(data) <= BLOB_MAX, "PT-BR blob %d bytes > %d" % (len(data), BLOB_MAX)
    return data


def chunks(n):
    return (n + CHUNK - 1) // CHUNK


def header():
    if os.environ.get("MOD_PTBR") != "1":
        write_if_changed(HDR, EMPTY)
        for f in ("F_GAME.BIN", "F_TITLE1.BIN", "F_SEL.BIN", "F_SELPT.BIN"):
            p = os.path.join(BUS, f)
            if os.path.exists(p):
                os.remove(p)
                print("removed PT-BR copy", p)
        try:
            import ptbr_vo
        except ImportError:
            return
        ptbr_vo.strip()
        return
    # the disc's own takes of the Maria fight's lines: rows after XA_STR1.
    # (repack_xa.py ran just before) and their table
    import ptbr_vo
    vo = ptbr_vo.append()
    # MONSTER2.BIN's own size (the edition decides record 19): make it now,
    # build_disc.sh makes it again later the same way (Python for the Ghost
    # Skull edition, else the builder's monster2.ts)
    if ptbr_text.ghost_edition():
        cmd = [sys.executable, "mods/sotn-ultimate/tools/build_monster2.py"]
    else:
        cmd = [os.environ.get("NODE", "node"),
               "mods/sotn-ultimate/tools/builder/node/monster2.ts"]
    subprocess.run(cmd, check=True, cwd=ROOT, stdout=subprocess.DEVNULL)
    sector = os.path.getsize(MON2) // 2048
    write_if_changed(HDR, EMPTY.replace("otherwise this stays empty.",
                                        "this is that build.") +
                     "#define MOD_PTBR\n"
                     "#define MOD_PTBR_SECTOR %d // blob in MONSTER2.BIN\n"
                     "#define MOD_PTBR_CHUNKS %d // 8 KB demo-key loads\n"
                     "#define MOD_PTBR_RESIDENT %d // the first ones: the blob\n"
                     "#define MOD_PTBR_DEMOKEY 0x%X // g_LbaBinDemokey's loc\n"
                     % (sector, chunks(len(blob(strict=False))) + 5,
                        chunks(len(blob(strict=False))), DEMOKEY) + vo)
    print("mod_ptbr_build.h: MONSTER2 sector %d" % sector)
    ptbr_text.lib_header(os.path.join(ROOT, "src/st/lib/mod_ptbr_lib.h"))
    ptbr_text.lib_text_header(os.path.join(ROOT, "src/st/lib/mod_ptbr_lib_text.h"))
    # the Extra settings popup's labels, compiled into ARC_F2.BIN
    ptbr_text.sysopt_header(os.path.join(ROOT, "mods/sotn-ultimate/src/menu/mod_ptbr_sysopt.h"))
    # Richter's / the Marias' pause menu texts, compiled into its blob
    ptbr_text.charmenu_header(os.path.join(ROOT, "mods/sotn-ultimate/src/menu/mod_ptbr_charmenu.h"))
    import ptbr_names
    ptbr_names.header(os.path.join(ROOT, "mods/sotn-ultimate/include/mod_ptbr_names.h"))


def font_cells(data):
    """The accented letters into one copy of the 8x8 sheet."""
    data = bytearray(data)

    def base(ch):
        c = ord(ch) - 0x20
        rows = []
        for row in range(8):
            o = FONT_OFF + ((c >> 4) * 8 + row) * 64 + (c & 0xF) * 4
            s = ""
            for b in data[o:o + 4]:
                s += "".join("." if v == 0 else "%x" % v for v in (b & 0xF, b >> 4))
            rows.append(s)
        return rows

    for i, ch in enumerate(G.ORDER):
        c = G.cell(i)
        rows = G.font8(base, ch)
        for row, s in enumerate(rows):
            o = FONT_OFF + ((c >> 4) * 8 + row) * 64 + (c & 0xF) * 4
            px = [0 if p == "." else int(p, 16) for p in s]
            data[o:o + 4] = bytes(px[k] | px[k + 1] << 4 for k in range(0, 8, 2))
    return bytes(data)


def apply():
    if os.environ.get("MOD_PTBR") != "1":
        return
    # 8x8 sheet: all three files that carry it
    for f, src in (("F_GAME.BIN", "disks/us/BIN/F_GAME.BIN"),
                   ("F_TITLE1.BIN", "disks/us/BIN/F_TITLE1.BIN"),
                   ("F_GAME2.BIN", "build/us/F_GAME2.BIN")):
        d = font_cells(open(os.path.join(ROOT, src), "rb").read())
        if f == "F_TITLE1.BIN":
            import ptbr_logo
            d = ptbr_logo.f_title1(d, open(os.path.join(ROOT, "disks/us/BIN/F_TITLE0.BIN"),
                                           "rb").read())
        open(os.path.join(BUS, f), "wb").write(d)
    import ptbr_sel_art
    d = open(os.path.join(ROOT, "disks/us/ST/SEL/F_SEL.BIN"), "rb").read()
    sel_art = ptbr_sel_art.apply(d)
    assert len(sel_art) == len(d) and len(d) % 2048 == 0
    for f in ("F_SEL.BIN", "F_SELPT.BIN"):
        if os.path.exists(os.path.join(BUS, f)):
            os.remove(os.path.join(BUS, f))
    # the blob, sector-aligned after MONSTER2's own records
    m = open(MON2, "rb").read()
    assert len(m) % 2048 == 0
    base = len(m) // 2048
    want = int([l for l in open(HDR) if "MOD_PTBR_SECTOR" in l][0].split()[2])
    if base != want:
        sys.exit("FAIL MONSTER2.BIN is %d sectors, SEL was built for %d: "
                 "build again" % (base, want))
    b = blob()
    b += b"\0" * (chunks(len(b)) * CHUNK - len(b))
    sel_used = len(SEL_BLOCK.rstrip(b"\0"))
    b += SEL_BLOCK  # block 0: the menus'
    b += TTL_BLOCK  # block 1: the title's (the language screen)
    import ptbr_names
    nb, _ = ptbr_names.block()
    p = os.path.join(ROOT, "build/us/ptbr_script/names_tail.bin")
    t = open(p, "rb").read() if os.path.exists(p) else b""
    if t:
        at = int.from_bytes(t[:4], "little") - 0x801E8000
        assert len(nb) <= at and at + len(t) - 4 <= CHUNK, (len(nb), at, len(t))
        nb = nb + b"\0" * (at - len(nb)) + t[4:]
    b += nb + b"\0" * (CHUNK - len(nb))
    e = open(os.path.join(ROOT, "build/us/ptbr_script/end.bin"), "rb").read()
    assert len(e) <= CHUNK, len(e)
    b += e + b"\0" * (CHUNK - len(e))
    p = os.path.join(ROOT, "build/us/ptbr_script/end_en.bin")
    e = open(p, "rb").read() if os.path.exists(p) else b""
    assert len(e) <= CHUNK, len(e)
    b += e + b"\0" * (CHUNK - len(e))
    assert len(b) == int([l for l in open(HDR) if "MOD_PTBR_CHUNKS" in l][0].split()[2]) * CHUNK
    import ptbr_logo
    t0 = open(os.path.join(ROOT, "disks/us/BIN/F_TITLE0.BIN"), "rb").read()
    fg = b""
    for f in ("F_GAME.BIN", "F_GAME2.BIN"):
        g = ptbr_logo.f_game(open(os.path.join(BUS, f), "rb").read(), t0)
        assert len(g) == 132 * 2048, (f, len(g))
        fg += g
    open(MON2, "wb").write(m + b + sel_art + fg)
    nm = subprocess.run(["mipsel-linux-gnu-nm", os.path.join(BUS, "dra.elf")],
                        capture_output=True, text=True).stdout
    addr = [l.split()[0] for l in nm.splitlines() if l.endswith(" g_LbaBinDemokey")]
    if not addr or int(addr[0], 16) != DEMOKEY:
        sys.exit("FAIL g_LbaBinDemokey moved (%s): fix DEMOKEY in build_ptbr.py" % addr)
    syms = {l.split()[2]: int(l.split()[0], 16) for l in nm.splitlines() if len(l.split()) == 3}
    for name, want in (("g_CdStep", 0x8006C398), ("g_LoadFile", 0x8006BAFC),
                       ("g_LoadOvlIdx", 0x80097918)):
        if syms.get(name) != want:
            sys.exit("FAIL %s moved: fix ptbr_blob.c's block loader" % name)
    print("PT-BR: fonts F_GAME/F_TITLE1/F_GAME2, menu art in MONSTER2, blob %d bytes (%d used of "
          "%d; title block %d of %d) at MONSTER2 sector %d" % (
              len(b), BLOB_USED, BLOB_MAX, sel_used,
              CHUNK, base))


if __name__ == "__main__":
    {"--header": header, "--apply": apply}[sys.argv[1]]()

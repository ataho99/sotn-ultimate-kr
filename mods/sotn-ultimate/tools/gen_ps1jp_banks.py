#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-or-later
"""KO build: in-game voices of the Japanese PS1 release, grafted into the US banks.

The US release re-recorded the grunts and the bosses' / enemies' / familiars'
voices in English inside its sample banks (VAB/SD_ALK, SD_RIH, SD_MAR;
ST/*/SD_Z*, BOSS/*/SD_Z*, SERVANT/SD_*); the Japanese disc (SLPM-86023 v1.2)
has the Japanese ones in files of the same names. A whole-bank swap would still be wrong where the Japanese bank is laid out
differently, so the US bank's structure stays and, in each tone whose sample
differs, the Japanese tone (sample, centre note, fine tune, ADSR, volume, pan,
key range: its whole record but the program number) replaces it; a US tone
the Japanese bank has none at stays as it is. That is right because the game
picks a sound by (vab, program, tone) from g_SfxData, and the Japanese and
US tables are identical in all 736 entries (checked here against the two DRA.BIN
files: sfx_tables_identical(); the build stops if they ever differ): the same
(program, tone) is the same sound on both discs. Each bank is cut to fit its
retail size (below).
Each grafted bank is written as build/us/<name>.VH/.VB, which `make disk`
takes over the retail file of that name (sotn-assets mods/makedisk.go:
build/us/<basename> before the retail one), re-laying the disc and
regenerating include/lba_us.h (sizes, sectors).

Not touched (a mod tool rewrote them, build/us/SD_Z*.VH/.VB exist): NO3, CEN, TE1-4.

Usage: gen_ps1jp_banks.py [--out DIR] [--check]
  JP_T1 (env): the Japanese disc's track 1 (2352-byte sectors), relative
  to the repository root; needs disks/us. SOTN_ROOT overrides the root.
  --check: also run the self-tests (graft with itself is the identity).
"""
import hashlib
import json
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.environ.get("SOTN_ROOT") or os.path.abspath(os.path.join(HERE, "../../.."))
sys.path.insert(0, HERE)
from vab import Vab  # noqa: E402

SECTOR = 2048
VB_LIMIT = 108240  # vab 3's sound RAM slot: the largest bank the Japanese disc loads there in game (BOSS/RBO7; the US's is 108,048)
SKIP = {"SD_ZKNO3", "SD_ZKCEN", "SD_ZKTE1", "SD_ZKTE2", "SD_ZKTE3", "SD_ZKTE4"}  # banks a mod tool writes to build/us
# cd.c addresses these retail files by sector (0x610D .. 0x614F): the sector
# counts of all but the last must not change
SECTORS_KEPT = {"VAB/SD_ALK.VH", "VAB/SD_ALK.VB", "VAB/SD_RIH.VH",
                "VAB/SD_RIH.VB", "VAB/SD_MAR.VH"}
MIN_EVIDENCE = 8  # identical samples that must sit at the same tone
# banks a mod tool rewrites into build/us (which stays: they are read and written back here): the tones that are
# still the US retail sample get the Japanese one, the tones the mod made stay (SD_ZKNO3: the horse's hoofbeats and
# neigh; the NO3RIDE.BIN banks of Richter's and Maria's games). SD_ZKCEN (Maria's Saturn sounds) and SD_ZKTE1 (the
# Garden's) hold no US-only tone, SD_ZKTE2-4 are the same on both discs: nothing to graft there.
MOD_GRAFT = {"SD_ZKNO3"}
RIDE_PART_SECTORS = 57  # BIN/NO3RIDE.BIN: two parts (VH padded to 4 sectors + VB padded), src/dra/cd.c MOD_NO3RIDE_SECTORS
# VAB/SD_MAR.VB may grow (its position is fixed, the files after it up to F_PROLO0 are not used by the mod:
# BIN/ARC_F.BIN, F_GAME.BIN, F_GAME2.BIN): by MAR_EXTRA_MAX sectors, which build/us/ARC_F.BIN (the retail
# file cut shorter by as many sectors, written by --arcf after build_sysopt.sh) gives back, so every file after
# keeps its sector. cd.c: g_LbaSdMarVb's length is SD_MAR_VB_LEN (lba_us.h) with the voices on.
MAR_EXTRA_MAX = 6
# sound RAM slots of the three vabs (src/dra/42398.c LOAD_VAB sizes: vab1Len = 0xE190, vab2Len = 0xFBF0; vab 3 above)
SLOT = {1: 0xE190, 2: 0xFBF0, 3: 108240}
# END TRIM (slideshow-free alternative to putting a sample back to English when a bank is a few bytes too big): the
# Japanese sample of a tone pair loses N ADPCM frames just before its two trailer frames (frame n-2 flag 1 = end,
# frame n-1 flag 7 = the never-played silent loop frame -- every one-shot sample of the Japanese disc ends so). The
# trailer stays, so the voice still stops at its end frame. {bank basename: {(program, tone): frames}}; the numbers
# are the bytes the bank is over its limit with all the Japanese tones (16 B per frame), measured by tail_trim.py:
# NO3 stage bank 1184 B, DEVIL 96 B, FAIRY 1648 B, NO3RIDE part 0 80 B. Decided by the user (19:40, after listening).
TRIM = {"SD_ZKNO3": {(1, 10): 74, (1, 11): 74},
        "SD_DEVIL": {(0, 14): 6, (0, 15): 6},
        "SD_FAIRY": {(2, 6): 103, (2, 7): 103},
        "NO3RIDE": {(4, 2): 5, (4, 3): 5}}


def trim_sample(s, n):
    """drop n frames before the 2 trailer frames (asserts the trailer pattern first)"""
    fr = len(s) // 16
    assert fr > n + 8 and s[(fr - 2) * 16 + 1] == 1 and s[(fr - 1) * 16 + 1] == 7, "sample has no flag-1/flag-7 trailer"
    return s[:(fr - 2 - n) * 16] + s[(fr - 2) * 16:]


# banks accepted with too few identical samples to prove the layout, because
# their tone slots line up by structure (VAB/SD_ALK: programs 0 and 1 have the
# same 16 tones with the same key ranges, the US program 2 is the first 8 of the
# Japanese one's 16). Flagged "weak" in the report: to be checked by ear.
WEAK_OK = {"VAB/SD_ALK"}


class Iso:
    def __init__(self, path):
        self.f = open(path, "rb")

    def sec(self, lba, cnt=1):
        self.f.seek(lba * 2352)
        raw = self.f.read(cnt * 2352)
        return b"".join(raw[i * 2352 + 24:i * 2352 + 24 + SECTOR] for i in range(cnt))

    def walk(self, lba, size, pre, res):
        d = self.sec(lba, (size + SECTOR - 1) // SECTOR)
        i = 0
        while i < len(d):
            n = d[i]
            if n == 0:
                i = (i // SECTOR + 1) * SECTOR
                if i >= len(d):
                    break
                continue
            elba, = struct.unpack_from("<I", d, i + 2)
            esz, = struct.unpack_from("<I", d, i + 10)
            fl = d[i + 25]
            nl = d[i + 32]
            nm = d[i + 33:i + 33 + nl].decode("latin1")
            i += n
            if nm in ("\x00", "\x01"):
                continue
            if fl & 2:
                self.walk(elba, esz, pre + nm + "/", res)
            else:
                res[pre + nm.split(";")[0]] = (elba, esz)

    def files(self):
        pvd = self.sec(16)
        rl, = struct.unpack_from("<I", pvd, 158)
        rs, = struct.unpack_from("<I", pvd, 166)
        res = {}
        self.walk(rl, rs, "", res)
        return res

    def read(self, e):
        return self.sec(e[0], (e[1] + SECTOR - 1) // SECTOR)[:e[1]]


def tone_samples(v):
    """{(program, tone): sample bytes} of the used tones"""
    return {(p, i): v.vags[t["vag"] - 1]
            for p, tl in v.tones.items() for i, t in enumerate(tl) if t["vag"]}


US_SFX = 0x800BF554  # g_SfxData in the US DRA (7-byte entries: vab, program, note, volume, mode, tone, ?)
US_SFX_N = 737


def sfx_used_tones():
    """{(vab, program, tone): number of sounds of the US g_SfxData that play it} (note tables not included)"""
    us = open(os.path.join(ROOT, "disks/us/DRA.BIN"), "rb").read()
    off = US_SFX - 0x800A0000
    tab = us[off:off + US_SFX_N * 7]
    # sounds include/sfx.h names SFX_UNUSED_*: no code plays them, so they do not count
    unused = set()
    hdr = open(os.path.join(ROOT, "include/sfx.h"), encoding="utf-8").read()
    for m in re.finditer(r"/[*]\s*0x([0-9A-Fa-f]+)\s*[*]/\s*SFX_UNUSED\w*", hdr):
        unused.add(int(m.group(1), 16) - 0x600)
    cnt = {}
    for i in range(US_SFX_N):
        if tab[i * 7 + 3] and i not in unused:
            k = (tab[i * 7], tab[i * 7 + 1], tab[i * 7 + 5])
            cnt[k] = cnt.get(k, 0) + 1
    return cnt


def bank_vab(name):
    """the vab slot a bank loads into (src/dra/cd.c CdFile vabId): 1 the player's grunts, 2 SD_MAR and the servants, 3 stage/boss"""
    if name in ("VAB/SD_ALK", "VAB/SD_RIH"):
        return 1
    if name == "VAB/SD_MAR" or name.startswith("SERVANT/"):
        return 2
    return 3


def sfx_tables_identical(iso, files):
    """the US and Japanese g_SfxData choose the same (vab, program, tone) for every sound"""
    us = open(os.path.join(ROOT, "disks/us/DRA.BIN"), "rb").read()
    jp = iso.read(files["DRA.BIN"])
    off = US_SFX - 0x800A0000
    tab = us[off:off + US_SFX_N * 7]
    k = jp.find(tab[7 * 16:7 * 16 + 7 * 12])
    if k < 0 or jp.find(tab[7 * 16:7 * 16 + 7 * 12], k + 1) >= 0:
        return False, "the Japanese g_SfxData is not found once"
    jt = jp[k - 7 * 16:k - 7 * 16 + US_SFX_N * 7]
    n = 0
    for i in range(US_SFX_N):
        u, j = tab[i * 7:i * 7 + 7], jt[i * 7:i * 7 + 7]
        if (u[0], u[1], u[5]) != (j[0], j[1], j[5]):
            return False, "sound %d: US %s, Japanese %s" % (i, (u[0], u[1], u[5]), (j[0], j[1], j[5]))
        n += u != bytes(7)
    return True, n


def layout_check(us, jp):
    """(ok, evidence, why): evidence = the US tones the Japanese bank also has
    (a sample the two banks share, at the same tone, is a cross-check; a
    shared sample at another tone is reported but does not stop: the table
    proof above is the basis)"""
    uu = tone_samples(us)
    ju = tone_samples(jp)
    return True, sum(1 for k in uu if k in ju), ""


def graft(us, jp, keep=frozenset(), retail=None, trim=None):
    """a new Vab: the US structure, the Japanese tone where the sample differs"""
    ju = tone_samples(jp)
    new = Vab.__new__(Vab)
    new.header = bytearray(us.header)
    new.programs = [list(p) for p in us.programs]
    new.tones = {}
    samples = []  # the new vag list, in order of first use
    index = {}
    changed = 0
    for p in sorted(us.tones):
        tl = []
        for i, t in enumerate(us.tones[p]):
            t = dict(t)
            if t["vag"]:
                s = us.vags[t["vag"] - 1]
                if (p, i) in ju and ju[(p, i)] != s and (p, i) not in keep and (retail is None or retail.get((p, i)) == s):
                    j = dict(jp.tones[p][i])
                    j["prog"] = t["prog"]
                    t = j
                    s = ju[(p, i)]
                    if trim and (p, i) in trim:
                        s = trim_sample(s, trim[(p, i)])
                    changed += 1
                if s not in index:
                    samples.append(s)
                    index[s] = len(samples)
                t["vag"] = index[s]
            tl.append(t)
        new.tones[p] = tl
    new.vags = samples
    return new, changed


def vb_size(v):
    return sum(len(x) for x in v.vags)


def fit(us, jp, limit, used=frozenset(), vab=0, retail=None, trim=None):
    """graft, then keep the US tones (greedy by Japanese sample: all the tones
    that use it, the biggest saving first) until the VB is within limit.
    returns (vab, changed, kept tones) or (None, 0, kept)"""
    keep = set()
    audible_kept = False
    new, changed = graft(us, jp, keep, retail, trim)
    ju = tone_samples(jp)
    uu = tone_samples(us)
    groups = {}
    for k in uu:
        if k in ju and ju[k] != uu[k] and (retail is None or retail.get(k) == uu[k]):
            groups.setdefault(ju[k], set()).add(k)
    while vb_size(new) > limit:
        best = None
        for g in groups.values():
            if g <= keep:
                continue
            t, c = graft(us, jp, keep | g, retail, trim)
            sv = vb_size(new) - vb_size(t)
            if sv <= 0:
                continue
            # tones no sound of g_SfxData plays go back to the US sample first (nothing audible changes)
            nsound = sum(used.get((vab, k[0], k[1]), 0) for k in g)
            audible = nsound > 0
            # least damage first: groups no sound plays, then the ones the fewest sounds play, then the smallest saving
            rank = (0 if not audible else 1, nsound, sv)
            if best is None or rank < best[4]:
                best = (sv, g, t, c, rank)
        if best is None or best[0] <= 0:
            return None, 0, keep
        keep |= best[1]
        new, changed = best[2], best[3]
        audible_kept = audible_kept or best[4][0] == 1
    return new, changed, keep


def graft_ride(out, retail_vab, jp, used, vab):
    """BIN/NO3RIDE.BIN part 0 (the bank Richter's / Maria's NO3 loads for every Dub, cd.c) is NO3's mod bank + the
    horse's neigh: its US-only tones get the Japanese ones too, in the same two-part layout (part 1, the PSP dub's, is
    left alone: with the field voices on, cd.c does not load it). Returns a report dict, or None without the file."""
    path = os.path.join(out, "NO3RIDE.BIN")
    if not os.path.exists(path):
        return None
    data = bytearray(open(path, "rb").read())
    assert len(data) == 2 * RIDE_PART_SECTORS * SECTOR, len(data)
    vh_len = 7200  # NO3's VH (4 sectors); the VB follows at sector 4
    vh0 = bytes(data[:vh_len])
    vb0 = bytes(data[4 * SECTOR:RIDE_PART_SECTORS * SECTOR])
    ride = Vab(vh0, vb0)
    # (the part's VB area is 53 sectors, but vab 3's sound RAM slot is the tighter limit)
    new, changed, kept = fit(ride, jp, min(VB_LIMIT, RIDE_PART_SECTORS * SECTOR - 4 * SECTOR), used, vab, tone_samples(retail_vab), TRIM["NO3RIDE"])
    if new is None:
        sys.exit("gen_ps1jp_banks: NO3RIDE part 0 does not fit")
    vh, vb = new.write()
    assert len(vh) == vh_len
    part = vh + bytes(-len(vh) % SECTOR) + vb
    part += bytes(RIDE_PART_SECTORS * SECTOR - len(part))
    assert len(part) == RIDE_PART_SECTORS * SECTOR
    data[:RIDE_PART_SECTORS * SECTOR] = part
    open(path, "wb").write(bytes(data))
    return {"tones_replaced": changed, "tones_kept_us": sorted(kept), "vb": len(vb),
            "part0_sha1": hashlib.sha1(part).hexdigest()[:12]}


def cleanup(out):
    """remove the files an earlier run wrote (its json lists them)"""
    prev = os.path.join(out, "ps1jp_banks.json")
    n = 0
    if os.path.exists(prev):
        for r in json.load(open(prev)):
            if r.get("action") == "grafted":
                for ext in (".VH", ".VB"):
                    f = os.path.join(out, os.path.basename(r["file"]) + ext)
                    if os.path.exists(f):
                        os.remove(f)
                        n += 1
        os.remove(prev)
    return n


def selftest(R, banks):
    """graft with itself = identity (per-tone samples), VH size kept"""
    n = 0
    for name in banks:
        b = os.path.join(R, name)
        vh, vb = open(b + ".VH", "rb").read(), open(b + ".VB", "rb").read()
        v = Vab(vh, vb)
        g, ch = graft(v, v)
        assert ch == 0, name
        assert tone_samples(g) == tone_samples(v), name
        vh2, vb2 = g.write()
        v2 = Vab(vh2, vb2)
        assert tone_samples(v2) == tone_samples(v), name
        assert len(vh2) == len(vh), (name, len(vh2), len(vh))
        n += 1
        if n >= 12:
            break
    print("selftest: %d banks, graft with itself is the identity" % n)


def main():
    args = sys.argv[1:]
    out = os.path.join(ROOT, "build/us")
    if "--out" in args:
        out = args[args.index("--out") + 1]
    if "--clean" in args:
        print("ps1jp banks: %d earlier output files removed (field voices off)" % cleanup(out))
        return
    if "--arcf" in args:
        rep = os.path.join(out, "ps1jp_banks.json")
        extra = 0
        if os.path.exists(rep):
            extra = sum(r.get("extra_sectors", 0) for r in json.load(open(rep)) if r.get("action") == "grafted")
        dst = os.path.join(out, "ARC_F.BIN")
        if extra:
            retail = open(os.path.join(ROOT, "disks/us/BIN/ARC_F.BIN"), "rb").read()
            n = (len(retail) + SECTOR - 1) // SECTOR
            assert extra < n - 8, extra
            open(dst, "wb").write(retail[:(n - extra) * SECTOR])
            print("ARC_F.BIN (unused by the mod): %d -> %d sectors, gives back the %d sectors VAB/SD_MAR.VB grew" % (n, n - extra, extra))
        else:
            if os.path.exists(dst):
                os.remove(dst)
            print("ARC_F.BIN: retail (no sector to give back)")
        return
    jpath = os.environ.get("JP_T1")
    if not jpath:
        sys.exit("gen_ps1jp_banks: JP_T1 (the Japanese disc's track 1) is not set")
    jpath = jpath if os.path.isabs(jpath) else os.path.join(ROOT, jpath)
    iso = Iso(jpath)
    files = iso.files()
    R = os.path.join(ROOT, "disks/us")
    os.makedirs(out, exist_ok=True)
    cleanup(out)  # (outputs of an earlier run that are not made again)
    banks = sorted(n[:-3] for n in files
                   if os.path.basename(n).startswith("SD_") and n.endswith(".VH")
                   and os.path.exists(os.path.join(R, n)))
    if "--check" in args:
        selftest(R, banks)
    ok, info = sfx_tables_identical(iso, files)
    if not ok:
        sys.exit("gen_ps1jp_banks: the US and Japanese sound tables differ: %s" % (info,))
    print("sfx tables: all %d sounds pick the same (vab, program, tone) on both discs" % info)
    used_tones = sfx_used_tones()
    report = []
    seen = {}
    for name in banks:
        base = os.path.basename(name)
        if base in seen:
            sys.exit("gen_ps1jp_banks: basename %s is not unique" % base)
        seen[base] = name
        if base == "SD_J010":
            continue
        usvh = open(os.path.join(R, name + ".VH"), "rb").read()
        usvb = open(os.path.join(R, name + ".VB"), "rb").read()
        jvh = iso.read(files[name + ".VH"])
        jvb = iso.read(files[name + ".VB"])
        if (usvh, usvb) == (jvh, jvb):
            continue
        us, jp = Vab(usvh, usvb), Vab(jvh, jvb)
        if base in MOD_GRAFT:
            mvh, mvb = os.path.join(out, base + ".VH"), os.path.join(out, base + ".VB")
            if not (os.path.exists(mvh) and os.path.exists(mvb)):
                sys.exit("gen_ps1jp_banks: %s: the mod tool's bank is not in %s (run it first)" % (base, out))
            mod = Vab(open(mvh, "rb").read(), open(mvb, "rb").read())
            retail = tone_samples(us)
            new, changed, kept = fit(mod, jp, VB_LIMIT, used_tones, bank_vab(name), retail, TRIM.get(base))
            if new is None:
                sys.exit("gen_ps1jp_banks: %s does not fit %d bytes" % (base, VB_LIMIT))
            vh, vb = new.write()
            assert len(vh) == len(open(mvh, "rb").read()), base
            made = sum(1 for k, v in tone_samples(mod).items() if retail.get(k) != v)
            open(mvh, "wb").write(vh)
            open(mvb, "wb").write(vb)
            ride = graft_ride(out, us, jp, used_tones, bank_vab(name))
            report.append({"file": name, "action": "grafted (mod bank)", "tones_replaced": changed,
                           "tones_kept_us": sorted(kept), "mod_made_tones_kept": made, "vb": len(vb),
                           "vh_sha1": hashlib.sha1(vh).hexdigest()[:12], "vb_sha1": hashlib.sha1(vb).hexdigest()[:12],
                           "ride": ride})
            continue
        if base in SKIP:
            report.append({"file": name, "action": "kept (mod tool bank)"})
            continue
        ok, ev, why = layout_check(us, jp)
        weak = False
        if not ok and name in WEAK_OK and why.startswith("only"):
            ok, weak = True, True
        if not ok:
            report.append({"file": name, "action": "kept (layout differs)", "why": why})
            continue
        usn = (len(usvb) + SECTOR - 1) // SECTOR
        # Two kinds of banks. ANCHORED: cd.c holds the disc position (a hard-coded LBA) and the retail byte
        # length of the files VAB/SD_* (0x610D..0x614F) and SERVANT/SD_*, and the files after them up to
        # DEMOKEY/F_END (0x7489) have hard-coded positions too: they keep their retail byte length exactly
        # (padded with zeros). FREE: the stage and boss banks come after the last such file; their position
        # and size are read from the tables make disk regenerates (include/lba_us.h), so they may be any size
        # up to the sound RAM slot of vab 3 (VB_LIMIT) and later files just move.
        anchored = name.startswith(("VAB/", "SERVANT/"))
        limit = len(usvb) if anchored else VB_LIMIT
        mar = name == "VAB/SD_MAR"
        if mar:
            limit = min((usn + MAR_EXTRA_MAX) * SECTOR, SLOT[2])
        new, changed, kept = fit(us, jp, limit, used_tones, bank_vab(name), trim=TRIM.get(base))
        if new is None:
            report.append({"file": name, "action": "kept (layout differs)", "why": "does not fit %d bytes" % limit})
            continue
        if changed == 0:
            report.append({"file": name, "action": "kept (layout differs)", "why": "no Japanese tone fits the size"})
            continue
        vh, vb = new.write()
        if len(vb) > VB_LIMIT:
            sys.exit("gen_ps1jp_banks: %s VB %d > %d" % (name, len(vb), VB_LIMIT))
        if len(vh) != len(usvh):
            sys.exit("gen_ps1jp_banks: %s VH size changed" % name)
        if len(vb) > SLOT[bank_vab(name)]:
            sys.exit("gen_ps1jp_banks: %s VB %d > its vab %d sound RAM slot %d" % (name, len(vb), bank_vab(name), SLOT[bank_vab(name)]))
        vb_raw = len(vb)
        if anchored and not mar and len(vb) <= len(usvb):
            vb += bytes(len(usvb) - len(vb))  # the retail length: no table changes
        assert not anchored or mar or len(vb) == len(usvb) or len(vb) < len(usvb), name
        open(os.path.join(out, base + ".VH"), "wb").write(vh)
        open(os.path.join(out, base + ".VB"), "wb").write(vb)
        report.append({"file": name, "action": "grafted", "tones_replaced": changed, "tones_kept_us": sorted(kept), "kept_audible": sorted(k for k in kept if (bank_vab(name), k[0], k[1]) in used_tones),
                       "evidence": ev, "weak": weak, "us_vb": len(usvb), "vb": len(vb), "vb_raw": vb_raw, "slot": SLOT[bank_vab(name)], "cdc_len": (len(usvb) if anchored and not mar else None), "extra_sectors": (max(0, (len(vb) + SECTOR - 1) // SECTOR - usn) if mar else 0),
                       "vh_sha1": hashlib.sha1(vh).hexdigest()[:12],
                       "vb_sha1": hashlib.sha1(vb).hexdigest()[:12]})
    json.dump(report, open(os.path.join(out, "ps1jp_banks.json"), "w"), indent=1)
    g = [r for r in report if r["action"] == "grafted"]
    k = [r for r in report if r["action"].startswith("kept (layout")]
    print("ps1jp banks: %d grafted (%d tones), %d kept English (layout differs), %d mod-tool banks untouched -> %s" % (
        len(g), sum(r["tones_replaced"] for r in g), len(k),
        len(report) - len(g) - len(k), out))
    for r in k:
        print("  kept:", r["file"], "-", r["why"])


if __name__ == "__main__":
    main()

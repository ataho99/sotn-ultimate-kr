#!/usr/bin/env python3
"""최소 ISO9660 리더 (PVD + 디렉터리 레코드). 재료2 검증·XA 영역 확인용."""
import struct

SECT = 2352
SYNC = bytes.fromhex("00" + "ff" * 10 + "00")


def read_sector(f, lba):
    f.seek(lba * SECT)
    return f.read(SECT)


def is_mode2(s):
    return s[0:12] == SYNC and s[15] == 0x02


def pvd(f):
    """LBA 16 PVD(user 2048B @+24)를 파싱해 dict 반환. 없으면 None."""
    s = read_sector(f, 16)
    if not is_mode2(s):
        return None
    u = s[24:24 + 2048]
    if u[0] != 1 or u[1:6] != b"CD001":
        return None
    return {
        "vol_id": u[40:72].rstrip(b" ").decode("ascii", "replace"),
        "vol_size": struct.unpack("<I", u[80:84])[0],
        "path_lba": struct.unpack("<I", u[132:136])[0],
        "root_lba": struct.unpack("<I", u[158:162])[0],
        "root_len": struct.unpack("<I", u[166:170])[0],
    }


def list_dir(f, lba, nsectors):
    """디렉터리 영역의 레코드 (name, extent_lba, size) 목록."""
    out = []
    for i in range(nsectors):
        s = read_sector(f, lba + i)
        u = s[24:24 + 2048]
        off = 0
        while off < 2048:
            ln = u[off]
            if ln == 0:
                break
            rec = u[off:off + ln]
            flags = rec[25]
            elba = struct.unpack("<I", rec[2:6])[0]
            size = struct.unpack("<I", rec[10:14])[0]
            nlen = rec[32]
            nm = rec[33:33 + nlen]
            out.append((bytes(nm), elba, size, flags))
            off += ln
    return out


# NOTE: cue 검사는 v0.py에서 파일 존재·트랙 수 문자열로 수행 — 풀 ISO 파서가 아닌 최소 리더임

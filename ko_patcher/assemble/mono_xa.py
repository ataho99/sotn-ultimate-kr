#!/usr/bin/env python3
"""mono_xa.py — repack_xa.py dual-mono -> mono 변환의 순수 파이썬 재현.

표준 라이브러리만 사용한다 (numpy 없음, 외부 바이너리 없음, 윈도 단일 exe 목표).
근거: mods/sotn-ultimate/tools/repack_xa.py
  - to_mono (200-214행): 스테레오 그룹 좌유닛 -> 모노 그룹 (리샘플·게인·클립 없음,
    정수 니블 재배열만)
  - units_of (157-165행): 그룹 -> 파라미터·니블 분해
  - sector_bytes (216-219행): 섹터 조립 (서브헤더 file/ch/sm/coding + 데이터)
  - 검증식 (550-561행): 좌유닛 동등 (numpy 동등식; 본 모듈은 정수 연산으로 동일 결과)
  - dual_mono 판정 (168-193행): satxa dec + 부동소수 dB 판정 — 본 모듈은 판정하지
    않는다. 판정은 빌드 시 이미 끝난 값(교체표 비고의 mono/stereo 유지, swap_cov.json)
    을 사용한다. 즉 본 모듈은 "변환"만 하며, "어느 스트림을 변환할지"는 입력표가 준다.

입력: 일본 PS1 v1.2 XA 섹터 페이로드 (재료2 T1에서 ISO extent로 읽음).
출력: 해당 모노 섹터의 [24:24+2304] 데이터 (F2 모노 18그룹 x 128B).

결정성: 시간·난수·딕셔너리 순서·부동소수 의존 없음 (아래 코드에 import time/random
없음, dict 순회로 출력 순서가 정해지는 부분 없음, //·&·|·<< 정수 연산만).
같은 입력 2회 실행은 바이트 동일 (오라클에서 실측).
다른 OS 동일성: 바이트 슬라이싱·int.from_bytes('little')·translate 표만 사용.
표는 모듈 import 시점에 결정적 루프로 생성 (LOW4/HI4: 256항, 순서 고정).
"""
import hashlib

SECT = 2352
DATA_OFF = 24          # 섹터 내 사운드 데이터 시작
GROUPS = 18            # 섹터당 128B 사운드 그룹 수
GROUP_SZ = 128
MONO_DATA = GROUPS * GROUP_SZ  # 2304


def _build_tables():
    """니블 표 2개. (순수 결정적 루프 — random/time/hash-seed 무관)"""
    low = bytearray(256)
    high = bytearray(256)
    for i in range(256):
        low[i] = i & 15
        high[i] = (i >> 4) & 15
    return bytes(low), bytes(high)


LOW4, HI4 = _build_tables()


def stereo_groups(payload):
    """스테레오 섹터 페이로드들(bytes) -> 128B 그룹 리스트.

    payload 길이는 2304(=18그룹)의 배수여야 한다.
    """
    if len(payload) % GROUP_SZ != 0:
        raise ValueError("payload %d not multiple of 128" % len(payload))
    return [payload[i:i + GROUP_SZ]
            for i in range(0, len(payload), GROUP_SZ)]


def to_mono_groups(groups):
    """스테레오 그룹 리스트 -> 모노 그룹 리스트 (repack to_mono와 동일).

    - 좌유닛(짝수 유닛)만 취한다: 파라미터 p[:,0::2], 니블 n[:,0::2].
    - 입력 그룹 수가 36의 배수가 아니면 0-그룹으로 패딩 (pad = -len % 36).
      36 스테레오 그룹 -> 18 모노 그룹 = 모노 1섹터 (repack: whole mono sector).
    - 출력 그룹 j의 파라미터: 좌유닛 4개씩 —
      out[0:4]=out[4:8]=[g0.p0,g0.p2,g0.p4,g0.p6],
      out[8:12]=out[12:16]=[g1.p0,g1.p2,g1.p4,g1.p6]
      (g0=스테레오 그룹 2j, g1=2j+1).
    - 출력 그룹 j 데이터 28행 x 4B: 행 r =
      [P(d0[4r],d0[4r+1]), P(d0[4r+2],d0[4r+3]),
       P(d1[4r],d1[4r+1]), P(d1[4r+2],d1[4r+3])]
      (P(a,b)=(a&15)|((b&15)<<4), d0=g0[16:128], d1=g1[16:128]).
    """
    n = len(groups)
    pad = (-n) % (2 * GROUPS)
    if pad:
        zero = b"\x00" * GROUP_SZ
        groups = list(groups) + [zero] * pad
        n += pad
    out = []
    for j in range(0, n // 2):
        g0 = groups[2 * j]
        g1 = groups[2 * j + 1]
        # 좌유닛 파라미터 (짝수 유닛 p0,p2,p4,p6) — units_of p[:,0::2]와 동일.
        lp0 = bytes((g0[4], g0[6], g0[8], g0[10]))
        lp1 = bytes((g1[4], g1[6], g1[8], g1[10]))
        head = lp0 + lp0 + lp1 + lp1
        d0 = g0[16:128]
        d1 = g1[16:128]
        # 저니블 문자열 (C 수준 translate) + bigint로 쌍별 결합:
        # C0 = E0 | (O0 << 4) — 바이트값이 0..15라 자리올림 없음.
        l0 = d0.translate(LOW4)
        l1 = d1.translate(LOW4)
        e0 = int.from_bytes(l0[0::2], "little")
        o0 = int.from_bytes(l0[1::2], "little")
        e1 = int.from_bytes(l1[0::2], "little")
        o1 = int.from_bytes(l1[1::2], "little")
        c0 = (e0 | (o0 * 16)).to_bytes(56, "little")
        c1 = (e1 | (o1 * 16)).to_bytes(56, "little")
        body = bytearray(112)
        for r in range(28):
            o = r * 4
            q = r * 2
            body[o] = c0[q]
            body[o + 1] = c0[q + 1]
            body[o + 2] = c1[q]
            body[o + 3] = c1[q + 1]
        out.append(bytes(head) + bytes(body))
    return out


def run_to_mono_sectors(span_payload):
    """한 런의 스테레오 페이로드(bytes, 2304B의 배수) -> 모노 섹터 데이터 리스트.

    각 원소는 2304B (18 모노 그룹). 섹터 수 = ceil(입력그룹수/36).
    """
    groups = stereo_groups(span_payload)
    mg = to_mono_groups(groups)
    if len(mg) % GROUPS != 0:  # to_mono는 항상 18의 배수를 만든다 (불변식)
        raise AssertionError("mono groups %d not multiple of 18" % len(mg))
    return [b"".join(mg[i:i + GROUPS]) for i in range(0, len(mg), GROUPS)]


def sector_bytes(sync_mode16, sub, data2304):
    """모노 F2 섹터 2352B 조립 (repack sector_bytes와 동일).

    sync_mode16: 템플릿 섹터의 [0:16] (sync+헤더; MSF는 호출자가 LBA로 계산).
    sub: 8B 서브헤더 (file/ch/sm/coding x2 — 호출자가 규칙/표로 생성).
    data2304: 모노 데이터. 꼬리는 0 (F2 EDC=0 규칙 — 전수 확인).
    """
    if len(sync_mode16) != 16:
        raise ValueError("sync_mode16 must be 16B")
    if len(sub) != 8:
        raise ValueError("sub must be 8B")
    if len(data2304) != MONO_DATA:
        raise ValueError("data must be 2304B")
    return bytes(sync_mode16) + bytes(sub) + bytes(data2304) + b"\x00\x00\x00\x00"


def msf_head(t45_lba, mode=0x02):
    """LBA -> [0:16] (sync + MSF BCD + mode). v1.py msf_head와 동일."""
    sync = bytes.fromhex("00" + "ff" * 10 + "00")

    def bcd(v):
        return ((v // 10) << 4) | (v % 10)

    f = t45_lba + 150
    return sync + bytes([bcd(f // 75 // 60), bcd((f // 75) % 60), bcd(f % 75),
                         mode])


def sha1(b):
    return hashlib.sha1(b).hexdigest()

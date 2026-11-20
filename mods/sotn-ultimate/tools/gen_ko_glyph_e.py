#!/usr/bin/env python3
# gen_v14e.py -- glyph block v14d (859 cells) -> v14e: 15 required Hangul + fill spare cells (keep SPARE free).
# Order: (1) chars of current translation tables missing from the map, by frequency; (2) other translation tables;
# (3) KS X 1001 Hangul by modern-frequency list. Same render() recipe as gen_ko_glyph_lat.py. First 859 cells unchanged.
import sys, io, os, re, glob, struct, hashlib, collections
TOOLS = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, TOOLS)
from ko_paths import KO_WK  # noqa: E402
import gen_ko_glyph_lat as G
W = KO_WK.rstrip("/") + "/"  # public repo: ko_data (translation tables that are not in the repo are skipped below)
M54 = W + "시험50/m54/"
P4 = G.P4
CB, BASE, LIMIT, SPARE = 23, 0xF040, 960, 10
REQ = "걷궂꾀닐뚝뭉민믿싶앞얄잊존큭헛"
blk = open(P4 + "ko_glyph_block_mgalmuri12_v14d.bin", "rb").read()
assert len(blk) == 859 * CB, len(blk)
mp = {}
for ln in io.open(P4 + "ko_glyph_map_v14d.tsv", encoding="utf-8"):
    q = ln.rstrip("\n").split("\t")
    if len(q) >= 2 and q[1].startswith("0x") and len(q[0]) == 1:
        mp[q[0]] = int(q[1], 16)
promo = set()
for ln in io.open(P4 + "ko_promo_map.tsv", encoding="utf-8"):
    q = ln.rstrip("\n").split("\t")
    if len(q) >= 3 and q[1].startswith("0x"):
        promo.add(q[0])
have = set(mp) | promo
def ishan(c): return "가" <= c <= "힣"
cur_files = [W + "번역/번역_v14_작업_시험56.tsv", W + "번역/번역_v14_wrap원본_시험56b.tsv", W + "번역/번역_v14_장비.tsv",
             W + "번역/번역_설정화면.tsv", W + "번역/번역_저장문구.tsv", W + "번역/번역_필드메시지.tsv"]
cur_files += glob.glob(W + "**/번역_v14*.tsv", recursive=True)
cur_files = sorted(set(f for f in cur_files if os.path.exists(f)))
old_files = sorted(set(glob.glob(W + "번역/번역_v1[0-3]_통합.tsv") + glob.glob(W + "번역/번역_v[3-9]_*.tsv")) - set(cur_files))
def scan(files):
    cnt = collections.Counter(); src = collections.defaultdict(set); other = collections.Counter()
    for f in files:
        try:
            txt = io.open(f, encoding="utf-8").read()
        except Exception as e:
            print("skip", f, e); continue
        for c in txt:
            if ishan(c) and c not in have:
                cnt[c] += 1; src[c].add(os.path.basename(f))
            elif ord(c) > 127 and not ishan(c) and c not in have and c not in "…“”‘’":
                other[c] += 1
    return cnt, src, other
c1, s1, o1 = scan(cur_files)
c2, s2, o2 = scan(old_files)
print("현재 번역표 %d개, 구판 %d개" % (len(cur_files), len(old_files)))
for f in cur_files: print("  cur", f.replace(W, ""))
missing_req = [c for c in REQ if c in have]
assert not missing_req, ("already mapped", missing_req)
tier = []
for c in REQ: tier.append((c, "필수", c1.get(c, 0) + c2.get(c, 0)))
seen = set(REQ)
for c, n in c1.most_common():
    if c not in seen: tier.append((c, "현재표", n)); seen.add(c)
for c, n in c2.most_common():
    if c not in seen: tier.append((c, "구판표", n)); seen.add(c)
cap = LIMIT - SPARE - 859
FREQ = ("이다는에의가을하고로서한지그도들인를있어기게리대사시자아나으수보전스제정상일면주라해때우것은적과성년동부미거여국무진세오경공안신원개연마위분장유드물같간명교구비방선바문별실만초체발영또내현당조결계레월력용심설표야몇목행류관답산음회양준즉통확감각러르"
        "습니않없었겠았될된됐를랑께랍란랄람랐랑래랙랜램랫량러럭런럼렀렇레렉렌렘렛려력련렬렴령례로록론롬롭롯료루룩룬룸류륙률륜르륵른름릇릎리릭린림립릿링"
        "곳골곰공과관광굳굴궁권귀규균그극근글금급긋기긴길김깃깊까깨꺼께꼭꽃꿈끄끈끊끌끝나낙난날남납낫낮낳내냄냉너넉널넓넘넣네녀녁년념녕노녹논놀농높놓놈뇌누눈눌뉴느늑는늘능늦니닉닌닐님닛다닥닦단닫달닭닮담답당대댁더덕던덜덤덧데델도독돈돌동돼되된두둑둔둘둥뒤뒷드득든듣들듬듯등디딩따딱땅때떠떡떤떨떻또똑뚜뛰뜨뜻띄"
        "마막만많말맑맘맛망맞맡매맥맨맴머먹먼멀멈멋멍메멘멜며면명몇모목몬몰몸못몽묘무묵묶문묻물뭇뭐미민믿밀밑바박반받발밝밟밤밥방밭배백뱃버벌범법벗베벤벽변별병보복볶본볼봄봇봉부북분불붉붙브블비빈빌빛빠빨빼뺨뿐삐사삭산살삶삼상새색샘생서석섞선설섬섭성세센셈셋소속손솔솜송쇠수숙순술숨숲쉬쉽스슨슬슴습승시식신실싫심십싱싶싸싹쌍써썩썰쎄쏘쏟쓰쓴쓸씀씨씩씻아악안앉않알앓암압앞애액앨야약얇양얕얗어억언얻얼엄업없엇엉에엔여역연열엷염엽영옆예옛오옥온올옮옳옷와완왕왜외왼요욕용우욱운울움웃웅워원월웨위윗유육율으윽은을음응의이익인일읽잃임입잇있잊잎자작잔잖잘잠잡장재쟁저적전절점접젓정젖제젠조족존졸좀종좋좌죄주죽준줄줌중쥐즈즉즐즘증지직진질짐집짓징짙짚짜짝짧째쨌쩌쩍쪽쫓쭈찌찍찢차착찬찰참창찾채책챙처척천철첫청체쳐초촉촌총최추축춘출춤충취측층치칙친칠침칭카칸칼캐커컨컴코콜콩쾌크큰클큼키킬타탄탈탑태택탱터턱털테토통퇴투튼트특튼틀틈티팀파판팔패퍼편평폐포폭표푸푹풀품풍퓨프플피필핏하학한할함합항해핵햇행향허헌험헤헬혀현혈협형혜호혹혼홀홍화확환활황회획횟효후훈훌훔훨휘휴흉흐흑흔흘흙흡흥흩희흰히힘")
ks = []
for hi in range(0xB0, 0xC9):
    for lo in range(0xA1, 0xFF):
        try: ks.append(bytes([hi, lo]).decode("euc-kr"))
        except Exception: pass
ks = [c for c in ks if ishan(c)]
print("KS X 1001 한글 %d자" % len(ks))
ksset = set(ks)
for c in FREQ:
    if c in ksset and c not in have and c not in seen:
        tier.append((c, "KSX빈도", 0)); seen.add(c)
add = tier[:cap]
print("추가 칸수: 한도 %d셀 - 비상 %d - 기존 859 = 추가 가능 %d; 후보 %d (필수 15, 현재표 %d, 구판표 %d, KSX %d)" % (
    LIMIT, SPARE, cap, len(tier), sum(1 for t in tier if t[1] == "현재표"), sum(1 for t in tier if t[1] == "구판표"), sum(1 for t in tier if t[1] == "KSX빈도")))
assert all(c in [a[0] for a in add] for c in REQ)
out = bytearray(blk)
rows = []
for k, (c, tr, n) in enumerate(add):
    d = G.render(c)
    assert d.any(), ("글꼴에 잉크 없음", c)
    code = BASE + 859 + k
    out += G.pack(d)
    mp[c] = code
    rows.append((c, code, tr, n, ",".join(sorted(s1.get(c, set()) | s2.get(c, set())))))
assert out[:len(blk)] == blk
assert len(out) // CB <= LIMIT - SPARE + 0, len(out) // CB
open(P4 + "ko_glyph_block_mgalmuri12_v14e.bin", "wb").write(bytes(out))
with io.open(P4 + "ko_glyph_map_v14e.tsv", "w", encoding="utf-8") as f:
    f.write("글자\t코드\tn\t오프셋\n")
    for ch, code in sorted(mp.items(), key=lambda x: x[1]):
        f.write("%s\t0x%04X\t%d\t%d\n" % (ch, code, code - BASE, (code - BASE) * CB))
with io.open(M54 + "v14e_추가글자.txt", "w", encoding="utf-8") as f:
    f.write("# 글리프 v14e 추가 %d자 (셀 859 -> %d, 한도 960, 비상 %d셀 남김)\n# 글자\tU+\t코드\t분류\t번역표 빈도\t출처\n" % (len(rows), 859 + len(rows), 960 - 859 - len(rows)))
    for c, code, tr, n, s in rows:
        f.write("%s\tU+%04X\t0x%04X\t%s\t%d\t%s\n" % (c, ord(c), code, tr, n, s))
    f.write("\n# 추가 글자 이어붙임\n" + "".join(r[0] for r in rows) + "\n")
han = sorted(c for c in mp if ishan(c))
eng = struct.unpack("<94H", open(P4 + "ko_engtab_253.bin", "rb").read())
with io.open(M54 + "글리프맵_글자목록_v14e.txt", "w", encoding="utf-8") as f:
    f.write("# 글리프 맵 v14e: 한글 %d자 (v14d %d + 추가 %d), 1바이트 승격 %d자 포함 여부는 맵 기준\n" % (len(han), len(han) - len(rows), len(rows), len(promo)))
    f.write("# 한글 (가나다순, 줄당 40자)\n")
    for i in range(0, len(han), 40): f.write("".join(han[i:i + 40]) + "\n")
    f.write("# 영문/기호 (engtab_253 에 칸이 있는 ASCII)\n" + "".join(chr(0x21 + i) for i, c in enumerate(eng) if c) + "\n")
    f.write("# 비한글 맵 글자\n" + "".join(c for c in mp if not ishan(c)) + "\n")
    f.write("# 새로 칸을 준 글자(v14e)\n" + "".join(r[0] for r in rows) + "\n")
if o1: print("현재표 비한글 미수록(참고):", "".join("%s(U+%04X,%d)" % (c, ord(c), n) for c, n in o1.most_common()))
print("v14e block %d cells %d B sha256 %s" % (len(out) // CB, len(out), hashlib.sha256(bytes(out)).hexdigest()))
print("v14d block sha256 %s" % hashlib.sha256(blk).hexdigest())
print("NEXT: KO_GLYPH_COUNT %d" % (len(out) // CB))

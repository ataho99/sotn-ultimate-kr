# SOTN Ultimate 한국어화 — v0.9 베타

[SOTN Ultimate](https://github.com/renanbianchi/sotn-ultimate)에 한국어를 넣는 비공식 포크입니다. 원작자의 작업이 아닙니다. 계획은 [ROADMAP.md](ROADMAP.md) 에 있습니다.

A fork of SOTN Ultimate that adds Korean. **v0.9 (beta, built on Ultimate v1.4)** is installed from a single web page — you supply your own discs; this repository and the page contain no game data. The original mod is by renanbianchi; this fork is unofficial.

**웹 패처: https://ataho99.github.io/sotn-ultimate-kr/** (크롬·엣지 최신판, 설치 없음 · GitHub Pages 는 `ko` 브랜치의 `docs/` 폴더에서 게시)

**[💖 후원](https://litt.ly/smash99)** — 작업 후원입니다. 게임 파일은 배포하지 않습니다.

## 진행 상황 (2026-11-19)

| 단계 | 상태 |
|---|---|
| 준비 · 원작 재현 · 조사 · 설계 · 한글 출력 시험 (P0–P4) | 완료 — 원작 v1.4 기준 |
| 원문 추출 · 용어집 (P5) | 완료 — 원문 1,715행, 용어는 일본판 기준 |
| 번역 · 삽입 (P6) | 완료(v0.9 범위) — 메뉴·장비·아이템·유물·주문·대사·저장 문구 한글 |
| **배포 (P7)** | **v0.9 베타 준비 중** — 웹 패처(재료 검사 → 조립 → 지문 확인) |
| 원작 추종 · 원작 합류 (P8–P9) | 예정 |

- 번역은 원문에서 새로 했습니다. 기존 PS1 한글패치 문장은 쓰지 않았고, 제작진은 크레딧에 「참고」로 밝힙니다.
- 글꼴: [MGalmuri](https://github.com/MonadABXY/mgalmuri-font)(OFL) · [Galmuri](https://github.com/quiple/galmuri)(OFL).
- 작업 브랜치: `ko` · 원작 추종 브랜치: `sotn-ultimate`

---

# 한글판 v0.9 사용·제작 안내


## ⚠ 법적 고지

- 이 저장소와 웹 패처에는 **게임 ROM·디스크 이미지·원본 음성·그림·대사 원문이 들어 있지 않습니다.** 앞으로도 넣지 않습니다(`.gitignore` 로 디스크 이미지 확장자 제외).
- 패치는 **본인이 가진 정품 디스크에서 만든 이미지**를 재료로 넣었을 때만 결과물을 만듭니다. 재료는 사용자 브라우저 안에서만 읽고, 외부 서버로 전송하지 않습니다(페이지는 외부 서버와 통신하지 않습니다).
- 원작과 얼티밋 모드의 권리는 각 권리자에게 있습니다. 이 프로젝트는 비영리 팬 번역입니다.
- 이슈·PR 에 ROM·디스크 이미지·세이브 외 원본 파일을 첨부하지 마세요.

## 빠른 시작 (플레이어용)

1. **재료를 준비합니다.** 아래 「재료」 표의 고정 이름 파일 5개를 한 폴더에 둡니다. 전부 필수이고, 판본이 다르면 패처가 진행하지 않습니다.
2. **웹 패처 페이지를 엽니다.** 크롬·엣지(최신판) 권장 — 그 밖의 브라우저는 지원 대상이 아닙니다.
3. **재료를 고릅니다**(「폴더 선택」 또는 「파일 선택」) **→ 「한글 이미지 만들기」.** 재료 검사 → 조립 → 결과 지문 확인까지 자동입니다. 실패하면 `REJECT <종류>: <사유>` 가 나옵니다(아래 「리젝트」).
4. **`SOTN_Ultimate_v1.4_KR_v0.9.bin` 과 `.cue` 를 저장해 같은 폴더에 두고 `.cue` 를 에뮬레이터에서 엽니다.** 저장 뒤 「임시 파일 지우기」를 누르면 브라우저 안의 작업 사본이 지워집니다. 이상한 곳은 이슈로 제보해 주세요.

### 재료 (고정 이름 · 왜 필요한가)

| 고정 이름 | 판본 | 왜 필요한가 |
|---|---|---|
| `SOTN_Ultimate_v1.4.bin` + `.cue` | 얼티밋 공식 **v1.4**(2026-11-08 릴리스), 704,762,688B · 299,644섹터 · bin md5 `901d74656aedbd8da3050d6a3716f32f` · sha1 `23bf226690c43663c93f75c4c82bb051d6eb15e1` · crc32 `9b22d649` | 바탕 이미지. 지문이 다르면 리젝트 (출처: 얼티밋 v1.4 릴리스 checksums, 재현 빌드 md5 일치) |
| `SOTN_JP_PS1_v1.2.cue` + `SOTN_JP_PS1_v1.2_T1.bin` + `SOTN_JP_PS1_v1.2_T2.ape` | 일본 PS1 v1.2 (SLPM-86023) · T1 545,440,560B · 231,905섹터 | 1997년 일본 성우 음성으로 PS1 음성 칸을 만듭니다. v1.2 만 받습니다 |

### 리젝트 (고정 문구)

| 상황 | 출력 |
|---|---|
| 재료 없음/이름 틀림 | `REJECT 없음: …` |
| 지문 다름 | `REJECT 지문다름: …` (필요한 판본 안내 포함) |
| 델타 적용 실패·결과 불일치 | `REJECT 적용실패: …` · `REJECT 불일치: …` → 화면의 로그를 그대로 이슈에 붙여 주세요 |

## 직접 빌드하기 (제작용)

클론 + 원판 재료만으로 배포판과 **바이트 동일**한 결과를 만들 수 있게 하는 것이 목표입니다. 절차와 해시는 [BUILD.md](BUILD.md) 를 보세요.

## 버그·오타 제보

[Issues](../../issues) 에서 양식 3종 중 고릅니다.

- **버그** — 판본·에뮬·위치·증상
- **번역 오류** — 화면·원문·현재 번역·제안
- **질문**

스크린샷·세이브는 도움이 됩니다. **ROM·디스크 이미지는 첨부하지 마세요.**

## 판본 비교 · 얼티밋 · 이 한글판

> 각 줄 끝 `[출처]` 는 아래 「출처」 목록 번호입니다. `—` 는 확인 가능한 출처를 찾지 못한 칸입니다(추측으로 채우지 않았습니다).

### 1. 기종·판본별 차이

| | PS1 일본판 (1997) | PS1 북미/유럽판 (1997) | 새턴판 (1998) | PSP 「드라큘라 X 크로니클」 수록판 (2007) |
|---|---|---|---|---|
| 발매 | 일본 1997-03-20 [W1] · 이후 「PlayStation the Best」 1998-03-19 [W1] · 일본판은 디스크 안 SLPM_860.23 파일 날짜로 v1.0(1997-02-19) · v1.1(1997-03-06) · v1.2(1998-01-19) 로 구분: v1.1 은 불 늑대의 카르마 코인 미드롭 수정 + 윙 스매시 연속 사용 추가, v1.2 는 사운드 설정(타이틀에서 Select) 추가이며 「the Best」·「PSOne Books」 및 영어판도 이 빌드 [T1] | 북미 1997-10-03 · 유럽 1997-11 [W1] · 북미 Greatest Hits 1998 [W1] | 일본 전용 1998-06-25, 미국 발매 없음 [W1] | 북미 2007-10-23 · 일본 2007-11-08 · 유럽 2008-02-18 [W1] |
| 음성(언어·녹음) | 일본어(원본). 성우: 알루카드 오키아유 료타로 · 리히터 야나다 키요유키 · 마리아 시이나 헤키루(어린 시절)/요코야마 치사(성인) · 드라큘라 와카모토 노리오 [C1] | 영어 재녹음. 현지화는 Jeremy Blaustein(녹음 현장엔 없었음) [W1]. 일본판의 게임오버 음성(마리아·서큐버스·리히터·데스)은 영어판에 없음 [T1] | — (일본 전용 발매 [W1]) | 일본판 외 지역: 새 영어 대본 + 새로 녹음한 영어 음성, 원래 일본어 음성 선택 가능 [W1] |
| 번역 | (원문) | 일본판과 뉘앙스가 다름. 오프닝 글을 새로 씀, 지역명 변경(예: "Olrox's Room" → "Olrox's Quarters", "Catacomb" → "Catacombs") [T1]. 번역 문구 추가: "What is a man?" 등 [W1] | 일본어판(일본 전용 발매 [W1]) | 일본 외: 새 대본 [W1] |
| 플레이어블 | 알루카드 + 리히터(마리아 없음) [C1] | 알루카드 + 리히터(마리아 없음) [C1] | 마리아 플레이 가능 + 보스, 리히터 처음부터 선택 [W1] | 마리아 플레이 가능 + 보스, 「Rondo of Blood」 기반 새 기술 [W1] |
| 추가 지역·보스 | 기준판 | 일본판 치명적 버그 1건 수정 [W1] | 저주받은 감옥·지하 정원 + 신규 보스 [W1] | — |
| 추가 아이템·사역마 | 기준판 | 사역마 스프라이트(Sprite)·노즈데빌(Nosedevil)이 영어판에서 빠짐 [T1] | 「갓 스피드 슈즈」(리히터식 달리기) 등 새턴 전용 아이템, 세 번째 손(스탯 아이템만) [W1] | 일본판에서 있고 국제판에서 빠졌던 사역마 여러 마리 복원, 영어판 「Nocturne」 추가 [W1] |
| 음악·효과음 | 기준판 | 사운드 테스트 추가 [W1] | 이전 작품 곡 리믹스 수록 [W1] | 마무리 곡 「I Am the Wind」 → 「Mournful Serenade」(야마네 미치루) [W1] |
| 화면/해상도 | — | — | 해상도를 활용하지 않고 화면에 늘림(스프라이트 왜곡), 반투명을 디더링으로 대체 [W1] | — |
| 로딩 | — | — | 다른 판보다 로딩이 잦고 길다 [W1] | — |
| 타이틀·로고·로딩 화면 | 기준판 | 코나미 로고·타이틀·로딩 화면·로고가 다름 [T1] | — | — |
| 조작(확인/취소) | ○ 확인 · ✕ 취소 | ✕ 확인 · △ 취소 [T1] | — | — |

이후 이식(한 줄): Xbox 360 XBLA 2007-03-21 · PSone Classics(PSN) 2007-07 · 「Castlevania Requiem」(PS4) 2018-10-26 · Android/iOS 2020-03-04 (크로니클판 기반) [W1]

### 2. 얼티밋(sotn-ultimate)이 구현한 것

- **한 장의 PS1 디스크에 합친 것**: 북미 PS1(SLUS-00067)을 바탕으로, 새턴판 독점 내용(지하 정원·저주받은 감옥과 각각의 거꾸로 성, 새턴 적·보스·아이템·유물·음악, 플레이어블 새턴 마리아)과 PSP 크로니클판 추가분(PSP 영어 음성·수정 대본, 일본어 음성, PSP 마리아(플레이어블·선택 보스))을 가져왔습니다. 밸런스 옵션(새턴 값/PS1 값)과 다수의 수정이 들어 있습니다. [U1]
- **원본 무포함 · 사용자 빌드**: 저장소에 게임 데이터는 없고, 사용자가 세 디스크(PS1 북미 · 새턴 일본 T-9527G v1.400 · PSP 북미 ULUS-10277)로 빌더 페이지에서 직접 만듭니다. 결과 디스크는 릴리스의 MD5 로 확인합니다. [U1]
- **소스**: sotn-decomp 포크(2026-10-14 `698aab2` 기준), AGPL-3.0-or-later. [U1][U2]
- **더빙 선택**: 처음 세이브(소매 세이브)는 PSP 더빙으로 시작하고, PS1 더빙·일본어 더빙(PSP 영어 대본)을 고를 수 있습니다. [U1: 개발자 노트 플래그표]
- **v1.4 (2026-11-08, 커밋 `22a9c6d68`, 최신)**: 입구 성 첫 방문에 새턴 샤프트 등 반영 · 왕실 예배당 음악 수정 · 새턴 마리아 소리·피격·1px 수정과 새 버튼 배치(Sakitoshi 기여) · 새 게임 기본 스테레오 · 새턴 음악 루프 정지 시간 · 갓 스피드 슈즈 변경 · 빌더가 덤프 크기 검사 · PS1 더빙의 보스 마리아가 일본어를 하는 것은 의도. 알려진 문제: 우드랜드 실드 조합은 새턴 밸런스에서만, 4~5분마다 약 0.5초 루프 정지 등. v1.3 세이브는 v1.4 에서 동작. 판 사이 패치 없음. [U3]
- **v1.3 (2026-11-03, 커밋 `df11991`)**: 터미너스 에스트·몬블레이드 수정, 더빙 선택(PS1·PSP·일본어) 수정, 새턴 밸런스 추가(쥬얼 소드 등), 소스 공개(AGPL). [U3]
- 크레딧: Konami · sotn-decomp(Xeeynamo) · sozud · Knight0fDragon(새턴 영어 번역 v1.021) · Castlevania Wiki · Redump. [U1]

### 3. 이 한글판이 더한 것

얼티밋 위에 얹은 변경만 적습니다(원문은 전부 새로 만든 것).

- **일본판 기준 전면 재번역** — 이름뿐 아니라 대사·설명 문장까지 일본판 원문 기준, 대사는 의역 허용. 기존 한글패치를 재료로 쓰려던 계획은 접고 전부 재번역. 지명은 기존 한패판 표기를 따름(예외).
- **장비·아이템 이름은 일본판 기준**, 가타카나 이름은 음차가 기본(영문판 이름 옮겨 적기 아님).
- **한글 글꼴 M갈무리·갈무리(두 가지)** — 대사는 MGalmuri12 로 12px 그대로 직출력(엔진의 16→12 압축 건너뜀), 메뉴는 8px(MGalmuri8·Galmuri7). 대사창 화자 이름은 영문 대문자 유지(DRACULA 식). 쓰인 곳·출처·라이선스는 아래 「글꼴」 절.
- **Dub 선택 = PSP(기본) · SATURN · PS1 세 칸, 모두 일본어 음성** — 영어 음성 칸은 없음. PSP 칸에는 PSP 전용 대사(일본어 PSP 본)가 별도로 연결됨.
- **PS1 칸 일본어 음성 = 일본 PS1 v1.2(SLPM-86023) 원판에서** — 그래서 재료로 일본 PS1 디스크가 하나 더 필요합니다.
- **새턴 배경음악 7곡은 얼티밋 원판에 든 같은 곡을 그대로 사용**(재인코드 안 함).
- **조작키**: ○ 결정 · ✕ 취소(일본판식). 얼티밋 추가 설정(Extra settings) 하위 화면은 번역하지 않고 영문 그대로.
- **컷신 자막 타이밍**: 자막 표시 길이를 음성 길이에 맞췄습니다.
- **타이틀·로딩 화면**: 한글 로고와 「ULTIMATE for v1.4」 부제가 들어간 타이틀, 한글 로고의 Now Loading 화면, 오프닝 「1792년 트란실바니아 지방」 자막을 넣었습니다.

**출처**
- [W1] Wikipedia, *Castlevania: Symphony of the Night* — Versions and re-releases (2026-11-18 열람) https://en.wikipedia.org/wiki/Castlevania:_Symphony_of_the_Night
- [U1] renanbianchi/sotn-ultimate README (태그 v1.4) https://github.com/renanbianchi/sotn-ultimate
- [U2] 같은 저장소 개발자 노트(포크 기준 커밋·라이선스)
- [U3] 같은 저장소 Releases 페이지 v1.4·v1.3 (2026-11-18 maxAge 0 수집) https://github.com/renanbianchi/sotn-ultimate/releases
- [T1] The Cutting Room Floor, *Castlevania: Symphony of the Night (PlayStation)* — Japanese revisions · Japanese vs. North American (2026-11-19 열람) https://www.tcrf.net/Castlevania:_Symphony_of_the_Night_(PlayStation)
- [C1] Castlevania Wiki, *Castlevania: Symphony of the Night* — Japanese voice cast · playable characters (2026-11-19 열람) https://castlevania.fandom.com/wiki/Castlevania:_Symphony_of_the_Night

## 글꼴

이 한글판의 한글·영문·숫자 글리프는 **두 글꼴**로만 만들었습니다(이 저장소의 빌드 스크립트·변환 도구 기준 — 다른 글꼴 시험분은 들어 있지 않습니다).

| 쓰인 곳 | 글꼴 · 크기 | 방식 |
|---|---|---|
| 컷신 대사의 한글·영문·숫자·문장부호, 화자 이름표, 파일 선택 저장 이름, 저장실 문구, 팝업, 손 칸 머리글, 상태·장비 화면의 큰 한글 라벨(12×16) | **MGalmuri12** (M갈무리 12px) | 12×15 셀 819개를 1비트로 구워 넣음(블록 `ko_glyph_block_mgalmuri12_v14c`). 대사는 엔진의 16→12 압축을 건너뛰고 12px 그대로 직출력 |
| 장비 이름 8px 판(371셀) | **MGalmuri8** 셀(M갈무리 8px) — 그 판에 없는 글자는 **Galmuri7** 8px | 8×8 셀, 1:1 |
| 메뉴의 영문·숫자·기호 8×8 시트 | **Galmuri7** 8px (mono) | 원판 시트를 다시 구움(셀 위치·8px 폭 그대로) |

| 글꼴 | 제작 | 출처 | 라이선스 |
|---|---|---|---|
| M갈무리 (MGalmuri) | Monad ABXY — 갈무리를 수정한 파생 글꼴 | https://github.com/MonadABXY/mgalmuri-font | SIL OFL 1.1 · Copyright (c) 2026 Monad ABXY · Copyright (c) 2019-2025 Lee Minseo (quiple) |
| 갈무리 (Galmuri) v2.40.4 | 이민서 (Lee Minseo, quiple) | https://github.com/quiple/galmuri | SIL OFL 1.1 · Copyright (c) 2019-2025 Lee Minseo (quiple@quiple.dev) |

- 글꼴 파일(TTF) 자체는 이 저장소·웹 패처에 넣지 않았고, 글리프를 도트로 구운 결과만 한글판 이미지에 들어갑니다.
- 라이선스 원문은 저장소 `licenses/fonts/` 에 둡니다: `MGalmuri-OFL.txt`, `Galmuri-OFL.txt`. 글꼴 파일을 함께 배포하게 되면 그 꾸러미에도 같은 원문을 넣어야 합니다.

## 저장소 구성

[폴더 안내](폴더안내.md) 참조. 상류(sotn-decomp 얼티밋) 소스 트리 구조를 그대로 두고, 한글화 데이터(`ko_data/`)·조립 도구(`ko_patcher/`)·웹 패처 게시본(`docs/`)을 더했습니다.

## 한글화 흐름

```
[제작자] 소스 빌드: src/ + mods/sotn-ultimate/ + ko_data/ ─► 한글 이미지
                                   │  ko_patcher/assemble/gen_v4.py (재료 대비 델타 생성)
                                   ▼
[사용자] 재료 5개 ─► 웹 패처 (docs/index.html): 재료 검사 → 조립 → 델타 적용 → 지문 확인
                                   ▼
                      SOTN_Ultimate_v1.4_KR_v0.9.cue / .bin
```

상세는 `BUILD.md`(제작·검증)와 `ko_patcher/web/README.md`(웹 패처)를 보세요.

## 요구 환경

- 크롬·엣지 최신판 (브라우저 임시 저장소 OPFS 사용)
- 디스크 여유 약 4GB (재료 약 1.3GB + 브라우저 임시 작업 사본 약 1.4GB + 저장본 0.7GB) — 재료와 저장 위치는 로컬 디스크 권장
- 별도 설치 없음 — 외부 라이브러리 없이 자체 구현(SHA-1·Adler-32·VCDIFF 디코더)

## 번역 수정 방법 (번역가용)

번역 오류는 이슈의 「번역 오류」 양식(화면 · 원문 · 현재 번역 · 제안)으로 알려 주세요. 번역 데이터를 직접 고쳐 PR 을 보내는 방법은 첫 공개 이후 이 절에 추가합니다.

## 저작권 / 라이선스

- 원작·얼티밋 모드의 저작권은 각 권리자에게 있습니다. ROM·그래픽·음성 원본은 포함하지 않습니다.
- 이 저장소 전체(도구·스크립트·번역 데이터)는 **GNU AGPL-3.0-or-later** 하나로 배포합니다(Ultimate 모드의 AGPL 을 승계). 전문은 `LICENSE` 파일을 보세요.
- 웹 패처는 외부 라이브러리를 쓰지 않습니다(자체 코드, AGPL-3.0-or-later). 델타 파일은 표준 VCDIFF(RFC 3284) 형식이며 xdelta 3.2.2(Apache-2.0)로 만들었습니다 — xdelta 코드는 웹 패처에 들어 있지 않습니다.
- 한글 글꼴 **M갈무리**(Monad ABXY, 갈무리 수정판)와 **갈무리**(이민서 quiple)는 SIL OFL 1.1 입니다. 자세한 내용은 위 「글꼴」 절.

---

# SOTN Ultimate (PS1 Version)

A mod of *Castlevania: Symphony of the Night* for the PlayStation that brings
the Sega Saturn version's exclusive content and the PSP *Dracula X Chronicles*
additions to the PS1, on one CD. **This repository contains no game data**:
players build the game from their own discs with the builder page on the
[Releases](https://github.com/renanbianchi/sotn-ultimate/releases) page.

The mod lives in [`mods/sotn-ultimate`](mods/sotn-ultimate); its
[README](mods/sotn-ultimate/README.md) covers the discs it takes, building
from source, the tests, how it sits on top of sotn-decomp (the developer
notes), the licence and the credits.

---

# About sotn-decomp (upstream)

The text below is sotn-decomp's own README.

# Castlevania: Symphony of the Night Decompilation

A work-in-progress decompilation of Castlevania Symphony of the Night for Sony PlayStation 1, Sony PlayStation Portable and Sega Saturn. It aims to recreate the source code from the existing binaries using static and/or dynamic analysis. The code compiles byte-for-byte to the same binaries of the game, effectively being a matching decompilation.

It currently supports the following versions of the game:

* `us` the reference build with the serial number SLUS-00067
* `hd` an unreleased PS1 Japanese build found in Castlevania: Dracula X Chronicles game data
* `pspeu` European build from Castlevania: Dracula X Chronicles
* `saturn` the port created by an external development team

This repo does not include any assets or assembly code necessary for compiling the binaries. A prior copy of the game is required to extract the required assets.

## Useful links

* [Build guide](https://github.com/Xeeynamo/sotn-decomp/wiki/Build)
* [Decompilation guide](https://github.com/Xeeynamo/sotn-decomp/wiki/Decompilation)
* [Progress report](https://sotn.xee.dev/)
* [decomp.dev progress](https://decomp.dev/Xeeynamo/sotn-decomp)

Decompiling a game is a mastodontic task. If you have some basic programming skills, please join us in this journey. Any contribution will be very appreciated!

[![Join to our Discord server](https://discord.com/api/guilds/1079389589950705684/widget.png?style=banner2)](https://sotn-discord.xee.dev/)

## Special thanks

This project is possible thanks to the hard work of tools provided by the Decompilation community:

* [mips2c](https://github.com/matt-kempster/m2c) from @matt-kempster to decompile MIPS assembly into C. This has proven to be more accurate than Hexrays IDA and Ghidra.
* [splat](https://github.com/ethteck/splat) from @ethteck to disassemble code and extract data with a symbol map. This tool provides the fundamentals of the SOTN decomp.
* [asm-differ](https://github.com/simonlindholm/asm-differ) from @simonlindholm to know how the decompiled code compares to the original binary.
* [decomp-permuter](https://github.com/simonlindholm/decomp-permuter) from @simonlindholm to pick different versions of the same code that better matches the original binary.
* [maspsx](https://github.com/mkst/maspsx) by @mkst to replicate the customized assembler used in the official PSX SDK.
* [decomp.me](https://github.com/decompme/decomp.me/) by @ethteck, @nanaian and @mkst to provide a collaborative decompilation site to share and contribute to work-in-progress decompiled functions.
* [frogress](https://github.com/decompals/frogress) by @ethteck to store and retrieve progression data.
* [esa-new](https://github.com/mkst/esa-new) by @mkst as an inspiration on how to set-up a PS1 decompilation project.
* [oot](https://github.com/zeldaret/oot) as an inspiration of what it is possible to achieve with a complete decompiled video game.

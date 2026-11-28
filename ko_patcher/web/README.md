# 웹 패처 (단일 HTML)

`dist/index.html` 한 장이 배포물입니다(GitHub Pages 에 그대로 올림). 외부 서버와 통신하지 않으며, 재료는 브라우저 안에서만 읽습니다.

## 구성
| 파일 | 역할 |
|---|---|
| `src/template.html` | 화면(파일·폴더 선택, 진행률, 로그, 저장 링크) |
| `src/worker.js` | 작업기(Worker): 재료 검사 → 바탕(U2c) 조립 → 델타 적용 → 결과 지문 확인 |
| `src/build.js` | 꾸러미 7개 파일(지문·표·레시피·델타)을 gzip+base64 로 HTML 에 박아 `dist/index.html` 생성 |
| `src/test_unit.js` | SHA-1·Adler-32·VCDIFF 디코더 단위 시험(xdelta3 출력 대조) |
| `src/test_web.js` · `src/test_rej.js` | 헤드리스 크롬 종단 시험 · 거부 문구 시험 |

빌드: `node build.js <꾸러미폴더> template.html worker.js dist/index.html`

## 꾸러미 (HTML 안에 내장)
`fingerprints.json` · `move_runs.tsv` · `subs.bin` · `recipe_b.json` · `recipe_mono.json` · `runs_span.json` · `u2c_to_t45.vcdiff`
(델타는 `xdelta3 -e -f -B 805306368 -S none` — 보조 압축 없는 표준 VCDIFF. 브라우저 디코더가 LZMA 보조 압축을 풀지 않아도 되게 한 것.)

## 사용 라이브러리 (이름·버전·라이선스)
| 구분 | 이름 | 버전 | 라이선스 | AGPL-3.0 과 |
|---|---|---|---|---|
| 배포물에 든 코드 | **없음** — SHA-1(FIPS 180-4)·Adler-32(RFC 1950)·VCDIFF(RFC 3284) 디코더·ISO9660 최소 파서·XA 스테레오→모노는 전부 자체 구현 | — | AGPL-3.0-or-later (저장소와 동일) | 해당 없음 |
| 브라우저 내장 기능 | OPFS(`navigator.storage.getDirectory`·`createSyncAccessHandle`), `FileReaderSync`, `DecompressionStream('gzip')`, Web Worker | 크롬·엣지 | 웹 표준(별도 라이선스 없음) | 해당 없음 |
| 델타 만들 때만(배포 안 됨) | xdelta3 | 3.2.2 | Apache-2.0 | 호환(Apache-2.0 은 AGPL-3.0 에 합칠 수 있음). 코드는 HTML 에 없음 |
| 시험할 때만(배포 안 됨) | playwright-core | 1.65.1 | Apache-2.0 | 호환. 코드는 HTML 에 없음 |
| 시험할 때만 | Node.js | 22.22.1 | MIT 등 | 배포 안 됨 |

참고: `.ape`(2번 T2)는 v4 알고리즘에서 **지문 확인용**입니다(조립에는 쓰이지 않음). 그래서 웹 패처에도 APE 디코더가 없습니다.

## 거부 문구
`REJECT 없음` · `REJECT 지문다름` · `REJECT 적용실패` · `REJECT 불일치` · (그 밖의 오류) `REJECT 오류`

## 합격 기준
크롬(헤드리스)에서 깨끗한 재료 → 출력 sha1 == `ec9b5739af6906a16346ee613523aa53c0d5e91c` (706,126,848 B). 출력 이름은 `SOTN_Ultimate_v1.5_KR_v0.9.5d.bin/.cue`. 종단 시험 결과: sha1 일치, 외부 요청 0건.

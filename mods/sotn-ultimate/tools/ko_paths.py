# 경로 환경값 한 곳 (공개 저장소). 환경변수로 바꿀 수 있다.
#   SOTN_KR   저장소 루트 (기본: 이 파일 기준 ../../..)
#   KO_WK     한글화 데이터 폴더 (기본: <저장소>/ko_data) — P4시험/ 번역/ 검수/ recipe/
#   KO_FONTS  글꼴 폴더 (기본: <KO_WK>/fonts; 글꼴은 글리프 재생성 때만 필요)
import os
_HERE = os.path.dirname(os.path.abspath(__file__))
KO_REPO = os.environ.get("SOTN_KR") or os.path.abspath(os.path.join(_HERE, "..", "..", ".."))
KO_WK = os.environ.get("KO_WK") or os.path.join(KO_REPO, "ko_data")
KO_FONTS = os.environ.get("KO_FONTS") or os.path.join(KO_WK, "fonts")

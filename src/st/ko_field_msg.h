// gen_ko_fieldmsg.py: KO stage message-box labels (do not edit; 번역_필드메시지.tsv).
// The label bytes live in ARC_KO.BIN at KO_FIELD_<KEY>_ADDR (build_ko_trial.sh); the overlays keep the
// retail English array and pick the KO address when its first byte is non-zero.
// A key without an _ADDR macro is still a placeholder: the stage shows the English label.
#pragma once
#define KO_FIELD_PICK(a, en) ((*(volatile u8*)(a)) ? (char*)(a) : (en))
#define KO_FIELD_MIST_ALUCARD_ADDR 0x8009F300 /* 이 벽은 안개가 되면 / 빠져나갈 수 있을 것 같다 (47 B) */
#define KO_FIELD_MIST_RICHTER_ADDR 0x8009F380 /* 이 벽은 나와는 해당 없군 (27 B) */
#define KO_FIELD_SEALED_DOOR_ADDR 0x8009F400 /* 마법으로 잠겨 있다 (21 B) */
#define KO_FIELD_NO4_BRIDGE_ADDR 0x8009F480 /* 폭포 너머의 나무다리에도 / 무언가 나타난 것 같다 (49 B) */

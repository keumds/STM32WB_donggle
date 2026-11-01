# ANVIX P1 Dongle v0.4 — FE43 selective discovery test build

Keil µVision 5.26 project: `MDK-ARM/ANVIX_P1_Dongle.uvprojx`.

## 변경사항
- 16비트 표기 FE40/FE41/FE42/FE43 기준 서비스/특성 판별 (128-bit UUID의 하위 16비트 표시 기준).
- Characteristic 검색에서 FE41 명령, FE42 상태, FE43 스트리밍을 독립적으로 저장.
- FE43 검색 완료 후 CCCD 탐색, CCCD 0x0001 설정, FE43의 Notification만 Cyton 파서로 전달.
- 128비트 서비스 discovery 목록의 이동 폭을 6에서 실제 Attribute_Data_Length로 수정.
- `+b+` 자동 전송 없음. P1은 연결 시 자동 스트리밍 시작한다고 가정.
- Cyton 수신 통계 `[P1 FE43]` 출력. USB CDC 전송 어댑터 기존 유지.

## 중요한 제약
- **P1 실제 128-bit UUID 전체가 제공되지 않아서 FE40~FE43 하위 16bit 비교 가정**입니다. 128비트 UUID의 base가 다르면 잘못 매칭 가능. 반드시 P1 원본 GATT UUID 정의와 일치 여부 확인.
- 현재 ST 샘플 코드의 연결 대상 필터는 ST Manufacturer ID 기반이며 P1 전용 광고 필터가 아님.
- 스캔/Discovery 후 FE43 `handle` 메시지가 나와야 하며, 나오지 않으면 P1 UUID 정의와 비교해야 함.
- 서비스/특성 탐색 완료(및 CCCD Write 완료)를 `APP_BLE_CONNECTED_CLIENT`라고 표시하는 ST 상태 전이는 기반 소스에서 유지. GATT 오류 상태 검증은 추가 필요.
- USB CDC 출력은 ST VCP를 기존 코드대로 사용. 디버그 VCP 재연결이 켜져 있으면 RAW 데이터와 디버그가 섞이므로 실기에서 점검.
- 소스는 정적 검사만 수행. ST CubeWB SDK/Keil 실제 빌드/실기 검증은 수행하지 못함.
- STM32CubeWB 원본 라이브러리와 패키지 메모리 레이아웃을 보존한 상태에서 빌드 필요.

## 테스트 로그 목표
`[P1] FE41 write handle=...`, `[P1] FE42 status handle=...`, `[P1] FE43 stream handle=...`, `[P1] Discover FE43 CCCD`, `[P1] Enable FE43 Notification`, `[P1 FE43] notif=...`.

**참고**: 이 프로젝트는 드라이버 통합 검증 단계이며, 최종 제품 펌웨어가 아닙니다.

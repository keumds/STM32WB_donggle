# ANVIX P1 USB Dongle v0.3 — 검증용 수정 프로젝트

## 변경
- 이전 v0.2 Keil 프로젝트 유지.
- p1_bridge: 33-byte Cyton 프레임 통계, notification 카운트, bad footer / 재동기화 계수, 연결 해제 시 프레임 파서 리셋.
- p2p_client_app: 2 byte가 아닌 기존 P2P notification에 대한 250 패킷 간격의 UART 통계 로그 (`[P1 candidate]`). **FE43이라고 확인된 것은 아님**.
- app_debug: 디버그 텍스트의 USB VCP 송신 비활성화. UART 디버그는 계속 사용. `VCP_Init`은 유지.
- USB CDC 송신 어댑터는 기존 구현을 유지. 성공 호출=드라이버 접수일 뿐 PC 수신까지 검증한 것은 아님.

## 주의 / 아직 미구현
1. 현재 ST P2P 기본 UUID 검색은 유지됩니다. 현재 디바이스의 실제 128-bit FE40~FE43 UUID 선언이 첨부되지 않았으므로 서비스 / FE43 매칭을 확정할 수 없습니다.
2. `p1_bridge`는 33바이트 전체 패킷을 검사합니다. **FE43 프래그먼트만 들어온다는 보장이 있어야 함.** 다른 특성의 일반 응답을 합치면 안 됩니다.
3. `P1_Usb_Rx()`가 VCP RX callback에 연결되어 있지 않으므로 PC → FE41 명령 전달은 미완성입니다. `+b+` 자동 전송도 아직 없습니다.
4. 공유 VCP가 UART 디버깅 출력의 하부 구현에서 충돌하지 않는지 실기 확인이 필요합니다. RX 콜백 연결과 CDC 초기화도 확인 바랍니다.
5. 빌드/기기 검증은 이 환경에서 시행하지 않았습니다. MDK-ARM/ANVIX_P1_Dongle.uvprojx에서 Rebuild 필요.

## 테스트
- BLE 연결 / 서비스 검색 / CCCD 출력 확인.
- 측정을 별도 경로에서 시작하고 UART `[P1 candidate]` 지표 확인.
- valid33가 증가하고 `bad`가 거의 없다면 Cyton 스트림 후보가 수신되는 정황.
- USB COM 포트에서 바이너리를 로깅하여 33바이트 프레임을 확인. 텍스트 콘솔은 ASCII 디버그가 없어야 정상.
- P1 디바이스의 BLE service characteristic 선언 헤더를 확보하면 다음 버전에서 완전한 FE40/41/42/43 검색, CCCD 두 개, 명령 송신을 구현.

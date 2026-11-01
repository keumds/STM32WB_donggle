ANVIX P1 Dongle patch v0.2 (integration work in progress)

수정한 파일:
  STM32_WPAN/App/p2p_client_app.c: FE41 write 함수 구현, 기존 P2P Notify 경로에서 2B 이외 payload 수신 브리지
  Core/Src/main.c: P1_Startup(), P1_Usb_Task() 실행 연결
  ANVIX_P1_App/p1_platform_cdc.c: 비동기 VCP 전송 어댑터 구현
  MDK-ARM/ANVIX_P1_Dongle.uvprojx: 소스 파일 추가

중요한 제한사항 (실동작 검증 전):
  1) 기존 ST BLE_p2pClient는 P2P 서비스 및 LED 버튼용 UUID/128-bit 설정으로 되어 있습니다.
     P1의 FE40/FE41/FE42/FE43 실제 UUID 형식(16비트 vs 128비트 베이스)을 확인하여
     서비스 검색, FE41 검색, FE43 별도 characteristic/CCCD를 구현해야 합니다.
     현 수신 hook은 기존 ST Notification handle의 2바이트 이외 데이터에만 반응하므로
     FE43 수신을 보장하지 않습니다.
  2) USB 초기화는 앱 디버그(app_debug.c)의 VCP_Init 방식과 연동되지만
     CFG_DEBUG_TRACE=1로 되어 있어 PC에 바이너리 데이터 대신 로그가 섞일 수 있습니다.
     전용 VCP 초기화와 VCP RX callback을 연결하고 Debug 출력은 분리/비활성화해야 합니다.
  3) FE41 Write는 현재 MTU 23 기본값 기준 최대 20바이트로 제한합니다.
     연결 타이밍이나 busy 재전송 큐는 추가해야 합니다.
  4) ST STM32CubeWB HAL / Middlewares 라이브러리는 이 ZIP에 포함되지 않습니다.
     ST 전체 저장소의 원본 디렉터리 위치에서 빌드해야 합니다.
  5) 수정 후 Keil 컴파일/보드 동작 검증은 이 환경에서 수행하지 못했습니다.

권장 다음 단계: P1 Peripheral 실제 소스의 BLE UUID 정의와 FE43 CCCD 확인 뒤
  STM32_WPAN/App/p2p_client_app.c에서 서비스 디스커버리와 알림 활성화 수정.

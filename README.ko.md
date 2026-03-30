# CanFeather - Tesla FSD CAN Bus Enabler

이 문서는 이 프로젝트를 처음 사용하는 분도 따라할 수 있도록, 설치부터 차량 연결까지 순서대로 아주 쉽게 정리한 한국어 가이드입니다.

## 먼저 꼭 확인하세요

- 이 프로젝트는 차량 CAN 메시지를 다루므로, 잘못 연결하거나 잘못 설정하면 차량 동작에 문제가 생길 수 있습니다.
- 사용 전 반드시 전기/배선 안전 수칙을 지키고, 본인 책임 하에 진행해야 합니다.
- 운전 중에는 항상 주의를 기울이고 핸들을 잡으세요.

## 이 프로젝트가 하는 일

이 펌웨어는 특정 CAN 메시지를 읽고 다시 송신하여 FSD 관련 기능이 동작하도록 보조합니다.

- 오토파일럿 관련 프레임을 수신
- FSD 활성화에 필요한 비트를 조건에 맞게 조정
- 팔로우 거리(또는 오프셋) 값을 속도 프로파일로 매핑
- 필요 시 조향 개입 경고(nag) 관련 비트 처리
- `enablePrint = true`일 때 시리얼(115200 baud)로 디버그 출력

## 사용 전 필수 조건 (가장 중요)

차량에 유효한 FSD 권한이 있어야 합니다.

- 구매형 FSD 또는 구독형 FSD 중 하나가 활성 상태여야 합니다.
- 이 보드는 CAN 레벨에서 동작을 보조하는 장치이며, Tesla 서버 권한 자체를 대체하지 않습니다.

지역에서 FSD 구독이 막혀 있다면, 기존 README에서 안내한 방식처럼 구독 가능한 지역 계정을 활용하는 방법이 있습니다.

## 준비물

- Adafruit Feather RP2040 CAN (또는 MCP25625/MCP2515 기반 호환 보드)
- USB 케이블
- Arduino IDE
- CAN 배선 (차량 CAN-H, CAN-L 연결)

보드에서 아래 핀 정의를 사용할 수 있어야 합니다.

- `PIN_CAN_CS`
- `PIN_CAN_INTERRUPT` (현재 폴링 모드에서 사실상 미사용)
- `PIN_CAN_STANDBY`
- `PIN_CAN_RESET`

## 내 차량 하드웨어 타입 고르기

`RP2040CAN.ino`에서 `#define HW` 값을 차량 타입에 맞춰 선택해야 합니다.

| Define   | 대상                       | 수신 CAN ID | 특징                           |
| -------- | -------------------------- | ----------- | ------------------------------ |
| `LEGACY` | HW3 레트로핏 (구형 S/X 등) | 1006        | FSD enable bit + 속도 프로파일 |
| `HW3`    | 일반 HW3 차량              | 1016, 1021  | 팔로우 거리 기반 속도 제어     |
| `HW4`    | HW4 차량                   | 1016, 1021  | 5단계 확장 속도 프로파일       |

주의:

- HW4 차량이라도 차량 펌웨어가 `2026.2.3`보다 낮으면 FSDV14가 아니므로 `HW3`로 컴파일해야 할 수 있습니다.

판별 방법:

- `LEGACY`: 세로형 센터 디스플레이 + HW3 (주로 구형 레트로핏)
- `HW3`/`HW4`: 차량 화면에서 `Controls -> Software -> Additional Vehicle Information`에서 확인

## 설치 순서 (처음부터 끝까지)

### 1) Arduino IDE 설치

아래에서 설치합니다.

- https://www.arduino.cc/en/software

### 2) 보드 매니저 URL 추가

Arduino IDE에서:

1. `File -> Preferences`
2. `Additional Board Manager URLs`에 아래 추가

```text
https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
```

3. `Tools -> Board -> Boards Manager`에서 RP2040 계열 패키지 설치
4. 보드를 `Adafruit Feather RP2040 CAN`으로 선택

### 3) 라이브러리 설치

`Sketch -> Include Library -> Manage Libraries...`에서 아래 설치:

- `MCP2515` by autowp

### 4) 코드에서 HW 타입 설정

`RP2040CAN.ino` 상단에서 본인 차량에 맞게 수정:

```cpp
#define HW HW3  // LEGACY, HW3, HW4 중 선택
```

UNO 버전([UNO_MCP2515_CAN.ino](UNO_MCP2515_CAN.ino))은 아래처럼 설정합니다.

```cpp
#define HW_TARGET TARGET_HW3  // TARGET_LEGACY, TARGET_HW3, TARGET_HW4 중 선택
```

MCP2515 모듈 오실레이터 클럭도 함께 확인하세요.

```cpp
#define MCP2515_CLOCK MCP_16MHZ  // 8MHz 모듈이면 MCP_8MHZ
```

### 5) 업로드

1. Feather 보드를 USB로 연결
2. `Tools`에서 보드/포트 확인
3. `Upload` 클릭

### 6) 차량 CAN 배선 연결

아래는 실제로 가장 많이 사용하는 2가지 연결 방식입니다.

### A. Adafruit Feather RP2040 CAN 사용 시

이 경우는 CAN 트랜시버가 보드에 포함되어 있어 차량 쪽 배선만 연결하면 됩니다.

1. Feather 보드 CAN-H -> 차량 커넥터 CAN-H
2. Feather 보드 CAN-L -> 차량 커넥터 CAN-L
3. GND(접지)는 차량 섀시/공통 GND와 기준을 맞추는 것을 권장

### B. Arduino UNO + MCP2515 모듈 사용 시

UNO는 CAN이 내장되어 있지 않으므로 MCP2515(TJA1050 계열 포함) 모듈이 필요합니다.

1. UNO <-> MCP2515(SPI) 배선

| UNO 핀 | MCP2515 핀 | 설명                               |
| ------ | ---------- | ---------------------------------- |
| D13    | SCK        | SPI 클럭                           |
| D12    | SO(MISO)   | MCP2515 -> UNO 데이터              |
| D11    | SI(MOSI)   | UNO -> MCP2515 데이터              |
| D10    | CS         | 칩 선택 (코드 기본값)              |
| D2     | INT        | 인터럽트 입력(옵션, 코드에서 예약) |
| 5V     | VCC        | 모듈 전원                          |
| GND    | GND        | 공통 접지                          |

참고: [UNO_MCP2515_CAN.ino](UNO_MCP2515_CAN.ino) 기준 기본 핀은 `CS=D10`, `INT=D2`입니다.

추가 참고:

- 우노 코드에는 CAN 송신 실패 시 `sendMessage failed: <에러코드>` 로그가 출력됩니다.
- 시리얼 모니터에서 해당 로그가 반복되면 배선, 클럭(`MCP2515_CLOCK`), 버스 상태를 먼저 점검하세요.

2. MCP2515 <-> 차량 CAN 배선

| MCP2515 핀 | 차량 배선  |
| ---------- | ---------- |
| CAN-H      | 차량 CAN-H |
| CAN-L      | 차량 CAN-L |

3. 전원 관련 주의

- 차량 12V를 UNO/MCP2515에 직접 넣지 마세요.
- 차량 전원을 쓸 경우, 검증된 DC-DC(12V -> 5V) 레귤레이터를 사용하세요.
- USB 전원으로 테스트 후 차량 전원으로 전환하는 순서가 안전합니다.

권장 연결 지점:

- 일반적으로 `X179` 커넥터
- 구형 Model 3(2020 이하) 중 X179가 없는 경우 `X652` 커넥터

X179:

| Pin | Signal |
| --- | ------ |
| 13  | CAN-H  |
| 14  | CAN-L  |

X652:

| Pin | Signal |
| --- | ------ |
| 1   | CAN-H  |
| 2   | CAN-L  |

중요:

- Feather CAN 보드의 온보드 `120 ohm` 종단 저항은 차량 버스와 중복되지 않도록 반드시 처리(절단)해야 합니다.
- 차량 CAN은 이미 종단이 구성되어 있어 중복 종단 시 통신 오류가 발생합니다.

추가 체크리스트:

- CAN-H/CAN-L이 뒤바뀌면 통신이 되지 않습니다.
- 배선 길이는 가능한 짧게 유지하고, 접촉 불량이 없도록 고정하세요.
- 전원 인가 전 멀티미터로 단락(쇼트) 여부를 먼저 확인하세요.

## 동작 확인 방법

업로드 후 Arduino Serial Monitor를 `115200 baud`로 열어 확인합니다.

- FSD 상태
- 현재 속도 프로파일
- CAN 처리 로그

로그가 너무 많으면 코드에서 `enablePrint = false`로 변경하세요.

## 속도 프로파일 표

### LEGACY (HW3 Retrofit)

LEGACY는 팔로우 거리 대신 속도 오프셋(km/h)로 프로파일을 선택합니다.

| Speed Offset (km/h) | Profile |
| ------------------- | ------- |
| 28                  | Chill   |
| 29                  | Normal  |
| 30                  | Hurry   |

### HW3 / HW4

| Distance | HW3    | HW4    |
| -------- | ------ | ------ |
| 2        | Hurry  | Max    |
| 3        | Normal | Hurry  |
| 4        | Chill  | Normal |
| 5        | -      | Chill  |
| 6        | -      | Sloth  |

## 가장 많이 막히는 포인트 (빠른 점검)

- 업로드 실패: 보드/포트 선택이 맞는지 확인
- CAN 통신 불량: CAN-H/CAN-L 극성, 커넥터 핀 번호, 종단 저항 중복 확인
- 동작 안 함: RP2040 코드는 `#define HW`, 우노 코드는 `#define HW_TARGET` 설정이 맞는지 확인
- 로그 없음: 시리얼 속도 `115200`, `enablePrint` 값 확인
- `setBitrate failed` 발생: MCP2515 클럭 값이 모듈 실물(8MHz/16MHz)과 일치하는지 확인

## 면책 고지

이 프로젝트 사용으로 인해 발생하는 차량 손상, 사고, 법적 문제 등에 대해 작성자는 책임지지 않습니다.

- 지역 법규 및 도로 안전 규정을 반드시 준수하세요.
- 차량 보증에 영향을 줄 수 있습니다.

## 라이선스

이 프로젝트는 `GNU General Public License v3.0`을 따릅니다.

- https://www.gnu.org/licenses/gpl-3.0.html

# I2C 센서: MPU-6050, SHT31, BH1750

## MPU-6050 (6축 IMU) — D1
| 항목 | 값 |
|---|---|
| 주소 | 1101000 (AD0 Low), 1101001 (AD0 High) |
| I2C | 400 kHz (Fast), 100 kHz (Standard) |
| 전원 | VDD 2.375~3.46 V, VLOGIC 1.71 V ~ VDD |
| 자이로 | ±250/±500/±1000/±2000 °/s, 감도 131 / 65.5 / 32.8 / 16.4 LSB/(°/s) |
| 가속도 | ±2/±4/±8/±16 g, 감도 16,384 / 8,192 / 4,096 / 2,048 LSB/g |
| ADC | 자이로·가속도 각 16비트 |
| 온도 | 감도 340 LSB/°C, 35 °C에서 오프셋 -521 LSB, 선형성 ±1 °C. 변환식은 이번 문서에 없음 (미확인) |
| FIFO | 1024바이트 |
| 출력률 | 자이로 8 kHz(DLPF 꺼짐) 또는 1 kHz(DLPF 켜짐). 샘플률 = 출력률 / (1 + SMPLRT_DIV) |

레지스터 (Register Map Rev 3.2, 확인): SMPLRT_DIV 0x19 · CONFIG 0x1A · GYRO_CONFIG 0x1B(FS_SEL 비트 4:3) · ACCEL_CONFIG 0x1C(AFS_SEL 비트 4:3) · INT_ENABLE 0x38 · INT_STATUS 0x3A · ACCEL_XOUT_H 0x3B · TEMP_OUT_H 0x41 · GYRO_XOUT_H 0x43 · PWR_MGMT_1 0x6B(리셋 0x40, SLEEP=비트 6) · WHO_AM_I 0x75(0x68). 그 외 모든 레지스터 리셋값 0x00. 전원 투입 후 sleep 상태라 PWR_MGMT_1 의 SLEEP 해제가 먼저입니다.
계산: ±2 g 범위에서 raw 16384 = 1 g. 연결 확인: 0x75 읽기 → 0x68.
SoC: AXI IIC 400 kHz, AD0 High(0x69) 권장(DS3231 과 충돌 회피), INT 는 GPIO.

## SHT31 (SHT3x-DIS, 온습도) — D1
| 항목 | 값 |
|---|---|
| 주소 | 0x44 (ADDR Low, 기본), 0x45 (ADDR High) |
| I2C | 최대 1000 kHz |
| 전원 | 2.15 / 3.3 typ / 5.5 V |
| 1회 측정 명령 (클럭 스트레칭 켬 / 끔) | High 0x2C06 / 0x2400 · Medium 0x2C0D / 0x240B · Low 0x2C10 / 0x2416 |
| 소프트 리셋 | 0x30A2 |
| 상태 레지스터 읽기 | 0xF32D |
| CRC-8 | 다항식 0x31, 초기값 0xFF, 반사 없음, 최종 XOR 0x00, CRC(0xBEEF)=0x92 |
| 측정 시간 (2.4~5.5 V) | Low 전형 2.5 / 최대 4 ms · Medium 4.5 / 6 ms · High 12.5 / 15 ms |
| 정확도 (전형) | SHT31: 습도 2 %RH, 온도 0.2 °C (0~90 °C) |
| 변환식 | T = -45 + 175 × S_T / (2^16 - 1) °C, RH = 100 × S_RH / (2^16 - 1) %. 추출 텍스트의 지수가 깨져 분모는 재구성한 값 (원본 PDF로 재확인 필요) |

응답 형식(6바이트: T 2바이트+CRC, RH 2바이트+CRC)은 CRC 규칙이 워드마다 붙는다는 점은 설계안으로 적고, 바이트 배치는 이번에 표를 보지 못해 미확인.
SoC: AXI IIC. 클럭 스트레칭 끔 명령 + 15 ms 대기 후 읽기(설계안, 폴링).

## BH1750FVI (조도) — D1
| 항목 | 값 |
|---|---|
| 주소 | 0100011 (ADDR Low), 1011100 (ADDR High) |
| I2C | 최대 400 kHz |
| 전원 | 2.4 / 3.0 typ / 3.6 V, 절대 최대 4.5 V |
| 명령 | Power Down 0x00 · Power On 0x01 · Reset 0x07 · Continuous H 0x10 · H2 0x11 · L 0x13 · One Time H 0x20 · H2 0x21 · L 0x23 |
| 분해능 | H 1 lx, H2 0.5 lx, L 4 lx |
| 측정 시간 | H typ 120 / max 180 ms, L typ 16 / max 24 ms, H2 typ 120 ms |
| 출력 | lux = count / 1.2, 범위 1~65535 lx (광창 보정 시 0.11~100000 lx) |

주의: Reset 은 0x07 입니다. 흔히 0x03 으로 알려졌으나 데이터시트 표(0000_0111)와 다릅니다.
SoC: AXI IIC. 연속 H 모드(0x10) 후 180 ms 이후 2바이트 읽기(설계안).

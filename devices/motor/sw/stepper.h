/* 28BYJ-48 스테퍼 + ULN2003 보드 드라이버 (교재 27장 대응). AXI GPIO(PG144) 4핀으로 코일 4개를 직접 구동한다.
 *
 * 근거와 등급 (자세한 표: motor/ref/stepper_facts.txt, 문서: motor/28byj-48.md)
 *  - 정격·주파수: 28BYJ-48 데이터시트 사본(pdf.direnc.net, WebFetch 요약 경유 D1): 5 V, 4상, "5.625°/64", 감속 1/64,
 *    동작 주파수 100 Hz, 무부하 시동(pull-in) > 600 Hz, 무부하 탈조(pull-out) > 1000 Hz, 코일 저항 50 Ω ±7 %.
 *    여자 순서표(핀 대응)는 이 사본에 없었다 → 아래 순서는 2차 자료(D3) 두 건에서 가져옴.
 *  - 시퀀스(D3): Arduino Stepper 라이브러리 소스(Stepper.cpp 사본) 4선 모드 case 0..3 = 1010, 0110, 0101, 1001 (핀 1..4).
 *    CheapStepper(GitHub README) 반스텝 8단계 "A-AB-B-BC-C-CD-D-DA", 4096 미니스텝/회전, 실측 감속비 63.68395(→ 약 4076).
 *    Stepper 라이브러리의 핀 순서로 28BYJ-48 을 쓸 때는 핀 2·3 을 맞바꾸는 것이 관례(8,10,9,11) — 그렇게 읽으면
 *    코일 순서가 (IN1,IN2) → (IN2,IN3) → (IN3,IN4) → (IN4,IN1) = AB, BC, CD, DA 가 되어 CheapStepper 와 맞는다(계산: 두 자료의 일관성 확인).
 *    A..D = ULN2003 보드의 IN1..IN4 라는 대응은 CheapStepper 의 배선 그림을 읽지 못해 확정하지 못했다(설계안).
 *  - 회전 수(계산): 모터 32 스텝(풀·웨이브, 11.25°) 또는 64 스텝(하프, 5.625°) × 감속비. 64:1 이면 2048 / 4096, 63.68395:1 이면 약 2037.89 / 4075.77.
 *  - ULN2003A 입력(TI SLRS027, 요약 경유 D1): VI(on) = 2.4 V(IC 200 mA)·2.7 V(250 mA)·3 V(300 mA) → 3.3 V GPIO 로 구동 가능(최대 하이 레벨 가정).
 * 미확인: 코일 4개 중 어느 것이 IN 몇 번인지의 확정(회로도 미열람), 회전 방향(정방향 = IN1→IN2→IN3→IN4 순서이고, 실제 시계/반시계는 배선에 달림),
 *  부하 시 안전한 최고 속도(데이터시트는 무부하 값만), 기어 백래시, 모터 전류·온도, 전원 5 V 외부 공급의 GND 공통.
 * 보드에서 실행해 보지 않았다(호스트 모의 레지스터 시험만). 모의 코일 모델은 위 표를 옮긴 자작이다. */
#ifndef STEPPER_H
#define STEPPER_H
#include <stdint.h>
#include "axi_gpio.h"

#define STEPPER_OK        0
#define STEPPER_ERR_ARGS  (-1)
#define STEPPER_ERR_SPEED (-2)   /* 속도 미설정, 0, 또는 한계 주파수 초과 */
#define STEPPER_ERR_BUSY  (-3)   /* 이동 중에 새 이동 */

/* 사실표(motor/ref/stepper_facts.txt)와 대조되는 상수 */
#define STEPPER_MOTOR_STEPS_FULL   32u        /* 모터 축 1회전당 풀·웨이브 스텝 (11.25°) */
#define STEPPER_MOTOR_STEPS_HALF   64u        /* 하프 스텝 (5.625°) */
#define STEPPER_GEAR_NOMINAL_MICRO 64000000u  /* 64:1 (표기값), 1e-6 단위 */
#define STEPPER_GEAR_MEASURED_MICRO 63683950u /* 63.68395:1 (CheapStepper 실측값) */
#define STEPPER_HZ_OPERATING       100u       /* 데이터시트 동작 주파수 */
#define STEPPER_HZ_PULL_IN         600u       /* 무부하 시동 주파수(> 600) */
#define STEPPER_HZ_PULL_OUT        1000u      /* 무부하 탈조 주파수(> 1000) */
#define STEPPER_PHASES             4u
#define STEPPER_PAT_A   0x1u    /* 코일 상태 8단계: 비트 0 = IN1 ... 비트 3 = IN4 */
#define STEPPER_PAT_AB  0x3u
#define STEPPER_PAT_B   0x2u
#define STEPPER_PAT_BC  0x6u
#define STEPPER_PAT_C   0x4u
#define STEPPER_PAT_CD  0xCu
#define STEPPER_PAT_D   0x8u
#define STEPPER_PAT_DA  0x9u

typedef enum {
    STEPPER_WAVE = 0,   /* 한 번에 코일 1개: A,B,C,D (토크 작음, 전류 적음) */
    STEPPER_FULL = 1,   /* 한 번에 코일 2개: AB,BC,CD,DA (Stepper 라이브러리 4선 모드와 같은 순서) */
    STEPPER_HALF = 2    /* 8단계: A,AB,B,BC,C,CD,D,DA (분해능 2배) */
} stepper_mode_t;

typedef struct {
    gpio_t *g;
    unsigned ch, shift;          /* IN1..IN4 = GPIO 채널 ch 의 비트 shift..shift+3 (연속 4비트) */
    stepper_mode_t mode;
    uint8_t idx;                 /* 8단계 표에서의 현재 위치 0..7 (A=0 ... DA=7). 모드에 따라 짝수/홀수만 쓴다 */
    uint8_t energized;           /* 코일에 전류가 흐르는 상태인가 */
    uint8_t release_on_done;     /* 이동이 끝나면 코일을 끈다(발열 방지, 정지 토크 포기) */
    uint64_t gear_micro;         /* 감속비 ×1e6 */
    uint32_t max_hz;             /* 허용 스텝 주파수 */
    uint32_t interval_us;        /* 스텝 간격(0 = 속도 미설정) */
    uint32_t next_us;            /* 다음 스텝 시각 */
    int32_t remaining;           /* 남은 스텝(부호 = 방향) */
    int32_t pos;                 /* 누적 스텝(모드 단위, 정방향 +1) */
} stepper_t;

/* IN1..IN4 4핀을 출력(모두 Low = 코일 꺼짐)으로. 기본: 64:1, 한계 STEPPER_HZ_PULL_IN. 이후 모드 FULL */
int stepper_init(stepper_t *s, gpio_t *g, unsigned ch, unsigned shift);
/* 이동 중이면 STEPPER_ERR_BUSY. 모드를 바꾸면 pos 를 0 으로, 속도를 "미설정"으로 되돌린다(스텝 수가 달라지므로 다시 정해야 함). 코일은 건드리지 않고,
 * 다음 스텝이 새 모드의 짝/홀에 맞춰 요청 방향으로 반 스텝만 가서 정렬한 뒤 이어서 간다 */
int stepper_set_mode(stepper_t *s, stepper_mode_t mode);
/* 감속비 ×1e6 (예: 63683950). 0 이거나 1e6 미만, 1e9 초과는 오류. 이동 중이면 BUSY */
int stepper_set_gear_micro(stepper_t *s, uint64_t gear_micro);
/* 한계 주파수 [Hz]: 1..STEPPER_HZ_PULL_OUT. 이미 설정한 속도가 새 한계를 넘으면 오류(속도는 그대로) */
int stepper_set_max_hz(stepper_t *s, uint32_t hz);
/* 속도: 출력축 milli-rpm (1000 = 1 rpm). 현재 모드·감속비로 스텝 간격을 계산한다. 한계를 넘거나 0 이면 STEPPER_ERR_SPEED */
int stepper_set_speed_mrpm(stepper_t *s, uint32_t mrpm);
/* 모드 단위 1회전 스텝 수 ×1e6 (계산용 공개) */
uint64_t stepper_steps_per_rev_micro(const stepper_t *s);

/* 즉시 한 스텝(dir>0 정방향, <0 역방향, 0 은 오류). 속도 제어 없음 — 호출 간격은 호출자가 책임진다 */
int stepper_step(stepper_t *s, int dir);
/* 비차단 이동: steps 부호 = 방향. 첫 스텝은 now_us 에 바로 나가고 이후 interval_us 간격. 속도 미설정이면 STEPPER_ERR_SPEED, 이동 중이면 BUSY */
int stepper_move(stepper_t *s, int32_t steps, uint32_t now_us);
/* 출력축 각도 [0.1°] 만큼 이동(반올림, 부호 = 방향). 한 스텝보다 작으면 0 스텝(= 아무것도 안 함) */
int stepper_move_deg10(stepper_t *s, int32_t deg10, uint32_t now_us);
/* now_us 에 스텝 시각이 되었으면 한 스텝 진행. 반환: 실행한 스텝 수(0/1). 한 간격 이상 밀려 다음 스텝 시각도 이미 지났으면 몰아치지 않고 now 부터 다시 간격을 센다 */
int stepper_poll(stepper_t *s, uint32_t now_us);
int32_t stepper_remaining(const stepper_t *s);
void stepper_stop(stepper_t *s);          /* 남은 이동을 취소(코일 상태는 유지) */
int stepper_release(stepper_t *s);        /* 코일을 끈다(이동 취소 포함) */
int stepper_hold(stepper_t *s);           /* 현재 위치의 코일 상태를 켠다(정지 토크) */
void stepper_set_release_on_done(stepper_t *s, int on);
uint8_t stepper_pattern(unsigned idx);    /* 8단계 표 값(idx & 7) */
#endif

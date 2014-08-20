#include "stm32f30x.h"
#include "stm32f3_discovery.h"
#include "motor.h"
#include "helpers.h"

/* Private variables ---------------------------------------------------------*/
__IO uint16_t current_motor_speeds[4] = {MIN_SERVO_TIME, MIN_SERVO_TIME, MIN_SERVO_TIME, MIN_SERVO_TIME};
__IO uint16_t target_motor_speeds[4] = {MIN_SERVO_TIME, MIN_SERVO_TIME, MIN_SERVO_TIME, MIN_SERVO_TIME};
uint16_t n_int_run = 0;
uint8_t m_int_run = 0;

NVIC_InitTypeDef NVIC_InitStructure;

/*  Private functions */
inline uint16_t limitServo(uint16_t s);
void ServoPortInit();
void ServoTimerInit();
void SetDirectMotorSpeeds(uint16_t *speeds);


/*
 * Poortindeling:
 * PD3 - CH1 - grijs
 * PD4 - CH2 - wit
 * PD6 - CH4 - bruin
 * PD7 - CH3 - groen
 *
 * bruin - rechtsonder
 * grijs - linksboven
 * groen - linksonder
 * wit   - rechtboven
 */

void StartupMotors() {
    uint16_t MotorSpeedsP1[4] = {0,0,0,0};
    // uint16_t MotorSpeedsP2[4] = {150,500,150,150};
    // uint16_t MotorSpeedsP3[4] = {1100,1100,1100,1100};
    uint8_t i;
    ServoPortInit();
    ServoTimerInit();
    SetDirectMotorSpeeds( MotorSpeedsP1 );
    for (i=0xFF; i; i>>=1) {
	LedByte(i);
	Delay(50);
    }
    /*
    LedByte(i);
    SetDirectMotorSpeeds( MotorSpeedsP2 );
    Delay(80);
    SetDirectMotorSpeeds( MotorSpeedsP1 );
    Delay(70);
    */
}

void ServoPortInit() {
    GPIO_InitTypeDef GPIO_InitStructure;

    // Enable clock on the system-bus to the timer
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_AHBPeriphClockCmd( RCC_AHBPeriph_GPIOD, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

    GPIO_Init(GPIOD, &GPIO_InitStructure);

    GPIO_PinAFConfig(GPIOD, GPIO_PinSource3, GPIO_AF_2);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource4, GPIO_AF_2);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource6, GPIO_AF_2);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource7, GPIO_AF_2);

}
void ServoTimerInit() {
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    // Configure timer:
    // System frequency is 72MHz
    // The motor controller wants a pulse every 20ms ( 50Hz )
    // The width of the pulse varies between 1ms and 2ms
    // When we want to be able to have a resolution of 1us (=0.1%), that means
    // the counter needs to count to 20ms/1us = 20000 steps and has a frequency of 1MHz
    // This means a prescaler of 72MHz/1MHz - 1 = 72 - 1 = 71
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Prescaler = 71; //(SystemCoreClock / 10000) - 1; // 71
    TIM_TimeBaseStructure.TIM_Period = 18474 - 1; // 1Mhz / 20000 = 50Hz
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    // Setup the Output Compare stages for 4 channels
    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;
    TIM_OCInitStructure.TIM_Pulse = 1000; // 1000us as starting pulse width
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset;

    TIM_OC1Init(TIM2, &TIM_OCInitStructure);
    TIM_OC2Init(TIM2, &TIM_OCInitStructure);
    TIM_OC3Init(TIM2, &TIM_OCInitStructure);
    TIM_OC4Init(TIM2, &TIM_OCInitStructure);

    // TIM2_IT_CC2 is used by the ADC to convert voltages in sync
    TIM_ITConfig(TIM2, TIM_IT_CC2, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
    TIM_CtrlPWMOutputs(TIM2, ENABLE);

    // Setup the timer-interupt which updates the PWM-width each cycle, if needed
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/* Increase the speed with a limited amount each cycle, decreases are not limited */
void TIM2_IRQHandler(void) {
    if (TIM_GetITStatus(TIM2, TIM_IT_CC2) != RESET) {
	TIM_ClearITPendingBit(TIM2, TIM_IT_CC2);
	if (target_motor_speeds[0] != current_motor_speeds[0]) {
	    if ( target_motor_speeds[0] > (current_motor_speeds[0] + MAX_SLEW)) {
		current_motor_speeds[0] += MAX_SLEW;
	    } else {
		current_motor_speeds[0] = target_motor_speeds[0];
	    }
	    TIM_SetCompare1(TIM2, current_motor_speeds[0]);
	}
	if (target_motor_speeds[1] != current_motor_speeds[1]) {
	    if ( target_motor_speeds[1] > (current_motor_speeds[1] + MAX_SLEW)) {
		current_motor_speeds[1] += MAX_SLEW;
	    } else {
		current_motor_speeds[1] = target_motor_speeds[1];
	    }
	    TIM_SetCompare2(TIM2, current_motor_speeds[1]);
	}
	if (target_motor_speeds[2] != current_motor_speeds[2]) {
	    if ( target_motor_speeds[2] > (current_motor_speeds[2] + MAX_SLEW)) {
		current_motor_speeds[2] += MAX_SLEW;
	    } else {
		current_motor_speeds[2] = target_motor_speeds[2];
	    }
	    TIM_SetCompare3(TIM2, current_motor_speeds[2]);
	}
	if (target_motor_speeds[3] != current_motor_speeds[3]) {
	    if ( target_motor_speeds[3] > (current_motor_speeds[3] + MAX_SLEW)) {
		current_motor_speeds[3] += MAX_SLEW;
	    } else {
		current_motor_speeds[3] = target_motor_speeds[3];
	    }
	    TIM_SetCompare4(TIM2, current_motor_speeds[3]);
	}
    }
}

/* Expects an array size 4 which indicates how more the pulsewidth should be above minimum*/
void SetMotorSpeeds(uint16_t *speeds) {
    target_motor_speeds[0] = limitServo(speeds[0]);
    target_motor_speeds[1] = limitServo(speeds[1]);
    target_motor_speeds[2] = limitServo(speeds[2]);
    target_motor_speeds[3] = limitServo(speeds[3]);
}

/* Expects an array size 4 which will receive the current speeds */
void GetMotorSpeeds(uint16_t *speeds) {
    speeds[0] = current_motor_speeds[0]-MIN_SERVO_TIME;
    speeds[1] = current_motor_speeds[1]-MIN_SERVO_TIME;
    speeds[2] = current_motor_speeds[2]-MIN_SERVO_TIME;
    speeds[3] = current_motor_speeds[3]-MIN_SERVO_TIME;
}

/* When the servo has to be set to a speed right now, w/o safety/delay */
void SetDirectMotorSpeeds(uint16_t *speeds) {
    TIM_SetCompare1(TIM2, limitServo(speeds[0]));
    TIM_SetCompare2(TIM2, limitServo(speeds[1]));
    TIM_SetCompare3(TIM2, limitServo(speeds[2]));
    TIM_SetCompare4(TIM2, limitServo(speeds[3]));
    current_motor_speeds[0] = speeds[0];
    current_motor_speeds[1] = speeds[1];
    current_motor_speeds[2] = speeds[2];
    current_motor_speeds[3] = speeds[3];
}

inline uint16_t limitServo(uint16_t s) {
    return s>(MAX_SERVO_TIME - MIN_SERVO_TIME)?MAX_SERVO_TIME:s+MIN_SERVO_TIME;
}


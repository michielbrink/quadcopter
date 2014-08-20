#include "stm32f30x.h"
#include "stm32f3_discovery.h"
#include "helpers.h"

// Private declarations
  RCC_ClocksTypeDef RCC_Clocks;
__IO uint32_t TimingDelay = 0;
void TimingDelay_Decrement();

/* Initializes the timers */
void Init_Helpers() {
    time_since_startup = 0;

    RCC_GetClocksFreq(&RCC_Clocks);
    SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);

    STM_EVAL_LEDInit(LED3);
    STM_EVAL_LEDInit(LED4);
    STM_EVAL_LEDInit(LED5);
    STM_EVAL_LEDInit(LED6);
    STM_EVAL_LEDInit(LED7);
    STM_EVAL_LEDInit(LED8);
    STM_EVAL_LEDInit(LED9);
    STM_EVAL_LEDInit(LED10);
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
  TimingDelay_Decrement();
  time_since_startup++;
}

/**
  * @brief  Inserts a delay time.
  * @param  nTime: specifies the delay time length, in 10 ms.
  * @retval None
  */
void Delay(__IO uint32_t nTime)
{
  TimingDelay = nTime;

  while(TimingDelay != 0);
}

/**
  * @brief  Decrements the TimingDelay variable.
  * @param  None
  * @retval None
  */
void TimingDelay_Decrement(void)
{
  if (TimingDelay != 0x00)
  { 
    TimingDelay--;
  }
}

void LedByte(uint8_t info) {
    if (info & 0x01) {
	STM_EVAL_LEDOn(LED3);
    } else {
	STM_EVAL_LEDOff(LED3);
    }
    if (info & 0x02) {
	STM_EVAL_LEDOn(LED5);
    } else {
	STM_EVAL_LEDOff(LED5);
    }
    if (info & 0x04) {
	STM_EVAL_LEDOn(LED7);
    } else {
	STM_EVAL_LEDOff(LED7);
    }
    if (info & 0x08) {
	STM_EVAL_LEDOn(LED9);
    } else {
	STM_EVAL_LEDOff(LED9);
    }
    if (info & 0x10) {
	STM_EVAL_LEDOn(LED10);
    } else {
	STM_EVAL_LEDOff(LED10);
    }
    if (info & 0x20) {
	STM_EVAL_LEDOn(LED8);
    } else {
	STM_EVAL_LEDOff(LED8);
    }
    if (info & 0x40) {
	STM_EVAL_LEDOn(LED6);
    } else {
	STM_EVAL_LEDOff(LED6);
    }
    if (info & 0x80) {
	STM_EVAL_LEDOn(LED4);
    } else {
	STM_EVAL_LEDOff(LED4);
    }
}

void LedAngle(float angle) {
    uint8_t lb = 1;
    while (angle>=360.0)
	angle -= 360.0;
    while (angle<0.0)
	angle += 360.0;
    while (angle>45) {
	lb<<=1;
	angle -= 45;
    }
    LedByte(lb);
}

void Inclination2Led(fVector3* accData) {
    uint8_t m = 0x00;
    if ( ((accData->x * accData->x) + (accData->y * accData->y)) < ( MIN_INCL_ACC * MIN_INCL_ACC) ) {
	LedByte(0);
	return;
    } else { 
	if ( accData->x < -MIN_INCL_ACC) {
	    m |= 0x10;
	} else if (accData->x > MIN_INCL_ACC) {
	    m |= 0x01;
	}
	if ( accData->y < -MIN_INCL_ACC) {
	    m |= 0x04;
	} else if (accData->y > MIN_INCL_ACC) {
	    m |= 0x40;
	}
	LedByte(m);
    }
}

void add(fVector3* a, fVector3 b) {
    a->x += b.x;
    a->y += b.y;
    a->z += b.z;
}
void sub(fVector3* a, fVector3 b) {
    a->x -= b.x;
    a->y -= b.y;
    a->z -= b.z;
}

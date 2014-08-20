#include "stm32f30x.h"
#include "stm32f3_discovery.h"
#include "adc.h"

void AdcConfig() {
    ADC_InitTypeDef ADC_InitStruct;
    ADC_InitStruct.ADC_ContinuousConvMode = ENABLE;
    ADC_InitStruct.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStruct.ADC_ExternalTrigConvEvent = ADC_ExternalTrigConvEvent_3; // = TIM2_CC2 : stm32f3-reference-manual.pdf page 221
    ADC_InitStruct.ADC_ExternalTrigEventEdge = ADC_ExternalTrigInjecEventEdge_RisingEdge;
    ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStruct.ADC_OverrunMode = ENABLE;
    ADC_InitStruct.ADC_AutoInjMode = DISABLE;
    ADC_InitStruct.ADC_NbrOfRegChannel = ADC_NUM_INPUTS;

    /* Configure the ADC clock */
    RCC_ADCCLKConfig(RCC_ADC12PLLCLK_Div2);

    /* Enable ADC1 clock */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_ADC12, ENABLE);

    /* ADC Channel configuration */
    /* GPIOC Periph clock enable */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);

    ADC_Init(ADC1, &ADC_InitStruct);

    // ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 
}

void AdcReadValues(uint32_t* t, uint16_t* v) {

}

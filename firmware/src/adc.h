#ifndef ADC_DEFINITIONS
#define ADC_DEFINITIONS

#include "helpers.h"

#define ADC_NUM_INPUTS 5

void AdcConfig();
void AdcReadValues(uint32_t* t, uint16_t* v);

#endif

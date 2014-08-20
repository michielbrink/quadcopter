#include "helpers.h"

void Gyro_Config( void );
uint8_t Gyro_ReadData(fVector3* pfData);

#define NEW_GYRO_DATA 0x04

//  * 1: 95Hz, 2: 190Hz, 3: 380Hz, 4: 760Hzu
#define GYRO_SAMPLE_RATE ((float)380.0)

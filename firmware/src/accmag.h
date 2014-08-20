#include "helpers.h"

void AccMagConfig();
uint8_t AccReadData(fVector3* accData);
uint8_t MagReadData(fVector3* magData);
uint8_t AccMagReadData(fVector3* accData, fVector3* magData);

// Own constants
#define DRDY_XYZ ((uint8_t)0x10)
#define ACC_SCALE LSM303DLHC_FULLSCALE_4G
#define ACC_RESOLUTION ((float)0.002)
#define NEW_ACC_DATA 0x01
#define NEW_MAG_DATA 0x02

#include "stm32f30x.h"
#include "stm32f3_discovery.h"
#include "stm32f3_discovery_l3gd20.h"
#include "stm32f3_discovery_lsm303dlhc.h"
#include "accmag.h"
#include "helpers.h"

void AccMagConfig( ) {
    LSM303DLHCAcc_InitTypeDef LSM303_AccConf = {
	.Power_Mode          = LSM303DLHC_NORMAL_MODE,
	.AccOutput_DataRate  = LSM303DLHC_ODR_1344_HZ,
	.Axes_Enable         = LSM303DLHC_AXES_ENABLE,
	.High_Resolution     = LSM303DLHC_HR_ENABLE,
	.BlockData_Update    = LSM303DLHC_BlockUpdate_Single,
	.Endianness          = LSM303DLHC_BLE_LSB,
	.AccFull_Scale       = ACC_SCALE,
    };
    LSM303DLHCAcc_FilterConfigTypeDef LSM303_AccFilterConf = {
	.HighPassFilter_Mode_Selection   = LSM303DLHC_HPM_NORMAL_MODE_RES,
	.HighPassFilter_CutOff_Frequency = LSM303DLHC_HPFCF_8,
	.HighPassFilter_AOI1             = LSM303DLHC_HPF_AOI1_DISABLE,
	.HighPassFilter_AOI2             = LSM303DLHC_HPF_AOI2_DISABLE,
    };

    LSM303DLHCMag_InitTypeDef LSM303_MagConf = {
	.Temperature_Sensor = LSM303DLHC_TEMPSENSOR_DISABLE,
	.MagOutput_DataRate = LSM303DLHC_ODR_30_HZ,
	.Working_Mode       = LSM303DLHC_CONTINUOS_CONVERSION,
	.MagFull_Scale      = LSM303DLHC_FS_8_1_GA,
    };

    LSM303DLHC_AccInit(&LSM303_AccConf);
    LSM303DLHC_AccFilterConfig(&LSM303_AccFilterConf);
    LSM303DLHC_MagInit(&LSM303_MagConf);
}


uint8_t AccReadData(fVector3* accData) {
    uint8_t data_status;
    uint8_t tmpBuf[6];

    data_status = LSM303DLHC_AccGetDataStatus();

    if (data_status & DRDY_XYZ) {
	LSM303DLHC_Read(ACC_I2C_ADDRESS, LSM303DLHC_OUT_X_L_A, &(tmpBuf[0]), 6);
	accData->t = time_since_startup;
	accData->x = ((float)((int16_t)((uint16_t)tmpBuf[1] << 8) + tmpBuf[0]))/800.0;
	accData->y = ((float)((int16_t)((uint16_t)tmpBuf[3] << 8) + tmpBuf[2]))/800.0;
	accData->z = ((float)((int16_t)((uint16_t)tmpBuf[5] << 8) + tmpBuf[4]))/800.0;
	return NEW_ACC_DATA;
    } else {
	return 0;
    }
}
uint8_t MagReadData(fVector3 *magData) {
    uint8_t data_status;
    uint8_t tmpBuf[6];

    data_status = LSM303DLHC_MagGetDataStatus();

    if (data_status & DRDY_XYZ) {
	LSM303DLHC_Read(MAG_I2C_ADDRESS, LSM303DLHC_OUT_X_H_M, &(tmpBuf[0]), 6);
	magData->t = time_since_startup;
	magData->x = ((float)((int16_t)((uint16_t)tmpBuf[1] << 8) + tmpBuf[0]));
	magData->y = ((float)((int16_t)((uint16_t)tmpBuf[3] << 8) + tmpBuf[2]));
	magData->z = ((float)((int16_t)((uint16_t)tmpBuf[5] << 8) + tmpBuf[4]));
	return NEW_MAG_DATA;
    } else {
	return 0;
    }
}
uint8_t AccMagReadData(fVector3* accData, fVector3* magData) {
    return ( AccReadData(accData) | MagReadData(magData) );
}

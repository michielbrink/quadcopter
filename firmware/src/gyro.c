#include "stm32f30x.h"
#include "stm32f3_discovery.h"
#include "stm32f3_discovery_l3gd20.h"
#include "stm32f3_discovery_lsm303dlhc.h"
#include "gyro.h"
#include "helpers.h"

// Private variables
float static_offset[3] = {0.0, 0.0, 0.0};

void Gyro_Config(void) {
    L3GD20_InitTypeDef L3GD20_ConfigS = {
	.Power_Mode		= L3GD20_MODE_ACTIVE,
	.Output_DataRate	= L3GD20_OUTPUT_DATARATE_4,
	.Axes_Enable		= L3GD20_AXES_ENABLE,
	.Band_Width		= L3GD20_BANDWIDTH_1,
	.BlockData_Update	= L3GD20_BlockDataUpdate_Single,
	.Endianness		= L3GD20_BLE_LSB,
	.Full_Scale		= L3GD20_FULLSCALE_250,
    };

    L3GD20_FilterConfigTypeDef L3GD20_FilterConfigS = {
	.HighPassFilter_Mode_Selection = L3GD20_HIGHPASSFILTER_ENABLE,
	.HighPassFilter_CutOff_Frequency = L3GD20_HPFCF_9,
    };

    /* Initialize the accelerometer */
    L3GD20_Init(&L3GD20_ConfigS);
    L3GD20_FilterConfig(&L3GD20_FilterConfigS);
}

/**
* @brief Read LSM303DLHC output register, and calculate the acceleration ACC=(1/SENSITIVITY)* (out_h*256+out_l)/16 (12 bit rappresentation)
* @param pnData: pointer to float buffer where to store data
* @retval None
*/
uint8_t Gyro_ReadData(fVector3* pfData)
{
  int16_t pnRawData[3];
  uint8_t ctrlx[2];
  uint8_t status[1];
  float L3GD20_Rot_Sensitivity = L3GD20_Rot_Sensitivity_250;
  uint8_t buffer[6], cDivider;
  uint8_t i = 0;

  /* Read from the status-register if new data has been generated */
  L3GD20_Read( ctrlx, L3GD20_STATUS_REG_ADDR, 1);
  if (!(ctrlx[0])) {
      return 0;
  }
  
  /* Read the register content */
  L3GD20_Read( ctrlx, L3GD20_CTRL_REG1_ADDR, 2); 
  L3GD20_Read( buffer, L3GD20_OUT_X_L_ADDR, 6);
  
  if(ctrlx[1]&0x40)
    cDivider=64;
  else
    cDivider=16;

  /* check in the control register4 the data alignment*/
  if(!(ctrlx[0] & 0x40) || (ctrlx[1] & 0x40)) /* Little Endian Mode or FIFO mode */
  {
    for(i=0; i<3; i++)
    {
      pnRawData[i]=((int16_t)((uint16_t)buffer[2*i+1] << 8) + buffer[2*i])/cDivider;
    }
  }
  else /* Big Endian Mode */
  {
    for(i=0; i<3; i++)
      pnRawData[i]=((int16_t)((uint16_t)buffer[2*i] << 8) + buffer[2*i+1])/cDivider;
  }
  /* Read the register content */
  L3GD20_Read(ctrlx,L3GD20_CTRL_REG4_ADDR, 2);

  if(ctrlx[1]&0x40)
  {
    /* FIFO mode */
    L3GD20_Rot_Sensitivity = L3GD20_Rot_Sensitivity_250;
  }
  else
  {
    /* normal mode */
    /* switch the sensitivity value set in the CRTL4*/
    switch(ctrlx[0] & 0x30)
    {
    case L3GD20_FULLSCALE_250:
      L3GD20_Rot_Sensitivity = L3GD20_Rot_Sensitivity_250 / GYRO_SAMPLE_RATE;
      break;
    case L3GD20_FULLSCALE_500:
      L3GD20_Rot_Sensitivity = L3GD20_Rot_Sensitivity_500 / GYRO_SAMPLE_RATE;
      break;
    case L3GD20_FULLSCALE_2000:
      L3GD20_Rot_Sensitivity = L3GD20_Rot_Sensitivity_2000 / GYRO_SAMPLE_RATE;
      break;
    }
  }

  /* Obtain the mg value for the three axis */
  pfData->t = time_since_startup;
  pfData->x =(float)pnRawData[0]*L3GD20_Rot_Sensitivity - static_offset[0];
  pfData->y =(float)pnRawData[1]*L3GD20_Rot_Sensitivity - static_offset[1];
  pfData->z =(float)pnRawData[2]*L3GD20_Rot_Sensitivity - static_offset[2];
  return NEW_GYRO_DATA;
}


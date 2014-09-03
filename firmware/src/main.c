#include "stm32f30x.h"
#include "stm32f3_discovery.h"
#include "gyro.h"
#include "accmag.h"
#include "helpers.h"
#include "motor.h"
#include "spi.h"
#include "control.h"
//#include "adc.h"
#include <string.h>


/**
  * @brief  Main program.
  * @param  None 
  * @retval None
  */
int main(void)
{
  fVector3 cur_gyro     = {0,0.0,0.0,0.0};
  fVector3 cur_angle    = {0,0.0,0.0,0.0};
  fVector3 cur_acc      = {0,0.0,0.0,0.0};
  fVector3 cur_mag      = {0,0.0,0.0,0.0};
  fVector3 target_angle = {0,0.0,0.0,0.0};
  raspiMessage recv, send;
  uint16_t received_motor_speeds[4] = {0,0,0,0};
  uint16_t control_motor_speeds[4] = {0,0,0,0};
  uint16_t max_motor_speeds[4] = {1000,1000,1000,1000};
  uint32_t cur_adc_time;
  uint16_t cur_adc_values[5] = {0,0,0,0,0};
  uint8_t new_data = 0;

  STM_OptionsStruct STM_Options, STM_NewOptions;
  STM_Options.motor_initialized = 0;
  STM_Options.sensor_streaming = 0;

  // uint16_t received_motor_speeds2[4] = {0,0,0,0};

  // Initialize all subsystems
  Init_Helpers();
  raspiComInit();
  Init_Controller();
  Gyro_Config();
  AccMagConfig();
  // AdcConfig();
   
  /* Infinite loop */
  while (1)
  {   
      new_data = Gyro_ReadData(&cur_gyro);
      // And parse the data if it was new
      if (new_data & NEW_GYRO_DATA) {
	  if (STM_Options.sensor_streaming) {
	      send.command = RASPI_CMD_GET_CUR_GYRO;
	      memcpy(send.payload, &cur_gyro, sizeof(fVector3));
	      raspiSendMessage(&send);
	  }
	  insert_gyro_value(&cur_gyro);
      }

      new_data = AccReadData(&cur_acc);
      if (new_data & NEW_ACC_DATA) {
	  if (STM_Options.sensor_streaming) {
	      send.command = RASPI_CMD_GET_CUR_ACC;
	      memcpy(send.payload, &cur_acc, sizeof(fVector3));
	      raspiSendMessage(&send);
	  }
	  insert_acc_value(&cur_acc);
      }

      new_data = MagReadData(&cur_mag);
      if (new_data & NEW_MAG_DATA) {
	  if (STM_Options.sensor_streaming) {
	      send.command = RASPI_CMD_GET_CUR_MAG;
	      memcpy(send.payload, &cur_mag, sizeof(fVector3));
	      raspiSendMessage(&send);
	  }
	  insert_mag_value(&cur_mag);
      }
      // AdcReadValues(&cur_adc_time, &cur_adc_values[0]);

      // Detect buffer-overflow on out-buffers, indicating a crashed raspi
      // Cutting motors for now
      if ( raspi_tx_num_messages == RASPI_MSG_BUF_SIZE ) {
	  insert_power_target(0);
      }

      // Update the motor-control-settings, and send updated values to the raspi
      if (update_motor_settings(&(control_motor_speeds[0])) != 0) {
	  SetMotorSpeeds(&(control_motor_speeds[0]));
	  if (STM_Options.sensor_streaming) {
	      send.command = RASPI_CMD_GET_CUR_ANGLES;
	      get_current_angle(&cur_angle);
	      memcpy(send.payload, &cur_angle, sizeof(fVector3));
	      raspiSendMessage(&send);
	  }
      }
      
      if (raspiGetMessage(&recv)) {
	  switch (recv.command) {
	      case (RASPI_CMD_GET_STM_OPTIONS):
		  send.command = RASPI_CMD_GET_STM_OPTIONS;
		  memcpy(send.payload, &STM_Options, sizeof(STM_OptionsStruct));
		  raspiSendMessage(&send);
		  break;
	      case (RASPI_CMD_SET_STM_OPTIONS):
		  memcpy(&STM_NewOptions, recv.payload, sizeof(STM_OptionsStruct));
		  if ( (STM_NewOptions.motor_initialized) && (!STM_Options.motor_initialized) ) {
		      StartupMotors();
		      STM_Options.motor_initialized = 1;
		  }
		  if (STM_NewOptions.sensor_streaming) {
		      STM_Options.sensor_streaming = 1;
		  }
		  break;
	      case (RASPI_CMD_UNSET_STM_OPTIONS):
		  if (STM_NewOptions.sensor_streaming) {
		      STM_Options.sensor_streaming = 0;
		  }
		  break;
	      case (RASPI_CMD_SET_MOTORS):
		  if (STM_Options.motor_initialized) {
		      memcpy(received_motor_speeds, recv.payload, sizeof(uint16_t));
		      insert_power_target( received_motor_speeds[0] << 2);
		      // SetMotorSpeeds(received_motor_speeds);
		  }
		  break;
	      case (RASPI_CMD_GET_MOTORS):
		  send.command = RASPI_CMD_GET_MOTORS;
		  GetMotorSpeeds(&(received_motor_speeds[0]));
		  memcpy(send.payload, received_motor_speeds, 4 * sizeof(uint16_t));
		  raspiSendMessage(&send);
		  break;
	      case (RASPI_CMD_SET_STM_LEDS):
		  LedByte(recv.payload[0]);
		  raspiSendMessage(&recv);
		  break;

	      // Angle-commands
	      case (RASPI_CMD_GET_CUR_ANGLES):
		  send.command = RASPI_CMD_GET_CUR_ANGLES;
		  get_current_angle(&cur_angle);
		  memcpy(send.payload, &cur_angle, sizeof(fVector3));
		  raspiSendMessage(&send);
		  break;
	      
	      case (RASPI_CMD_SET_TARGET_ANGLES):
		  memcpy(&target_angle, recv.payload, sizeof(fVector3));
		  insert_angle_target(&target_angle);
		  break;
	
	      // Acc-commands
	      case (RASPI_CMD_GET_CUR_ACC):
		  send.command = RASPI_CMD_GET_CUR_ACC;
		  memcpy(send.payload, &cur_acc, sizeof(fVector3));
		  raspiSendMessage(&send);
		  break;

	      // Mag-commands
	      case (RASPI_CMD_GET_CUR_MAG):
		  send.command = RASPI_CMD_GET_CUR_MAG;
		  memcpy(send.payload, &cur_mag, sizeof(fVector3));
		  raspiSendMessage(&send);
		  break;

	      default:
		  // LedByte(recv.command);
		  break;
	  }
      }
  }
}


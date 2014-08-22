/* 
 * This sets up the STM as a SPI-slave to the raspi
 *
 * Pin layout STM - Raspi:
 * CE   : PB12 - 24
 * CLK  : PB13 - 23
 * MISO : PB14 - 21
 * MOSI : PB15 - 19
 * GND  : GND  - 25
 **/

#ifndef RASPI_MSG_DEFINITIONS
#define RASPI_MSG_DEFINITIONS

#define RASPI_PAYLOAD_LENGTH 16
#define RASPI_MSG_BUF_SIZE 16

// These two command-codes should not be used, because they are used to detect wire-problems
#define RASPI_CMD_WIRE_SHORTED 0x00
#define RASPI_CMD_WIRE_BROKEN  0xFF

#define RASPI_CMD_NOP 0x01
#define RASPI_CMD_SET_MOTORS 0x02
#define RASPI_CMD_GET_MOTORS 0x04
#define RASPI_CMD_SET_STM_LEDS 0x03
#define RASPI_CMD_SET_CAM_ANGLE 0x05
#define RASPI_CMD_SET_STM_OPTIONS 0x06
#define RASPI_CMD_UNSET_STM_OPTIONS 0x07
#define RASPI_CMD_GET_STM_OPTIONS 0x08

#define RASPI_CMD_GET_CUR_ANGLES 0x20
#define RASPI_CMD_GET_CUR_GYRO 0x21
#define RASPI_CMD_SET_TARGET_ANGLES 0x22
#define RASPI_CMD_GET_TARGET_ANGLES 0x23

#define RASPI_CMD_GET_CUR_ACC 0x24

#define RASPI_CMD_GET_CUR_MAG 0x28

#define RASPI_REINIT 0x55

#define RASPI_CMD_SET_EXTRA_LEDS0 0x80
#define RASPI_CMD_SET_EXTRA_LEDS1 0x81
#define RASPI_CMD_SET_EXTRA_LEDS2 0x82

#define RASPI_CMD_ERR 0xFC
#define RASPI_CMD_STOP 0xFD
#define RASPI_CMD_START 0xFE

#define SPI_STATE_UNINITIALIZED 0x01
#define SPI_STATE_INITIALIZING  0x02
#define SPI_STATE_INITIALIZED   0x03

#endif

// Global variables
// Number of messages in the receive-queue
__IO uint8_t raspi_rx_num_messages;
// Number of messages in the send-queue
__IO uint8_t raspi_tx_num_messages;

// structs: parity will be calculated always in de raspiSendMessage function,
// so does not need to filled by the application itself
typedef struct {
    uint8_t command;
    uint8_t payload[RASPI_PAYLOAD_LENGTH];
    uint8_t parity;
} raspiMessage;

// Function definitions
void raspiComInit();
uint8_t raspiGetMessage(raspiMessage *rxBuffer);
uint8_t raspiSendMessage(raspiMessage *txBuffer);

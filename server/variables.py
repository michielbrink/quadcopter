#debug
debug_option = 1

#stm
PAYLOAD_LENGTH = 16
MAX_SPEED = 750

CMD_NOP           = 0x01
CMD_SET_MOTORS    = 0x02
CMD_SET_STM_LEDS  = 0x03
CMD_SET_CAM_ANGLE = 0x05

CMD_GET_CUR_ANGLE             = 0x20
CMD_CALIBRATE_ANGLE           = 0x21
CMD_GET_CUR_CALIBRATION_ANGLE = 0x22
CMD_SET_CUR_CALIBRATION_ANGLE = 0x23

RASPI_CMD_SET_EXTRA_LEDS0 = 0x80
RASPI_CMD_SET_EXTRA_LEDS1 = 0x81
RASPI_CMD_SET_EXTRA_LEDS2 = 0x82

#tcp
TCP_IP = ''
TCP_PORT = 5555
BUFFER_SIZE = 1024

#assign variables
motor = [0,0,0,0]
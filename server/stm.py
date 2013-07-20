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

class stm_message:
    def __init__(self, command, payload):
        msg = []
        msg.append(command)
        msg += payload[:PAYLOAD_LENGTH]
        msg += [0] * ( PAYLOAD_LENGTH - len(payload) )
        par = reduce(lambda x, y: x^y, msg)
        msg.append(par)
        self.msg = [x for x in msg]

    def get_message(self):
        return self.msg

    def __repr__(self):
        return "--> %02X : %s : %02X" % (
            self.msg[0],  ' '.join(["%02X" % x for x in self.msg[1:-2]]), self.msg[-1] )

class stm_response:
    def __init__(self, message):
        print repr(message)
        self.msg = message
        cmd = message[0]
        payload = message[1:PAYLOAD_LENGTH+1]
        par = message[-1]
        par_check = reduce(lambda x, y: x^y, message)
    def __repr__(self):
        return "<-- %02X : %s : %02X" % (
            self.msg[0],  ' '.join(["%02X" % x for x in self.msg[1:-2]]), self.msg[-1] )

# Class to interact with the QuadCopter-STM in a more sane way
class stm_device:
    def __init__(self):
        import spidev
        self.spi = spidev.SpiDev()
        self.spi.open(0,0)
        self.q = []

    def _handle_response(self, resp):
        if resp[0] != CMD_NOP:
            self.q.append([x for x in resp])

    def _do_request(self, cmd_code, payload = []):
        m1 = stm_message(cmd_code, payload)
        resp = self.spi.xfer2(m1.get_message())
        self._handle_response(resp)
    def __repr__(self):
        return repr(self.q)

    def nop(self):
        self._do_request(CMD_NOP)

    def set_leds(self, b):
        self._do_request(CMD_SET_STM_LEDS, [b])

    # Expects, per motor, a percentage of the max
    def set_motors(self, speeds):
        tspeeds = [int(min(100, max(0, x))*(MAX_SPEED/100)) for x in speeds]
        motors = [ord(x) for x in struct.pack('<HHHH', tspeeds[0], tspeeds[1], [1], tspeeds[2], tspeeds[3])]
        self._do_request(CMD_SET_MOTORS, motors)

    def calibrate_angle(self):
        self._do_request(CMD_CALIBRATE_ANGLE)

    def get_angle(self):
        self._do_request(CMD_GET_CUR_ANGLE)
        while CMD_GET_CUR_ANGLE not in [x[0] for x in self.q]:
            self.nop()
        angles = [x for x in self.q if x[0] == CMD_GET_CUR_ANGLE][0]
        return struct.unpack('<fff', ''.join([chr(x) for x in angles[1:13]]))



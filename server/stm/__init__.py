#! /usr/bin/python2.7
import struct
import threading
import Queue
import time

# http://louisthiery.com/spi-python-hardware-spi-for-raspi/
import spidev

PAYLOAD_LENGTH = 17
MAX_SPEED = 750

CMD_NOP           = 0x01
CMD_SET_MOTORS    = 0x02
CMD_SET_STM_LEDS  = 0x03
CMD_GET_MOTORS    = 0x04
CMD_SET_CAM_ANGLE = 0x05
CMD_SET_STM_OPTIONS   = 0x06
CMD_UNSET_STM_OPTIONS = 0x07
CMD_GET_STM_OPTIONS   = 0x08


CMD_GET_CUR_ANGLE             = 0x20
CMD_GET_CUR_GYRO              = 0x21
CMD_SET_TARGET_ANGLE          = 0x22

RASPI_CMD_GET_CUR_ACC               = 0x24
RASPI_CMD_ACC_CALIBRATE             = 0x25
RASPI_CMD_GET_CUR_ACC_CALIBRATION   = 0x26
RASPI_CMD_SET_NEW_ACC_CALIBRATION   = 0x27

RASPI_CMD_GET_CUR_MAG               = 0x28
RASPI_CMD_MAG_CALIBRATE             = 0x29
RASPI_CMD_GET_CUR_MAG_CALIBRATION   = 0x2A
RASPI_CMD_SET_NEW_MAG_CALIBRATION   = 0x2B

RASPI_CMD_SET_EXTRA_LEDS0 = 0x80
RASPI_CMD_SET_EXTRA_LEDS1 = 0x81
RASPI_CMD_SET_EXTRA_LEDS2 = 0x82


class STM_Options:
    def __init__(self, motor_initialized = False, sensor_streaming = False):
        self.motor_initialized = motor_initialized
        self.sensor_streaming = sensor_streaming
    def get_payload(self):
        return [self.motor_initialized, self.sensor_streaming]
def STM_Options_from_resp(resp):
    return STM_Options(
        motor_initialized = resp[1],
        sensor_streaming  = resp[2]
        )


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


# Class to interact with the QuadCopter-STM in a more sane way
class _stm_device(threading.Thread):
    def __init__(self, createFiles = True):
        threading.Thread.__init__(self)
        self.daemon = True
        self.spi = spidev.SpiDev()
        self.spi.open(0,0)
        self.q = []
        self.requestQueue = Queue.Queue()
        self.keep_running = True
        self.initialized = False
        self.createFiles = createFiles

        # Initialized variables
        self.stm_options = STM_Options(False, False)
        self.location     = [0, 0.0, 0.0, 0.0]
        self.speed        = [0, 0.0, 0.0, 0.0]
        self.acceleration = [0, 0.0, 0.0, 0.0]
        self.angle        = [0, 0.0, 0.0, 0.0]
        self.angularspeed = [0, 0.0, 0.0, 0.0]
        self.magfield     = [0, 0.0, 0.0, 0.0]
        self.motors       = [0.0, 0.0, 0.0, 0.0]
        self.leds         = [0,0,0,0,0,0,0,0]
        self.target_angles= [0, 0.0, 0.0, 0.0]

        if createFiles:
            self.f_acc = open('acc.csv', 'w')
            self.f_gyro = open('gyro.csv', 'w')
            self.f_mag = open('mag.csv', 'w')

    def stop(self):
        self.keep_running = False
        self.initialized = False

    def run(self):
        while self.keep_running:
            if self.initialized:
                self._main_loop()
            else:
                self._initialize()

    def _main_loop(self):
        def unpack_int(resp):
            return struct.unpack('<HHHH', ''.join([chr(x) for x in resp[1:9]]))
        def unpack_float(resp):
            return struct.unpack('<lfff', ''.join([chr(x) for x in resp[1:17]]))
        def check_parity(resp):
            par = reduce(lambda x, y: x^y, resp)
            print repr((['%02X' % x for x in resp],par))
            return par == 0x00

        while self.keep_running:
            last_motors = 0
            try:
                m = self.requestQueue.get(False)
            except Queue.Empty:
                if time.time() - last_motors > 1:
                    m = stm_message(CMD_GET_MOTORS, [])
                    last_motors = time.time()
                else:
                    m = stm_message(CMD_NOP, [])
            print repr(['%02X' % x for x in m.get_message()])
            resp = self.spi.xfer2(m.get_message())

            if not check_parity(resp):
                print "Parity failed"
                self.initialized = False
                return

            # Update local states
            if resp[0] == CMD_NOP:
                continue
            elif resp[0] == CMD_GET_MOTORS:
                self.motors = [float(x)/float(MAX_SPEED)*100.0 for x in unpack_int(resp)]
            elif resp[0] == CMD_GET_CUR_ANGLE:
                self.angle = unpack_float(resp)
            elif resp[0] == CMD_GET_CUR_GYRO:
                self.angularspeed = unpack_float(resp)
                if self.createFiles:
                    self.f_gyro.write("\t".join(['%.8f' % x for x in self.angularspeed]))
                    self.f_gyro.write("\n")
            elif resp[0] == RASPI_CMD_GET_CUR_ACC:
                self.acceleration = unpack_float(resp)
                if self.createFiles:
                    self.f_acc.write("\t".join(['%.8f' % x for x in self.acceleration]))
                    self.f_acc.write("\n")
            elif resp[0] == RASPI_CMD_GET_CUR_MAG:
                self.magfield = unpack_float(resp)
                if self.createFiles:
                    self.f_mag.write("\t".join(['%.8f' % x for x in self.magfield]))
                    self.f_mag.write("\n")
            else:
                self.q.append([x for x in resp])
        if self.createFiles:
            self.f_acc.close()
            self.f_gyro.close()
            self.f_mag.close()

    def _initialize(self):
        while self.keep_running and not self.initialized:
            time.sleep(.01)
            resp = self.spi.xfer2([0x55])
            if resp == [ 0x55 ]:
                resp = self.spi.xfer2([0xFE, 0xFE, 0xFE])
                if resp == [ 0x55, 0x55, 0x55 ]:
                    print "Initialized"
                    self.initialized = True

    def _do_request(self, cmd_code, payload = []):
        self.requestQueue.put(stm_message(cmd_code, payload))

    def _get_data_vector(self, command_type):
        self._do_request(command_type)
        while command_type not in [x[0] for x in self.q]:
            time.sleep(0.01)
        # Toch meestal het laatste request, dus probeer die eerst
        if self.q[-1][0] == command_type:
            data = self.q.pop()
        else:
            for i in range(len(self.q)):
                if self.q[i][0] == command_type:
                    data = self.q[i]
                    del self.q[i]
                    break
        return data

    def _get_float_vector(self, command_type):
        data = self._get_data_vector(command_type)
        return struct.unpack('<lfff', ''.join([chr(x) for x in data[1:17]]))

    def _get_int_vector(self, command_type):
        data = self._get_data_vector(command_type)
        return struct.unpack('<HHHH', ''.join([chr(x) for x in data[1:9]]))

    def set_leds(self, b):
        self._do_request(CMD_SET_STM_LEDS, [b])

    def set_target_angles(self, target_angles):
        if (len(target_angles) == 3):
            self.target_angles[1:4] = target_angles
            data = [ ord(x) for x in struct.pack('<lfff', self.target_angles[0], self.target_angles[1], self.target_angles[2], self.target_angles[3]) ]
            self._do_request(CMD_SET_TARGET_ANGLE, data)
        else:
            raise ValueError("3 angles need to be defined")
            

    # Expects, per motor, a percentage of the max
    def set_motors(self, speeds):
        tspeeds = [int(min(100, max(0, x))*(MAX_SPEED/100)) for x in speeds]
        motors = [ord(x) for x in struct.pack('<HHHH', tspeeds[0], tspeeds[1], tspeeds[2], tspeeds[3])]
        self._do_request(CMD_SET_MOTORS, motors)

    def enable_streaming(self):
        self._do_request(CMD_SET_STM_OPTIONS, STM_Options( sensor_streaming = True).get_payload() )
    def disable_streaming(self):
        self._do_request(CMD_UNSET_STM_OPTIONS, STM_Options( sensor_streaming = True).get_payload() )
    def enable_motors(self):
        self._do_request(CMD_SET_STM_OPTIONS, STM_Options( motor_initialized = True).get_payload() )
    def disable_motors(self):
        self._do_request(CMD_UNSET_STM_OPTIONS, STM_Options( motor_initialized = True).get_payload() )

# Wrapper class to nicely give back a running stm_device
# To be used as:
# with stm_device() as dev:
#    dev.set_leds(0x00)
class stm_device:
    def __enter__(self):
        self.device = _stm_device()
        self.device.start()
        self.device.enable_streaming()
        return self.device

    def __exit__(self, type, value, traceback):
        # Tear down by stopping all engine and stopping the thread
        self.device.set_motors([0,0,0,0])
        self.device.disable_streaming()
        time.sleep(0.1)
        self.device.stop()
        self.device.join(5)


if __name__ == '__main__':
    device = stm_device()
    device.start()
    print 'stopping'
    time.sleep(1)

#! /usr/bin/python2.7
import struct
import threading
import Queue
import time

# http://louisthiery.com/spi-python-hardware-spi-for-raspi/
import spidev

PAYLOAD_LENGTH = 16
MAX_SPEED = 750
GET_MOTOR_SPEED_INTERVAL = 1

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

CMD_INIT = 0x55
CMD_START = 0xFE
CMD_LINE_BROKEN = 0xFF
CMD_LINE_SHORTED = 0x00

class ParityError(ValueError):
    pass
class LineError(ValueError):
    pass


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
    def __init__(self, command, payload = [], parity = None):
        self.command = command
        msg = []
        msg.append(command)
        msg += payload[:PAYLOAD_LENGTH]
        msg += [0] * ( PAYLOAD_LENGTH - len(payload) )
        par = reduce(lambda x, y: x^y, msg)

        if parity is None:
            msg.append(par)
        else:
            if parity != par:
                raise ParityError("Parity failure: expected 0x%02x but got 0x%02x" % (par, parity))
            else:
                msg.append(par)
            if command in [CMD_INIT, CMD_START, CMD_LINE_BROKEN, CMD_LINE_SHORTED]:
                raise LineError("Command-code %02x is not valid" % (command))
        self.msg = [x for x in msg]

    @classmethod
    def fromData(klass, data):
        if len(data) != (PAYLOAD_LENGTH + 2):
            raise ValueError("Not enough data given")
        return klass(data[0], data[1:-1], data[-1])

    def get_message(self):
        # Return a copy of the data, so the contents don't get modified
        return [x for x in self.msg]
    def unpack_int(self):
        return struct.unpack('<HHHH', ''.join([chr(x) for x in self.msg[1:9]]))
    def unpack_float(self):
        return struct.unpack('<lfff', ''.join([chr(x) for x in self.msg[1:17]]))

    def __repr__(self):
        return "%02x: [ %s ] %02x" % (self.msg[0], ' '.join(["%02x" % x for x in self.msg[1:-1]]), self.msg[-1])
# Used so often that having it permanently here is good
NOP_MSG = stm_message(CMD_NOP)
GET_MOTOR_MSG = stm_message(CMD_GET_MOTORS)


class message_outqueue:
    def __init__(self):
        self._mq = Queue.Queue()
        self._bq = []
        self._last_motor = 0

    def insert(self, stm_msg):
        self._mq.put(stm_msg)

    def reset(self):
        self._bq = []

    # Returns the next message from the out-queue
    # If the queue is empty, a motor-message is outputted when needed, otherwise a NOP_MSG
    # So a message is always returned
    def get_next_message(self):
        try:
            return self._mq.get(False)
        except Queue.Empty:
            if time.time() - self._last_motor > GET_MOTOR_SPEED_INTERVAL:
                self._last_motor = time.time()
                return GET_MOTOR_MSG
            else:
                return NOP_MSG

    def get_bytes(self, n):
        while len(self._bq) < n:
            self._bq += self.get_next_message().get_message()
        ret_bytes = self._bq[0:n]
        self._bq = self._bq[n:]
        return ret_bytes

class message_inqueue:
    def __init__(self):
        self._bq = []

    def __iter__(self):
        return self

    def next(self):
        if len(self._bq) < (PAYLOAD_LENGTH + 2):
            raise StopIteration
        ret_msg = stm_message.fromData(self._bq[0:PAYLOAD_LENGTH+2])
        self._bq = self._bq[PAYLOAD_LENGTH+2:]
        return ret_msg

    def reset(self):
        self._bq = []

    # b: list of bytes from xfer/xfer2
    def insert(self, b):
        self._bq += b

        
SPI_UNINITIALIZED = 0x01
SPI_INITIALIZING = 0x02
SPI_INITIALIZED = 0x03
# Class to interact with the QuadCopter-STM in a more sane way
# Auto-reconnecting on line-error and parity-error
class _stm_device(threading.Thread):
    def __init__(self, createFiles = True):
        threading.Thread.__init__(self)
        self.daemon = True
        self.spi_state = SPI_UNINITIALIZED
        self.in_buffer = message_inqueue()
        self.out_buffer = message_outqueue()
        self.offset = 0
        self.keeprunning = True
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
        self.keeprunning = False
        self.initialized = False

    def run(self):
        while self.keeprunning:
            if self.spi_state == SPI_INITIALIZED:
                self._main_loop()
            else:
                self._initialize()
        if self.createFiles:
            self.f_acc.close()
            self.f_gyro.close()
            self.f_mag.close()

    def _main_loop(self):
        while self.keeprunning and (self.spi_state == SPI_INITIALIZED):
            resp = self.spi.xfer2( self.out_buffer.get_bytes(PAYLOAD_LENGTH + 2) )
            self.in_buffer.insert(resp)
            try:
                for msg in self.in_buffer:
                    # Update local states
                    if msg.command in [CMD_NOP, CMD_SET_STM_LEDS]:
                        continue
                    elif msg.command == CMD_GET_MOTORS:
                        self.motors = [float(x)/float(MAX_SPEED)*100.0 for x in msg.unpack_int()]
                    elif msg.command == CMD_GET_CUR_ANGLE:
                        self.angle = msg.unpack_float()
                    elif msg.command == CMD_GET_CUR_GYRO:
                        self.angularspeed = msg.unpack_float()
                        if self.createFiles:
                            self.f_gyro.write("\t".join(['%.8f' % x for x in self.angularspeed]))
                            self.f_gyro.write("\n")
                    elif msg.command == RASPI_CMD_GET_CUR_ACC:
                        self.acceleration = msg.unpack_float()
                        if self.createFiles:
                            self.f_acc.write("\t".join(['%.8f' % x for x in self.acceleration]))
                            self.f_acc.write("\n")
                    elif msg.command == RASPI_CMD_GET_CUR_MAG:
                        self.magfield = msg.unpack_float()
                        if self.createFiles:
                            self.f_mag.write("\t".join(['%.8f' % x for x in self.magfield]))
                            self.f_mag.write("\n")
            except (LineError, ParityError) as e:
                self.in_buffer.reset()
                self.out_buffer.reset()
                self.spi_state = SPI_UNINITIALIZED

    def _initialize(self):
        self.spi_state = SPI_UNINITIALIZED
        try:
            self.spi.close()
            del self.spi
        except AttributeError:
            pass
        self.spi = spidev.SpiDev()
        self.spi.open(0,0)

        while self.keeprunning and ( self.spi_state != SPI_INITIALIZED ):
            if self.spi_state == SPI_UNINITIALIZED:
                if self.spi.xfer2( [CMD_INIT] ) == [CMD_INIT]:
                    self.spi_state = SPI_INITIALIZING
                    self.offset = 0
                    resp = self.spi.xfer2( [CMD_START ] )
                    if resp != [CMD_INIT]:
                        self.spi_state = SPI_UNINITIALIZED
            elif self.spi_state == SPI_INITIALIZING:
                resp = self.spi.xfer2( self.out_buffer.get_bytes(1) )
                if resp == [CMD_INIT]:
                    self.offset += 1
                    continue
                elif resp[0] in [0x00, 0xFF, CMD_START]:
                    self.spi_state = SPI_UNINITIALIZED
                    self.out_buffer.reset()
                else:
                    self.in_buffer.insert(resp)
                    self.spi_state = SPI_INITIALIZED


    def _do_request(self, cmd_code, payload = []):
        self.out_buffer.insert(stm_message(cmd_code, payload))

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

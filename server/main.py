#!/usr/bin/env python
# http://louisthiery.com/spi-python-hardware-spi-for-raspi/

import time
import struct
import commands
import client
from stm import *

#debug
debug_option = 1

#assign variables
motor = [0,0,0,0]

#connections
stm = stm_device()
client.connect()

#main
while 1:
    data_string = client.request()

    data_list = eval(data_string)
    debug("data_list", data_list)

    motor[0] = data_list[4]
    motor[1] = data_list[4]
    motor[2] = data_list[4]
    motor[3] = data_list[4]

    debug("motor", motor)

    stm.set_motors([motor[0],motor[1],motor[2],motor[3]])
    stm.set_leds(motor[0])

    print "angle = " + repr(stm.get_angle())

conn.close()

#!/usr/bin/env python
# http://louisthiery.com/spi-python-hardware-spi-for-raspi/

import time
import struct
import commands
import socket
from stm import *

#debug
debug_option = 1

#tcp
TCP_IP = ''
TCP_PORT = 5555
BUFFER_SIZE = 1024

#assign variables
motor = [0,0,0,0]

#connections
stm = stm_device()

#connect with client (tcp)
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind((TCP_IP, TCP_PORT))
s.listen(1)
conn, addr = s.accept()

#debug
def debug( debugvar, debugstring ):
   if debug_option == 1:
       print str(debugvar) + " = " + str(debugstring)

#main
while 1:
    data_string = conn.recv(BUFFER_SIZE)

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

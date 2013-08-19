#!/usr/bin/env python
# http://louisthiery.com/spi-python-hardware-spi-for-raspi/

import time
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
motor_mode = 0 # 0 = stable # 1 = speed # 2 = debug

#ratio
forward_ratio = 10
sideward_ratio = 10
upward_ratio = 10
downward_ratio = 10
forward_hang_ratio = 10


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

#commands
def motorrefresh():

    if motor_mode == 0: #stable

        #motor[0] = - (forward_ratio*ly) + (sideward_ratio*lx) + (upward_ratio*rt) - (downward_ratio*lt) /100
        #motor[1] = - (forward_ratio*ly) - (sideward_ratio*lx) + (upward_ratio*rt) - (downward_ratio*lt) /100
        #motor[2] = + (forward_ratio*ly) + (sideward_ratio*lx) + (upward_ratio*rt) - (downward_ratio*lt) /100
        #motor[3] = + (forward_ratio*ly) - (sideward_ratio*lx) + (upward_ratio*rt) - (downward_ratio*lt) /100

        motor[0] = (-(forward_hang_ratio*data_list[1])+(sideward_ratio*data_list[0])+(upward_ratio*data_list[5])-(downward_ratio*data_list[4]))/100
        motor[1] = (-(forward_hang_ratio*data_list[1])-(sideward_ratio*data_list[0])+(upward_ratio*data_list[5])-(downward_ratio*data_list[4]))/100
        motor[2] = (+(forward_hang_ratio*data_list[1])+(sideward_ratio*data_list[0])+(upward_ratio*data_list[5])-(downward_ratio*data_list[4]))/100
        motor[3] = (+(forward_hang_ratio*data_list[1])-(sideward_ratio*data_list[0])+(upward_ratio*data_list[5])-(downward_ratio*data_list[4]))/100

    if motor_mode == 1: #fast

        #motor[0] = - (forward_ratio*ly) + (sideward_ratio*lx) - (forward_hang_ratio*rt) / 100
        #motor[1] = - (forward_ratio*ly) - (sideward_ratio*lx) - (forward_hang_ratio*rt) / 100
        #motor[2] = + (forward_ratio*ly) + (sideward_ratio*lx) + (forward_hang_ratio*rt) / 100
        #motor[3] = + (forward_ratio*ly) - (sideward_ratio*lx) + (forward_hang_ratio*rt) / 100

        motor[0] = -(forward_hang_ratio*data_list[1])+(sideward_ratio*data_list[0])-(forward_ratio*data_list[5])/100
        motor[1] = -(forward_hang_ratio*data_list[1])-(sideward_ratio*data_list[0])-(forward_ratio*data_list[5])/100
        motor[2] = +(forward_hang_ratio*data_list[1])+(sideward_ratio*data_list[0])+(forward_ratio*data_list[5])/100
        motor[3] = +(forward_hang_ratio*data_list[1])-(sideward_ratio*data_list[0])+(forward_ratio*data_list[5])/100

    if motor_mode == 3: #debug

        #motor[0] = rt
        #motor[1] = rt
        #motor[2] = rt
        #motor[3] = rt

        motor[0] = data_list[4]
        motor[1] = data_list[4]
        motor[2] = data_list[4]
        motor[3] = data_list[4]

    for x in xrange(4):
        if motor[x] < 0:
            motor[x] = 0
        if motor[x] > 100:
            motor[x] = 100

    debug("motor", motor)
    stm.set_motors([motor[0],motor[1],motor[2],motor[3]])

#main
while 1:
    data_list = [int(i) for i in conn.recv(BUFFER_SIZE).split(',')]
    debug("data_list", data_list)
    motorrefresh()
    stm.set_leds(motor[0])
    conn.send("angle = " + repr(stm.get_angle()))

conn.close()

#!/usr/bin/env python

import xcontroller, socket, time, string

TCP_IP = 'localhost'
TCP_PORT = 5555
BUFFER_SIZE = 1024
pad1 = xcontroller.get_controller(0)
state_old = ""

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((TCP_IP, TCP_PORT))
while 1:
    time.sleep(0.1)
    state_new = str(pad1.l_x()) + "," + str(pad1.l_y()) + "," + str(pad1.r_x()) + "," + str(pad1.r_y()) + "," + str(pad1.l_t()) + "," + str(pad1.r_t()) + "," + str(pad1.h_u()) + "," + str(pad1.h_d()) + "," + str(pad1.h_l()) + "," + str(pad1.h_r()) + "," + str(pad1.b_a()) + "," + str(pad1.b_b()) + "," + str(pad1.b_x()) + "," + str(pad1.b_y())
    
    if state_old != state_new :
        print state_new
        state_old = state_new

        s.send(state_new)
        data = s.recv(BUFFER_SIZE)

print data
s.close()

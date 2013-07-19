#!/usr/bin/env python
# http://louisthiery.com/spi-python-hardware-spi-for-raspi/

import time
import spidev
import socket
from struct import *
from stm import *

#tcp
TCP_IP = ''
TCP_PORT = 5555
BUFFER_SIZE = 1024

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind((TCP_IP, TCP_PORT))
s.listen(1)

conn, addr = s.accept()
print 'Connection address:', addr

#spi
spi = spidev.SpiDev()
spi.open(0,0)

#motor
c0x02_list = [0,0,0,0,0,0,0,0]

#main
while 1:
    data_string = conn.recv(BUFFER_SIZE)
    #if not data_string: break
    print "data_string = " + data_string

    data_list = eval(data_string)
    print "data_list = " + str(data_list).translate(None, "'")
    print "data_list[4] = " + str(data_list[4])
    c0x02_list[0] = 65535 * data_list[4] / 100
    c0x02_list[1] = 65535 * data_list[4] / 100
    c0x02_list[2] = 65535 * data_list[4] / 100
    c0x02_list[3] = 65535 * data_list[4] / 100
    print "65535 * data_list[4] = " + str(65535 * data_list[4])
    print "c0x02_list[0] (65535 * data_list[4] / 100) = " + str(c0x02_list[0])

    m1 = stm_message(0x02, [c0x02_list[0] >> 8, c0x02_list[0] % 256, c0x02_list[1] >> 8, c0x02_list[1] % 256, c0x02_list[2] >> 8, c0x02_list[2] % 256, c0x02_list[3] >> 8, c0x02_list[3] % 256])

    resp = spi.xfer2(m1.get_message())
    print "resp = " + str(resp)
    conn.send("accu = ... V")
conn.close()

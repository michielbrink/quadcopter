#!/usr/bin/env python
#for testing without a controller

import socket, time, string

TCP_IP = 'localhost'
TCP_PORT = 5555
BUFFER_SIZE = 1024

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((TCP_IP, TCP_PORT))

time.sleep(0.1)

while 1:
    s.send("0,0,0,0,0,0,0,0,0,0,0,0,0,0")
    data = s.recv(BUFFER_SIZE)
    time.sleep(10)

s.close()

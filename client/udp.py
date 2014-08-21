#!/usr/bin/env python

import socket
import sys
import thread
import time

class udp:
    __init__(self ,UDP_IP ,UDP_PORT):
        self.UDP_IP = UDP_IP
        self.UDP_PORT = UDP_PORT
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        thread.start_new_thread(self.send_message, ())
        thread.start_new_thread(get_message, ())

    def get_message():
        data = None
        try:
            data, addr = self.sock.recvfrom( 1024, socket.MSG_DONTWAIT ) # buffer size is 1024 bytes
        except socket.error:
            # wait a bit
            time.sleep(0.01)
        if data:
            print data

    def send_message():
        try:
            while True:
                self.sock.sendto( "hello world", (server_ip, int(server_port)) )

        except KeyboardInterrupt:
            print("byebye now")

# place here code to use if the connection is made

try:
    while 1:
        time.sleep(0.01)
except KeyboardInterrupt:
    print("bye")
    # send motor off command or something
    sys.exit(0)

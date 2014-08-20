#!/usr/bin/env python

import socket
import errno
import time
import threading
import json

class tcp:
    def __init__(self ,TCP_IP ,TCP_PORT ,BUFFER_SIZE):
        self.BUFFER_SIZE = BUFFER_SIZE
        self.TCP_IP = TCP_IP
        self.TCP_PORT = TCP_PORT
        self.tcpconnect()
                
    def tcpconnect(self):
        repeat = 1
        while repeat == 1:
            repeat = 0
            try:
                self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self.s.connect((self.TCP_IP, self.TCP_PORT))
            except IOError as e:
                if e.errno == errno.ECONNREFUSED:
                    repeat = 1

    def pingwait(self):
        sleep(1)

    def sendtcp(self,MESSAGE):
        try:
            self.s.send(MESSAGE)
            return self.s.recv(self.BUFFER_SIZE)
        except IOError as e:
            if e.errno == errno.EPIPE:
                self.s.close()
                self.tcpconnect()            

#    def send(self):
#        json.dumps([1,2,3,{'4': 5, '6': 7}], separators=(',',':'))   

    def close(self):
        self.s.close()

#test script
if __name__ == "__main__":
    import time

    connection = tcp('127.0.0.1',5001,1024)

    while True:
        time.sleep(0.5)
        connection.sendtcp("lalalalalalalallatest")

# usefull links
# https://docs.python.org/2/library/threading.html#timer-objects
# timer: https://docs.python.org/2/library/timeit.html

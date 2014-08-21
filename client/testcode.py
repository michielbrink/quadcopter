#!/usr/bin/env python

import socket
import sys
import thread
import time

server_ip = sys.argv[1]
server_port = sys.argv[2]

sock = socket.socket( socket.AF_INET, # Internet
                      socket.SOCK_DGRAM ) # UDP


def get_message():
  global sock
  while True:
    data = None
    try:
      data, addr = sock.recvfrom( 1024, socket.MSG_DONTWAIT ) # buffer size is 1024 bytes
    except socket.error:
      # wait a bit
      time.sleep(0.01)
    if data:
      print data

def send_message():
  global sock
  try:
    while True:
      sock.sendto( "hello world", (server_ip, int(server_port)) )

  except KeyboardInterrupt:
    print("byebye now")

thread.start_new_thread(send_message, ())
thread.start_new_thread(get_message, ())

# place here code to use if the connection is made

try:
  while 1:
    time.sleep(0.01)
except KeyboardInterrupt:
  print("bye")
  # send motor off command or something
  sys.exit(0)

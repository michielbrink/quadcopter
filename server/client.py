import socket

TCP_IP = ''
TCP_PORT = 5555
BUFFER_SIZE = 1024

def connect():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind((TCP_IP, TCP_PORT))
    s.listen(1)
    conn, addr = s.accept()
    return conn


TCP_IP = ''
TCP_PORT = 5555
BUFFER_SIZE = 1024

class client_device:
    def __init__(self):
        import socket
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.bind((TCP_IP, TCP_PORT))
        s.listen(1)
        conn, addr = s.accept()

    def _request(self):
        return conn.recv(BUFFER_SIZE)

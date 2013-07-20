class client_device:
    def __init__(self):
        import socket
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.bind((TCP_IP, TCP_PORT))
        s.listen(1)
        conn, addr = s.accept()

    def _request(self):
        conn.recv(BUFFER_SIZE)

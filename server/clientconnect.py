
TCP_IP = ''
TCP_PORT = 5555
BUFFER_SIZE = 1024

def tcp_connect():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind((TCP_IP, TCP_PORT))
    s.listen(1)
    conn, addr = s.accept()

def tcp_request():
    return conn.recv(BUFFER_SIZE)

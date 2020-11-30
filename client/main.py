import socket
import threading
import sys

# --- functions ---

def recv_msg():
    while True:
        recv_msg = s.recv(1024)
        if not recv_msg:
            sys.exit(0)
        recv_msg = recv_msg.decode()
        print(recv_msg)

def send_msg():
    while True:
        send_msg = input()
        send_msg = send_msg.encode()
        s.send(send_msg)
        # print("message sent")

# --- main ---

host = '192.168.2.113'
port = 3001

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

s.connect((host, port))

# thread has to start before other loop
t = threading.Thread(target=recv_msg)
t.start()

send_msg()


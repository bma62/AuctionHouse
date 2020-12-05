# import necessary packages
import socket
import threading
import sys

# --- functions ---
def recv_msg():
    # wait to receive message from the server and print it out
    while True:
        recv_msg = s.recv(1024)
        if not recv_msg:
            sys.exit(0)
        recv_msg = recv_msg.decode()
        print(recv_msg)

def send_msg():
    # collect any user input and send to server
    while True:
        send_msg = input()
        #the user may shutdown the client if they enter 'quit'
        if send_msg == 'QUIT' :
            sys.exit(0)
        send_msg = send_msg.encode()
        s.send(send_msg)

# --- main ---
host = '192.168.2.113' # IP of the server
port = 3001 # which port the server is operating on
#welcome message and instructions
print('Welcome to The SE3313 Auction House CLient Application\n')
print('You May Exit This Application at Any Time By Typing "QUIT" ')
# create a socket
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# connect to server
s.connect((host, port))

# start another thread on receiving message from the same socket
t = threading.Thread(target=recv_msg)
t.start()

# main thread will enter the send_msg function
send_msg()


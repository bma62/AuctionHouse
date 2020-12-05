# import necessary packages
import errno
import socket
import threading
import sys

# --- functions ---
def recv_msg():
    # wait to receive message from the server and print it out
    while True:
        recv_msg = s.recv(1024)
        # if length of recv is 0, the other side has closed the socket
        if len(recv_msg) == 0:
            print("RECV THREAD: Connection has been gracefully terminated at the server side.")
            s.close()  # close the socket on client side as well
            sys.exit(0)
        if not recv_msg:
            sys.exit(0)
        recv_msg = recv_msg.decode()
        print(recv_msg)

def send_msg():
    # collect any user input and send to server
    while True:
        send_msg = input()
        # the user may shutdown the client if they enter 'quit'
        if send_msg == 'QUIT':
            sys.exit(0)
        send_msg = send_msg.encode()
        # try to send thru socket
        try:
            s.send(send_msg)
        except socket.error as e:
            # if socket already closed at the server side, this EPIPE exception will be raised
            print("SEND THREAD: Connection has been gracefully terminated at the server side.")
            break

# --- main ---
host = '192.168.2.113'  # IP of the server
port = 3001  # which port the server is operating on

# welcome message and instructions
print('Welcome to The SE3313 Auction House Client Application\n')
print('You May Exit This Application at Any Time By Typing "QUIT" ')

# create a socket
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# connect to server
s.connect((host, port))
print("**Connection established**")

# start another thread on receiving message from the same socket
t = threading.Thread(target=recv_msg)
t.start()

# main thread will enter the send_msg function
send_msg()


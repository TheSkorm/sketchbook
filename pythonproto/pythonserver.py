import socket
from ctypes import *

TCP_IP = '59.167.158.119'
TCP_PORT = 23
BUFFER_SIZE=8         #start,action,address,value,end
MESSAGE = create_string_buffer("\xff\x00\xff\xcd\xA5")
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.settimeout(1)
s.connect((TCP_IP, TCP_PORT))
s.send(MESSAGE)
data = s.recv(BUFFER_SIZE)
print data
s.close()

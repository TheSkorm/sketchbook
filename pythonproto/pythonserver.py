import socket
from ctypes import *

TCP_IP = '59.167.158.119'
TCP_PORT = 58008
BUFFER_SIZE=8         #start,action,address,value,value2,end
MESSAGE = create_string_buffer("\xff\x05\x02\x02\x02\xA5")
MESSAGE2 = create_string_buffer("\xff\x03\x02\x00\x01\xA5")
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.settimeout(1)
s.connect((TCP_IP, TCP_PORT))
s.send(MESSAGE)
s.send(MESSAGE2)
data = s.recv(BUFFER_SIZE)
print data
s.close()

import socket
import struct



TCP_IP = '59.167.158.119'
TCP_PORT = 58008
BUFFER_SIZE=7         #start,action,address,value,value2,end

#MESSAGE = "\xff\x05\x02\x02\x02\xA5\x00"
#MESSAGE2 = "\xff\x03\x02\x00\x01\xA5\x00"

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# s.settimeout(2)
s.connect((TCP_IP, TCP_PORT))

#s.send(MESSAGE)
#s.send(MESSAGE2)

def send_keepalive():
    message = "\xff\x00\x00\x00\x00\xA5\x00"
    s.send(message)

def send_digitalread(port):
    message = "\xff\x01"+struct.pack("!B",port)+"\x00\x00\xA5\x00"
    s.send(message)

def send_analogread(port):
    message = "\xff\x02"+struct.pack("!B",port)+"\x00\x00\xA5\x00"
    s.send(message)

def send_digitalwrite(port,state):
    message = "\xff\x03"+struct.pack("!B",port)+"\x00"+struct.pack("!B",state)+"\xA5\x00"
    s.send(message)

def send_setoutput(port,state):
    message = "\xff\x05"+struct.pack("!B",port)+"\x00"+struct.pack("!B",state)+"\xA5\x00"
    s.send(message)


def recv_data():
    data = s.recv(BUFFER_SIZE)
    output = {}
    if (len(data)==7):
        output["start"] = struct.unpack("!B",data[0])[0]
        output["action"] = struct.unpack("!B",data[1])[0]
        output["address"] = struct.unpack("!B",data[2])[0]
        output["value"] = struct.unpack("!B",data[3])[0]
        output["value2"] = struct.unpack("!B",data[4])[0]
        output["checksum"] = struct.unpack("!B",data[5])[0]
    return output

def action_data(packet):
    #TODO ADD CHECKS
    actions = {
0: recv_loopback,
1: recv_read_digital,
2: recv_read_analog,
3: recv_write_digital,
4: recv_read_dht,
5: recv_set_output
}
    actions[packet["action"]](packet)
    


def recv_loopback(packet):
    print "Got loopback"

def recv_read_digital(packet):
    print "D" + str(packet["address"]) + " - " + str(packet["value2"])
    return


def recv_read_analog(packet):
    return

def recv_write_digital(packet):
    return

def recv_read_dht(packet):
    return

def recv_set_output(packet):
    return

send_keepalive()
send_setoutput(3,2)
send_digitalread(2)
send_setoutput(3,0)
send_digitalwrite(3,1)

while (True):
    action_data(recv_data())

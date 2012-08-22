import socket
import struct
import BaseHTTPServer
import SocketServer
import threading
import time

TCP_IP = '59.167.158.119'
TCP_PORT = 58008
BUFFER_SIZE=7   



class http_handler(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/":
            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.end_headers()
            self.wfile.write("<html><head></head><body>test</body></html>")
        elif self.path == "/test":
            global a
            a.send_keepalive()
            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.end_headers()
            self.wfile.write("<html><head></head><body>Sent Keep Alive</body></html>")
        else:
            self.send_error(404, "File not found")








class ArduinoControl(threading.Thread):
    def run(self):
        self.ready = False
        self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.s.connect((TCP_IP, TCP_PORT))
        self.send_keepalive()
        self.ready = True
        while 1:
            self.action_data(self.recv_data())
    def send_keepalive(self):
        message = "\xff\x00\x00\x00\x00\xA5\x00"
        self.s.send(message)

    def send_digitalread(self,port):
        message = "\xff\x01"+struct.pack("!B",port)+"\x00\x00\xA5\x00"
        self.s.send(message)

    def send_analogread(self,port):
        message = "\xff\x02"+struct.pack("!B",port)+"\x00\x00\xA5\x00"
        self.s.send(message)

    def send_digitalwrite(self,port,state):
        message = "\xff\x03"+struct.pack("!B",port)+"\x00"+struct.pack("!B",state)+"\xA5\x00"
        self.s.send(message)

    def send_setoutput(self,port,state):
        message = "\xff\x05"+struct.pack("!B",port)+"\x00"+struct.pack("!B",state)+"\xA5\x00"
        self.s.send(message)


    def recv_data(self):
        data = self.s.recv(BUFFER_SIZE)
        output = {}
        if (len(data)==7):
            output["start"] = struct.unpack("!B",data[0])[0]
            output["action"] = struct.unpack("!B",data[1])[0]
            output["address"] = struct.unpack("!B",data[2])[0]
            output["value"] = struct.unpack("!B",data[3])[0]
            output["value2"] = struct.unpack("!B",data[4])[0]
            output["checksum"] = struct.unpack("!B",data[5])[0]
        return output

    def action_data(self,packet):
        #TODO ADD CHECKS
        actions = {
    0: self.recv_loopback,
    1: self.recv_read_digital,
    2: self.recv_read_analog,
    3: self.recv_write_digital,
    4: self.recv_read_dht,
    5: self.recv_set_output
    }
        actions[packet["action"]](packet)
        


    def recv_loopback(self,packet):
        print "Got loopback"

    def recv_read_digital(self,packet):
        print "D" + str(packet["address"]) + " - " + str(packet["value2"])
        return


    def recv_read_analog(self,packet):
        print "A" + str(packet["address"]) + " - " + str((0x100 * packet["value"]) + packet["value2"])
        return

    def recv_write_digital(self,packet):
        return

    def recv_read_dht(self,packet):
        return

    def recv_set_output(self,packet):
        return

##send_keepalive()
##send_setoutput(3,2)
##send_digitalread(2)
##send_setoutput(3,0)
##send_digitalwrite(3,1)

##while (True):
##    action_data(recv_data())

a = ArduinoControl()
a.start()

#wait for connection
while (a.ready == False):
    time.sleep(0.1)

class httpserverthread(threading.Thread):
    def run(self):
        server_address = ('', 8094)
        httpd = BaseHTTPServer.HTTPServer(server_address, http_handler)
        httpd.serve_forever()

b = httpserverthread()
b.start()

while (True):
    time.sleep(10)
    for x in range(0,54):
        a.send_digitalread(x)
    for x in range(0,16):
        a.send_analogread(x)

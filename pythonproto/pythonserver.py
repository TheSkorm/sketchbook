import socket
import struct
import BaseHTTPServer
import SocketServer
import threading
import time
import traceback
import md5
import string
import random

TCP_IP = '59.167.158.119'
TCP_PORT = 58008
BUFFER_SIZE=7   



class http_handler(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_GET(self):
        self.psk = "test"
        if self.path == "/":
            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.end_headers()
            f = open('index.htm', 'r')
            self.wfile.write(f.read())
            f.close()
        elif  "/status" in self.path:
            global controller
            self.send_response(200)
            self.send_header("Content-type", "application/javascript")
            self.end_headers()
            self.wfile.write("status({")
            for x in controller.digitals:
              self.wfile.write("\"D" + str(x) + "\": " + str(controller.digital(x)) + ",");  
            for x in controller.analogs:
              self.wfile.write("\"A" + str(x) + "\": " + str(controller.analog(x)) + ",");               
            self.wfile.write("});")
            self.wfile.write("token({\"token\": \""+token+"\"});")
        elif  "/t" in self.path:
            print self.path
            self.send_response(200)
            self.send_header("Content-type", "application/javascript")
            self.end_headers()
            self.wfile.write("status({")
            c = self.path.split("/")
            if md5.new(token + self.psk + "".join(c[3:])).hexdigest() == c[2] or md5.new(lasttoken + self.psk + "".join(c[3:])).hexdigest():
                print "Auth passed"
                if c[3] == "output":
                    if c[4] == "45":
                        controller.doorunlock()
                    else:
                        if controller.digital(int(c[4])) == 1:
                            controller.digital(int(c[4]),0)
                        else:
                            controller.digital(int(c[4]),1)
            else:
                print "auth failed"
                self.send_response(403)
                self.send_header("Content-type", "text/html")
                self.end_headers()
                self.wfile.write("You suck")


        elif self.path == "/config.js":
            self.send_response(200)
            self.send_header("Content-type", "application/javascript")
            self.end_headers()
            self.wfile.write("""
   config({
   \"D22\" : [{\"description\": \"Front Outside Light\", \"type\": \"switch\",\"read\": \"A0\"}],
   \"D23\" : [{\"description\": \"Kitchen\", \"type\": \"switch\",\"read\": \"A1\"}],
   \"D24\" : [{\"description\": \"Dinning Room\", \"type\": \"switch\",\"read\": \"A2\"}],
   \"D25\" : [{\"description\": \"Lounge Room 1\", \"type\": \"switch\",\"read\": \"A3\"}],
   \"D26\" : [{\"description\": \"Lounge Room 2\", \"type\": \"switch\",\"read\": \"A4\"}],
   \"D27\" : [{\"description\": \"Lounge Room Fan\", \"type\": \"switch\",\"read\": \"A5\"}],
   \"D28\" : [{\"description\": \"Bedroom 1\", \"type\": \"switch\",\"read\": \"A6\"}],
   \"D29\" : [{\"description\": \"Bedroom 2\", \"type\": \"switch\",\"read\": \"A7\"}],
   \"D30\" : [{\"description\": \"Bedroom 3\", \"type\": \"switch\",\"read\": \"A8\"}],
   \"D31\" : [{\"description\": \"Outside Light 1\", \"type\": \"switch\",\"read\": \"A9\"}],
   \"D32\" : [{\"description\": \"Outside Light 2\", \"type\": \"switch\",\"read\": \"A10\"}],
   \"D45\" : [{\"description\": \"Front Door\", \"type\": \"door\"}],
   \"T0\" : [{\"description\": \"Outside Temp\", \"type\": \"temp\"}],
   \"T1\" : [{\"description\": \"Inside Temp\", \"type\": \"temp\"}],
   \"H0\" : [{\"description\": \"Outside Humid\", \"type\": \"humid\"}],
   \"H1\" : [{\"description\": \"Inside Humid\", \"type\": \"humid\"}],
   \"A1\" : [{\"description\": \"Light Sensor\", \"type\": \"light\"}],
     });
                """)
            self.wfile.write("token({\"token\": \""+token+"\"});")
        elif self.path == "/favicon.ico":
            self.send_response(200)
            self.send_header("Content-type", "image/ico")
            self.end_headers()
            f = open('favicon.ico', 'r')
            self.wfile.write(f.read())
            f.close()
        else:
            self.send_error(404, "File not found")








class ArduinoControl(threading.Thread):
    def run(self):
        self.ready = False
        self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.s.connect((TCP_IP, TCP_PORT))
        self.s.setblocking(0)
        self.send_keepalive()
        self.ready = True
        self.digitals = {}
        self.analogs = {}
        self.next_trueup = time.time()+0.8
        self.doorbuttonpin=43
        self.doorlockpin=45
        self.next_doorcheck = time.time() + 0.1
        self.autolocktime = time.time()
        self.frontlighttime = time.time()
        self.frontlightinput = 0
        self.frontlightout = 22
        self.doorlock_button_laststate = -1
        self.running = True
        while self.running == True:
            try:
                self.action_data(self.recv_data())
            except:
                if self.digital(self.doorlockpin):
                    if time.time() > self.autolocktime:
                        self.doorlock()
                if self.frontlighttime:
                    if time.time() > self.frontlighttime:
                        if self.analog(self.frontlightinput) > 5:
                            if self.digital(self.frontlightout):
                                self.digital(self.frontlightout, 0)
                            else:
                                self.digital(self.frontlightout, 1)
                        self.frontlighttime = 0
                if time.time() > self.next_doorcheck:
                    self.send_digitalread(self.doorbuttonpin)
                    self.next_doorcheck = time.time() + 0.1
                if time.time() > self.next_trueup:
                    for x in range(0,54):
                        self.send_digitalread(x)
                    for x in range(0,16):
                        self.send_analogread(x)
                    self.next_trueup = time.time()+0.8
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
        self.digitals[packet["address"]] = packet["value2"]
        if (packet["address"] == self.doorbuttonpin) & (packet["value2"] != self.doorlock_button_laststate):
            if self.doorlock_button_laststate == -1:
                self.doorlock_button_laststate = packet["value2"] # Don't unlock door on first packet
            else :    
                print "unlocking door"
                self.doorlock_button_laststate = packet["value2"]
                self.doorunlock()
        return


    def recv_read_analog(self,packet):
        self.analogs[packet["address"]] = (0x100 * packet["value"]) + packet["value2"]
        #print "A" + str(packet["address"]) + " - " + str((0x100 * packet["value"]) + packet["value2"])
        return

    def recv_write_digital(self,packet):
        self.digitals[packet["address"]] = packet["value2"]
        return

    def recv_read_dht(self,packet):
        return

    def recv_set_output(self,packet):
        return

    def set_output(self, port,type):
        self.send_setoutput(port, type)
    def analog(self, port):
        if port in self.analogs:
            return self.analogs[port]
        else:
            return -1
    def digital(self, port,value=-1):
        if value == -1:
            if port in self.digitals:
                return self.digitals[port]
            else:
                return -1
        else:
            print "Sending packet to set port " + str(port) + " to " + str(value)
            self.send_digitalwrite(port, value)
            self.digitals[port] = value
            return value
    def doorunlock(self):
        self.digital(self.doorlockpin, 1)
        self.autolocktime = time.time() + 10
        if time.gmtime()[3] > 17  or time.gmtime()[3] > 3:
            if self.analog(self.frontlightinput) < 5:
                if self.digital(self.frontlightout):
                    self.digital(self.frontlightout, 0)
                else:
                    self.digital(self.frontlightout, 1)
                self.frontlighttime = time.time() + 120 # 2 minutes

    def doorlock(self):
        self.digital(self.doorlockpin, 0)

class httpserverthread(threading.Thread):
    def run(self):
        server_address = ('', 8094)
        self.httpd = BaseHTTPServer.HTTPServer(server_address, http_handler)
        self.httpd.serve_forever()




def startup():
    global controller
    global webserver
    global token
    global lasttoken
    lasttoken = ""
    token =  "".join(random.sample(string.ascii_letters,24))
    controller = ArduinoControl()
    controller.start()
    webserver = httpserverthread()
    webserver.start()
    time.sleep(0.2) #TODO most likely a better way of waiting for threads to start up
    while (controller.ready == False): # wait until we have a connection to ardiuno
        time.sleep(0.1)
    setup_outputs(controller)

def setup_outputs(ard):
    for x in range(22,54):
        ard.set_output(x,0)

startup()

try:
    while (True):
        time.sleep(5)
        lasttoken = token
        token =  "".join(random.sample(string.ascii_letters,24))

except KeyboardInterrupt:
    controller.running = False
    webserver.httpd.shutdown()
except:
    print "something went wrong"
    controller.running = False
    webserver.httpd.shutdown()
    traceback.print_exc()

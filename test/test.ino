#define DEBUG_MESSAGES true
#include "header.h"
#include <SdFat.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>
#include <MemoryFree.h>
#include <utility/w5100.h> 
#include <MD5.h>
#include "WString.h"

/************ENCRYPTION STUFF ************/
String PSK = "insertPSKhere"; // TODO multi PSKs read off SD card or something
String tokens[3];
int currenttoken = 0;
unsigned long lastkeychange = 0 ;

/************ AC stuff ************/
unsigned long lastaccheck = 0 ;
int maxac[15];
int laststate[15];


/************ TEMP/HUMID STUFF ************/
//#define DHT11_PIN 6      // ADC0
// How big our line buffer should be. 100 is plenty!
unsigned long nexttemp;
#include "DHT.h"
#define DHTTYPE DHT11
DHT dhtin(47, DHTTYPE);
DHT dhtout(46, DHTTYPE);
float hin = 0;
  float tin = 0;
  float hout = 0;
  float tout = 0;
  

/************ ETHERNET STUFF ************/
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
EthernetServer server(80);

/************ SDCARD STUFF ************/
Sd2Card card;
SdVolume volume;
SdFile root;
SdFile file;
SdFat sd;
SdFile myFile;
const int chipSelect = 4;

/************ WEBSERVER STUFF ************/
#define BUFSIZ 1000


void setup() {
        lastkeychange = millis() ;
	Serial.begin(57600);
	debug("SERIAL", -1, "STARTED");
	randomSeed(analogRead(0)); //randommize the ardiuno generotor
	setup_temp(8);
	setup_temp(14);
	setup_pins();
	setup_sdcard();
	setup_network();
	refreshtoken();
	refreshtoken();
	refreshtoken();

 if (!sd.init(SPI_HALF_SPEED, chipSelect)) sd.initErrorHalt();  // Code to make the json config file
sd.remove("config.js");
 if (!myFile.open("config.js", O_RDWR | O_CREAT | O_AT_END)) {
    sd.errorHalt("opening config.js for write failed");
 }
 myFile.println("config({");
 myFile.println("\"D33\" : [{\"description\": \"Lounge Room Light\", \"type\": \"switch\",\"read\": \"A10\"}],");
 myFile.println("\"D41\" : [{\"description\": \"Something Else\", \"type\": \"switch\",\"read\": \"D41\"}],");
 myFile.println("\"T0\" : [{\"description\": \"Outside Temp\", \"type\": \"temp\"}],");
 myFile.println("\"T1\" : [{\"description\": \"Inside Temp\", \"type\": \"temp\"}],");
 myFile.println("\"H0\" : [{\"description\": \"Outside Humid\", \"type\": \"humid\"}],");
 myFile.println("\"H1\" : [{\"description\": \"Inside Humid\", \"type\": \"humid\"}],");
 myFile.println("\"A1\" : [{\"description\": \"Light Sensor\", \"type\": \"light\"}],");
 myFile.println("  });");

    myFile.close();

}

String templine;
void loop() {
	char clientline[BUFSIZ];
	int index = 0;
  unsigned long currentMillis = millis();
        if ( currentMillis - lastkeychange > 10000 ){
          lastkeychange = currentMillis;
        refreshtoken();

hin = dhtin.readHumidity();
tin = dhtin.readTemperature();
hout = dhtout.readHumidity();
tout = dhtout.readTemperature();
        }
        
        
        if ( currentMillis - lastaccheck > 500 ){
          lastaccheck = currentMillis;
          for (int i =0; i<15; i++){
laststate[i] = maxac[i];
maxac[i] = 0;   
       }

}
          for (int i =0; i<15; i++){
                    int accurrent = analogRead(i);
        if (accurrent > maxac[i]){
           maxac[i] = accurrent;        
        }
          }

	EthernetClient client = server.available();

	if (client) {
		// an http request ends with a blank line
		boolean current_line_is_blank = true;

		// reset the input buffer
		index = 0;
               unsigned long maxtime = millis() + 1000;
		while (client.connected()) {
                        if (maxtime < millis()) break;
			if (client.available()) {
				char c = client.read();

				// If it isn't a new line, add the character to the buffer
				if (c != '\n' && c != '\r') {
					clientline[index] = c;
					index++;
					// are we too big for the buffer? start tossing out data
					if (index >= BUFSIZ)
						index = BUFSIZ - 1;

					// continue to read more data!
					continue;
				}

				// got a \n or \r new line, which means the string is done
				clientline[index] = 0;

				// Look for substring such as a request to get the root file
				if (strstr(clientline, "GET / ") != 0) {
					// send a standard http response header
					client.println("HTTP/1.1 200 OK");
					client.println("Content-Type: text/html");
					client.println();
					if (!file.open(&root, "INDEX.HTM", O_READ)) {
			                     client.println("Failed to read SD card");
					}

					int16_t c;
                                        
					while ((c = file.read()) > 0) {
						// uncomment the serial to debug (slow!)
						//Serial.print((char)c);
                                                client.print((char) c);				
 
                                          }
					file.close();



				} else if (strstr(clientline, "GET /t/") != 0) {
					char *check;
					String args[8] = "";
					int upto = 0;
					check = clientline + 7;
					(strstr(clientline, " HTTP"))[0] = 0;
					char * pnt;
					char dem[] = "/";
					pnt = strtok(check, dem);
					while (pnt != NULL && upto < 8) {

						debug("TOKEN", -1, pnt);
						args[upto] = pnt;
						pnt = strtok(NULL, dem);
						upto++;
					}


					if ((MakeHash(
							tokens[0] + PSK + args[1] + args[2] + args[3]
									+ args[4] + args[5] + args[6] + args[7])
							== args[0])||
(MakeHash(
							tokens[1] + PSK + args[1] + args[2] + args[3]
									+ args[4] + args[5] + args[6] + args[7])
							== args[0])||
(MakeHash(
							tokens[2] + PSK + args[1] + args[2] + args[3]
									+ args[4] + args[5] + args[6] + args[7])
							== args[0]))

{
						debug("TOKEN", -1, "Passed Auth");

						if (args[1] == "output") {
								char name[args[2].length()];
								for (int i = 0; i < args[2].length(); i++) {
									name[i] = args[2].charAt(i);
								}
								int output = atoi(name);
			                                        output_toggle(output);
  sendstatus(client);

	
						}					
					} else {
						client.println("HTTP/1.1 403 Forbidden");
						client.println("Content-Type: text/html");
						client.println();
						delay(200);
						client.println("403 - Failed Auth");
						debug("TOKEN", -1, "Failed Auth");

					}

				} else if (strstr(clientline, "GET /t ") != 0) {
					client.println("HTTP/1.1 200 OK");
					client.println("Content-Type: application/javascript");
					client.println();
					client.println("token({\"token\": \"" + tokens[currenttoken] + "\"});");

					// print all the files, use a helper to keep it clean
                                } else if (strstr(clientline, "GET /t?") != 0) {
                              		client.println("HTTP/1.1 200 OK");
					client.println("Content-Type: application/javascript");
					client.println();
					client.println("token({\"token\": \"" + tokens[currenttoken] + "\"});");

 				} else if (strstr(clientline, "GET /status") != 0) {
sendstatus(client);
                                                                   
				} else if (strstr(clientline, "GET /") != 0) {
					// this time no space after the /, so a sub-file!
					char *filename;

					filename = clientline + 5; // look after the "GET /" (5 chars)
					// a little trick, look for the " HTTP/1.1" string and
					// turn the first character of the substring into a 0 to clear it out.
					(strstr(clientline, " HTTP"))[0] = 0;

					if (!file.open(&root, filename, O_READ)) {
					client.println("HTTP/1.1 200 OK");
					client.println("Content-Type: text/html");
					client.println();
					ListFiles(client, LS_SIZE);
						break;
					}

					client.println("HTTP/1.1 200 OK");
					client.println("Content-Type: text/html");
					client.println();

					int16_t c;
					while ((c = file.read()) > 0) {
						// uncomment the serial to debug (slow!)
						//Serial.print((char)c);
						client.print((char) c);
					}
					file.close();
				} else {
					// everything else is a 404
					client.println("HTTP/1.1 200 OK");
					client.println("Content-Type: text/html");
					client.println();
					ListFiles(client, LS_SIZE);
				}
				break;
			}
		}

		// give the web browser time to receive the data
		delay(2);
		client.stop();
	}

}

void debug(String component, int subcomponent, String message) {
	if (DEBUG_MESSAGES == true) {
		Serial.print(component);
		Serial.print("/");
		Serial.print(subcomponent);
		Serial.print(" - ");
		Serial.println(message);
		Serial.print("MEM/0 - Free:");
		Serial.println(freeMemory());
	} 
}

void setup_temp(int DHT11_PIN) {
	debug("TEMP", DHT11_PIN, "STARTING");
	nexttemp = millis() + 2000;
  dhtin.begin();
  dhtout.begin();
	debug("TEMP", DHT11_PIN, "STARTED");

}

bool output_toggle(int OUTPUT_PIN) {
	if (digitalRead(OUTPUT_PIN) == HIGH) {
		digitalWrite(OUTPUT_PIN, LOW);
		debug("RELAY", OUTPUT_PIN, "TOGGLED OFF");
		return (false);
	} else {
		digitalWrite(OUTPUT_PIN, HIGH);
		debug("RELAY", OUTPUT_PIN, "TOGGLED ON");
		return (true);
	}
}

void setup_pins() {
	debug("OUTPUTS", -1, "STARTING");
	pinMode(13, OUTPUT);
	pinMode(4, OUTPUT);
	for (int i = 22; i < 42; i++){
		pinMode(i, OUTPUT);	}
	debug("OUTPUTS", -1, "STARTED");
}

void setup_sdcard() {
	debug("SD", -1, "STARTING");
	// initialize the SD card at SPI_HALF_SPEED to avoid bus errors with
	// breadboards.  use SPI_FULL_SPEED for better performance.
	pinMode(10, OUTPUT); // set the SS pin as an output (necessary!)
	digitalWrite(10, HIGH); // but turn off the W5100 chip!

	if (!card.init(SPI_FULL_SPEED, 4))
		debug("SD", -1, "CARD INIT FAILED");

	// initialize a FAT volume
	if (!volume.init(&card))
		debug("SD", -1, "VOLUME INIT FAILED");

	if (!root.openRoot(&volume))
		debug("SD", -1, "OPEN ROOT FAILED");

	debug("SD", -1, "STARTED");
}

void setup_network() {
	debug("NETWORK", -1, "STARTING");
	Ethernet.begin(mac);
	W5100.setRetransmissionTime(0x07D0); // This code is meant to stoy lock ups with wiznet
	W5100.setRetransmissionCount(3);
	server.begin();
	debug("NETWORK", -1, "STARTED");
}

void ListFiles(EthernetClient client, uint8_t flags) {
	// This code is just copied from SdFile.cpp in the SDFat library
	// and tweaked to print to the client output in html!
	dir_t p;

	root.rewind();
	client.println("<ul>");
	while (root.readDir(p) > 0) {
		// done if past last used entry
		if (p.name[0] == DIR_NAME_FREE)
			break;

		// skip deleted entry and entries for . and  ..
		if (p.name[0] == DIR_NAME_DELETED || p.name[0] == '.')
			continue;

		// only list subdirectories and files
		if (!DIR_IS_FILE_OR_SUBDIR(&p))
			continue;

		// print any indent spaces
		client.print("<li><a href=\"");
		for (uint8_t i = 0; i < 11; i++) {
			if (p.name[i] == ' ')
				continue;
			if (i == 8) {
				client.print('.');
			}
			client.print((char) p.name[i]);
		}
		client.print("\">");

		// print file name with possible blank fill
		for (uint8_t i = 0; i < 11; i++) {
			if (p.name[i] == ' ')
				continue;
			if (i == 8) {
				client.print('.');
			}
			client.print((char) p.name[i]);
		}

		client.print("</a>");

		if (DIR_IS_SUBDIR(&p)) {
			client.print('/');
		}

		// print modify date/time if requested
		if (flags & LS_DATE) {
			root.printFatDate(p.lastWriteDate);
			client.print(' ');
			root.printFatTime(p.lastWriteTime);
		}
		// print size if requested
		if (!DIR_IS_SUBDIR(&p) && (flags & LS_SIZE)) {
			client.print(' ');
			client.print(p.fileSize);
		}
		client.println("</li>");
	}
	client.println("</ul>");
}

String MakeChallenge() {
	debug("CHALLENGE", -1, "GENERATING CHALLENGE");
	char challenge[17];
	for (int i = 0; i <= 15; i++) {
		int randomnumber = random(1, 62);
		if (randomnumber < 26) {
			challenge[i] = char(int(65) + randomnumber);
		} else if (randomnumber >= 26 & randomnumber < 52) {
			challenge[i] = char(int(97) + randomnumber - 26);
		} else {
			challenge[i] = char(int(48) + randomnumber - 52);
		}

	}
	challenge[16] = 0x00; //Add null terminator to string
	debug("CHALLENGE", -1, challenge);
	return (String(challenge));
}

String MakeHash(String test) {
	char tochar[test.length() + 1];
	for (int i = 0; i < test.length(); i++) {
		tochar[i] = test.charAt(i);
	}
	tochar[test.length()] = NULL;
	debug("HASH", -1, "HASHING " + String(tochar));

	unsigned char* hash = MD5::make_hash(tochar);
	char* md5str = MD5::make_digest(hash, 16);

	String returnstring = String(md5str);
	returnstring.toLowerCase();
	free(md5str); // stupid malloc issue.
	debug("HASH", -1, returnstring);
	return (returnstring);
}

void refreshtoken() {
        currenttoken++;
        if (currenttoken > 2){
          currenttoken = 0;
        };
     
	tokens[currenttoken] = MakeChallenge();
}

void sendstatus(EthernetClient client){
  					client.println("HTTP/1.1 200 OK");
					client.println("Content-Type: application/javascript");
					client.println();
							client.println("status({");
							for (int i = 22; i < 42; i++){
								client.println("\"D" + String(i)+"\":"+String(digitalRead(i))+",");
							} 
                                                        for (int i = 0; i < 15; i++){
								client.println("\"A" + String(i)+"\":"+String(laststate[i])+",");
							} 
								client.print("\"T0\":");
                                                                client.print(tin);
								client.print(",\n\"T1\":");
                                                                client.print(tout);
								client.print(",\n\"H0\":");
                                                                client.print(hin);
								client.print(",\n\"H1\":");
                                                                client.print(hout);
							client.println("});");
					client.println("token({\"token\": \"" + tokens[currenttoken] + "\"});");
                                  
}

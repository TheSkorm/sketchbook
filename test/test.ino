#define DEBUG_MESSAGES  true
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
String tokens[2];
int currenttoken = 0;
unsigned long lastkeychange = 0;
String PSK = "";

/************ AC stuff ************/
unsigned long lastaccheck = 0;
int maxac[15];
int laststate[15];
int currentaccheck =0;
int accycletimes =0;

/************ TEMP/HUMID STUFF ************/
//#define DHT11_PIN 6      // ADC0
// How big our line buffer should be. 100 is plenty!
unsigned long nexttemp;
#include "DHT.h"
#define DHTTYPE DHT11
DHT dhtin(46, DHTTYPE);
DHT dhtout(47, DHTTYPE);
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

/************ Scheduler STUFF ************/
unsigned long lastschedcheck;
unsigned long actionmillis[5]; // time to perform action  TODO Turn this into struct
bool action[5]; // 0/1 on/off
bool active[5]; // 0/1 enabled
int actionpin[5]; // 0-255 pin to turn on or off
bool readpinad[5]; // 0/1 is the pin a or digital
int readpin[5]; // 0-255 pin to read to check if on or off

/************ Door lock *****************/
bool lastdoor;
#define DOORLOCKOUTPUT 45
#define DOORLOCKINPUT 43
/** checks the door unlock switch **/
void checkdoor(){ 
  bool reading = digitalRead(DOORLOCKINPUT);
  if (lastdoor != reading){
      lastdoor = reading;
      toggle_door(); 
      debug("LCK",0,"Door lock toggled");
   }
}

//changes door state
void toggle_door(){
         debug("LCK",0,"Door lock toggled2");
   digitalWrite(DOORLOCKOUTPUT, HIGH);
 //  int x = 0;
   int y = -1;
     for (int x = 0; x < 5; x++) {
         if (active[x] == 1) {
            if (actionpin[x] == DOORLOCKOUTPUT){  // check for existing lock to extend
               y = x;
            }
         }
      }
      if (y == -1)
      {
           for (int x = 0; x < 5; x++) {
            if (active[x] == 0) {
             y = x;   //findd empty sched
             break;
             }
      }
   }
      if (y == -1){ // if all else fails overwrite 0
         y = 0;
      }

         actionmillis[y] =  millis() + 2000; //set the door to unlock for 12 seconds
         active[y] = true;
         action[y] = false;
         actionpin[y] = DOORLOCKOUTPUT;
         readpinad[y] = 1;
         readpin[y] = DOORLOCKOUTPUT; // just read the state of the IO.

}

void setuppsk() {
   if (!sd.init(SPI_HALF_SPEED, chipSelect))
      sd.initErrorHalt(); // Code to make the json config file
   if (!file.open("password", O_READ)) {
      #if DEBUG_MESSAGES == true
      debug("PSK", -1, "Failed to read SD card for PSK");
      #endif
   }

   int16_t c;
   String pass = "";
   int b = 0;
   while ((c = file.read()) > 0) {
      if ((char) c != 0x0A && (char) c != 0x0D) { //skip new lines
         pass = String(pass + (char) c);
      } else {
         break;
      }
   }
   file.close();
   PSK = pass; // TODO multi PSKs read off SD card or something
         #if DEBUG_MESSAGES == true
   debug("PSK", -1, "password is " + PSK);
   #endif

}

void setup() {




   lastkeychange = millis();
   Serial.begin(57600);
   #if DEBUG_MESSAGES == true
   debug("SERIAL", -1, "STARTED");
   #endif
   randomSeed(analogRead(0)); //randommize the ardiuno generotor
   setuppsk();
   setup_pins();
   setup_sdcard();
   setup_network();
   refreshtoken();
   refreshtoken();




   if (!sd.init(SPI_HALF_SPEED, chipSelect))
      sd.initErrorHalt(); // Code to make the json config file


// TODO Make config get sent as script MIME
      sd.remove("config.js");
   if (!myFile.open("config.js", O_RDWR | O_CREAT | O_TRUNC)) {
      sd.errorHalt("opening config.js for write failed");
   }
   myFile.println("config({");
   myFile.println("\"D22\" : [{\"description\": \"Front Outside Light\", \"type\": \"switch\",\"read\": \"A0\"}],");
   myFile.println("\"D23\" : [{\"description\": \"Kitchen\", \"type\": \"switch\",\"read\": \"A1\"}],");
   myFile.println("\"D24\" : [{\"description\": \"Dinning Room\", \"type\": \"switch\",\"read\": \"A2\"}],");
   myFile.println("\"D25\" : [{\"description\": \"Lounge Room 2\", \"type\": \"switch\",\"read\": \"A3\"}],");
   myFile.println("\"D26\" : [{\"description\": \"Lounge Room 1\", \"type\": \"switch\",\"read\": \"A4\"}],");
   myFile.println("\"D27\" : [{\"description\": \"Lounge Room Fan\", \"type\": \"switch\",\"read\": \"A5\"}],");
   myFile.println("\"D28\" : [{\"description\": \"Bedroom 1\", \"type\": \"switch\",\"read\": \"A6\"}],");
   myFile.println("\"D29\" : [{\"description\": \"Bedroom 2\", \"type\": \"switch\",\"read\": \"A7\"}],");
   myFile.println("\"D30\" : [{\"description\": \"Bedroom 3\", \"type\": \"switch\",\"read\": \"A8\"}],");
   myFile.println("\"D32\" : [{\"description\": \"Bedroom 1 Fan\", \"type\": \"switch\",\"read\": \"A10\"}],");
   myFile.println("\"D31\" : [{\"description\": \"Bedroom 2 Fan\", \"type\": \"switch\",\"read\": \"A9\"}],");
   myFile.println("\"D45\" : [{\"description\": \"Front Door\", \"type\": \"door\"}],");
   myFile.println("\"T0\" : [{\"description\": \"Outside Temp\", \"type\": \"temp\"}],");
   myFile.println("\"T1\" : [{\"description\": \"Inside Temp\", \"type\": \"temp\"}],");
   myFile.println("\"H0\" : [{\"description\": \"Outside Humid\", \"type\": \"humid\"}],");
   myFile.println("\"H1\" : [{\"description\": \"Inside Humid\", \"type\": \"humid\"}],");
   myFile.println("\"A1\" : [{\"description\": \"Light Sensor\", \"type\": \"light\"}],");
   myFile.println("  });");

   myFile.close();

}

void loop() {
   checkdoor();


   static char clientline[BUFSIZ];
   int index = 0;
   unsigned long currentMillis = millis();
      #if DEBUG_MESSAGES == true
    //  debug("LOOP", -1, String(currentMillis));
      #endif
   if (currentMillis - lastkeychange > 10000) {
      lastkeychange = currentMillis;
      refreshtoken();
      updatetemp(); //update temp takes time. Maybe check one sensor ever 10 seconds rather than both

   }

   if (currentMillis - lastschedcheck > 50) { // check the scheduler
      checksched(currentMillis);
   }
   
if (currentMillis - lastaccheck > 500) { //reset AC 
   resetac(currentMillis);
   }

   checkac(currentaccheck);
   accycletimes = accycletimes +1;
   currentaccheck = currentaccheck + 1;
   if (currentaccheck > 14)
      currentaccheck =0;

   EthernetClient client = server.available();

   if (client) {
      // an http request ends with a blank line
      boolean current_line_is_blank = true;

      // reset the input buffer
      index = 0;
      unsigned long maxtime = millis() + 5000;
      while (client.connected()) {
         if (maxtime < millis())
            break;
         if (client.available()) {
            char c = client.read();

            // If it isn't a new line, add the character to the buffer
            if (c != '\n' && c != '\r') {
               clientline[index] = c;
               index++;
               // are we too big for the buffer? start tossing out data
               if (index >= BUFSIZ -1)
                  index = BUFSIZ - 1;

               // continue to read more data!
               continue;
            }

            // got a \n or \r new line, which means the string is done
            clientline[index] = 0;

            // Look for substring such as a request to get the root file
            clientline[BUFSIZ -1] = NULL;
            if (strstr(clientline, "GET / ") != 0) {
               // send a standard http response header
               showindex(client);

            } else if (strstr(clientline, "GET /t/") != 0) {

               String args[8];
               String tosplit = String(clientline);
               tosplit = tosplit.substring(0, tosplit.indexOf(" HTTP"));
               int lastfind = 0;
               int togo = 0;
               int argno = 2;
               Serial.println(tosplit);
               while (tosplit.indexOf('/',lastfind) != -1){
                     argno = togo - 2;
                     if (togo != 0 && togo != 1){ 
                     Serial.println(tokens[0]);
                     Serial.println(tokens[1]);
                     args[argno] = tosplit.substring(lastfind,tosplit.indexOf('/',lastfind));
                     Serial.println(String(argno) + " - " + args[argno]);
                     Serial.println(tokens[0]);
                     Serial.println(tokens[1]);
                     }
                     lastfind = tosplit.indexOf('/',lastfind) + 1;
                     togo++;
                     if (togo == 9){
                        break;
                     }
               }
                     argno = togo - 2;
                     Serial.println(tokens[0]);  // get the last one
                     Serial.println(tokens[1]);
                     args[argno] = tosplit.substring(lastfind);
                     Serial.println(String(argno) + " - " + args[argno]);
                     Serial.println(tokens[0]);
                     Serial.println(tokens[1]);





//                 char *p = clientline + NULL;
//                 char *str;
//                  char *args[8];
//                int upto = 0;
//                (strstr(clientline, " HTTP"))[0] = 0;
// Serial.println(clientline);
//                while ((str = strtok_r(p, "/", &p)) != NULL) {
//                      if (upto != 0 && upto != 1){ //skip the first and second
//                      int  argno = upto - 2; //adjust for skipping the first two
//                      args[argno] = str;
//                      Serial.println(String(argno) + " - " + String(str));
//                      }
//                      upto++;
//                      if (upto ==8 + 2) {
//                         break; // we got our 8 inputs, break out.
//                      }
//                }
                        Serial.println("Reached end");
               if (checkhash(args)) {
                        #if DEBUG_MESSAGES == true
                  debug("TOKEN", -1, "Passed Auth");
                  #endif

                  if (args[1] == "output") {
                     httptoggleoutput(args, client);

                  } else if (args[1] == "schedu") {
                     httpschedule(args, client);
                  }

               } else {
                  http403(client);
               }

            } else if (strstr(clientline, "GET /t ") != 0) {
               httptoken(client);

            } else if (strstr(clientline, "GET /t?") != 0) {
               httptoken(client);
            } else if (strstr(clientline, "GET /password") != 0) {
               http403(client);
            } else if (strstr(clientline, "GET /status") != 0) {
               sendstatus(client);

            } else if (strstr(clientline, "GET /") != 0) {
               httpreadfile(client, clientline);
            } else {
               // everything else is a 404
               client.println("HTTP/1.1 404 Not found");
               client.println("Content-Type: text/html");
               client.println();
               client.println("Not found - nothing to see here");
            }
            break;
         }
      }

      // give the web browser time to receive the data
      delay(2);
      client.stop();
   }

}
      #if DEBUG_MESSAGES == true
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
#endif
void setup_temp(int DHT11_PIN) {
         #if DEBUG_MESSAGES == true
   debug("TEMP", DHT11_PIN, "STARTING");
   #endif
   nexttemp = millis() + 2000;
   dhtin.begin();
   dhtout.begin();
         #if DEBUG_MESSAGES == true
   debug("TEMP", DHT11_PIN, "STARTED");
   #endif

}

bool output_toggle(int OUTPUT_PIN) {
   if (digitalRead(OUTPUT_PIN) == HIGH) {
      digitalWrite(OUTPUT_PIN, LOW);
            #if DEBUG_MESSAGES == true
      debug("RELAY", OUTPUT_PIN, "TOGGLED OFF");
      #endif
      return (false);
   } else {
      digitalWrite(OUTPUT_PIN, HIGH);
            #if DEBUG_MESSAGES == true
      debug("RELAY", OUTPUT_PIN, "TOGGLED ON");
      #endif
      return (true);
   }
}

void setup_pins() {
         #if DEBUG_MESSAGES == true

   debug("OUTPUTS", -1, "STARTING");
#endif
   pinMode(13, OUTPUT);
   pinMode(4, OUTPUT);
   pinMode(45, OUTPUT);
   for (int i = 22; i < 42; i++) {
      pinMode(i, OUTPUT);
   }
         #if DEBUG_MESSAGES == true

   debug("OUTPUTS", -1, "STARTED");
#endif
}

void setup_sdcard() {
         #if DEBUG_MESSAGES == true
   debug("SD", -1, "STARTING");
   #endif
   // initialize the SD card at SPI_HALF_SPEED to avoid bus errors with
   // breadboards.  use SPI_HALF_SPEED for better performance.
   pinMode(10, OUTPUT); // set the SS pin as an output (necessary!)
   digitalWrite(10, HIGH); // but turn off the W5100 chip!

   if (!card.init(SPI_HALF_SPEED, 4)){
            #if DEBUG_MESSAGES == true
      debug("SD", -1, "CARD INIT FAILED");
#endif
}
   // initialize a FAT volume
   if (!volume.init(&card)){
            #if DEBUG_MESSAGES == true
      debug("SD", -1, "VOLUME INIT FAILED");

#endif
   }
   if (!root.openRoot(&volume)){
      #if DEBUG_MESSAGES == true
      debug("SD", -1, "OPEN ROOT FAILED");
#endif
}
      #if DEBUG_MESSAGES == true
   debug("SD", -1, "STARTED");
   #endif
}

void setup_network() {
         #if DEBUG_MESSAGES == true
   debug("NETWORK", -1, "STARTING");
   #endif
   Ethernet.begin(mac);
   W5100.setRetransmissionTime(0x07D0); // This code is meant to stoy lock ups with wiznet
   W5100.setRetransmissionCount(3);
   server.begin();
         #if DEBUG_MESSAGES == true
   debug("NETWORK", -1, "STARTED");
   #endif
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
         #if DEBUG_MESSAGES == true
   debug("CHALLENGE", -1, "GENERATING CHALLENGE");
   #endif
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
         #if DEBUG_MESSAGES == true
   debug("CHALLENGE", -1, challenge);
   #endif
   return (String(challenge));
}

String MakeHash(String test) {
            #if DEBUG_MESSAGES == true
   debug("HASH", -1, "HASHING " + test);
   #endif
   char tochar[test.length() + 1];
   for (int i = 0; i < test.length(); i++) {
      tochar[i] = test.charAt(i);
   }
   tochar[test.length()] = NULL;

         #if DEBUG_MESSAGES == true
   debug("HASH", -1, "HASHING2 " + String(tochar));
   #endif
   unsigned char* hash = MD5::make_hash(tochar);
   char* md5str = MD5::make_digest(hash, 16);

   String returnstring = String(md5str);
   returnstring.toLowerCase();
   free(md5str); // stupid malloc issue.
         #if DEBUG_MESSAGES == true
   debug("HASH", -1, returnstring);
   #endif
   return (returnstring);
}

void refreshtoken() {
   currenttoken++;
   if (currenttoken > 1) {
      currenttoken = 0;
   };

   tokens[currenttoken] = MakeChallenge();
}

void sendstatus(EthernetClient client) {
   client.println("HTTP/1.1 200 OK");
   client.println("Content-Type: application/javascript");
   client.println();
   client.println("status({");
   for (int i = 22; i < 42; i++) {
      client.println("\"D" + String(i) + "\":" + String(digitalRead(i)) + ",");
   }
   for (int i = 0; i < 15; i++) {
      client.println("\"A" + String(i) + "\":" + String(laststate[i]) + ",");
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

void updatetemp() {
   hin = dhtin.readHumidity();
   tin = dhtin.readTemperature();
  // hout = dhtout.readHumidity();
  // tout = dhtout.readTemperature();
}

void checksched(unsigned long currentMillis) {
   lastschedcheck = currentMillis;
   for (int i = 0; i < 5; i++) { // this can be a funciotn - scheduler
      if (active[i]) {
         if (currentMillis > actionmillis[i]) {
                  #if DEBUG_MESSAGES == true
      debug("SCHED", -1, "Fired");
      #endif
            if (readpinad[i] == true) {

                                 #if DEBUG_MESSAGES == true
      debug("SCHED", -1, "Fired2");
      #endif
               if ((bool) digitalRead(readpin[i]) != (bool) action[i])
                  output_toggle(actionpin[i]);
            } else {
                                 #if DEBUG_MESSAGES == true
      debug("SCHED", -1, "Fired3");
      #endif
               if ((bool) laststate[readpin[i]] != (bool) action[i])
                  output_toggle(actionpin[i]);
            }
            active[i] = false;
         }
      }
   }
}

void resetac(unsigned long currentMillis) {
   lastaccheck = currentMillis;
   for (int i = 0; i < 15; i++) {
      laststate[i] = maxac[i];
      maxac[i] = 0;
   }
}

void checkac(int pin) {
      int accurrent = analogRead(pin);
      if (accurrent > maxac[pin]) {
         maxac[pin] = accurrent;
      }
   }

void showindex(EthernetClient client) {
   client.println("HTTP/1.1 200 OK");
   client.println("Content-Type: text/html");
   client.println();
   if (!file.open(&root, "INDEX.HTM", O_READ)) {
      client.println("Failed to read SD card");
   }

   int16_t c;

   while ((c = file.read()) > 0) {
      client.print((char) c);

   }
   file.close();
}

bool checkhash(String args[8]) {

   if ((MakeHash(tokens[0] + PSK + args[1] + args[2] + args[3] + args[4] + args[5] + args[6] + args[7]) == args[0]) || (MakeHash(tokens[1] + PSK + args[1] + args[2] + args[3] + args[4] + args[5] + args[6]  + args[7]) == args[0])) {
      return (true);
   } else {
      return (false);
   }
}
;

void httptoggleoutput(String args[8], EthernetClient client) {
   char name[args[2].length() + 1];
   for (int i = 0; i < args[2].length(); i++) {
      name[i] = args[2].charAt(i);
   }
   int output = atoi(name);

   output_toggle(output);
   delay(5);
   unsigned long lastaccheck = millis();
   for (int i = 0; i < 15; i++) {
      maxac[i] = 0;
   }
   while (lastaccheck + 100 > millis()) {
      for (int i = 0; i < 15; i++) {
         int accurrent = analogRead(i);
         if (accurrent > maxac[i]) {
            maxac[i] = accurrent;
         }
      }
   }
   for (int i = 0; i < 15; i++) {
      laststate[i] = maxac[i];
   }

   sendstatus(client);
}

void httpschedule(String args[8], EthernetClient client) {
     for (int x = 0; x < 5; x++) {
      if (active[x] == 0) {
         char newactiontime[args[2].length() + 1]; //TODO this should be a long or some shit
         for (int i = 0; i < args[2].length(); i++) {
            newactiontime[i] = args[2].charAt(i);
         }
         actionmillis[x] = (atoi(newactiontime)*1000) + millis();
         char newaction[args[3].length() + 1];
         for (int i = 0; i < args[3].length(); i++) {
            newaction[i] = args[3].charAt(i);
         }
         if (atoi(newaction) > 0) {
            action[x] = true;
         } else {
            action[x] = false;
         };
         char newactionpin[args[4].length() + 1];
         for (int i = 0; i < args[4].length(); i++) {
            newactionpin[i] = args[4].charAt(i);
         }
         actionpin[x] = atoi(newactionpin);

         if (args[5] == "A") {
            readpinad[x] = false;
         } else {
            readpinad[x] = true;
         }

         char newreadpin[args[6].length() + 1];
         for (int i = 0; i < args[6].length(); i++) {
            newreadpin[i] = args[6].charAt(i);
         }
         readpin[x] = atoi(newreadpin);
         active[x] = true;
         Serial.println();
         client.println("HTTP/1.1 200 OK");
         client.println("Content-Type: text/html");
         client.println();
         client.println(x);
         break;
      }
   }

   client.println("HTTP/1.1 502");
   client.println("Content-Type: text/html");
   client.println();
   client.println("failed to find free queue"); //todo make failure callback
}

void httptoken(EthernetClient client) {
   client.println("HTTP/1.1 200 OK");
   client.println("Content-Type: application/javascript");
   client.println();
   client.println("token({\"token\": \"" + tokens[currenttoken] + "\"});");
}

void http403(EthernetClient client) {
   client.println("HTTP/1.1 403 Forbidden");
   client.println("Content-Type: text/html");
   client.println();
   client.println("403 - Failed Auth");
         #if DEBUG_MESSAGES == true
   debug("TOKEN", -1, "Failed Auth");
   #endif
}

void httpreadfile(EthernetClient client, char* clientline) {
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
      return;
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
}

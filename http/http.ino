/*
 * USERS OF ARDUINO 0023 AND EARLIER: use the 'SDWebBrowse.pde' sketch...
 * 'SDWebBrowse.ino' can be ignored.
 * USERS OF ARDUINO 1.0 AND LATER: **DELETE** the 'SDWebBrowse.pde' sketch
 * and use ONLY the 'SDWebBrowse.ino' file.  By default, BOTH files will
 * load when using the Sketchbook menu, and the .pde version will cause
 * compiler errors in 1.0.  Delete the .pde, then load the sketch.
 *
 * I can't explain WHY this is necessary, but something among the various
 * libraries here appears to be wreaking inexplicable havoc with the
 * 'ARDUINO' definition, making the usual version test unusable (BOTH
 * cases evaluate as true).  FML.
 */

/*
 * This sketch uses the microSD card slot on the Arduino Ethernet shield
 * to serve up files over a very minimal browsing interface
 *
 * Some code is from Bill Greiman's SdFatLib examples, some is from the
 * Arduino Ethernet WebServer example and the rest is from Limor Fried
 * (Adafruit) so its probably under GPL
 *
 * Tutorial is at http://www.ladyada.net/learn/arduino/ethfiles.html
 * Pull requests should go to http://github.com/adafruit/SDWebBrowse
 */

#include <SdFat.h>
/* #include <SdFatUtil.h> */
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>
#include <MemoryFree.h>

#include <utility/w5100.h> 

/************ TEMP/HUMID STUFF ************/
//#define DHT11_PIN 6      // ADC0
// How big our line buffer should be. 100 is plenty!
unsigned long nexttemp; 

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


void setup() {
  Serial.begin(9600); // Set serial at 9600 baud rate for debugging
 
 //pin configuration for relays and LEDs
 pinMode(13, OUTPUT);
 pinMode(33, OUTPUT);
 pinMode(35, OUTPUT);
 
 //Internal timer for how often to check temp
 nexttemp = millis() + 2000;

  // initialize the SD card at SPI_HALF_SPEED to avoid bus errors with
  // breadboards.  use SPI_FULL_SPEED for better performance.
  pinMode(10, OUTPUT);                       // set the SS pin as an output (necessary!)
  digitalWrite(10, HIGH);                    // but turn off the W5100 chip!

  if (!card.init(SPI_HALF_SPEED, 4)) Serial.println("card.init failed!");
  
  // initialize a FAT volume
  if (!volume.init(&card)) Serial.println("vol.init failed!");

  if (!root.openRoot(&volume))Serial.println("openRoot failed");
  
  // Debugging complete, we start the server!
  Ethernet.begin(mac);
  W5100.setRetransmissionTime(0x07D0); // This code is meant to stoy lock ups with wiznet
  W5100.setRetransmissionCount(3); 
  server.begin();
}

void ListFiles(EthernetClient client, uint8_t flags) {
  // This code is just copied from SdFile.cpp in the SDFat library
  // and tweaked to print to the client output in html!
  dir_t p;
  
  root.rewind();
  client.println("<ul>");
  while (root.readDir(p) > 0) {
    // done if past last used entry
    if (p.name[0] == DIR_NAME_FREE) break;

    // skip deleted entry and entries for . and  ..
    if (p.name[0] == DIR_NAME_DELETED || p.name[0] == '.') continue;

    // only list subdirectories and files
    if (!DIR_IS_FILE_OR_SUBDIR(&p)) continue;

    // print any indent spaces
    client.print("<li><a href=\"");
    for (uint8_t i = 0; i < 11; i++) {
      if (p.name[i] == ' ') continue;
      if (i == 8) {
        client.print('.');
      }
      client.print((char)p.name[i]);
    }
    client.print("\">");
    
    // print file name with possible blank fill
    for (uint8_t i = 0; i < 11; i++) {
      if (p.name[i] == ' ') continue;
      if (i == 8) {
        client.print('.');
      }
      client.print((char)p.name[i]);
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
byte read_dht11_dat(int READ_DHT11_PIN)
{
  byte i = 0;
  byte result=0;
  for(i=0; i< 8; i++){
 
 
    while(!(PINF & _BV(READ_DHT11_PIN)));  // wait for 50us
    delayMicroseconds(30);
 
    if(PINF & _BV(READ_DHT11_PIN)) 
      result |=(1<<(7-i));
    while((PINF & _BV(READ_DHT11_PIN)));  // wait '1' finish
 
 
  }
  return result;
}

String CheckTemp(int DHT11_PIN){
  #define BUFSIZ 100
  byte dht11_dat[5]; // define this so that our DHT data is avaiable everywhere ; TODO make a proper place to store temp and check the checksum
  DDRF |= _BV(DHT11_PIN);
  PORTF |= _BV(DHT11_PIN);
  byte dht11_in;
  byte i;
  String prettytemph;
  // start condition
  // 1. pull-down i/o pin from 18ms
  PORTF &= ~_BV(DHT11_PIN);
  delay(18);
  PORTF |= _BV(DHT11_PIN);
  delayMicroseconds(40);
 
  DDRF &= ~_BV(DHT11_PIN);
  delayMicroseconds(40);
 
  dht11_in = PINF & _BV(DHT11_PIN);
 
  if(dht11_in){
    Serial.println("dht11 start condition 1 not met");
    return "Error";
  }
  delayMicroseconds(80);
 
  dht11_in = PINF & _BV(DHT11_PIN);
 
  if(!dht11_in){
    Serial.println("dht11 start condition 2 not met");
    return "Error";
  }

  delayMicroseconds(80);
  // now ready for data reception
  
  for (i=0; i<5; i++){
    dht11_dat[i] = read_dht11_dat(DHT11_PIN);
    Serial.println(i);
  }
 
  DDRF |= _BV(DHT11_PIN);
  PORTF |= _BV(DHT11_PIN);

  byte dht11_check_sum = dht11_dat[0]+dht11_dat[1]+dht11_dat[2]+dht11_dat[3];
  // check check_sum
  if(dht11_dat[4]!= dht11_check_sum)
  {
    Serial.println("DHT11 checksum error");
  }
  
  prettytemph = "Current humdity = ";
  return prettytemph;
/*  prettytemph.println(dht11_dat[0], DEC);
  prettytemph.println(".");
  prettytemph.println(dht11_dat[1], DEC);
  prettytemph.println("%  ");
  prettytemph.println("temperature = ");
  prettytemph.println(dht11_dat[2], DEC);
  prettytemph.println(".");
  prettytemph.println(dht11_dat[3], DEC);
  prettytemph.println("C  "); */
}

 String templine;
void loop()
{


  if (nexttemp < millis()   ){
    nexttemp = millis() + 2000;
   templine = CheckTemp(6);
  }
          


  char clientline[BUFSIZ];
  int index = 0;
  
  EthernetClient client = server.available();
  
  
  
  
  
  
  
  
  
  if (client) {
    Serial.println("client av");
    // an http request ends with a blank line
    boolean current_line_is_blank = true;
    
    // reset the input buffer
    index = 0;
    
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        
        // If it isn't a new line, add the character to the buffer
        if (c != '\n' && c != '\r') {
          clientline[index] = c;
          index++;
          // are we too big for the buffer? start tossing out data
          if (index >= BUFSIZ) 
            index = BUFSIZ -1;
          
          // continue to read more data!
          continue;
        }
        
        // got a \n or \r new line, which means the string is done
        clientline[index] = 0;
        
        // Print it out for debugging
        Serial.println(clientline);
        
        // Look for substring such as a request to get the root file
        if (strstr(clientline, "GET / ") != 0) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          
          // print all the files, use a helper to keep it clean
          client.println("<h2>Files:</h2>");
          ListFiles(client, LS_SIZE);
        } else if (strstr(clientline, "GET /lies ") != 0) {
           if (!sd.init(SPI_HALF_SPEED, chipSelect)) sd.initErrorHalt();

           if (!myFile.open("test.txt", O_RDWR | O_CREAT | O_AT_END)) {
    sd.errorHalt("opening test.txt for write failed");
  }
    Serial.print("Writing to test.txt...");

  Serial.println("done.");
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          
          if (digitalRead(13)==HIGH){
           digitalWrite(13, LOW); 
           client.println("<h2>LED OFF</h2>");
             myFile.println("LED Turned off by user - add time here or something");

    myFile.close();
          } else {
              digitalWrite(13, HIGH);
                   client.println("<h2>LED ON</h2>");
                     myFile.println("LED Turned on by user - add time here or something");

    myFile.close();
          }
          
          
          
          
                  } else if (strstr(clientline, "GET /relay ") != 0) {
           if (!sd.init(SPI_HALF_SPEED, chipSelect)) sd.initErrorHalt();

           if (!myFile.open("test.txt", O_RDWR | O_CREAT | O_AT_END)) {
    sd.errorHalt("opening test.txt for write failed");
  }
    Serial.print("Writing to test.txt...");

  Serial.println("done.");
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          
          if (digitalRead(33)==HIGH){
           digitalWrite(33, LOW); 
           client.println("<h2>RELAY OFF</h2>");
             myFile.println("RELAY Turned off by user - add time here or something");

    myFile.close();
          } else {
              digitalWrite(33, HIGH);
                   client.println("<h2>RELAY ON</h2>");
                     myFile.println("RELAY Turned on by user - add time here or something");

    myFile.close();
          }
          
          
          
          
          
          
          
               } else if (strstr(clientline, "GET /relay2 ") != 0) {
           if (!sd.init(SPI_HALF_SPEED, chipSelect)) sd.initErrorHalt();

           if (!myFile.open("test.txt", O_RDWR | O_CREAT | O_AT_END)) {
    sd.errorHalt("opening test.txt for write failed");
  }
    Serial.print("Writing to test.txt...");

  Serial.println("done.");
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          
          if (digitalRead(35)==HIGH){
           digitalWrite(35, LOW); 
           client.println("<h2>RELAY OFF</h2>");
             myFile.println("RELAY Turned off by user - add time here or something");

    myFile.close();
          } else {
              digitalWrite(35, HIGH);
                   client.println("<h2>RELAY ON</h2>");
                     myFile.println("RELAY Turned on by user - add time here or something");

    myFile.close();
          }
          
          
          
          
          
          
                    } else if (strstr(clientline, "GET /light ") != 0) {  
          
            int sensorValue = analogRead(0);
  float Rsensor;
  Rsensor=(float)(1023-sensorValue)*10/sensorValue;
          
            
               client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();

 


 
  client.print("Current light = ");     
    client.println(Rsensor,DEC);
          
          } else if (strstr(clientline, "GET /temp ") != 0) {     
            
               client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();

          client.println(templine );
          
          // print all the files, use a helper to keep it clean
        } else if (strstr(clientline, "GET /") != 0) {
          // this time no space after the /, so a sub-file!
          char *filename;
          
          filename = clientline + 5; // look after the "GET /" (5 chars)
          // a little trick, look for the " HTTP/1.1" string and 
          // turn the first character of the substring into a 0 to clear it out.
          (strstr(clientline, " HTTP"))[0] = 0;
          
          // print the file we want
          Serial.println(filename);

          if (! file.open(&root, filename, O_READ)) {
            client.println("HTTP/1.1 404 Not Found");
            client.println("Content-Type: text/html");
            client.println();
            client.println("<h2>File Not Found!</h2>");
            break;
          }
          
          Serial.println("Opened!");
                    
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/plain");
          client.println();
          
          int16_t c;
          while ((c = file.read()) > 0) {
              // uncomment the serial to debug (slow!)
              //Serial.print((char)c);
              client.print((char)c);
          }
          file.close();
        } else {
          // everything else is a 404
          client.println("HTTP/1.1 404 Not Found");
          client.println("Content-Type: text/html");
          client.println();
          client.println("<h2>File Not Found!</h2>");
        }
        break;
      }
    }
    
    // give the web browser time to receive the data
    delay(50);
    client.stop();
    Serial.println("let go of client");
  }

}



 

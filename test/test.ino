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

String PSK="insertPSKhere";

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
	Serial.begin(9600);
	debug("SERIAL",-1,"STARTED"); 
  randomSeed(analogRead(0)); //randommize the ardiuno generotor
	setup_temp(6);
  setup_temp(7);
  setup_relays();
  setup_sdcard();
  setup_network();
}

byte read_dht11_dat(int DHT11_PIN)
{
  byte i = 0;
  byte result=0;
  for(i=0; i< 8; i++){
 
 
    while(!(PINF & _BV(DHT11_PIN)));  // wait for 50us
    delayMicroseconds(30);
 
    if(PINF & _BV(DHT11_PIN)) 
      result |=(1<<(7-i));
    unsigned long deadtimer = millis() + 500; //max wait is 100ms
    while((PINF & _BV(DHT11_PIN)) && millis() < deadtimer);  // wait '1' finish
 
 
  }
  return result;
}


dhtawesome CheckTemp(int DHT11_PIN){
	dhtawesome current;
	current.humid = 0;
	current.humid_point = 0;
	current.temp = 0;
	current.temp_point = 0;

   DDRF |= _BV(DHT11_PIN);
	PORTF |= _BV(DHT11_PIN);
 byte dht11_dat[5];
  byte dht11_in;
  byte i;
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
    debug("TEMP",DHT11_PIN,"dht11 start condition 1 not met");
    return(current);
  }
  delayMicroseconds(80);
 
  dht11_in = PINF & _BV(DHT11_PIN);
 
  if(!dht11_in){
    debug("TEMP",DHT11_PIN,"dht11 start condition 2 not met");
    return(current);
  }
  delayMicroseconds(80);
  // now ready for data reception
  for (i=0; i<5; i++){
    dht11_dat[i] = read_dht11_dat(DHT11_PIN);
  }

 
  DDRF |= _BV(DHT11_PIN);
  PORTF |= _BV(DHT11_PIN);
 
  byte dht11_check_sum = dht11_dat[0]+dht11_dat[1]+dht11_dat[2]+dht11_dat[3];
  // check check_sum
  if(dht11_dat[4]!= dht11_check_sum)
  {
     debug("TEMP",DHT11_PIN,"DHT11 checksum error");
  } else {
  current.humid = int(dht11_dat[0]);
  current.humid_point = int(dht11_dat[1]);
  current.temp = int(dht11_dat[2]);
  current.temp_point = int(dht11_dat[3]);
  }
 return(current);
}

 String templine;
void loop()
{
  if (nexttemp < millis()   ){ //2000ms timer
    nexttemp = millis() + 2000;
   dhtawesome latesttemp;
   latesttemp = CheckTemp(6);
    debug("HUMID",6,String(String(latesttemp.humid) + "." + String(latesttemp.humid_point)));
    debug("TEMP",6,String(String(latesttemp.temp) + "." + String(latesttemp.temp_point)));
   latesttemp = CheckTemp(7);
    debug("HUMID",7,String(String(latesttemp.humid) + "." + String(latesttemp.humid_point)));
    debug("TEMP",7,String(String(latesttemp.temp) + "." + String(latesttemp.temp_point)));
      MakeHash();   
    //Serial.println(relay_toggle(33));
   // MakeHash();
  }
  digitalWrite(13, HIGH);
  delay(20) ;    
    digitalWrite(13, LOW);

}






void debug(String component, int subcomponent, String message){
	if (DEBUG_MESSAGES == true){
		Serial.print(component);
		Serial.print("/");
		Serial.print(subcomponent);
		Serial.print(" - ");
		Serial.println(message);
  //  Serial.print("MEM/0 - Free:");
    //Serial.println(freeMemory());
	}
}


void setup_temp(int DHT11_PIN){
	debug("TEMP",DHT11_PIN,"STARTING");
	nexttemp = millis() + 2000;
	debug("TEMP",DHT11_PIN,"STARTED");
}

bool relay_toggle(int RELAY_PIN){
  if (digitalRead(RELAY_PIN)==HIGH){
    digitalWrite(RELAY_PIN, LOW); 
    debug("RELAY",RELAY_PIN,"TOGGLED OFF"); 
    return(false);
  } else {
    digitalWrite(RELAY_PIN, HIGH);
   debug("RELAY",RELAY_PIN,"TOGGLED ON");
   return(true);
  }
}

void setup_relays(){
  debug("RELAYS",-1,"STARTING");
  pinMode(13, OUTPUT);
  pinMode(33, OUTPUT);
  pinMode(35, OUTPUT);
  debug("RELAYS",-1,"STARTED");
}

void setup_sdcard(){
    debug("SD",-1,"STARTING");
 // initialize the SD card at SPI_HALF_SPEED to avoid bus errors with
  // breadboards.  use SPI_FULL_SPEED for better performance.
  pinMode(10, OUTPUT);                       // set the SS pin as an output (necessary!)
  digitalWrite(10, HIGH);                    // but turn off the W5100 chip!

  if (!card.init(SPI_HALF_SPEED, 4)) debug("SD",-1,"CARD INIT FAILED");
  
  // initialize a FAT volume
  if (!volume.init(&card)) debug("SD",-1,"VOLUME INIT FAILED");

  if (!root.openRoot(&volume)) debug("SD",-1,"OPEN ROOT FAILED");
  
    debug("SD",-1,"STARTED");
}

void setup_network(){
    debug("NETWORK",-1,"STARTING");
  Ethernet.begin(mac);
  W5100.setRetransmissionTime(0x07D0); // This code is meant to stoy lock ups with wiznet
  W5100.setRetransmissionCount(3); 
  server.begin();
    debug("NETWORK",-1,"STARTED");
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

void MakeHash (){
  debug("HASH",-1,"HASHING"); 
  char challenge[17] ;
  for (int i=0; i <= 15; i++){
  int randomnumber = random(1,62);
  if (randomnumber < 26){  
  challenge[i] = char(int(65)+randomnumber);
  } else if (randomnumber >= 26 & randomnumber < 52) {
  challenge[i] = char(int(97)+randomnumber-26);
  } else { 
  challenge[i] = char(int(48)+randomnumber-52);    
  }

   }
   challenge[16] = 0x00; //Add null terminator to string

    String test(challenge);
    debug("HASH-PSK",-1,PSK); 
    debug("HASH-CHALLENGE",-1,test);

    char tochar[30];      //TODO clean this shit up
   for (int i=0; i <= 29; i++){ 
   tochar[i] = test.charAt(i);
   }

   unsigned char* hash=MD5::make_hash( tochar );
   char* md5str = MD5::make_digest(hash, 16);

   debug("HASH",-1,String(md5str)); 
   free(md5str); // stupid malloc issue.
}

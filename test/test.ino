#define DEBUG_MESSAGES true
#include "header.h"



unsigned long nexttemp; 


void setup() {
	Serial.begin(9600);
	debug("SERIAL",1,"STARTED"); 
	setup_temp(6);
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
    while((PINF & _BV(DHT11_PIN)));  // wait '1' finish
 
 
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
  for (i=0; i<5; i++)
    dht11_dat[i] = read_dht11_dat(DHT11_PIN);
 
  DDRF |= _BV(DHT11_PIN);
  PORTF |= _BV(DHT11_PIN);
 
  byte dht11_check_sum = dht11_dat[0]+dht11_dat[1]+dht11_dat[2]+dht11_dat[3];
  // check check_sum
  if(dht11_dat[4]!= dht11_check_sum)
  {
     debug("TEMP",DHT11_PIN,"DHT11 checksum error");
  }
  current.humid = int(dht11_dat[0]);
  current.humid_point = int(dht11_dat[1]);
  current.temp = int(dht11_dat[2]);
  current.temp_point = int(dht11_dat[3]);
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

  }
  delay(200) ;       

}






void debug(String component, int subcomponent, String message){
	if (DEBUG_MESSAGES == true){
		Serial.print(component);
		Serial.print("/");
		Serial.print(subcomponent);
		Serial.print(" - ");
		Serial.println(message);
	}
}


void setup_temp(int DHT11_PIN){
	debug("TEMP",DHT11_PIN,"STARTING");
	nexttemp = millis() + 2000;
	debug("TEMP",DHT11_PIN,"STARTED");
}

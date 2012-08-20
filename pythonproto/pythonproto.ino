#include <SPI.h>
#include <Ethernet.h>
#include <avr/pgmspace.h>

// network configuration.  gateway and subnet are optional.
#define WAITTIME 1000

// the media access control (ethernet hardware) address for the shield:
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  
//the IP address for the shield:
byte ip[] = { 
  59, 167, 158, 119 };    
// the router's gateway address:
byte gateway[] = { 
  59, 167, 158, 65 };
// the subnet:
byte subnet[] = { 
  255, 255, 255, 192 };

// telnet defaults to port 23
EthernetServer server(23);



static PROGMEM prog_uint32_t crc_table[16] = {
  0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
  0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
  0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
  0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

unsigned long crc_update(unsigned long crc, byte data)
{
  byte tbl_idx;
  tbl_idx = crc ^ (data >> (0 * 4));
  crc = pgm_read_dword_near(crc_table + (tbl_idx & 0x0f)) ^ (crc >> 4);
  tbl_idx = crc ^ (data >> (1 * 4));
  crc = pgm_read_dword_near(crc_table + (tbl_idx & 0x0f)) ^ (crc >> 4);
  return crc;
}

//unsigned long crc_string(char *s)
//{
//  unsigned long crc = ~0L;
//  while (*s)
//    crc = crc_update(crc, *s++);
//  crc = ~crc;
//  return crc;
//}


void setup()
{
  // initialize the ethernet device
  Ethernet.begin(mac, ip, gateway, subnet);

  // start listening for clients
  server.begin();
  Serial.begin(9600);
  Serial.println("Started");
}

void loop()
{
  // if an incoming client connects, there will be bytes available to read:
  EthernetClient client = server.available();
  if (client) {
    while (client.connected()) {
      // read bytes from the incoming client and write them back
      // to any clients connected to the server:
      // server.write(client.read());

      boolean started = false;
      byte    action;
      int     address;
      int     value;
      byte    checksum;
      byte    nullterm;

      if (client.available()){
        byte input  = client.read();       
        if (input == 255 && started==false){
          started = true;
          Serial.println("Start detected"); 
        } 
        else {
          Serial.println("Bad data, exiting out"); 
          client.stop();
          break; // got data but bad 
        }


        bool timeout = true;
        for (int x=0; x<WAITTIME; x++){
          if (client.available()){
            action  = client.read();
            Serial.print(action);
            Serial.println("Got action");
            timeout = false;
            break; // got our data lets bail  
          } 
          else{
            delay(1) ;
          }
        }
        if (timeout==true){
          Serial.println("Timeout after start");
          client.stop();
          break; // no data to get. bail.
        }


        timeout = true;
        for (int x=0; x<WAITTIME; x++){
          if (client.available()){
            address  = client.read();
            Serial.print(address);
            Serial.println("Got address");
            timeout = false;
            break; // got our data lets bail  
          } 
          else{
            delay(1) ;
          }
        }
        if (timeout==true){
          Serial.println("Timeout after start");
          client.stop();
          break; // no data to get. bail.
        }


        timeout = true;
        for (int x=0; x<WAITTIME; x++){
          if (client.available()){
            value  = client.read();
            Serial.print(value);
            Serial.println("Got value");
            timeout = false;
            break; // got our data lets bail  
          } 
          else{
            delay(1) ;
          }
        }
        if (timeout==true){
          Serial.println("Timeout after start");
          client.stop();
          break; // no data to get. bail.
        }


        timeout = true;
        for (int x=0; x<WAITTIME; x++){
          if (client.available()){
            checksum  = client.read();
            Serial.print(checksum);
            Serial.println("checksum");
            timeout = false;
            break; // got our data lets bail  
          } 
          else{
            delay(1) ;
          }
        }
        if (timeout==true){
          Serial.println("Timeout after start");
          client.stop();
          break; // no data to get. bail.
        }


        timeout = true;
        for (int x=0; x<WAITTIME; x++){
          if (client.available()){
            nullterm  = client.read();
            Serial.print(nullterm);
            Serial.println("nullterminator");
            timeout = false;
            break; // got our data lets bail  
          } 
          else{
            delay(1) ;
          }
        }
        if (timeout==true){
          Serial.println("Timeout after start");
          client.stop();
          break; // no data to get. bail.
        }      

        // Checks go here

        if(nullterm != 0){
          Serial.println("Null terminator not null");
          client.stop();
          break;
        }
        //TODO MAKE THIS CRC
        if(checksum != 165){  //0b10100101
          Serial.println("Checksum failed");
          client.stop();
          break;
        }      

        switch(action){
        case 0: //keepalive
          send_packet_keepalive(client, value);
          break;
        case 1: //read digital
          break; 
        case 2: //read analog
          break; 
        case 3:  //write digital
          break;
        case 4: //read DHT
          break; 

        }
      }

    }
  }
}

void send_packet_keepalive(EthernetClient client, int value){
  byte buffer[6] = {255, //start
                  (byte) 0, //action
                  (byte) 0, //address
                   value, //value
                   165, //checksum
                    0}; //null term
  client.write(buffer,6); //start byte
//  client.write((byte) 0); //action (keepalive)
//  client.write((byte) 0); //address
//  client.write(value); //value
//  client.write(165); // checksum
}






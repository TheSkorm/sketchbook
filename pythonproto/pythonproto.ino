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
EthernetServer server(58008);

// adc - measures highest NOT average
#define ACYCLETIME 1000 // 1 second highest 
int next_a_to_check = 0;
int atable[16];
int atablelast[16];
unsigned long alastcycle;

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
  Serial.begin(115200);
  Serial.println("Started");
  //set the last cycle time
  alastcycle = millis();
}

void loop()
{
  cycle(); 

  // if an incoming client connects, there will be bytes available to read:
  EthernetClient client = server.available();
  if (client) {
    while (client.connected()) {
      // read bytes from the incoming client and write them back
      // to any clients connected to the server:
      // server.write(client.read());

      boolean started = false;
      byte    action;
      byte     address;
      byte     value;
      byte     value2;
      byte    checksum;
      byte    nullterm;

      cycle();

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


        //Get bytes from the packets

        if (nonblocking_read_byte(client, &action)){
          Serial.println("Timeout after start");
          break; // no data to get. bail.
        } 


        if (nonblocking_read_byte(client, &address)){
          Serial.println("Timeout after start");
          break; // no data to get. bail.
        } 

        if (nonblocking_read_byte(client, &value)){
          Serial.println("Timeout after start");
          break; // no data to get. bail.
        } 

        if (nonblocking_read_byte(client, &value2)){
          Serial.println("Timeout after start");
          break; // no data to get. bail.
        } 

        if (nonblocking_read_byte(client, &checksum)){
          Serial.println("Timeout after start");
          break; // no data to get. bail.
        } 



        if (nonblocking_read_byte(client, &nullterm)){
          Serial.println("Timeout after start");
          break; // no data to get. bail.
        } 

        Serial.println("Got all the bytes - checking now");


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
          send_packet_keepalive(client, value, value2);
          break;
        case 1: //read digital
          send_packet_digtal(client, address);
          break; 
        case 2: //read analog
          send_packet_analog(client,address);
          break; 
        case 3:  //write digital
          send_packet_digtal_write(client, address, (bool) value2);
          break;
        case 4: //read DHT
          break; 
        case 5: //input / output selector
          send_packet_input_output_selection(client, address, value2);
          break;

        }
      }

    }
  }
}



void send_packet_keepalive(EthernetClient client, byte value, byte value2){
  byte buffer[7] = {
    255, //start
    (byte) 0, //action
    (byte) 0, //address
    value, //value
    value2, //value
    165, //checksum
    0  }; //null term
  client.write(buffer,7); //write bytes (length must reflect correctly)
  Serial.println("Sending keepalive");
}
void send_packet_digtal(EthernetClient client, byte address){
  byte buffer[7] = {
    255, //start
    (byte) 1, //action
    (byte) address, //address
    0, //value
    digitalRead(address), //value
    165, //checksum
    0  }; //null term
  client.write(buffer,7); //write bytes (length must reflect correctly)
  Serial.println("Sending digital");
}

void send_packet_analog(EthernetClient client, byte address){
  byte buffer[7] = {
    255, //start
    (byte) 2, //action
    (byte) address, //address
    highByte(atablelast[address]), //value
    lowByte(atablelast[address]), //value
    165, //checksum
    0  }; //null term
  client.write(buffer,7); //write bytes (length must reflect correctly)
  Serial.println("Sending analog");
}

void send_packet_digtal_write(EthernetClient client, byte address, bool value){
  digitalWrite(address, value);
  byte buffer[7] = {
    255, //start
    (byte) 1, //action
    (byte) address, //address
    0, //value
    value, //value
    165, //checksum
    0  }; //null term
  client.write(buffer,7); //write bytes (length must reflect correctly)
  Serial.println("Sending digital write");
}


void send_packet_input_output_selection(EthernetClient client, byte address, byte value){
  if(value == 0){
    pinMode(address, OUTPUT);
  } 
  else if(value==1) {
    pinMode(address, INPUT);
    digitalWrite(address, LOW);
  } 
  else if(value==2) {
    pinMode(address, INPUT);
    digitalWrite(address, HIGH);
    Serial.println("Internal pullup turned on");
  }

  byte buffer[7] = {
    255, //start
    (byte) 5, //action
    (byte) address, //address
    0, //value
    value, //value
    165, //checksum
    0  }; //null term
  client.write(buffer,7); //write bytes (length must reflect correctly)
  Serial.println("Sending output / input selection");
}


boolean nonblocking_read_byte(EthernetClient client, byte *packet){
  for (int x=0; x<WAITTIME; x++){
    if (client.available()){
      *packet = client.read();
      return false;
    } 
    else{
      delay(1) ;
    }
  }
  client.stop();
  return true;
}

void check_analog(){
  int value = analogRead(next_a_to_check);
  if ( atable[next_a_to_check] < value) {
    atable[next_a_to_check] = value;
  } 
  next_a_to_check++;
  if(millis() - alastcycle > ACYCLETIME)  {
    alastcycle = millis();
    for (int i = 0; i < 16; i++) {
      atablelast[i] = atable[i];
      atable[i] = 0;
    }
  }
  if (next_a_to_check == 16){
    next_a_to_check = 0;
  }
}

void cycle(){
  //check analog and get max
  check_analog();
}


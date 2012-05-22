#include <MD5.h>
#include <MemoryFree.h>
#include "WString.h"
//#define maxLength 512

/*
This is en example of how to use my MD5 library. It provides two
easy-to-use methods, one for generating the MD5 hash, and the second
one to generate the hex encoding of the hash, which is frequently used.
*/
String PSK="thisisthepsk";
void setup()
{
  //initialize serial
  Serial.begin(9600);
  randomSeed(analogRead(0));
}

void loop()
{
  char challenge[17] ;
  delay(2000);
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
   
      Serial.print("freeMemory()=");
    Serial.println(freeMemory());
    String test(challenge);
    Serial.print("PSK=");
    Serial.println( PSK);
    Serial.print("Challenge=");
    Serial.println( test);
    test = test + PSK;
    Serial.print("Combined=");
    Serial.println( test);
    
    char tochar[30];
   for (int i=0; i <= 29; i++){ 
   tochar[i] = test.charAt(i);
   }
   unsigned char* hash=MD5::make_hash( tochar );
   char *md5str = MD5::make_digest(hash, 16);
   Serial.print("Hash=");
   Serial.println(md5str);
}

#include <WString.h>

struct dhtawesome{
int humid;
int humid_point;
int temp;
int temp_point;
};

dhtawesome CheckTemp(int DHT11_PIN);

struct challengetype{
String test;
String hash;
};

challengetype MakeChallenge();
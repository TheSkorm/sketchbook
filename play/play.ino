void setup() {
  // put your setup code here, to run once:
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly: 
 int higha;
int highb;

 for (int x; x>10; x++){
int inputa = 0;
int inputb = 0;
inputa = analogRead(0);
inputb = analogRead(1);

  
if (inputa < higha)
higha = inputa;
if (inputb < highb)
highb = inputb;

 delay(100);
  Serial.println(inputa);
  Serial.println(inputb);
  Serial.println(higha);
  Serial.println(highb);
Serial.println();

 }
  
  

Serial.println();
  analogWrite(2, higha);   
 if (higha < 500){
  digitalWrite(3, HIGH);   
 } else{
      digitalWrite(3, LOW);   
  }
  
  analogWrite(4, highb);    
  if (highb < 500){
  digitalWrite(5, HIGH); 
  } else{
      digitalWrite(5, LOW);   
  }
  
delay(500);
}

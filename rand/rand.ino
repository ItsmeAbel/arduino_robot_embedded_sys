#include <time.h>

void setup() {
  // put your setup code here, to run once:
       
}

void loop() {
  // put your main code here, to run repeatedly:
int num = (rand() % 
           (400 - (800 + 1))) + 800;
Serial.print(num); 
}

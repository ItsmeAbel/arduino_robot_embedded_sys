/*Kan sväng 90 grader åt höger och vänster*/


#include <MeAurigaLab.h>
#include "scheduler.h"
#include "semphr.h"


MeEncoderOnBoard Encoder_1(SLOT1); //Högermotor
MeEncoderOnBoard Encoder_2(SLOT2); //VänsterMotor
MeLineFollower lineFinder(PORT_9); //Line follower sensor
MeUltrasonicSensor ultraSensor(PORT_10); //Ultrasonic sensor


TaskHandle_t xHandle1 = NULL; //lineFollowerMotor
/*
TaskHandle_t xHandle2 = NULL; //turnLeft90
TaskHandle_t xHandle3 = NULL; //turnRight90
*/
TaskHandle_t xHandle4 = NULL; //senseHinderTask
/*
TaskHandle_t xHandle5 = NULL; //goStraightLong
TaskHandle_t xHandle6 = NULL; //goStraightShort
*/
TaskHandle_t xHandle7 = NULL; //goRight

char c1 = 'a';
/*
char c2 = 'b';
char c3 = 'c';
*/
char c4 = 'd';
/*
char c5 = 'e';
char c6 = 'f';
*/
char c7 = 'g';

SemaphoreHandle_t xMutex1;  //isHinder
SemaphoreHandle_t xMutex2;  //dirLandRAid
SemaphoreHandle_t xMutex3;   //followLine

SemaphoreHandle_t xMutex4;   


#define ULTRASONICSENSORDISTANCE 15
#define NOHINDER 1
#define HINDER 0
#define GORIGHT 1
#define GOLEFT -1
#define GOSTRAIGHTLONG 2
#define GOSTRAIGHTSHORT 3
#define GOAROUND 3
#define SPEED 125

//Task turnRight90
#define NINTYRIGHT 350

//Task turnLeft90
#define NINTYLEFT -450

//Task goStraight
#define GOSTRAIGHTDISTANCELONG 900
#define GOSTRAIGHTDISTANCESHORT 450

int isHinder = 1; //Koll om det finns ett hinder, Mutex1
int dirLandRAid = 0; //Koll vilken riktning raider är, Mutex2

int followLine = 1;

//Task goStraight
int distance = 0;

int rightCurPos = 0;
int leftCurPos = 0;






void lineFollowerMotor(void *pv)
{
  int sensorState = lineFinder.readSensors();
  if( (isHinder == NOHINDER) &&  followLine )
  {
  switch(sensorState)
  {
    case S1_IN_S2_IN: 
      Serial.println("Sensor 1 and 2 are inside of black line"); 
      Encoder_1.setMotorPwm(-130);
      Encoder_2.setMotorPwm(130); // Sätts bara i början?
   
      break;
    case S1_IN_S2_OUT: 
      Serial.println("Sensor 2 is outside of black line"); 
      Encoder_1.setMotorPwm(-130);
      Encoder_2.setMotorPwm(0);
    
      break;
    case S1_OUT_S2_IN: 
      Serial.println("Sensor 1 is outside of black line");
      Encoder_1.setMotorPwm(0);
      Encoder_2.setMotorPwm(130);  
    
      break;
    case S1_OUT_S2_OUT: 
      Serial.println("Sensor 1 and 2 are outside of black line");
      Encoder_1.setMotorPwm(0);
      Encoder_2.setMotorPwm(0); 
     
      break;
    default: 
      break;
  }
  Encoder_1.updateSpeed(); 
  Encoder_2.updateSpeed(); 
  //delay(200);
  }
  
}

/* 
 *  senseHinderTask känner av hinder och sätter variabel 
 *  isHinder till HINDER = 0, eller NOHINDER = 1;
 *  Global variabel ULTRASONICSENSORDISTANCE bestämmer avstånd till hinder.
 *  
 */
void senseHinderTask(void *pv)
{
  Serial.print("ultrasonicSensorTask: ");
  Serial.println(ultraSensor.distanceCm() );
  delay(100);
  
  //HINDER
  if( (ultraSensor.distanceCm() < ULTRASONICSENSORDISTANCE) )
  { 
    Serial.println("HINDER!" );
     
    xSemaphoreTake( xMutex1, portMAX_DELAY );
    {
      isHinder = HINDER;
       Serial.println(isHinder );
    }
    xSemaphoreGive( xMutex1 );

    xSemaphoreTake( xMutex2, portMAX_DELAY );
    {
       //dirLandRAid = GORIGHT;
       //dirLandRAid = GOLEFT;
       //dirLandRAid = GOSTRAIGHTLONG;
       //dirLandRAid = GOSTRAIGHTSHORT;
       dirLandRAid = GOAROUND;
    }
    xSemaphoreGive( xMutex2 );

    xSemaphoreTake( xMutex3, portMAX_DELAY );
    {
       followLine = 0;
    }
    xSemaphoreGive( xMutex3 );
  }
    //NOHINDER
  else if(ultraSensor.distanceCm() > ULTRASONICSENSORDISTANCE )
  {
    Serial.println("NOHINDER!" );

    xSemaphoreTake( xMutex1, portMAX_DELAY );
    {
      isHinder = NOHINDER;
       Serial.println(isHinder );
    }
    xSemaphoreGive( xMutex1 );
 
  }
}

void goStraight(int dist)
{
    Serial.print("Gå rakt fram  ==============");
    Serial.println(" ");
    Encoder_2.updateCurPos();
    
    int currentPosition = Encoder_2.getCurPos();
    distance = currentPosition;
    
    Serial.print("currentPosition: ");
    Serial.print(currentPosition );
    Serial.println(" ");
  
    Serial.print("distance: ");
    Serial.print(distance );
    Serial.println(" ");

      while( currentPosition - distance < dist )
      {
        Encoder_1.setMotorPwm(-125);
        Encoder_2.setMotorPwm(125);
        Encoder_1.updateSpeed(); 
        Encoder_2.updateSpeed();

        Encoder_2.updateCurPos();
        currentPosition = Encoder_2.getCurPos();

          Serial.print("currentPosition: ");
    Serial.print(currentPosition );
    Serial.println(" ");
  
    Serial.print("distance: ");
    Serial.print(distance );
    Serial.println(" ");
        delay(100);
     }

      Encoder_1.setMotorPwm(0);
      Encoder_2.setMotorPwm(0);
      Encoder_1.updateSpeed(); 
      Encoder_2.updateSpeed();
        
    //distance = 0;
    
    xSemaphoreTake( xMutex2, portMAX_DELAY );
    {
       dirLandRAid = 0;
    }
    xSemaphoreGive( xMutex2 );  
}

void goStraightLong(/*void *pv*/)
{
 // if(dirLandRAid == GOSTRAIGHTLONG)
 // {
    goStraight( GOSTRAIGHTDISTANCELONG );
 // }
}

void goStraightShort(/*void *pv*/)
{
  //if(dirLandRAid == GOSTRAIGHTSHORT)
  //{
    goStraight( GOSTRAIGHTDISTANCESHORT );
  //}
 
}


void turnLeft90(/*void *pv*/)
{
  //if(dirLandRAid == GOLEFT)
  //{
    Serial.print("Sväng vänster ==============");
    Serial.println(" ");
    
    Encoder_1.updateCurPos();
    
    int currentRightPosition = Encoder_1.getCurPos();
    rightCurPos = currentRightPosition;
    
    Serial.print("currentRightPosition: ");
    Serial.print(currentRightPosition );
    Serial.println(" ");
    Serial.print("rightCurPos: ");
    Serial.print(rightCurPos );
    Serial.println(" ");

    while( currentRightPosition - rightCurPos > NINTYLEFT )
    {
      Encoder_1.setMotorPwm(-125);
      Encoder_2.setMotorPwm(-125);
      Encoder_1.updateSpeed(); 
      Encoder_2.updateSpeed();

      Encoder_1.updateCurPos();
      currentRightPosition = Encoder_1.getCurPos();

        Serial.print("currentRightPosition: ");
    Serial.print(currentRightPosition );
    Serial.println(" ");
    Serial.print("rightCurPos: ");
    Serial.print(rightCurPos );
    Serial.println(" ");
        delay(100);
     }

     Encoder_1.setMotorPwm(0);
     Encoder_2.setMotorPwm(0);
     Encoder_1.updateSpeed(); 
     Encoder_2.updateSpeed();

     rightCurPos = 0;
    leftCurPos = 0;
     
     xSemaphoreTake( xMutex2, portMAX_DELAY );
    {
       dirLandRAid = 0;
    }
    xSemaphoreGive( xMutex2 );
  //}
  
}


void turnRight90(/*void *pv*/)
{  
  //if(dirLandRAid == GORIGHT)
  //{
   Serial.print("Sväng höger ==============");
     Serial.println(" ");
    Encoder_2.updateCurPos();
    
    int currentLeftPosition = Encoder_2.getCurPos();
    leftCurPos = currentLeftPosition;
    
    Serial.print("currentLeftPosition: ");
    Serial.print(currentLeftPosition );
    Serial.println(" ");
  
    Serial.print("leftCurPos: ");
    Serial.print(leftCurPos );
    Serial.println(" ");

      while( currentLeftPosition - leftCurPos < NINTYRIGHT )
      {
        Encoder_1.setMotorPwm(125);
        Encoder_2.setMotorPwm(125);
        Encoder_1.updateSpeed(); 
        Encoder_2.updateSpeed();

        Encoder_2.updateCurPos();
        currentLeftPosition = Encoder_2.getCurPos();
        delay(100);
     }

      Encoder_1.setMotorPwm(0);
        Encoder_2.setMotorPwm(0);
        Encoder_1.updateSpeed(); 
        Encoder_2.updateSpeed();
        
    rightCurPos = 0;
    leftCurPos = 0;
    
    xSemaphoreTake( xMutex2, portMAX_DELAY );
    {
       dirLandRAid = 0;
    }
    xSemaphoreGive( xMutex2 );
  //}
}

int turnRight = 1;

void goRight(void *pv)
{
   
  if( dirLandRAid == GOAROUND )
  {
    if( isHinder == HINDER ) //Forsätter åt höger sålänge ett hinder är ivägen.                       
    {                        // så länge dirLandRAid = GOAROUND och isHinder == HINDER gå åt höger.
      if( turnRight)
      {
        turnRight90();
        goStraightShort();
        turnLeft90();
        Serial.println("Turn right, HINDER---------");
       // turnRight = 0;
      }else if( !turnRight )
      {
        turnRight90();
        goStraightShort();
        turnLeft90();
        Serial.println("Turn left, HINDER--------------");
      }
     
    }
    else if( isHinder == NOHINDER )//Inget hinder 
    {

      if( turnRight == 1 ) //Går inte in här
      {
        
        goStraightShort();
        turnLeft90();
        turnRight = 0;
        Serial.println("Turn left, NOHINDER");
      }else if( turnRight == 0 )
      {
        goStraightShort();
        Serial.println("Go straight to line, NOHINDER------------");
        
        Encoder_1.setMotorPwm(0);
        Encoder_2.setMotorPwm(0);
        Encoder_1.updateSpeed(); 
        Encoder_2.updateSpeed();
        xSemaphoreTake( xMutex2, portMAX_DELAY );
        {
          dirLandRAid = 0;
        }
        xSemaphoreGive( xMutex2 );
        turnRight = 0;
     }
    }
  }
}



void isr_process_encoder1(void)
{
  if(digitalRead(Encoder_1.getPortB()) == 0)
  { 
    Encoder_1.pulsePosMinus();
  } 
  else 
  {
  Encoder_1.pulsePosPlus();; 
  }
}

void isr_process_encoder2(void)
{
  if(digitalRead(Encoder_2.getPortB()) == 0)
  { 
    Encoder_2.pulsePosMinus();
  } 
  else
  { 
    Encoder_2.pulsePosPlus();
  } 
}

void setup()
{
  attachInterrupt(Encoder_1.getIntNum(), isr_process_encoder1, RISING); 
  attachInterrupt(Encoder_2.getIntNum(), isr_process_encoder2, RISING);
  Serial.begin(9600);
  //Serial.begin(115200);

  TCCR1A = _BV(WGM10);
  TCCR1B = _BV(CS11) | _BV(WGM12);
  
  TCCR2A = _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(CS21);


  xMutex1 = xSemaphoreCreateMutex();
  xMutex2 = xSemaphoreCreateMutex();
  xMutex3 = xSemaphoreCreateMutex();
  xMutex4 = xSemaphoreCreateMutex();

     if( (xMutex1 != NULL) && (xMutex2 != NULL) && (xMutex3 != NULL) && (xMutex3 != NULL) )//om semaphore lyckades skapas
    { 
  vSchedulerInit(); 

  //Lägre prioritet
  vSchedulerPeriodicTaskCreate(lineFollowerMotor, "t1", configMINIMAL_STACK_SIZE,
  &c1, 1, &xHandle1, pdMS_TO_TICKS(0), 
  pdMS_TO_TICKS(100), pdMS_TO_TICKS(100), pdMS_TO_TICKS(100));

/*
  vSchedulerPeriodicTaskCreate(turnLeft90, "t2", configMINIMAL_STACK_SIZE,
  &c2, 20, &xHandle2, pdMS_TO_TICKS(0), 
  pdMS_TO_TICKS(100), pdMS_TO_TICKS(100), pdMS_TO_TICKS(100));

  vSchedulerPeriodicTaskCreate(turnRight90, "t3", configMINIMAL_STACK_SIZE,
  &c3, 40, &xHandle3, pdMS_TO_TICKS(0), 
  pdMS_TO_TICKS(100), pdMS_TO_TICKS(100), pdMS_TO_TICKS(100));
*/
  vSchedulerPeriodicTaskCreate(senseHinderTask, "t4", configMINIMAL_STACK_SIZE,
  &c4, 20, &xHandle4, pdMS_TO_TICKS(0), 
  pdMS_TO_TICKS(100), pdMS_TO_TICKS(100), pdMS_TO_TICKS(100));

/*
   vSchedulerPeriodicTaskCreate(goStraightLong, "t5", configMINIMAL_STACK_SIZE,
  &c5, 40, &xHandle5, pdMS_TO_TICKS(0), 
  pdMS_TO_TICKS(100), pdMS_TO_TICKS(100), pdMS_TO_TICKS(100));

  vSchedulerPeriodicTaskCreate(goStraightShort, "t6", configMINIMAL_STACK_SIZE,
  &c6, 40, &xHandle6, pdMS_TO_TICKS(0), 
  pdMS_TO_TICKS(100), pdMS_TO_TICKS(100), pdMS_TO_TICKS(100));
*/
  vSchedulerPeriodicTaskCreate(goRight, "t7", configMINIMAL_STACK_SIZE,
  &c7, 10, &xHandle7, pdMS_TO_TICKS(0), 
  pdMS_TO_TICKS(100), pdMS_TO_TICKS(100), pdMS_TO_TICKS(100));
  

  vSchedulerStart();
    }
}


void loop() {
  // put your main code here, to run repeatedly:
}

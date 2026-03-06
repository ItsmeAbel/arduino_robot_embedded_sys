#include <MeAurigaLab.h>
#include "scheduler.h"
#include "semphr.h"

MeEncoderOnBoard Encoder_1(SLOT1); //Högermotor
MeEncoderOnBoard Encoder_2(SLOT2); //VänsterMotor
MeLineFollower lineFinder(PORT_9); //Line follower sensor
MeUltrasonicSensor ultraSensor(PORT_10); //Ultrasonic sensor

#define ULTRASONICSENSORDISTANCE 15
#define NOHINDER 1
#define HINDER 0
#define GOAROUND 3
#define NINTYRIGHT 350 /*380*/      //TurnRight
#define NINTYLEFT -400              //TurnLeft
#define GOSTRAIGHTDISTANCELONG 900  //Task goStraight
#define GOSTRAIGHTDISTANCESHORT 300 //Task goStraight

TaskHandle_t xHandle1 = NULL; //lineFollowerMotor
TaskHandle_t xHandle2 = NULL; //findLine
TaskHandle_t xHandle3 = NULL; //senseSensorState
TaskHandle_t xHandle4 = NULL; //senseHinderTask
TaskHandle_t xHandle5 = NULL; //turnLine
TaskHandle_t xHandle7 = NULL; //goAround går åt höger sålänge ett hinder är i vägen när den går runt.

char c1 = 'a';
char c2 = 'b';
char c3 = 'c';
char c4 = 'd';
char c5 = 'e';
char c7 = 'g';

SemaphoreHandle_t xMutex1;  //isHinder
SemaphoreHandle_t xMutex2;  //dirLandRAid
SemaphoreHandle_t xMutex3;  //followLine
SemaphoreHandle_t xMutex4;  //sensorOnLine
SemaphoreHandle_t xMutex5;  //findLineAgain
SemaphoreHandle_t xMutex6;  //start

int isHinder = 1;     //Koll om det finns ett hinder, Mutex1
int dirLandRAid = 0;  //Koll Mutex2
                      // 0 idle  
                      // 1 findLine
                      // 3 goAround
int followLine = 1;   //Följ linje, Mutex3
int sensorOnLine = 1; //Koll om sensorer är på linje, Mutex4
                      // 0 båda utanför linje
                      // 1 båda innanför linje
                      // 2 höger utanför linje
                      // 3 Vänster utanför linje
int findLineAgain = 0; //Hitta linje igen, Mutex5
int start = 1;        //Startvariabel tills linje är hittad, Mutex6


/* 
 *  lineFollowerMotor följer linje
 */
void lineFollowerMotor(void *pv)
{
  if( isHinder == NOHINDER && (sensorOnLine != 0) && (findLineAgain == 0) && (start == 0 ) && (followLine == 1) ) 
  {
   // Serial.print("inne i lineFollowerMotor ==============");
   // Serial.println(" ");
    switch( sensorOnLine )
    {
      case 1: 
        Encoder_1.setMotorPwm(-100);
        Encoder_2.setMotorPwm(100);
        break;
      
      case 2: 
        Encoder_1.setMotorPwm(-100);
        Encoder_2.setMotorPwm(0);
        break;
      
      case 3: 
        Encoder_1.setMotorPwm(0);
        Encoder_2.setMotorPwm(100);
        break;
      
     default: 
        break;
    }
   Encoder_1.updateSpeed(); 
   Encoder_2.updateSpeed(); 
  //delay(200);
  }
}
//===============================================================================

/* 
 *  senseHinderTask känner av hinder och sätter variabel 
 *  isHinder till HINDER = 0, eller NOHINDER = 1;
 *  Global variabel ULTRASONICSENSORDISTANCE bestämmer avstånd till hinder.
 */
void senseHinderTask(void *pv)
{
  Serial.print("ultrasonicSensorTask: ");
  Serial.println(ultraSensor.distanceCm() );
  delay(100);

  //HINDER
  if( (ultraSensor.distanceCm() <= ULTRASONICSENSORDISTANCE) )
  { 
    Serial.println("HINDER!" );
     
    xSemaphoreTake( xMutex1, portMAX_DELAY );
    {
      isHinder = HINDER;
      Serial.println(isHinder );
    }
    xSemaphoreGive( xMutex1 );

    if( dirLandRAid == 0 && findLineAgain == 0 )
    {
      xSemaphoreTake( xMutex3, portMAX_DELAY );
      {
       followLine = 0;
      }
      xSemaphoreGive( xMutex3 );

      xSemaphoreTake( xMutex2, portMAX_DELAY );
      {
        dirLandRAid = GOAROUND;
      }
      xSemaphoreGive( xMutex2 ); 
    }

    
    
  }
    //NOHINDER
  else if(ultraSensor.distanceCm() > ULTRASONICSENSORDISTANCE )
  {
    xSemaphoreTake( xMutex1, portMAX_DELAY );
    {
      isHinder = NOHINDER;
      Serial.println(isHinder );
    }
    xSemaphoreGive( xMutex1 );
  }
}
//===============================================================================

/* 
 *  senseSensorState kollar sensor1 och sensor2
 *  sensorOnLine = 0 båda utanför linje
 *  sensorOnLine = 1 båda innanför linje
 *  sensorOnLine = 2 höger (sensor2) utanför linje 
 *  sensorOnLine = 3 Vänster (sensor1) utanför linje
 */
void senseSensorState(void *pv)
{
  int sensorState = lineFinder.readSensors();
  switch(sensorState)
  {
    case S1_IN_S2_IN: 
      Serial.println("Sensor 1 and 2 are inside of black line"); 
      xSemaphoreTake( xMutex4, portMAX_DELAY );
      {
        sensorOnLine = 1; 
      }
      xSemaphoreGive( xMutex4 );
      break;
      
    case S1_IN_S2_OUT: 
      Serial.println("Sensor 2 is outside of black line");  
      xSemaphoreTake( xMutex4, portMAX_DELAY );
      {
        sensorOnLine = 2; 
      }
      xSemaphoreGive( xMutex4 );
      break;
      
    case S1_OUT_S2_IN: 
      Serial.println("Sensor 1 is outside of black line");
      xSemaphoreTake( xMutex4, portMAX_DELAY );
      {
        sensorOnLine = 3; 
      }
      xSemaphoreGive( xMutex4 );
      break;
      
    case S1_OUT_S2_OUT: 
      Serial.println("Sensor 1 and 2 are outside of black line");
      xSemaphoreTake( xMutex4, portMAX_DELAY );
      {
        sensorOnLine = 0; 
      }
      xSemaphoreGive( xMutex4 );
      break;
    default: 
      break;
  }
}
//===============================================================================

void turnRightDist(int dist)
{ 
    Encoder_2.updateCurPos();
    
    int currentLeftPosition = Encoder_2.getCurPos();
    int leftCurPos = currentLeftPosition;

    while( currentLeftPosition - leftCurPos < dist )
    {
        Encoder_1.setMotorPwm(100);
        Encoder_2.setMotorPwm(100);
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
}
//===============================================================================

void turnLeftDist(int dist)
{
    Encoder_1.updateCurPos();
    
    int currentRightPosition = Encoder_1.getCurPos();
    int rightCurPos = currentRightPosition;

    while( currentRightPosition - rightCurPos > dist )
    {
      Encoder_1.setMotorPwm(-125);
      Encoder_2.setMotorPwm(-125);
      Encoder_1.updateSpeed(); 
      Encoder_2.updateSpeed();

      Encoder_1.updateCurPos();
      currentRightPosition = Encoder_1.getCurPos();
      delay(100);
    }

    Encoder_1.setMotorPwm(0);
    Encoder_2.setMotorPwm(0);
    Encoder_1.updateSpeed(); 
    Encoder_2.updateSpeed();
}
//===============================================================================


void goStraight(int dist)
{
  Encoder_2.updateCurPos();
    
  int currentPosition = Encoder_2.getCurPos();
  int distance = currentPosition;
 
  while( currentPosition - distance < dist )
  {
    Encoder_1.setMotorPwm(-125);
    Encoder_2.setMotorPwm(125);
    Encoder_1.updateSpeed(); 
    Encoder_2.updateSpeed();

    Encoder_2.updateCurPos();
    currentPosition = Encoder_2.getCurPos();
    delay(100);
  }

  Encoder_1.setMotorPwm(0);
  Encoder_2.setMotorPwm(0);
  Encoder_1.updateSpeed(); 
  Encoder_2.updateSpeed();
}
//===============================================================================
int turnRight = 1;

void goAround(void *pv)
{
  if( dirLandRAid == GOAROUND )
  {
    if( isHinder == HINDER ) //Så länge dirLandRAid = GOAROUND och isHinder == HINDER gå åt höger.                      
    {                        
      turnRightDist(NINTYRIGHT);
      goStraight(GOSTRAIGHTDISTANCESHORT);
      turnLeftDist(NINTYLEFT);
      Serial.println("dirLandRAid = GOAROUND och isHinder = HINDER---------");
    }
    else if( isHinder == NOHINDER )//Inget hinder 
    {
      if( turnRight == 1 ) 
      {
        Serial.println("dirLandRAid = GOAROUND och isHinder = NOHINDER---------");
        goStraight(600);
        turnLeftDist(NINTYLEFT);
        turnRight = 0;
      }else if( turnRight == 0 )
      {
        //goStraight(GOSTRAIGHTDISTANCESHORT);
        Serial.println("Find line again, NOHINDER------------");
        Encoder_1.setMotorPwm(0);
        Encoder_2.setMotorPwm(0);
        Encoder_1.updateSpeed(); 
        Encoder_2.updateSpeed();

        turnRight = 1;

        //Hoppa in i findLine
        xSemaphoreTake( xMutex2, portMAX_DELAY );
        {
          dirLandRAid = 1;
        }
        xSemaphoreGive( xMutex2 );

        xSemaphoreTake( xMutex6, portMAX_DELAY );
        {
          start = 1;
        }
        xSemaphoreGive( xMutex6 );
     }
    }
  }
}

//===============================================================================

void findLine(void *pv)
{    
  if( sensorOnLine == 0  && (start == 1) ) //Om båda sensorerna är utanför linje, VID START.
  {
     xSemaphoreTake( xMutex5, portMAX_DELAY );
     {
        findLineAgain = 1; //Stoppar lineFollowerMotor task.
        Serial.print("findLineAgain = 1" );
        Serial.println(" ");
     }
     xSemaphoreGive( xMutex5 );

    if( isHinder == HINDER )
    {
      Serial.println("Hinder i findLine vänd 90 grader ");
      turnRightDist(525);
    }
    
    Encoder_1.setMotorPwm(-100);
    Encoder_2.setMotorPwm(100);
    Encoder_1.updateSpeed(); 
    Encoder_2.updateSpeed(); 
  }
  if( sensorOnLine == 1 && start == 1 && findLineAgain == 1 )
  {
      xSemaphoreTake( xMutex6, portMAX_DELAY );
     {
        start = 0;
     }
     xSemaphoreGive( xMutex6 );
    
  }/*
  if( ( sensorOnLine == 1 || sensorOnLine == 2 || sensorOnLine == 3 ) && start == 1 && findLineAgain == 0 )
  {
    xSemaphoreTake( xMutex6, portMAX_DELAY );
     {
        start = 0;
     }
     xSemaphoreGive( xMutex6 );

  }*/
}
//===============================================================================


void turnLine(void *pv)
{
   if( sensorOnLine == 1 && findLineAgain == 1 && start == 0 )
   {
    while( sensorOnLine != 2 &&  sensorOnLine != 3)
    {
        Encoder_1.setMotorPwm(100);
        Encoder_2.setMotorPwm(100);
        Encoder_1.updateSpeed(); 
        Encoder_2.updateSpeed();
        Serial.print("While loop vänd");
        Serial.println(" ");
    }
    xSemaphoreTake( xMutex5, portMAX_DELAY );
    {
      findLineAgain = 0; 
    }
    xSemaphoreGive( xMutex5 );

    xSemaphoreTake( xMutex3, portMAX_DELAY );
    {
      followLine = 1;
    }
    xSemaphoreGive( xMutex3 );

       xSemaphoreTake( xMutex2, portMAX_DELAY );
      {
        dirLandRAid = 0;
      }
      xSemaphoreGive( xMutex2 );
   }
}
//===============================================================================
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

//===============================================================================

void setup()
{
  attachInterrupt(Encoder_1.getIntNum(), isr_process_encoder1, RISING); 
  attachInterrupt(Encoder_2.getIntNum(), isr_process_encoder2, RISING);
  Serial.begin(9600);

  TCCR1A = _BV(WGM10);
  TCCR1B = _BV(CS11) | _BV(WGM12);
  
  TCCR2A = _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(CS21);


  xMutex1 = xSemaphoreCreateMutex();
  xMutex2 = xSemaphoreCreateMutex();
  xMutex3 = xSemaphoreCreateMutex();
  xMutex4 = xSemaphoreCreateMutex();
  xMutex5 = xSemaphoreCreateMutex();
  xMutex6 = xSemaphoreCreateMutex();

  if( (xMutex1 != NULL) && (xMutex2 != NULL) && (xMutex3 != NULL) && (xMutex4 != NULL) && (xMutex5 != NULL)&& (xMutex6 != NULL) )//om semaphore lyckades skapas
  { 
    vSchedulerInit(); 

    vSchedulerPeriodicTaskCreate(lineFollowerMotor, "t1", configMINIMAL_STACK_SIZE,
    &c1, 1, &xHandle1, pdMS_TO_TICKS(0), 
    pdMS_TO_TICKS(100), pdMS_TO_TICKS(100), pdMS_TO_TICKS(100));

    vSchedulerPeriodicTaskCreate(findLine, "t2", configMINIMAL_STACK_SIZE,
    &c2, 1, &xHandle2, pdMS_TO_TICKS(0), 
    pdMS_TO_TICKS(100), pdMS_TO_TICKS(100), pdMS_TO_TICKS(100));

    vSchedulerPeriodicTaskCreate(senseSensorState, "t3", configMINIMAL_STACK_SIZE,
    &c3, 40, &xHandle3, pdMS_TO_TICKS(0), 
    pdMS_TO_TICKS(100), pdMS_TO_TICKS(100), pdMS_TO_TICKS(100));

    vSchedulerPeriodicTaskCreate(senseHinderTask, "t4", configMINIMAL_STACK_SIZE,
    &c4, 40, &xHandle4, pdMS_TO_TICKS(0), 
    pdMS_TO_TICKS(100), pdMS_TO_TICKS(100), pdMS_TO_TICKS(100));

    vSchedulerPeriodicTaskCreate(turnLine, "t5", configMINIMAL_STACK_SIZE,
    &c5, 20, &xHandle5, pdMS_TO_TICKS(0), 
    pdMS_TO_TICKS(100), pdMS_TO_TICKS(100), pdMS_TO_TICKS(100));

    vSchedulerPeriodicTaskCreate(goAround, "t7", configMINIMAL_STACK_SIZE,
    &c7, 10, &xHandle7, pdMS_TO_TICKS(0), 
    pdMS_TO_TICKS(100), pdMS_TO_TICKS(100), pdMS_TO_TICKS(100));

    vSchedulerStart();
   }
}


void loop() {
  // put your main code here, to run repeatedly:
}

#include <Sparki.h>
#include "pitches.h"

double xOffset = 0;
double yOffset = 0;
int thetaOffset = 0;

int gripperInc = 1500;

bool inKitchen = false;

void openGrip()
{
  sparki.gripperOpen();
  delay(gripperInc);
  sparki.gripperStop();
}

void closeGrip()
{
  sparki.gripperClose();
  delay(gripperInc);
  sparki.gripperStop();
}

void turn_rec(int angle)
{
  if(angle > 0)
    sparki.moveLeft(angle);
   else
    sparki.moveRight(-angle);
    
  thetaOffset += angle;

  if(thetaOffset < 0)
    thetaOffset = (-1)*((-thetaOffset) % 360);
  else
    thetaOffset = thetaOffset % 360;
}

void move_fwd_rec(int delayTime)
{
  sparki.moveForward();
  delay(delayTime);
  sparki.moveStop();
  
  xOffset += delayTime * cos(M_PI * thetaOffset/180);
  yOffset += delayTime * sin(M_PI * thetaOffset/180);
}

void move_bkwd_rec(int delayTime)
{
  sparki.moveBackward();
  delay(delayTime);
  sparki.moveStop();
  
  xOffset -= delayTime * cos(M_PI * thetaOffset/180);
  yOffset -= delayTime * sin(M_PI * thetaOffset/180);
}

void grab_item()
{
  sparki.beep(NOTE_C4, 1000/4);
  closeGrip();
}

void drop_item()
{
  sparki.beep(NOTE_G3, 1000/4);
  openGrip();
}

void return_to_start()
{
  //return to initial x offset
  turn_rec(-thetaOffset);
    
  if(xOffset > 0)
    move_bkwd_rec(xOffset);
  else
    move_fwd_rec(-xOffset);

  turn_rec(90);
  
  if(yOffset > 0)
    move_bkwd_rec(yOffset);
  else
    move_fwd_rec(-yOffset);

  turn_rec(-90);
  
  xOffset = 0;
  yOffset = 0;
  thetaOffset = 0;
}

void move_to_table()
{
  int turnInc = 10;
  int moveInc = 500;
  int minDist = 5;

  turn_rec(180);

  int left, ctr, right;
  while(sparki.ping() > minDist)
  {
    left = sparki.lightLeft();
    ctr = sparki.lightCenter();
    right = sparki.lightRight();

    if(left > ctr && left > right)
    {
      turn_rec(turnInc);
    }
    else if(right > ctr && right > left)
    {
      turn_rec(-turnInc);
    }
    else
    {
      move_fwd_rec(moveInc);
    }
  }
  
  inKitchen = false;
}

void find_kitchen()
{
  Serial.println("finding kitchen");
  int turnInc = 2;
  int moveInc = 300;
  int threshhold = 300;
  
  bool onLine = false;
  int eLeft, left, ctr, right, eRight;
  while(!inKitchen)
  {
    eLeft = sparki.edgeLeft();
    left = sparki.lineLeft();
    ctr = sparki.lineCenter();
    right = sparki.lineRight();
    eRight = sparki.edgeRight();

    if(left <= threshhold && ctr > threshhold && right > threshhold)
    {
      //turn left to stay on line
      turn_rec(turnInc);
    }
    else if(left > threshhold && ctr > threshhold && right <= threshhold)
    {
      //turn right to stay on line
      turn_rec(-turnInc);
    }
    else if(left <= threshhold && ctr <= threshhold && right <= threshhold)
    {
      if(onLine)
      {
        //found kitchen
        inKitchen = true;
      } 
      else 
      {
        Serial.println("found line!");
        //found line, getting on it
        while(sparki.lineCenter() <= threshhold) 
        {
          move_fwd_rec(moveInc * 2);
        }
          
        while(sparki.lineCenter() > threshhold)
        {
          turn_rec(turnInc);
        }
          
        onLine = true;
      }
    }
    else if(eLeft > threshhold && left > threshhold
      && ctr > threshhold && right > threshhold
      && eRight > threshhold)
    {
      if(onLine)
      {
        //went off line end, turn around
        while(sparki.lineCenter() > threshhold)
        {
          turn_rec(turnInc);
        }
          
      }
      else
      {
        //finding line
        move_fwd_rec(moveInc);
      }
    }
    else
    {
      //on line and moving forward
      move_fwd_rec(moveInc);
    }
  }
}

void handling_order()
{
  Serial.println("handling order");

  find_kitchen();

  grab_item();

  move_to_table();

  drop_item();

  turn_rec(180);
}

void shift_over()
{
  Serial.println("shift over");
  closeGrip();
  return_to_start();
}

void setup()
{
  sparki.RGB(RGB_RED);
  sparki.servo(SERVO_CENTER);
  openGrip();
}

void loop()
{
  //check for orders to handle, add them to queue here
  //check for shift over command

  int orderCode = sparki.readIR();

  switch(orderCode)
  {
    case 70:
      sparki.RGB(RGB_RED);
      handling_order();
      break;
    case 64:
      sparki.RGB(RGB_RED);
      shift_over();
      break;
    case 7:
      sparki.RGB(RGB_RED);
      closeGrip();
      break;
    case 9:
      sparki.RGB(RGB_RED);
      openGrip();
      break;
    default:
      sparki.RGB(RGB_GREEN);
      break;
  }

  delay(1000);
}


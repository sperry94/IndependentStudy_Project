#include <Sparki.h>
#include "pitches.h"

struct loc_offset {
  double x, y;
};

struct full_offset {
  loc_offset loc;
  short theta;
};

//config vars
short turnInc = 2;
short moveInc = 300;

//state vars
full_offset startOffset;
full_offset edgeOffsets[4];
short edgesFound = 0;

void turn_rec(short angle)
{
  //don't need to do a full spin
  if(angle < 0)
    angle = (-1)*((-angle) % 360);
  else
    angle = angle % 360;

  //turn
  if(angle > 0)
    sparki.moveLeft(angle);
   else
    sparki.moveRight(-angle);
    
  //record
  startOffset.theta += angle;
  
  //keep thetaOffset between +-360
  if(startOffset.theta < 0)
    startOffset.theta = (-1)*((-startOffset.theta) % 360);
  else
    startOffset.theta = startOffset.theta % 360;
}

void move_fwd_rec(int delayTime)
{
  sparki.moveForward();
  delay(delayTime);
  sparki.moveStop();
  
  startOffset.loc.x += delayTime * cos(M_PI * startOffset.theta/180);
  startOffset.loc.y += delayTime * sin(M_PI * startOffset.theta/180);
}

void move_bkwd_rec(int delayTime)
{
  sparki.moveBackward();
  delay(delayTime);
  sparki.moveStop();
  
  startOffset.loc.x -= delayTime * cos(M_PI * startOffset.theta/180);
  startOffset.loc.y -= delayTime * sin(M_PI * startOffset.theta/180);
}

void return_to_start_x()
{
  //return to initial x offset
  turn_rec(-startOffset.theta);
    
  if(startOffset.loc.x > 0)
    move_bkwd_rec(startOffset.loc.x);
  else
    move_fwd_rec(-startOffset.loc.x);
    
  startOffset.loc.x = 0;
}

void return_to_start_y()
{
  
  if(startOffset.loc.y > 0)
    move_bkwd_rec(startOffset.loc.y);
  else
    move_fwd_rec(-startOffset.loc.y);
  
  startOffset.loc.y = 0;
}

void return_to_start()
{
  return_to_start_x();

  turn_rec(90);

  return_to_start_y();

  turn_rec(-90);
  
  startOffset.theta = 0;
}

full_offset proceed_to_edge()
{
  short minIR = 100;
  
  short startingEdges = edgesFound;
  bool eLeftOver, ctrOver, eRightOver;
  while(edgesFound == startingEdges)
  {
    eLeftOver = sparki.edgeLeft() <= minIR;
    ctrOver = sparki.lineCenter() <= minIR;
    eRightOver = sparki.edgeRight() <= minIR;

    if(eLeftOver && ctrOver && eRightOver)
      edgesFound++;
    else if(eLeftOver)
      turn_rec(turnInc);
    else if(eRightOver)
      turn_rec(-turnInc);
    else
      move_fwd_rec(moveInc);
  }

  edgeOffsets[edgesFound-1] = { { startOffset.loc.x, startOffset.loc.y }, startOffset.theta };
  
  sparki.beep(NOTE_C4, 1000/4);
  move_bkwd_rec(moveInc*4);
}

void find_first_edge()
{
  proceed_to_edge();
}

void find_next_edge(full_offset previousEdge)
{
  return_to_start();
  turn_rec(previousEdge.theta + 90);
  proceed_to_edge();
}

void setup() {}

void loop()
{
  if(edgesFound == 0)
    find_first_edge();
  else if(edgesFound < 4)
    find_next_edge(edgeOffsets[edgesFound-1]);
  else
    return_to_start();

  delay(1000);
}

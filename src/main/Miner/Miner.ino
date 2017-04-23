#include <Sparki.h>
#include <StackArray_Gettable.h>
#include "pitches.h"

struct loc_offset {
  double x, y;
};

struct full_offset {
  loc_offset loc;
  short theta;
};

//state machine variables
const short FINDING_MINE = 0;
const short TRAVERSING_MINE = FINDING_MINE + 1;
const short FINDING_CHARGE = TRAVERSING_MINE + 1;
const short RETURNING_HOME = FINDING_CHARGE + 1;
const short COMPLETE = RETURNING_HOME + 1;

//turn variables
const short LEFT = 0;
const short CTR = LEFT + 1;
const short RIGHT = CTR + 1;

//config variables
const short maxOverLine = 300;
const short moveInc = 150;
const short turnInc = 1;

//state variables
full_offset startOffset = {{0, 0}, 0};
short mode = FINDING_MINE;
bool headingHome = false;
short forks = 0;
StackArray_Gettable<short> branches;

void printBranches()
{
  short branchCount = branches.count();
  if(branchCount > 0) {
    sparki.println("stack:");
    for(int i = 0; i < branchCount - 1; i++)
    {
      sparki.print(branches.get(i));
      sparki.print(",");
    }
    sparki.println(branches.get(branchCount - 1));
  }
}

void turn_sparki(short angle)
{
  //don't need to do a full spin
  if(angle < 0) {
    angle = (-1)*((-angle) % 360);
  }
  else {
    angle = angle % 360;
  }

  //turn
  if(angle > 0) {
    sparki.moveLeft(angle);
  }
  else {
    sparki.moveRight(-angle);
  }
}

void turn_sparki_left()
{
  turn_sparki(90);
}

void turn_sparki_right()
{
  turn_sparki(-90);
}

void turn_sparki_around()
{
  turn_sparki(180);
}

void turn_sparki_rec(short angle)
{
  turn_sparki(angle);
    
  //record
  startOffset.theta += angle;
  
  //keep thetaOffset between +-360
  if(startOffset.theta < 0) {
    startOffset.theta = (-1)*((-startOffset.theta) % 360);
  }
  else {
    startOffset.theta = startOffset.theta % 360;
  }
}

void move_sparki(int delayTime, bool fwd)
{  
  if(fwd) {
    sparki.moveForward();
  }
  else {
    sparki.moveBackward();
  }
    
  delay(delayTime);
  sparki.moveStop();
}

void move_sparki_rec(int delayTime, bool fwd)
{
  move_sparki(delayTime, fwd);

  short fwdBkwd = 1;

  if(!fwd) {
    fwdBkwd = -1;
  }
    startOffset.loc.x += (fwdBkwd) * delayTime * cos(M_PI * startOffset.theta/180);
    startOffset.loc.y += (fwdBkwd) * delayTime * sin(M_PI * startOffset.theta/180);
}

void handleRemoteInput(bool rec)
{
  int code = sparki.readIR();

  switch(code) {
    case 70:
      if(rec) {
        move_sparki_rec(100, true);
      }
      else {
        move_sparki(100, true);
      }
      break;
    case 21:
      if(rec) {
        move_sparki_rec(100, false);
      }
      else {
        move_sparki(100, false);
      }
      break;
    case 69:
    case 68:
      if(rec) {
        turn_sparki_rec(3);
      }
      else {
        turn_sparki(3);
      }
      break;
    case 71:
    case 67:
      if(rec) {
        turn_sparki_rec(-3);
      }
      else {
        turn_sparki(-3);
      }
      break;
    case 7:
      sparki.gripperClose();
      break;
    case 9:
      sparki.gripperOpen();
      break;
    case 64:
      sparki.gripperStop();
      break;
    case 25:
      mode = TRAVERSING_MINE;
      break;
    case 24:
      mode = RETURNING_HOME;
      break;
    default:
      break;
  }
}

void findMine()
{
  handleRemoteInput(true);
}

void traverseMine()
{ 
  bool lEdgeOver = sparki.edgeLeft() <= maxOverLine;
  bool leftOver = sparki.lineLeft() <= maxOverLine;
  bool ctrOver = sparki.lineCenter() <= maxOverLine;
  bool rightOver = sparki.lineRight() <= maxOverLine;
  bool rEdgeOver = sparki.edgeRight() <= maxOverLine;

  bool couldGoForward = false;
  bool atFork = false;

  if(lEdgeOver || rEdgeOver)
  {
    //could be at a fork
    //make sure sparki didn't hit one side of a fork before the other
    move_sparki(2*moveInc, true);
    lEdgeOver = lEdgeOver || sparki.edgeLeft() <= maxOverLine;
    rEdgeOver = rEdgeOver || sparki.edgeRight() <= maxOverLine;

    //move forward, will use to check forward path and to properly turn if needed
    move_sparki(1200, true);

    //check if can go forward
    couldGoForward = sparki.lineLeft() <= maxOverLine || sparki.lineCenter() <= maxOverLine && sparki.lineRight() <= maxOverLine;

    if(lEdgeOver + couldGoForward + rEdgeOver > 1)
    {
      atFork = true;
      sparki.beep(NOTE_A1, 1000/4);
    }
  }

  if(atFork && !headingHome) //hit fork when going away
  {
    forks++;
    short branchCount = branches.count();
    if(forks < branchCount) //follow the branch decision
    {
      //will go fwd by default
      switch(branches.get(forks-1)) {
        case LEFT:
          turn_sparki_left();
          break;
        case RIGHT:
          turn_sparki_right();
          break;
        default:
          break;
      }
    }
    else if(forks == branchCount) //at last recorded branch, go down next path
    {
      short lastDecision = branches.pop();
      switch(lastDecision) {
        case LEFT: //went left last, go center if possible then right
          if(!couldGoForward) {
            branches.push(RIGHT);
            turn_sparki_right();
          }
          else {
            branches.push(CTR);
          }
          break;
        case CTR: //went fwd last, go right (should always be possible. if false, branch should have been popped off)
          if(rEdgeOver) {
            branches.push(RIGHT);
            turn_sparki_right();
          }
          break;
        default:
          break;
      }
    }
    else //add new branch
    {
      short decision = LEFT;

      //only case to look at. if left turn isn't possible then center must be
      if(!lEdgeOver)
      {
        decision = CTR;
      }
        
      branches.push(decision); //record decision

      //will go fwd by default
      switch(decision) {
        case LEFT:
          turn_sparki_left();
          break;
        default:
          break;
      }
    }
  }
  else if(atFork) //hit fork when returning home
  {
    short lastDecision = branches.get(forks - 1);
    short branchCount = branches.count();

    switch(lastDecision) {
      case LEFT: //turned left, came from the right
        turn_sparki_right();
        break;
      case CTR: //went straight, came from center
        if(forks == branchCount && !lEdgeOver) { //end branch, if can't turn left here, no RIGHT was possible, all paths visited for branch
          branches.pop();
        }
        break;
      case RIGHT: //turned right, came from left
        if(forks == branchCount) { //end branch, went right, so all paths visited for branch
          branches.pop();
        }
        turn_sparki_left();
        break;
      default:
        break;
    }

    forks--;
  }
  else if(lEdgeOver) //no branch, go left
  {      
    turn_sparki_left();
  }
  else if(rEdgeOver) //no branch, go right
  {
    turn_sparki_right();
  }
  else if(leftOver && !ctrOver && !rightOver) //drifted too far to right, move left slightly
  {
    turn_sparki(turnInc);
  }
  else if(!leftOver && (ctrOver || rightOver)) //drifted too far to left, move right slightly
  {
    turn_sparki(-turnInc);
  }
  else if(!lEdgeOver && !leftOver && !ctrOver && !rightOver && !rEdgeOver) //went off end of map, turn around
  {
    if(headingHome) //arrived at home
    {
      headingHome = false;
      
      sparki.beep(NOTE_G2, 1000/4);

      //clear the IR code in case one was sent during the traversal
      while(sparki.readIR() != -1) { }
      
      //state machine branch point: take input, if charges are all gone, return home
      mode = FINDING_CHARGE; //go find another charge
    }
    else //arrived at branch end
    {
      move_sparki(1500, true);
      sparki.gripperOpen(4);
      delay(2000);
      move_sparki(1500, false);
      turn_sparki_around();
      headingHome = true;
    }
  }
  else //go fwd
  { 
    move_sparki(moveInc, true);
  }
}

void findCharge()
{
  handleRemoteInput(false);
}

void returnHome()
{
  //sparki will be facing the opposite direction
  startOffset.theta += 180;
  if(startOffset.theta > 360) {
    startOffset.theta = startOffset.theta % 360;
  }

  //move to original x position
  turn_sparki(180 - startOffset.theta); //turn to angle of +-180
  if(startOffset.loc.x > 0) {
    move_sparki(startOffset.loc.x, true);
  }
  else {
    move_sparki(-startOffset.loc.x, false);
  }

  //move to original y position
  turn_sparki(90); //turn to angle of 90
  if(startOffset.loc.y > 0) {
    move_sparki(startOffset.loc.y, true);
  }
  else {
    move_sparki(-startOffset.loc.y, false);
  }

  turn_sparki(90);

  mode = COMPLETE;
}

void setup() { }

void loop()
{
  sparki.clearLCD();
  sparki.print("mode:");
  sparki.println(mode);
  sparki.print("offset:");
  sparki.print(startOffset.loc.x);
  sparki.print(",");
  sparki.print(startOffset.loc.y);
  sparki.print(",");
  sparki.println(startOffset.theta);
  sparki.print("going home:");
  sparki.println(headingHome);
  sparki.print("forks:");
  sparki.println(forks);
  printBranches();
  
  switch(mode) {
    case FINDING_MINE:
      findMine(); //DO THIS
      break;
    case TRAVERSING_MINE:
      traverseMine(); //DETERMINE WHEN FINISHED
      break;
    case FINDING_CHARGE:
      findCharge();
      break;
    case RETURNING_HOME:
      returnHome(); //DO THIS
      break;
    default:
      break;
  }

  sparki.updateLCD();
}

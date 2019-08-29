#include "axisMotor.h"
#include "readSerial.h"

axisMotor::axisMotor(motorShield *ms, char axis, uint pin, uint limit, uint rpm, uint steps, uint port) {

  if ( !ms->hasBegun() )
    ms->begin();

  motor = ms->getStepperMotor(port, steps); 

  if ( !motor )
    return;

  motor->setSpeed(rpm);
  this->axis = axis;
  this->limitPin = pin;
  this->limit = limit;
  this->direction = FORWARD;
  this->amIHome = false;
  
  pinMode(this->limitPin, INPUT);

  // Send the motors home so we're starting from a known position
  //home();

  if ( !cmd )
    cmd = reader->getCmdPtr();

  return;
}

axisMotor::~axisMotor(void) {
  delete motor;
  return;
}

// After any movement, call this to track the "position" of the rotating motor
void axisMotor::setPosition( float steps ) {

  if ( direction != FORWARD && direction != BACKWARD ) 
    return;
  
  position += (direction==FORWARD) ? steps : -steps;

  return;
}

void axisMotor::setHome(bool isHome, float pos) {
  this->amIHome = isHome;
  if (amIHome)
    this->position = 0.0f;
  else if ( pos > 0.0 )
    this->position = pos;
    
}

void axisMotor::home(void) {

  if ( !motor ) {
    return;
  }
  if ( digitalRead(limitPin) == HIGH )
    return;

  stepMotor( limit+1, BACKWARD );
  away();
  position = 0;
  amIHome = true;

  releaseMotor();

  return;
}
bool axisMotor::away( void ) {

  if ( digitalRead(limitPin) != HIGH )
    return true;

  if ( !motor ) {
    return false;
  }

  while ( digitalRead(limitPin) == HIGH ) {

    motor->step(1, FORWARD, MICROSTEP);

    // If someone pushed the panic button, stop RIGHT NOW!
    if ( CRASH_STOP ) {
      motor->release();
      return false;
    }
  }

  if ( digitalRead(limitPin) != HIGH ) {
    position = 0;
#ifdef TEST
    Serial.println("Off the limit switch");
#endif
  }

  releaseMotor();
  return (digitalRead(limitPin) != HIGH);
}

void axisMotor::stepMotor(float steps, uint type) {

  if ( !motor ) {
#ifdef TEST
    Serial.print(F("No such motor on "));Serial.println(axis);
#endif
    return;
  }

  if ( steps == 0.0 ) {
#ifdef TEST
    Serial.println(F("No motion requested"));
#endif
    return;
  }

  this->direction = (steps>0.0) ? FORWARD : BACKWARD;
  steps = fabs(steps);

  // Make sure we're off the limit switch
  away();

  // Refuse to allow the screw to move the carriage past it's physical limit
  if ( direction == FORWARD && steps+position > limit ) {
    float decimal = steps - (uint)steps;
    steps = limit - position + decimal;
  }

  // Always approach the final posiion from the same direction.... 
  if ( axis != 'X' && direction == FORWARD )
    steps += overshoot;

  // Take the major steps....
  int stp = 0, ustp = 0;
  bool cont = true;
  while ( stp < (int)steps ) {

    motor->step(stpSize, direction, type);
    stp += stpSize;

    // Make sure we need to be keepin' on
    cont = checkContinueStatus();
    if ( !cont )
      break;
    
  } // End while ( stp <= steps )

  // If we weren't interupted along the way ...
  if ( cont ) {


    if ( axis != 'X' && direction == FORWARD ) {
      motor->step(overshoot, BACKWARD, type);
      stp -= overshoot;
    }

    // Take the minor (micro) steps to our destination
    steps -= (int)steps;
    steps *= 100;

    while ( ustp < steps ) {

      motor->step(stpSize, direction, MICROSTEP);
      ustp += stpSize;
      
      // Make sure we ought to be continuting
      cont = checkContinueStatus();
      if ( !cont )
       break;

    } // End while ( ustp <= steps )

  } // End if ( stp > steps )

  if ( cont )
    setPosition(stp + ustp/100.0);
  else
    setPosition(0.0f);

#ifdef TEST
  Serial.print(axis);Serial.print(" took ");Serial.print(stp);Serial.print(" steps ");
  if ( ustp > 0 ) {
    Serial.print(", and ");Serial.print(ustp);Serial.print(" microsteps");
  }
  Serial.print(" to position ");Serial.println(getPosition());
#endif

  amIHome = false;
  return;
}

bool axisMotor::checkContinueStatus(void) {

  if ( digitalRead(limitPin) == HIGH ) {
#ifdef TEST
    Serial.println(F("On the limit switch"));
#endif
    away();
    return false;
  }

  // Check and see if we need to stop what we're doing
  reader->read();
  if ( cmd->operation == 's' || cmd->operation == 'S' ) {
#ifdef TEST
    Serial.println(F("User requested stop"));
#endif
    return false;
  }

  // If someone pushed the panic button, stop RIGHT NOW!
  if ( CRASH_STOP || FATAL_ERROR ) {
#ifdef TEST
    Serial.println(F("Something bad is happening"));
#endif
    motor->release();
    return false;
  }
  
  return true;
}


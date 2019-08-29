#ifndef MOTORBOSS_H
#define MOTORBOSS_H

#include "config.h"
#include "motorShield.h"
#include "axisMotor.h"

class motorBoss {
 public:
  motorBoss(void) {

    // Create the motor shield class...
    shield1 = new motorShield();
    if ( !shield1 ) {
      Serial.println(F("ff No such shield on 0x60"));
      FATAL_ERROR = true;
      return;
    }

    // ...for both shields
    shield2 = new motorShield(0x61);
    if ( !shield2 ) {
      Serial.println(F("ff No such shield on 0x61"));
      FATAL_ERROR = true;
      return;
    }

    // Add the 28BYJ-48 motors for the X and R axes, these are 2048 steps/revolution
    if ( shield2 ) {
      xmotor = new axisMotor(shield2, 'X', XInputPin, XLimit, xRPM, x_steps_per_revolution, xPort);
      if ( !xmotor ) {
        Serial.println(F("ff X motor failed"));
        FATAL_ERROR = true;
        return;
      }

      rmotor = new axisMotor(shield2, 'R', RInputPin, RLimit, rRPM, r_steps_per_revolution, rPort);
      if ( !rmotor ) {
        Serial.println(F("ff R motor failed"));
        FATAL_ERROR = true;
        return;
      }
    }

    // Add the NEMA-17 motors for the Y & Z axes, these motors are 200 steps/revolution
    // Motor instansiator (shield, axis, limit switch pin, max steps, rpm, steps/rev, shield port)
    if ( shield1 ) {
      zmotor = new axisMotor(shield1, 'Z', ZInputPin, ZLimit, zRPM, z_steps_per_revolution, zPort);
      if ( !zmotor ) {
        Serial.println(F("ff Z motor failed"));
        FATAL_ERROR = true;
        return;
      }

      ymotor = new axisMotor(shield1, 'Y', YInputPin, YLimit, yRPM, y_steps_per_revolution, yPort);
      if ( !ymotor ) {
         Serial.println(F("ff Y motor failed"));
        FATAL_ERROR = true;
        return;
      }
    }

    pluggedIn = false;
    type = 1;

    resetHome();

    return;
  }
  ~motorBoss(void) {
    home();
    return;
  }

  axisMotor *getMotorPtr( char axis ) {
   // Make the axis label case insensitive
   if ( axis > 90 )
      axis -= 32;
    if ( axis == 'Y' )
      return ymotor;
    else if ( axis == 'X' )
      return xmotor;
    else if ( axis == 'Z' && zmotor )
      return zmotor;
    else if ( axis == 'R' && rmotor )
      return rmotor;
    else 
      return 0x0;
  }

 // Find the home position 
  void resetHome( void ) {

    // Make sure we're not dragging an ODU with us
    if ( pluggedIn )
      unPlug();
       
    // Send each motor backward until it hits the limit switch
    // And reset the position & home switch in axisMotor appropriately
    if ( xmotor ) {
      xmotor->stepMotor( XLimit+1, BACKWARD);
      xmotor->setHome(true);
    }
    if ( rmotor ) {
      rmotor->stepMotor( RLimit+1, BACKWARD);
      rmotor->setHome(true);
    }
    if ( zmotor ) {
      zmotor->stepMotor( ZLimit+1, BACKWARD);
      zmotor->setHome(true);
    }
    if ( ymotor ) {
      ymotor->stepMotor( YLimit+1, BACKWARD);
      ymotor->setHome(true);
    }

    return;
  }

  // Relative move to the ODU connector at the 
  // position defined in the y_steps_to_connector array
  void moveTo( uint connector ) {

    // Make sure we're not dragging an ODU with us
    if ( pluggedIn )
      unPlug();
    
    if ( connector == 0 ) {
      home();
      return;
    }

    // Figure out where we are, and where we're supposed to be based on 
    // which type of ODU this is (2 & 4 are backwards from 1 & 3) and go
    float distanceY = y_steps_to_connector[type-1][connector] - ymotor->getPosition();

    // And move x into something like proper position
    float distanceX;
    if ( connector < 19 )
      distanceX = x_steps_to_odu - plugSteps - xmotor->getPosition();
    else 
      distanceX = calibSteps - plugSteps - xmotor->getPosition();

    float distanceR = 0.0f;
    float distanceZ = 0.0f;

    if ( rmotor && zmotor ) {
      // Make sure the connector is in the proper orientation
      if ( connector <= 16 ) {
        distanceR = r_steps_to_90 - rmotor->getPosition();
        distanceZ = z_steps_to_low_connectors - zmotor->getPosition();
      }
      else if ( connector == 17 ) {
        if ( type % 2 ) {                                                 // Odd Types... 
          distanceR = r_steps_to_180 - rmotor->getPosition();             // Rotate to 180 degrees
          distanceZ = z_steps_to_high_connectors +z_offset_from_rotation - zmotor->getPosition();
        }                                                                 // And move up to the connector
        else {                                                            // Even Types
          distanceR = r_steps_to_0 - rmotor->getPosition();               // Stay at 0 degrees
          distanceZ = z_steps_to_high_connectors - zmotor->getPosition(); // Move up to the connector
        }
      }
      else if ( connector == 18 ) {
        if ( type % 2 ) {                                                 // Odd Types... 
          distanceR = r_steps_to_0 - rmotor->getPosition();               // Return to 0 degrees
          distanceZ = z_steps_to_high_connectors - zmotor->getPosition(); // Move up to the connector
        }
        else {                                                            // Even Types
          distanceR = r_steps_to_180 - rmotor->getPosition();             // Return to 0 degrees
          distanceZ = z_steps_to_high_connectors + z_offset_from_rotation - zmotor->getPosition();
        }                                                                 // And move to the connector up, up, and up
      }
      else if ( connector == 19 ) {                                       // Position for calibrating the system
        distanceR = r_steps_to_0 - rmotor->getPosition();
        distanceZ = z_steps_to_low_connectors - zmotor->getPosition();
      }

    } // End if ( rmotor && zmotor )

    // Move to the connector, but do it in the proper order so we don't hit anything or bang into something
    if ( distanceY )
      ymotor->stepMotor(fabs(distanceY), (distanceY>0) ? FORWARD : BACKWARD);
      
    if ( (connector == 17 && type % 2) || (connector == 18 && !(type % 2)) ) { // If we're going high & turning over....
      if ( distanceX )
        xmotor->stepMotor(fabs(distanceX), (distanceX>0) ? FORWARD : BACKWARD);
      if ( distanceZ )
        zmotor->stepMotor(fabs(distanceZ), (distanceZ>0) ? FORWARD : BACKWARD);
      if ( distanceR )
        rmotor->stepMotor(fabs(distanceR), (distanceR>0) ? FORWARD : BACKWARD);
    }
    else {                                                                     // If we're not going high or not turning over
      if ( distanceX )
        xmotor->stepMotor(fabs(distanceX), (distanceX>0) ? FORWARD : BACKWARD);
      if ( distanceR )
        rmotor->stepMotor(fabs(distanceR), (distanceR>0) ? FORWARD : BACKWARD);
      if ( distanceZ )
        zmotor->stepMotor(fabs(distanceZ), (distanceZ>0) ? FORWARD : BACKWARD);
    }
    
    return;
  }

  // Move back until we hit the stop switch at the 0 position
  // By default, send both motors home
  void home(char axis='\0') {

    // Case insensitive
    if ( axis > 90 )
      axis -= 32;

    unlockMotors();

    if (pluggedIn )
      unPlug();

    if ( !axis ) {
      if ( zmotor && !zmotor->getHome() )
        zmotor->home();
      if ( xmotor && !xmotor->getHome() )
        xmotor->home();
      if ( rmotor && !rmotor->getHome() )
        rmotor->home();
      if ( ymotor && !ymotor->getHome() )
        ymotor->home();
    }
    else {
      axisMotor *motor = (getMotorPtr(axis));
      if ( motor && !motor->getHome() )
        motor->home();
    }

    return;
  }

  void checkPlacement( int conn ) {
    moveTo(conn);
    plugIn();
    delay(2500);
    unPlug();
    home();
    return;
  }

  void plugIn(bool calib=false) {
    if ( !xmotor )
      return false;
    if ( pluggedIn )
      return;

    if ( !calib )
      xmotor->stepMotor(x_steps_to_odu-xmotor->getPosition(), FORWARD);
    else
      xmotor->stepMotor(calibSteps-xmotor->getPosition(), FORWARD);
      
    delay(50);
    pluggedIn = true;
    return;
  }

  void unPlug(void) {
    if ( !xmotor )
      return false;

    float pos = (x_steps_to_odu - plugSteps) - xmotor->getPosition();
    if ( pos < 0 ) {
      xmotor->stepMotor(fabs(pos), BACKWARD);
      delay(50);
    }  
    pluggedIn = false;

    return;
  }

  uint setType(int whichType) {
    
    if (whichType == 1 || whichType == 3 )
      this->type = 1;
    else
      this->type = 2;
      
    return this->type;
  }

  void unlockMotors(void) {
    release('X');
    release('Y');
    release('Z');
    release('R');
    return;
  }

  void release(char axis) {
    if (!getMotorPtr( axis ))
      return;
    (getMotorPtr( axis ))->releaseMotor();
    return;
  }

  // rotate the R axis motor by <angle> degrees
  void rotate ( float angle ) {

    if ( !rmotor )
      return;

    if ( !angle )
      return;

    // Do the rotation
    float steps = steps_per_degree * angle;
    rmotor->stepMotor(steps);

    return;
  }

  // Spit out where the motors think they are
  void position(void) {
    int a = 0, b = 0;
    char pos[10];
    Serial.print(F("Connector head at ("));

    a = (int)xmotor->getPosition();
    b = 100*(xmotor->getPosition() - a);
    sprintf(pos, "%i.%i", a, b);
    Serial.print(pos);
    Serial.print(F(", "));

    a = (int)ymotor->getPosition();
    b = 100*(ymotor->getPosition() - a);
    sprintf(pos, "%i.%i", a, b);
    Serial.print(pos);
    Serial.print(F(", "));
  
    a = (int)zmotor->getPosition();
    b = 100*(zmotor->getPosition() - a);
    sprintf(pos, "%i.%i", a, b);
    Serial.print(pos);
    Serial.print(F(", "));

    //a = degrees_per_step * (int)rmotor->getPosition();
    //b = degrees_per_step * 100*(rmotor->getPosition() - a);
    
    a = (int)rmotor->getPosition();
    b = 100*(rmotor->getPosition() - a);
    sprintf(pos, "%i.%i", a, b);
    Serial.print(pos);
    //Serial.print((char)0xC2);
    //Serial.print((char)0xB0);
    Serial.println(F(")"));

    return;
  }

  // Move the motor for X||Y||Z "steps" steps forward or backward
  void step ( char axis, float steps ) {

    // Make the axis label case insensitive
    if ( axis > 90 )
      axis -= 32;

    axisMotor *motor = (getMotorPtr(axis));
    if ( !motor ) {
      Serial.println(F("No motor defined!"));
      return;
    }

    // Move the requested motor, full checking and pause for stop requests
    motor->stepMotor( steps );

    return;
  }

 private:

  axisMotor   *xmotor;            // motor on the plug in/plug out axis
  axisMotor   *ymotor;            // motor on the long axis
  axisMotor   *zmotor;            // Up & down motor
  axisMotor   *rmotor;            // Rotation axis
  
  motorShield *shield1;           // circuit board controlling the motors
  motorShield *shield2;           // circuit board controlling the motors

  bool pluggedIn;
  uint type;
  
};

static motorBoss *boss;

#endif

/*
 * Arduino driver code for the Notre Dame HB ODU Test Stand in the ODUQC Package
 * D. Karmgard -- 15 Apr 18 (Beware the ides of April!)
 */

#include "readSerial.h"
#include "circuit.h"
#include "motorBoss.h"
#include "config.h"

bool CRASH_STOP  = false;
bool FATAL_ERROR = false;

void panicSwitch(void);

// Run setup once, the first time through before loop()
void setup() {

  reader     = new readSerial();
  cmd        = reader->getCmdPtr();
  controller = new circuit();

  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), panicSwitch, LOW);

#ifdef TEST
  randomSeed(analogRead(7));
#endif

  Serial.println(F("00 Ready"));

  return;
} // End setup()

// Run through the loop continuously
void loop() {

   static bool echo = false;

  int whichType = 0;

  // If there's available input, go get it
  if (reader->isAvailable()) {

    // Get the available input
    reader->read();
    if ( !reader->msgAvailable ) {
      delay(500);
      return;
    }

    if ( echo ) {
      Serial.print(cmd->operation);
      Serial.print(" ");
      Serial.println(reader->getInput());
    }

    // And decide what to do with it based 
    // on the 1 character operation code
    switch (cmd->operation) {

    case 'i':
      // Define the response to the programs ID request
      //Serial.println(F("01 QCBot"));
      Serial.println(F("01 ODUQC"));
      break;

    // Toggle GORT mode
    case 'G':
      if ( !boss ) {
        boss = new motorBoss();
        boss->setType(cmd->steps);
      } 
      Serial.println(F("a1 GORT is awake!"));
      break;
    case 'g':
      if ( boss ) {
        delete boss;
        boss = 0x0;
        Serial.println(F("a2 Klautu barada nictu"));
      }
      else
        Serial.println(F("GORT is sleeping"));
      break;

    case 'Q': case 'q':
#ifndef TEST
      if ( controller )
        controller->turnEmOff();
#endif
      if ( boss ) {
        boss->home();
        //Serial.println(F("02 Lights out"));
      }
      break;

    case 'c':
      if ( CRASH_STOP ) {
        CRASH_STOP = false;
        if ( controller )
          controller->roxanne();
        Serial.println(F("ff Crash Stop cleared"));
      }
      else if (FATAL_ERROR) {
        FATAL_ERROR = false;
        if ( controller )
          controller->roxanne();
        Serial.println(F("ff Fatal Error cleared"));
      }
      break;

    case 'C':

      if ( boss ) {
        boss->moveTo(19);
        boss->plugIn(true);
      }

      if ( controller )
        controller->calibrate();

      if ( boss ) {
        boss->unPlug();
        boss->moveTo(0);
      }

      break;

    case 'D':

      if ( controller ) {
        uint d = controller->setDelay( cmd->steps );
        Serial.print(F("11 Delay set to "));Serial.println(d);
      }
      break;

    case 's':

      // Clear the reader in case we get a stop request later
      reader->flushCommand();

      // Plug the connector into the ODU
      if ( boss )
        boss->plugIn();
      
      // Light 'em up!
      if  ( controller )
        controller->sequence();

      // Unplug the connector
      if ( boss )
        boss->unPlug();

      Serial.println(F("04 done"));

      if ( controller )
        delay( controller->getDelay() );

      break;

    case 'a':
      if ( boss ) 
        boss->checkPlacement( atoi(cmd->input) );
      break;
      
    case 'b':
      if ( controller ) {
        bool sub = controller->setSubtract((bool)(atoi(cmd->input)));
        Serial.print(F("05 Background subtraction "));  

        if ( sub )
          Serial.println(F("on"));
        else
          Serial.println(F("off"));
      }
      break;

    case 'N':
      if ( controller )
        controller->dumpNorms();
      break;

    case 'n':
      if ( controller ) {
        uint leds = controller->setNumLEDs(atoi(cmd->input));
        Serial.print(F("06 Number of LEDs = ")); 
        char msg[2];
        itoa(leds, msg, 10);
        Serial.println(msg);
      }
      break;

    case 'S':
      if ( controller ) {
        uint samples = controller->setSampleSize(atoi(cmd->input));
        Serial.print(F("07 Sample size = "));
        char msg[4];
        itoa(samples, msg, 10);
        Serial.println(msg);
      }

      break;
      
    case 'h':
      if ( boss ) {
        boss->home();
        Serial.println(F("08 Motors parked"));
      }
      break;

    case 'm':
      if ( boss ) {
        boss->moveTo( cmd->steps );
        Serial.println(F("09 Move Complete"));
      }
      break;
 
    case 'H':
      if ( boss )
        boss->resetHome();
      break;

    case 'T':
      if ( boss ) {
        uint type = boss->setType((int)cmd->steps);
        Serial.print(F("10 ODU type "));
        ((int)(cmd->steps) % 2) ? Serial.println(F("odd")) : Serial.println(F("even"));
      }
      break;

    case 'e':
      echo = !echo;
      if ( echo )
        Serial.println(F("Echo on"));
      else
        Serial.println(F("Echo off"));
      break;

    case 'A':
      if ( controller ) {
        if ( !controller->getAllOn() )
          controller->turnEmOn();
        else
          controller->turnEmOff();
      }
      break;
    
    case 'p': 
      if ( boss )
        boss->position();
      break;

    case 'x':
      if ( boss ) {
        boss->step('X', cmd->steps );
        boss->release('X');
      }
      break;

    case 'y':
      if ( boss ) {
        boss->step('Y', cmd->steps );
        boss->release('Y');
      }
      break;

    case 'z':
      if ( boss ) {
        boss->step('Z', cmd->steps );
        boss->release('Z');
      }
      break;

    case 'r':
      if ( boss ) {
        boss->step('R', cmd->steps );
        boss->release('R');
      }
     break;

    case 'R':
     if ( boss ) {
       boss->rotate(cmd->steps );
       boss->release('R');
     }
     break;

    case 'U':
      if ( boss )
        boss->unlockMotors();
      break;

    case 't':
      if ( controller )
        controller->toggleLED( cmd->steps );
      break;

    case 'I':
      if ( boss )
        boss->plugIn();
      break;

    case 'O':
      if ( boss )
        boss->unPlug();
      break;

    default:
      Serial.print(cmd->operation);
      Serial.print(cmd->input);

    } // End switch (cmd->operation)
  } // End if (reader->isAvailable())

  // Check the light levels in the box
  uint lightLevel = analogRead(photoPin);
  if ( lightLevel > maxLightLevel ) {
    if ( !FATAL_ERROR ) {
      FATAL_ERROR = true;
      if ( controller )
        controller->roxanne();
      Serial.println(F("fe Fatal Error: Too much light in the box"));
    }
  }
  else if ( FATAL_ERROR ) {
    FATAL_ERROR = false;
    Serial.println(F("ff Fatal Error: light leak cleared"));
    if ( controller )
      controller->roxanne();
  }
  else if ( CRASH_STOP ) {
    if ( controller )
      controller->roxanne();
    if ( boss )
      boss->unlockMotors();
  }

  delay(500);

  return;
  
} // End loop()

/*
 * Hitting the panic switch brings us here, where we 
 * stop whatever's going on in the robot
 */
void panicSwitch(void) {
  if ( !CRASH_STOP ) {
    Serial.println(F("fd PANICING!"));
    CRASH_STOP = true;
  }
  return;
}

#ifndef MOTORSHIELD_H
#define MOTORSHIELD_H

#include <Adafruit_MotorShield.h>
#include "config.h"

class motorShield : public Adafruit_MotorShield {
 public:
  motorShield(int address=0x60) {
    start(address);
  }
  ~motorShield(void){begun = false;}

  void start(int address=0x60) {
    // Create the motor shield object with the default I2C address
    AFMS = Adafruit_MotorShield(address); 
    AFMS.begin();
    begun = true;
  }

  Adafruit_StepperMotor *getStepperMotor(uint port, uint steps=200 ) {
    // Connect a stepper motor with steps/revolution to port
    Adafruit_StepperMotor *m = AFMS.getStepper(steps, port);
    if ( m == 0x0 ) {
      Serial.println(F("fa No such motor on port "));
      char msg[3];
      itoa(port, msg, 10);
      Serial.println(msg);
      return 0x0;
    }
    return m;
  }

  bool hasBegun(void) {return begun;}

 private:
  Adafruit_MotorShield AFMS;
  bool begun;

};
#endif

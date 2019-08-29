#ifndef AXISMOTOR_H
#define AXISMOTOR_H

#include "motorShield.h"
#include "config.h"

class axisMotor : public Adafruit_StepperMotor {

 public:
  axisMotor           (motorShield *,   char, uint, uint, uint, uint, uint);
  ~axisMotor          (void);

  void  setPosition   (float);
  float getPosition   (void)   {return this->position;}
  bool  getHome       (void)   {return this->amIHome;}
  void  setHome       (bool=true, float=-1.0f);
  
  void home           (void);
  bool away           (void);

  void stepMotor      (float, uint=DOUBLE);
  void releaseMotor   (void) {motor->release();}

 private:
  Adafruit_StepperMotor *motor;

  bool checkContinueStatus(void);
  bool  amIHome;
  uint  direction;
  uint  limitPin;
  uint  limit;
  float position;
  char  axis;

#endif
};

#ifndef READSERIAL_H
#define READSERIAL_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "config.h"

#define FORWARD  1
#define BACKWARD 2

// Define a structure to hold a request accross the serial port
struct command {
  char   operation = '\0';    // What to do
  float  steps     = 0;       // Steps to take
  char   input[INPUT_SIZE+2]; // Input the user (or program) entered
};
static command * cmd;

// Accept input on the serial line and build the structure
// which will tell the arduino what, exactly, to do
class readSerial {

 public:
  readSerial(void);
  ~readSerial(void);
  bool isAvailable(void);
  void read(void);
  void flushCommand(void);
  command * getCmdPtr(void);

  char * getInput( void );

  char input[INPUT_SIZE+2];
  unsigned int characters;
  bool msgAvailable;
};
static readSerial * reader;
#endif


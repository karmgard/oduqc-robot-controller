#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <Arduino.h>
#include <EEPROM.h>
#include "readSerial.h"
#include "config.h"

class circuit {

 public:
  circuit                    ( void );
  ~circuit                   ( void );

  void    sequence           ( void );
  void    calibrate          ( void );
  bool    setSubtract        ( bool );
  uint    setNumLEDs         ( int );
  uint    setSampleSize      ( int );

  bool    turnEmOff          ( void );
  void    roxanne            ( void ); // You don't have to turn on the reeeeeed light
  void    dumpNorms          ( void );

  void    turnEmOn           ( void );
  void    toggleLED          ( int );
  bool    getAllOn           ( void ) {return allOn;}

  uint    setDelay           ( uint );
  uint    getDelay           ( void ) {return DELAY;}

 private:
  bool    allOn;
 
  void    setRegisters       ( char [INPUT_SIZE+2] );
  void    updateRegisters    ( byte[3] );

  void    getDiodeNoise      ( void );

  // EEPROM read/write utilities
  void    writeData          ( void );
  void    readData           ( void );
  void    EEPROMWriteInt     ( int, int );
  uint    EEPROMReadInt      ( int ) ;
  
  // Three 8-bit shift registers
  byte registers[3];

  //  Program variables -- loaded by the human interface from oduqc.rcp
  int  NUM_SAMPLES;
  int  NUM_CHANNELS;
  bool SUBTRACT_NOISE;
  uint DELAY;
  uint LED_DELAY;
  uint PD_DELAY;

  float Snoise = 0, Lnoise = 0;
  float normalization[18];

};
static circuit * controller;
#endif

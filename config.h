#ifndef CONFIG_H
#define CONFIG_H

// Defs for all
#define TEST            // Add a load of test routines

// Maximum number of steps to allow in software
#define XLimit 2000
#define YLimit 3225
#define ZLimit 1635
#define RLimit 1200  // a smidge over 200 degrees.... we only need 180 + a bit for lash

// For circuit.h
#define latchPin      8    // These are the digital pins
#define clockPin      12   // used for PWM that talk to the 
#define dataPin       11   // shift registers on the card

#define photoPin      0    // These three are the analog 
#define smallDiodePin 4    // channels that the photo-
#define largeDiodePin 2    // sensors connect to

// For axisMotor.h
// How many steps to take at once before checking for stop conditions?
#define stpSize 1

// Limit switch input channels on the arduino
#define XInputPin 4
#define YInputPin 5
#define ZInputPin 6
#define RInputPin 7

// How fast the motor moves (RPM) (really, how much power is supplied to the motor)
#define xRPM 10
#define yRPM 25
#define zRPM 25
#define rRPM 10

// NEMA 17 motors are 200 steps/revolution
#define z_steps_per_revolution 200
#define y_steps_per_revolution 200

// 28BYJ-48 motors are 2048 steps/revolution
#define x_steps_per_revolution 2048
#define r_steps_per_revolution 2048

// Step sizes for the x, y, & z axes in millimeters. Documentation, not used
#define steps_per_millimeter_28BYJ_48   40.0f
#define steps_per_millimeter_NEMA_17    25.0f

// 360 degrees/revolution / 2048 steps/revolution
#define degrees_per_step    360.0 / 2048.0    // 0.17578125 degrees = 1 step

// 2048 steps/revolution / 360 degrees/revolution
#define steps_per_degree    2048.0 / 360.0    // 5.68889 steps = 1 degree rotation

// Which bus on the motor shield is this motor connected to
#define yPort 1   // Shield 1
#define zPort 2

#define xPort 1   // Shield 2
#define rPort 2

// For oduqc.ino the Uno listens for interupts on digital pin 2
#define interruptPin 2

// For readSerial.h
#define INPUT_SIZE 32   // Max size of the serial line on the Uno = 64 bytes
#define BAUD       115200

// General defines
typedef unsigned int uint;
extern bool CRASH_STOP;
extern bool FATAL_ERROR;

// For the EEPROM write to store the calibration constants (in circuit.cpp)
const int ADDRESS_OFFSET = 50;

/***********************************************************************************************/

/***
  * Where the connectors are located relative to home on the robot
  * for all 18 connectors and both styles of ODU (there's ~ 150 steps
  * between connectors on the faceplate)
  * 
  * These are floats with the format 
  * (int)position = steps from HOME
  * position - (int)position = microsteps from (int)position
  * y_steps_to_connector[0] == HOME
  * types 1 & 3 are listed as y_steps_to_connector[0][...], types 2 & 4
  * are in connectorsPosition[1][...]
  * 
 ***/
const float y_steps_to_connector[2][20] = {
       // Odd type ODUs 1 & 3 -- Connectors 1, 2, 3, 18, and 17 completed
      {   0.00f,  46.00f,  194.00f,  339.00f,  570.00f,  720.00f,  870.00f, 1020.00f, 1170.00f, 1320.00f, 
       1470.00f, 1620.00f, 1770.00f, 1920.00f, 2070.00f, 2220.00f, 2370.00f,  136.00f, 1236.00f,  750.00f},

      // Even type ODUs 2 & 4
      {   0.00f, 2370.00f, 2225.00f, 2080.00f, 1935.00f, 1790.00f, 1645.00f, 1500.00f, 1355.00f, 1210.00f,
       1065.00f,  920.00f,  775.00f,  630.00f,  485.00f,  340.00f,  155.00f, 1755.00f,  565.00f,  750.00f}
};

// Maximum reading on the photo diode in analog channel 
// <photoPin> before yelling about light leaks
#ifdef TEST
#define maxLightLevel 1025
#else
#define maxLightLevel 24
#endif

// Always approach from the same direction... this defines the overshoot/return going forward
#define overshoot 25.0f

// For axisMotor.h

// How far is it from the connector in the parking
// position to the face of the ODU (NOT to the pins!)
#define x_steps_to_odu 950.0f //1070.0f

// When movin to a connector the X motor stops plugSteps
// short of the ODU. Testing subsequent connectors 
// moves the connector in-and-out by plugSteps amount
// Needs to be big enough to clear the alignment pins
// while moving and turning. 
#define plugSteps       500.0f

// When calibating the diodes to each other with the acrylic
// light guide on the ODU platform, this is how far you go 
// with the connector to just touch the face of the guide
// should be a bit bigger than x_steps_to_odu
#define calibSteps     1950.0f

// How far up the Z axis do we need to go to reach the first 16 connectors?
// with the jumpper connector vertical at 90 degrees (r_steps_to_90)?
#define z_steps_to_low_connectors 469.0f

// How far up the Z axis do we need to move to reach the 17th & 18th connectors
// while the connector is in the upper horizontal position (r_steps_to_0)
#define z_steps_to_high_connectors 679.0f

// If we turn the connector to 180 degrees (r_steps_to_180), into the lower
// horizontal position, we need to move up the Z axis by this much to compensate
#define z_offset_from_rotation 931.0f

// From the parking position (home) on the r axis, how many steps
// does the motor have to take to be in the upper horizontal position
// (r_steps_to_0)?, the vertical position (r_steps_to_90)?, or the 
// lower horizontal position (r_steps_to_180)? 
#define r_steps_to_0   63.0f
#define r_steps_to_90  581.0f
#define r_steps_to_180 1107.0f

/***********************************************************************************************/

#endif

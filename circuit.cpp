#include "circuit.h"
#include <elapsedMillis.h>

circuit::circuit(void) {

  // Initialize the registers
  registers[0] = registers[1] = registers[2] = 0;

  // Set the digital pins for output
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin,  OUTPUT);
  pinMode(clockPin, OUTPUT);

  // Defaults for illuminating the ODU connector
  NUM_SAMPLES    = 64;
  NUM_CHANNELS   = 18;
  SUBTRACT_NOISE = true;
  DELAY          = 750;
  LED_DELAY      = 0.5 * DELAY;
  PD_DELAY       = 0.5 * DELAY;

  Snoise = Lnoise = 0.0f;

  // Set the normalization array to zero
  memset( (void *)normalization, 0, 18 * sizeof(float) );

  // And fill it from EEPROM
  readData();
 
  if ( !cmd )
    cmd = reader->getCmdPtr();

  updateRegisters(registers);
  allOn = false;
  
  return;
}

circuit::~circuit(void) {
  return;
}

// Update the LED bar from the light sensor when we're not doing anything else
void circuit::roxanne(void) {
  // You might have to turn on the red light
  byte none[3] = {0,0,0};
  updateRegisters(none);
  return;
}

// Write the three bytes to the shift registers
void circuit::updateRegisters(byte reg[3]) {

  // Open the latch... (prevents updates to the pins)
  digitalWrite(latchPin, LOW);
  byte leds;

  if ( CRASH_STOP || FATAL_ERROR )
    leds = 188;
  else
    leds = 124;

  shiftOut(dataPin, clockPin, MSBFIRST, reg[2]+leds);
  shiftOut(dataPin, clockPin, MSBFIRST, reg[1]);
  shiftOut(dataPin, clockPin, MSBFIRST, reg[0]);

  // and close the latch (allows updates to go)
  digitalWrite(latchPin, HIGH);

  return;
} // End void updateRegisters(byte reg[3])

// Turn off the first 18 leds
inline bool circuit::turnEmOff(void) {
  registers[0] = registers[1] = registers[2] = 0;
  updateRegisters(registers);

  allOn = false;
  return false;
} // End turnEmOff()

// Turn on the first 18 leds
void circuit::turnEmOn(void) {
  registers[0] = registers[1] = 255;
  registers[2] = 3;
  updateRegisters(registers);

  allOn = true;
  return false;
} // End turnEmAllOn()

void circuit::toggleLED( int whichLED ) {
  if ( whichLED < 0 || whichLED >= NUM_CHANNELS )
    return;

  if ( allOn )
    turnEmOff();

  if ( registers[ (int)(whichLED/8) ] & (1<<(whichLED%8)) )
    registers[ (int)(whichLED/8) ] -= (1<<(whichLED%8));
  else
    registers[ (int)(whichLED/8) ] += (1<<(whichLED%8));  
    
  updateRegisters(registers);

  return;
}

void circuit::setRegisters( char input[INPUT_SIZE+2] ) {

  // Copy the input to a local variable
  char local[INPUT_SIZE+1];
  strcpy( local, input );

  // otherwise strtok will destroy the original
  char *pChar = strtok( local, " ," );

  // Loop over the input tokens
  while ( pChar != NULL ) {

    // Extract the number from each token 
    int ledNum = atoi(pChar);

    // And set the proper bit in registers[]
    if ( ledNum >= 0 && ledNum < 8 )
      bitSet( registers[0], ledNum );
    else if ( ledNum < 16 )
      bitSet( registers[1], ledNum-8 );
    else if ( ledNum < 18 )
      bitSet( registers[2], ledNum-16 );

    // Get the next token, for another ride
    // (unless you have an E ticket)
    pChar = strtok(NULL, " ,");

  } // End while ( pChar != NULL )
  
  return;
} // End void setRegisters( char input[INPUT_SIZE+2] )

void circuit::getDiodeNoise(void) {
  Snoise = Lnoise = 0.0f;

  // Read once & discard to flush out the diodes
  analogRead(largeDiodePin);
  delay(5);
  analogRead(smallDiodePin);
  delay(10);

  // Now, read for reals
  for ( int j=0; j<NUM_SAMPLES; j++ ) {
    Lnoise += analogRead(largeDiodePin); // Arduino hardware issue. Multiple analog reads 
    delay(5);                            // give random results unless seperated by a few ms
    Snoise += analogRead(smallDiodePin); 

    // If someone pushed the panic button, stop RIGHT NOW!
    if ( CRASH_STOP || FATAL_ERROR )
      return;
  }
  Lnoise /= NUM_SAMPLES;
  Snoise /= NUM_SAMPLES;

  return;
}

void circuit::sequence( void ) {

  // Read in the diodes without the LEDs and average the noise
  // But only one time per connector
#ifndef TEST
  if ( SUBTRACT_NOISE )
    getDiodeNoise();
#else
  Lnoise = Snoise = 0.0f;
#endif

  if ( CRASH_STOP || FATAL_ERROR )
    return;

  char msg[40];
  int ran = -1.0;
  
  for ( int i=0; i<NUM_CHANNELS; i++ ) {
    if ( normalization[i] <= 0.0f ) {
      Serial.println(F("Unknown normalization!"));  
      normalization[i] = 1.0;
    }

    ran = random(1000,9999);
    sprintf(msg, "c1 %i LED %i",ran, i);
    Serial.println(msg);

    // Initialize the time counter
    elapsedMillis timeElapsed = 0;
    
    /*** Turn the LED on ***/
    registers[ (int)(i/8) ] = (1<<(i%8));  
    updateRegisters(registers);
    
    int n = 0;
    float Smean = 0.0, Sm2 = 0.0, Sdelta;
    float Lmean = 0.0, Lm2 = 0.0, Ldelta;

#ifndef TEST
    // Read once & discard to flush out the diodes
    analogRead(largeDiodePin);
    delay(5);
    analogRead(smallDiodePin);
    delay(5);
#else
    int small = random( 700,800 );
    delay(5);
    int large = normalization[i] * random(0.667*small, 0.90*small);
    delay(5);
#endif

    /*** Turn the LED off ...
    registers[ (int)(i/8) ] = 0;
    updateRegisters(registers);
    delay(5);
    ***/
    // Now, read for reals
    for ( int j=0; j<NUM_SAMPLES; j++ ) {

      /***Turn the LED on ...
      registers[ (int)(i/8) ] = (1<<(i%8));  
      updateRegisters(registers);
      ***/
#ifndef TEST
      int large = normalization[i] * (analogRead(largeDiodePin) - Lnoise);
      delay(5);
      int small = analogRead(smallDiodePin) - Snoise;  // As above, so below.
      delay(5);
#else
      int small = random( 700,800 );
      delay(5);
      int large = normalization[i] * random(0.667*small, 0.90*small);
      delay(5);
#endif
      
      /*** Turn the LED off ...
      registers[ (int)(i/8) ] = 0;
      updateRegisters(registers);
      delay(5);
      ***/

      small = (small<0) ? 0 : small;
      large = (large<0) ? 0 : large;

      n++;
      
      Sdelta = small - Smean;
      Smean += Sdelta/n;
      Sm2   += Sdelta * (small-Smean);

      Ldelta = large - Lmean;
      Lmean += Ldelta/n;
      Lm2   += Ldelta * (large-Lmean);

      registers[0] = registers[1] = registers[2];

      // If someone pushed the panic button, stop RIGHT NOW!
      if ( CRASH_STOP || FATAL_ERROR )
        return;
    }
    turnEmOff();

    float Sstdev = sqrt(Sm2/(n-1));
    float Lstdev = sqrt(Lm2/(n-1));

    ran = random(1000,9999);
    sprintf(msg, "c2 %i PD %i.%i(%i.%i)/%i.%i(%i.%i)",ran,
        (int)Lmean,  (int)(100*(Lmean  - (int)Lmean)),
        (int)Lstdev, (int)(100*(Lstdev - (int)Lstdev)),
        (int)Smean,  (int)(100*(Smean  - (int)Smean)),
        (int)Sstdev, (int)(100*(Sstdev - (int)Sstdev))
    );

    // Pause a moment to give the Pi a chance to process the LED line
    uint temp = (timeElapsed>=LED_DELAY) ? 0 : LED_DELAY-timeElapsed;
    delay(temp);

    Serial.println(msg);

    // And again to allow it time to run the PD line through
    // before sending the next LED number
    delay(PD_DELAY);
    
  } // End for ( int i=0; i<NUM_CHANELS; i++ )

  /**** Refuse to go any further if there is lots of light in the box ***/
  uint lightLevel = analogRead(photoPin);
  if ( lightLevel > maxLightLevel ) {
    if ( !FATAL_ERROR ) {
      FATAL_ERROR = true;
      Serial.println(F("fe Fatal Error: Too much light in the box"));
      this->roxanne();
    }
  }
  else if ( FATAL_ERROR ) {
    FATAL_ERROR = false;
    Serial.println(F("ff Fatal Error: light leak cleared"));
    this->roxanne();
  }
  
  return;
} // End funciton sequence

void circuit::calibrate(void) {

  Serial.println(F("c3 Calibrating ... "));

  turnEmOff();
  int sampleSize = NUM_SAMPLES;
  float Smean = 0.0, Lmean = 0.0;

  // Read the diodes with no LEDs and store that as the noise in the system
  getDiodeNoise();
  
  // Read in the diodes and calculate the ratios
  for ( int i=0; i<NUM_CHANNELS; i++ ) {

    registers[ (int)(i/8) ] = (1<<(i%8));
    updateRegisters(registers);
    
    for ( int j=0; j<sampleSize; j++ ) {

#ifndef TEST
      int small = analogRead(smallDiodePin) - Snoise;
      delay(5);
      int large = analogRead(largeDiodePin) - Lnoise;
#else
      int small = random(700,800);
      delay(5);
      int large = random( 0.85*small, 0.95*small );
#endif
      small = (small<0) ? 0 : small;
      large = (large<0) ? 0 : large;

      Smean += small;
      Lmean += large;

      // If someone pushed the panic button, stop RIGHT NOW!
      if ( CRASH_STOP )
        return;

    } // End for ( int j=0; j<sampleSize; j++ ) 
    
    Smean /= sampleSize;
    Lmean /= sampleSize;

    if  (Smean != 0.0)
      normalization[i] = Smean / Lmean;
    else
      normalization[i] = 1.0;
      
    registers[0] = registers[1] = registers[2] = 0;
  
  } // End for ( int i=0; i<NUM_CHANNELS; i++ )

  turnEmOff();

  // write the calibration constants to EEPROM
  writeData();

  Serial.println(F("c4  Calibrating ... Done."));
  
  return;
} // End calibrate(void)

uint circuit::setSampleSize( int sample_size ) {

  if ( sample_size >= 8 && sample_size <= 2048 )
    NUM_SAMPLES = sample_size;
  return NUM_SAMPLES;
}

uint circuit::setNumLEDs( int number_of_leds ) {
  
  if ( number_of_leds >= 0 && number_of_leds <= 18)
    NUM_CHANNELS = number_of_leds;
  return NUM_CHANNELS;
  
}

// Set the per fiber delay in the flash+read sequencer routine
uint circuit::setDelay( uint delay ) {

  if ( delay < 6000 ) {
    DELAY     = delay;
    LED_DELAY = 0.5 * DELAY;
    PD_DELAY  = 0.5 * DELAY;
  }

  return DELAY;
}

bool circuit::setSubtract( bool subtract ) {
  SUBTRACT_NOISE = subtract;
  return SUBTRACT_NOISE;
}

void circuit::dumpNorms(void) {
  char msg[12] = {0};
  for(int i=0; i<NUM_CHANNELS; i++) {
    Serial.print(F("normalization["));
    sprintf(msg, "%i", i);
    Serial.print(msg);
    Serial.print(F("] = "));
    int a = (int)normalization[i];
    float b = normalization[i] - a;
    int c = 1000*b;
    sprintf(msg, "%i.%i", a,c);
    Serial.println(msg);
  }
  return;
}


/********************************* EEPROM Read/Write Functions ************************************/
void circuit::writeData(void) {
  int address;
  const byte size = sizeof(unsigned int);

  for(int i=0; i<NUM_CHANNELS; i++){
    address = size*i + NUM_CHANNELS*size + ADDRESS_OFFSET;
    unsigned int value = 1000 * normalization[i];
    EEPROMWriteInt(address, value);
  }

  return;
}

void circuit::readData(void) {
  int address;
  const byte size = sizeof(unsigned int);
  
  for(int i=0; i<NUM_CHANNELS; i++){
    address = size*i + NUM_CHANNELS*size + ADDRESS_OFFSET;
    unsigned int value = EEPROMReadInt(address);
    normalization[i] = 0.001 * value;
  }

  bool hollad = false;
  for ( int i=0; i<NUM_CHANNELS; i++ ) {
    if ( normalization[i] <= 0.0 ) {
      normalization[i] = 1.0f;
      
      if ( !hollad ) {
        Serial.println(F("ff Please run the calibration for the QCBot"));
        hollad = true;
      }
    }
  }
 
 return;  
}

//integer read/write functions found at http://forum.arduino.cc/index.php/topic,37470.0.html
//This function will write a 2 byte integer to the eeprom at the specified address and address + 1

void circuit::EEPROMWriteInt(int p_address, int p_value) {
  byte lowByte = ((p_value >> 0) & 0xFF);
  byte highByte = ((p_value >> 8) & 0xFF);

  EEPROM.write(p_address, lowByte);
  EEPROM.write(p_address + 1, highByte);
}

//This function will read a 2 byte integer from the eeprom at the specified address and address + 1
unsigned int circuit::EEPROMReadInt(int p_address) {
  byte lowByte = EEPROM.read(p_address); 
  byte highByte = EEPROM.read(p_address + 1);
  return ((lowByte << 0) & 0xFF) + ((highByte << 8)& 0xFF00);
}

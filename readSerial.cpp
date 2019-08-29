#include "readSerial.h"
 
readSerial::readSerial(void) {
  Serial.begin(BAUD);     // set up Serial library
  while (!Serial);        // And wait until the line is initialized

  // Allocate the memory for the command structure
  cmd = new command;

  // Initialize the input buffer & counters & flags
  memset( input, '\0', (INPUT_SIZE+2)*sizeof(char) );
  characters = 0;
  msgAvailable = false;

  return;
}

readSerial::~readSerial(void) {return;}

// Is there input on the line?
bool readSerial::isAvailable(void) {
  return Serial.available();
}

// Read in whatever is on the serial line up 
// to a \r\n or INPUT_SIZE, whichever comes first
void readSerial::read(void) {

  // If there's nothing there, there's nothing to do
  if ( !isAvailable() )
    return;

  // Check how much data is on the line
  int bytes = Serial.available();
  if ( bytes <= 0 )
    return;

  // Assign a buffer
  char buffer[bytes+3];

  // And read the line. The return is the number of bytes read
  int size = Serial.readBytes( buffer, bytes );
  if ( size <= 0 )
    return;

  characters += size;

  if ( characters < INPUT_SIZE )
    strcat(input, buffer);

  memset(buffer, '\0', sizeof(char)*(bytes+3));

  if ( input[characters-1] == '\n' || input[characters-1] == '\r' ) {
    input[characters] = '\0';
    msgAvailable = true;

  }
  else {
    msgAvailable = false;
    return;
  }

  // Save a copy of the input
  strcpy( cmd->input, input );

  // Strip off the first character... and
  cmd->operation = input[0];

  // Shift the rest of the string front and lose the spaces
  // (I don't know why, but just doing atof(input+1) doesn't work)
  uint i=0,j=0;
  for ( i=1; i<INPUT_SIZE; i++ ) {
    if ( input[i] != 32 )
      input[j++] = input[i];
    if ( input[i] == '\0' )
      break;
  }

  // Keep the number handy. We'll need it later
  cmd->steps = atof(input);

  // And we're done with the input buffer.
  memset( input, '\0', sizeof(char)*(INPUT_SIZE+2) );
  characters = 0;

  return;
}

command * readSerial::getCmdPtr(void) {
  return cmd;
}

// Zero out the last command read
void readSerial::flushCommand(void) {
  cmd->operation  = '\0';
  cmd->steps      = 0.0;
  cmd->input[0]   = '\0';
}

char * readSerial::getInput( void ) {
  return cmd->input;
}


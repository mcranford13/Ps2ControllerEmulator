#include <SPI.h>

#define ackPin 9
#define triPin 7
#define sqPin 6
#define oPin 5
#define exPin 4

/*
  Build Process

  1. Set up MCU as slave - check
  2. Set up MCU for interrupts, letting the ps2 handle SS and clock rate - check
  3. Collect data regarding the controller state
  4. When an interrupt occurs, transfer this data to the Ps2, byte by byte
  5.

*/

struct DATA { //Main data structure

  byte response[20];

  volatile size_t count;
  volatile size_t sendAck;
  size_t responseLength;


  bool configState;
  bool aboutToConfig;
  bool aboutToSwitchBetweenAnalogDigital;
  bool secondPass;
  bool modeLocked;
  bool isDigital;
  bool isAnalog;
  bool ssState;
  bool oldSsState;
  bool FourcSecondPass;
  bool aboutToSetMotorStates;
  bool smallMotorOn;

  long lastChange;

} data = {{0x41, 0x5A}, 0, 0, 5, false, false, false, false, false, true, false, false, false, false, false, false, 0};



void SlaveInit(void) {

  /*
     This function sets up the arduino in the correct configureation to communicate with the PS2.
  */

  pinMode(MOSI, INPUT);  //pin 11
  pinMode(MISO, OUTPUT); //pin 12
  pinMode(SCK, INPUT);   //pin 13
  pinMode(SS, INPUT);    //pin 10

  SPCR |= bit (SPE); //Init as slave
  SPCR |= 0x2C;      //Configure for LSB & correct SPI_Mode

  pinMode(ackPin, OUTPUT);
  digitalWrite(ackPin, LOW); //Because of external circuitry (open drain) need to flip this so it works appropriately

  pinMode(triPin, INPUT_PULLUP);
  pinMode(sqPin, INPUT_PULLUP);
  pinMode(oPin, INPUT_PULLUP);
  pinMode(exPin, INPUT_PULLUP);

  SPCR |= _BV(SPIE); //Attach Interrupts

}

void setup() {

  SlaveInit();



}


/*
   This function reads the states of the pins and returns the correct byte describing the state of the controller
*/
byte getControllerStateFirstByte() {
  byte data = 0xFF; //Buttons are ative low

  byte pinState = PIND; //digitalRead() is too slow, we need to read the pins directly from the register

  
  //The following are examples of reading the pins and correctly setting up the data to be transferred.
  
/*
  if (!(pinState & 0b00001000)) {

    data &= ~0x10; // up
  }

  
    if (!(pinState & 00000010)) {


    data &= ~0x40; //down
    }
  */
  
  return data;
}

byte getControllerStateSecondByte() {
  /*
     This function reads the states of the pins and
     sets up the byte describing the state of the controller appropriately
  */
  byte pinState = PIND;

  byte data = 0xFF;
  
  //The following are examples of reading the pins and correctly setting up the data to be transferred.

  /*
  if (!(pinState & 0b10000000)) {

    data &= ~0x10; //triangle
  }

  if (!(pinState & 0b01000000)) {

    data &= ~0x40; // X
  }


  if (!(pinState & 0b00100000)) {

    data &= ~0x20; // O
  }

  if (!(pinState & 0b00010000)) {

    data &= ~0x80; //Square
    
  }


  */

  return data; //Buttons are active low in the bus, so we invert all the bits
}



// SPI interrupt routine
ISR (SPI_STC_vect) {


  //By the time we reach this routine, we have already transferred the first byte i.e CMD: 0x01, Data: 0xFF, aka the first byte of the header has already been sent
  byte cmd = SPDR; //Read incomming command from ps2

  switch (cmd) { //Main logic handling of the cmds of the ps2

    //See http://store.curiousinventor.com/guides/PS2/ for more information about these commands.

    case 0x41:

      data.responseLength = 9;

      if (data.configState && data.isAnalog) {

        data.response[0] = 0x79;
        data.response[1] = 0x5a;
        data.response[2] = 0x00;
        data.response[3] = 0x00;
        data.response[4] = 0x03;
        data.response[5] = 0x00;
        data.response[6] = 0x00;
        data.response[7] = 0x5A; //last byte is always 0x5A

      } else if (data.configState && data.isDigital) {

        data.response[0] = 0x41;
        data.response[1] = 0x5A;
        data.response[2] = 0xFF;
        data.response[3] = 0xFF;
        data.response[4] = 0x00;
        data.response[5] = 0x00;
        data.response[6] = 0x00;
        data.response[7] = 0x5A; //last byte is always 0x5A

      }
      break;


    case 0x42:


      if (data.isDigital) {

        data.responseLength = 5;

        data.response[0] = 0x41;

      }

      else if (data.isAnalog) {

        data.responseLength = 9;

        data.response[0] = 0x79;
        data.response[4] = 0x7F;
        data.response[5] = 0x7F;
        data.response[6] = 0x7F;
        data.response[7] = 0x7F;
        data.response[8] = 0x7F;

      }

      /*
         TODO: handle rumble here
      */
      data.response[1] = 0x5A;
      data.response[2] = getControllerStateFirstByte();
      data.response[3] = getControllerStateSecondByte(); //This is the 5th byte

      break;

    case 0x43: // enter/exit config mode

      data.aboutToConfig = true;
      data.responseLength = 9;

      if (data.isDigital) {
        data.responseLength = 5;
        data.response[0] = 0x41;

      }

      else if (data.isAnalog) {


        data.response[0] = 0x79; //analog mode

        if (!(data.configState)) {
          data.response[4] = 0x7F; //analog sticks
          data.response[5] = 0x7F;
          data.response[6] = 0x7F;
          data.response[7] = 0x7F;
        }
      }

      if (!(data.configState)) {
        data.response[2] = getControllerStateFirstByte();
        data.response[3] = getControllerStateSecondByte();

        //No vibration motor control necessary
      }

      break;


    case 0x44:

      data.responseLength = 9;

      if (data.configState) { //Only works if we are in config mode

        data.aboutToSwitchBetweenAnalogDigital = true;
        data.response[0] = 0xF3; //Stating that we are indeed in config mode
        data.response[1] = 0x5A;


      }

      break;


    case 0x45:

      data.responseLength = 9;

      // if (data.configState) { //For some reason this causes glitches though all documentation says we should be in config mode

      data.response[0] = 0xF3;
      data.response[1] = 0x5A;
      data.response[2] = 0x03;
      data.response[3] = 0x02;
      data.response[4] = 0x00; //0x01 if LED is on, possibly a way to switch between analog & digtial?
      data.response[5] = 0x02;
      data.response[6] = 0x01;
      data.response[7] = 0x00;
      // }

      break;

    case 0x46:

      data.responseLength = 9;

      if (data.configState && data.secondPass == false) { //Only works if we are in config mode

        data.response[0] = 0xF3;
        data.response[1] = 0x5A;
        data.response[3] = 0x00;
        data.response[4] = 0x00;
        data.response[5] = 0x02;
        data.response[6] = 0x00;
        data.response[7] = 0x0A;

        data.secondPass = true;

      }

      break;

    case 0x47:

      data.responseLength = 9;

      if (data.configState) {

        data.response[0] = 0xF3;
        data.response[1] = 0x5A;
        data.response[3] = 0x00;
        data.response[4] = 0x02;
        data.response[5] = 0x00;
        data.response[6] = 0x00;
        data.response[7] = 0x0A;
      }

      break;


    case 0x4C:

      data.responseLength = 9;

      if (data.configState && data.FourcSecondPass == false) {

        data.response[0] = 0xF3;
        data.response[1] = 0x5A;
        data.response[2] = 0x00;
        data.response[3] = 0x00;
        data.response[4] = 0x00;
        data.response[5] = 0x04;
        data.response[6] = 0x00;
        data.response[7] = 0x00;

        data.FourcSecondPass = true;

      }

      break;

    case 0x4D:

      //Not implemented

      break;

    case 0x4F:

      //Not implemented

      break;


    case 0x00: //Special commands after recieving a specific command

      if (data.count == 2) { //This jumps over the end of the header where the cmd would be 0x00
        
        if (data.aboutToConfig) {
          
          data.configState = false;
          data.response[0] = 0x41;
          data.aboutToConfig = false;
          
        }


        else if (data.aboutToSwitchBetweenAnalogDigital && data.modeLocked == false) {

          data.isDigital = true;
          data.isAnalog = false;
          data.response[0] = 0x41;

          data.aboutToSwitchBetweenAnalogDigital = false;
        }

        else if (data.aboutToSetMotorStates) {

          data.smallMotorOn = true;

        }

      }

      break;

    case 0x01: //Special commands after recieving a specific command

      if (data.count == 2) { //this jumps over the header where cmd would be 0x01, may not need this

        if (data.aboutToConfig) {

          data.configState = true;
          data.response[0] = 0xF3;
          data.aboutToConfig = false;

        }


        else if (data.aboutToSwitchBetweenAnalogDigital && data.modeLocked == false) {

          data.isDigital = false;
          data.isAnalog = true;

          data.response[0] = 0xF3;

          data.aboutToSwitchBetweenAnalogDigital = false;

        }

        else if (data.configState && data.secondPass) { //Only works if we are in config mode

          data.response[0] = 0xF3;
          data.response[1] = 0x5A;
          data.response[3] = 0x00;
          data.response[4] = 0x00;
          data.response[5] = 0x00;
          data.response[6] = 0x00;
          data.response[7] = 0x14;

        }
        else if (data.configState && data.FourcSecondPass) { //Only works if we are in config mode

          data.response[0] = 0xF3;
          data.response[1] = 0x5A;
          data.response[3] = 0x00;
          data.response[4] = 0x00;
          data.response[5] = 0x06;
          data.response[6] = 0x00;
          data.response[7] = 0x00;

        }
      }

      break;

    case 0x03: //Lock Analog/Digital

      data.modeLocked = true;
      data.response[4] = 0x00;
      data.response[5] = 0x00;
      data.response[6] = 0x00;
      data.response[7] = 0x00;

      data.aboutToSwitchBetweenAnalogDigital = false;

      break;


  }// end switch


  SPDR = data.response[data.count]; //Put the data into the registers to be transferred during this interuupt


  if (data.count < (data.responseLength - 1)) { //only increase our counter until we are at the end of the needed response

    data.count++;
  }

  data.sendAck = 1;

}


void loop() {


  //Send ack after each bit transfer
  if (data.sendAck) {

    digitalWrite(ackPin, HIGH); //Again in opposite configuration due to external open drain circuitry
    delayMicroseconds(4);
    digitalWrite(ackPin, LOW);
    data.sendAck = 0;
  }

  //Debouce the SS line

  /*
     We debounce this line so that our count is reset appropriately when we aren't communicating
  */
  data.ssState = digitalRead(SS);

  if (data.ssState == HIGH) { //Reset where we are when we aren't communicating

    if (millis() - data.lastChange > 10) { //State is valid
      data.count = 0;
      data.responseLength = 0;


    }
  }
  if (data.ssState != data.oldSsState) {

    data.lastChange = millis();
  }

  data.oldSsState = data.ssState;


} //end loop()





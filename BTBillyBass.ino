/*This is my crack at a state-based approach to automating a Big Mouth Billy Bass.
 This code was built on work done by both Donald Bell and github user jswett77. 
 See links below for more information on their previous work.

 In this code you'll find reference to the MX1508 library, which is a simple 
 library I wrote to interface with the extremely cheap 2-channel H-bridges that
 use the MX1508 driver chip. It may also work with other H-bridges that use different
 chips (such as the L298N), so long as you can PWM the inputs.

 This code watches for a voltage increase on input A0, and when sound rises above a
 set threshold it opens the mouth of the fish. When the voltage falls below the threshold,
 the mouth closes.The result is the appearance of the mouth "riding the wave" of audio
 amplitude, and reacting to each voltage spike by opening again. There is also some code
 which adds body movements for a bit more personality while talking.

 Most of this work was based on the code written by jswett77, and can be found here:
 https://github.com/jswett77/big_mouth/blob/master/billy.ino

 Donald Bell wrote the initial code for getting a Billy Bass to react to audio input,
 and his project can be found on Instructables here:
 https://www.instructables.com/id/Animate-a-Billy-Bass-Mouth-With-Any-Audio-Source/

 Author: Jordan Bunker <jordan@hierotechnics.com> 2019
 License: MIT License (https://opensource.org/licenses/MIT)
*/

#include "MX1508.h"

MX1508 bodyMotor(6, 9); // Sets up an MX1508 controlled motor on PWM pins 6 and 9
MX1508 mouthMotor(3, 5); // Sets up an MX1508 controlled motor on PWM pins 5 and 3


int soundPin = A0; // Sound input
int buttonPin = 0;
int resistancePin = A3;

bool DEBUG_VOLUME = true;
bool DEBUG_STATE = true;
bool DEBUG_MOUTH = false;
bool DEBUG_RESISTANCE = false;

int silence = 150; // Threshold for "silence". Anything below this level is ignored.
int bodySpeed = 0; // body motor speed initialized to 0
int soundVolume = 0; // variable to hold the analog audio value
int fishState = 1; // variable to indicate the state Billy is in

int mouthOpenDuration = 400; // milliseconds
int fishBoredomDuration = 1500; // milliseconds

bool talking = true; //indicates whether the fish should be talking or not

//these variables are for storing the current time, scheduling times for actions to end, and when the action took place
long currentTime;
long mouthActionEndTime = 0;
long bodyActionEndTime = 0;
long lastActionTime = 0;

int resistanceValue = 0; // potentiometer for scaling volume to silence

void setup() {
//make sure both motor speeds are set to zero
  bodyMotor.setSpeed(0); 
  mouthMotor.setSpeed(0);

//input mode for sound pin
  pinMode(soundPin, INPUT);

// input mode for button pin
  pinMode(buttonPin, INPUT);

  Serial.begin(9600);
}

void loop() {
  currentTime = millis(); //updates the time each time the loop is run
  
  readResistanceValue();
  updateSoundInput(); //updates the volume level detected
    
  SMBillyBass(); //this is the switch/case statement to control the state of the fish
  //buttonDrivenMouth();
}

void readResistanceValue() {  
  resistanceValue = analogRead(resistancePin);
  if (DEBUG_RESISTANCE) {
      Serial.print("R=");
      Serial.println(resistanceValue);
  }
}
 
bool detectedSound() {
  //return soundVolume > silence;
  return digitalRead(buttonPin);
} 

void buttonDrivenMouth() {
  //Serial.println(".");
  int buttonValue = digitalRead(buttonPin);
  //Serial.println(buttonValue);
  if (buttonValue) {
    openMouth();
  } else {
    closeMouth();
  }
}

void SMBillyBass() {
  if (DEBUG_STATE) {
      Serial.print("          S=");
      Serial.println(fishState);
  }

  
  switch (fishState) {
    case 0: // WAITING
      if (detectedSound()) { //if we detect audio input above the threshold
        if (currentTime > mouthActionEndTime) { //and if we haven't yet scheduled a mouth movement
          talking = true; //  set talking to true and schedule the mouth movement action
          mouthActionEndTime = currentTime + mouthOpenDuration;
          fishState = 1; // jump to a talking state
        }
      } else { // Sound is too quiet to be currently talking
         if (currentTime > mouthActionEndTime) { //if we're beyond the scheduled talking time, halt the motors
          bodyMotor.halt();
          //mouthMotor.halt();
          closeMouth();
        }
      }
      if (currentTime - lastActionTime > fishBoredomDuration) { //if Billy hasn't done anything in a while, we need to show he's bored
        lastActionTime = currentTime + floor(random(30, 60)) * 1000L; //you can adjust the numbers here to change how often he flaps
        fishState = 2; //jump to a flapping state!
      }
      break;

    case 1: //TALKING until mouthActionEndTime
      if (currentTime < mouthActionEndTime) { //if we have a scheduled mouthActionEndTime in the future....
        if (talking) { // and if we think we should be talking
          openMouth(); // then open the mouth and articulate the body
          lastActionTime = currentTime;
          articulateBody(true);
        }
      } else { // otherwise, close the mouth, don't articulate the body, and set talking to false
        Serial.println("mouthActionEndTime passed");
        closeMouth();
        articulateBody(false);
        talking = false;
        fishState = 0; //jump back to waiting state
      }
      break;

    case 2: //GOTTA FLAP!
      Serial.println("I'm bored. Gotta flap.");
      flap();
      fishState = 0;
      break;
  }
}

int updateSoundInput() {
  soundVolume = analogRead(soundPin);
  Serial.print("V=");
  Serial.println(soundVolume);

  if (DEBUG_VOLUME) {
    if (soundVolume > silence) {
      Serial.println(soundVolume);
    }
  }
}

void openMouth() {
  if (DEBUG_MOUTH) {
      Serial.println("Open mouth");
  }  
  mouthMotor.halt(); //stop the mouth motor
  mouthMotor.setSpeed(220); //set the mouth motor speed
  mouthMotor.forward(); //open the mouth
}

void closeMouth() {
  if (DEBUG_MOUTH) {
      Serial.println("Close mouth");
  }  
  mouthMotor.halt(); //stop the mouth motor
  mouthMotor.setSpeed(180); //set the mouth motor speed
  mouthMotor.backward(); // close the mouth
}

void articulateBody(bool talking) { //function for articulating the body
  if (talking) { //if Billy is talking
    if (currentTime > bodyActionEndTime) { // and if we don't have a scheduled body movement
      int r = floor(random(0, 8)); // create a random number between 0 and 7)
      if (r < 1) {
        bodySpeed = 0; // don't move the body
        bodyActionEndTime = currentTime + floor(random(500, 1000)); //schedule body action for .5 to 1 seconds from current time
        bodyMotor.forward(); //move the body motor to raise the head

      } else if (r < 3) {
        bodySpeed = 150; //move the body slowly
        bodyActionEndTime = currentTime + floor(random(500, 1000)); //schedule body action for .5 to 1 seconds from current time
        bodyMotor.forward(); //move the body motor to raise the head

      } else if (r == 4) {
        bodySpeed = 200;  // move the body medium speed
        bodyActionEndTime = currentTime + floor(random(500, 1000)); //schedule body action for .5 to 1 seconds from current time
        bodyMotor.forward(); //move the body motor to raise the head

      } else if ( r == 5 ) {
        bodySpeed = 0; //set body motor speed to 0
        bodyMotor.halt(); //stop the body motor (to keep from violent sudden direction changes)
        bodyMotor.setSpeed(255); //set the body motor to full speed
        bodyMotor.backward(); //move the body motor to raise the tail
        bodyActionEndTime = currentTime + floor(random(900, 1200)); //schedule body action for .9 to 1.2 seconds from current time
      }
      else {
        bodySpeed = 255; // move the body full speed
        bodyMotor.forward(); //move the body motor to raise the head
        bodyActionEndTime = currentTime + floor(random(1500, 3000)); //schedule action time for 1.5 to 3.0 seconds from current time
      }
    }

    bodyMotor.setSpeed(bodySpeed); //set the body motor speed
  } else {
    if (currentTime > bodyActionEndTime) { //if we're beyond the scheduled body action time
      bodyMotor.halt(); //stop the body motor
      bodyActionEndTime = currentTime + floor(random(20, 50)); //set the next scheduled body action to current time plus .02 to .05 seconds
    }
  }
}


void flap() {
  bodyMotor.setSpeed(180); //set the body motor to full speed
  bodyMotor.backward(); //move the body motor to raise the tail
  delay(500); //wait a bit, for dramatic effect
  bodyMotor.halt(); //halt the motor
}

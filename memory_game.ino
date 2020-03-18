#include <Arduino.h>
#include <Button.h>
/*
Fredericton MakerSpace Memory Game Relay Control Sketch
Based on SparkFun Inventor's Kit, Example sketch 16
https://learn.sparkfun.com/tutorials/sik-experiment-guide-for-arduino---v33/all#experiment-16-simon-says
*/

/*************************************************
* Public Constants
*************************************************/
#include "pitches.h"

/*************************************************
* Game Constants
*************************************************/
#define CHOICE_OFF 0  //Used to control LEDs
#define CHOICE_NONE 0 //Used to check buttons
#define CHOICE_RED (1 << 0)
#define CHOICE_GREEN (1 << 1)
#define CHOICE_BLUE (1 << 2)
#define CHOICE_YELLOW (1 << 3)

#define LED_RED 8
#define LED_GREEN 9
#define LED_BLUE 11
#define LED_YELLOW 10

// Button pin definitions
#define BUTTON_RED 2
#define BUTTON_GREEN 3
#define BUTTON_BLUE 5
#define BUTTON_YELLOW 4

// Buzzer pin definitions
#define BUZZER1 6
#define BUZZER2 7

// Define game parameters
#define ROUNDS_TO_WIN 13      //Number of rounds to succesfully remember before you win. 13 is do-able.
#define ENTRY_TIME_LIMIT 3000 //Amount of time to press a button before game times out. 3000ms = 3 sec

#define MODE_MEMORY 0
#define MODE_BATTLE 1
#define MODE_BEEGEES 2

#define DEBOUNCE 10

// Game state variables
byte gameMode = MODE_MEMORY; //By default, let's play the memory game
byte gameBoard[32];          //Contains the combination of buttons as we advance
byte gameRound = 0;          //Counts the number of succesful rounds the player has made it through

// Button variables
Button button_red = Button(BUTTON_RED, true, true, DEBOUNCE);
Button button_green = Button(BUTTON_GREEN, true, true, DEBOUNCE);
Button button_blue = Button(BUTTON_BLUE, true, true, DEBOUNCE);
Button button_yellow = Button(BUTTON_YELLOW, true, true, DEBOUNCE);

Button buttons[] = {
    button_red, button_green, button_blue, button_yellow};

int button_index[] = {
    BUTTON_RED,
    BUTTON_GREEN,
    BUTTON_BLUE,
    BUTTON_YELLOW};

#define NUMBUTTONS 4

void read_buttons()
{
  for (int i = 0; i < NUMBUTTONS; i++)
  {
    buttons[i].read();
  }
}

void setup()
{
  Serial.begin(9600);

  //Setup hardware inputs/outputs. These pins are defined in the hardware_versions header file
  //Enable pull ups on inputs
  // pinMode(BUTTON_RED, INPUT_PULLUP);
  // pinMode(BUTTON_GREEN, INPUT_PULLUP);
  // pinMode(BUTTON_BLUE, INPUT_PULLUP);
  // pinMode(BUTTON_YELLOW, INPUT_PULLUP);

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);

  //turn the relays off
  digitalWrite(LED_RED, LOW); // the hardware here is a HIGH trigger relay, the others are not.
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);
  digitalWrite(LED_YELLOW, HIGH);

  pinMode(BUZZER1, OUTPUT);
  pinMode(BUZZER2, OUTPUT);

  //Mode checking
  gameMode = MODE_MEMORY; // By default, we're going to play the memory game

  // Check to see if the lower right button is pressed
  if (old_checkButton() == CHOICE_YELLOW)
    play_beegees();

  if (old_checkButton() == CHOICE_BLUE)
    play_starwars();

  if (old_checkButton() == CHOICE_RED)
    button_test();

  // Check to see if upper right button is pressed
  if (old_checkButton() == CHOICE_GREEN)
  {
    gameMode = MODE_BATTLE; //Put game into battle mode

    //Turn on the upper right (green) LED
    setLEDs(CHOICE_GREEN);
    toner(CHOICE_GREEN, 150);

    setLEDs(CHOICE_RED | CHOICE_BLUE | CHOICE_YELLOW); // Turn on the other LEDs until you release button

    while (old_checkButton() != CHOICE_NONE)
      ; // Wait for user to stop pressing button

    //Now do nothing. Battle mode will be serviced in the main routine
  }

  play_winner(); // After setup is complete, say hello to the world
}

void button_test()
{
  while (1)
  {
    byte button = checkButton();
    if (button == CHOICE_NONE)
      continue;

    setLEDs(button);
    delay(500);

    setLEDs(CHOICE_OFF); // Turn off LEDs
  }
}

void loop()
{

  attractMode(); // Blink lights while waiting for user to press a button

  // Indicate the start of game play
  setLEDs(CHOICE_RED | CHOICE_GREEN | CHOICE_BLUE | CHOICE_YELLOW); // Turn all LEDs on
  delay(1000);
  setLEDs(CHOICE_OFF); // Turn off LEDs
  delay(250);

  if (gameMode == MODE_MEMORY)
  {
    // Play memory game and handle result
    if (play_memory() == true)
      play_winner(); // Player won, play winner tones
    else
      play_loser(); // Player lost, play loser tones
  }

  if (gameMode == MODE_BATTLE)
  {
    play_battle(); // Play game until someone loses

    play_loser(); // Player lost, play loser tones
  }
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//The following functions are related to game play only

// Play the regular memory game
// Returns 0 if player loses, or 1 if player wins
boolean play_memory(void)
{
  randomSeed(millis()); // Seed the random generator with random amount of millis()

  gameRound = 0; // Reset the game to the beginning

  while (gameRound < ROUNDS_TO_WIN)
  {
    add_to_moves(); // Add a button to the current moves, then play them back

    playMoves(); // Play back the current game board

    // Then require the player to repeat the sequence.
    for (byte currentMove = 0; currentMove < gameRound; currentMove++)
    {
      byte choice = wait_for_button(); // See what button the user presses

      if (choice == 0)
        return false; // If wait timed out, player loses

      if (choice != gameBoard[currentMove])
        return false; // If the choice is incorect, player loses
    }

    delay(1000); // Player was correct, delay before playing moves
  }

  return true; // Player made it through all the rounds to win!
}

// Play the special 2 player battle mode
// A player begins by pressing a button then handing it to the other player
// That player repeats the button and adds one, then passes back.
// This function returns when someone loses
boolean play_battle(void)
{
  gameRound = 0; // Reset the game frame back to one frame

  while (1) // Loop until someone fails
  {
    byte newButton = wait_for_button(); // Wait for user to input next move
    gameBoard[gameRound++] = newButton; // Add this new button to the game array

    // Then require the player to repeat the sequence.
    for (byte currentMove = 0; currentMove < gameRound; currentMove++)
    {
      byte choice = wait_for_button();

      if (choice == 0)
        return false; // If wait timed out, player loses.

      if (choice != gameBoard[currentMove])
        return false; // If the choice is incorect, player loses.
    }

    delay(100); // Give the user an extra 100ms to hand the game to the other player
  }

  return true; // We should never get here
}

// Plays the current contents of the game moves
void playMoves(void)
{
  for (byte currentMove = 0; currentMove < gameRound; currentMove++)
  {
    toner(gameBoard[currentMove], 150);

    // Wait some amount of time between button playback
    // Shorten this to make game harder
    delay(150); // 150 works well. 75 gets fast.
  }
}

// Adds a new random button to the game sequence, by sampling the timer
void add_to_moves(void)
{
  byte newButton = random(0, 4); //min (included), max (exluded)

  // We have to convert this number, 0 to 3, to CHOICEs
  if (newButton == 0)
    newButton = CHOICE_RED;
  else if (newButton == 1)
    newButton = CHOICE_GREEN;
  else if (newButton == 2)
    newButton = CHOICE_BLUE;
  else if (newButton == 3)
    newButton = CHOICE_YELLOW;

  gameBoard[gameRound++] = newButton; // Add this new button to the game array
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//The following functions control the hardware

// Lights a given LEDs
// Pass in a byte that is made up from CHOICE_RED, CHOICE_YELLOW, etc
void setLEDs(byte leds)
{
  if ((leds & CHOICE_RED) != 0)
    digitalWrite(LED_RED, HIGH);
  else
    digitalWrite(LED_RED, LOW);

  if ((leds & CHOICE_GREEN) != 0)
    digitalWrite(LED_GREEN, LOW);
  else
    digitalWrite(LED_GREEN, HIGH);

  if ((leds & CHOICE_BLUE) != 0)
    digitalWrite(LED_BLUE, LOW);
  else
    digitalWrite(LED_BLUE, HIGH);

  if ((leds & CHOICE_YELLOW) != 0)
    digitalWrite(LED_YELLOW, LOW);
  else
    digitalWrite(LED_YELLOW, HIGH);
}

// Wait for a button to be pressed.
// Returns one of LED colors (LED_RED, etc.) if successful, 0 if timed out
byte wait_for_button(void)
{
  long startTime = millis(); // Remember the time we started the this loop

  while ((millis() - startTime) < ENTRY_TIME_LIMIT) // Loop until too much time has passed
  {
    byte button = checkButton();

    if (button != CHOICE_NONE)
    {
      toner(button, 150); // Play the button the user just pressed

      while (checkButton() != CHOICE_NONE)
        ; // Now let's wait for user to release button

      delay(10); // This helps with debouncing and accidental double taps

      return button;
    }
  }

  return CHOICE_NONE; // If we get here, we've timed out!
}

byte old_checkButton(void)
{
  if (digitalRead(BUTTON_RED) == 0)
    return (CHOICE_RED);
  else if (digitalRead(BUTTON_GREEN) == 0)
    return (CHOICE_GREEN);
  else if (digitalRead(BUTTON_BLUE) == 0)
    return (CHOICE_BLUE);
  else if (digitalRead(BUTTON_YELLOW) == 0)
    return (CHOICE_YELLOW);

  return (CHOICE_NONE); // If no button is pressed, return none
}

byte checkButton(void)
{
  read_buttons();
  
  //get the button
  int button_idx = -1;
  for (int i = 0; i < NUMBUTTONS; i++)
  {
    if (buttons[i].wasPressed())
    {
      Serial.print("Button ");
      Serial.print(i);
      Serial.println(" was pressed.");
      button_idx = i;
      break;
    }
  }

  if (button_idx == -1)
    return (CHOICE_NONE);

  switch (button_index[button_idx])
  {
  case BUTTON_RED:
    return (CHOICE_RED);
  case BUTTON_GREEN:
    return (CHOICE_GREEN);
  case BUTTON_BLUE:
    return (CHOICE_BLUE);
  case BUTTON_YELLOW:
    return (CHOICE_YELLOW);
  }
}

// Light an LED and play tone
// Red, upper left:     440Hz - 2.272ms - 1.136ms pulse
// Green, upper right:  880Hz - 1.136ms - 0.568ms pulse
// Blue, lower left:    587.33Hz - 1.702ms - 0.851ms pulse
// Yellow, lower right: 784Hz - 1.276ms - 0.638ms pulse
void toner(byte which, int buzz_length_ms)
{
  setLEDs(which); //Turn on a given LED

  //Play the sound associated with the given LED
  switch (which)
  {
  case CHOICE_RED:
    buzz_sound(buzz_length_ms, 1136);
    break;
  case CHOICE_GREEN:
    buzz_sound(buzz_length_ms, 568);
    break;
  case CHOICE_BLUE:
    buzz_sound(buzz_length_ms, 851);
    break;
  case CHOICE_YELLOW:
    buzz_sound(buzz_length_ms, 638);
    break;
  }

  setLEDs(CHOICE_OFF); // Turn off all LEDs
}

// Toggle buzzer every buzz_delay_us, for a duration of buzz_length_ms.
void buzz_sound(int buzz_length_ms, int buzz_delay_us)
{
  // Convert total play time from milliseconds to microseconds
  long buzz_length_us = buzz_length_ms * (long)1000;

  // Loop until the remaining play time is less than a single buzz_delay_us
  while (buzz_length_us > (buzz_delay_us * 2))
  {
    buzz_length_us -= buzz_delay_us * 2; //Decrease the remaining play time

    // Toggle the buzzer at various speeds
    digitalWrite(BUZZER1, LOW);
    digitalWrite(BUZZER2, HIGH);
    delayMicroseconds(buzz_delay_us);

    digitalWrite(BUZZER1, HIGH);
    digitalWrite(BUZZER2, LOW);
    delayMicroseconds(buzz_delay_us);
  }
}

// Play the winner sound and lights
void play_winner(void)
{
  setLEDs(CHOICE_GREEN | CHOICE_BLUE);
  winner_sound();
  setLEDs(CHOICE_RED | CHOICE_YELLOW);
  winner_sound();
  setLEDs(CHOICE_GREEN | CHOICE_BLUE);
  winner_sound();
  setLEDs(CHOICE_RED | CHOICE_YELLOW);
  winner_sound();
}

// Play the winner sound
// This is just a unique (annoying) sound we came up with, there is no magic to it
void winner_sound(void)
{
  // Toggle the buzzer at various speeds
  for (byte x = 250; x > 70; x--)
  {
    for (byte y = 0; y < 3; y++)
    {
      digitalWrite(BUZZER2, HIGH);
      digitalWrite(BUZZER1, LOW);
      delayMicroseconds(x);

      digitalWrite(BUZZER2, LOW);
      digitalWrite(BUZZER1, HIGH);
      delayMicroseconds(x);
    }
  }
}

// Play the loser sound/lights
void play_loser(void)
{
  setLEDs(CHOICE_RED | CHOICE_GREEN);
  buzz_sound(255, 1500);

  setLEDs(CHOICE_BLUE | CHOICE_YELLOW);
  buzz_sound(255, 1500);

  setLEDs(CHOICE_RED | CHOICE_GREEN);
  buzz_sound(255, 1500);

  setLEDs(CHOICE_BLUE | CHOICE_YELLOW);
  buzz_sound(255, 1500);
}

const long attractModeDelay = 1000;

// Show an "attract mode" display while waiting for user to press button.
void attractMode(void)
{
  unsigned long previousMillis = 0;

  Serial.println("attract mode");
  byte choice = CHOICE_NONE;

  while (1)
  {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= attractModeDelay)
    {
      Serial.println(currentMillis);
      // save the last time you blinked
      previousMillis = currentMillis;

      if (choice == CHOICE_NONE)
        choice = CHOICE_RED;

      setLEDs(choice);
      switch (choice)
      {
      case CHOICE_RED:
        choice = CHOICE_BLUE;
        break;
      case CHOICE_BLUE:
        choice = CHOICE_GREEN;
        break;
      case CHOICE_GREEN:
        choice = CHOICE_YELLOW;
        break;
      case CHOICE_YELLOW:
        choice = CHOICE_RED;
        break;
      }
    }

    if (checkButton() != CHOICE_NONE)
      return;
  }
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// The following functions are related to Beegees Easter Egg only

// Notes in the melody. Each note is about an 1/8th note, "0"s are rests.
int melody[] = {
    NOTE_G4, NOTE_A4, 0, NOTE_C5, 0, 0, NOTE_G4, 0, 0, 0,
    NOTE_E4, 0, NOTE_D4, NOTE_E4, NOTE_G4, 0,
    NOTE_D4, NOTE_E4, 0, NOTE_G4, 0, 0,
    NOTE_D4, 0, NOTE_E4, 0, NOTE_G4, 0, NOTE_A4, 0, NOTE_C5, 0};

int noteDuration = 115; // This essentially sets the tempo, 115 is just about right for a disco groove :)
int LEDnumber = 0;      // Keeps track of which LED we are on during the beegees loop

// Do nothing but play bad beegees music
// This function is activated when user holds bottom right button during power up
void play_beegees()
{
  //Turn on the bottom right (yellow) LED
  setLEDs(CHOICE_YELLOW);
  toner(CHOICE_YELLOW, 150);

  setLEDs(CHOICE_RED | CHOICE_GREEN | CHOICE_BLUE); // Turn on the other LEDs until you release button

  while (checkButton() != CHOICE_NONE)
    ; // Wait for user to stop pressing button

  setLEDs(CHOICE_NONE); // Turn off LEDs

  delay(1000); // Wait a second before playing song

  digitalWrite(BUZZER1, LOW); // setup the "BUZZER1" side of the buzzer to stay low, while we play the tone on the other pin.

  while (checkButton() == CHOICE_NONE) //Play song until you press a button
  {
    // iterate over the notes of the melody:
    for (int thisNote = 0; thisNote < 32; thisNote++)
    {
      changeLED();
      tone(BUZZER2, melody[thisNote], noteDuration);
      // to distinguish the notes, set a minimum time between them.
      // the note's duration + 30% seems to work well:
      int pauseBetweenNotes = noteDuration * 1.30;
      delay(pauseBetweenNotes);
      // stop the tone playing:
      noTone(BUZZER2);
      if (checkButton() != CHOICE_NONE)
        break;
    }
  }
}

const int c = 261;
const int d = 294;
const int e = 329;
const int f = 349;
const int g = 391;
const int gS = 415;
const int a = 440;
const int aS = 455;
const int b = 466;
const int cH = 523;
const int cSH = 554;
const int dH = 587;
const int dSH = 622;
const int eH = 659;
const int fH = 698;
const int fSH = 740;
const int gH = 784;
const int gSH = 830;
const int aH = 880;

// {note, duration}
int swmelody[][2] = {

    //first section
    {NOTE_A4, 500},
    {NOTE_A4, 500},
    {NOTE_A4, 500},
    {NOTE_F4, 350},
    {NOTE_C5, 150},
    {NOTE_A4, 500},
    {NOTE_F4, 350},
    {NOTE_C5, 150},
    {NOTE_A4, 650},
    {0, 500},
    {NOTE_E5, 500},
    {NOTE_E5, 500},
    {NOTE_E5, 500},
    {NOTE_F5, 350},
    {NOTE_C5, 150},
    {NOTE_GS4, 500},
    {NOTE_F4, 350},
    {NOTE_C5, 150},
    {NOTE_A4, 650},
    {0, 500},

    //second section
    {NOTE_A5, 500},
    {NOTE_A4, 300},
    {NOTE_A4, 150},
    {NOTE_A5, 500},
    {gSH, 325},
    {NOTE_G5, 175},
    {NOTE_FS5, 125},
    {NOTE_F5, 125},
    {NOTE_FS5, 250},
    {0, 325}, //delay
    {aS, 250},
    {NOTE_CS5, 500},
    {NOTE_D5, 325},
    {NOTE_CS5, 175},
    {NOTE_C5, 125},
    {NOTE_AS4, 125},
    {NOTE_C5, 250},
    {0, 350},

    //Variant 1
    {f, 250},
    {gS, 500},
    {f, 350},
    {a, 125},
    {cH, 500},
    {a, 375},
    {cH, 125},
    {eH, 650},

    {0, 500},

    //second section
    {NOTE_A5, 500},
    {NOTE_A4, 300},
    {NOTE_A4, 150},
    {NOTE_A5, 500},
    {gSH, 325},
    {NOTE_G5, 175},
    {NOTE_FS5, 125},
    {NOTE_F5, 125},
    {NOTE_FS5, 250},
    {0, 325}, //delay
    {aS, 250},
    {NOTE_CS5, 500},
    {NOTE_D5, 325},
    {NOTE_CS5, 175},
    {NOTE_C5, 125},
    {NOTE_AS4, 125},
    {NOTE_C5, 250},
    {0, 350},

    //variant2
    {f, 250},
    {gS, 500},
    {f, 375},
    {cH, 125},
    {a, 500},
    {f, 375},
    {cH, 125},
    {a, 650},
    {0, 650}};

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

void play_starwars()
{
  //Turn on the bottom right (yellow) LED
  setLEDs(CHOICE_YELLOW);
  toner(CHOICE_YELLOW, 150);

  setLEDs(CHOICE_RED | CHOICE_GREEN | CHOICE_BLUE); // Turn on the other LEDs until you release button

  while (checkButton() != CHOICE_NONE)
    ; // Wait for user to stop pressing button

  setLEDs(CHOICE_NONE); // Turn off LEDs

  delay(1000); // Wait a second before playing song

  digitalWrite(BUZZER1, LOW); // setup the "BUZZER1" side of the buzzer to stay low, while we play the tone on the other pin.

  int swsize = ARRAY_SIZE(swmelody);

  while (checkButton() == CHOICE_NONE) //Play song until you press a button
  {
    // iterate over the notes of the melody:

    for (int thisNote = 0; thisNote < swsize; thisNote++)
    {
      changeLED();
      tone(BUZZER2, swmelody[thisNote][0], swmelody[thisNote][1]);
      // to distinguish the notes, set a minimum time between them.
      // the note's duration + 30% seems to work well:
      int pauseBetweenNotes = noteDuration * 1.30;
      delay(swmelody[thisNote][1]);
      // stop the tone playing:
      noTone(BUZZER2);
      delay(50);
      if (checkButton() != CHOICE_NONE)
        break;
    }
  }
}

// Each time this function is called the board moves to the next LED
void changeLED(void)
{
  setLEDs(1 << LEDnumber); // Change the LED

  LEDnumber++; // Goto the next LED
  if (LEDnumber > 3)
    LEDnumber = 0; // Wrap the counter if needed
}

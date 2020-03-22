// ===========================================================================================
//                  LISTEN TO MORSE CODE SIGNALS AND DISPLAY AS TEXT ON LCD
// ===========================================================================================

// Using a microphone module recieved pulses of morse code are sent to an input pin of a UNE or Nano. 
// A speed adjustment set by a potentiometer allows the receiver to be adjusted for the transmission 
// speed. Alternatively, using an auto switch places the unit in automatic mode that attemtps to 
// automatically track and set the speed.
// 
// The set length (in words per minute) and the actual speed of the last receved dot are displayed
// on the top line of a 20x4 LCD display. The second and third line display the most recent characters 
// of received morse in plain text. The bottom line displays the the most recent morse displayed as 
// dots and dashes.

// Author Garry Edmonds
// Version 1 22/03/2020
// MorseDecoder is an open source project licensed under the terms of the GNU General Public License v3.0
// GPL licensing information can found here: https://www.gnu.org/licenses/

// Connections
// ===========
//  Microphone module
//  ----------------- 
//  Dot/dash pulse LOW
//  Input pin from microphone module (morse receive) to 2 (D2 on Nano) via 27 ohm resisitor. 
//  6.8 uF capacitor between ground and pin 2 (D2 on Nano) (between ground and input)
//  Power to 5V and ground
//
//  LCD with I2C backpack
//  ---------------------
//  LCD SDA to A4 
//  LCD SCL to A5
//  
//  LCD without I2C backpack
//  ------------------------
//  LCD RS pin to digital pin 7
//  LCD Enable pin to digital pin 12
//  LCD D4 pin to digital pin 11
//  LCD D5 pin to digital pin 10
//  LCD D6 pin to digital pin 9
//  LCD D7 pin to digital pin 8
//  LCD R/W pin to ground
//  LCD VSS pin to ground
//  LCD VCC pin to 5V
//  10K (contrast control) resistor:
//    ends to +5V and ground
//    wiper to LCD VO pin (pin 3)
//  LCD backlight 220ohm resistor LCD pin 15 (LED+) and +5V
//  LCD backlight pin 16 (LED-) and ground
//  
//  LED
//  ---
//  Receive status LED to D5 on Nano and ground via a 1K Resistor 
//  
//  Mode switch
//  -----------
//  Manual/Auto speed detection switch to D6 on Nano and ground
//
//  Speed pot
//  ---------
//  Centre pin to A3. Outer pins to GND and +5V


//  Converting dot length to words per minute
//  =========================================
//  Convert dot duration to words per minute. Based upon a 50 dot duration standard word such as PARIS
//    
//  W = 1200 / T (ms)
//  Where: T is the unit time, or dot duration in milliseconds, W is the speed in wpm
//  
//  150ms = 8wpm, 100ms = 12wpm, 50ms = 24wpm, 20ms = 60wpm


// LCD with and without a backpack and LCD libraries
// =================================================
// The LCD can be use with or witout an I2C backpack. With a backpack requires 
// fewer connections but update of the display is slower and is likely to affect the 
// maximum speed.
// 
// There are multple libraries for the LCD. The common LiquidCrystal.h library works at
// lower speeds, but the hd44780.h updates the display much quicker which is what I
// had the best success and ultimately used. A few minor changes need to be made to 
// switch between the two. Look for the two zzzzzzzzzz areas in the code to change.

// zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz
// ********************************** LCD library and setup *********************************
/*
// I2C LCD libraries and setup
#include <Wire.h>                       // Wire for I2C communication to the LCD
#include <LiquidCrystal_I2C.h>          // LCD I2C library for LCD functions
const byte I2C_ADDR = 0x3F;             // Hexadecimal address of the LCD unit
LiquidCrystal_I2C lcd(I2C_ADDR, 20, 4); // Set the LCD address to 0x27 for a 20 chars and 4 line display
*/

/*
// LCD without I2C, using LiquidCrystal library
#include <LiquidCrystal.h>
// initialize the library with the numbers of the interface pins
const byte LCD_COLS = 20;
const byte LCD_ROWS = 4;
const byte rs = 7, en = 12, d4 = 11, d5 = 10, d6 = 9, d7 = 8;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
*/

// LCD without I2C using HD44780 library
#include <hd44780.h>
#include <hd44780ioClass/hd44780_pinIO.h> // Arduino pin i/o class header
const byte LCD_COLS = 20;
const byte LCD_ROWS = 4;
const int rs = 7, en = 12, d4 = 11, d5 = 10, d6 = 9, d7 = 8;
hd44780_pinIO lcd(rs, en, d4, d5, d6, d7);
// zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz

const byte INPUT_PIN = 2;       // The morse input pin (D2 on Nano)
const byte SPEED_POT_PIN = A3;  // The analog pin for the speed pot (A3 on Nano)
const byte STATUS_LED_PIN = 5;  // The LED receive status pin (D5 on Nano)
const byte AUTO_MODE_PIN = 6;   // Pin for auto/manual speed switch (D6 on Nano)

// Define Morse characters
String morsePlain[] = {"A","B","C","D","E","F","G","H","I","J","K","L","M","N","O",
  "P","Q","R","S","T","U","V","W","X","Y","Z","0","1","2","3","4","5","6","7","8","9",
  "'","\"","@",")",":",";",",","!",".","-","?","$","/","_","<AA>","<AR/+>",
  "<AS/&>","<BK>","<BT/=>","<CL>","<CT>","<DO>","<HH>","<KN/(>","<SK>","<SN>","<SOS>"};

byte morseCode[][9]={     //  1 = dot, 2 = dash, 0 = not used
  {1,2,0,0,0,0,0,0,0},    //  [0]    A   .-
  {2,1,1,1,0,0,0,0,0},    //  [1]    B   -...
  {2,1,2,1,0,0,0,0,0},    //  [2]    C   -.-.
  {2,1,1,0,0,0,0,0,0},    //  [3]    D   -..
  {1,0,0,0,0,0,0,0,0},    //  [4]    E   .
  {1,1,2,1,0,0,0,0,0},    //  [5]    F   ..-.
  {2,2,1,0,0,0,0,0,0},    //  [6]    G   --.
  {1,1,1,1,0,0,0,0,0},    //  [7]    H   ....
  {1,1,0,0,0,0,0,0,0},    //  [8]    I   ..
  {1,2,2,2,0,0,0,0,0},    //  [9]    J   .---
  {2,1,2,0,0,0,0,0,0},    //  [10]   K   -.-
  {1,2,1,1,0,0,0,0,0},    //  [11]   L   .-..
  {2,2,0,0,0,0,0,0,0},    //  [12]   M   --
  {2,1,0,0,0,0,0,0,0},    //  [13]   N   -.
  {2,2,2,0,0,0,0,0,0},    //  [14]   O   ---
  {1,2,2,1,0,0,0,0,0},    //  [15]   P   .--.
  {2,2,1,2,0,0,0,0,0},    //  [16]   Q   --.-
  {1,2,1,0,0,0,0,0,0},    //  [17]   R   .-.
  {1,1,1,0,0,0,0,0,0},    //  [18]   S   ...
  {2,0,0,0,0,0,0,0,0},    //  [19]   T   -
  {1,1,2,0,0,0,0,0,0},    //  [20]   U   ..-
  {1,1,1,2,0,0,0,0,0},    //  [21]   V   ...-
  {1,2,2,0,0,0,0,0,0},    //  [22]   W   .--
  {2,1,1,2,0,0,0,0,0},    //  [23]   X   -..-
  {2,1,2,2,0,0,0,0,0},    //  [24]   Y   -.--
  {2,2,1,1,0,0,0,0,0},    //  [25]   Z   --..
  {2,2,2,2,2,0,0,0,0},    //  [26]   0   -----
  {1,2,2,2,2,0,0,0,0},    //  [27]   1   .----
  {1,1,2,2,2,0,0,0,0},    //  [28]   2   ..---
  {1,1,1,2,2,0,0,0,0},    //  [29]   3   ...--
  {1,1,1,1,2,0,0,0,0},    //  [30]   4   ....-
  {1,1,1,1,1,0,0,0,0},    //  [31]   5   .....
  {2,1,1,1,1,0,0,0,0},    //  [32]   6   -....
  {2,2,1,1,1,0,0,0,0},    //  [33]   7   --...
  {2,2,2,1,1,0,0,0,0},    //  [34]   8   ---..
  {2,2,2,2,1,0,0,0,0},    //  [35]   9   ----.
  {1,2,2,2,2,1,0,0,0},    //  [36]   '   Apostrophe  .----.
  {1,2,1,1,2,1,0,0,0},    //  [37]   "   Quotation mark .-..-.
  {1,2,2,1,2,1,0,0,0},    //  [38]   @   At sign .--.-.
  {2,1,2,2,1,2,0,0,0},    //  [39]   )   Bracket, close (parenthesis)  -.--.-
  {2,2,2,1,1,1,0,0,0},    //  [40]   :   Colon ---...
  {2,1,2,1,2,1,0,0,0},    //  [41]   ;   Semicolon -.-.-.
  {2,2,1,1,2,2,0,0,0},    //  [42]   ,   Comma --..--
  {2,1,2,1,2,2,0,0,0},    //  [43]   !   Exclamation mark  -.-.--
  {1,2,1,2,1,2,0,0,0},    //  [44]   .   Full-stop  .-.-.-
  {2,1,1,1,1,2,0,0,0},    //  [45]   -   Hyphen / Minus -....-
  {1,1,2,2,1,1,0,0,0},    //  [46]   ?   Question mark ..--..
  {1,1,1,2,1,1,2,0,0},    //  [47]   $   Dollar sign ...-..-
  {2,1,1,2,1,0,0,0,0},    //  [48]   /   Slash/Franction -..-.
  {1,1,2,2,1,2,0,0,0},    //  [49]   _   Underscore ..--.-
  {1,2,1,2,0,0,0,0,0},    //  [50]   <AA>   New line .-.-
  {1,2,1,2,1,0,0,0,0},    //  [51]   <AR/+> End of message OR + .-.-.
  {1,2,1,1,1,0,0,0,0},    //  [52]   <AS/&> Wait OR & .-...
  {2,1,1,1,2,1,2,0,0},    //  [53]   <BK>   Break  -...-.-
  {2,1,1,1,2,0,0,0,0},    //  [54]   <BT/=> New paragraph, Break OR = -...-
  {2,1,2,1,1,2,1,1,0},    //  [55]   <CL>   Going off the air ("clear")  -.-..-..
  {2,1,2,1,2,0,0,0,0},    //  [56]   <CT>   Start copying  -.-.-
  {2,1,1,2,2,2,0,0,0},    //  [57]   <DO>   Change to wabun code -..---
  {1,1,1,1,1,1,1,1,0},    //  [58]   <HH>   Error ........
  {2,1,2,2,1,0,0,0,0},    //  [59]   <KN/(> Invite a specific station to transmit OR (  -.--.
  {1,1,1,2,1,2,0,0,0},    //  [60]   <SK>   End of transmission  ...-.-
  {1,1,1,2,1,0,0,0,0},    //  [61]   <SN>   Understood (also VE) ...-.
  {1,1,1,2,2,2,1,1,1},    //  [62]   <SOS>  Distress message  ...---...
  };

// Define the bit patterns for each of our custom chars 5 bits wide and 8 dots deep
uint8_t space[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000
  };
  
uint8_t dot[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b00110,
  0b00110,
  0b00000,
  0b00000,
  0b00000
  };
  
uint8_t dash[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b11111,
  0b11111,
  0b00000,
  0b00000,
  0b00000
  };


// Set the starting duration of a dot and create vaiables for other timings.
int dotDuration = 60;                         // The duration of a dot in milliseconds

// Other timing values are set in the setTiming function
int maxDotDuration;                           // Maximum length of a dot. Minimum length of a dash
int maxDashDuration;                          // The maximum length of a dash. Any more and it is considered stuck
int minDuration;                              // Minimum length of a dot or gap. Anything less is a bounce or static
int minWordGap;                               // Anything less is considered a part of a word or character
int minCharacterGap;                          // Anything less is considered part of a character

int lastDotDuration = 0;                      // Used with dotDuration to check if speed (pot or auto) has changed.
byte smoothingFactor = 9;                     // For auto mode - sets amount of smoothing. Use 1 - 10. 1 = too much smoothing, 10 = None

// Variables for the interrupt
volatile bool stateNow = 0;                   // Current receive state - 0 = within a mark, 1 = within a gap
volatile unsigned long lastInterruptTime = 0; // Time of the last interupt in milliseconds
volatile bool newDataFlag = 0;                // New mark received flag - 1 = there is new data to process
volatile bool lastMarkType = 0;               // Stores the last mark - 0 = dot, 1 = dash
volatile unsigned int lastMarkLength;         // Length of last mark in milliseconds

byte currentCharacter [9] = {0,0,0,0,0,0,0,0,0}; // Array of dots and dashes of the current received character
byte bufferPosition = 0;                      // The current bit/mark position received for the current character (currentCharacter array)
byte currentCharacterNumber;                  // Plain text character array number of current morse character
bool spaceFlag = 0;                           // A flag to control when to print a space 0 = no, 1= yes

// Variables to track display data
String completeLineText = "";                 // All text in the first scrolling line
int lastCharLoc = 0;                          // The location of the last character on the scrolling line
byte morseDisplayBuffer[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; // Array of dots, dashes and spaces for storing scrolling morse display

// Track mode (manual/auto)
bool currentMode;                             // The current mode setting. 0 = Manual, 1 = Auto
bool lastMode;                                // The mode when last checked

// ===========================================================================================
//                                            SETUP
// ===========================================================================================

void setup() {

  // Map input and output pins
  pinMode(INPUT_PIN, INPUT);            // Set pin as input for the morse input pin
  pinMode(STATUS_LED_PIN, OUTPUT);      // Set pin as output for the status LED
  pinMode(SPEED_POT_PIN, INPUT);        // Set pin as input for the analog speed pot
  pinMode(AUTO_MODE_PIN, INPUT_PULLUP); // Set pin as input for the auto/manual mode switch

  lastMode = digitalRead(AUTO_MODE_PIN); // Set the mode;
  
  // Attach an interrupt. Set to trigger on rising AND falling edges to catch beginning and end of marks
  attachInterrupt(digitalPinToInterrupt(INPUT_PIN), morse_ISR, CHANGE);

  // zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz
  // ******** Initialise LCD - Use method for your library and connection ********
  // For HD44780 library (with a backpack?) **************
  //lcd.begin(LCD_COLS, LCD_ROWS);
  
  // Initialise the LCD and turn on backlight for LiquidCrystal library
  // lcd.begin();        // Use with I2C backpack
  // lcd.backlight();    // Use with I2C backpack 

  // For LiquidCrystal and HD44780 libraries without a backpack
  lcd.begin(LCD_COLS, LCD_ROWS);   // Use when not using a backpack
  // zzzzzzzzzzzzzzzzzzzzzzzzzzzzz End of LCD initialisation zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz

  // Create custom LCD characters (.- and space) for morse
  lcd.createChar(0, space);
  lcd.createChar(1, dot);
  lcd.createChar(2, dash);
  
  welcomeScreen();  // Display the welcome screen

  currentMode = digitalRead(AUTO_MODE_PIN);
  setTiming(); // Check and update timing values. As dotDuration and lastDotDuration are set differently all
  //displaySetSpeed(0); // Update display
  //lastDotDuration = map(analogRead(SPEED_POT_PIN), 0, 1023, 20, 150);  // Set it to the value from the pot - May not need this
}

// ===========================================================================================
//                                          MAIN LOOP
// ===========================================================================================

void loop() {

  // Check mode and update if required
  currentMode = digitalRead(AUTO_MODE_PIN);
  if (currentMode != lastMode) {  // If it's changed
    displaySetSpeed(dotDuration); // Update display
    lastMode = currentMode;       // Remember what it is for next time
  }

  // If in manual mode, check speed pot and update timings
  if (currentMode == 0) {         // if auto mode is off

    // Get the value and map it to a useful range 20 - 150
    dotDuration = map(analogRead(SPEED_POT_PIN), 0, 1023, 20, 150);  

    // If the speed has changed update timing values
    if (dotDuration != lastDotDuration) { 
      setTiming();
    }
  }

  // Check for dodgy data
  validationCheck();
    
  // Check if enough time has elapsed after a dot or dash for a character or word to end.
  if (stateNow == 0) {            // If nothing is currently being sent
    dotsAndDashes();              // Check if there are any new dots or dashes and process
    endOfCharacter();             // If enough time has elapsed to end a character and there is something in the buffer, convert and display it
    endOfWord();                  // Print a space if enough time has elapsed and one has not already been printed
  }
}

// ===========================================================================================
//                                        PROCESS DATA
// ===========================================================================================
// -------------------------------------------------------------------------------------------
//                                   Check and set timings
// -------------------------------------------------------------------------------------------

void setTiming() {

  // Check if speed has changed. If it has recalculate timing values and update set speed
  // The length of dashes and gaps are set here as a multiple of the dot length
  // We are not using the exact morse specs to allow for variations in operator performance

  // Official morse specifications:
  // -----------------------------
  //  Dashes = 3 x dot length
  //  Gaps between dots and dashes within a character = 1 x dot length
  //  Gaps between characters = 3 x dot length
  //  Gaps between words = 7 x dot length

  // Minimum duration - This method sets it to a ratio of the dot length, but doesn't track so well in auto mode
  /*minDuration = 0.5 * dotDuration;        // Minimum length of a dot or gap. Anything less is a bounce or static
  if (minDuration < 10) {
    minDuration = 10;                     // Ensure minimum duration always at least 10 milliseconds
  }*/
  minDuration = 10;                       // Alternatively, just a minimum length of a dot or gap in millis.
  
  maxDotDuration = 1.7 * dotDuration;     // Maximum length of a dot. Minimum length of a dash  2
  maxDashDuration = 15 * dotDuration;     // The maximum length of a dash. Any more and it is considered stuck  12
  minWordGap = 5.5 * dotDuration;         // Anything less is considered a part of a word 5
  minCharacterGap = 1.8 * dotDuration;    // Anything less is considered part of a character  2

  displaySetSpeed(dotDuration);           // Update speed on display

  lastDotDuration = dotDuration;          // Store current value for next check 
}

// -------------------------------------------------------------------------------------------
//                   Check for buffer overflow and if stuck on receive
// -------------------------------------------------------------------------------------------

void validationCheck() {
  // Check data for the current character to ensure it has not exceeded the number of bits and not stuck on high

  // Check if stuck in a high input
  if (stateNow == 1 && (millis() - lastInterruptTime) > maxDashDuration) {
    stateNow = 0; // Reset state
    digitalWrite(STATUS_LED_PIN, LOW);    // turn the LED off
    // This could be improved. These are some limitations:
    //  - There is no further processing. It's not stored as a dash, just ignored
    //  - The next ISR call could be affected and this is not being dealt with
    //  - lastInteruptTime is not being reset
  }
  
  // Check if more than 9 dots/dashes have been received for a character. If so that's more than the buffer holds
  // Assume it is garbage and reset bufferPosition ready to start monitoring again
  if (bufferPosition > 9) { 
    for (byte a=0; a<9; a++) { // Reset character array
      currentCharacter[a] = 0;
    }    
    bufferPosition = 0; // Reset buffer position back to zero
  }
}

// -------------------------------------------------------------------------------------------
//                                  Process dots and dashes
// -------------------------------------------------------------------------------------------

void dotsAndDashes() {
  // Print any dots or dashes that have not already been displayed and update buffer
  
  if (newDataFlag == 1) {

    // Record and display dots and dashes
    if (lastMarkType == 0) {                    // It's a dot
      displayMark(1);                           // display dot
      currentCharacter[bufferPosition] = 1;     // Add it to the current character array
      displayActualSpeed(lastMarkLength);       // Display actual length of dot
    } 
    else {                                      // It's a dash
      displayMark(2);                           // display dash
      currentCharacter[bufferPosition] = 2;     //  Add it to the current character array      
      //lastMarkLength = lastMarkLength / 2.8;    // If it is a dash, divide by 2.8. Theoretically it should be 3, but this works better
    }

    //displayActualSpeed(lastMarkLength);         // Display last mark length, divided by three for dashes
    bufferPosition++;                           // Update buffer position ready for next mark
    newDataFlag = 0;                            // Reset back to zero until next mark received

    // If it is in auto mode, update dotDuration and speed
    if (currentMode == 1) {                     // If auto mode on
      if (lastMarkType == 1) {                  // Dashes
        // Using dash lengths helps speed adjustment when their is an immediate changes in speed
        lastMarkLength = lastMarkLength / 3;    // If it is a dash, divide by 3.
      }
      dotDuration = ((smoothingFactor * lastDotDuration) + ((10 - smoothingFactor) * lastMarkLength)) / 10; // Smooth timing
      setTiming();                              // Update other timing values
    }
  }
}

// -------------------------------------------------------------------------------------------
//                                  Process characters as plain text
// -------------------------------------------------------------------------------------------

void endOfCharacter() { 
  // If enough time has elapsed to end a character and there is something in the buffer, work
  // out what it is and print it

  bool matchFound = 0; // Tracks if a known character is found  

  // Check if enough time has elapsed and there is something in the buffer
  if ((millis() - lastInterruptTime) > minCharacterGap && bufferPosition != 0) {

    // Loop through the known character codes and return the matching character or prosign if found
    for (byte c = 0; c < 63; c++) {
      // There may be more efficient way to do this
      if (currentCharacter[0] == morseCode[c][0] &&
        currentCharacter[1] == morseCode[c][1] &&
        currentCharacter[2] == morseCode[c][2] &&
        currentCharacter[3] == morseCode[c][3] &&
        currentCharacter[4] == morseCode[c][4] &&
        currentCharacter[5] == morseCode[c][5] &&
        currentCharacter[6] == morseCode[c][6] &&
        currentCharacter[7] == morseCode[c][7] &&
        currentCharacter[8] == morseCode[c][8]) {
          // It's a match
          currentCharacterNumber = c;  // Set number to match arrays
          matchFound = 1; // flag that a match has been found
          break;  // Break out of the loop
      }
    }

    if (matchFound != 1) {
      // If the character was not found, use the question mark character and display that instead
      currentCharacterNumber = 46;
    }

    // Process character 
      updateTextDisplay(currentCharacterNumber); // Display it as a character
      displayMark(0);   // Add a space to the morse line
      spaceFlag = 1;    // Set the space flag so a space will be printed later if there is a sufficient gap

    // Reset character array - Even if a match was not found, reset it
    for (byte b=0; b<9; b++) {
      currentCharacter[b] = 0;
    }
    
    bufferPosition = 0; // Reset the buffer position back to 0  
  }
}

// -------------------------------------------------------------------------------------------
//                               Add a space to the end of words
// -------------------------------------------------------------------------------------------

void endOfWord() {
  // Print a space if enough time has elapsed and one has not already been printed
  
  if ((millis() - lastInterruptTime) > minWordGap && spaceFlag == 1) {
    
    // Add a space to the text line
    updateTextDisplay(99);  // 99 is the magic space number

    // Add a space to the morse line - There will now be two; one for end of character and one for end of word
    displayMark(0);

    spaceFlag = 0;          // Reset flag so spaces are not continuously printed
  } 
}

// ===========================================================================================
//                                           DISPLAY
// ===========================================================================================
// -------------------------------------------------------------------------------------------
//                             Display welcome screen and title
// -------------------------------------------------------------------------------------------

void welcomeScreen() {
  // Display a welcome message on the display on power up

  // Set cursor to top left
  lcd.home();

  lcd.setCursor(0, 0);
  lcd.print(F(""));                 // Text for top row
  lcd.setCursor(0, 1);
  lcd.print(F("     Morse Code"));  // Text for second row
  lcd.setCursor(0, 2);
  lcd.print(F("      Decoder")); // Text for third row
  lcd.setCursor(0, 1);
  lcd.print(F(""));                 // Text for last row

  delay (1000);                     // Show for a while

  lcd.clear();                      // Clear the screen ready for the real stuff
}

// -------------------------------------------------------------------------------------------
//                                      Display set speed
// -------------------------------------------------------------------------------------------

void displaySetSpeed(int dotLength) {
  // Display set dot length on the top row

  byte setWPM;
  
  // Convert from Words per minute
  setWPM = 1200 / dotLength;          // Based upon a 50 dot duration standard word such as PARIS

  // Display as words per minute
  lcd.setCursor(0, 0);                // Set cursor to top left
  
  if (currentMode == 1) { // Display Mode
    lcd.print(F("Auto: "));
  } 
  else {
    lcd.print(F("Man:  "));
  }
  
  lcd.setCursor(11, 0);
  lcd.print(F("Rec: "));
  lcd.setCursor(6, 0); 
  lcd.print(setWPM);                  // Display speed in words per minute
  lcd.print(" ");                     // A blank space at the end to clear old bits

}

// -------------------------------------------------------------------------------------------
//                                     Display actual speed
// -------------------------------------------------------------------------------------------
void displayActualSpeed(int dotLength) {
  // Display the actual last received dot length. This is only called when a dot is received

  byte actualWPM;
  
  // Convert from Words per minute
  actualWPM = 1200 / dotLength;       // Based upon a 50 dot duration standard word such as PARIS

  lcd.setCursor(16, 0);               // Set cursor on top row
  lcd.print(actualWPM);               // Display received speed in words per minute
  lcd.print(" ");                     // A blank space at the end to clear old bits
}

// -------------------------------------------------------------------------------------------
//                                 Update Morse plain text display
// -------------------------------------------------------------------------------------------

void updateTextDisplay (byte charToDisplay) {
  // Print characters one at a time on one line from left to right. When the line is full print 
  // a copy of the line on the line above. Clear the  line and continue printing on it from
  // left to right until it is once again full.
  // 99 is a space. All other numbers refer to the morse characters

  String currentCharacter;    // Stores each part of the plain text. Used to support prosigns that have multiple characters

  // Check if it is a space or something else and set currentCharacter value
  if (charToDisplay == 99) {                // It's a space
    currentCharacter = " ";
  } 
  else {                                    // It's something else
    currentCharacter = morsePlain[charToDisplay];  // Get the string of the current character number
  }

  // Prosigns have multiple characters so loop through string
  for (int a = 0; a < currentCharacter.length(); a++) { 

    // Check if we are at the end of the row or we've hit a new line character.
    // If so move the text up, clear the line and start again
    if (lastCharLoc > 19) {
      // Update the top line of text
      lcd.setCursor(0, 1);
      lcd.print(completeLineText);          // Display text from the scrolling line
      completeLineText = "";                // Reset scrolling line text back to empty
      lastCharLoc = 0;                      // Reset scrolling line cursor to first position
  
      // Clear the scrolling line
      lcd.setCursor(0, 2);                  // Select the scrolling line
      lcd.print(F("                    ")); // Clear the line - 20 spaces
      lcd.setCursor(0, 2);                  // Set cursor back to the start of the line
    }
    
    // Set the cursor to the current location on the scrolling line
    lcd.setCursor(lastCharLoc, 2);
    
    lcd.print(currentCharacter[a]);         // Print this character
    completeLineText = completeLineText + currentCharacter[a]; // Add latest character to the line string
    
    lastCharLoc++;                          // Remember where we are on the line
  }
}

// -------------------------------------------------------------------------------------------
//                                 Display Morse Dots and Dashes
// -------------------------------------------------------------------------------------------

void displayMark(char markToDisplay) {
  // Display dots, dashes and spaces on bottom line of the display.
  // Start at the right hand side of the line and scroll left.

  // Move everything in the buffer along 1 place
  for (byte b=0; b<19; b++) {
    morseDisplayBuffer[b] = morseDisplayBuffer[b+1];
  }
  
  // Add the new character to the end of the array
  morseDisplayBuffer[19] = markToDisplay;

  // Update display
  for (byte d=0; d<20; d++) {
    
    lcd.setCursor(d, 3); // Set cursor to correct place on the last line
    
    // Display latest dot, dash or space
    lcd.write(morseDisplayBuffer[d]);
  }
}

// ===========================================================================================
//                                         INTERRUPT
// ===========================================================================================

void morse_ISR() {
  // This interrupt is triggered on all changes. It determines if a dot or dash has just been received. 
  // Gaps/spaces are processed in other functions via the main loop.

  unsigned long interruptTime = millis();

  // Check if it is a bit of static/bounce
  if (interruptTime - lastInterruptTime >= minDuration) {
 
    // Are we starting or ending a dot/dash? - HIGH is off
    if (digitalRead(INPUT_PIN) == HIGH) { // We have just ended a dot or dash
  
      lastMarkLength = interruptTime - lastInterruptTime; // Get length in milliseconds
      
      // Was it a dot or a dash?
      if (lastMarkLength < maxDotDuration) { 
        lastMarkType = 0;                 // It's a dot
      }
      else { 
        lastMarkType = 1;                 // It's a dash
      }  
         
      newDataFlag = 1;                    // Flag there is a new mark to process
      stateNow = 0;                       // Set current state to nothing being currently sent
      digitalWrite(STATUS_LED_PIN, LOW);  // turn the LED off     
    }
    else { // We are just starting a dot/dash
      stateNow = 1; // Set current state to currently recieving a mark
      digitalWrite(STATUS_LED_PIN, HIGH); // turn the LED on
    }
    //lastInterruptTime = interruptTime;    // Remember interrupt time for next time ******** Seems to work better with inteference here ********
  }
  
  lastInterruptTime = interruptTime;      // Remember interrupt time for next time ******** than here ********
}

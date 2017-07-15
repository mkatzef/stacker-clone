/* 
 * stacker-clone.ino
 * 
 * A clone of the arcade game/merchandiser STACKER for an Arduino Nano and 32x16
 * LED matrix display.
 * 
 * Written by Marc Katzef
 */

#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

//Libraries for Dot Matrix
#include <SPI.h>
#include <DMD.h>
#include <TimerOne.h>

// Display brightness
#define SCAN_PERIOD_MICROS 500
#define BRIGHTNESS_STEPS 2  // Number of screen scans per PWM period
#define BRIGHTNESS 1        // (Brightness %) * BRIGHTNESS_STEPS

// Display and game dimensions
#define SCREEN_COLUMNS 16 // Number of LEDs in width
#define SCREEN_ROWS 32    // Number of LEDs in height
#define BORDER_SIZE 1     // Number of LEDs from the display edge to use as a border
#define CELL_SIZE 2       // Width and height of each STACKER block
#define GAME_COLUMNS 7    // Number of STACKER blocks in width (SCREEN_COLUMNS - 2 * BORDER_SIZE) / CELL_SIZE
#define GAME_ROWS 15      // Number of STACKER blocks in height

// Game properties
#define START_WIDTH 3
#define WIDTH_DECREASES 2 // Length of global array widthDecreaseLevels
#define QUICK_ANIMATION_MILLIS 50
#define STANDARD_ANIMATION_MILLIS 100

// Initial and final time values used in difficulty curve
#define INITIAL_DELAY 200     // Initial time between game steps, in milliseconds.
#define STEADY_STATE_DELAY 45 // The smallest time between game steps, in milliseconds.

DMD dmd(1, 1);

int brightnessCount = 0; // Keeps track of brightness PWM phase

enum BLOCK_DIRECTIONS {DIRECTION_LEFT=0, DIRECTION_RIGHT};

unsigned short highScore = 1; // Highest level reached since start up
byte widthDecreaseLevels[WIDTH_DECREASES] = {5, 9}; // Heights at which maximum number of blocks is reduced

/* 
 * Scans the display, temporarily enabling each DMD pixel written HIGH. Also
 * implements a simple PWM-based brightness control scheme. Supplies additional
 * power to display for a number of cycles every period.
 * Should be called very frequently, perhaps through timer interrupt.
 */
void scanDmd() {
  dmd.scanDisplayBySPI();
  brightnessCount++;
  
  if(brightnessCount < BRIGHTNESS) {
    digitalWrite(9, HIGH);
  } else {
    digitalWrite(9, LOW);
  }
  
  if (brightnessCount >= BRIGHTNESS_STEPS) {
    brightnessCount = 0;
  }
}


/* 
 * Initializes required display and communication features.
 */
void setup(){
  Timer1.initialize(SCAN_PERIOD_MICROS);
  Timer1.attachInterrupt(scanDmd);
  dmd.clearScreen(true);

  Serial.begin(9600);

  SetBorder(1);
  Serial.println("Started");
}


/* 
 * Does a bit too much at the moment...
 */
void loop(void) {
  // Store the height of the highest block in each column. 0 is below screen
  byte board[GAME_COLUMNS] = {0};

  byte width = START_WIDTH;   // Number of blocks currently active
  unsigned short score = 1;   // Start at level 1
  byte altitude = 1;          // Active row of display, the result of (score - 1) % GAME_ROWS + 1
  bool dir = DIRECTION_RIGHT; // The direction in which the blocks of the active row are moving

  short leader = width - 1;   // Position of the block furthest in the direction that all blocks are moving
  short follower = 0;         // Position of the block facing away from the movement direction
  for (short i = follower; i <= leader; i++) {
    SetCell(altitude, i, 1);
  }
  
  ShowHighScore(score);
  int delayMillis = GetDelay(score); // Amount of time to wait between game steps
  
  while (1) {
    while (Serial.available() > 0) { // Clear serial buffer
      Serial.read();
    }
    delay(delayMillis);         // Keep blocks stationary, waiting for input
    if (Serial.available()) {   // Input was received
      if (follower > leader) {  // Swap values so leader is largest value
        short temp = follower;
        follower = leader;
        leader = temp;
      }
      bool active = false;          // Unknown if blocks were stopped with support
      byte fallenCols[width] = {0}; // The columns in which blocks were stopped without support
      byte fallenNumber = 0;        // Number of blocks stopped without support
      
      for (short i = follower; i <= leader; i++) {
        if(i < 0 || i == GAME_COLUMNS) {         // This block was off screen when stopped
          width--;
        } else if ((board[i] == altitude - 1)) { // This block is supported by the previous level
          board[i]++;
          active = true;
        } else {                                 // This block was on screen but unsupported
          width--;
          fallenCols[fallenNumber++] = i;
        }
      }

      if (fallenNumber) { // If any blocks were stopped unsupported, animate their fall
        byte fallenRows[fallenNumber];
        for (byte i = 0; i < fallenNumber; i++) {
          fallenRows[i] = altitude;
        }
        FlashBlocks(fallenRows, fallenCols, fallenNumber);
        for (byte i = 0; i < fallenNumber; i++) {
          BlockFall(fallenCols[i], altitude, board[fallenCols[i]]+1, STANDARD_ANIMATION_MILLIS);
        }
      }
      
      if (active) { // If any blocks were stopped with support
        altitude++;
        score++;
        highScore = max(score, highScore);
        
        delayMillis = GetDelay(score);
        
        if (altitude == GAME_ROWS + 1) { // Made it off of current screen
          Sweep(1, 1, QUICK_ANIMATION_MILLIS);
          Sweep(1, 0, QUICK_ANIMATION_MILLIS);
          
          altitude = 1; // Reset position on board
          for (byte i = 0; i < GAME_COLUMNS; i++) {
            board[i] = 0;
          }
          
          dmd.clearScreen(true);
          SetBorder(1);
          ShowHighScore(score); // Score is used to get bearings
        }

        // Find what width the next level of active blocks must be
        byte maxWidth = START_WIDTH;
        for (byte i = 0; i < WIDTH_DECREASES; i++) {
          if (altitude >= widthDecreaseLevels[i]) {
            maxWidth--;
          }
        }
        width = min(width, maxWidth);

        // Set initial position and movement direction of the next level's blocks
        if (altitude % 2 != 0) { // Left start
          leader = width - 1;
          follower = 0;
          dir = DIRECTION_RIGHT;
          for (short i = follower; i <= leader; i++) {
            SetCell(altitude, i, 1);
          }
        } else {
          leader = GAME_COLUMNS - width;
          follower = GAME_COLUMNS - 1;
          dir = DIRECTION_LEFT;
          for (short i = leader; i <= follower; i++) {
            SetCell(altitude, i, 1);
          }
        }

        continue; // Restart loop (waiting for input)
        
      } else { // Game over, perform animation
        for (byte col = 0; col < GAME_COLUMNS; col++) {
          if (board[col] > 0) {
            BlockFall(col, board[col], 1, QUICK_ANIMATION_MILLIS);
          }
        }
        Sweep(0, 1, QUICK_ANIMATION_MILLIS);
        Sweep(0, 0, QUICK_ANIMATION_MILLIS);
        break;
      }
    }

    // Regardless of whether input was received:
    
    if (leader == GAME_COLUMNS) { // Reached right end, change direction, return active blocks to normal width
      leader = GAME_COLUMNS - width;
      follower = GAME_COLUMNS - 1;
      dir = DIRECTION_LEFT;
    } else if (leader == -1) {
      leader = width - 1;
      follower = 0;
      dir = DIRECTION_RIGHT;
    } else { // Move leader and follower in current direction (allows movement one cell off of display)
      SetCell(altitude, follower, 0);
      switch (dir) {
        case DIRECTION_RIGHT:
          follower++;
          leader++;
          break;
        case DIRECTION_LEFT:
          follower--;
          leader--;
          break;
      }
    }
    
    // Undo movement off of display if only one block is active
    if (width == 1) { 
      if (leader == GAME_COLUMNS) {
        leader = follower = GAME_COLUMNS - 2;
        dir = DIRECTION_LEFT;
      } else if (leader == -1) {
        leader = follower = 1;
        dir = DIRECTION_RIGHT;
      }
    }

    // Enable latest leader block LEDs
    if (leader >= 0 && leader < GAME_COLUMNS) {
      SetCell(altitude, leader, 1);
    }
  }
}


void Sweep(bool dir, bool val, int delayMillis) {
  byte startAltitude;
  char change;

  if (dir) { // Up
    startAltitude = 0;
    change = 1;
  } else {
    startAltitude = GAME_ROWS+1;
    change = -1;
  }

  byte altitude = startAltitude;
  for (byte row = 0; row < GAME_ROWS; row++) {
    altitude += change;
    for (byte col = 0; col < GAME_COLUMNS; col++) {
      SetCell(altitude, col, val);
    }
    delay(delayMillis);
  }
}

void FlashBlocks(byte* rows, byte* cols, byte n) {
  bool val = 0;
  byte reps = 2;
  int delayMillis = 250;
  for (byte i = 0; i < 2 * reps; i++) {
    for (byte block = 0; block < n; block++) {
      SetCell(rows[block], cols[block], val);
    }
    val = !val;
    delay(delayMillis);
  }
}


// Turns off cols, end altitude after carrying this thing out.
void BlockFall(byte col, char startAltitude, char endAltitude, int delayMillis) {
  char change;
  byte difference;
  if (startAltitude > endAltitude) {
    change = -1;
    difference = startAltitude - endAltitude;
  } else {
    change =  1;
    difference = endAltitude - startAltitude;
  }

  byte altitude = startAltitude;
  
  for (byte i = 0; i < difference; i++) {
    SetCell(altitude, col, 0);
    altitude += change;
    SetCell(altitude, col, 1);
    
    delay(delayMillis);
  }
  
  SetCell(endAltitude, col, 0);
}


void SetCell(short givenRow, short givenCol, int value) {  
  int startRow = (givenRow - 1) * CELL_SIZE + BORDER_SIZE;
  int startCol = givenCol * CELL_SIZE + BORDER_SIZE;
  
  for (byte row = 0; row < CELL_SIZE; row++) {
    for (byte col = 0; col < CELL_SIZE; col++) {
      dmd.writePixel(SCREEN_ROWS - (row + startRow) - 1, col + startCol, GRAPHICS_NORMAL, value);
    }
  }
}


void SetBorder(bool value) {
  for (byte col = 0; col < SCREEN_COLUMNS; col++) {
     for (byte row = 0; row < BORDER_SIZE; row++) {
       dmd.writePixel(row, col, GRAPHICS_NORMAL, value);
       dmd.writePixel(SCREEN_ROWS - row - 1, col, GRAPHICS_NORMAL, value);
     }
  }

  for (byte row = BORDER_SIZE; row < SCREEN_ROWS - BORDER_SIZE; row++) {
    for (byte col = 0; col < BORDER_SIZE; col++) {
       dmd.writePixel(row, col, GRAPHICS_NORMAL, value);
       dmd.writePixel(row, SCREEN_COLUMNS - col - 1, GRAPHICS_NORMAL, value);
     }
  }
}


void ShowHighScore(unsigned short score) { 
  unsigned short lowest = (score / (GAME_ROWS+1)) * GAME_ROWS + 1;
  unsigned short highest = lowest + GAME_ROWS - 1;

  if (highScore >= lowest && highScore <= highest) {
    byte row = (highScore - 1) % GAME_ROWS;
    if (row <  1) {
     return;
    }
    SetCell(row, -1, 0);
    SetCell(row, GAME_COLUMNS, 0);
  }
}

int GetDelay(unsigned short score) {
  double result = 2.0 * (INITIAL_DELAY - STEADY_STATE_DELAY) / PI * atan(-2.5 * score / GAME_ROWS) + INITIAL_DELAY;
  return (int) result;
}


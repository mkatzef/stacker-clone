//Libraries for Dot Matrix
#include <SPI.h>
#include <DMD.h>
#include <TimerOne.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

DMD dmd(1, 1);

#define SPEAKERPIN 4
#define BUTTON1PIN 2
#define BUTTON2PIN 3
#define SCREEN_COLUMNS 16
#define SCREEN_ROWS 32
#define GAME_COLUMNS 7
#define GAME_ROWS 15
#define CELL_SIZE 2
#define BORDER_SIZE 1
#define BSTEPS 7
#define DIRECTION_RIGHT 1
#define DIRECTION_LEFT 0
#define WIDTH_DECREASES 2
#define START_WIDTH 3
#define QUICK_ANIMATION 50
#define STANDARD_ANIMATION 100

#define INITIAL_DELAY 200
#define STEADY_STATE_DELAY 45

int brightness = 15;   //display brightness
int bcount = 0;   //variable used to keep track of pwm phase

unsigned short highScore = 1; // Since boot
byte width_decrease_levels[WIDTH_DECREASES] = {5, 9};//Change whenever

void ScanDMD(){       //also implements a basic pwm brightness control
  dmd.scanDisplayBySPI();
  bcount = bcount+brightness;
  if(bcount < BSTEPS){
    digitalWrite(9,LOW);
  }
  bcount = bcount % BSTEPS;
}

void setup(){
  Timer1.initialize( 500 );           //period in microseconds to call ScanDMD. Anything longer than 5000 (5ms) and you can see flicker.
  Timer1.attachInterrupt( ScanDMD );   //attach the Timer1 interrupt to ScanDMD which goes to dmd.scanDisplayBySPI()
  dmd.clearScreen( true );     //clear/init the DMD pixels held in RAM
  pinMode(BUTTON1PIN,INPUT_PULLUP);
  pinMode(BUTTON2PIN,INPUT_PULLUP);
  pinMode(SPEAKERPIN,OUTPUT);
  digitalWrite(SPEAKERPIN,LOW);    //speaker off
  Serial.begin(9600);
}


void loop(){
  dmd.clearScreen( true );
  // clear board
  // (re) set up game
  SetBorder(1);
  Serial.print("started\n");
  game();
}


void game(void) {
  // Store the height of the highest block in each column. 0 is below screen
  byte board[GAME_COLUMNS] = {0};

  byte width = START_WIDTH;
  unsigned short score = 1;
  byte altitude = 1; // should change to 0, make board signed char or something.
  bool dir = DIRECTION_RIGHT;

  short leader = width - 1;
  short follower = 0;
  for (short i = follower; i <= leader; i++) {
    SetCell(altitude, i, 1);
  }
  
  ShowHighScore(score);
  int delayMillis = GetDelay(score);
  
  while (1) {
    while (Serial.available() > 0) {
      Serial.read();
    }
    delay(delayMillis); // Time for input
    int input = Serial.read();
    if (input != -1) {
      if (follower > leader) {
        short temp = follower;
        follower = leader;
        leader = temp;
      }
      bool active = false;
      byte fallenCols[width] = {0};
      byte fallenNumber = 0;
      
      for (short i = follower; i <= leader; i++) {

        if(i < 0 || i == GAME_COLUMNS) {
          width--;
        } else if ((board[i] == altitude - 1)) {
          board[i]++;
          active = true;
        } else {
          //SetCell(altitude, i, 0);
          width--;
          fallenCols[fallenNumber++] = i;
        }
      }
      for (int i = 0; i < GAME_COLUMNS; i++) {
      }

      if (fallenNumber) {
        byte fallenRows[fallenNumber];
        for (byte i = 0; i < fallenNumber; i++) {
          fallenRows[i] = altitude;
        }
        FlashBlocks(fallenRows, fallenCols, fallenNumber);
        for (byte i = 0; i < fallenNumber; i++) {
          BlockFall(fallenCols[i], altitude, board[fallenCols[i]]+1, STANDARD_ANIMATION);
        }
      }
      if (active) {
        altitude++;
        score++;
        highScore = max(score, highScore);
        
        delayMillis = GetDelay(score);
        
        if (altitude == GAME_ROWS + 1) { // Made it to next round
          Sweep(1, 1, QUICK_ANIMATION);
          Sweep(1, 0, QUICK_ANIMATION);
          
          altitude = 1; //Reset
          for (byte i = 0; i < GAME_COLUMNS; i++) {
            board[i] = 0;
          }
          
          dmd.clearScreen( true );
          SetBorder(1);
          ShowHighScore(score); // Score used to get bearings.
          // Used to "break" here
        }
        byte maxWidth = START_WIDTH;
        for (byte i = 0; i < WIDTH_DECREASES; i++) {
          if (altitude >= width_decrease_levels[i]) {
            maxWidth--;
          }
        }
        width = min(width, maxWidth);
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

        continue;
      } else {
        for (byte col = 0; col < GAME_COLUMNS; col++) {
          if (board[col] > 0) {
            BlockFall(col, board[col], 1, QUICK_ANIMATION);
          }
        }
        Sweep(0, 1, QUICK_ANIMATION);
        Sweep(0, 0, QUICK_ANIMATION);
        break;
      }
    }
    
    if (leader == GAME_COLUMNS) {
      leader = GAME_COLUMNS - width;
      follower = GAME_COLUMNS - 1;
      dir = DIRECTION_LEFT;
    } else if (leader == -1) {
      leader = width - 1;
      follower = 0;
      dir = DIRECTION_RIGHT;
    } else {
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
    if (width == 1) {
      if (leader == GAME_COLUMNS) {
        leader = follower = GAME_COLUMNS - 2;
        dir = DIRECTION_LEFT;
      } else if (leader == -1) {
        leader = follower = 1;
        dir = DIRECTION_RIGHT;
      }
    }
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
  Serial.print((int)startAltitude);
  Serial.print(", ");
  Serial.print((int)endAltitude);
  Serial.print(", ");
  Serial.print((int)change);
  Serial.print("\n");
  Serial.flush();
  byte altitude = startAltitude;
  
  for (byte i = 0; i < difference; i++) {
    SetCell(altitude, col, 0);
    altitude += change;
    SetCell(altitude, col, 1);
    
    delay(delayMillis);
  }
  
  SetCell(endAltitude, col, 0);
}
/*
// Turns off cols, end altitude after carrying this thing out.
void BlockFall(byte* cols, byte n, char startAltitude, char* endAltitudes) {
  int delayMillis = 150;
  bool change;
  byte difference;
  char theEnd;
  if (startAltitude > endAltitudes[0]) {
    change = -1;
    theEnd = 0;
    difference = startAltitude - theEnd;
  } else {
    change =  1;
    theEnd = GAME_ROWS + 1;
    difference = theEnd - startAltitude;
  }

  byte altitude = startAltitude;
  
  for (byte i = 0; i < difference; i++) {
    for (byte entry = 0; entry < n; entry++) {
      if ((change == 1 && altitude <= endAltitudes[entry]) ||
        (change == -1 && altitude >= endAltitudes[entry])) {
        
        SetCell(altitude, cols[entry], 0);
        altitude += change;
        SetCell(altitude, cols[entry], 1);
      }
    }
    delay(delayMillis);
  }
  
  //for (byte entry = 0; entry < n; entry++) {
  //  SetCell(endAltitude, cols[entry], 0);
  //}
}
*/

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



#include <TimerOne.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#ifndef PSTR
 #define PSTR // Make Arduino Due happy
#endif

#define PIN 13
#define LEFT 2
#define RIGHT 3 
#define RLEFT 4
#define RRIGHT 5
#define DOWN 6
#define WIDTH 6
#define POT 0
#define HEIGHT 12
#define LEVEL 8

// MATRIX DECLARATION:
// Parameter 1 = width of NeoPixel matrix
// Parameter 2 = height of matrix
// Parameter 3 = pin number (most are valid)
// Parameter 4 = matrix layout flags, add together as needed:
//   NEO_MATRIX_TOP, NEO_MATRIX_BOTTOM, NEO_MATRIX_LEFT, NEO_MATRIX_RIGHT:
//     Position of the FIRST LED in the matrix; pick two, e.g.
//     NEO_MATRIX_TOP + NEO_MATRIX_LEFT for the top-left corner.
//   NEO_MATRIX_ROWS, NEO_MATRIX_COLUMNS: LEDs are arranged in horizontalz`
//     rows or in vertical columns, respectively; pick one or the other.
//   NEO_MATRIX_PROGRESSIVE, NEO_MATRIX_ZIGZAG: all rows/columns proceed
//     in the same order, or alternate lines reverse direction; pick one.
//   See example below for these values in action.
// Parameter 5 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)


// Example for NeoPixel Shield.  In this application we'd like to use it
// as a 5x8 tall matrix, with the USB port positioned at the top of the
// Arduino.  When held that way, the first pixel is at the top right, and
// lines are arranged in columns, progressive order.  The shield uses
// 800 KHz (v2) pixels that expect GRB color data.
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(WIDTH, HEIGHT, PIN,
  NEO_MATRIX_TOP     + NEO_MATRIX_LEFT +
  NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE,
  NEO_GRB            + NEO_KHZ800);

const int ncolors = 4;
const uint16_t colors[] = {
  matrix.Color(0, 0, 0), matrix.Color(LEVEL, 0, 0), matrix.Color(0, LEVEL/2, 0), matrix.Color(0, 0, LEVEL), matrix.Color(LEVEL, 0, LEVEL)};
  
volatile bool working = false;

class puyo {
public:
  volatile int x1; //x axis of first puyo
  volatile int y1; //y axis of first puyo
  volatile int color1; //color of first puyo
  volatile int x2; //x axis of second puyo
  volatile int y2; //y axis of second puyo
  volatile int color2; //color of second puyo
  volatile char rot; //d = down r = right u = up l = left
  puyo() {
    x1 = WIDTH/2;
    y1 = 0;
    x2 = WIDTH/2;
    y2 = 1;
    rot = 'd';
    color1 = random(1,ncolors + 1);
    color2 = random(1,ncolors + 1);
  }
  void reset() {
    x1 = WIDTH/2;
    y1 = 0;
    x2 = WIDTH/2;
    y2 = 1;
    rot = 'd';
    color1 = random(1,ncolors + 1);
    color2 = random(1,ncolors + 1);
  }
  //x and y can be any int
  //rot can be 'd', 'r', 'l', 'u'
  //color1 and color2 can be 'r', 'g', 'b'
  puyo(int x, int y, char r, int c1, int c2) {
    x1 = x;
    y1 = y;
    switch (r) {
    case 'u':
      x2 = x;
      y2 = y - 1;
      break;
    case 'd':
      x2 = x;
      y2 = y+1;
      break;
    case 'l':
      x2 = x - 1;
      y2 = y;
      break;
    case 'r':
      x2 = x + 1;
      y2 = y;
    }
    rot = r;
    color1 = c1;
    color2 = c2;
  }
  void up() {
    y1--;
    y2--;
  }
  void down() {
    y1++;
    y2++;
  }
  void left() {
    x1--;
    x2--;
  }
  void right() {
    x1++;
    x2++;
  }
  void rotateleft() {
    switch (rot) {
    case 'u':
      rot = 'l';
      break;
    case 'd':
      rot = 'r';
      break;
    case 'l':
      rot = 'd';
      break;
    case 'r':
      rot = 'u';
      break;
    }
  }
  void rotateright() {
    switch (rot) {
    case 'u':
      rot = 'r';
      break;
    case 'd':
      rot = 'l';
      break;
    case 'l':
      rot = 'u';
      break;
    case 'r':
      rot = 'd';
      break;
    }
  }
};
class board {
private:
  int array[HEIGHT][WIDTH];
public:
  volatile int score;
  volatile int combo;
  puyo pu;
  volatile bool halfy; //half the puyo got stuck on a ledge lol
  volatile int whichhalf; //first or second?
  volatile int tempx; //x coord of free puyo
  volatile int tempy; //y coord of free puyo
  volatile bool puyodone = false; //puyo is done, signal a new one
  volatile bool freefalling = false;
  volatile bool combofound = false;
  volatile bool gameover = false;
  board() {
    score = 0;
    for (int j = 0; j < HEIGHT; j++) {
      for (int i =0; i < WIDTH; i++) {
        array[j][i] = 0;
      }
    }
    newpuyo();
  }
  void newpuyo() {
    if (checkpoint(WIDTH/2, 1) == 0) {
      pu.reset();
      combo = 0;
      puyodone = false;
      halfy = false;
      drawpu();
    }
    else {
      //Serial.print("Gameover\n");
      gameover = true;
      Timer1.stop();
      gameoverrun();
    }
  }
  void drawpu() {
    setpoint(pu.x1, pu.y1, pu.color1);
    setpoint(pu.x2, pu.y2, pu.color2);
  }
  void setpoint(int x, int y, int c) {
    if (x < WIDTH && x >= 0 && y < HEIGHT && y >= 0) {
      array[y][x] = c;
      /*Serial.print("Setting ");
      Serial.print(x);
      Serial.print(",");
      Serial.print(y);
      Serial.print(" to ");
      Serial.print(c);
      Serial.print("\n");*/
    }
  }
  int checkpoint(int x, int y) {
    if (x < WIDTH && x >= 0 && y < HEIGHT && y >= 0)
      return array[y][x];
    else
      return -1;
  }
  //Function calls to check specific points
  int checkup(int x, int y) {
    if (y > 0)
      return array[y-1][x];
    else
      return -1;
  }
  int checkdown(int x, int y) {
    if (y < HEIGHT - 1)
      return array[y+1][x];
    else
      return -1;
  }
  int checkleft(int x, int y) {
    if (x > 0)
      return array[y][x-1];
    else
      return -1;
  }
  int checkright(int x, int y) {
    if (x < WIDTH - 1)
      return array[y][x+1];
    else
      return -1;
  }
  //Function calls to move specific points
  void moveup(int x, int y) {
    /*Serial.print("Moving up ");
    Serial.print(x);
    Serial.print(",");
    Serial.print(y);
    Serial.print(" to ");
    Serial.print(x);
    Serial.print(",");
    Serial.print(y-1);
    Serial.print(" containing ");
    Serial.print(checkpoint(x, y));
    Serial.print("\n");*/
    swap(array[y][x], array[y-1][x]);
  }
  void movedown(int x, int y) {
    /*Serial.print("Moving down ");
    Serial.print(x);
    Serial.print(",");
    Serial.print(y);
    Serial.print(" to ");
    Serial.print(x);
    Serial.print(",");
    Serial.print(y+1);
    Serial.print(" containing ");
    Serial.print(checkpoint(x, y));
    Serial.print("\n");*/
    swap(array[y][x], array[y+1][x]);
  }
  void moveleft(int x, int y) {
    /*Serial.print("Moving left ");
    Serial.print(x);
    Serial.print(",");
    Serial.print(y);
    Serial.print(" to ");
    Serial.print(x-1);
    Serial.print(",");
    Serial.print(y);
    Serial.print(" containing ");
    Serial.print(checkpoint(x, y));
    Serial.print("\n");*/
    swap(array[y][x], array[y][x-1]);
  }
  void moveright(int x, int y) {
    /*Serial.print("Moving right ");
    Serial.print(x);
    Serial.print(",");
    Serial.print(y);
    Serial.print(" to ");
    Serial.print(x+1);
    Serial.print(",");
    Serial.print(y);
    Serial.print(" containing ");
    Serial.print(checkpoint(x, y));
    Serial.print("\n");*/
    swap(array[y][x], array[y][x+1]);
  }
  //Function calls to move pu around
  void left() {
    if (puyodone == false && halfy == false) {
      if (pu.x1 != 0 && pu.x2 != 0) {
        if ((pu.rot == 'd' || pu.rot == 'l') && checkleft(pu.x2, pu.y2) == 0){
          moveleft(pu.x2, pu.y2);
          moveleft(pu.x1, pu.y1);
          pu.left();
        }
        if ((pu.rot == 'u' || pu.rot == 'r') && checkleft(pu.x1, pu.y1) == 0) {
          moveleft(pu.x1, pu.y1);
          moveleft(pu.x2, pu.y2);
          pu.left();
        }
      }
      drawmatrix();
    }
  }
  void right() {
    if (puyodone == false && halfy == false) {
      if (pu.x1 != WIDTH && pu.x2 != WIDTH) {
        if ((pu.rot == 'd' || pu.rot == 'r') && checkright(pu.x2, pu.y2) == 0){
          moveright(pu.x2, pu.y2);
          moveright(pu.x1, pu.y1);
          pu.right();
        }
        else if ((pu.rot == 'u' || pu.rot == 'l') && checkright(pu.x1, pu.y1) == 0) {
          moveright(pu.x1, pu.y1);
          moveright(pu.x2, pu.y2);
          pu.right();
        }
      }
      drawmatrix();
    }
  }
  int down() {
    if (puyodone == false && halfy == false) {
      if (pu.rot == 'l' || pu.rot == 'r') { //rotation is either left or right
        if (checkdown(pu.x1, pu.y1) == 0 && checkdown(pu.x2, pu.y2) == 0) { //both puyo are clear
          movedown(pu.x1, pu.y1);
          movedown(pu.x2, pu.y2);
          pu.down();
        }
        else { //one piece didn't clear
          halfy = true;
          if (checkdown(pu.x1, pu.y1) != 0) { //first puyo isn't clear
            whichhalf = 1;
            tempx = pu.x2;
            tempy = pu.y2;
          }
          else {                              //second puyo isn't clear
            whichhalf = 2;
            tempx = pu.x1;
            tempy = pu.y1;
          }
        }
      }
      else if (pu.rot == 'u' && checkdown(pu.x1, pu.y1) == 0) {
        movedown(pu.x1, pu.y1);
        movedown(pu.x2, pu.y2);
        pu.down();
      }
      else if (pu.rot == 'd' && checkdown(pu.x2, pu.y2) == 0) {
        movedown(pu.x2, pu.y2);
        movedown(pu.x1, pu.y1);
        pu.down();
      }
      else {
        //Serial.print("Hit a wall\n");
        puyodone = true;
        return -1;
      }
    }
    //drawmatrix();
    return 0;
  }
  void rotateleft() {
    if (puyodone == false && halfy == false) {
      switch (pu.rot) {
      case 'u':
        if (checkleft(pu.x2, pu.y2) == 0) {
          moveleft(pu.x2, pu.y2);
          pu.x2--;
          movedown(pu.x2, pu.y2);
          pu.y2++;
          pu.rotateleft();
        }
        break;
      case 'd':
        if (checkright(pu.x2, pu.y2) == 0) {
          moveright(pu.x2, pu.y2);
          pu.x2++;
          moveup(pu.x2, pu.y2);
          pu.y2--;
          pu.rotateleft();
        }
        break;
      case 'l':
        if (checkdown(pu.x2, pu.y2) == 0) {
          movedown(pu.x2, pu.y2);
          pu.y2++;
          moveright(pu.x2, pu.y2);
          pu.x2++;
          pu.rotateleft();
        }
        break;
      case 'r':
        if (checkup(pu.x2, pu.y2) == 0) {
          moveup(pu.x2, pu.y2);
          pu.y2--;
          moveleft(pu.x2, pu.y2);
          pu.x2--;
          pu.rotateleft();
        }
        break;
      }
      drawmatrix();
    }
  }
  void rotateright() {
    if (puyodone == false && halfy == false) {
      switch (pu.rot) {
      case 'u':
        if (checkright(pu.x2, pu.y2) == 0) {
          moveright(pu.x2, pu.y2);
          pu.x2++;
          movedown(pu.x2, pu.y2);
          pu.y2++;
          pu.rotateright();
        }
        break;
      case 'd':
        if (checkleft(pu.x2, pu.y2) == 0) {
          moveleft(pu.x2, pu.y2);
          pu.x2--;
          moveup(pu.x2, pu.y2);
          pu.y2--;
          pu.rotateright();
        }
        break;
      case 'l':
        if (checkup(pu.x2, pu.y2) == 0) {
          moveup(pu.x2, pu.y2);
          pu.y2--;
          moveright(pu.x2, pu.y2);
          pu.x2++;
          pu.rotateright();
        }
        break;
      case 'r':
        if (checkdown(pu.x2, pu.y2) == 0) {
          movedown(pu.x2, pu.y2);
          pu.y2++;
          moveleft(pu.x2, pu.y2);
          pu.x2--;
          pu.rotateright();
        }
        break;
      }
      drawmatrix();
    }
  }
};

board test;
volatile int loopn = 0;
bool keepgoing = false;
bool toggle = false;
bool left;
bool right;
bool oldleft = false;
bool oldright = false;
bool rleft;
bool rright;
bool oldrleft = false;
bool oldrright = false;
bool down;
int tick = 0;
void setup() {
  Serial.begin(9600);
  matrix.begin();
  //matrix.setBrightness(40);
  pinMode(PIN, OUTPUT);
  pinMode(LEFT, INPUT);
  pinMode(RIGHT, INPUT);
  pinMode(RLEFT, INPUT);
  pinMode(RRIGHT, INPUT);
  Timer1.initialize(map(analogRead(0), 0, 1023, 100000, 1000000)); // set a timer of length 250000 microseconds (or 1/4 sec)
  Timer1.attachInterrupt( Next ); // attach the service routine here
  test.setpoint(0, 11, 1);
  test.setpoint(1, 11, 3);
  test.setpoint(2, 11, 3);
  test.setpoint(3, 11, 2);
  test.setpoint(4, 11, 3);
  test.setpoint(5, 11, 1);
  test.setpoint(0, 10, 1);
  test.setpoint(1, 10, 1);
  test.setpoint(2, 10, 3);
  test.setpoint(3, 10, 1);
  test.setpoint(4, 10, 3);
  test.setpoint(5, 10, 1);
  test.setpoint(0, 9, 3);
  test.setpoint(1, 9, 1);
  test.setpoint(2, 9, 1);
  test.setpoint(3, 9, 1);
  test.setpoint(4, 9, 3);
  test.setpoint(5, 9, 1);
  test.setpoint(0, 8, 2);
  test.setpoint(1, 8, 2);
  test.setpoint(2, 8, 2);
  test.setpoint(3, 8, 3);
  test.setpoint(4, 8, 3);
  test.setpoint(5, 8, 1);
  test.setpoint(4, 7, 1);
  test.setpoint(5, 7, 1);
}
void loop() {
  while(test.gameover == false) {
    delay(10);
    tick++;
    left = digitalRead(LEFT);
    right = digitalRead(RIGHT);
    rleft = digitalRead(RLEFT);
    rright = digitalRead(RRIGHT);
    down = digitalRead(DOWN);
    if (oldleft == true && left == false) {
      oldleft = false;
    }
    if (oldright == true && right == false) {
      oldright = false;
    }
    if (oldrleft == true && rleft == false) {
      oldrleft = false;
    }
    if (oldrright == true && rright == false) {
      oldrright = false;
    }
    if (left == 1) {
      while (working == true) {
      }
      Timer1.stop();
      if (oldleft == false) {
        test.left();
        oldleft = true;
        oldright = false;
      }
      Timer1.resume();
    }
    if (right == 1) {
      while (working == true) {
      }
      Timer1.stop();
      if (oldright == false) {
        test.right();
        oldright = true;
        oldleft = false;
      }
      Timer1.resume();
    }
    if (rleft == 1) {
      while (working == true) {
      }
      Timer1.stop();
      if (oldrleft == false) {
        test.rotateleft();
        oldrleft = true;
        oldrright = false;
      }
      Timer1.resume();
    }
    if (rright == 1) {
      while (working == true) {
        }
      Timer1.stop();
      if (oldrright == false) {
        test.rotateright();
        oldrright = true;
        oldrleft = false;
      }
      Timer1.resume();
    }
    if (down == 1 && tick > 10) {
      while (working == true) {
        }
      Timer1.stop();
      test.down();
      drawmatrix();
      tick = 0;
      Timer1.resume();
    }
  }
}
  
void drawmatrix() {
  for (int j = 0; j < HEIGHT; j++) {
    for (int i =0; i < WIDTH; i++) {
      matrix.drawPixel(i, j, colors[test.checkpoint(i, j)]);
    }
  }
  matrix.show();
}
/*New solution, recursion uses too much memory
stick starting tile location in delete array
check directions around
if same color, add them to neighbor array
set a counter of how many neighbors were found
add 1 to number of same color
change tile to -1
while neighbor array isn't empty
  stack the tile location in delete array
  check directions around
  if same color, add them to the neighbor array
  add to counter of how many neighbors were found matching same color
  add 1 to number of same color
once neighbor array is empty
if number of same color is 4 or more
  run through delete array coordinates and change their values all to zero
  return number of same array
else return number of neighbors found
*/
int scan(int x, int y) {
  int delx[20], dely[20], arox[20], aroy[20]; //Delete arrays and neighbor arrays
  int delpos = 0, aropos = 0; //Index of the arrays
  int color = test.checkpoint(x, y);  //Store the color
  if (color == test.checkup(x, y)) { arox[aropos] = x; aroy[aropos] = y-1; aropos++; }    //Check if the adjacent adjacent enighbor has
  if (color == test.checkdown(x, y)) { arox[aropos] = x; aroy[aropos] = y+1; aropos++; }  //the same color or not. If it shares the color
  if (color == test.checkleft(x, y)) { arox[aropos] = x-1; aroy[aropos] = y; aropos++; }  //then add it to the neighbor array and increase
  if (color == test.checkright(x, y)) { arox[aropos] = x+1; aroy[aropos] = y; aropos++; } //the neighbor array size by one
  if (aropos == 0) { return 1; } //No neighbors were found, rip
  delx[delpos] = x; dely[delpos] = y; //Neighbors were found, add current coord to delete array
  delpos++; //Increase delete array size by one
  test.setpoint(x, y, -1);  //Set current point to -1 flag
  while (aropos > 0) {
    x = arox[aropos - 1]; y = aroy[aropos - 1]; //Change coord to last item in neighbor array
    aropos--; //"pop" the coord out of the neighbor array so other neighbors can overwrite it
    if (color == test.checkup(x, y)) { arox[aropos] = x; aroy[aropos] = y-1; aropos++; }
    if (color == test.checkdown(x, y)) { arox[aropos] = x; aroy[aropos] = y+1; aropos++; }
    if (color == test.checkleft(x, y)) { arox[aropos] = x-1; aroy[aropos] = y; aropos++; }
    if (color == test.checkright(x, y)) { arox[aropos] = x+1; aroy[aropos] = y; aropos++; }
    delx[delpos] = x; dely[delpos] = y; //Neighbors were found, add current coord to delete array
    delpos++; //Increase delete array size by one
    test.setpoint(x, y, -1);  //Set current point to -1 flag
  }
  for (int i = 0; i < delpos; i++) {
    test.setpoint(delx[i], dely[i], color); //Reset color to what it was before
  }
  if (delpos >= 4) { //Not enough neighbors were found {  //Enough neighbors were found
    for (int i = 0; i < delpos; i++) {
      test.setpoint(delx[i], dely[i], 0); //Set them to 0 to erase them
    }
  }
  return delpos;  //The size of this is the number of neighbors found?
}
//recursive function to scan through same colors
/*int scan(int x, int y, int sum) {
  sum++;
  int c = test.checkpoint(x, y);
  test.setpoint(x, y, -1);
  if (c == test.checkup(x, y)) {
    sum = sum + scan(x, y-1, sum);
  }
  if (c == test.checkdown(x, y)) {
    sum = sum + scan(x, y+1, sum);
  }
  if (c == test.checkleft(x, y)) {
    sum = sum + scan(x-1, y, sum);
  }
  if (c == test.checkright(x, y)) {
    sum = sum + scan(x+1, y, sum);
  }
  test.setpoint(x, y, c);
  return sum;
}
//Recursive function to erase the same ones found
void erase(int x, int y) {
  test.setpoint(x, y, 0);
  int c = test.checkpoint(x, y);
  if (c == test.checkup(x, y)) {
    erase(x, y-1);
  }
  if (c == test.checkdown(x, y)) {
    erase(x, y+1);
  }
  if (c == test.checkleft(x, y)) {
    erase(x-1, y);
  }
  if (c == test.checkright(x, y)) {
    erase(x+1, y);
  }
}
*/
bool fallable() {
  for (int j = HEIGHT - 1; j >= 0; j--) {
    for (int i = 0; i < WIDTH; i++) {
      if (test.checkpoint(i, j) != 0 && test.checkdown(i, j) == 0) {
        //Serial.print("falling!!\n");
        return true;
      }
    }
  }
  //Serial.print("falling done\n");
  return false;
}

void gameoverrun() {
  for (int j = HEIGHT; j >= 0; j--) {
    for (int i = 0; i < WIDTH; i++) {
      test.setpoint(i, j, 0);
      matrix.drawPixel(i, j, colors[0]);
      matrix.show();
      delay(10);
    }
  }
}

void Next() {
  Serial.print(test.score);
  Serial.print("\n");
  working = true;
  drawmatrix();
  /*Serial.print("loop: ");
  Serial.print(loopn);
  Serial.print("  puyodone = ");
  Serial.print(test.puyodone);
  Serial.print(" halfy = ");
  Serial.print(test.halfy);
  Serial.print("\n");*/
  loopn++;
  //Move puyo down
  if (test.puyodone == false && test.halfy == false) {
    /*Serial.print(test.pu.rot);
    Serial.print("\n");
    Serial.print(test.pu.x1);
    Serial.print(",");
    Serial.print(test.pu.y1);
    Serial.print(" ");
    Serial.print(test.pu.color1);
    Serial.print(test.checkpoint(test.pu.x1, test.pu.y1));
    Serial.print(" puyo 1\n");
    Serial.print(test.pu.x2);
    Serial.print(",");
    Serial.print(test.pu.y2);
    Serial.print(" ");
    Serial.print(test.pu.color2);    
    Serial.print(test.checkpoint(test.pu.x2, test.pu.y2));
    Serial.print(" puyo 2\n");*/
    test.down();
  }
  //Move everything down by one step if possible starting from bottom row
  else if (test.puyodone == true && test.halfy == false) { //only enter this if puyo is done moving
    if (test.freefalling == true) {
      for (int j = HEIGHT - 1; j >= 0; j--) {
        for (int i = 0; i < WIDTH; i++) {
          if (test.checkdown(i, j) == 0) {
            test.movedown(i, j);
          }
        }
      }
      if (fallable() == false) {
        test.freefalling = false;
      }
    }
    //Do checks across the entire board to see if any matches
    else {
      int count;
      int color;
      test.combofound = false;
      for (int j = HEIGHT - 1; j >= 0; j--) {
        for (int i =0; i < WIDTH; i++) {
          if (test.checkpoint(i, j) != 0) {
            count = scan(i, j);
            if (count > 3) { //if there are 4 or more, incriment combo and do score stuff
              test.combofound = true;
              test.combo++;
              test.score = test.score + test.combo * count;
            }
          }
        }
      }
      if (test.combofound == true) {
        test.freefalling = true;
      }
      else {
        //Serial.print("Resetting...\n");    
        if (toggle) {
          //Serial.print("Setting to false\n");
          toggle = false;
        }
        else {
          //Serial.print("Setting to true\n");
          toggle = true;
        }
        keepgoing = true;
        test.newpuyo(); //reset puyo to top
      }
    }
  }
  //one puyo got stuck during a down function call
  else if (test.halfy == true) {
    if (test.checkdown(test.tempx, test.tempy) == 0) { //there is space under the free puyo
      test.movedown(test.tempx, test.tempy); //move the pixel down
      test.tempy++; //adjust to compensate
    }
    else {                                   //no space under free puyo
      test.puyodone = true;                  //no more movement possible. move to check for combos
      test.halfy = false;
    }
  }
  working = false;
}

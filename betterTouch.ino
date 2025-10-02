
#include <UTouch.h>
#include <UTouchCD.h>
#include <UTFT.h>
//#include <UTFT_SdRaw.h>
#include <memorysaver.h>
#include <stdint.h>
#include <UTFT.h>
#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include <Ethernet.h>



#define SD_CHIP_SELECT 47


const int MAX_ARRAY_SIZE = 5;

struct workoutInfo {
  int user_id = -1;
  int wk_id = -1;
  double num_reps[6] = {0};
  double avg_heart_rate[6] = {0};
  double avg_vo2[6]= {0};
  double avg_force[6]= {0};
};

struct dailyWorkoutInfo {
  int day_id;
  struct workoutInfo wks[MAX_ARRAY_SIZE];
  String date;
};

struct monthlyWorkoutInfo {
  int month_id;
  struct dailyWorkoutInfo day_wks[MAX_ARRAY_SIZE];
  String month;
  String year;
};

struct allWorkoutInfo {
  struct monthlyWorkoutInfo mon_wks[MAX_ARRAY_SIZE];
};

uint8_t addr  = 0x5d;  //CTP IIC ADDRESS
// Declare which fonts we will be using
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t SevenSegNumFont[];
extern uint8_t Arial_round_16x24[];
extern uint8_t GroteskBold32x64[];
extern uint8_t Grotesk24x48[];
extern uint8_t Grotesk32x64[];

// Set the pins to the correct ones for your development shield
// Standard Arduino Mega/Due shield       

#define GT9271_RESET 41   //CTP RESET
#define GT9271_INT   48   //CTP  INT

UTFT myGLCD(SSD1963_800480,38,39,40,41);  //(byte model, int RS, int WR, int CS, int RST)
int xMax, yMax; //800, 480
int userID;
int xStart, xEnd;
int yStart, yEnd;
uint32_t thatTouchTime = 0;
int currentInput = 0;
String test = "";
char pos = 0;
char t;
uint32_t totalWorkoutTime;
uint32_t pausedTime;
uint32_t legReps = 0;
uint32_t rowWorkoutTime;
uint32_t timeElapsed;
bool pauseMainTimer = false;
bool pauseRowTimer = false;
bool paused = false;
int cur_workout = 0;
int cur_day_workout = 0;
int cur_month_workout = 0;
struct allWorkoutInfo all_wks;
int cur_reps = 0;
int cur_hr = 0;
int cur_vo2 = 0;
int cur_force = 0; 
int pre_reps = 0;
int pre_hr = 0;
int pre_vo2 = 0;
int pre_force = 0;
int ref_reps = 0;
bool parsing = false;

enum screen {
  LOGIN,
  SIGNUP,
  MAIN,
  HIST,
  WORK,
  WORKLEG, // up to here implemented
  WORKCALF,
  WORKSHLDR,
  WORKBI,
  WORKHAND,
  WORKROW,
  END,
  DAILY,
};

int workout = -1;

static screen currentScreen = LOGIN;
static screen previousScreen;

unsigned char  GTP_CFG_DATA[] =
{

0x00,0x20,0x03,0xE0,0x01,0x0A,0x0D,0x00,
0x01,0x0A,0x28,0x0F,0x50,0x32,0x03,0x08,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x8E,0x2E,0x88,0x23,0x21,
0x31,0x0D,0x00,0x00,0x00,0x01,0x03,0x1D,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x1E,0x50,0x94,0xC5,0x02,
0x07,0x00,0x00,0x04,0x80,0x21,0x00,0x6B,
0x28,0x00,0x59,0x31,0x00,0x4B,0x3B,0x00,
0x3F,0x48,0x00,0x3F,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x15,0x14,0x11,0x10,0x0F,0x0E,0x0D,0x0C,
0x09,0x08,0x07,0x06,0x05,0x04,0x01,0x00,
0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,
0x04,0x06,0x07,0x08,0x0A,0x0C,0x0D,0x0F,
0x10,0x11,0x12,0x13,0x19,0x1B,0x1C,0x1E,
0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,
0x27,0x28,0xFF,0xFF,0xFF,0xFF,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x4D,0x01
};

struct TouchLocation
{
  uint16_t x;
  uint16_t y;
};

TouchLocation touchLocations[10];

uint8_t GT9271_Send_Cfg(uint8_t * buf,uint16_t cfg_len);
void writeGT9271TouchRegister( uint16_t regAddr,uint8_t *val, uint16_t cnt);
uint8_t readGT9271TouchAddr( uint16_t regAddr, uint8_t * pBuf, uint8_t len );
uint8_t readGT9271TouchLocation( TouchLocation * pLoc, uint8_t num );

uint8_t buf[80];



uint8_t GT9271_Send_Cfg(uint8_t * buf,uint16_t cfg_len)
{
	//uint8_t ret=0;
	uint8_t retry=0;
	for(retry=0;retry<2;retry++)
	{
		writeGT9271TouchRegister(0x8047,buf,cfg_len);
		//if(ret==0)break;
		delay(10);	 
	}
	//return ret;
}

void writeGT9271TouchRegister( uint16_t regAddr,uint8_t *val, uint16_t cnt)
{	uint16_t i=0;
  Wire.beginTransmission(addr);
   Wire.write( regAddr>>8 );  // register 0
  Wire.write( regAddr);  // register 0 
	for(i=0;i<cnt;i++,val++)//data
	{		
          Wire.write( *val );  // value
	}
  uint8_t retVal = Wire.endTransmission(); 
}

uint8_t readGT9271TouchAddr( uint16_t regAddr, uint8_t * pBuf, uint8_t len )
{
  Wire.beginTransmission(addr);
  Wire.write( regAddr>>8 );  // register 0
  Wire.write( regAddr);  // register 0  
  uint8_t retVal = Wire.endTransmission();
  
  uint8_t returned = Wire.requestFrom(addr, len);    // request 1 bytes from slave device #2
  
  uint8_t i;
  for (i = 0; (i < len) && Wire.available(); i++)
  
  {
    pBuf[i] = Wire.read();
    //Serial.print(pBuf[i]);
  }
  
  return i;
}

uint8_t readGT9271TouchLocation( TouchLocation * pLoc, uint8_t num )
{
  uint8_t retVal;
  uint8_t i;
  uint8_t k;
  uint8_t  ss[1];
  
  do
  {  
    
    if (!pLoc) break; // must have a buffer
    if (!num)  break; // must be able to take at least one
     ss[0]=0;
      readGT9271TouchAddr( 0x814e, ss, 1);
      uint8_t status=ss[0];

    if ((status & 0x0f) == 0) break; // no points detected
    uint8_t hitPoints = status & 0x0f;
    
    // Serial.print("number of hit points = ");
    // Serial.println( hitPoints );
    
   uint8_t tbuf[32]; uint8_t tbuf1[32];uint8_t tbuf2[16];  
    readGT9271TouchAddr( 0x8150, tbuf, 32);
    readGT9271TouchAddr( 0x8150+32, tbuf1, 32);
    readGT9271TouchAddr( 0x8150+64, tbuf2,16);
    
      if(hitPoints<=4)
            {   
              for (k=0,i = 0; (i <  4*8)&&(k < num); k++, i += 8)
              {
                pLoc[k].x = tbuf[i+1] << 8 | tbuf[i+0];
                pLoc[k].y = tbuf[i+3] << 8 | tbuf[i+2];
              }   
            }
        if(hitPoints>4)   
            {  
               for (k=0,i = 0; (i <  4*8)&&(k < num); k++, i += 8)
              {
                pLoc[k].x = tbuf[i+1] << 8 | tbuf[i+0];
                pLoc[k].y = tbuf[i+3] << 8 | tbuf[i+2];
              }               
              
              for (k=4,i = 0; (i <  4*8)&&(k < num); k++, i += 8)
              {
                pLoc[k].x = tbuf1[i+1] << 8 | tbuf1[i+0];
                pLoc[k].y = tbuf1[i+3] << 8 | tbuf1[i+2];
              }   
            } 
            
          if(hitPoints>8)   
            {  
                for (k=0,i = 0; (i <  4*8)&&(k < num); k++, i += 8)
              {
                pLoc[k].x = tbuf[i+1] << 8 | tbuf[i+0];
                pLoc[k].y = tbuf[i+3] << 8 | tbuf[i+2];
              }               
              
              for (k=4,i = 0; (i <  4*8)&&(k < num); k++, i += 8)
              {
                pLoc[k].x = tbuf1[i+1] << 8 | tbuf1[i+0];
                pLoc[k].y = tbuf1[i+3] << 8 | tbuf1[i+2];
              }                          
             
              for (k=8,i = 0; (i <  2*8)&&(k < num); k++, i += 8)
              {
                pLoc[k].x = tbuf2[i+1] << 8 | tbuf2[i+0];
                pLoc[k].y = tbuf2[i+3] << 8 | tbuf2[i+2];
              }   
            }       

            
    
    retVal = hitPoints;
    
  } while (0);
  
    ss[0]=0;
    writeGT9271TouchRegister( 0x814e,ss,1); 
    delay(2); 
  return retVal;
}

void setRefReps(){
  // if (Serial1.available()){
  //   //message[pos] = Serial1.read();
  //   message = Serial1.read();
  //   String mes = String(message);
  //   ref_reps = mes.toInt();
  //   pre_reps = ref_reps;
  // }
}

void updateData(){
  if (Serial3.available()){
    Serial.println("3");
    Serial.println(Serial3.read());
  //   message = Serial1.read();
  //   cur_reps = message.toInt();
  //   if (cur_reps != pre_reps) {
  //     cur_reps = cur_reps - ref_reps;
  //     message = String(cur_reps)
  //     Serial.println(message);
  //     myGLCD.print(message, 365, 325);
  //     pre_reps = cur_reps;
  //   }
  }  
  if (Serial1.available()){
    Serial.println("serial 1");
    parsing = true;
    while (parsing){
      t = Serial1.read();
      test += t;
      if (test[pos] == 'e') {
        test.remove(pos);
        Serial.println(test);
        myGLCD.print(test, 365, 325);
        test = "";
        pos = 0;
        parsing = false;
      }
      else pos++;
    }
  //   // message = Serial1.read();
  //   // String mes = String(message);
  //   // cur_reps = mes.toInt();
  //   // if (cur_reps != pre_reps) {
  //   //   cur_reps = cur_reps - ref_reps;
  //   //   mes = String(cur_reps);
  //   //   Serial.println(mes);
  //   //   myGLCD.print(mes, 365, 325);
  //   //   pre_reps = cur_reps;
  //   // }
  }
  
  if (Serial2.available()){
    Serial.println("serial 2");
    parsing = true;
    while (parsing) {
    t = Serial2.read();
    test += t;
    if (test[pos] == 'e') {
      test.remove(pos);
      Serial.println(test);
      myGLCD.print(test, 365, 415);
      test = "";
      pos = 0;
      parsing = false;
    }
    else pos++;
    // message = Serial2.read();
    // cur_force = message.toInt();
    // if (cur_force != pre_force) {
    //   message = String(cur_force);
    //   Serial.println(message);
    //   myGLCD.print(message, 365, 395);
    //   pre_force = cur_force;
    }
  }
}


void drawBorder(int xStart, int yStart, int xEnd, int yEnd)
{
  myGLCD.setColor(0, 0, 0);
  for (int i = -2; i <= 2; ++i){
      myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
  }
}

void drawNumButtons()
{
    myGLCD.setFont(SevenSegNumFont); //32x50
    myGLCD.setBackColor(217,217,217);
    int currentNum = 1;
  //Draw button shapes
  for (int i = 1; i <= 8; i = i + 2){
    xStart = 40 + 80*i;
    xEnd = xStart + 80;
    yStart = 280;
    yEnd = 440;
    myGLCD.setColor(217,217,217);
    myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
    myGLCD.setColor(0,0,0);
    //Thicken Borders
    for (int i = -2; i <= 2; ++i){
      myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
    }
    if (currentNum == 1) myGLCD.printNumI(currentNum, xStart + 16, yStart + 55);
    else myGLCD.printNumI(currentNum, xStart + 24, yStart + 55);
    currentNum++;
  }
}

void drawLoginButtons()
{
  myGLCD.setFont(Arial_round_16x24);
  //combo: 400x80
  xStart = 120;
  xEnd = 520;
  yStart = 40;
  yEnd = 120;
  myGLCD.setColor(217, 217, 217);
  myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
  myGLCD.setColor(0, 0, 0);
  for (int i = -2; i <= 2; ++i){
      myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
  }
  myGLCD.setColor(0, 0, 255);
  myGLCD.fillRect(xEnd - 80, yStart, xEnd, yEnd);
  myGLCD.setColor(0, 0, 0);
  for (int i = -2; i <= 2; ++i){
      myGLCD.drawRect(xEnd - 80 + i, yStart + i, xEnd - i, yEnd - i);
  }
  myGLCD.setBackColor(0, 0, 255);
  myGLCD.print("<-", xEnd - 80 + 16, yStart + 28);
  //Login: 120x80
  xStart = xEnd + 40;
  xEnd = xStart + 120;
  myGLCD.setColor(217,217,217);
  myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
  myGLCD.setColor(0, 0, 0);
  for (int i = -2; i <= 2; ++i){
      myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
  }
  myGLCD.setBackColor(217, 217, 217);
  myGLCD.print("Login", xStart + 24, yStart + 28);
  //login: 240x80
  xStart = 280;
  xEnd = xStart + 240;
  yStart = 160;
  yEnd = yStart + 80;
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
  myGLCD.setColor(0, 0, 0);
  for (int i = -2; i <= 2; ++i){
      myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
  }
  myGLCD.setBackColor(0, 255, 0);
  myGLCD.print("Sign Up", xStart + 24, yStart + 28);
}

void drawLoginScreen()
{
  myGLCD.clrScr();
  myGLCD.fillScr(255, 255, 255);
  drawNumButtons();
  drawLoginButtons();
  myGLCD.setBackColor(217, 217, 217);

}

void drawUserIDCornerButton(){
  xStart = 20;
  xEnd = xStart + 200;
  yStart = 20;
  yEnd = yStart + 60;
  myGLCD.setColor(217, 217, 217);
  myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
  myGLCD.setColor(0, 0, 0);
  for (int i = -2; i <= 2; ++i){
      myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
  }
  myGLCD.setBackColor(217, 217, 217);
  String userText = "User: " + String(userID);
  myGLCD.print(userText, xStart + 12, yStart + 20);
}

void drawLogoutButton()
{
  xStart = 580;
  xEnd = xStart + 200;
  yStart = 20;
  yEnd = yStart + 60;
  myGLCD.setColor(0, 0, 255);
  myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
  myGLCD.setColor(0, 0, 0);
  for (int i = -2; i <= 2; ++i){
      myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
  }
  myGLCD.setBackColor(0, 0, 255);
  myGLCD.print("Log out", xStart + 42, yStart + 20);
}

void drawMainScreen() 
{
  myGLCD.setFont(Arial_round_16x24);
  myGLCD.clrScr();
  myGLCD.fillScr(255, 255, 255);
  drawUserIDCornerButton();
  drawLogoutButton();
  //Draw Main Labels
  xStart = 200;
  xEnd = xStart + 400;
  for (size_t i = 0; i < 3; ++i){
    yStart = 120 + 120*i;
    yEnd = yStart + 100;
    myGLCD.setColor(217, 217, 217);
    myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
    myGLCD.setColor(0, 0, 0);
    for (int i = -2; i <= 2; ++i){
        myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
    }
    myGLCD.setBackColor(217, 217, 217);
    switch(i){
      case 0: myGLCD.print("History", xStart + 140, yStart + 40);
              break;
      case 1: myGLCD.print("Workout", xStart + 140, yStart + 40);
              break;
      case 2: myGLCD.print("Diagnostic", xStart + 120, yStart + 40);
              break;
    }

  }

}

void drawBackButton()
{
  xStart = 660;
  xEnd = xStart + 120;
  yStart = 20;
  yEnd = yStart + 60;
  myGLCD.setColor(217, 217, 217);
  myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
  myGLCD.setColor(0, 0, 0);
  for (int i = -2; i <= 2; ++i){
      myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
  }
  myGLCD.setBackColor(217, 217, 217);
  myGLCD.print("Back", xStart + 28, yStart + 20);
}

/*
void drawStageButton()
{
  xStart = 240;
  xEnd = xStart + 200;
  yStart = 20;
  yEnd = yStart + 60;
  myGLCD.setColor(217, 217, 217);
  myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
  myGLCD.setColor(0, 0, 0);
  for (int i = -2; i <= 2; ++i){
      myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
  }
  myGLCD.setBackColor(217, 217, 217);
  String userText = "Stage: ####";
  myGLCD.print(userText, xStart + 12, yStart + 20);
}
*/

void drawBigPauseButton()
{
  xStart = 600;
  xEnd = xStart + 180;
  yStart = 280;
  yEnd = yStart + 180;
  myGLCD.setColor(0, 0, 255);
  myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
  myGLCD.setColor(0, 0, 0);
  for (int i = -2; i <= 2; ++i){
      myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
  }
  myGLCD.setColor(255, 255, 255);
  myGLCD.fillRect(xStart + 50, yStart + 40, xStart + 80, yStart + 140);
  myGLCD.fillRect(xStart + 100, yStart + 40, xStart + 130, yStart + 140);
}

void drawTriangle(int x1, int y1, int x2, int y2, int x3, int y3){
  myGLCD.drawLine(x1, y1, x2, y2);
  myGLCD.drawLine(x2, y2, x3, y3);
  myGLCD.drawLine(x3, y3, x1, y1);
}

void drawPlayButton() {
  xStart = 600;
  xEnd = xStart + 180;
  yStart = 280;
  yEnd = yStart + 180;
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
  myGLCD.setColor(0, 0, 0);
  for (int i = -2; i <= 2; ++i){
      myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
  }
  myGLCD.setColor(255, 255, 255);
  int v1x = 650;
  int v1y = 320;
  int v2x = 650;
  int v2y = 420;
  int v3x = 737;
  int v3y = 370;
  for (int i = 0; i <= 50; ++i){
    drawTriangle(v1x, v1y + i, v2x, v2y - i, v3x, v3y);
  }
}

/*
void drawRepBasedWorkoutScreen()
{
  myGLCD.setFont(Arial_round_16x24);
  myGLCD.clrScr();
  myGLCD.fillScr(255,255,255);
  drawUserIDCornerButton();
  //drawStageButton();
  drawBackButton();
  drawBigPauseButton();
  //Draw Main Labels
  xStart = 20;
  xEnd = xStart + 420;
  for (size_t i = 0; i < 4; ++i){
    yStart = 120 + 90*i;
    yEnd = yStart + 70;
    myGLCD.setColor(217, 217, 217);
    myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
    myGLCD.setColor(0, 0, 0);
    for (int i = -2; i <= 2; ++i){
        myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
    }
    myGLCD.setBackColor(217, 217, 217);
    switch(i){
      case 0: myGLCD.print("Heart Rate: ", xStart + 20, yStart + 25);
              myGLCD.print("####", xStart + 345, yStart + 25);
              break;
      case 1: myGLCD.print("VO2 Max: ", xStart + 20, yStart + 25);
              myGLCD.print("####", xStart + 345, yStart + 25);
              break;
      case 2: myGLCD.print("Num Reps: ", xStart + 20, yStart + 25);
              myGLCD.print("####", xStart + 345, yStart + 25);
              break;
      case 3: myGLCD.print("Muscle Strength: ", xStart + 20, yStart + 25);
              myGLCD.print("####", xStart + 345, yStart + 25);
              break;
      default: break;
    }
  }

  xStart = 660;
  xEnd = xStart + 120;
  yStart = 120;
  yEnd = yStart + 60;
  myGLCD.setColor(217, 217, 217);
  myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
  myGLCD.setColor(0, 0, 0);
  for (int i = -2; i <= 2; ++i){
      myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
  }
  myGLCD.setBackColor(217, 217, 217);
  myGLCD.print("####", xStart + 28, yStart + 20);
}
*/

void drawHistoryScreen()
{
  myGLCD.setFont(Arial_round_16x24);
  myGLCD.clrScr();
  myGLCD.fillScr(255,255,255);
  drawUserIDCornerButton();
  drawBackButton();
  //Draw Main Labels
  xStart = 200;
  xEnd = xStart + 400;
  for (size_t i = 0; i < 3; ++i){
    yStart = 120 + 120*i;
    yEnd = yStart + 100;
    myGLCD.setColor(217, 217, 217);
    myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
    myGLCD.setColor(0, 0, 0);
    for (int i = -2; i <= 2; ++i){
        myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
    }
    myGLCD.setBackColor(217, 217, 217);
    switch(i){
      case 0: myGLCD.print("Daily History", xStart + 110, yStart + 40);
              break;
      case 1: myGLCD.print("Monthly History", xStart + 90, yStart + 40);
              break;
      case 2: myGLCD.print("All-time History", xStart + 80, yStart + 40);
              break;
    }
  }
}

/*
void drawTimeBasedWorkoutScreen()
{
  myGLCD.setFont(Arial_round_16x24);
  myGLCD.clrScr();
  myGLCD.fillScr(255,255,255);
  drawUserIDCornerButton();
  //drawStageButton();
  drawBackButton();
  drawBigPauseButton();
  //Draw Main Labels
  xStart = 20;
  xEnd = xStart + 420;
  for (size_t i = 0; i < 4; ++i){
    yStart = 120 + 90*i;
    yEnd = yStart + 70;
    myGLCD.setColor(217, 217, 217);
    myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
    myGLCD.setColor(0, 0, 0);
    for (int i = -2; i <= 2; ++i){
        myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
    }
    myGLCD.setBackColor(217, 217, 217);
    switch(i){
      case 0: myGLCD.print("Heart Rate: ", xStart + 20, yStart + 25);
              myGLCD.print("####", xStart + 345, yStart + 25);
              break;
      case 1: myGLCD.print("VO2 Max: ", xStart + 20, yStart + 25);
              myGLCD.print("####", xStart + 345, yStart + 25);
              break;
      case 2: myGLCD.print("Num Reps: ", xStart + 20, yStart + 25);
              myGLCD.print("####", xStart + 345, yStart + 25);
              break;
      case 3: myGLCD.print("Muscle Strength: ", xStart + 20, yStart + 25);
              myGLCD.print("####", xStart + 345, yStart + 25);
              break;
      default: break;
    }
  }

  xStart = 660;
  xEnd = xStart + 120;
  yStart = 120;
  yEnd = yStart + 60;
  myGLCD.setColor(217, 217, 217);
  myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
  myGLCD.setColor(0, 0, 0);
  for (int i = -2; i <= 2; ++i){
      myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
  }
  myGLCD.setBackColor(217, 217, 217);
  myGLCD.print("00:00", xStart + 20, yStart + 20);
}
*/

void drawNewAccountScreen()
{
  myGLCD.clrScr();
  myGLCD.fillScr(255, 255, 255);
  //set font
  myGLCD.setFont(Arial_round_16x24);
  drawBackButton();
  //Combo display text coordinates:
  xStart = 200;
  xEnd = 600;
  yStart = 40;
  yEnd = 120;
  //make gray box
  myGLCD.setColor(217, 217, 217);
  myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
  //create black outline for box above
  myGLCD.setColor(0, 0, 0);
  drawBorder(xStart, yStart, xEnd, yEnd);
  // for (int i = -2; i <= 2; ++i){
  //     myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
  // }
  //set font + color to print text
  myGLCD.setBackColor(217, 217, 217);
  myGLCD.setFont(Arial_round_16x24);
  myGLCD.print("Your Code Is:", 300, 68);

  //code for combination num:
  //generate new combination
  int dig1 = random(1, 5);
  int dig2 = random(1, 5);
  int dig3 = random(1, 5);
  int dig4 = random(1, 5);
  //create rectangle for combination display
  xStart = 200;
  xEnd = 600;
  yStart = 160;
  yEnd = 300;
  //make gray box
  myGLCD.setColor(217, 217, 217);
  myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
  //create black outline for box above
  myGLCD.setColor(0, 0, 0);
  drawBorder(xStart, yStart, xEnd, yEnd);
  // for (int i = -2; i <= 2; ++i){
  //     myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
  // }
  //set font + color to print text
  myGLCD.setBackColor(217, 217, 217);
  //print combination
  myGLCD.setFont(Grotesk32x64);
  myGLCD.printNumI(dig1, xStart + 70, yStart + 35);
  myGLCD.printNumI(dig2, xStart + 145, yStart + 35);
  myGLCD.printNumI(dig3, xStart + 220, yStart + 35);
  myGLCD.printNumI(dig4, xStart + 300, yStart + 35);

  //confirm:
  xStart = 40;
  xEnd = xStart + 240;
  yStart = yEnd + 40;
  yEnd = yStart + 100;
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
  myGLCD.setColor(0, 0, 0);
  drawBorder(xStart, yStart, xEnd, yEnd);
  // for (int i = -2; i <= 2; ++i){
  //     myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
  // }
  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setFont(Arial_round_16x24);
  myGLCD.print("Confirm", xStart + 60, yStart + 35);

  //deny: 
  xStart = xEnd + 240;
  xEnd = xStart + 240;
  
  myGLCD.setColor(0, 0, 255);
  myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
  myGLCD.setColor(0, 0, 0);
  drawBorder(xStart, yStart, xEnd, yEnd);
  // for (int i = -2; i <= 2; ++i){
  //     myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
  // }
  myGLCD.setBackColor(0, 0, 255);
  myGLCD.setFont(Arial_round_16x24);
  myGLCD.print("Refresh", xStart + 85, yStart + 35);
}

void drawInsertThingConfirmation()
{
  myGLCD.clrScr();
  myGLCD.fillScr(255, 255, 255);
  //set font
  myGLCD.setFont(Arial_round_16x24);
  //Combo display text coordinates:
  xStart = 180;
  xEnd = 620;
  yStart = 40;
  yEnd = 160;
  //make gray box
  myGLCD.setColor(217, 217, 217);
  myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
  //create black outline for box above
  myGLCD.setColor(0, 0, 0);
  drawBorder(xStart, yStart, xEnd, yEnd);
  // for (int i = -2; i <= 2; ++i){
  //     myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
  // }

  //set font + color to print text
  myGLCD.setBackColor(217, 217, 217);
  myGLCD.setFont(Arial_round_16x24);
  myGLCD.print("Is [insert thing] true?", xStart + 40, yStart + 50);

  //confirm: 
  xStart = 60;
  xEnd = xStart + 240;
  yStart = yEnd + 120;
  yEnd = yStart + 120;
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
  myGLCD.setColor(0, 0, 0);
  drawBorder(xStart, yStart, xEnd, yEnd);
  // for (int i = -2; i <= 2; ++i){
  //     myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
  // }
  myGLCD.setBackColor(0, 255, 0);
  myGLCD.setFont(Arial_round_16x24);
  myGLCD.print("Confirm", xStart + 55, yStart + 45);

  //deny: 
  xStart = xEnd + 210;
  xEnd = xStart + 240;
  
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
  myGLCD.setColor(0, 0, 0);
  drawBorder(xStart, yStart, xEnd, yEnd);
  // for (int i = -2; i <= 2; ++i){
  //     myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
  // }
  myGLCD.setBackColor(255, 0, 0);
  myGLCD.setFont(Arial_round_16x24);
  myGLCD.print("Deny", xStart + 80, yStart + 45);
}

void drawWorkoutTable()
{
  //Table
  xStart = 50;
  xEnd = xStart + 700;
  yStart = 100;
  yEnd = yStart + 350;
  myGLCD.setColor(217, 217, 217);
  myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
  drawBorder(xStart, yStart, xEnd, yEnd);
  myGLCD.setColor(0, 0, 0);
  
  //column separators
  for (int i = 1; i < 5; i++) {
    myGLCD.fillRect(xStart, yStart + (i * 70), xEnd, yStart + 5 + (i * 70));
  }
  //row separators
  for (int i = 1; i < 6; i++) {
    myGLCD.fillRect(xStart + 100 + (100 * i), yStart, xStart + 105 + (100 * i), yEnd);
  }
  myGLCD.setBackColor(217, 217, 217);
  myGLCD.setFont(Arial_round_16x24);
  for (int i = 0; i < 4; i++) {
    switch(i) {
      case(0):
        myGLCD.print("HR Avg   :", xStart + 25, yStart + 95 + (70 * i));
        break;
      case(1):
        myGLCD.print("Avg VO2  :", xStart + 25, yStart + 95 + (70 * i));
        break;
      case(2):
        myGLCD.print("Num Reps :", xStart + 25, yStart + 95 + (70 * i));
        break;
      case(3):
        myGLCD.print("Avg Power:", xStart + 25, yStart + 95 + (70 * i));
        break;
    }
  }
  for (int i = 0; i < 5; i++){
    switch(i) {
      case(0):
        myGLCD.print("1", xStart + 245 + (100 * i), yStart + 25);
        break;
      case(1):
        myGLCD.print("2", xStart + 245 + (100 * i), yStart + 25);
        break;
      case(2):
        myGLCD.print("3", xStart + 245 + (100 * i), yStart + 25);
        break;
      case(3):
        myGLCD.print("4", xStart + 245 + (100 * i), yStart + 25);
        break;
      case(4):
        myGLCD.print("5", xStart + 245 + (100 * i), yStart + 25);
        break;
    }
  }
}

void drawBackButton2()
{
  xStart = 553;
  xEnd = xStart + 197;
  yStart = 15;
  yEnd = yStart + 60;
  myGLCD.setColor(217, 217, 217);
  myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
  drawBorder(xStart, yStart, xEnd, yEnd);
  myGLCD.print("Back", xStart + 70, yStart + 20);
}

void drawWorkoutID(){
  xStart = 50;
  xEnd = xStart + 300;
  yStart = 15;
  yEnd = yStart + 60;
  myGLCD.setColor(217, 217, 217);
  myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
  drawBorder(xStart, yStart, xEnd, yEnd);
  myGLCD.setBackColor(217, 217, 217);
  myGLCD.setFont(Arial_round_16x24);
  myGLCD.print("Workout ID:1234", xStart + 30, yStart + 20);
}

void drawPostWorkout()
{
  myGLCD.clrScr();
  myGLCD.fillScr(255, 255, 255);
  //Workout ID:
  drawWorkoutID();
  //back
  drawBackButton2();
  //Table
  drawWorkoutTable();
}

void drawDailyHistoryScreen(){
  myGLCD.setFont(Arial_round_16x24);
  myGLCD.clrScr();
  myGLCD.fillScr(255, 255, 255);
  drawUserIDCornerButton();
  drawBackButton();
  myGLCD.setBackColor(255, 255, 255);
  myGLCD.print("Daily History", 290, 40);
  xStart = 100;
  xEnd = 700;
  yStart = 120;
  yEnd = 420;
  myGLCD.setColor(217, 217, 217);
  myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
  drawBorder(xStart, yStart, xEnd, yEnd);
  for (int i = -2; i <= 2; ++i){
    myGLCD.drawLine(400 + i, yStart, 400 + i, yEnd);
  }
  for (int i = 0; i < 4; ++i){
    yStart = yStart + 60;
    for (int i = -2; i <= 2; ++i){
    myGLCD.drawLine(xStart, yStart + i, xEnd, yStart + i);
    }
  }

}

void drawEndButton() {
  xStart = 660;
  xEnd = xStart + 120;
  yStart = 400;
  yEnd = yStart + 60;
  myGLCD.setColor(0, 0, 255);
  myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
  myGLCD.setColor(0, 0, 0);
  for (int i = -2; i <= 2; ++i){
      myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
  }
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(0, 0, 255);
  myGLCD.print("End", xStart + 30, yStart + 20);
  return;

}

void drawMainClock() {
  xStart = 640;
  xEnd = xStart + 140;
  yStart = 100;
  yEnd = yStart + 60;
  myGLCD.setColor(217, 217, 217);
  myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
  myGLCD.setColor(0, 0, 0);
  for (int i = -2; i <= 2; ++i){
      myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
  }
}

void drawSecondaryClock() {
  xStart = 530;
  xEnd = xStart + 240;
  yStart = 180;
  yEnd = yStart + 80;
  myGLCD.setColor(217, 217, 217);
  myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
  myGLCD.setColor(0, 0, 0);
  for (int i = -2; i <= 2; ++i){
      myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
  }
  myGLCD.print("Rowing Timer", 557, 190);
}


void updateClock(int clockNum, uint32_t currentTime, uint32_t baseTime) {
  int timeElapsed = currentTime - baseTime;
  int target = 7200000;
  if (clockNum == 2) {
    target = 2700000;
  }
  int passed = target - timeElapsed;
  if (passed < 0) return;
  String hours = String(passed/3600000);
  String minutes = String((passed/60000) % 60);
  int seconds = (passed/1000) % 60;
  if (seconds % 2 == 0 && clockNum == 1 && currentScreen != WORK) updateData();
  // Serial.println(seconds);
  myGLCD.setColor(0,0,0);
  String timeOutput;
  if (int(seconds) < 10) timeOutput = "0" + hours + ":" + minutes + ":" + "0" + String(seconds);
  else timeOutput = "0" + hours + ":" + minutes + ":" + String(seconds);
  switch(clockNum) {
    case 1: 
      myGLCD.print(timeOutput, 647, 120);
      break;
    case 2:
      myGLCD.print(timeOutput, 577, 220);
      break;
  }
  return;
}

void elapsedTime(uint32_t currentTime, uint32_t baseTime) {
  timeElapsed = currentTime - baseTime;
}


void drawMainWorkoutScreen() {
  myGLCD.clrScr();
  myGLCD.fillScr(255, 255, 255);
  myGLCD.setFont(Arial_round_16x24);
  drawUserIDCornerButton();
  drawEndButton();
  drawMainClock();
  drawBackButton();
  myGLCD.setColor(0,0,0);
  myGLCD.setBackColor(255,255,255);
  myGLCD.print("Choose a Workout:", 300, 40);
  xStart = 20;
  xEnd = xStart + 420;
  myGLCD.setBackColor(217, 217, 217);
  //myGLCD.print("00:00:00",637,40);
  for (size_t i = 0; i < 6; ++i){
    yStart = 120 + 60*i;
    yEnd = yStart + 50;
    myGLCD.setColor(217, 217, 217);
    myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
    myGLCD.setColor(0, 0, 0);
    for (int i = -2; i <= 2; ++i){
        myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
    }
    switch(i){
      case 0: myGLCD.print("Leg Press", xStart + 20, yStart + 15);
              break;
      case 1: myGLCD.print("Calf Raise", xStart + 20, yStart + 15);
              break;
      case 2: myGLCD.print("Delt Fly/Lateral Raise", xStart + 20, yStart + 15);
              break;
      case 3: myGLCD.print("Bicep Curl", xStart + 20, yStart + 15);
              break;
      case 4: myGLCD.print("Hand Squeeze", xStart + 20, yStart + 15);
              break;
      case 5: myGLCD.print("Rowing", xStart + 20, yStart + 15);
              break;
              
      default: break;
    }
  }
}

void drawRepBasedWorkoutScreen() 
{
  myGLCD.clrScr();
  myGLCD.fillScr(255, 255, 255);
  myGLCD.setFont(Arial_round_16x24);
  drawUserIDCornerButton();
  drawMainClock();
  if (currentScreen == WORKROW) {
    drawSecondaryClock();
  }
  drawBigPauseButton();
  drawBackButton();
  myGLCD.setColor(0,0,0);
  myGLCD.setBackColor(255,255,255);
  switch (currentScreen){
    case WORKLEG:
      myGLCD.print("Leg Press", 300, 40);
      break;
    case WORKCALF:
      myGLCD.print("Calf Workout", 300, 40);
      break;
    case WORKSHLDR:
      myGLCD.print("Shoulder Workout", 300, 40);
      break;
    case WORKBI:
      myGLCD.print("Bicep Workout", 300, 40);
      break;
    case WORKHAND:
      myGLCD.print("Hand Workout", 300, 40);
      break;
    case WORKROW:
      myGLCD.print("Rowing", 300, 40);
      break;
  }
  xStart = 20;
  xEnd = xStart + 420;
  myGLCD.setBackColor(217, 217, 217);
  //myGLCD.print("00:00:00",637,40);
  xStart = 20;
  xEnd = xStart + 420;
  for (size_t i = 0; i < 4; ++i){
    yStart = 120 + 90*i;
    yEnd = yStart + 70;
    myGLCD.setColor(217, 217, 217);
    myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
    myGLCD.setColor(0, 0, 0);
    for (int i = -2; i <= 2; ++i){
        myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i); 
    }
    myGLCD.setBackColor(217, 217, 217);
    switch(i){
      case 0: myGLCD.print("Heart Rate: ", xStart + 20, yStart + 25);
              myGLCD.print("####", xStart + 345, yStart + 25);
              break;
      case 1: myGLCD.print("VO2 Max: ", xStart + 20, yStart + 25);
              myGLCD.print("####", xStart + 345, yStart + 25);
              break;
      case 2: myGLCD.print("Num Reps: ", xStart + 20, yStart + 25);
              myGLCD.print("0", xStart + 345, yStart + 25);
              break;
      case 3: myGLCD.print("Muscle Strength: ", xStart + 20, yStart + 25);
              break;
      default: break;
    }
  }
}

void drawWorkoutSetupScreen() {
  return;
}

void drawEndWorkoutScreen() {
  myGLCD.clrScr();
  myGLCD.fillScr(255, 255, 255);
  myGLCD.setFont(Arial_round_16x24);
  myGLCD.setColor(217, 217, 217);
  xStart = 200;
  xEnd = xStart + 400;
  yStart = 100;
  yEnd = yStart + 200;
  myGLCD.fillRect(xStart, yStart, xEnd, yEnd);
  myGLCD.setColor(0, 255, 0);
  myGLCD.fillRect(xStart, yStart + 100, xStart + 200, yEnd);
  myGLCD.setColor(0, 0, 255);
  myGLCD.fillRect(xStart + 200, yStart + 100, xEnd, yEnd);
  myGLCD.setColor(0,0,0);
  for (int i = -2; i <= 2; ++i){
    myGLCD.drawRect(xStart + i, yStart + i, xEnd - i, yEnd - i);
  }
  myGLCD.print("Do you want to end", xStart + 40, yStart + 15);
  myGLCD.print("your workout?", xStart + 70, yStart + 40);
  myGLCD.setBackColor(0, 255, 0);
  myGLCD.print("Yes", xStart + 75, yStart + 135);
  myGLCD.setBackColor(0, 0, 255);
  myGLCD.print("No", xStart + 280, yStart + 135);
}

void setup()
{
  randomSeed(analogRead(0));
  
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial2.begin(9600);
  Wire.begin();       // join i2c bus (address optional for master)
  

   delay(300);  
       pinMode(GT9271_RESET, OUTPUT); 
    pinMode     (GT9271_INT, OUTPUT);
    digitalWrite(GT9271_RESET, LOW);
      delay(20);  
    digitalWrite(GT9271_INT, LOW);
    delay(50);
     digitalWrite(GT9271_RESET, HIGH);
     delay(100);  
    pinMode     (GT9271_INT, INPUT);
     delay(100);  
          
     uint8_t re=GT9271_Send_Cfg((uint8_t*)GTP_CFG_DATA,sizeof(GTP_CFG_DATA));
      
       pinMode(GT9271_RESET, OUTPUT); 
    pinMode     (GT9271_INT, OUTPUT);
    digitalWrite(GT9271_RESET, LOW);
      delay(20);  
    digitalWrite(GT9271_INT, LOW);
    delay(50);
     digitalWrite(GT9271_RESET, HIGH);
     delay(100);  
    pinMode     (GT9271_INT, INPUT);
     delay(100);  
          
     re=GT9271_Send_Cfg((uint8_t*)GTP_CFG_DATA,sizeof(GTP_CFG_DATA));
       
     
         uint8_t bb[2];
 	 readGT9271TouchAddr(0x8047,bb,2);  
         while(bb[1]!=32)
         { Serial.println("Capacitive touch screen initialized failure");  
       pinMode(GT9271_RESET, OUTPUT); 
    pinMode     (GT9271_INT, OUTPUT);
    digitalWrite(GT9271_RESET, LOW);
      delay(20);  
    digitalWrite(GT9271_INT, LOW);
    delay(50);
     digitalWrite(GT9271_RESET, HIGH);
     delay(100);  
    pinMode     (GT9271_INT, INPUT);
     delay(100);  

      
     uint8_t re=GT9271_Send_Cfg((uint8_t*)GTP_CFG_DATA,sizeof(GTP_CFG_DATA));
   
         }
    
  
  pinMode (SD_CHIP_SELECT, OUTPUT);

  if (!SD.begin(SD_CHIP_SELECT)) {
    Serial.print("fucked\n");
  }
  else {
    Serial.print("Capacitive touch screen initialized success\n");
    File myFile = SD.open("test.txt", FILE_WRITE);
    myFile.println("testing 1, 2, 3.");
    myFile.close();
    myFile = SD.open("test.txt");
    Serial.print(myFile.read());
  }
  Serial.print("testing...\n");
  Serial.print("mem test: \n");
    
  xMax = myGLCD.getDisplayXSize(); //800
  yMax = myGLCD.getDisplayYSize(); //480

  Serial.print("Screen width = ");
  Serial.println(xMax); 
  Serial.print("Screen height = ");
  Serial.println(yMax);
 // Setup the LCD
  myGLCD.InitLCD();
 // -------------------------------------------------------------
  pinMode(8, OUTPUT);  //backlight 
  digitalWrite(8, HIGH);//on

  struct monthlyWorkoutInfo mon_wk_info;
  struct dailyWorkoutInfo daily_wk_info;
  mon_wk_info.day_wks[0] = daily_wk_info;
  all_wks.mon_wks[0] = mon_wk_info;
// -------------------------------------------------------------

  drawLoginScreen();
  
}


void handleLoginScreen(int x, int y, int & currentInput) {
  if ((440 <= x && x <= 520) && (40 <= y && y <= 120)) { //BACKSPACE
    currentInput = currentInput/10;
    Serial.print(currentInput);
    if (currentInput == 0){
      snprintf((char*)buf,sizeof(buf)," ", currentInput);
      myGLCD.print((const char *)buf,144,64);
    }
    else {
    snprintf((char*)buf,sizeof(buf),"%d ", currentInput);
    myGLCD.print((const char *)buf,144,64);
    }
 }

  if ((280 <= x && x <= 520) && (160 <= y && y <= 240)) { // SIGNUP
    //if (currentInput is valid){
      currentScreen = SIGNUP;
      drawNewAccountScreen();
    //}
  }

  if ((560 <= x && x <= 680) && (40 <= y && y <= 120)) { // LOGIN
    currentScreen = MAIN;
    userID = currentInput;
    drawMainScreen();
  }

  if (currentInput < 1000){
    if ((120 <= x && x <= 200) && (280 <= y && y <= 440)) { // 1
      Serial.print("Button 1\n");
      currentInput = 10*currentInput + 1;
      snprintf((char*)buf,sizeof(buf),"%d", currentInput);
      myGLCD.print((const char *)buf,144,64);
    }

    if ((280 <= x && x <= 360) && (280 <= y && y <= 440)) { // 2
      Serial.print("Button 2\n");
      currentInput = 10*currentInput + 2;
      snprintf((char*)buf,sizeof(buf),"%d", currentInput);
      myGLCD.print((const char *)buf,144,64);
    }

    if ((440 <= x && x <= 520) && (280 <= y && y <= 440)) { // 3
      Serial.print("Button 3\n");
      currentInput = 10*currentInput + 3;
      snprintf((char*)buf,sizeof(buf),"%d", currentInput);
      myGLCD.print((const char *)buf,144,64);
    }

    if ((600 <= x && x <= 680) && (280 <= y && y <= 440)) { //4
      Serial.print("Button 4\n");
      currentInput = 10*currentInput + 4;
      snprintf((char*)buf,sizeof(buf),"%d", currentInput);
      myGLCD.print((const char *)buf,144,64);
    }
  }
  return;
}

void handleNewAccountScreen(int x, int y){
  if ((40 <= x && x <= 280) && (360 <= y && y <= 460)){ //CONFIRM
    Serial.print("\nConfirmed");
    currentInput = 0;
    currentScreen = LOGIN;
    drawLoginScreen();
  }
  if ((520 <= x && x <= 760) && (360 <= y && y <= 460)){ //DENY
    drawNewAccountScreen();
  }
  if ((660 <= x && x <= 780) && (20 <= y && y <= 80)){ //BACK
    Serial.print("\nLeft");
    currentInput = 0;
    currentScreen = LOGIN;
    drawLoginScreen();
  }
  return;
}

void handleMainScreen(int x, int y, uint32_t & workoutTime){
  if ((580 <= x && x <= 780) && (20 <= y && y <= 80)){
    currentInput = 0;
    currentScreen = LOGIN;
    drawLoginScreen();
  }
  if ((200 <= x && x <= 600) && (120 <= y && y <= 220)) {
    currentScreen = HIST;
    drawHistoryScreen();
  }
  if ((200 <= x && x <= 600) && (240 <= y && y <= 340)) {
    workoutTime = millis();
    currentScreen = WORK;
    struct workoutInfo wk_data;
    struct dailyWorkoutInfo daily_wk_data;
    wk_data.user_id = 1111; // TODO: replace
    wk_data.wk_id = 0;
    for (int i = 0; i < 6; i++) {
      wk_data.num_reps[i] = 0;
      wk_data.avg_heart_rate[i] = 0;
      wk_data.avg_vo2[i] = 0;
      wk_data.avg_force[i] = 0;
    }
    daily_wk_data.wks[cur_workout] = wk_data;
    all_wks.mon_wks[cur_month_workout].day_wks[cur_day_workout] = daily_wk_data;
    drawMainWorkoutScreen();
  }
  return;
}

void handleMainWorkoutScreen(int x, int y, uint32_t & rowWorkoutTime ){
  if ((20 <= x && x <= 440) && (120 <= y && y <= 170)){
    setRefReps();
    currentScreen = WORKLEG;
    drawRepBasedWorkoutScreen();
    workout = 0;
    return;
  }
  else if ((20 <= x && x <= 440) && (180 <= y && y <= 230)){
    setRefReps();
    currentScreen = WORKCALF;
    drawRepBasedWorkoutScreen();
    workout = 1;
    return;
  }
  else if ((20 <= x && x <= 440) && (240 <= y && y <= 290)){
    setRefReps();
    currentScreen = WORKSHLDR;
    drawRepBasedWorkoutScreen();
    workout = 2;
    return;
  }
  else if ((20 <= x && x <= 440) && (300 <= y && y <= 350)){
    setRefReps();
    currentScreen = WORKBI;
    drawRepBasedWorkoutScreen();
    workout = 3;
    return;
  }
  else if ((20 <= x && x <= 440) && (360 <= y && y <= 410)){
    setRefReps();
    currentScreen = WORKHAND;
    drawRepBasedWorkoutScreen();
    workout = 4;
    return;
  }
  else if ((20 <= x && x <= 440) && (420 <= y && y <= 470)){
    setRefReps();
    rowWorkoutTime = millis();
    currentScreen = WORKROW;
    drawRepBasedWorkoutScreen();
    workout = 5;
    return;
  }
  else if ((660 <= x && x <= 780) && (20 <= y && y <= 80)) {
    currentScreen = MAIN;
    drawMainScreen();
    return;
  }
  else if ((660 <= x && x <= 780) && (400 <= y <= 460)){
    previousScreen = currentScreen;
    currentScreen = END;
    paused = true;
    pausedTime = millis();
    drawEndWorkoutScreen();
    return;
  }
  return;
}

void handleLegPressScreen(int x, int y){
  return;
}

void handleHistoryScreen(int x, int y){
  if ((660 <= x && x <= 780) && (20 <= y && y <= 80)){ //BACK
    currentScreen = MAIN;
    drawMainScreen();
    return;
  }
  else if ((200 <= x && x <= 600) && (120 <= y && y <= 220)) {
    currentScreen = DAILY;
    drawDailyHistoryScreen();
  }
}

void handleMidWorkoutScreen(int x, int y) {
  if ((660 <= x && x <= 780) && (20 <= y && y <= 80)) {
    currentScreen = WORK;
    paused = false;
    cur_reps = 0;
    cur_hr = 0;
    cur_vo2 = 0;
    cur_force = 0;
    drawMainWorkoutScreen();
    return;
  }
  if ((600 <= x && x <= 780) && (280 <= y <= 460)) {
    if (paused) {
      drawBigPauseButton();
      totalWorkoutTime = totalWorkoutTime + timeElapsed;
      paused = false;
    }
    else {
      drawPlayButton();
      pausedTime = millis();
      paused = true;
    }
  }
}

void handleEndWorkoutScreen(int x, int y) {
  if ((200 <= x && x <= 400) && (200 <= y && y <= 300)) { // Yes
    currentScreen = MAIN;
    paused = false;
    drawMainScreen();
    return;
  }
  if ((400 <= x && x <= 600) && (200 <= y && y <= 300)) { //No
    currentScreen = previousScreen;
    paused = false;
    totalWorkoutTime = totalWorkoutTime + timeElapsed;
    drawMainWorkoutScreen();
    return;
  }
}


void loop()
{
  int buf[798];
  int x;
  int y;

  
  uint32_t thisTouchTime = millis();

  uint8_t count = readGT9271TouchLocation( touchLocations, 10 );

  if (paused) {
    timeElapsed = thisTouchTime - pausedTime;
  }
  else {
    switch (currentScreen) {
      case WORK:      updateClock(1, thisTouchTime, totalWorkoutTime);
                      break;
      case WORKLEG:   updateClock(1, thisTouchTime, totalWorkoutTime);
                      break;
      case WORKCALF:  updateClock(1, thisTouchTime, totalWorkoutTime);
                      break;
      case WORKSHLDR: updateClock(1, thisTouchTime, totalWorkoutTime);
                      break;
      case WORKBI:    updateClock(1, thisTouchTime, totalWorkoutTime);
                      break;
      case WORKHAND:  updateClock(1, thisTouchTime, totalWorkoutTime);
                      break;
      case WORKROW:   updateClock(1, thisTouchTime, totalWorkoutTime);
                      updateClock(2, thisTouchTime, rowWorkoutTime);
                      break;           
      default: break; 
    }
  }  


  if (count)
  {
    x = 800 - touchLocations[0].x;
    y = 480 - touchLocations[0].y;
    
    if (thisTouchTime - thatTouchTime > 500){
      switch(currentScreen){
        case LOGIN: //Serial.print("\nCurrent: " + String(currentScreen));
                    handleLoginScreen(x, y, currentInput);
                    break;
        case SIGNUP:  //Serial.print("\nCurrent: " + String(currentScreen));
                      handleNewAccountScreen(x, y);
                      break;
        case MAIN:  //Serial.print("\nCurrent: " + String(currentScreen));
                    handleMainScreen(x, y, totalWorkoutTime);
                    break; 
        case HIST:  //Serial.print("\nCurrent: " + String(currentScreen));
                    handleHistoryScreen(x, y);
                    break;   
        case WORK:  //Serial.print("\nCurrent: " + String(currentScreen));
                    handleMainWorkoutScreen(x, y, rowWorkoutTime);
                    break;
        case WORKLEG: handleMidWorkoutScreen(x, y);
                      break;
        case WORKCALF:handleMidWorkoutScreen(x, y);
                      break;
        case WORKSHLDR: handleMidWorkoutScreen(x, y);
                        break;
        case WORKBI:  handleMidWorkoutScreen(x, y);
                      break;
        case WORKHAND:
          handleMidWorkoutScreen(x, y);
          break;
        case WORKROW:
          handleMidWorkoutScreen(x, y);
          break;
        case END: handleEndWorkoutScreen(x, y);
                  break;
        default:  Serial.print("meh");
                  break;
      }
      thatTouchTime = thisTouchTime;
    }
   
  }
}
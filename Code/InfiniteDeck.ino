#define LGFX_USE_V1
//#define LGFX_AUTODETECT

//#define DEBUG
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <string.h>
#include <TJpg_Decoder.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <ArduinoJson.h>
/*
      --- If you use a ESP32 TouchDown ---
      - Dustin Watts FT6236 Library (version 1.0.2), https://github.com/DustinWatts/FT6236

      --- If you use a GT911 touchscreen or the esp323248s035 capacitive board ---
      - TAMCTEC GT911 library (version 1.0.2 or newer), https://github.com/TAMCTec/gt911-arduino

      --- If you use the esp322432s028 resistive board ---
      - TFT_Touch (Latest), https://github.com/Bodmer/TFT_Touch 

      --- If you use a XPT2046 touchscreen ---
      - TFT_Touch (Latest), https://github.com/Bodmer/TFT_Touch 
      - 1. Uncomment the esp322432s028r line
      - 2. Comment the autobrightness option for the esp322432s028r
      - 3. Change the resolution to match the resolution for your screen (the screen resolution in the line after #ifdef esp322432s028r)
      - 4. Change the pins at the touchscreen part for the esp322432s028r somewhere down there to match your pin layout
      - 5. Try and see if it works
*/

//SDCARD, change the following things to match your device
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18
#define SD_CS 5

//String FILE_NAME = "/0/0.bmp";
// ------- Uncomment the line of the device/touchscreen you use -------
// (The ESP32 TOUCHDOWN and the ESP32 TouchDown S3 uses this!)
//#define TouchDown
// (The esp323248s035 capacitive uses the below one!)
//#define esp323248s035c
// (The gt911 touchscreen uses the lowest one)
//#define GT911 //no need to set this if you use a esp323248s035c, setting it wont affect anything tho
// (The esp322432s028r uses the below one!)
//#define esp322432s028r


// ------- Extra things
//Possible global (affects all devices) button spam fix (makes it a bit less)
#define SpamFix

// ------- Settings for the esp323248s035 capacitive board -------
#ifdef esp323248s035c
//Brightness related stuff, an option to change the color and disable the led light on the board will come soon
#define AUTO_BRIGHTNESS //disable this if you dont want auto brightness
#ifdef AUTO_BRIGHTNESS
uint8_t AutoOffset = 0; //change this number to change the offset of the auto brightness
unsigned int iLightTolerance = 20; //the required diffrence in brightness before it updates the brightness of the screen
#endif //defined(AUTO_BRIGHTNESS)
//Touchscreen related
//Possible fixes (for touchscreen button spamming and the info page directly closing):
#define GT911_Possible_Spam_Fix1 // makes it read the touch info 5 times instead of once a loop
#define GT911_Possible_Spam_Fix2 // diffrent way of reading the touches, does cause the screen to only have 1 touch point
#define LONGER_DELAY //adds a bit more delay in the loop which might fix some touch issues
#define GT911
#endif // defined(esp323248s035c)
// ------- Settings for the GT911 touchscreen (default is for the esp323248s035c board)-------
#ifdef GT911
#define TOUCH_SDA  33 //33
#define TOUCH_SCL  32 //32
#define TOUCH_INT 21 //21
#define TOUCH_RST 25 //25
#define TOUCH_ROTATION ROTATION_RIGHT //(Default: ROTATION_RIGHT)Possible values(or smth): ROTATION_LEFT  ROTATION_RIGHT  ROTATION_NORMAL ROTATION_INVERSED
#endif

// ------- Settings for the esp322432s028r board -------
#ifdef esp322432s028r
//Brightness related stuff, an option to change the color and disable the led light on the board will come soon
#define AUTO_BRIGHTNESS //disable this if you dont want auto brightness
#ifdef AUTO_BRIGHTNESS
#define LIGHT_SENSOR 34 //A test to see if it uses the same pin as the esp323248s035c
uint8_t AutoOffset = 0; //change this number to change the offset of the auto brightness
unsigned int iLightTolerance = 20; //the required diffrence in brightness before it updates the brightness of the screen
#endif //defined(AUTO_BRIGHTNESS)
#endif // defined(esp323248s035c)

// ------- Settings for the ESP32 TouchDown board -------
#ifdef TouchDown
// ------- Uncomment and populate the following if your cap touch uses custom i2c pins -------
//#define CUSTOM_TOUCH_SDA 17
//#define CUSTOM_TOUCH_SCL 18
#endif


#define DEBUG
#define Wallpaper
#define Splash
#ifdef GT911
#define DefaultRes
#endif
#ifdef Touchdown
#define DefaultRes
#endif
#ifdef ResistiveTouch
#define DefaultRes
#endif

//Set your resolution here (default is for the ESP32 Touchdown and esp323248s035c)
//No need to change anything if you use a esp322432s028r
#ifdef DefaultRes
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 320
#endif

#ifdef esp322432s028r
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#endif
//LCD

// Touchscreen part i guess
#ifdef TouchDown
#include <Wire.h>
#include <FT6236.h>
FT6236 ts = FT6236();
#endif // defined(TouchDown)

#ifdef GT911
#include <Wire.h>
#include <TAMC_GT911.h>
TAMC_GT911 ts = TAMC_GT911(TOUCH_SDA, TOUCH_SCL, TOUCH_INT, TOUCH_RST, SCREEN_WIDTH, SCREEN_HEIGHT);
#endif

#ifdef esp322432s028r
uint16_t touchX, touchY;
#include <Wire.h>
#include <TFT_Touch.h>
// These are the pins used to interface between the 2046 touch controller and Arduino Pro
#define DOUT 39  /* Data out pin (T_DO) of touch screen */
#define DIN  32  /* Data in pin (T_DIN) of touch screen */
#define DCS  33  /* Chip select pin (T_CS) of touch screen */
#define DCLK 25  /* Clock pin (T_CLK) of touch screen */
/* Create an instance of the touch screen library */
TFT_Touch touch = TFT_Touch(DCS, DCLK, DIN, DOUT);
#endif

// auto brightness part i guess
#ifdef AUTO_BRIGHTNESS
//some required things you need to set
int iLastLightLevel = 0;
int iLastBrightness = 0;
uint8_t ConvertedLightLevel = 0;
uint8_t OffsetLightLevel = 0;
uint8_t OffsetLast = 0;
uint8_t iSomething = 0;
#ifdef esp323248s035c
#define LIGHT_SENSOR 34 //no need to change this
#endif
#endif

// Initial LED brightness
int ledBrightness = 255;

bool prevthing = false;
bool pressed = false;
uint16_t t_x = 0, t_y = 0;

// ------- NimBLE definition, use only if the NimBLE library is installed 
// and if you are using the original ESP32-BLE-Keyboard library by T-VK -------
//#define USE_NIMBLE

// Bluetooth stuff
#include <BleKeyboard.h>
BleKeyboard bleKeyboard("InfiniteDeck", "OwO");
#ifndef BLE_KEYBOARD_VERSION
  #warning Old BLE Keyboard version detected. Please update.
  #define BLE_KEYBOARD_VERSION "Outdated"
#endif // !defined(BLE_KEYBOARD_VERSION) 

#ifdef USE_NIMBLE
  #include "NimBLEDevice.h"   // Additional BLE functionaity using NimBLE
  #include "NimBLEUtils.h"    // Additional BLE functionaity using NimBLE
  #include "NimBLEBeacon.h"   // Additional BLE functionaity using NimBLE
#else
  #include "BLEDevice.h"   // Additional BLE functionaity
  #include "BLEUtils.h"    // Additional BLE functionaity
  #include "BLEBeacon.h"   // Additional BLE functionaity
#endif

#include "esp_sleep.h"   // Additional BLE functionaity
#include "esp_bt_main.h"   // Additional BLE functionaity
#include "esp_bt_device.h" // Additional BLE functionaity

//#include "Action.h"
//#include "UserActions.h"

//#include "USB.h"
//#include "USBHIDKeyboard.h"
//#include "Keydefines.h"
//USBHIDKeyboard bleKeyboard;

#define ROWS 3
#define COLUMNS 5
#define thickness_spaces 10
#define border_text 3
#define font_width 18
#define font_height 24
#define FONT_SIZE 3

#define TOTAL_BUTTONS ROWS*COLUMNS
#define max_pixel_per_row int(320/ROWS)
#define max_pixel_per_col int(480/COLUMNS)
#define b_width max_pixel_per_col-thickness_spaces-thickness_spaces
#define b_height max_pixel_per_row-thickness_spaces-thickness_spaces
#define max_font_space_w int(b_width-border_text)
#define max_font_space_h int(b_height-border_text)
#define max_letters_per_row int(max_font_space_w/font_width)
#define max_rows_per_button int(max_font_space_h/font_height)
#define max_letters max_letters_per_row*max_rows_per_button

uint8_t profile_index = 0;
uint8_t last_profile = 0;
int buttons_positions[TOTAL_BUTTONS][2] = {};
uint32_t buttons_colors[TOTAL_BUTTONS];
String buttons_names[TOTAL_BUTTONS];
bool button_state[TOTAL_BUTTONS];

boolean CLICKED_PREV_LOOP = false;

int last_x = 240;
int last_y = 160;
int pos[2] = {0, 0};
int pos_2[2] = {0, 0};
int last_i = 0;
uint8_t brightness_l = 255;
uint8_t ConvertedLightLevel = 0;
uint8_t OffsetLightLevel = 0;
uint8_t OffsetLast = 0;
uint8_t iSomething = 0;

String wallpaperpathjpg = "placeholder";
String wallpaperpathbmp = "placeholder";

TFT_eSPI lcd = TFT_eSPI();

void GetTouch() {
  bool prevthing = pressed;
  uint16_t t_x = 0, t_y = 0;
#ifdef GT911
  ts.read();
  bool pressed = ts.isTouched;
  if (pressed) {
#ifndef GT911_Possible_Spam_Fix2
    for (int i = 0; i < ts.touches; i++) {
      t_x = ts.points[i].x;
      t_y = ts.points[i].y;
    }
#else
    t_x = ts.points[0].x; //need to test this, might need to set it to 1 instead
    t_y = ts.points[0].y;
#endif
  }
#endif

#ifdef TouchDown
  bool pressed = ts.touched();
  if (pressed)
  {

    // Retrieve a point
    TS_Point p = ts.getPoint();

    //Flip things around so it matches our screen rotation
    p.x = map(p.x, 0, 320, 320, 0);
    t_y = p.x;
    t_x = p.y;
  }
#endif

#ifdef esp322432s028r
  bool pressed = touch.Pressed();//tft.getTouch( &touchX, &touchY, 600 );
  if (pressed)
  {
    t_x = touch.X();
    t_y = touch.Y();

    /*Set the coordinates*/
    //data->point.x = touchX;
    //data->point.y = touchY;

    //Serial.print( "Data x " );
    //Serial.println( touchX );

    //Serial.print( "Data y " );
    //Serial.println( touchY );
    // Retrieve a point
    //TS_Point p = ts.getPoint();

    //Flip things around so it matches our screen rotation
    //p.x = map(p.x, 0, 320, 320, 0);
    //t_y = p.x;
    //t_x = p.y;

  }
#endif
#ifdef ResistiveTouch
pressed = tft.getTouch(&t_x, &t_y);
#endif

#ifdef SpamFix
  if (prevthing) {
    bool pressed = false;
  }
#endif
}

void SetTouch() {
#ifdef GT911 //delay can be made lower, but that might cause the touch driver to not function
  delay(300); //delay to prevent the touchscreen from not working right
  ts.begin();
  delay(300);
  ts.reset();
  delay(60);
  ts.setRotation(TOUCH_ROTATION);
  Serial.println("[INFO]: Capacitive touch started! (GT911)");
#endif

#ifdef TouchDown
#ifdef CUSTOM_TOUCH_SDA
  if (!ts.begin(40, CUSTOM_TOUCH_SDA, CUSTOM_TOUCH_SCL))
#else
  if (!ts.begin(40))
#endif // defined(CUSTOM_TOUCH_SDA)
  {
    Serial.println("[WARNING]: Unable to start the capacitive touchscreen.");
  }
  else
  {
    Serial.println("[INFO]: Capacitive touch started!");
  }
#endif // defined(TouchDown)

#ifdef esp322432s028r
  /*Set the touchscreen calibration data,
    the actual data for your display can be acquired using
    the Generic -> Touch_calibrate example from the TFT_eSPI library*/
  //uint16_t calData[] = { 456, 3608, 469, 272, 3625, 3582, 3518, 263,  };
  //    tft.setTouchCalibrate(calData);//
  //    uint16_t calData[5] = { 275, 3620, 264, 3532, 1 };
  //    tft.setTouch( calData );
  touch.setCal(526, 3443, 750, 3377, 320, 240, 1);
#endif // defined(esp322432s028r)
}

void ExtraThings() {
#ifdef AUTO_BRIGHTNESS
  pinMode(LIGHT_SENSOR, ANALOG);
  analogSetPinAttenuation(LIGHT_SENSOR, ADC_0db); // 0dB(1.0) 0~800mV
  iLastLightLevel = analogReadMilliVolts(LIGHT_SENSOR);
#endif
}


void setup(void){
  pinMode(SD_CS, OUTPUT);
  pinMode(SPI_SCK, OUTPUT);
  pinMode(SPI_MOSI, OUTPUT);
  pinMode(SPI_MISO, INPUT);

  digitalWrite(SD_CS, LOW);
#ifdef DEBUG
  Serial.begin(115200);
#endif
  lcd.init();
  lcd.setRotation(1);
    ledcSetup(0, 5000, 8);
#ifdef TFT_BL
  ledcAttachPin(TFT_BL, 0);
#else
  ledcAttachPin(backlightPin, 0);
#endif // defined(TFT_BL)
  ledcWrite(0, ledBrightness); // Start @ initial Brightness
  //lcd.setBrightness(brightness_l);
  //ledcSetup(0, 2000, 8);
  //ledcAttachPin(LCD_BLK, 0);
  //ledcWrite(0, 255);

  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  if (!SD.begin(SD_CS,SPI,25000000))
  {
#ifdef DEBUG    
    Serial.println("Card Mount Failed");
#endif
    displayNoSD();
    return;
  }
//lcd.fillScreen(0x000000U); //filling the screen with a black background to fix the issue of the background looking glitched
#ifdef Splash
  show_splash();
#endif
  SetTouch();

  ExtraThings();


Serial.println("[INFO]: Capacitive touch started! (GT911)");
Serial.println("[INFO]: Starting BLE");
bleKeyboard.begin();
load_conf();  
calculate_positions();
#ifdef Splash
  lcd.fillScreen(0x000000U);
#endif
#ifdef Wallpaper
  display_wallpaper();
#endif
display_buttons();
}

void loop(){
    delay(20);
    #ifdef AUTO_BRIGHTNESS
    char sLightLevel[32];
// change brightness if light level changed
    int iNewLightLevel = analogReadMilliVolts(LIGHT_SENSOR);
      if ((iNewLightLevel > (iLastLightLevel + iLightTolerance)) || (iNewLightLevel < (iLastLightLevel - iLightTolerance))) {
      snprintf_P(sLightLevel, sizeof(sLightLevel), PSTR("{\"light\":%d}"), iNewLightLevel);
      //dispatch_state_subtopic("light",sLightLevel);
      #ifdef smooth
        OffsetLast = iLastLightLevel + AutoOffset;
        iLastBrightness = iLastLightLevel * 0.25;
      #endif
      if (iNewLightLevel > 1020){
        iNewLightLevel = 1020;
      }
    iLastLightLevel = iNewLightLevel;
    OffsetLightLevel = 255 - iNewLightLevel / 4;
    //ConvertedLightLevel = OffsetLightLevel * 0.25;  // 0.2490234375
    //ConvertedLightLevel = iNewLightLevel * 0.25 - AutoOffset;  // 0.2490234375
    ConvertedLightLevel = OffsetLightLevel - AutoOffset; // 25.5
    //ConvertedLightLevel = 255 - OffsetLightLevel;
      #ifdef DEBUG
        Serial.println(' '); Serial.print("Light level sensor: "); Serial.print(iNewLightLevel); Serial.println(' ');
        Serial.println(' '); Serial.print("Light level sensor offset: "); Serial.print(OffsetLightLevel); Serial.println(' ');
        Serial.println(' '); Serial.print("Light level converted: "); Serial.print(ConvertedLightLevel); Serial.println(' ');
      #endif
      brightness_l = ConvertedLightLevel;

      if (brightness_l > 255){
        brightness_l = 255;
      }
      else{
        if (brightness_l < 10){
          brightness_l = 10;
        }
      }
      #ifdef DEBUG
        Serial.println(' '); Serial.print("Light level: "); Serial.print(brightness_l); Serial.println(' ');
      #endif
      #ifdef smooth
        if (iLastBrightness >= brightness_l){
          for (int i = iLastBrightness; i >= brightness_l; i = i - iLightTolerance){
            //iSomething = 255 - i;
            Serial.println(' '); Serial.print("iPrev greater: "); Serial.print(i);// Serial.println(' ');
            ledcWrite(0, i);
            delay(10);
          }
        }
        else{
          for (int i = iLastBrightness; i <= brightness_l; i = i + iLightTolerance){
            Serial.println(' '); Serial.print("iLast greater: "); Serial.print(i);// Serial.println(' ');
            ledcWrite(0, i);
            delay(10);
          }          
        }
      #else
        ledcWrite(0, brightness_l);
      #endif
      }
    #endif
    if (bleKeyboard.isConnected()){
      bmpDraw("/icons/bleconnected.bmp", 0, 0);
    }
    else {  
      bmpDraw("/icons/bledisconnected.bmp", 0, 0);
    }
   GetTouch();
    if (pressed){
      for (int i = 0; i < TOTAL_BUTTONS; i++){
        if (t_x >= buttons_positions[i][0] && t_x <= (buttons_positions[i][0] + b_height)) {
          if (t_y >= buttons_positions[i][1] && t_y <= (buttons_positions[i][1] + b_width)) {
            if (button_state[i] == false) {
#ifdef DEBUG
            Serial.print("button:");
            Serial.print(i);
            Serial.println("");
#endif
            draw_rectangle(buttons_positions[i][0]-thickness_spaces, buttons_positions[i][1]-thickness_spaces, b_width+2*thickness_spaces, thickness_spaces, 0xFFFFFFU);
            draw_rectangle(buttons_positions[i][0]-thickness_spaces, buttons_positions[i][1], thickness_spaces, b_height, 0xFFFFFFU);
            draw_rectangle(buttons_positions[i][0]+b_width, buttons_positions[i][1], thickness_spaces, b_height, 0xFFFFFFU);
            draw_rectangle(buttons_positions[i][0]-thickness_spaces, buttons_positions[i][1]+b_height, b_width+2*thickness_spaces, thickness_spaces, 0xFFFFFFU);
            button_state[i] = true;
            last_i = i;
            send_key(i);
          }
        }
      }
      
    }
  } else {
    for(int e = 0; e<TOTAL_BUTTONS;e++){
      if (button_state[e] == true) {
  #ifdef DEBUG
        Serial.print("released:");
        Serial.print(e);
        Serial.println("");
  #endif
        draw_rectangle(buttons_positions[e][0]-thickness_spaces, buttons_positions[e][1]-thickness_spaces, b_width+2*thickness_spaces, thickness_spaces, 0x000000U);
        draw_rectangle(buttons_positions[e][0]-thickness_spaces, buttons_positions[e][1], thickness_spaces, b_height, 0x000000U);
        draw_rectangle(buttons_positions[e][0]+b_width, buttons_positions[e][1], thickness_spaces, b_height, 0x000000U);
        draw_rectangle(buttons_positions[e][0]-thickness_spaces, buttons_positions[e][1]+b_height, b_width+2*thickness_spaces, thickness_spaces, 0x000000U);
        button_state[e] = false;
        bleKeyboard.releaseAll();
      }
    }
  }
}





void send_key(int number){
  String archive = "/" + String(profile_index) + "/" + String(number) + ".txt";
  char buf[12];
  archive.toCharArray(buf,archive.length()+1);
  readFile(SD, buf);
}

void save_conf(){
  String archive = "/conf.txt";
  char buf[12];
  char bufB[10];
  archive.toCharArray(buf,archive.length()+1);
  String brightness_S = "BRI " + String(brightness_l);
  brightness_S.toCharArray(bufB,brightness_S.length()+1);
  writeFile(SD, buf, bufB);
}


void readFile(fs::FS &fs, const char *path){
#ifdef DEBUG
  Serial.printf("Reading file: %s\n", path);
#endif

  File file = fs.open(path);
  if (!file)
  {
#ifdef DEBUG
    Serial.println("Failed to open file for reading");
#endif
    return;
  }
#ifdef DEBUG
  Serial.print("Read from file: ");
#endif
  String line = "";
  while (file.available()){
    char m = file.read();
    if (m == '\n') {
#ifdef DEBUG
      Serial.println(line);
#endif
      Line(line);
      line = "";
    } else if ((int) m != 13) {
      line += m;
    }
  }
  Line(line);
  file.close();
#ifdef DEBUG
  Serial.println(line);
#endif
}

void Line(String l)
{
  int space_1 = l.indexOf(" ");
  if (space_1 == -1){
#ifdef DEBUG
    Serial.println("Press Only 1 key");
#endif
    if(l.equals("BACK")){
      profile_index = last_profile;
      #ifdef Wallpaper
      lcd.fillScreen(0x000000U);
      delay(30);
      display_wallpaper();
      #endif
      display_buttons(); 
    }else if (l.equals("BRDOWN")) {
      if(brightness_l <= 10){
        brightness_l = 10;
      }else{
        brightness_l = brightness_l - 15;
      }
      //lcd.setBrightness(brightness_l);
      ledcWrite(0, brightness_l);
    }else if (l.equals("BRUP")) {
      if(brightness_l<=240){
        brightness_l = brightness_l + 15;
      }else{
        brightness_l = 255;
      }
      //lcd.setBrightness(brightness_l);
      ledcWrite(0, brightness_l);
    }else if (l.equals("SAVEBR")) {
      //Save in conf.txt
      save_conf();
    }else{
      Press(l);  
    }
  }else if (l.substring(0, space_1) == "STRING"){
    bleKeyboard.print(l.substring(space_1 + 1));
  }else if (l.substring(0, space_1) == "KEY"){
    Press(l.substring(space_1 + 1));
    return;
  }else if (l.substring(0, space_1) == "DELAY"){
    int delaytime = l.substring(space_1 + 1).toInt();
    delay(delaytime);
  }else if (l.substring(0, space_1) == "GOTO"){
    last_profile = profile_index;
    profile_index = l.substring(space_1 + 1).toInt();
    #ifdef Wallpaper
      lcd.fillScreen(0x000000U);
      delay(30);
      display_wallpaper();
    #endif
    display_buttons();
  }else if (l.substring(0, space_1) == "REM") {
    //do nothing
  }else{
    String remain = l;

    while (remain.length() > 0)
    {
      int latest_space = remain.indexOf(" ");
      if (latest_space == -1)
      {
        Press(remain);
        remain = "";
      }
      else
      {
        Press(remain.substring(0, latest_space));
        remain = remain.substring(latest_space + 1);
      }
      delay(5);
    }
  }

  bleKeyboard.releaseAll();
}

void readConfFile(fs::FS &fs, const char *path, int a){
#ifdef DEBUG
  Serial.printf("Reading file: %s\n", path);
#endif

  File file = fs.open(path);
  if (!file){
#ifdef DEBUG
    Serial.println("Failed to open file for reading");
#endif
    buttons_names[a] = "";
    buttons_colors[a] = 0x000000U;
    return;
  }
#ifdef DEBUG
  Serial.print("Read from file: ");
#endif
  String line = "";
  while (file.available()){
    //Serial.write(file.read());
    char m = file.read();
    if (m == '\n'){
#ifdef DEBUG
      Serial.println(line);
#endif
      confLine(line, a);
      line = "";
      file.close();
      return;
    }else if ((int) m != 13){
      line += m;
    }
  }
  confLine(line, a);
  file.close();
#ifdef DEBUG
  Serial.println(line);
#endif
}

void confLine(String l, int a){
  int space_1 = l.indexOf(" ");

  if (space_1 == -1){
    //Press(l);
    buttons_names[a] = String(a);
    buttons_colors[a] = 0xFFFFFFU;    
  }else if (l.substring(0, space_1) == "STRING"){
    buttons_names[a] = String(a);
    buttons_colors[a] = 0xFFFFFFU;
  }else if (l.substring(0, space_1) == "DELAY"){
    buttons_names[a] = String(a);
    buttons_colors[a] = 0xFFFFFFU;
  }else if (l.substring(0, space_1) == "REM"){
    int space_2 = l.lastIndexOf(" ");
    buttons_names[a] = l.substring(space_1+1,space_2);
    String color_button = l.substring(space_2+1);
    buttons_colors[a] = str2hex(color_button);
  }else if (l.substring(0, space_1) == "BRI"){
    String brightness_S = l.substring(space_1+1);
    brightness_l = brightness_S.toInt();
    ledcWrite(0, brightness_l);
  }else{
    buttons_names[a] = String(a);
    buttons_colors[a] = 0xFFFFFFU;

    String remain = l;
    while (remain.length() > 0){
      int latest_space = remain.indexOf(" ");
      if (latest_space == -1){
        remain = "";
      }
      else{
        remain = remain.substring(latest_space + 1);
      }
    }
  }
}
/*
void JsonAction(){

}

void JsonRead(){

}

void JsonWrite(){

}

void Actions(int casenum, int subcase){
  switch (casenum)
  {
  case 0: //default actions
    switch (subcase)
    {
    case 0:
      //da action
      break;
    case 1:
      //...
      break;
    
    }
  case 1:
    //...
  }
}
*/

void Press(String b){
#ifdef DEBUG
  Serial.print("Press: ");
  Serial.print(b);
  Serial.print(" Length: ");
  Serial.println(b.length());
#endif
  if (b.length() == 1){
    char c = b[0];
    bleKeyboard.press(c);
  }else if (b.equals("ENTER")){
    bleKeyboard.press(KEY_RETURN);
  }else if (b.equals("CTRL") || b.equals("CONTROL")){
    bleKeyboard.press(KEY_LEFT_CTRL);
  }else if (b.equals("SHIFT")){
    bleKeyboard.press(KEY_LEFT_SHIFT);
  }else if (b.equals("ALT") || b.equals("OPTION")){
    bleKeyboard.press(KEY_LEFT_ALT);
  }else if (b.equals("ALTGR")){
    bleKeyboard.press(KEY_RIGHT_ALT);
  }else if (b.equals("GUI") || b.equals("WINDOWS") || b.equals("COMMAND")){
    bleKeyboard.press(KEY_LEFT_GUI);
  }else if (b.equals("UP") || b.equals("UPARROW")){
    bleKeyboard.press(KEY_UP_ARROW);
  }else if (b.equals("DOWN") || b.equals("DOWNARROW")){
    bleKeyboard.press(KEY_DOWN_ARROW);
  }else if (b.equals("LEFT") || b.equals("LEFTARROW")){
    bleKeyboard.press(KEY_LEFT_ARROW);
  }else if (b.equals("RIGHT") || b.equals("RIGHTARROW")){
    bleKeyboard.press(KEY_RIGHT_ARROW);
  }else if (b.equals("DELETE")){
    bleKeyboard.press(KEY_DELETE);
  }else if (b.equals("BACKSPACE")){
    bleKeyboard.press(KEY_BACKSPACE);
  }else if (b.equals("PAGEUP")){
    bleKeyboard.press(KEY_PAGE_UP);
  }else if (b.equals("PAGEDOWN")){
    bleKeyboard.press(KEY_PAGE_DOWN);
  }else if (b.equals("HOME")){
    bleKeyboard.press(KEY_HOME);
  }else if (b.equals("ESC")){
    bleKeyboard.press(KEY_ESC);
  }else if (b.equals("INSERT")){
    bleKeyboard.press(KEY_INSERT);
  }else if (b.equals("TAB")){
    bleKeyboard.press(KEY_TAB);
  }else if (b.equals("END")){
    bleKeyboard.press(KEY_END);
  }else if (b.equals("CAPSLOCK")){
    bleKeyboard.press(KEY_CAPS_LOCK);
  }else if (b.equals("F1")){
    bleKeyboard.press(KEY_F1);
  }else if (b.equals("F2")){
    bleKeyboard.press(KEY_F2);
  }else if (b.equals("F3")){
    bleKeyboard.press(KEY_F3);
  }else if (b.equals("F4")){
    bleKeyboard.press(KEY_F4);
  }else if (b.equals("F5")){
    bleKeyboard.press(KEY_F5);
  }else if (b.equals("F6")){
    bleKeyboard.press(KEY_F6);
  }else if (b.equals("F7")){
    bleKeyboard.press(KEY_F7);
  }else if (b.equals("F8")){
    bleKeyboard.press(KEY_F8);
  }else if (b.equals("F9")){
    bleKeyboard.press(KEY_F9);
  }else if (b.equals("F10")){
    bleKeyboard.press(KEY_F10);
  }else if (b.equals("F11")){
    bleKeyboard.press(KEY_F11);
  }else if (b.equals("F12")){
    bleKeyboard.press(KEY_F12);
  }else if (b.equals("F13")){
    bleKeyboard.press(KEY_F13);
  }else if (b.equals("F14")){
    bleKeyboard.press(KEY_F14);
  }else if (b.equals("F15")){
    bleKeyboard.press(KEY_F15);
  }else if (b.equals("F16")){
    bleKeyboard.press(KEY_F16);
  }else if (b.equals("F17")){
    bleKeyboard.press(KEY_F17);
  }else if (b.equals("F18")){
    bleKeyboard.press(KEY_F18);
  }else if (b.equals("F19")){
    bleKeyboard.press(KEY_F19);
  }else if (b.equals("F20")){
    bleKeyboard.press(KEY_F20);
  }else if (b.equals("F21")){
    bleKeyboard.press(KEY_F21);
  }else if (b.equals("F22")){
    bleKeyboard.press(KEY_F22);
  }else if (b.equals("F23")){
    bleKeyboard.press(KEY_F23);
  }else if (b.equals("F24")){
    bleKeyboard.press(KEY_F24);
  }else if (b.equals("SPACE")){
    bleKeyboard.press(' ');
  }else if (b.equals("NUMLOCK")){
    //bleKeyboard.press(0x53);
  }else if (b.equals("KP_SLASH")){
    bleKeyboard.press(KEY_NUM_SLASH);
  }else if (b.equals("KP_ASTERISK")){
    bleKeyboard.press(KEY_NUM_ASTERISK);
  }else if (b.equals("KP_MINUS")){
    bleKeyboard.press(KEY_NUM_MINUS);
  }else if (b.equals("KP_PLUS")){
    bleKeyboard.press(KEY_NUM_PLUS);
  }else if (b.equals("KP_ENTER")){
    bleKeyboard.press(KEY_NUM_ENTER);
  }else if (b.equals("KP_0")){
    bleKeyboard.press(KEY_NUM_0);
  }else if (b.equals("KP_1")){
    bleKeyboard.press(KEY_NUM_1);
  }else if (b.equals("KP_2")){
    bleKeyboard.press(KEY_NUM_2);
  }else if (b.equals("KP_3")){
    bleKeyboard.press(KEY_NUM_3);
  }else if (b.equals("KP_4")){
    bleKeyboard.press(KEY_NUM_4);
  }else if (b.equals("KP_5")){
    bleKeyboard.press(KEY_NUM_5);
  }else if (b.equals("KP_6")){
    bleKeyboard.press(KEY_NUM_6);
  }else if (b.equals("KP_7")){
    bleKeyboard.press(KEY_NUM_7);
  }else if (b.equals("KP_8")){
    bleKeyboard.press(KEY_NUM_8);
  }else if (b.equals("KP_9")){
    bleKeyboard.press(KEY_NUM_9);
  }else if (b.equals("KP_DOT")){
    bleKeyboard.press(KEY_NUM_PERIOD);
  }else if (b.equals("KP_EQUAL")){
    bleKeyboard.press('=');
  }else if (b.equals("VolUP")){
    bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
  }else if (b.equals("VolDOWN")){
    bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
  }else if (b.equals("MStop")){
    bleKeyboard.write(KEY_MEDIA_STOP);
  }else if (b.equals("MPause")){
    bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
  }else if (b.equals("MMute")){
    bleKeyboard.write(KEY_MEDIA_MUTE);
   }else if (b.equals("MNext")){
    bleKeyboard.write(KEY_MEDIA_NEXT_TRACK);
  }else if (b.equals("MPrev")){
    bleKeyboard.write(KEY_MEDIA_PREVIOUS_TRACK);
  }
}

uint32_t str2hex(String s){
  uint32_t x = 0;
  int largo = s.length();
  if(s.equals("WHITE")){
    x = 0xFFFFFF;
  }else if(s.equals("BLACK")){
    x = 0x000000;
  }else if(s.equals("RED")){
    x = 0xFF0000;
  }else if(s.equals("GREEN")){
    x = 0x00FF00;
  }else if(s.equals("BLUE")){
    x = 0x0000FF;
  }else if(s.equals("YELLOW")){
    x = 0xFFFF00;
  }else if(s.equals("CYAN")){
    x = 0x00FFFF;
  }else if(s.equals("MAGENTA")){
    x = 0xFF00FF;
  }else if(s.equals("OLIVE")){
    x = 0xAFB83B;
  }else if(s.equals("TEAL")){
    x = 0x158FAD;
  }else if(s.equals("LIME")){
    x = 0x7ECC49;
  }else if(s.equals("VIOLET")){
    x = 0xAF38EB;
  }else if(s.equals("PURPLE")){
    x = 0x884DFF;
  }else if(s.equals("GREY")){
    x = 0xB8B8B8;
  }else if(s.equals("SKY")){
    x = 0x14AAF5;
  }else if(s.equals("MINT")){
    x = 0x6ACCBC;
  }else if(s.equals("ORANGE")){
    x = 0xF88800;
  }else{
    for(int i = 0; i < largo; i++){
      char c = s.charAt(i);
      if (c >= '0' && c <= '9') {
        x *= 16;
        x += c - '0'; 
      }else if (c >= 'A' && c <= 'F') {
        x *= 16;
        x += (c - 'A') + 10; 
      }else if (c >= 'a' && c <= 'f') {
        x *= 16;
        x += (c - 'a') + 10;
      }else break;
    }
  }
  return x;
}

boolean listDir(fs::FS &fs, const char *dirname, uint8_t levels){
#ifdef DEBUG
  Serial.printf("Listing directory: %s\n", dirname);
#endif
  File root = fs.open(dirname);
  if (!root)
  {
#ifdef DEBUG
    Serial.println("Failed to open directory");
#endif
    return false;
  }
  if (!root.isDirectory())
  {
#ifdef DEBUG
    Serial.println("Not a directory");
#endif
    return false;
  }
  return true;
}

void displayNoSD(){
  lcd.fillScreen(0x000000U);
  lcd.setCursor(border_text, border_text);
  lcd.printf("SD NOT FOUND!!!");
  delay(5000);
}

void draw_rectangle(int x, int y, int w, int h, uint32_t color){
  lcd.fillRect(x, y, w, h, color);
}

uint16_t read16(fs::File &f){
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(fs::File &f){
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

bool bmpDraw(String filename, int16_t x, int16_t y){

  if ((x >= 480) || (y >= 320)) return false;

  fs::File bmpFS;

  bmpFS = SD.open(filename, "r");

  if (!bmpFS){
#ifdef DEBUG
    Serial.print("File not found:");
    Serial.println(filename);
#endif
    return false;
  }

  uint32_t seekOffset;
  uint16_t w, h, row;
  uint8_t r, g, b;

  if (read16(bmpFS) == 0x4D42){
    read32(bmpFS);
    read32(bmpFS);
    seekOffset = read32(bmpFS);
    read32(bmpFS);
    w = read32(bmpFS);
    h = read32(bmpFS);

    if ((read16(bmpFS) == 1) && (read16(bmpFS) == 24) && (read32(bmpFS) == 0)){
      y += h - 1;

      bool oldSwapBytes = lcd.getSwapBytes();
      lcd.setSwapBytes(true);
      bmpFS.seek(seekOffset);

      uint16_t padding = (4 - ((w * 3) & 3)) & 3;
      uint8_t lineBuffer[w * 3 + padding];

      for (row = 0; row < h; row++){

        bmpFS.read(lineBuffer, sizeof(lineBuffer));
        uint8_t *bptr = lineBuffer;
        uint16_t *tptr = (uint16_t *)lineBuffer;
        // Convert 24 to 16 bit colours
        for (uint16_t col = 0; col < w; col++)
        {
          b = *bptr++;
          g = *bptr++;
          r = *bptr++;
          *tptr++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);

        }

        // Push the pixel row to screen, pushImage will crop the line if needed
        // y is decremented as the BMP image is drawn bottom up
        lcd.pushImage(x, y--, w, 1, (uint16_t *)lineBuffer);
      }
      lcd.setSwapBytes(oldSwapBytes);
    }
    else{
#ifdef DEBUG
      Serial.println("[WARNING]: BMP format not recognized.");
#endif
      return false;
    }

  }
  bmpFS.close();
  return true;
}

void calculate_positions() {
  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLUMNS; j++) {
      int button_num = j + (i * COLUMNS);
      buttons_positions[button_num][0] = (max_pixel_per_col * j + thickness_spaces);
      buttons_positions[button_num][1] = (max_pixel_per_row * i + thickness_spaces);
    }
  }
}

void load_conf(){
  readConfFile(SD, "/conf.txt", 0);
}

void display_wallpaper() {
  wallpaperpathbmp = "/" + String(profile_index) + "/wallpaper.bmp";
  wallpaperpathjpg = "/" + String(profile_index) + "/wallpaper.jpg";
  if (!SD.exists(wallpaperpathbmp)){
    if (!SD.exists(wallpaperpathjpg)){
    Serial.println("No wallpaper found in profile: "); Serial.print(profile_index);
    lcd.fillScreen(0x000000U);
    }
    else{
      Serial.println("JPG Wallpaper found for profile: "); Serial.print(profile_index);
      jpgDraw(wallpaperpathjpg, 0, 0);
    }
  }
  else{
    Serial.println("BMP Wallpaper found for profile: "); Serial.print(profile_index);
    bmpDraw(wallpaperpathbmp, 0, 0);
  }
}
void jpgDraw(String filename, int16_t x, int16_t y) {
    // The jpeg image can be scaled by a factor of 1, 2, 4, or 8
    TJpgDec.setJpgScale(1);
    // The decoder must be given the exact name of the rendering function above
    TJpgDec.setCallback(tft_output);
    TJpgDec.drawSdJpg(x, y, filename);
}
void show_splash() {
    if (!SD.exists("/splashscreen.bmp")){
    if (!SD.exists("/splashscreen.jpg")){
    Serial.println("No splashscreen found");
    }
    else{
      Serial.println("JPG splashscreen found");
      jpgDraw("/splashscreen.jpg", 0, 0);
    }

  }
  else{
    Serial.println("BMP splashscreen found");
    bmpDraw("/splashscreen.bmp", 0, 0);
  }
}

void display_buttons() {
  lcd.setTextSize(FONT_SIZE);
  lcd.setTextColor(0x000000U);
  char bufc[9];
  String archive = "/" + String(profile_index);
  char buf[12];
  archive.toCharArray(buf,archive.length()+1);
  if(!listDir(SD, buf, 0)){
    lcd.fillScreen(0x000000U);
    lcd.setCursor(border_text, border_text);
    lcd.printf("Profile does NOT exist");
    delay(2000);
    profile_index = last_profile;
    //return;
  }
  for(int a=0; a < TOTAL_BUTTONS; a++){
    archive = "/" + String(profile_index) + "/" + String(a) + ".bmp";
    strcpy(buf,"");
    archive.toCharArray(buf,archive.length()+1);
    if(bmpDraw(buf,buttons_positions[a][0], buttons_positions[a][1])){
      buttons_names[a] = "";
      buttons_colors[a] = 0x000000U;
    }else{
      archive = "/" + String(profile_index) + "/" + String(a) + ".txt";
      strcpy(buf,"");
      archive.toCharArray(buf,archive.length()+1);
      readConfFile(SD, buf, a);
      draw_rectangle(buttons_positions[a][0], buttons_positions[a][1], b_width, b_height, buttons_colors[a]);
      buttons_names[a].toCharArray(bufc,buttons_names[a].length()+1);
      for(int x=0; x < max_rows_per_button; x++){
        for(int y=0; y < max_letters_per_row; y++){
          if((y+x*max_letters_per_row) < buttons_names[a].length()){
            lcd.setCursor(buttons_positions[a][0]+border_text+(y*font_width), buttons_positions[a][1]+border_text+(x*font_height));
            lcd.print(bufc[y+x*max_letters_per_row]);
          }
        }
      }
    }
  }
}

void writeFile(fs::FS &fs, const char *path, const char *message){
#ifdef DEBUG
  Serial.printf("Writing file: %s\n", path);
#endif
  File file = fs.open(path, FILE_WRITE);
  if (!file){
#ifdef DEBUG
    Serial.println("Failed to open file for writing");
#endif
    return;
  }
  if (file.print(message)){
#ifdef DEBUG
    Serial.println("File written");
#endif
  }else{
#ifdef DEBUG
    Serial.println("Write failed");
#endif
  }
  file.close();
}


// This next function will be called during decoding of the jpeg file to
// render each block to the TFT.  If you use a different TFT library
// you will need to adapt this function to suit.
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
   // Stop further decoding as image is running off bottom of screen
  if ( y >= lcd.height() ) return 0;

  // This function will clip the image block rendering automatically at the TFT boundaries
  lcd.pushImage(x, y, w, h, bitmap);

  // This might work instead if you adapt the sketch to use the Adafruit_GFX library
  // tft.drawRGBBitmap(x, y, bitmap, w, h);

  // Return 1 to decode next block
  return 1;
}

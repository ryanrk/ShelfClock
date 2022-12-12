#define FASTLED_ESP32_FLASH_LOCK 1
#include <NonBlockingRtttl.h>
#include <MegunoLink.h>
#include <FastLED.h>
#include <WiFi.h>
#include "WebServer.h"
#include <FS.h>     
#include <HTTPUpdateServer.h>
#include "DHT.h"
#include <Preferences.h>
#include <ESPmDNS.h>
#include <Update.h>
#include "RTClib.h"
#include <AutoConnect.h>
#include <ArduinoJson.h>
#include "../include/ShelfClick.h"


#define LED_TYPE  WS2812B
#define COLOR_ORDER GRB
#define DHTTYPE DHT11         // DHT 11 tempsensor
#define MIC_IN_PIN 34         // Use 34 for mic input
#define AUDIO_GATE_PIN 15     // for sound gate input trigger
#define BUZZER_PIN 16         // peizo speaker
#define DHT_PIN 18            // temp sensor pin
#define LED_PIN 2             // led control pin
#define PHOTORESISTER_PIN 36  // select the analog input pin for the photoresistor
#define MILLI_AMPS 2400 
#define NUM_LEDS  350
#define NUMBER_OF_DIGITS 7 //4 real, 3 fake
#define LEDS_PER_SEGMENT 7
#define SEGMENTS_PER_NUMBER 7
#define LEDS_PER_DIGIT (LEDS_PER_SEGMENT * SEGMENTS_PER_NUMBER)
#define FAKE_NUM_LEDS (NUMBER_OF_DIGITS * LEDS_PER_DIGIT)
#define SPECTRUM_PIXELS  37  //times 7 to get all 258 leds
#define PHOTO_SAMPLES 15  //number of samples to take from the photoresister


const char* host = "shelfclock";
const int   daylightOffset_sec = 3600;
const char* ntpServer = "pool.ntp.org";
int breakOutSet = 0; //jump out of count
int colorWheelPosition = 255; // COLOR WHEEL POSITION
int colorWheelPositionTwo = 255; // 2nd COLOR WHEEL POSITION
int decay = 0; // HOW MANY MS BEFORE ONE LIGHT DECAY
int decay_check = 0;
long pre_react = 0; // NEW SPIKE CONVERSION
long react = 0; // NUMBER OF LEDs BEING LIT
long post_react = 0; // OLD SPIKE CONVERSION
const int colorWheelSpeed = 3;
int sleepTimerCurrent = 0;
int isAsleep = 0;
int photoresisterReadings[PHOTO_SAMPLES];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int lightSensorValue = 255;
int previousTimeMin = 0;
int previousTimeHour = 0;
int previousTimeDay = 0;
int previousTimeWeek = 0;
int previousTimeMonth = 0;
byte randomMinPassed = 1;
byte randomHourPassed = 1;
byte randomDayPassed = 1;
byte randomWeekPassed = 1;
byte randomMonthPassed = 1;
bool dotsOn = true;
unsigned long prevTime = 0;
unsigned long prevTime2 = 0;
unsigned long countdownMilliSeconds;
unsigned long endCountDownMillis;
byte  fakeclockrunning = 0;    
unsigned long countupMilliSeconds;
unsigned long CountUpMillis;
int scoreboardLeft = 0;
int scoreboardRight = 0; 
int currentMode = 0;
int currentReal = 0;
int cylonPosition = 0;
int clearOldLeds = 0; 
byte rain[SPECTRUM_PIXELS];
byte greenMatrix[SPECTRUM_PIXELS];
int lightshowSpeed = 1;
int snakeLastDirection = 0;  //snake's last dirction
int snakePosition = 0;  //snake's position
int foodSpot = random(36);  //food spot
int snakeWaiting = 0;  //waiting
int getSlower = 180;
int daysUptime = 0;
int averageAudioInput = 0;
float outdoorTemp = -500;
struct TemperatureConfig tempConfig;

struct tm timeinfo; 
CRGB LEDs[NUM_LEDS];
Preferences preferences;
DHT dht(DHT_PIN, DHTTYPE);
WebServer server(80);
HTTPUpdateServer httpUpdateServer;
RTC_DS3231 rtc;
AutoConnect      Portal(server);
AutoConnectConfig  Config;

// global settings that get saved to flash via preffs
byte cd_r_val = 0;
byte cd_g_val = 255;
byte cd_b_val = 0;
byte r0_val = 255;  //spotlight
byte g0_val = 255;
byte b0_val = 255;
byte r1_val = 255;  //colorHour
byte g1_val = 0;
byte b1_val = 0;
byte r2_val = 255;  //colorMins
byte g2_val = 0;
byte b2_val = 0;
byte r3_val = 255;   //colorColon
byte g3_val = 0;
byte b3_val = 0;
byte r4_val = 255;   //dayColor
byte g4_val = 0;
byte b4_val = 0;
byte r5_val = 255;   //monthColor
byte g5_val = 0;
byte b5_val = 0;
byte r6_val = 255;    //separatorColor
byte g6_val = 0;
byte b6_val = 0;
byte r7_val = 255;  //TempColor
byte g7_val = 0;
byte b7_val = 0;
byte r8_val = 255;  //TypeColor
byte g8_val = 0;
byte b8_val = 0;
byte r9_val = 255;  //DegreeColor
byte g9_val = 0;
byte b9_val = 0;
byte r10_val = 255;  //HumiColor
byte g10_val = 0;
byte b10_val = 0;
byte r11_val = 255;  //SymbolColor
byte g11_val = 0;
byte b11_val = 0;
byte r12_val = 255;  //HumiDecimalColor
byte g12_val = 0;
byte b12_val = 0;
byte r13_val = 255;  //scoreboard left
byte g13_val = 0;
byte b13_val = 0;
byte r14_val = 255;  //scoreboard right
byte g14_val = 0;
byte b14_val = 0;
byte r15_val = 255;  //spectrum analyzer
byte g15_val = 0;
byte b15_val = 0;
byte r16_val = 255;  //scrolling text analyzer
byte g16_val = 255;
byte b16_val = 255;
byte r17_val = 0;  //spectrum analyzer background
byte g17_val = 0;
byte b17_val = 0;
byte clockMode = 0;   // Clock modes: 0=Clock, 1=Countdown, 2=Temperature, 3=Scoreboard, 4=Stopwatch, 5=Lightshow, 6=Rainbows/Scroll, 7=Date, 8=Humidity, 9=Spectrum, 10=Display Off
byte clockDisplayType = 3; //0-Center Times, 1-24-hour Military Time, 2-12-hour Space-Padded, 3-Blinking Center Light
byte dateDisplayType = 5; //0-Zero-Padded (MMDD), 1-Space-Padded (MMDD), 2-Center Dates (1MDD), 3-Just Day of Week (Sun), 4-Just Numeric Day (DD), 5-With "." Separator (MM.DD), 6-Just Year (YYYY)
byte tempDisplayType = 0; //0-Temperature with Degree and Type (79°F), 1-Temperature with just Type (79 F), 2-Temperature with just Degree (79°), 3-Temperature with Decimal (79.9), 4-Just Temperature (79)
byte humiDisplayType = 0; //0-Humidity with Symbol (34 H), 1-Humidity with Decimal (34.9), 2-Just Humidity (79)
byte pastelColors = 0;
byte temperatureSymbol = 39;   // 36=Celcius, 39=Fahrenheit check 'numbers'
bool DSTime = 0;
long gmtOffset_sec = -28800;
byte ClockColorSettings = 0;
byte DateColorSettings = 0;
byte tempColorSettings = 0;
byte humiColorSettings = 0;
int temperatureCorrection = 0;
int colonType = 0;
byte ColorChangeFrequency = 0;
byte brightness = 10;  //set to 10 for photoresister control at startup
String scrollText = "dAdS ArE tHE bESt";
bool colorchangeCD = 1;
bool useAudibleAlarm = 0;
int spectrumMode = 0;
int spectrumColorSettings = 2;
int spectrumBackgroundSettings = 0;
int realtimeMode = 0;
int spotlightsColorSettings = 0;
bool useSpotlights = 1;
int scrollColorSettings = 0;
bool scrollOverride = 1;
bool scrollOptions1 = 0;   //Military Time (HHMM)
bool scrollOptions2 = 0;   //Day of Week (DOW)
bool scrollOptions3 = 0;   //Today's Date (DD-MM)
bool scrollOptions4 = 0;   //Year (YYYY)
bool scrollOptions5 = 0;   //Temperature (70 °F)
bool scrollOptions6 = 0;   //Humidity (47 H)
bool scrollOptions7 = 0;   //Text Message
bool scrollOptions8 = 0;   //IP Address of Clock
int scrollFrequency = 1;
int lightshowMode = 0;
byte randomSpectrumMode = 0;
int suspendFrequency = 1;  //in minutes
byte suspendType = 0; //0-off, 1-digits-only, 2-everything

CRGB hourColor = CRGB(r1_val, g1_val, b1_val); 
CRGB colonColor = CRGB(r3_val, g3_val, b3_val); 
CRGB spotlightsColor = CRGB(r0_val, g0_val, b0_val);
CRGB alternateColor = CRGB::Black; 
CRGB scrollColor = CRGB(r16_val, g16_val, b16_val);
CRGB spectrumColor = CRGB(r15_val, g15_val, b15_val);
CRGB spectrumBackground = CRGB(r17_val, g17_val, b17_val);
CRGB humiColor = CRGB(r10_val, g10_val, b10_val);
CRGB tinyhumiColor = CRGB(r10_val, g10_val, b10_val);
CRGB symbolColor = CRGB(r11_val, g11_val, b11_val);
CRGB humiDecimalColor = CRGB(r12_val, g12_val, b12_val);
CRGB tinytempColor = CRGB(r7_val, g7_val, b7_val);
CRGB tempColor = CRGB(r7_val, g7_val, b7_val);
CRGB typeColor = CRGB(r8_val, g8_val, b8_val);
CRGB degreeColor = CRGB(r9_val, g9_val, b9_val);
CRGB dayColor = CRGB(r4_val, g4_val, b4_val);
CRGB monthColor = CRGB(r5_val, g5_val, b5_val);
CRGB tinymonthColor = CRGB(r5_val, g5_val, b5_val);
CRGB separatorColor = CRGB(r6_val, g6_val, b6_val);
CRGB minColor = CRGB(r2_val, g2_val, b2_val);
CRGB tinyhourColor = CRGB(r3_val, g3_val, b3_val);
CRGB lightchaseColorOne = CRGB::Blue;
CRGB lightchaseColorTwo = CRGB::Red;
CRGB oldsnakecolor = CRGB::Green;
CRGB spotcolor = CHSV(random(0, 255), 255, 255);

const uint16_t FAKE_LEDs[FAKE_NUM_LEDS] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,  //real digit #0
                                      20,19,18,17,16,15,14,49,50,51,52,53,54,55,76,75,74,73,72,71,70,111,110,109,108,107,106,105,56,57,58,59,60,61,62,27,26,25,24,23,22,21,63,64,65,66,67,68,69,  //fake digit #1
                                      70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,  //real digit #2
                                      90,89,88,87,86,85,84,119,120,121,122,123,124,125,146,145,144,143,142,141,140,181,180,179,178,177,176,175,126,127,128,129,130,131,132,97,96,95,94,93,92,91,133,134,135,136,137,138,139,  //fake digit #3
                                      140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,  //real digit #4
                                      160,159,158,157,156,155,154,189,190,191,192,193,194,195,216,215,214,213,212,211,210,251,250,249,248,247,246,245,196,197,198,199,200,201,202,167,166,165,164,163,162,161,203,204,205,206,207,208,209,  //fake digit #5
                                      210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,256,257,258};  //real digit #6

//fake LED layout for spectrum (from the middle out)
const uint16_t FAKE_LEDs_C_BMUP[259] = {119,120,121,122,123,124,125,77,78,79,80,81,82,83,147,148,149,150,151,152,153,84,85,86,87,88,89,90,140,141,142,143,144,145,146,133,134,135,136,137,138,139,189,190,191,192,193,194,195,49,50,51,52,53,54,55,154,155,156,157,158,159,160,70,71,72,73,74,75,76,182,183,184,185,186,187,188,112,113,114,115,116,117,118,175,176,177,178,179,180,181,91,92,93,94,95,96,97,126,127,128,129,130,131,132,7,8,9,10,11,12,13,217,218,219,220,221,222,223,14,15,16,17,18,19,20,210,211,212,213,214,215,216,63,64,65,66,67,68,69,203,204,205,206,207,208,209,105,106,107,108,109,110,111,161,162,163,164,165,166,167,98,99,100,101,102,103,104,168,169,170,171,172,173,174,0,1,2,3,4,5,6,224,225,226,227,228,229,230,42,43,44,45,46,47,48,252,253,254,255,256,257,258,21,22,23,24,25,26,27,245,246,247,248,249,250,251,56,57,58,59,60,61,62,196,197,198,199,200,201,202,35,36,37,38,39,40,41,231,232,233,234,235,236,237,28,29,30,31,32,33,34,238,239,240,241,242,243,244};
//fake LED layout for spectrum (bfrom the outside in)
const uint16_t FAKE_LEDs_C_CMOT[259] = {133,134,135,136,137,138,139,182,183,184,185,186,187,188,112,113,114,115,116,117,118,140,141,142,143,144,145,146,91,92,93,94,95,96,97,175,176,177,178,179,180,181,84,85,86,87,88,89,90,119,120,121,122,123,124,125,126,127,128,129,130,131,132,147,148,149,150,151,152,153,98,99,100,101,102,103,104,168,169,170,171,172,173,174,77,78,79,80,81,82,83,203,204,205,206,207,208,209,63,64,65,66,67,68,69,154,155,156,157,158,159,160,105,106,107,108,109,110,111,161,162,163,164,165,166,167,70,71,72,73,74,75,76,196,197,198,199,200,201,202,49,50,51,52,53,54,55,189,190,191,192,193,194,195,56,57,58,59,60,61,62,252,253,254,255,256,257,258,42,43,44,45,46,47,48,210,211,212,213,214,215,216,21,22,23,24,25,26,27,245,246,247,248,249,250,251,14,15,16,17,18,19,20,238,239,240,241,242,243,244,7,8,9,10,11,12,13,217,218,219,220,221,222,223,28,29,30,31,32,33,34,224,225,226,227,228,229,230,35,36,37,38,39,40,41,231,232,233,234,235,236,237,0,1,2,3,4,5,6};
//fake LED layout for spectrum (bottom-left to right-top)
const uint16_t FAKE_LEDs_C_BLTR[259] = {224,225,226,227,228,229,230,217,218,219,220,221,222,223,189,190,191,192,193,194,195,210,211,212,213,214,215,216,252,253,254,255,256,257,258,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,203,204,205,206,207,208,209,154,155,156,157,158,159,160,147,148,149,150,151,152,153,119,120,121,122,123,124,125,140,141,142,143,144,145,146,182,183,184,185,186,187,188,161,162,163,164,165,166,167,196,197,198,199,200,201,202,168,169,170,171,172,173,174,175,176,177,178,179,180,181,133,134,135,136,137,138,139,84,85,86,87,88,89,90,77,78,79,80,81,82,83,49,50,51,52,53,54,55,70,71,72,73,74,75,76,112,113,114,115,116,117,118,91,92,93,94,95,96,97,126,127,128,129,130,131,132,98,99,100,101,102,103,104,105,106,107,108,109,110,111,63,64,65,66,67,68,69,14,15,16,17,18,19,20,7,8,9,10,11,12,13,0,1,2,3,4,5,6,42,43,44,45,46,47,48,21,22,23,24,25,26,27,56,57,58,59,60,61,62,28,29,30,31,32,33,34,35,36,37,38,39,40,41};
//fake LED layout for spectrum (top-left to bottom-right) 
const uint16_t FAKE_LEDs_C_TLBR[259] = {238,239,240,241,242,243,244,231,232,233,234,235,236,237,224,225,226,227,228,229,230,252,253,254,255,256,257,258,245,246,247,248,249,250,251,196,197,198,199,200,201,202,168,169,170,171,172,173,174,161,162,163,164,165,166,167,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,189,190,191,192,193,194,195,154,155,156,157,158,159,160,182,183,184,185,186,187,188,175,176,177,178,179,180,181,126,127,128,129,130,131,132,98,99,100,101,102,103,104,91,92,93,94,95,96,97,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,119,120,121,122,123,124,125,84,85,86,87,88,89,90,112,113,114,115,116,117,118,105,106,107,108,109,110,111,56,57,58,59,60,61,62,28,29,30,31,32,33,34,21,22,23,24,25,26,27,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,49,50,51,52,53,54,55,14,15,16,17,18,19,20,42,43,44,45,46,47,48,35,36,37,38,39,40,41,0,1,2,3,4,5,6,7,8,9,10,11,12,13};
//fake LED layout for spectrum (top-middle down)  
const uint16_t FAKE_LEDs_C_TMDN[259] = {126,127,128,129,130,131,132,98,99,100,101,102,103,104,168,169,170,171,172,173,174,91,92,93,94,95,96,97,175,176,177,178,179,180,181,133,134,135,136,137,138,139,56,57,58,59,60,61,62,196,197,198,199,200,201,202,105,106,107,108,109,110,111,161,162,163,164,165,166,167,112,113,114,115,116,117,118,182,183,184,185,186,187,188,84,85,86,87,88,89,90,140,141,142,143,144,145,146,119,120,121,122,123,124,125,28,29,30,31,32,33,34,238,239,240,241,242,243,244,21,22,23,24,25,26,27,245,246,247,248,249,250,251,63,64,65,66,67,68,69,203,204,205,206,207,208,209,70,71,72,73,74,75,76,154,155,156,157,158,159,160,77,78,79,80,81,82,83,147,148,149,150,151,152,153,35,36,37,38,39,40,41,231,232,233,234,235,236,237,42,43,44,45,46,47,48,252,253,254,255,256,257,258,14,15,16,17,18,19,20,210,211,212,213,214,215,216,49,50,51,52,53,54,55,189,190,191,192,193,194,195,0,1,2,3,4,5,6,224,225,226,227,228,229,230,7,8,9,10,11,12,13,217,218,219,220,221,222,223};
//fake LED layout for spectrum (center-sides in)  
const uint16_t FAKE_LEDs_C_CSIN[259] = {35,36,37,38,39,40,41,224,225,226,227,228,229,230,0,1,2,3,4,5,6,231,232,233,234,235,236,237,42,43,44,45,46,47,48,252,253,254,255,256,257,258,28,29,30,31,32,33,34,217,218,219,220,221,222,223,7,8,9,10,11,12,13,238,239,240,241,242,243,244,21,22,23,24,25,26,27,210,211,212,213,214,215,216,14,15,16,17,18,19,20,245,246,247,248,249,250,251,63,64,65,66,67,68,69,203,204,205,206,207,208,209,56,57,58,59,60,61,62,189,190,191,192,193,194,195,49,50,51,52,53,54,55,196,197,198,199,200,201,202,105,106,107,108,109,110,111,154,155,156,157,158,159,160,70,71,72,73,74,75,76,161,162,163,164,165,166,167,112,113,114,115,116,117,118,182,183,184,185,186,187,188,98,99,100,101,102,103,104,147,148,149,150,151,152,153,77,78,79,80,81,82,83,168,169,170,171,172,173,174,91,92,93,94,95,96,97,140,141,142,143,144,145,146,84,85,86,87,88,89,90,175,176,177,178,179,180,181,133,134,135,136,137,138,139,126,127,128,129,130,131,132,119,120,121,122,123,124,125};
//fake LED layout for spectrum (bottom-right to left-top)
const uint16_t FAKE_LEDs_C_BRTL[259] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,49,50,51,52,53,54,55,14,15,16,17,18,19,20,48,47,46,45,44,43,42,35,36,37,38,39,40,41,28,29,30,31,32,33,34,21,22,23,24,25,26,27,69,68,67,66,65,64,63,70,71,72,73,74,75,76,77,78,79,80,81,82,83,125,124,123,122,121,120,119,84,85,86,87,88,89,90,118,117,116,115,114,113,112,105,106,107,108,109,110,111,56,57,58,59,60,61,62,98,99,100,101,102,103,104,91,92,93,94,95,96,97,139,138,137,136,135,134,133,146,145,144,143,142,141,140,153,152,151,150,149,148,147,195,194,193,192,191,190,189,160,159,158,157,156,155,154,182,183,184,185,186,187,188,181,180,179,178,177,176,175,132,131,130,129,128,127,126,174,173,172,171,170,169,168,167,166,165,164,163,162,161,203,204,205,206,207,208,209,216,215,214,213,212,211,210,223,222,221,220,219,218,217,230,229,228,227,226,225,224,252,253,254,255,256,257,258,251,250,249,248,247,246,245,202,201,200,199,198,197,196,244,243,242,241,240,239,238,237,236,235,234,233,232,231};
//fake LED layout for spectrum (top-right to bottom-left)
const uint16_t FAKE_LEDs_C_TRBL[259] = {28,29,30,31,32,33,34,35,36,37,38,39,40,41,0,1,2,3,4,5,6,42,43,44,45,46,47,48,21,22,23,24,25,26,27,56,57,58,59,60,61,62,98,99,100,101,102,103,104,105,106,107,108,109,110,111,63,64,65,66,67,68,69,14,15,16,17,18,19,20,7,8,9,10,11,12,13,49,50,51,52,53,54,55,70,71,72,73,74,75,76,112,113,114,115,116,117,118,91,92,93,94,95,96,97,126,127,128,129,130,131,132,168,169,170,171,172,173,174,175,176,177,178,179,180,181,133,134,135,136,137,138,139,84,85,86,87,88,89,90,77,78,79,80,81,82,83,119,120,121,122,123,124,125,140,141,142,143,144,145,146,182,183,184,185,186,187,188,161,162,163,164,165,166,167,196,197,198,199,200,201,202,238,239,240,241,242,243,244,245,246,247,248,249,250,251,203,204,205,206,207,208,209,154,155,156,157,158,159,160,147,148,149,150,151,152,153,189,190,191,192,193,194,195,210,211,212,213,214,215,216,252,253,254,255,256,257,258,231,232,233,234,235,236,237,224,225,226,227,228,229,230,217,218,219,220,221,222,223};
//fake LED layout for spectrum (horizontal parts)   
const uint16_t FAKE_LEDs_C_OUTS[259] = {217,218,219,220,221,222,223,252,253,254,255,256,257,258,238,239,240,241,242,243,244,189,190,191,192,193,194,195,203,204,205,206,207,208,209,196,197,198,199,200,201,202,147,148,149,150,151,152,153,182,183,184,185,186,187,188,168,169,170,171,172,173,174,119,120,121,122,123,124,125,133,134,135,136,137,138,139,126,127,128,129,130,131,132,77,78,79,80,81,82,83,112,113,114,115,116,117,118,98,99,100,101,102,103,104,49,50,51,52,53,54,55,63,64,65,66,67,68,69,56,57,58,59,60,61,62,7,8,9,10,11,12,13,42,43,44,45,46,47,48,28,29,30,31,32,33,34,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281};
const uint16_t FAKE_LEDs_C_OUTS2[259] = {7,8,9,10,11,12,13,42,43,44,45,46,47,48,28,29,30,31,32,33,34,49,50,51,52,53,54,55,63,64,65,66,67,68,69,56,57,58,59,60,61,62,77,78,79,80,81,82,83,112,113,114,115,116,117,118,98,99,100,101,102,103,104,119,120,121,122,123,124,125,133,134,135,136,137,138,139,126,127,128,129,130,131,132,147,148,149,150,151,152,153,182,183,184,185,186,187,188,168,169,170,171,172,173,174,189,190,191,192,193,194,195,203,204,205,206,207,208,209,196,197,198,199,200,201,202,217,218,219,220,221,222,223,252,253,254,255,256,257,258,238,239,240,241,242,243,244,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281};
//fake LED layout for spectrum (vertical parts)  
const uint16_t FAKE_LEDs_C_VERT[259] = {224,225,226,227,228,229,230,231,232,233,234,235,236,237,210,211,212,213,214,215,216,245,246,247,248,249,250,251,154,155,156,157,158,159,160,161,162,163,164,165,166,167,140,141,142,143,144,145,146,175,176,177,178,179,180,181,84,85,86,87,88,89,90,91,92,93,94,95,96,97,70,71,72,73,74,75,76,105,106,107,108,109,110,111,14,15,16,17,18,19,20,21,22,23,24,25,26,27,0,1,2,3,4,5,6,35,36,37,38,39,40,41,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281};
const uint16_t FAKE_LEDs_C_VERT2[259] = {0,1,2,3,4,5,6,35,36,37,38,39,40,41,14,15,16,17,18,19,20,21,22,23,24,25,26,27,70,71,72,73,74,75,76,105,106,107,108,109,110,111,84,85,86,87,88,89,90,91,92,93,94,95,96,97,140,141,142,143,144,145,146,175,176,177,178,179,180,181,154,155,156,157,158,159,160,161,162,163,164,165,166,167,210,211,212,213,214,215,216,245,246,247,248,249,250,251,224,225,226,227,228,229,230,231,232,233,234,235,236,237,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281,275,276,277,278,279,280,281};

//fake LED layout for fire display  
const uint16_t FAKE_LEDs_C_FIRE[259] = {119,120,121,122,123,124,125,77,78,79,80,81,82,83,147,148,149,150,151,152,153,84,85,86,87,88,89,90,140,141,142,143,144,145,146,133,134,135,136,137,138,139,189,190,191,192,193,194,195,49,50,51,52,53,54,55,154,155,156,157,158,159,160,70,71,72,73,74,75,76,182,183,184,185,186,187,188,112,113,114,115,116,117,118,175,176,177,178,179,180,181,91,92,93,94,95,96,97,126,127,128,129,130,131,132,7,8,9,10,11,12,13,217,218,219,220,221,222,223,14,15,16,17,18,19,20,210,211,212,213,214,215,216,63,64,65,66,67,68,69,203,204,205,206,207,208,209,105,106,107,108,109,110,111,161,162,163,164,165,166,167,98,99,100,101,102,103,104,168,169,170,171,172,173,174,0,1,2,3,4,5,6,224,225,226,227,228,229,230,42,43,44,45,46,47,48,252,253,254,255,256,257,258,21,22,23,24,25,26,27,245,246,247,248,249,250,251,56,57,58,59,60,61,62,196,197,198,199,200,201,202,35,36,37,38,39,40,41,231,232,233,234,235,236,237,28,29,30,31,32,33,34,238,239,240,241,242,243,244};
//fake LED layout for Rain display  
const uint16_t FAKE_LEDs_C_RAIN[259] = {210,211,212,213,214,215,216,245,246,247,248,249,250,251,7,8,9,10,11,12,13,42,43,44,45,46,47,48,28,29,30,31,32,33,34,154,155,156,157,158,159,160,161,162,163,164,165,166,167,217,218,219,220,221,222,223,252,253,254,255,256,257,258,238,239,240,241,242,243,244,84,85,86,87,88,89,90,91,92,93,94,95,96,97,49,50,51,52,53,54,55,63,64,65,66,67,68,69,56,57,58,59,60,61,62,0,1,2,3,4,5,6,35,36,37,38,39,40,41,119,120,121,122,123,124,125,133,134,135,136,137,138,139,126,127,128,129,130,131,132,70,71,72,73,74,75,76,105,106,107,108,109,110,111,147,148,149,150,151,152,153,182,183,184,185,186,187,188,168,169,170,171,172,173,174,14,15,16,17,18,19,20,21,22,23,24,25,26,27,189,190,191,192,193,194,195,203,204,205,206,207,208,209,196,197,198,199,200,201,202,140,141,142,143,144,145,146,175,176,177,178,179,180,181,77,78,79,80,81,82,83,112,113,114,115,116,117,118,98,99,100,101,102,103,104,224,225,226,227,228,229,230,231,232,233,234,235,236,237};
//fake LED layout for Snake display 
const uint16_t FAKE_LEDs_SNAKE[259] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,49,50,51,52,53,54,55,14,15,16,17,18,19,20,42,43,44,45,46,47,48,35,36,37,38,39,40,41,28,29,30,31,32,33,34,21,22,23,24,25,26,27,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,119,120,121,122,123,124,125,84,85,86,87,88,89,90,112,113,114,115,116,117,118,105,106,107,108,109,110,111,56,57,58,59,60,61,62,98,99,100,101,102,103,104,91,92,93,94,95,96,97,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,189,190,191,192,193,194,195,154,155,156,157,158,159,160,182,183,184,185,186,187,188,175,176,177,178,179,180,181,126,127,128,129,130,131,132,168,169,170,171,172,173,174,161,162,163,164,165,166,167,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,252,253,254,255,256,257,258,245,246,247,248,249,250,251,196,197,198,199,200,201,202,238,239,240,241,242,243,244,231,232,233,234,235,236,237};


const char * rickroll2 = "Together:d=8,o=5,b=225:4d#,f.,c#.,c.6,4a#.,4g.,f.,d#.,c.,4a#,2g#,4d#,f.,c#.,c.6,2a#,g.,f.,1d#.,d#.,4f,c#.,c.6,2a#,4g.,4f,d#.,f.,g.,4g#.,g#.,4a#,c.6,4c#6,4c6,4a#,4g.,4g#,4a#,2g#";
const char * auldlang = "AuldLang:d=4,o=6,b=125:a5,d.,8d,d,f#,e.,8d,e,8f#,8e,d.,8d,f#,a,2b.,b,a.,8f#,f#,d,e.,8d,e,8f#,8e,d.,8b5,b5,a5,2d,16p";
const char * startrek = "Star Trek:d=4,o=5,b=63:8f.,16a#,d#.6,8d6,16a#.,16g.,16c.6,f6";
const char * starwars = "StarWars:d=4,o=5,b=250:8a,8p,8d6,8p,8a,8p,8d6,8p,8a,8d6,8p,8a,8p,8g#,a,8a,8g#,8a,g,8f#,8g,8f#,f.,8d.,16p,p.,8a,8p,8d6,8p,8a,8p,8d6,8p,8a,8d6,8p,8a,8p,8g#,8a,8p,8g,8p,g.,8f#,8g,8p,8c6,a#,a,g";
const char * birthday = "HappyBir:d=8,o=5,b=100:16c,16c,d,c,f,e.,16p,16c,16c,d,c,g,f.,16p,16c,16c,c6,a,f,e,d.,16p,16a#,16a#,a,f,g,f.";
const char * rickroll = "Never Gonna:d=4,o=5,b=200:8g,8a,8c6,8a,e6,8p,e6,8p,d6.,p,8p,8g,8a,8c6,8a,d6,8p,d6,8p,c6,8b,a.,8g,8a,8c6,8a,2c6,d6,b,a,g.,8p,g,2d6,2c6.,p,8g,8a,8c6,8a,e6,8p,e6,8p,d6.,p,8p,8g,8a,8c6,8a,2g6,b,c6.,8b,a,8g,8a,8c6,8a,2c6,d6,b,a,g.,8p,g,2d6,2c6";
const char * cinco = "Macarena:d=16,o=5,b=180:4f6,8f6,8f6,4f6,8f6,8f6,8f6,8f6,8f6,8f6,8f6,8a6,8c6,8c6,4f6,8f6,8f6,4f6,8f6,8f6,8f6,8f6,8f6,8f6,8d6,8c6,4p,4f6,8f6,8f6,4f6,8f6,8f6,8f6,8f6,8f6,8f6,8f6,8a6,4p,2c.7,4a6,8c7,8a6,8f6,4p,2p";
const char * xmas = "WeWishYo:d=4,o=5,b=200:d,g,8g,8a,8g,8f#,e,e,e,a,8a,8b,8a,8g,f#,d,d,b,8b,8c6,8b,8a,g,e,d,e,a,f#,2g,d,g,8g,8a,8g,8f#,e,e,e,a,8a,8b,8a,8g,f#,d,d,b,8b,8c6,8b,8a,g,e,d,e,a,f#,1g,d,g,g,g,2f#,f#,g,f#,e,2d,a,b,8a,8a,8g,8g,d6,d,d,e,a,f#,2g";
const char * macgyver = "MacGyver:d=8,o=5,b=160:c6,c6,c6,c6,c6,c6,c6,c6,2b,f#,4a,2g,p,c6,4c6,4b,a,b,a,4g,4e6,2a,c6,4c6,2b,p,f#,4a,2g,p,c6,4c6,4b,a,b,a,4g,4e6,2a,2b,c6,b,a,4c6,b,a,4d6,c6,b,4d6,c6,b,4e6,d6,e6,4f#6,4b,1g6";
const char * aniver = "TakeOnMe:d=16,o=5,b=100:8p,a#,a#,a#,8f#,8d#,8g#,8g#,g#,c6,c6,c#6,d#6,c#6,c#6,c#6,8g#,8f#,8a#,8a#,a#,g#,g#,a#,g#,a#,a#,a#,8f#,8d#,8g#,8g#,g#,c6,c6,c#6,d#6,c#6,c#6,c#6,8g#,8f#,8a#,8a#";
const char * melody = "NokiaTun:d=4,o=5,b=225:8e6,8d6,f#,g#,8c#6,8b,d,e,8b,8a,c#,e,2a";
const char * halloween = "Hallowee:d=4,o=5,b=160:8c6,8f,8f,8c6,8f,8f,8c6,8f,8c#6,8f,8c6,8f,8f,8c6,8f,8c6,8f,8c#6,8f,8b,8e,8e,8b,8e,8e,8b,8e,8c6,8e,8b,8e,8e,8b,8e,8e,8b,16e";
const char * mandy = "Mandy:d=8,o=6,b=120:d#,f,d#,d,4c.,f5,d#,d,c,4d#,f,d,4c,2a#5,f5,d,4c,a#5,d.5,4c,a#5,d,4c,16g5,2f5,f5,f5,d#,d,d#,4d,c,d,4c,2a#5,f5,d,4c,a#5,d,4c,a#5,a#5,4c,2c.5,f5,f5,d#,d,d#,d.,16c,4d,c,a#5,2a#.5";
const char * burgertime = "Burgertime:d=4,o=6,b=285:8f,8f,8f#,8f#,8g#,8g#,8a,8a,a#,f,a#,f,8g#,8c#7,8c7,8a#,8g#,8g,8g#,8g,g#,c#7,g#,c#7,g#,f7,g#,f7,g#,d#7,g#,c#7,g#,d#7,g#,c#7";
const char * tron = "tron:d=4,o=5,b=200:8f6,8c6,8g,e,8p,8f6,8c6,8g,8f6,8c6,8g,e.";
const char * mario = "mario:d=4,o=5,b=100:16e6,16e6,32p,8e6,16c6,8e6,8g6,8p,8g,8p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,16p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,8p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16g#,16a,16c6,16p,16a,16c6,16d6,8p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16c7,16p,16c7,16c7,p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16g#,16a,16c6,16p,16a,16c6,16d6,8p,16d#6,8p,16d6,8p,16c6";
const char * smb_under = "smb_under:d=4,o=6,b=100:32c,32p,32c7,32p,32a5,32p,32a,32p,32a#5,32p,32a#,2p";
const char * finalcount = "FinalCou:d=4,o=5,b=125:16c#6,16b,c#6,f#,p.,16d6,16c#6,8d6,8c#6,b,p.,16d6,16c#6,d6,f#,p.,16b,16a,8b,8a,8g#,8b,a.,16c#6,16b,c#6,f#,p.,16d6,16c#6,8d6,8c#6,b,p.,16d6,16c#6,d6,f#,p.,16b,16a,8b,8a,8g#,8b,a.,16g#,16a,b.,16a,16b,8c#6,8b,8a,8g#,f#,d6,2c#.6,16c#6,16d6,16c#6,16b,1c#6,2p";
const char * adams = "AddamsFa:d=4,o=6,b=50:32p,32c#,16f#,32a#,16f#,32c#,16c,8g#,32f#,16f,32g#,16f,32c#,16a#5,8f#,32c#,16f#,32a#,16f#,32c#,16c,8g#,32f#,16f,32c#,16d#,32f,f#,32c#,32d#,32f,f#,32c#,32d#,32g,g#,32d#,32f,32g,16g#.,32d#,32f,32g,16g#.,32c#,32d#,32f,32f#";
const char * mspacman = "mspacman:d=4,o=5,b=100:32d,32e,8f,8a,8g,8a#,16a,16a#,16c6,16a,8g,8a#,16a,16a#,16c6,16a,16a#,16c6,16d6,16e6,8f6,8e6,8f6";
const char * galaga = "Galaga:d=4,o=5,b=125:8g4,32c,32p,8d,32f,32p,8e,32c,32p,8d,32a,32p,8g,32c,32p,8d,32f,32p,8e,32c,32p,8g,32b,32p,8c6,32a#,32p,8g#,32g,32p,8f,32d#,32p,8d,32a#4,32p,8a#,32c6,32p,8a#,32g,32p,16a,16f,16d,16g,16e,16d";
const char * xmen = "xmen:d=4,o=6,b=140:16f#5,16g5,16b5,16d,c#,8b5,8f#5,p,16f#5,16g5,16b5,16d,c#,8b5,8g5,p,16f#5,16g5,16b5,16d,c#,8b5,8d,2p,8c#,8b5,2p,16b5,16e,16f#,16g,f#,8e,8b5,p,16b5,16e,16f#,16g,f#,8e,8c,p,16f#5,16g5,16b5,16d,c#,8b5,8d,2p,8c#,8b5,2p";
const char * beethoven = "Beethoven - Fur Elise : d=4,o=5,b=140:8e6,8d#6,8e6,8d#6,8e6,8b,8d6,8c6,a,8p,8c,8e,8a,b,8p,8e,8g#,8b,c6,p,8e,8e6,8d#6,8e6,8d#6,8e6,8b,8d6,8c6,a,8p,8c,8e,8a,b,8p,8e,8c6,8b,2a";
const char * puffs = "Powerpuf:d=4,o=5,b=200:8c,p,8c,8p,8d#,8g,8a#,a.,8g,2p,8c6,p,8c6,8p,8d#6,8g6,8a#6,a#.6,8c7,p,8p,8c6,8c,p,8c,8p,8d#,8g,8a#,2c.6,p,1c,d#.,c.,g,1f#,p,8g,8c6";




//  real6     fake5     real4     fake3     real2     fake1     real0
// RRRRRRR   RRRRRRR   RRRRRRR   RRRRRRR   RRRRRRR   RRRRRRR   RRRRRRR 
//R       R F       F R       R F       F R       R F       F R       R
//R   S   R F   S   F R   S   R F   S   F R   S   R F   S   F R   S   R
//R       R F       F R       R F       F R       R F       F R       R
//R       R F       F R       R F       F R       R F       F R       R
//R       R F       F R       R F       F R       R F       F R       R
//R       R F       F R       R F       F R       R F       F R       R
//R       R F       F R       R F       F R       R F       F R       R
// RRRRRRR   RRRRRRR   RRRRRRR   RRRRRRR   RRRRRRR   RRRRRRR   RRRRRRR 
//R       R F       F R       R F       F R       R F       F R       R
//R   S   R F   S   F R   S   R F   S   F R   S   R F   S   F R   S   R
//R       R F       F R       R F       F R       R F       F R       R
//R       R F       F R       R F       F R       R F       F R       R
//R       R F       F R       R F       F R       R F       F R       R
//R       R F       F R       R F       F R       R F       F R       R
//R       R F       F R       R F       F R       R F       F R       R
// RRRRRRR   RRRRRRR   RRRRRRR   RRRRRRR   RRRRRRR   RRRRRRR   RRRRRRR 

//digit wiring order
//  4444444 
// 3       5
// 3       5
// 3       5
// 3       5
// 3       5
// 3       5
// 3       5
//  6666666 
// 2       0
// 2       0
// 2       0
// 2       0
// 2       0
// 2       0
// 2       0
//  1111111 
// byte numbers digit order 0b6543210
byte numbers[] = { 
   0b0111111,   //  [0] 0
   0b0100001,   //  [1] 1
   0b1110110,   //  [2] 2
   0b1110011,   //  [3] 3
   0b1101001,   //  [4] 4
   0b1011011,   //  [5] 5
   0b1011111,   //  [6] 6
   0b0110001,   //  [7] 7
   0b1111111,   //  [8] 8
   0b1111011,   //  [9] 9
   0b0000000,   //  [10] all off, space
   0b0100001,   //  [11] !
   0b0101000,   //  [12] "
   0b1101111,   //  [13] #
   0b1011011,   //  [14] $
   0b1100100,   //  [15] %
   0b1100001,   //  [16] &
   0b0001000,   //  [17] '
   0b0011010,   //  [18] (
   0b0110010,   //  [19] )
   0b0011000,   //  [20] *
   0b1001100,   //  [21] +
   0b0000100,   //  [22] ,
   0b1000000,   //  [23] -
   0b0000010,   //  [24] .
   0b1100100,   //  [25] /
   0b1111000,   //  [26] degrees symbol
   0b0010010,   //  [27] :
   0b0010011,   //  [28] ;
   0b1011000,   //  [29] <
   0b1000010,   //  [30] =
   0b1110000,   //  [31] >
   0b1110100,   //  [32] ?
   0b1110111,   //  [33] @
   0b1111101,   //  [34] A
   0b1001111,   //  [35] B
   0b0011110,   //  [36] C(elsius)
   0b1100111,   //  [37] D
   0b1011110,   //  [38] E
   0b1011100,   //  [39] F(ahrenheit)
   0b0011111,   //  [40] G
   0b1101101,   //  [41] H(umidity)
   0b0001100,   //  [42] I
   0b0100111,   //  [43] J
   0b1011101,   //  [44] K
   0b0001110,   //  [45] L
   0b0010101,   //  [46] M
   0b0111101,   //  [47] N
   0b0111111,   //  [48] O
   0b1111100,   //  [49] P
   0b1111010,   //  [50] Q
   0b0111100,   //  [51] R
   0b1011011,   //  [52] S
   0b1001110,   //  [53] T
   0b0101111,   //  [54] U
   0b0101111,   //  [55] V
   0b0101010,   //  [56] W
   0b1101101,   //  [57] X
   0b1101011,   //  [58] Y
   0b1110110,   //  [59] Z
   0b0011110,   //  [60] left bracket
   0b1001001,   //  [61] backslash
   0b0110011,   //  [62] right bracket
   0b0111000,   //  [63] carrot
   0b0000010,   //  [64] underscore
   0b0100000,   //  [65] aposhtophe
   0b1110111,   //  [66] a
   0b1001111,   //  [67] b
   0b1000110,   //  [68] c
   0b1100111,   //  [69] d
   0b1111110,   //  [70] e
   0b1011100,   //  [71] f
   0b1111011,   //  [72] g
   0b1001101,   //  [73] h
   0b0000100,   //  [74] i
   0b0000011,   //  [75] j
   0b1011101,   //  [76] k
   0b0001100,   //  [77] l
   0b0000101,   //  [78] m
   0b1000101,   //  [79] n
   0b1000111,   //  [80] o
   0b1111100,   //  [81] p
   0b1111001,   //  [82] q
   0b1000100,   //  [83] r
   0b1011011,   //  [84] s
   0b1001110,   //  [85] t
   0b0000111,   //  [86] u
   0b0000111,   //  [87] v
   0b0000101,   //  [88] w
   0b1101101,   //  [89] x
   0b1101011,   //  [90] y
   0b1110110,   //  [91] z
   0b1100001,   //  [92] left squigly bracket
   0b0001100,   //  [93] pipe
   0b1001100,   //  [94] right squigly bracket
   0b0010000,   //  [95] tilde
   0b1010010    //  [96] 3-lines (special pad)
   };

TaskHandle_t Task1;

void setup() {
  Serial.begin(115200);

  loadWebPageHandlers();  //load about 900 webpage handlers from the bottom of this sketch

  // Initialize SPIFFS 
  Serial.println(F("Inizializing FS..."));
  if (SPIFFS.begin()){
      Serial.println(F("SPIFFS mounted correctly."));
      Serial.println("Total Bytes");
      Serial.println(SPIFFS.totalBytes());
      Serial.println("Used Bytes");
      Serial.println(SPIFFS.usedBytes());
  }else{
      Serial.println(F("!An error occurred during SPIFFS mounting"));
  }

  //load preferences from flash
  loadSetupSettings();  //load all previously saved setting changes from the web interface
  
  //init DS3231 RTC
  if (! rtc.begin()) {
    Serial.println("Couldn't find DS3231 RTC");
    Serial.flush();
  //  abort();
  }

  //init audio gate inpute detection
  pinMode(AUDIO_GATE_PIN, INPUT_PULLUP);
  pinMode(MIC_IN_PIN, INPUT);  //setup microphone

  // init temp & humidity sensor
  Serial.println(F("DHTxx test!"));
  dht.begin();

  //setup LEDs
  FastLED.addLeds<LED_TYPE,LED_PIN,COLOR_ORDER>(LEDs,NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(255);
  fill_solid(LEDs, NUM_LEDS, CRGB::Black);
  FastLED.show();
  
  fakeClock(2);  // blink 12:00 like old clocks once did
  
  Config.autoReconnect = true; // Enable auto-reconnect. 
  Config.portalTimeout = 20000; // Sets timeout value for the captive portal 
  Config.retainPortal = true; // Retains the portal function after timed-out 
  Config.autoRise = true; // False for disabling the captive portal
  Portal.config(Config);      

  Serial.println();
  WiFi.hostname("shelfclock"); //set hostname

  // setup AutoConnect to control WiFi
  if (Portal.begin()) {
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
   }  

  //use mdns for host name resolution
  if (!MDNS.begin(host)) { //http://shelfclock
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

  //init and set the time of the internal RTC from NTP server
  configTime(gmtOffset_sec, (daylightOffset_sec * DSTime), ntpServer);
  
  //was the internal RTC time set by the NTP server?, if not set it via the RTC DS3231 stored time, will be wrong if daylight savings time is active
  if(!getLocalTime(&timeinfo)){ 
    struct tm tm;
    DateTime now = rtc.now();
    tm.tm_year = now.year() - 1900;
    tm.tm_mon = now.month() - 1;
    tm.tm_mday = now.day();
    tm.tm_hour = now.hour();
    tm.tm_min = now.minute();
    tm.tm_sec = now.second();
    time_t t = mktime(&tm);
    printf("NTP server not found, setting localtime from DS3231: %s", asctime(&tm));
    struct timeval now1 = { .tv_sec = t };
    settimeofday(&now1, NULL);
  }
  
  //did the DS3231 lose power (battery dead/changed), if so, set from time recieved from the NTP above
  if (rtc.lostPower()) {
    Serial.println("DS3231's RTC lost power, setting the time via NTP!");
    if(!getLocalTime(&timeinfo)){Serial.println("Error, no NTP Server found!");}
    int tempyear = (timeinfo.tm_year +1900);
    int tempmonth = (timeinfo.tm_mon + 1);
    rtc.adjust(DateTime(tempyear, tempmonth, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
  }

  printLocalTime(); 

  //create something to know if now is not then
  previousTimeMin = timeinfo.tm_min;
  previousTimeHour = timeinfo.tm_hour;
  previousTimeDay = timeinfo.tm_mday;
  previousTimeWeek = ((timeinfo.tm_yday + 7 - (timeinfo.tm_wday ? (timeinfo.tm_wday - 1) : 6)) / 7);
  previousTimeMonth = timeinfo.tm_mon;
  
  // I assume this starts the OTA stuff
  httpUpdateServer.setup(&server);

  initGreenMatrix();   //setup lightshow functions
  raininit();          //setup lightshow functions
  
  allBlank();   //clear everything off the leds
  
  server.enableCrossOrigin(true);
  server.enableCORS(true);

  //Webpage Handlers for SPIFFS access to flash
  server.serveStatic("/", SPIFFS, "/index.html");  //send default webpage from root request
  server.serveStatic("/", SPIFFS, "/", "max-age=86400");

  //OTA firmware Upgrade Webpage Handlers
  Serial.println("OTA Available");
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
      HTTPUpload& upload = server.upload();
      Serial.println("Update..");
      Serial.println(upload.status);
      if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Update: %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        /* flashing firmware to ESP*/
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
      }
  });

  //init rtttl (functions that play the alarms)
  pinMode(BUZZER_PIN, OUTPUT);
  rtttl::begin(BUZZER_PIN, smb_under);  //play mario sound and set initial brightness level
  while( !rtttl::done() ){GetBrightnessLevel(); rtttl::play();}


  xTaskCreatePinnedToCore(Task1code, "Task1", 10000, NULL, 0, &Task1, 0);
}    //end of Setup()

void Task1code(void * parameter) {
  unsigned long lastTime = 0;
  unsigned long timerDelay = 300000;  //5 seconds  600000 10 min

  getRemoteWeather();

  while(true) {
    if ((millis() - lastTime) > timerDelay) {
      Serial.print("loop() running on core ");
      Serial.println(xPortGetCoreID());

      getRemoteWeather();

      lastTime = millis();
    }  
  }
}

void getRemoteWeather() {
  if (WiFi.status() == WL_CONNECTED && tempConfig.outdoor_enable && tempConfig.outdoor_apikey && tempConfig.outdoor_lat && tempConfig.outdoor_long) {
    HTTPClient http;
    String serverPath = String("http://api.openweathermap.org/data/2.5/weather?lat=") + tempConfig.outdoor_lat + String("&lon=") + tempConfig.outdoor_long + String("&APPID=") + tempConfig.outdoor_apikey + String("&units=imperial");
    Serial.println(serverPath);

    http.begin(serverPath.c_str());
    int httpResponseCode = http.GET();
  
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);

      DynamicJsonDocument payload(2048);
      deserializeJson(payload, http.getStream());

      JsonObject main = payload["main"].as<JsonObject>();
      outdoorTemp = main["temp"].as<String>().toFloat();

    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }

    // Free resources
    http.end();
  }
}

void loop(){
  server.handleClient(); 
  Portal.handleRequest(); 
  if (WiFi.status() == WL_IDLE_STATUS) {
   ESP.restart();
   delay(1000);
  }

  //Change Frequency so as to not use hard-coded delays
  unsigned long currentMillis = millis();  
  //run everything inside here every second
  if ((unsigned long)(currentMillis - prevTime) >= 1000) {
    prevTime = currentMillis;
    //struct tm timeinfo; 
    if(!getLocalTime(&timeinfo)){ 
      Serial.println("Failed to obtain time");
    }

    //setup time-passage trackers
    int secs = timeinfo.tm_sec;
    int currentTimeMin = timeinfo.tm_min;
    byte m1 = currentTimeMin / 10;
    byte m2 = currentTimeMin % 10;
    int currentTimeHour = timeinfo.tm_hour;
    int currentTimeDay = timeinfo.tm_mday;
    int currentTimeWeek = ((timeinfo.tm_yday + 7 - (timeinfo.tm_wday ? (timeinfo.tm_wday - 1) : 6)) / 7);
    int currentTimeMonth = timeinfo.tm_mon;
    
    checkSleepTimer();  //time to sleep?

    if ((currentTimeHour == 23 && currentTimeMin == 11 && timeinfo.tm_sec == 0 && clockDisplayType != 1) || (currentTimeHour == 11 && currentTimeMin == 11 && timeinfo.tm_sec == 0)) { scroll("MAkE A WISH"); }  //at 1111 make a wish
 
    if (abs(currentTimeMin - previousTimeMin) >= 1) { //run every minute
      previousTimeMin = currentTimeMin; 
      randomMinPassed = 1; 
      GetBrightnessLevel(); 
      if (scrollFrequency == 1 && (suspendType == 0 || isAsleep == 0) && scrollOverride == 1 && ((clockMode != 11) && (clockMode != 1) && (clockMode != 4))) {displayScrollMode();}
      if (scrollFrequency == 1 && randomSpectrumMode == 1 && clockMode == 9) {allBlank(); spectrumMode = random(11);}
      } //end of run every minute

    if ((m2 == 0 || m2 == 5) && (secs == 0)) { //run every 5 minutes
      if (scrollFrequency == 2 && (suspendType == 0 || isAsleep == 0) && scrollOverride == 1 && ((clockMode != 11) && (clockMode != 1) && (clockMode != 4))) {displayScrollMode();}
      if (scrollFrequency == 2 && randomSpectrumMode == 1 && clockMode == 9) {allBlank(); spectrumMode = random(11);}
      } //end of run every 5 minutes

    if ((m2 == 0) && (secs == 0)) { //run every 10 minutes
      if (scrollFrequency == 3 && (suspendType == 0 || isAsleep == 0) && scrollOverride == 1 && ((clockMode != 11) && (clockMode != 1) && (clockMode != 4))) {displayScrollMode();}
      if (scrollFrequency == 3 && randomSpectrumMode == 1 && clockMode == 9) {allBlank(); spectrumMode = random(11);}
      } //end of run every 10 minutes

    if (((m1 == 0 && m2 == 0) || (m1 == 1 && m2 == 5) || (m1 == 3 && m2 == 0) || (m1 == 4 && m2 == 5)) && (secs == 0)) { //run every 15 minutes
      if (scrollFrequency == 4 && (suspendType == 0 || isAsleep == 0) && scrollOverride == 1 && ((clockMode != 11) && (clockMode != 1) && (clockMode != 4))) {displayScrollMode();}
      if (scrollFrequency == 4 && randomSpectrumMode == 1 && clockMode == 9) {allBlank(); spectrumMode = random(11);}
      } //end of run every 15 minutes

    if (((m1 == 0 && m2 == 0) || (m1 == 3 && m2 == 0)) && (secs == 0)) { //run every 30 minutes
      if (scrollFrequency == 5 && (suspendType == 0 || isAsleep == 0) && scrollOverride == 1 && ((clockMode != 11) && (clockMode != 1) && (clockMode != 4))) {displayScrollMode();}
      if (scrollFrequency == 5 && randomSpectrumMode == 1 && clockMode == 9) {allBlank(); spectrumMode = random(11);}
      } //end of run every 30 minutes
      
    if (abs(currentTimeHour - previousTimeHour) >= 1) { //run every hour
      previousTimeHour = currentTimeHour; 
      randomHourPassed = 1;
      if (scrollFrequency == 6 && (suspendType == 0 || isAsleep == 0) && scrollOverride == 1 && ((clockMode != 11) && (clockMode != 1) && (clockMode != 4))) {displayScrollMode();}
      if (scrollFrequency == 6 && randomSpectrumMode == 1 && clockMode == 9) {allBlank(); spectrumMode = random(11);}
      } //end of run every hour
      
    if (abs(currentTimeDay - previousTimeDay) >= 1) { 
        previousTimeDay = currentTimeDay; 
        randomDayPassed = 1; 
        configTime(gmtOffset_sec, (daylightOffset_sec * DSTime), ntpServer);
        int tempyear = (timeinfo.tm_year +1900);
        int tempmonth = (timeinfo.tm_mon + 1);
        rtc.adjust(DateTime(tempyear, tempmonth, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
        daysUptime = daysUptime + 1;
     }
    if (abs(currentTimeWeek - previousTimeWeek) >= 1) { previousTimeWeek = currentTimeWeek; randomWeekPassed = 1;}
    
    if (abs(currentTimeMonth - previousTimeMonth) >= 1) { previousTimeMonth = currentTimeMonth; randomMonthPassed = 1;}

    if (realtimeMode == 0) {    //give autodim sensors some CPU time, update display
       GetBrightnessLevel();        
       FastLED.show();
    }
    //give the various clock modes CPU time every 1 seconds
    if ((suspendType == 0 || isAsleep == 0) && clockMode == 0) {
       displayTimeMode();
    }  else if (clockMode == 1) {
       displayCountdownMode();
    }  else if ((suspendType == 0 || isAsleep == 0) && clockMode == 2) {
       displayTemperatureMode();  
    }  else if ((suspendType == 0 || isAsleep == 0) && clockMode == 3) {
       displayScoreboardMode();  
    }  else if (clockMode == 4) {
       displayStopwatchMode();           
    }  else if ((suspendType == 0 || isAsleep == 0) && clockMode == 5) {
       displayLightshowMode();             
    }  else if ((suspendType == 0 || isAsleep == 0) && clockMode == 6) {
       //notused
    }  else if ((suspendType == 0 || isAsleep == 0) && clockMode == 7) {
       displayDateMode();         
    }  else if ((suspendType == 0 || isAsleep == 0) && clockMode == 8) {
       displayHumidityMode();    
    }  else if (clockMode == 9) {
      //spectrum mode
    }  else if (clockMode == 10) {
      //display off
    }  else if ((suspendType == 0 || isAsleep == 0) && clockMode == 11) {
       displayScrollMode();  
    } 
    ShelfDownLights(); 
    randomMinPassed = 0; 
    randomHourPassed = 0; 
    randomDayPassed = 0; 
    randomWeekPassed = 0; 
    randomMonthPassed = 0;
    
    if (brightness != 10) {  //if not set to auto-dim just use user set brightness
      FastLED.setBrightness(brightness);
    } else if (brightness == 10) {  //auto-dim use the value from above
      FastLED.setBrightness(lightSensorValue);     
    } 
  }

  displayRealtimeMode();  //always run outside time loop for speed, but only really show when it's needed

}  // end of main loop




void displayTimeMode() {  //main clock function
  currentMode = 0;
	if(!getLocalTime(&timeinfo)){ 
	  Serial.println("Failed to obtain time");
	}
 // printLocalTime();  //display time in monitor window
	int hour = timeinfo.tm_hour;
	int mins = timeinfo.tm_min;
	int secs = timeinfo.tm_sec;

  if (clockDisplayType != 1 && hour > 12){hour = hour - 12; }
  if (clockDisplayType != 1 && hour < 1){hour = hour + 12; }
	byte h1 = hour / 10;
	byte h2 = hour % 10;
	byte m1 = mins / 10;
	byte m2 = mins % 10;  
	byte s1 = secs / 10;
	byte s2 = secs % 10;
  
  if (ClockColorSettings == 0) {	hourColor = CRGB(r1_val, g1_val, b1_val);  minColor = CRGB(r2_val, g2_val, b2_val); }
  if (ClockColorSettings == 1) { hourColor = CRGB(r1_val, g1_val, b1_val);  minColor = hourColor; }
  if ((ClockColorSettings == 2 && pastelColors == 0)  && ( (ColorChangeFrequency == 0 ) || (ColorChangeFrequency == 1 && randomMinPassed == 1) || (ColorChangeFrequency == 2 && randomHourPassed == 1) || (ColorChangeFrequency == 3 && randomDayPassed == 1) || (ColorChangeFrequency == 4 && randomWeekPassed == 1) || (ColorChangeFrequency == 5 && randomMonthPassed == 1) )) { hourColor = CHSV(random(0, 255), 255, 255);  minColor = CHSV(random(0, 255), 255, 255);}
  if ((ClockColorSettings == 2 && pastelColors == 1)  && ( (ColorChangeFrequency == 0 ) || (ColorChangeFrequency == 1 && randomMinPassed == 1) || (ColorChangeFrequency == 2 && randomHourPassed == 1) || (ColorChangeFrequency == 3 && randomDayPassed == 1) || (ColorChangeFrequency == 4 && randomWeekPassed == 1) || (ColorChangeFrequency == 5 && randomMonthPassed == 1) )) { hourColor = CRGB(random(0, 255), random(0, 255), random(0, 255));  minColor = CRGB(random(0, 255), random(0, 255), random(0, 255));}
  if ((ClockColorSettings == 3 && pastelColors == 0) && ( (ColorChangeFrequency == 0 ) || (ColorChangeFrequency == 1 && randomMinPassed == 1) || (ColorChangeFrequency == 2 && randomHourPassed == 1) || (ColorChangeFrequency == 3 && randomDayPassed == 1) || (ColorChangeFrequency == 4 && randomWeekPassed == 1) || (ColorChangeFrequency == 5 && randomMonthPassed == 1) )) { hourColor = CHSV(random(0, 255), 255, 255);  minColor = hourColor;}
  if ((ClockColorSettings == 3 && pastelColors == 1) && ( (ColorChangeFrequency == 0 ) || (ColorChangeFrequency == 1 && randomMinPassed == 1) || (ColorChangeFrequency == 2 && randomHourPassed == 1) || (ColorChangeFrequency == 3 && randomDayPassed == 1) || (ColorChangeFrequency == 4 && randomWeekPassed == 1) || (ColorChangeFrequency == 5 && randomMonthPassed == 1) )) { hourColor = CRGB(random(0, 255), random(0, 255), random(0, 255));  minColor = hourColor;}
  if ((clockDisplayType == 3)) {          //Blinking Center Light
  	if (h1 > 0) {
      tinyhourColor = hourColor;
      if (ClockColorSettings == 4 && pastelColors == 0){ tinyhourColor = CHSV(random(0, 255), 255, 255); }
      if (ClockColorSettings == 4 && pastelColors == 1){ tinyhourColor = CRGB(random(0, 255), random(0, 255), random(0, 255)); }
        for (int i=224; i<231; i++) { LEDs[i] = tinyhourColor;}
      if (ClockColorSettings == 4 && pastelColors == 0){ tinyhourColor = CHSV(random(0, 255), 255, 255); }
      if (ClockColorSettings == 4 && pastelColors == 1){ tinyhourColor = CRGB(random(0, 255), random(0, 255), random(0, 255)); }
        for (int i=231; i<238; i++) { LEDs[i] = tinyhourColor;}
  	} else {
  	    for (int i=224; i<238; i++) { LEDs[i] = CRGB::Black;}
  	  }
      displayNumber(h2,5,hourColor);
      displayNumber(m1,2,minColor);
      displayNumber(m2,0,minColor); 
      BlinkDots();    
  }
  
  if (clockDisplayType == 2) {  //12-hour Space-Padded
    if (h1 < 1) { displayNumber(10,6,hourColor); }
    else { displayNumber(h1,6,hourColor); }
      displayNumber(h2,4,hourColor);
      displayNumber(m1,2,minColor);
      displayNumber(m2,0,minColor); 
  }
  if (clockDisplayType == 0) {  //center set and hour is less than 1 and no 0 is set, default
  if (h1 > 0) {
    tinyhourColor = hourColor;
    if (ClockColorSettings == 4 && pastelColors == 0){ tinyhourColor = CHSV(random(0, 255), 255, 255); }
    if (ClockColorSettings == 4 && pastelColors == 1){ tinyhourColor = CRGB(random(0, 255), random(0, 255), random(0, 255)); }
      for (int i=224; i<231; i++) { LEDs[i] = tinyhourColor;}
    if (ClockColorSettings == 4 && pastelColors == 0){ tinyhourColor = CHSV(random(0, 255), 255, 255); }
    if (ClockColorSettings == 4 && pastelColors == 1){ tinyhourColor = CRGB(random(0, 255), random(0, 255), random(0, 255)); }
      for (int i=231; i<238; i++) { LEDs[i] = tinyhourColor;}
  } else {for (int i=224; i<238; i++) { LEDs[i] = CRGB::Black;}
   }
    	displayNumber(h2,5,hourColor);
    	displayNumber(m1,3,minColor);
    	displayNumber(m2,1,minColor); 
  }
  if (clockDisplayType == 1) {     //24-hour Military Time
    if (h1 < 1) { displayNumber(0,6,hourColor);}
    else  { displayNumber(h1,6,hourColor);}
     
      displayNumber(h2,4,hourColor);
      displayNumber(m1,2,minColor);
      displayNumber(m2,0,minColor); 
  }

  if (clockDisplayType == 4) {     //New Year Countdown
    int hour = timeinfo.tm_hour;
    int mins = timeinfo.tm_min;
    int secs = timeinfo.tm_sec;
    int mday = timeinfo.tm_mday;
    int mont = timeinfo.tm_mon + 1;
    int year = (timeinfo.tm_year +1900);
    int daysInMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int daysLeft = 0;
    if (((year % 4 == 0) && (year % 100 != 0)) || ((year % 100 != 0) && (year % 400 == 0))){daysLeft = daysLeft + 1;}  //leapyear?
    if (mont !=12) {for (int i=(mont+1); i<13; i++) { daysLeft = daysLeft + daysInMonth[i-1];}}
    daysLeft = daysLeft + (daysInMonth[mont-1]-mday);
    int hoursLeft = (daysLeft * 24) + (23 - hour);
    bool DST;  //start of DST adjustment algorithm
    int y = year-2000; // uses two digit year 
    int x = (y + y/4 + 2) % 7; // remainder will identify which day of month
    if(mont == 3 && mday == (14 - x) && hour >= 2){DST = 1;} //DST begins on 2nd Sunday of March @ 2:00 AM
    if((mont == 3 && mday > (14 - x)) || mont > 3){DST = 1;}
    if(mont == 11 && mday == (7 - x) && hour >= 2){DST = 0; }  //DST ends on 1st Sunday of Nov @ 2:00 AM
    if((mont == 11 && mday > (7 - x)) || mont > 11 || mont < 3){DST = 0;}
    if(DST == 1) {hoursLeft = hoursLeft + 1; }  //adjust for DST
    int minutesLeft = (hoursLeft * 60) + (59 - mins);
    int secondsLeft = (minutesLeft * 60) + (60 - secs);
    int inputNums = hoursLeft;
    if ((minutesLeft <= 9999) && (secondsLeft > 9999)) {inputNums = minutesLeft; hourColor = minColor;}
    if ((secondsLeft <= 9999) && (secondsLeft > 10)) {inputNums = secondsLeft; hourColor = colonColor;}
    if (secondsLeft <= 10) {inputNums = secondsLeft;FastLED.setBrightness(255);hourColor = CRGB::Red;}
    byte ledNum1 = inputNums / 1000;
    byte ledNum2 = (inputNums - (ledNum1 * 1000)) / 100;
    byte ledNum3 = ((inputNums - (ledNum1 * 1000)) - (ledNum2 * 100)) / 10;
    byte ledNum4 = inputNums % 10;
    
    if (inputNums >= 1000) {
      if (clearOldLeds != 1000){clearOldLeds = 1000; allBlank();}
      displayNumber(ledNum1,6,hourColor);
      displayNumber(ledNum2,4,hourColor);
      displayNumber(ledNum3,2,hourColor);
      displayNumber(ledNum4,0,hourColor); 
    }
    if ((inputNums >= 100) && (inputNums < 1000)) {
      if (clearOldLeds != 100){clearOldLeds = 100; allBlank();}
      displayNumber(ledNum2,5,hourColor);
      displayNumber(ledNum3,3,hourColor);
      displayNumber(ledNum4,1,hourColor); 
    }
    if ((inputNums > 0) && (inputNums < 100)) {
      if (clearOldLeds != 10){clearOldLeds = 10; allBlank();}
      displayNumber(ledNum3,4,hourColor);
      displayNumber(ledNum4,2,hourColor); 
    }
    if (mday == 1 && mont == 1 && hour == 0 && mins == 0 && secs <= 3) { happyNewYear();}
  //  if (mday == 7 && mont == 6 && hour == 16 && mins == 10 && secs <= 3) { happyNewYear();}  //for testing
    }
} // end of update clock


void displayDateMode() {  //main date function
  currentMode = 0;
  if(!getLocalTime(&timeinfo)){ 
    Serial.println("Failed to obtain time");
  }
  int mday = timeinfo.tm_mday;
  int mont = timeinfo.tm_mon + 1;
  int year = (timeinfo.tm_year +1900)-2000;
  byte d1 = mday / 10;
  byte d2 = mday % 10;
  byte m1 = mont / 10;
  byte m2 = mont % 10;  
  byte y1 = year / 10;
  byte y2 = year % 10;
  if (DateColorSettings == 0) { monthColor = CRGB(r5_val, g5_val, b5_val);  dayColor = CRGB(r4_val, g4_val, b4_val); separatorColor = CRGB(r6_val, g6_val, b6_val);}
  if (DateColorSettings == 1) { monthColor = CRGB(r5_val, g5_val, b5_val);  dayColor = monthColor; separatorColor = CRGB(r6_val, g6_val, b6_val);}
  if ((DateColorSettings == 2 && pastelColors == 0)  && ( (ColorChangeFrequency == 0 ) || (ColorChangeFrequency == 1 && randomMinPassed == 1) || (ColorChangeFrequency == 2 && randomHourPassed == 1) || (ColorChangeFrequency == 3 && randomDayPassed == 1) || (ColorChangeFrequency == 4 && randomWeekPassed == 1) || (ColorChangeFrequency == 5 && randomMonthPassed == 1) )) { monthColor = CHSV(random(0, 255), 255, 255);  dayColor = CHSV(random(0, 255), 255, 255);separatorColor = CHSV(random(0, 255), 255, 255);}
  if ((DateColorSettings == 2 && pastelColors == 1)  && ( (ColorChangeFrequency == 0 ) || (ColorChangeFrequency == 1 && randomMinPassed == 1) || (ColorChangeFrequency == 2 && randomHourPassed == 1) || (ColorChangeFrequency == 3 && randomDayPassed == 1) || (ColorChangeFrequency == 4 && randomWeekPassed == 1) || (ColorChangeFrequency == 5 && randomMonthPassed == 1) )) { monthColor = CRGB(random(0, 255), random(0, 255), random(0, 255));  dayColor = CRGB(random(0, 255), random(0, 255), random(0, 255));separatorColor = CRGB(random(0, 255), random(0, 255), random(0, 255));}
  if ((DateColorSettings == 3 && pastelColors == 0) && ( (ColorChangeFrequency == 0 ) || (ColorChangeFrequency == 1 && randomMinPassed == 1) || (ColorChangeFrequency == 2 && randomHourPassed == 1) || (ColorChangeFrequency == 3 && randomDayPassed == 1) || (ColorChangeFrequency == 4 && randomWeekPassed == 1) || (ColorChangeFrequency == 5 && randomMonthPassed == 1) )) { monthColor = CHSV(random(0, 255), 255, 255);  dayColor = monthColor; separatorColor = monthColor;}
  if ((DateColorSettings == 3 && pastelColors == 1) && ( (ColorChangeFrequency == 0 ) || (ColorChangeFrequency == 1 && randomMinPassed == 1) || (ColorChangeFrequency == 2 && randomHourPassed == 1) || (ColorChangeFrequency == 3 && randomDayPassed == 1) || (ColorChangeFrequency == 4 && randomWeekPassed == 1) || (ColorChangeFrequency == 5 && randomMonthPassed == 1) )) { monthColor = CRGB(random(0, 255), random(0, 255), random(0, 255));  dayColor = monthColor; separatorColor = monthColor;}

 if ((dateDisplayType == 5)) {    //With "." Separator (MM.DD)
  if (m1 > 0) {
    tinymonthColor = monthColor;
    if (DateColorSettings == 4 && pastelColors == 0){ tinymonthColor = CHSV(random(0, 255), 255, 255); }
    if (DateColorSettings == 4 && pastelColors == 1){ tinymonthColor = CRGB(random(0, 255), random(0, 255), random(0, 255)); }
      for (int i=224; i<231; i++) { LEDs[i] = tinymonthColor;}
    if (DateColorSettings == 4 && pastelColors == 0){ tinymonthColor = CHSV(random(0, 255), 255, 255); }
    if (DateColorSettings == 4 && pastelColors == 1){ tinymonthColor = CRGB(random(0, 255), random(0, 255), random(0, 255)); }
      for (int i=231; i<238; i++) { LEDs[i] = tinymonthColor;}
  } else {for (int i=224; i<238; i++) { LEDs[i] = CRGB::Black;}
   }
    displayNumber(m2,5,monthColor);
    displayNumber(d1,2,dayColor);
    displayNumber(d2,0,dayColor); 
    if ((DateColorSettings == 4 ) && pastelColors == 0){ separatorColor = CHSV(random(0, 255), 255, 255); }
    if ((DateColorSettings == 4 ) && pastelColors == 1){ separatorColor = CRGB(random(0, 255), random(0, 255), random(0, 255)); }
      for (int i=140; i<147; i++) { LEDs[i] = separatorColor;}  //separator
  }
  
  if (dateDisplayType == 1) {    //Space-Padded (MMDD)
    if (m1 < 1) { displayNumber(10,6,monthColor); }
    else { displayNumber(m1,6,monthColor); }
      displayNumber(m2,4,monthColor);
      displayNumber(d1,2,dayColor);
      displayNumber(d2,0,dayColor); 
  }
  
  if (dateDisplayType == 2) {    //Center Dates (1MDD)
  if (m1 > 0) {
    tinymonthColor = monthColor;
    if (DateColorSettings == 4 && pastelColors == 0){ tinymonthColor = CHSV(random(0, 255), 255, 255); }
    if (DateColorSettings == 4 && pastelColors == 1){ tinymonthColor = CRGB(random(0, 255), random(0, 255), random(0, 255)); }
      for (int i=224; i<231; i++) { LEDs[i] = tinymonthColor;}
    if (DateColorSettings == 4 && pastelColors == 0){ tinymonthColor = CHSV(random(0, 255), 255, 255); }
    if (DateColorSettings == 4 && pastelColors == 1){ tinymonthColor = CRGB(random(0, 255), random(0, 255), random(0, 255)); }
      for (int i=231; i<238; i++) { LEDs[i] = tinymonthColor;}
  } else {for (int i=224; i<238; i++) { LEDs[i] = CRGB::Black;}
   }
      displayNumber(m2,5,monthColor);
      displayNumber(d1,3,dayColor);
      displayNumber(d2,1,dayColor); 
  }
  
  if (dateDisplayType == 0) {    //Zero-Padded (MMDD)
    if (m1 < 1) { displayNumber(0,6,monthColor);}
    else  { displayNumber(m1,6,monthColor);}
     
      displayNumber(m2,4,monthColor);
      displayNumber(d1,2,dayColor);
      displayNumber(d2,0,dayColor); 
  }
  
  if (dateDisplayType == 4) {    //Just Numeric Day (DD)
  if (d1 < 1) {
      displayNumber(d2,3,dayColor);
  } else {
      displayNumber(d1,4,dayColor);
      displayNumber(d2,2,dayColor); 
  }
  }
  
  if (dateDisplayType == 3) {    //Just Day of Week (Sun)
  if (timeinfo.tm_wday == 1)    {displayNumber(78,5,dayColor);displayNumber(80,3,dayColor); displayNumber(79,1,dayColor);}  //mon
  if (timeinfo.tm_wday == 2)  {displayNumber(85,6,dayColor);displayNumber(54,4,dayColor); displayNumber(38,2,dayColor); displayNumber(52,0,dayColor);} //tUES
  if (timeinfo.tm_wday == 3) {displayNumber(88,5,dayColor);displayNumber(38,3,dayColor); displayNumber(69,1,dayColor);}  //wEd
  if (timeinfo.tm_wday == 4)  {displayNumber(85,6,dayColor);displayNumber(73,4,dayColor); displayNumber(86,2,dayColor); displayNumber(83,0,dayColor);}  //thur
  if (timeinfo.tm_wday == 5)    {displayNumber(39,5,dayColor);displayNumber(83,3,dayColor); displayNumber(42,1,dayColor);}  //FrI
  if (timeinfo.tm_wday == 6)  {displayNumber(52,5,dayColor);displayNumber(34,3,dayColor); displayNumber(85,1,dayColor);}  //SAt
  if (timeinfo.tm_wday == 0)    {displayNumber(52,5,dayColor);displayNumber(86,3,dayColor); displayNumber(79,1,dayColor);} //Sun
  }
  
  if (dateDisplayType == 6) {    //Just Year (YYYY)
  displayNumber(2,6,monthColor);
  displayNumber(0,4,monthColor);
  displayNumber(y1,2,dayColor);
  displayNumber(y2,0,dayColor); 
  } 
} // end of update date

void displayTemperatureMode() {   //miain temp function
  currentMode = 0;
  float h = dht.readHumidity();        // read humidity
  float sensorTemp = dht.readTemperature();     // read temperature
  float f = dht.readTemperature(true);
  if (isnan(h) || isnan(sensorTemp) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  float correctedTemp = sensorTemp + temperatureCorrection;
  if (temperatureSymbol == 39) {  correctedTemp = ((sensorTemp * 1.8000) + 32) + temperatureCorrection; }
  byte t1 = 0;
  byte t2 = 0;
  int tempDecimal = correctedTemp * 10;
  if (correctedTemp >= 100) {
    int tempHundred = correctedTemp / 10;
    t1 = tempHundred / 10;
    t2 = tempHundred % 10;
    } else {
    t2 = int(correctedTemp) / 10;
    }
  byte t3 = int(correctedTemp) % 10;
  byte t4 = tempDecimal % 10;
  if (tempColorSettings == 0) {tempColor = CRGB(r7_val, g7_val, b7_val);  typeColor = CRGB(r8_val, g8_val, b8_val); degreeColor = CRGB(r9_val, g9_val, b9_val);}
  if (tempColorSettings == 1) {tempColor = CRGB(r7_val, g7_val, b7_val);  typeColor = tempColor; degreeColor = CRGB(r9_val, g9_val, b9_val);}
  if ((tempColorSettings == 2 && pastelColors == 0) && ( (ColorChangeFrequency == 0 ) || (ColorChangeFrequency == 1 && randomMinPassed == 1) || (ColorChangeFrequency == 2 && randomHourPassed == 1) || (ColorChangeFrequency == 3 && randomDayPassed == 1) || (ColorChangeFrequency == 4 && randomWeekPassed == 1) || (ColorChangeFrequency == 5 && randomMonthPassed == 1) )) { tempColor = CHSV(random(0, 255), 255, 255);  typeColor = CHSV(random(0, 255), 255, 255);degreeColor = CHSV(random(0, 255), 255, 255);}
  if ((tempColorSettings == 2 && pastelColors == 1) && ( (ColorChangeFrequency == 0 ) || (ColorChangeFrequency == 1 && randomMinPassed == 1) || (ColorChangeFrequency == 2 && randomHourPassed == 1) || (ColorChangeFrequency == 3 && randomDayPassed == 1) || (ColorChangeFrequency == 4 && randomWeekPassed == 1) || (ColorChangeFrequency == 5 && randomMonthPassed == 1) )) { tempColor = CRGB(random(0, 255), random(0, 255), random(0, 255));  typeColor = CRGB(random(0, 255), random(0, 255), random(0, 255));degreeColor = CRGB(random(0, 255), random(0, 255), random(0, 255));}
  if ((tempColorSettings == 3 && pastelColors == 0) && ( (ColorChangeFrequency == 0 ) || (ColorChangeFrequency == 1 && randomMinPassed == 1) || (ColorChangeFrequency == 2 && randomHourPassed == 1) || (ColorChangeFrequency == 3 && randomDayPassed == 1) || (ColorChangeFrequency == 4 && randomWeekPassed == 1) || (ColorChangeFrequency == 5 && randomMonthPassed == 1) )) { tempColor = CHSV(random(0, 255), 255, 255);  typeColor = tempColor; degreeColor = tempColor;}
  if ((tempColorSettings == 3 && pastelColors == 1) && ( (ColorChangeFrequency == 0 ) || (ColorChangeFrequency == 1 && randomMinPassed == 1) || (ColorChangeFrequency == 2 && randomHourPassed == 1) || (ColorChangeFrequency == 3 && randomDayPassed == 1) || (ColorChangeFrequency == 4 && randomWeekPassed == 1) || (ColorChangeFrequency == 5 && randomMonthPassed == 1) )) { tempColor = CRGB(random(0, 255), random(0, 255), random(0, 255));  typeColor = tempColor; degreeColor = tempColor;}
    
//0-Temperature with Degree and Type (79°F), 1-Temperature with just Type (79 F), 2-Temperature with just Degree (79°), 3-Temperature with Decimal (79.9), 4-Just Temperature (79)
  if ((tempDisplayType == 0) && (correctedTemp < 100)) {  //0-Temperature with Degree and Type (79°F) under 100 only
    displayNumber(t2,6,tempColor);
    displayNumber(t3,4,tempColor);
    displayNumber(26,2,degreeColor);
    displayNumber(temperatureSymbol,0,typeColor);
  }
  if ((tempDisplayType == 1) && (correctedTemp < 100)) {   // 1-Temperature with just Type (79 F) under 100
    displayNumber(t2,5,tempColor);
    displayNumber(t3,3,tempColor);
    displayNumber(temperatureSymbol,1,typeColor);
  }
  if (((tempDisplayType == 1) || (tempDisplayType == 0)) && (correctedTemp >= 100)) {   // 1-Temperature with just Type (79 F) over 100
    displayNumber(t1,6,tempColor);
    displayNumber(t2,4,tempColor);
    displayNumber(t3,2,tempColor);
    displayNumber(temperatureSymbol,0,typeColor);
  }
  if ((tempDisplayType == 2) && (correctedTemp < 100)) {  //2-Temperature with just Degree (79°) under 100
    displayNumber(t2,5,tempColor);
    displayNumber(t3,3,tempColor);
    displayNumber(26,1,degreeColor);
  }
  if ((tempDisplayType == 2) && (correctedTemp >= 100)) {  //2-Temperature with just Degree (79°) over 100
    displayNumber(t1,6,tempColor);
    displayNumber(t2,4,tempColor);
    displayNumber(t3,2,tempColor);
    displayNumber(26,0,degreeColor);
  }
  if ((tempDisplayType == 3) && (correctedTemp < 100)) {   //3-Temperature with Decimal (79.9) under 100
    displayNumber(t2,6,tempColor);
    displayNumber(t3,4,tempColor);
    displayNumber(t4,1,typeColor); 
    if (tempColorSettings == 4 && pastelColors == 0){ degreeColor = CHSV(random(0, 255), 255, 255); }
    if (tempColorSettings == 4 && pastelColors == 1){ degreeColor = CRGB(random(0, 255), random(0, 255), random(0, 255)); }
    for (int i=84; i<91; i++) { LEDs[i] = degreeColor;}  //period goes here
  }
  if ((tempDisplayType == 3) && (correctedTemp >= 100)) {   //3-Temperature with Decimal (79.9) over 100
    displayNumber(t2,5,tempColor);
    displayNumber(t3,3,tempColor);
    displayNumber(t4,0,typeColor);
    tinytempColor = tempColor;
    if (tempColorSettings == 4 && pastelColors == 0){ tinytempColor = CHSV(random(0, 255), 255, 255); }
    if (tempColorSettings == 4 && pastelColors == 1){ tinytempColor = CRGB(random(0, 255), random(0, 255), random(0, 255)); }
    for (int i=224; i<231; i++) { LEDs[i] = tinytempColor;}  //1xx split across 2 for color reasons
    if (tempColorSettings == 4 && pastelColors == 0){ tinytempColor = CHSV(random(0, 255), 255, 255); }
    if (tempColorSettings == 4 && pastelColors == 1){ tinytempColor = CRGB(random(0, 255), random(0, 255), random(0, 255)); }
    for (int i=231; i<238; i++) { LEDs[i] = tinytempColor;}  //1xx split across 2 for color reasons
    if (tempColorSettings == 4 && pastelColors == 0){ degreeColor = CHSV(random(0, 255), 255, 255); }
    if (tempColorSettings == 4 && pastelColors == 1){ degreeColor = CRGB(random(0, 255), random(0, 255), random(0, 255)); }
    for (int i=70; i<77; i++) { LEDs[i] = degreeColor;}  //period goes here
  }
  if ((tempDisplayType == 4) && (correctedTemp < 100)) {  //4-Just Temperature (79) under 100
    displayNumber(t2,4,tempColor);
    displayNumber(t3,2,typeColor);
  }
  if ((tempDisplayType == 4) && (correctedTemp >= 100)) {  //4-Just Temperature (79) over 100
    displayNumber(t1,5,degreeColor);
    displayNumber(t2,3,tempColor);
    displayNumber(t3,1,typeColor);
  } 
}//end of temp settings


void displayHumidityMode() {   //main humidity function
  currentMode = 0;
  float sensorHumi = dht.readHumidity();        // read humidity
  float t = dht.readTemperature();     // read temperature
  float f = dht.readTemperature(true);
  if (isnan(sensorHumi) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  byte t1 = 0;
  byte t2 = 0;
  int humiDecimal = sensorHumi * 10;
  if (sensorHumi >= 100) {
    int humiHundred = sensorHumi / 10;
    t1 = humiHundred / 10;
    t2 = humiHundred % 10;
    } else {
    t2 = int(sensorHumi) / 10;
    }
  byte t3 = int(sensorHumi) % 10;
  byte t4 = humiDecimal % 10;
  if (humiColorSettings == 0) {humiColor = CRGB(r10_val, g10_val, b10_val);  symbolColor = CRGB(r11_val, g11_val, b11_val); humiDecimalColor = CRGB(r12_val, g12_val, b12_val);}
  if (humiColorSettings == 1) {humiColor = CRGB(r10_val, g10_val, b10_val);  symbolColor = humiColor; humiDecimalColor = CRGB(r12_val, g12_val, b12_val);}
  if ((humiColorSettings == 2 && pastelColors == 0) && ( (ColorChangeFrequency == 0 ) || (ColorChangeFrequency == 1 && randomMinPassed == 1) || (ColorChangeFrequency == 2 && randomHourPassed == 1) || (ColorChangeFrequency == 3 && randomDayPassed == 1) || (ColorChangeFrequency == 4 && randomWeekPassed == 1) || (ColorChangeFrequency == 5 && randomMonthPassed == 1) )) { humiColor = CHSV(random(0, 255), 255, 255);  symbolColor = CHSV(random(0, 255), 255, 255);humiDecimalColor = CHSV(random(0, 255), 255, 255); }
  if ((humiColorSettings == 2 && pastelColors == 1) && ( (ColorChangeFrequency == 0 ) || (ColorChangeFrequency == 1 && randomMinPassed == 1) || (ColorChangeFrequency == 2 && randomHourPassed == 1) || (ColorChangeFrequency == 3 && randomDayPassed == 1) || (ColorChangeFrequency == 4 && randomWeekPassed == 1) || (ColorChangeFrequency == 5 && randomMonthPassed == 1) )) { humiColor = CRGB(random(0, 255), random(0, 255), random(0, 255));  symbolColor = CRGB(random(0, 255), random(0, 255), random(0, 255));humiDecimalColor = CRGB(random(0, 255), random(0, 255), random(0, 255));}
  if ((humiColorSettings == 3 && pastelColors == 0) && ( (ColorChangeFrequency == 0 ) || (ColorChangeFrequency == 1 && randomMinPassed == 1) || (ColorChangeFrequency == 2 && randomHourPassed == 1) || (ColorChangeFrequency == 3 && randomDayPassed == 1) || (ColorChangeFrequency == 4 && randomWeekPassed == 1) || (ColorChangeFrequency == 5 && randomMonthPassed == 1) )) { humiColor = CHSV(random(0, 255), 255, 255);  symbolColor = humiColor; humiDecimalColor = humiColor;}
  if ((humiColorSettings == 3 && pastelColors == 1) && ( (ColorChangeFrequency == 0 ) || (ColorChangeFrequency == 1 && randomMinPassed == 1) || (ColorChangeFrequency == 2 && randomHourPassed == 1) || (ColorChangeFrequency == 3 && randomDayPassed == 1) || (ColorChangeFrequency == 4 && randomWeekPassed == 1) || (ColorChangeFrequency == 5 && randomMonthPassed == 1) )) { humiColor = CRGB(random(0, 255), random(0, 255), random(0, 255));  symbolColor = humiColor; humiDecimalColor = humiColor;}
  if ((humiDisplayType == 0) && (sensorHumi < 100)) {   //0-Humidity with Symbol (34 H) under 100
    displayNumber(t2,5,humiColor);
    displayNumber(t3,3,humiColor);
    displayNumber(41,1,symbolColor); //H(umitidy
  }
  if ((humiDisplayType == 0) && (sensorHumi >= 100)) {   //0-Humidity with Symbol (34 H) over 100
    displayNumber(t1,6,humiColor);
    displayNumber(t2,4,humiColor);
    displayNumber(t3,2,humiColor);
    displayNumber(41,0,symbolColor); //H(umitidy
  }
  if ((humiDisplayType == 1) && (sensorHumi < 100)) {   //1-Humidity with Decimal (34.9) under 100
    displayNumber(t2,6,humiColor);
    displayNumber(t3,4,humiColor);
    displayNumber(t4,1,symbolColor);
    if (humiColorSettings == 4 && pastelColors == 0){ humiDecimalColor = CHSV(random(0, 255), 255, 255); }
    if (humiColorSettings == 4 && pastelColors == 1){ humiDecimalColor = CRGB(random(0, 255), random(0, 255), random(0, 255)); }
    for (int i=84; i<91; i++) { LEDs[i] = humiDecimalColor;}  //period goes here
  }
  if ((humiDisplayType == 1) && (sensorHumi >= 100)) {   //1-Humidity with Decimal (34.9) over 100
    displayNumber(t2,5,humiColor);
    displayNumber(t3,3,humiColor);
    displayNumber(t4,0,symbolColor);
    tinyhumiColor = humiColor;
    if (humiColorSettings == 4 && pastelColors == 0){ tinyhumiColor = CHSV(random(0, 255), 255, 255); }
    if (humiColorSettings == 4 && pastelColors == 1){ tinyhumiColor = CRGB(random(0, 255), random(0, 255), random(0, 255)); }
    for (int i=224; i<231; i++) { LEDs[i] = tinyhumiColor;}  //1xx split across 2 for color reasons
    if (humiColorSettings == 4 && pastelColors == 0){ tinyhumiColor = CHSV(random(0, 255), 255, 255); }
    if (humiColorSettings == 4 && pastelColors == 1){ tinyhumiColor = CRGB(random(0, 255), random(0, 255), random(0, 255)); }
    for (int i=231; i<238; i++) { LEDs[i] = tinyhumiColor;} //1xx
    if (humiColorSettings == 4 && pastelColors == 0){ humiDecimalColor = CHSV(random(0, 255), 255, 255); }
    if (humiColorSettings == 4 && pastelColors == 1){ humiDecimalColor = CRGB(random(0, 255), random(0, 255), random(0, 255)); }
    for (int i=70; i<77; i++) { LEDs[i] = humiDecimalColor;}   //period goes here
  }
  if ((humiDisplayType == 2) && (sensorHumi < 100)) {  //2-Just Humidity (79) under 100
    displayNumber(t2,4,humiColor);
    displayNumber(t3,2,symbolColor);
  }
  if ((humiDisplayType == 2) && (sensorHumi >= 100)) {  //2-Just Humidity (79) over 100
    displayNumber(t1,5,humiDecimalColor);
    displayNumber(t2,3,humiColor);
    displayNumber(t3,1,symbolColor);
  }
}//end of update humidity


void displayScrollMode(){   //scrollmode for displaying clock things not just text
  currentMode = 0;
  if (realtimeMode == 0) {
    if (!getLocalTime(&timeinfo)){ Serial.println("Failed to obtain time");  }
    char strTime[10];
    char strDate[10];
    char strYear[10];
    char strTemp[25];
    char strOutdoorTemp[25];
    char strHumitidy[10];
    char strIPaddy[20];
    char processedText[255];
    char DOW[10]; 
    int hour = timeinfo.tm_hour;
    int mins = timeinfo.tm_min;
    int mday = timeinfo.tm_mday;
    int mont = timeinfo.tm_mon + 1;
    int year = timeinfo.tm_year +1900;
    float h = dht.readHumidity();        // read humidity
    float sensorTemp = dht.readTemperature();     // read temperature
    float f = dht.readTemperature(true);
    if (isnan(h) || isnan(sensorTemp) || isnan(f)) {  Serial.println(F("Failed to read from DHT sensor!"));  return;  }
    float correctedTemp = sensorTemp + temperatureCorrection;

    if (temperatureSymbol == 39) {  correctedTemp = ((sensorTemp * 1.8000) + 32) + temperatureCorrection; }
    if (timeinfo.tm_wday == 1)    {sprintf(DOW,"%s","Mon    ");}
    if (timeinfo.tm_wday == 2)    {sprintf(DOW,"%s","tUES    ");}
    if (timeinfo.tm_wday == 3)    {sprintf(DOW,"%s","WEd    ");}
    if (timeinfo.tm_wday == 4)    {sprintf(DOW,"%s","thur    ");}
    if (timeinfo.tm_wday == 5)    {sprintf(DOW,"%s","FrI    ");}
    if (timeinfo.tm_wday == 6)    {sprintf(DOW,"%s","SAt    ");}
    if (timeinfo.tm_wday == 0)    {sprintf(DOW,"%s","Sun    ");}
    sprintf(strTime, "%.2d%.2d    ", hour, mins);  //1111
    sprintf(strDate, "%.2d-%.2d    ", mont, mday);  //10-22
    sprintf(strYear, "%d    ", year);  //2021
    sprintf(strTemp, "In: %.1f^F    ", correctedTemp );  //98_6 ^F
    sprintf(strOutdoorTemp, "Out: %.1f^F    ", outdoorTemp);
    sprintf(strHumitidy, "%.0fH    ", h);  //48_6 H
    sprintf(strIPaddy, "%s", WiFi.localIP().toString().c_str());  //192_168_0_10
    strcpy(processedText, " ");
    if (scrollOptions1 == 1)    {strcat(processedText, strTime);}
    if (scrollOptions2 == 1)    {strcat(processedText, DOW);}
    if (scrollOptions3 == 1)    {strcat(processedText, strDate);}
    if (scrollOptions4 == 1)    {strcat(processedText, strYear);}
    if (scrollOptions5 == 1)    {strcat(processedText, strTemp);}
    if (scrollOptions5 == 1 && outdoorTemp != -500)    {strcat(processedText, strOutdoorTemp);}
    if (scrollOptions6 == 1)    {strcat(processedText, strHumitidy);}
    if (scrollOptions7 == 1)    {strcat(processedText, scrollText.c_str()); strcat(processedText, "    ");}
    if (scrollOptions8 == 1)    {strcat(processedText, strIPaddy);}
    if (scrollOptions1 == 0 && scrollOptions2 == 0 && scrollOptions3 == 0 && scrollOptions4 == 0 && scrollOptions5 == 0 && scrollOptions6 == 0 && scrollOptions7 == 0 && scrollOptions8 == 0)    {strcat(processedText, scrollText.c_str());}
    scroll(processedText);  
    }
}


void displayCountdownMode() {     //main countdown function
  if (countdownMilliSeconds == 0 && endCountDownMillis == 0) 
    return;
  unsigned long restMillis = endCountDownMillis - millis();
  unsigned long hours   = ((restMillis / 1000) / 60) / 60;
  unsigned long minutes = (restMillis / 1000) / 60;
  unsigned long seconds = restMillis / 1000;
  int remSeconds = seconds - (minutes * 60);
  int remMinutes = minutes - (hours * 60); 
  byte h1 = hours / 10;
  byte h2 = hours % 10;
  byte m1 = remMinutes / 10;
  byte m2 = remMinutes % 10;  
  byte s1 = remSeconds / 10;
  byte s2 = remSeconds % 10;
  CRGB color = CRGB(cd_r_val, cd_g_val, cd_b_val); 
  if (restMillis <= 10000 && colorchangeCD == 1) {  //red mode last 10 seconds
    color = CRGB::Red;
  }
  if (hours > 0) {   // hh:mm
    displayNumber(h1,6,color); 
    displayNumber(h2,4,color);
    displayNumber(m1,2,color);
    displayNumber(m2,0,color);  
  } else {   // mm:ss   
    displayNumber(m1,6,color);
    displayNumber(m2,4,color);
    displayNumber(s1,2,color);
    displayNumber(s2,0,color);  
  }
  if (hours <= 0 && remMinutes <= 0 && remSeconds <= 0) { //timer ended
    endCountdown();
    countdownMilliSeconds = 0;
    endCountDownMillis = 0;
    return;
  }   
}

void displayStopwatchMode() {     //main stopwatch timer function
  if (millis() >= endCountDownMillis) {  //timer ended
    CountUpMillis = 0;
    endCountDownMillis = 0;
    endCountdown();
    return;
  }
  unsigned long restMillis = millis() - CountUpMillis;
  unsigned long hours   = ((restMillis / 1000) / 60) / 60;
  unsigned long minutes = (restMillis / 1000) / 60;
  unsigned long seconds = restMillis / 1000;
  int remSeconds = seconds - (minutes * 60);
  int remMinutes = minutes - (hours * 60); 
  byte h1 = hours / 10;
  byte h2 = hours % 10;
  byte m1 = remMinutes / 10;
  byte m2 = remMinutes % 10;  
  byte s1 = remSeconds / 10;
  byte s2 = remSeconds % 10;
  CRGB color = CRGB(cd_r_val, cd_g_val, cd_b_val); 
  if (millis() >= (endCountDownMillis - 10000) && colorchangeCD == 1) {  //red mode at last 10 seconds
    color = CRGB::Red;
  } 
  if (hours > 0) { //show hours and minutes
    displayNumber(h1,6,color); 
    displayNumber(h2,4,color);
    displayNumber(m1,2,color);
    displayNumber(m2,0,color); 
  } else { //or show minutes and seconds  
    displayNumber(m1,6,color);
    displayNumber(m2,4,color);
    displayNumber(s1,2,color);
    displayNumber(s2,0,color);  
  }
}

void displayScoreboardMode() {  //main scoreboard function
  currentMode = 0;
  byte sl1 = scoreboardLeft / 10;
  byte sl2 = scoreboardLeft % 10;
  byte sr1 = scoreboardRight / 10;
  byte sr2 = scoreboardRight % 10;
  CRGB scoreboardColorLeft = CRGB(r13_val, g13_val, b13_val);
  CRGB scoreboardColorRight = CRGB(r14_val, g14_val, b14_val);
  displayNumber(sl1,6,scoreboardColorLeft);
  displayNumber(sl2,4,scoreboardColorLeft);
  displayNumber(sr1,2,scoreboardColorRight);
  displayNumber(sr2,0,scoreboardColorRight);
}//end of update scoreboard

void displayLightshowMode() {
  currentMode = 0;
  if (lightshowMode == 0) {Chase();}
  //if (lightshowMode == 1) {Twinkles();}
  //if (lightshowMode == 2) {Rainbow();}
  //if (lightshowMode == 3) {GreenMatrix();}
  //if (lightshowMode == 4) {Chase();}
  //if (lightshowMode == 5) {Fire2021();}
  //if (lightshowMode == 6) {Snake();}
  //if (lightshowMode == 7) {Cylon();}
}

void displayRealtimeMode(){   //main RealtimeModes function, always is running
  if ( (suspendType == 0 || isAsleep == 0) && clockMode == 9 && realtimeMode == 1) {SpectrumAnalyzer(); }
  if ( (suspendType == 0 || isAsleep == 0) && clockMode == 5 && lightshowMode == 1) {EVERY_N_MILLISECONDS(30) {Twinkles();FastLED.show();}}
  if ( (suspendType == 0 || isAsleep == 0) && clockMode == 5 && lightshowMode == 2) {Rainbow();FastLED.show();}
  if ( (suspendType == 0 || isAsleep == 0) && clockMode == 5 && lightshowMode == 3) {EVERY_N_MILLISECONDS(100) {GreenMatrix();}}
  if ( (suspendType == 0 || isAsleep == 0) && clockMode == 5 && lightshowMode == 4) {blueRain();FastLED.show();}
  if ( (suspendType == 0 || isAsleep == 0) && clockMode == 5 && lightshowMode == 5) {EVERY_N_MILLISECONDS(60) {Fire2021();FastLED.show();}}
  if ( (suspendType == 0 || isAsleep == 0) && clockMode == 5 && lightshowMode == 6) {EVERY_N_MILLISECONDS(getSlower) {Snake();FastLED.show();}}
  if ( (suspendType == 0 || isAsleep == 0) && clockMode == 5 && lightshowMode == 7) {EVERY_N_MILLISECONDS(150) {Cylon(); FastLED.show();}}
}//end of RealtimeModes

void SpectrumAnalyzer() {    //mostly from github.com/justcallmekoko/Arduino-FastLED-Music-Visualizer/blob/master/music_visualizer.ino
  currentMode = 0;
  const TProgmemRGBPalette16 FireColors = {0xFFFFCC, 0xFFFF99, 0xFFFF66, 0xFFFF33, 0xFFFF00, 0xFFCC00, 0xFF9900, 0xFF6600, 0xFF3300, 0xFF3300, 0xFF0000, 0xCC0000, 0x990000, 0x660000, 0x330000, 0x110000};
  const TProgmemRGBPalette16 FireColors2 = {0xFFFF99, 0xFFFF66, 0xFFFF33, 0xFFFF00, 0xFFCC00, 0xFF9900, 0xFF6600, 0xFF3300, 0xFF3300, 0xFF0000, 0xCC0000, 0x990000, 0x660000, 0x330000, 0x110000};
  const TProgmemRGBPalette16 FireColors3 = {0xFFFF66, 0xFFFF33, 0xFFFF00, 0xFFCC00, 0xFF9900, 0xFF6600, 0xFF3300, 0xFF3300, 0xFF0000, 0xCC0000, 0x990000, 0x660000, 0x330000, 0x110000};
  int audio_input = analogRead(MIC_IN_PIN); // ADD x2 HERE FOR MORE SENSITIVITY  
  if (audio_input > 0) {
    pre_react = ((long)SPECTRUM_PIXELS * (long)audio_input) / 1023L; // TRANSLATE AUDIO LEVEL TO NUMBER OF LEDs
    if (pre_react > react) // ONLY ADJUST LEVEL OF LED IF LEVEL HIGHER THAN CURRENT LEVEL
      react = pre_react;
   }
  for(int i = SPECTRUM_PIXELS - 1; i >= 0; i--) {
    int fake =  i * LEDS_PER_SEGMENT;
    int fireChoice = random(4);
    if (i < react)
    for (byte s=0; s<LEDS_PER_SEGMENT; s++ ){              // 7 LEDs per segment
      if (spectrumColorSettings == 0) { spectrumColor = CRGB(r15_val, g15_val, b15_val); }
      if (spectrumColorSettings == 1) { spectrumColor = CHSV((255/SPECTRUM_PIXELS)*i, 255, 255);}
      if (spectrumColorSettings == 2) { spectrumColor = colorWheel((i * 256 / 50 + colorWheelPosition) % 256);}
      if (spectrumColorSettings == 3 && fireChoice >= 2) { spectrumColor = ColorFromPalette( FireColors, (255/SPECTRUM_PIXELS)*i, 255, LINEARBLEND);}
      if (spectrumColorSettings == 3 && fireChoice == 0) { spectrumColor = ColorFromPalette( FireColors2, (255/SPECTRUM_PIXELS)*i, 255, LINEARBLEND);}
      if (spectrumColorSettings == 3 && fireChoice == 1) { spectrumColor = ColorFromPalette( FireColors3, (255/SPECTRUM_PIXELS)*i, 255, LINEARBLEND);}
      if (spectrumColorSettings == 4) { spectrumColor = ColorFromPalette( OceanColors_p, (255/SPECTRUM_PIXELS)*i, 255, LINEARBLEND);}
      if (spectrumColorSettings == 5) { spectrumColor = ColorFromPalette( ForestColors_p, (255/SPECTRUM_PIXELS)*i, 255, LINEARBLEND);}
      if (spectrumColorSettings == 6) { spectrumColor = colorWheel2(((255) + colorWheelPositionTwo) % 256);}
      if (spectrumMode == 0) {LEDs[FAKE_LEDs_C_BMUP[s+((fake))]] = spectrumColor;}
      if (spectrumMode == 1) {LEDs[FAKE_LEDs_C_CMOT[s+((fake))]] = spectrumColor;}
      if (spectrumMode == 2) {LEDs[FAKE_LEDs_C_BLTR[s+((fake))]] = spectrumColor;}
      if (spectrumMode == 3) {LEDs[FAKE_LEDs_C_TLBR[s+((fake))]] = spectrumColor;}
      if (spectrumMode == 4) {LEDs[FAKE_LEDs_C_VERT[s+((fake))]] = spectrumColor;}
      if (spectrumMode == 5) {LEDs[FAKE_LEDs_C_TMDN[s+((fake))]] = spectrumColor;}
      if (spectrumMode == 6) {LEDs[FAKE_LEDs_C_CSIN[s+((fake))]] = spectrumColor;}
      if (spectrumMode == 7) {LEDs[FAKE_LEDs_C_BRTL[s+((fake))]] = spectrumColor;}
      if (spectrumMode == 8) {LEDs[FAKE_LEDs_C_TRBL[s+((fake))]] = spectrumColor;}
      if (spectrumMode == 9) {LEDs[FAKE_LEDs_C_OUTS[s+((fake))]] = spectrumColor;}
      if (spectrumMode == 10) {LEDs[FAKE_LEDs_C_VERT2[s+((fake))]] = spectrumColor;}
      if (spectrumMode == 11) {LEDs[FAKE_LEDs_C_OUTS2[s+((fake))]] = spectrumColor;}
    }
    else
    for (byte s=0; s<LEDS_PER_SEGMENT; s++ ){              // 7 LEDs per segment
      if (spectrumBackgroundSettings == 0) { spectrumBackground = CRGB(r17_val, g17_val, b17_val); }
      if (spectrumBackgroundSettings == 1) { spectrumBackground = colorWheel((i * 256 / 50 + colorWheelPosition) % 256);}
      if (spectrumBackgroundSettings == 2) { spectrumBackground = colorWheel2(((255) + colorWheelPositionTwo) % 256);}
      if (spectrumMode == 0) {LEDs[FAKE_LEDs_C_BMUP[s+((fake))]] = spectrumBackground;}
      if (spectrumMode == 1) {LEDs[FAKE_LEDs_C_CMOT[s+((fake))]] = spectrumBackground;}
      if (spectrumMode == 2) {LEDs[FAKE_LEDs_C_BLTR[s+((fake))]] = spectrumBackground;}
      if (spectrumMode == 3) {LEDs[FAKE_LEDs_C_TLBR[s+((fake))]] = spectrumBackground;}
      if (spectrumMode == 4) {LEDs[FAKE_LEDs_C_VERT[s+((fake))]] = spectrumBackground;}
      if (spectrumMode == 5) {LEDs[FAKE_LEDs_C_TMDN[s+((fake))]] = spectrumBackground;}
      if (spectrumMode == 6) {LEDs[FAKE_LEDs_C_CSIN[s+((fake))]] = spectrumBackground;}
      if (spectrumMode == 7) {LEDs[FAKE_LEDs_C_BRTL[s+((fake))]] = spectrumBackground;}
      if (spectrumMode == 8) {LEDs[FAKE_LEDs_C_TRBL[s+((fake))]] = spectrumBackground;}
      if (spectrumMode == 9) {LEDs[FAKE_LEDs_C_OUTS[s+((fake))]] = spectrumBackground;}
      if (spectrumMode == 10) {LEDs[FAKE_LEDs_C_VERT2[s+((fake))]] = spectrumBackground;}
      if (spectrumMode == 11) {LEDs[FAKE_LEDs_C_OUTS2[s+((fake))]] = spectrumBackground;}
    }    
  }
  FastLED.show();                              // Increment the Hue to get the Rainbow
  colorWheelPosition = colorWheelPosition - colorWheelSpeed; // SPEED OF COLOR WHEEL
  if (colorWheelPosition < 0) // RESET COLOR WHEEL
    colorWheelPosition = 255;
  decay_check++;
  if (decay_check > decay) {
    decay_check = 0;
    if (react > 0)
      react--;
  }
}

void endCountdown() {  //countdown timer has reached 0, sound alarm and flash End for 30 seconds
  //  Serial.println("endcountdown function");
  breakOutSet = 0;
  FastLED.setBrightness(255);
  CRGB color = CRGB::Red; 
  allBlank();
  if (useAudibleAlarm == 1) { //alarm, ok
    if(!getLocalTime(&timeinfo)){ Serial.println("Failed to obtain time"); }
    int mday = timeinfo.tm_mday;
    int mont = timeinfo.tm_mon + 1;
    int whatisit = random(20);
    rtttl::begin(BUZZER_PIN, finalcount);  //play finalcountdown 10 out of 20 times randomly
    if (whatisit == 1) {rtttl::begin(BUZZER_PIN, puffs);}
    if (whatisit == 2) {rtttl::begin(BUZZER_PIN, adams);}
    if (whatisit == 3) {rtttl::begin(BUZZER_PIN, burgertime);}
    if (whatisit == 4) {rtttl::begin(BUZZER_PIN, rickroll2);}
    if (whatisit == 5) {rtttl::begin(BUZZER_PIN, melody);}
    if (whatisit == 6) {rtttl::begin(BUZZER_PIN, rickroll);}
    if (whatisit == 7) {rtttl::begin(BUZZER_PIN, mario);}
    if (whatisit == 8) {rtttl::begin(BUZZER_PIN, mspacman);}
    if (whatisit == 9) {rtttl::begin(BUZZER_PIN, xmen);}
    if (whatisit == 10) {rtttl::begin(BUZZER_PIN, galaga);}
    if (mday == 22 && mont == 10) {rtttl::begin(BUZZER_PIN, birthday);}
    if (mday == 7 && mont == 5) {rtttl::begin(BUZZER_PIN, mandy);}
    if (mday == 18 && mont == 5) {rtttl::begin(BUZZER_PIN, macgyver);}
    if (mday == 18 && mont == 3) {rtttl::begin(BUZZER_PIN, aniver);}
    if (mday == 4 && mont == 5) {rtttl::begin(BUZZER_PIN, starwars);}
    if (mday == 25 && mont == 12) {rtttl::begin(BUZZER_PIN, xmas);}
    if (mday == 8 && mont == 9) {rtttl::begin(BUZZER_PIN, startrek);}
    if (mday == 5 && mont == 5) {rtttl::begin(BUZZER_PIN, cinco);}
    if (mday == 31 && mont == 10) {rtttl::begin(BUZZER_PIN, halloween);}
    while( !rtttl::done() && !breakOutSet) {   //play song and flash lights
        rtttl::play();
        color = colorWheel((205 / 50 + colorWheelPosition) % 256);
        displayNumber(38,5,color);  //E 38
        displayNumber(79,3,color);  //n 79
        displayNumber(69,1,color);  //d 69
        FastLED.show();
        colorWheelPosition++;
        server.handleClient();   
      }
    } else {  //no alarm, ok, flash lights
    for (int i=0; i<1000 && !breakOutSet; i++) {
      color = colorWheel((205 / 50 + colorWheelPosition) % 256);
      displayNumber(38,5,color);  //E 38
      displayNumber(79,3,color);  //n 79
      displayNumber(69,1,color);  //d 69
      FastLED.show();
      colorWheelPosition++;
      server.handleClient();   
    }
      }
  clockMode = currentMode; 
  realtimeMode = currentReal;
  preferences.putInt("clockMode", clockMode);
  if (!breakOutSet) {scroll("tIMEr Ended      tIMEr Ended");}
  allBlank(); 
}

CRGB colorWheel(int pos) {   //color wheel for spectrum analyzer
  CRGB color (0,0,0);
  if(pos < 85) {
    color.g = 0;
    color.r = ((float)pos / 85.0f) * 255.0f;
    color.b = 255 - color.r;
  } else if(pos < 170) {
    color.g = ((float)(pos - 85) / 85.0f) * 255.0f;
    color.r = 255 - color.g;
    color.b = 0;
  } else if(pos < 256) {
    color.b = ((float)(pos - 170) / 85.0f) * 255.0f;
    color.g = 255 - color.b;
    color.r = 1;
  }
  return color;
}

CRGB colorWheel2(int pos) {   //color wheel for things not the spectrum analyzer
  CRGB color (0,0,0);
  if(pos < 85) {
    color.g = 0;
    color.r = ((float)pos / 85.0f) * 255.0f;
    color.b = 255 - color.r;
  } else if(pos < 170) {
    color.g = ((float)(pos - 85) / 85.0f) * 255.0f;
    color.r = 255 - color.g;
    color.b = 0;
  } else if(pos < 256) {
    color.b = ((float)(pos - 170) / 85.0f) * 255.0f;
    color.g = 255 - color.b;
    color.r = 1;
  }
  return color;
}

void scroll(String IncomingString) {    //main scrolling function
  breakOutSet = 0;
  scrollColor = CRGB(r16_val, g16_val, b16_val);
  if (IncomingString.length() > 256 ) { IncomingString = "ArE U A HAckEr"; }   //too big?
  char SentenceArray[IncomingString.length() + 1];
  IncomingString.toCharArray(SentenceArray, IncomingString.length()+1);
  uint16_t TranslatedSentence[(((IncomingString.length()*2)+6)+6)+1];
  TranslatedSentence[0] = 96;    //pad first 6 at front with a marker
  TranslatedSentence[1] = 96;    
  TranslatedSentence[2] = 96;    
  TranslatedSentence[3] = 96;    
  TranslatedSentence[4] = 96;    
  TranslatedSentence[5] = 96;    
  TranslatedSentence[(((IncomingString.length()*2)+6)+0)] = 96;  //pad last 6 at back with a marker
  TranslatedSentence[(((IncomingString.length()*2)+6)+1)] = 96;
  TranslatedSentence[(((IncomingString.length()*2)+6)+2)] = 96;
  TranslatedSentence[(((IncomingString.length()*2)+6)+3)] = 96;
  TranslatedSentence[(((IncomingString.length()*2)+6)+4)] = 96;
  TranslatedSentence[(((IncomingString.length()*2)+6)+5)] = 96;
  for (uint16_t realposition=0; realposition<(IncomingString.length()); realposition++){   //run string through translation
    char SentenceLetter = SentenceArray[realposition];  
    uint16_t LetterNumber = 10;  //for all unknown characters
    if( SentenceLetter == '0') { LetterNumber = 0; }
    if( SentenceLetter == '1') { LetterNumber = 1; }
    if( SentenceLetter == '2') { LetterNumber = 2; }
    if( SentenceLetter == '3') { LetterNumber = 3; }
    if( SentenceLetter == '4') { LetterNumber = 4; }
    if( SentenceLetter == '5') { LetterNumber = 5; }
    if( SentenceLetter == '6') { LetterNumber = 6; }
    if( SentenceLetter == '7') { LetterNumber = 7; }
    if( SentenceLetter == '8') { LetterNumber = 8; }
    if( SentenceLetter == '9') { LetterNumber = 9; }
    if( SentenceLetter == ' ') { LetterNumber = 10; }
    if( SentenceLetter == '`') { LetterNumber = 17; }
    if( SentenceLetter == 'A') { LetterNumber = 34; }
    if( SentenceLetter == 'B') { LetterNumber = 35; }
    if( SentenceLetter == 'C') { LetterNumber = 36; }
    if( SentenceLetter == 'D') { LetterNumber = 37; }
    if( SentenceLetter == 'E') { LetterNumber = 38; }
    if( SentenceLetter == 'F') { LetterNumber = 39; }
    if( SentenceLetter == 'G') { LetterNumber = 40; }
    if( SentenceLetter == 'H') { LetterNumber = 41; }
    if( SentenceLetter == 'I') { LetterNumber = 42; }
    if( SentenceLetter == 'J') { LetterNumber = 43; }
    if( SentenceLetter == 'K') { LetterNumber = 44; }
    if( SentenceLetter == 'L') { LetterNumber = 45; }
    if( SentenceLetter == 'M') { LetterNumber = 46; }
    if( SentenceLetter == 'N') { LetterNumber = 47; }
    if( SentenceLetter == 'O') { LetterNumber = 48; }
    if( SentenceLetter == 'P') { LetterNumber = 49; }
    if( SentenceLetter == 'Q') { LetterNumber = 50; }
    if( SentenceLetter == 'R') { LetterNumber = 51; }
    if( SentenceLetter == 'S') { LetterNumber = 52; }
    if( SentenceLetter == 'T') { LetterNumber = 53; }
    if( SentenceLetter == 'U') { LetterNumber = 54; }
    if( SentenceLetter == 'V') { LetterNumber = 55; }
    if( SentenceLetter == 'W') { LetterNumber = 56; }
    if( SentenceLetter == 'X') { LetterNumber = 57; }
    if( SentenceLetter == 'Y') { LetterNumber = 58; }
    if( SentenceLetter == 'Z') { LetterNumber = 59; }
    if( SentenceLetter == 'a') { LetterNumber = 66; }
    if( SentenceLetter == 'b') { LetterNumber = 67; }
    if( SentenceLetter == 'c') { LetterNumber = 68; }
    if( SentenceLetter == 'd') { LetterNumber = 69; }
    if( SentenceLetter == 'e') { LetterNumber = 70; }
    if( SentenceLetter == 'f') { LetterNumber = 71; }
    if( SentenceLetter == 'g') { LetterNumber = 72; }
    if( SentenceLetter == 'h') { LetterNumber = 73; }
    if( SentenceLetter == 'i') { LetterNumber = 74; }
    if( SentenceLetter == 'j') { LetterNumber = 75; }
    if( SentenceLetter == 'k') { LetterNumber = 76; }
    if( SentenceLetter == 'l') { LetterNumber = 77; }
    if( SentenceLetter == 'm') { LetterNumber = 78; }
    if( SentenceLetter == 'n') { LetterNumber = 79; }
    if( SentenceLetter == 'o') { LetterNumber = 80; }
    if( SentenceLetter == 'p') { LetterNumber = 81; }
    if( SentenceLetter == 'q') { LetterNumber = 82; }
    if( SentenceLetter == 'r') { LetterNumber = 83; }
    if( SentenceLetter == 's') { LetterNumber = 84; }
    if( SentenceLetter == 't') { LetterNumber = 85; }
    if( SentenceLetter == 'u') { LetterNumber = 86; }
    if( SentenceLetter == 'v') { LetterNumber = 87; }
    if( SentenceLetter == 'w') { LetterNumber = 88; }
    if( SentenceLetter == 'x') { LetterNumber = 89; }
    if( SentenceLetter == 'y') { LetterNumber = 90; }
    if( SentenceLetter == 'z') { LetterNumber = 91; }
    if( SentenceLetter == ',') { LetterNumber = 22; }
    if( SentenceLetter == '-') { LetterNumber = 23; }
    if( SentenceLetter == '.') { LetterNumber = 24; }
    if( SentenceLetter == ':') { LetterNumber = 27; }
    if( SentenceLetter == '^') { LetterNumber = 26; }
    if( SentenceLetter == '\'') { LetterNumber = 17; }
    TranslatedSentence[(realposition*2)+6] = LetterNumber;  //letter starting at position 7 
    TranslatedSentence[(realposition*2)+7] = 96; //add padding to next position because fake digit shares a leg with adjacent real ones and can't be on at the same time
  }  
  if (scrollColorSettings == 0){ scrollColor = CRGB(r16_val, g16_val, b16_val); }
  if (scrollColorSettings == 1 && pastelColors == 0){ scrollColor = CHSV(random(0, 255), 255, 255); }
  if (scrollColorSettings == 1 && pastelColors == 1){ scrollColor = CRGB(random(0, 255), random(0, 255), random(0, 255)); }
  for (uint16_t finalposition=0; finalposition<((IncomingString.length()*2)+6); finalposition++){  //count to end of padded array
    for (int i=0; i<259; i++) { LEDs[i] = CRGB::Black;  }    //clear 
    if( TranslatedSentence[finalposition] != 96) { displayNumber(TranslatedSentence[finalposition],6,scrollColor); }
    if( TranslatedSentence[finalposition+1] != 96) { displayNumber(TranslatedSentence[finalposition+1],5,scrollColor); }
    if( TranslatedSentence[finalposition+2] != 96) { displayNumber(TranslatedSentence[finalposition+2],4,scrollColor); }
    if( TranslatedSentence[finalposition+3] != 96) { displayNumber(TranslatedSentence[finalposition+3],3,scrollColor); }
    if( TranslatedSentence[finalposition+4] != 96) { displayNumber(TranslatedSentence[finalposition+4],2,scrollColor); }
    if( TranslatedSentence[finalposition+5] != 96) { displayNumber(TranslatedSentence[finalposition+5],1,scrollColor); }
    if( TranslatedSentence[finalposition+6] != 96) { displayNumber(TranslatedSentence[finalposition+6],0,scrollColor); }
    FastLED.show();
    if ( finalposition < ((IncomingString.length()*2)+6)) {
      for (int i=0; i<400 && !breakOutSet; i++) {
        server.handleClient();   
      }    //slow down on non-padded parts with web server polls
    } 
  }
} //end of scroll function




void printLocalTime() {  //what could this do
  DateTime now = rtc.now();
  char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  char monthsOfTheYear[12][12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
  if (!rtc.lostPower()) {
    Serial.print("DS3231 Time: ");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(", ");
    Serial.print(monthsOfTheYear[now.month()-1]);
    Serial.print(" ");
    Serial.print(now.day(), DEC);
    Serial.print(" ");
    Serial.print(now.year(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
  }
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "Local Time: %A, %B %d %Y %H:%M:%S");
}




void checkSleepTimer(){  //controls suspend mode
  if (suspendType != 0) {sleepTimerCurrent++;}  //sleep enabled? add one to timer
  //if (digitalRead(AUDIO_GATE_PIN)==HIGH) {sleepTimerCurrent = 0; isAsleep = 0;}   //sound sensor went off (while checking this function)? wake up
  int audio_input1 = analogRead(MIC_IN_PIN); 
  if (audio_input1 > 200) {sleepTimerCurrent = 0; isAsleep = 0;} //try it with the real sensor, the digital one was tripping false positives just as much
  int audio_input2 = analogRead(MIC_IN_PIN); 
  if (audio_input2 > 200) {sleepTimerCurrent = 0; isAsleep = 0;} //try it with the real sensor, the digital one was tripping false positives just as much
  int audio_input3 = analogRead(MIC_IN_PIN); 
  if (audio_input3 > 200) {sleepTimerCurrent = 0; isAsleep = 0;} //try it with the real sensor, the digital one was tripping false positives just as much
  int audio_input4 = analogRead(MIC_IN_PIN); 
  if (audio_input4 > 200) {sleepTimerCurrent = 0; isAsleep = 0;} //try it with the real sensor, the digital one was tripping false positives just as much
  int audio_input5 = analogRead(MIC_IN_PIN); 
  if (audio_input5 > 200) {sleepTimerCurrent = 0; isAsleep = 0;} //try it with the real sensor, the digital one was tripping false positives just as much
  averageAudioInput = (audio_input1 + audio_input2 + audio_input3 + audio_input4 + audio_input5) / 5;
  if (averageAudioInput > 50) {sleepTimerCurrent = 0; isAsleep = 0;} //try it with the real sensor, the digital one was tripping false positives just as much
  if ((suspendType != 0) && sleepTimerCurrent >= (suspendFrequency * 60)) {sleepTimerCurrent = 0; isAsleep = 1; allBlank(); }  //sleep enabled, been some amount of time, go to sleep
}




void GetBrightnessLevel() {   //samples the photoresister and set brightness
  photoresisterReadings[readIndex] = analogRead(PHOTORESISTER_PIN); //get an average light level from previouse set of samples
  readIndex = readIndex + 1; // advance to the next position in the array:
  if (readIndex >= PHOTO_SAMPLES) {   //reset count of readings even N times around
    readIndex = 0;
  }
  int sumBrightness = 0;
  for (int i=0; i < PHOTO_SAMPLES; i++)
    {
     sumBrightness += photoresisterReadings[i];  // add all the current readings together
    }
 //   Serial.println(analogRead(PHOTORESISTER_PIN));
   // lightSensorValue = 255 - (((sumBrightness / PHOTO_SAMPLES) * (215)) / 4095);  //linear conversion of 0-4095 to 255 to 40, after getting the average of the readings
  lightSensorValue = 275 - (((sumBrightness / PHOTO_SAMPLES) * (245)) / 4095);  //linear conversion of 0-4095 to 305 to 10 (a little brighter), after getting the average of the readings
  if (lightSensorValue > 255) {lightSensorValue = 255;} //constrain brightness
    if (brightness != 10) {  //if not set to auto-dim just use user set brightness
      FastLED.setBrightness(brightness);
    } else if (brightness == 10) {  //auto-dim use the value from above
      FastLED.setBrightness(lightSensorValue);     
    } 
}  // end of auto-brightness




void displayNumber(uint16_t number, byte segment, CRGB color) {   //main digit rendering (except when scrolling)
  // segments from left to right: 6, 5, 4, 3, 2, 1, 0
  uint16_t startindex = 0;
 //   Serial.println(startindex);
  switch (segment) {
    case 0:
      startindex = 0;
      break;
    case 1:
      startindex = (LEDS_PER_DIGIT * 1);
      break;
    case 2:
      startindex = (LEDS_PER_DIGIT * 2);
      break;
    case 3:
      startindex = (LEDS_PER_DIGIT * 3);
      break;    
    case 4:
      startindex = (LEDS_PER_DIGIT * 4);
      break;    
    case 5:
      startindex = (LEDS_PER_DIGIT * 5);
      break;    
    case 6:
      startindex = (LEDS_PER_DIGIT * 6);
      break;    
  }

  for (byte i=0; i<NUMBER_OF_DIGITS; i++){                // 7 segments
    if (fakeclockrunning == 0 && (((ClockColorSettings == 4 && clockMode == 0) || (DateColorSettings == 4 && clockMode == 7) || (tempColorSettings == 4 && clockMode == 2) || (humiColorSettings == 4 && clockMode == 8)) && pastelColors == 0))  { color = CHSV(random(0, 255), 255, 255);}
    if (fakeclockrunning == 0 && (((ClockColorSettings == 4 && clockMode == 0) || (DateColorSettings == 4 && clockMode == 7) || (tempColorSettings == 4 && clockMode == 2) || (humiColorSettings == 4 && clockMode == 8)) && pastelColors == 1)) { color = CRGB(random(0, 255), random(0, 255), random(0, 255));}
    for (byte j=0; j<LEDS_PER_SEGMENT; j++ ){              // 7 LEDs per segment
      yield();
      LEDs[FAKE_LEDs[i * LEDS_PER_SEGMENT + j + startindex]] = ((numbers[number] & 1 << i) == 1 << i) ? color : alternateColor;
    }
  }
} //end of displayNumber


void allBlank() {   //clears all non-shelf LEDs to black
  //  Serial.println("allblank function");
  for (int i=0; i<259; i++) {
    LEDs[i] = CRGB::Black;
  }
  FastLED.show();
  randomMinPassed = 1;
  randomHourPassed = 1;
  randomDayPassed = 1;
  randomWeekPassed = 1;
  randomMonthPassed = 1;
}  // end of all-blank


void fakeClock(int loopy) {  //flashes 12:00 like all old clocks did
  fakeclockrunning = 1;
  for (int i=0; i<loopy; i++) {
      for (int i=224; i<238; i++) { LEDs[i] = CRGB::Red;}
      displayNumber(2,5,CRGB::Red);
      for (int i=177; i<180; i++) { LEDs[i] = CRGB::Black; }
      for (int i=142; i<145; i++) { LEDs[i] = CRGB::Black; }
      displayNumber(0,2,CRGB::Red);
      displayNumber(0,0,CRGB::Red); 
      FastLED.show();
      delay(500);
      for (int i=224; i<238; i++) { LEDs[i] = CRGB::Black;}
      displayNumber(2,5,CRGB::Black);
      for (int i=177; i<180; i++) { LEDs[i] = CRGB::Red; }
      for (int i=142; i<145; i++) { LEDs[i] = CRGB::Red; }
      displayNumber(0,2,CRGB::Black);
      displayNumber(0,0,CRGB::Black); 
      FastLED.show();
      delay(500);
   }
  for (int i=175; i<182; i++) { LEDs[i] = CRGB::Black; }
  for (int i=140; i<147; i++) { LEDs[i] = CRGB::Black; }
  fakeclockrunning = 0;  // so the digit render knows not to apply the rainbow colors
}  //end of fakeClock




void ShelfDownLights() {  //turns on the drop lights on the underside of each shelf
 //   Serial.println("ShelfDownLights function");
 if ((suspendType != 2 || isAsleep == 0) && useSpotlights == 1) {  //not sleeping? suposed to be running?
  unsigned long currentMillis = millis();  
  if (currentMillis - prevTime2 >= 250) {  //run everything inside here every quarter second
    for (int i=259; i<273; i++) {
        if (spotlightsColorSettings == 0){ spotlightsColor = CRGB(r0_val, g0_val, b0_val);  LEDs[i] = spotlightsColor;}
        if ((spotlightsColorSettings == 1 && pastelColors == 0)  && ( (ColorChangeFrequency == 0 ) || (ColorChangeFrequency == 1 && randomMinPassed == 1) || (ColorChangeFrequency == 2 && randomHourPassed == 1) || (ColorChangeFrequency == 3 && randomDayPassed == 1) || (ColorChangeFrequency == 4 && randomWeekPassed == 1) || (ColorChangeFrequency == 5 && randomMonthPassed == 1) )) { spotlightsColor = CHSV(random(0, 255), 255, 255);  LEDs[i] = spotlightsColor;}
        if ((spotlightsColorSettings == 1 && pastelColors == 1)  && ( (ColorChangeFrequency == 0 ) || (ColorChangeFrequency == 1 && randomMinPassed == 1) || (ColorChangeFrequency == 2 && randomHourPassed == 1) || (ColorChangeFrequency == 3 && randomDayPassed == 1) || (ColorChangeFrequency == 4 && randomWeekPassed == 1) || (ColorChangeFrequency == 5 && randomMonthPassed == 1) )) { spotlightsColor = CRGB(random(0, 255), random(0, 255), random(0, 255));  LEDs[i] = spotlightsColor;}
        if (spotlightsColorSettings == 2 ){ LEDs[i] = colorWheel2(((i-259)  * 18 + colorWheelPositionTwo) % 256); }
        if (spotlightsColorSettings == 3 ){ LEDs[i] = colorWheel2(((255) + colorWheelPositionTwo) % 256); }
    }
    colorWheelPositionTwo = colorWheelPositionTwo - 1; // SPEED OF 2nd COLOR WHEEL
    if (colorWheelPositionTwo < 0) {colorWheelPositionTwo = 255;} // RESET 2nd COLOR WHEEL 
    if ((spotlightsColorSettings == 2 || spotlightsColorSettings == 3) && clockMode != 11){FastLED.show();}
    prevTime2 = currentMillis;
  }
 } else {  //or turn them all off
  for (int i=259; i<273; i++) {
    LEDs[i] = CRGB::Black;
    FastLED.show();
  }
 }
 
}//end of ShelfDownLights 




void BlinkDots() {  //displays the 2 dots in the middle of the time (colon)
  if (dotsOn) {
  if (ClockColorSettings == 0 || ClockColorSettings == 1) {colonColor = CRGB(r3_val, g3_val, b3_val);}
    if (((ClockColorSettings == 2) || (ClockColorSettings == 4)) && ( (ColorChangeFrequency == 0 ) || (ColorChangeFrequency == 1 && randomMinPassed == 1) || (ColorChangeFrequency == 2 && randomHourPassed == 1) || (ColorChangeFrequency == 3 && randomDayPassed == 1) || (ColorChangeFrequency == 4 && randomWeekPassed == 1) || (ColorChangeFrequency == 5 && randomMonthPassed == 1) )) { colonColor = CRGB(random(0, 255), random(0, 255), random(0, 255));}
    if (ClockColorSettings == 3 && ( (ColorChangeFrequency == 0 ) || (ColorChangeFrequency == 1 && randomMinPassed == 1) || (ColorChangeFrequency == 2 && randomHourPassed == 1) || (ColorChangeFrequency == 3 && randomDayPassed == 1) || (ColorChangeFrequency == 4 && randomWeekPassed == 1) || (ColorChangeFrequency == 5 && randomMonthPassed == 1) )) { colonColor = hourColor;}
    if (colonType == 0) {
      if (ClockColorSettings == 4 && pastelColors == 0){ colonColor = CHSV(random(0, 255), 255, 255); }
      if (ClockColorSettings == 4 && pastelColors == 1){ colonColor = CRGB(random(0, 255), random(0, 255), random(0, 255)); }
      for (int i=177; i<180; i++) { LEDs[i] = colonColor;}
      if (ClockColorSettings == 4 && pastelColors == 0){ colonColor = CHSV(random(0, 255), 255, 255); }
      if (ClockColorSettings == 4 && pastelColors == 1){ colonColor = CRGB(random(0, 255), random(0, 255), random(0, 255)); }
      for (int i=142; i<145; i++) { LEDs[i] = colonColor;}
    }
    if (colonType == 1) {
      if (ClockColorSettings == 4 && pastelColors == 0){ colonColor = CHSV(random(0, 255), 255, 255); }
      if (ClockColorSettings == 4 && pastelColors == 1){ colonColor = CRGB(random(0, 255), random(0, 255), random(0, 255)); }
      for (int i=175; i<182; i++) { LEDs[i] = colonColor;}
      if (ClockColorSettings == 4 && pastelColors == 0){ colonColor = CHSV(random(0, 255), 255, 255); }
      if (ClockColorSettings == 4 && pastelColors == 1){ colonColor = CRGB(random(0, 255), random(0, 255), random(0, 255)); }
      for (int i=140; i<147; i++) { LEDs[i] = colonColor;}
    }
    if (colonType == 2) {
      if (ClockColorSettings == 4 && pastelColors == 0){ colonColor = CHSV(random(0, 255), 255, 255); }
      if (ClockColorSettings == 4 && pastelColors == 1){ colonColor = CRGB(random(0, 255), random(0, 255), random(0, 255)); }
      for (int i=140; i<147; i++) { LEDs[i] = colonColor;}
    }
  } else {
    for (int i=175; i<182; i++) { LEDs[i] = CRGB::Black; }
    for (int i=140; i<147; i++) { LEDs[i] = CRGB::Black; }
  }
  dotsOn = !dotsOn;  
}//end of shelf and gaps




void happyNewYear() {  
  CRGB color = CRGB::Red; 
  allBlank();
  breakOutSet = 0;
  int year = (timeinfo.tm_year +1900)-2000;
  byte y1 = year / 10;
  byte y2 = year % 10;
  rtttl::begin(BUZZER_PIN, auldlang);
  while( !rtttl::done() && !breakOutSet )
  {
    rtttl::play();
    color = colorWheel((205 / 50 + colorWheelPosition) % 256);
    displayNumber(2,6,color);  //2
    displayNumber(0,4,color);  //0
    displayNumber(y1,2,color);  //Y
    displayNumber(y2,0,color);  //Y
    FastLED.show();
    colorWheelPosition++;
    server.handleClient();   
       }
    if (!breakOutSet) {scroll("HAPPy nEW yEAr");}
  allBlank();
}




void Chase() {   //lightshow chase mode
  int chaseMode = random(0, 7);
  if (pastelColors == 0){ lightchaseColorOne = CHSV(random(0, 255), 255, 255); }
  if (pastelColors == 0){ lightchaseColorTwo = CHSV(random(0, 255), 255, 255); }
  if (pastelColors == 1){ lightchaseColorOne = CRGB(random(0, 255), random(0, 255), random(0, 255)); }
  if (pastelColors == 1){ lightchaseColorTwo = CRGB(random(0, 255), random(0, 255), random(0, 255)); }
  for (int i=0; i<SPECTRUM_PIXELS; i++) {  //draw forward
    for (byte s=0; s<LEDS_PER_SEGMENT; s++ ){              // 7 LEDs per segment
   int fake =  i * LEDS_PER_SEGMENT;
      if (chaseMode == 0) {LEDs[FAKE_LEDs_C_BLTR[s+((fake))]] = lightchaseColorOne;}
      if (chaseMode == 1) {LEDs[FAKE_LEDs_C_BRTL[s+((fake))]] = lightchaseColorOne;}
      if (chaseMode == 2) {LEDs[FAKE_LEDs_C_BMUP[s+((fake))]] = lightchaseColorOne;}
      if (chaseMode == 3) {LEDs[FAKE_LEDs_C_TMDN[s+((fake))]] = lightchaseColorOne;}
      if (chaseMode == 4) {LEDs[FAKE_LEDs_C_CMOT[s+((fake))]] = lightchaseColorOne;}
      if (chaseMode == 5) {LEDs[FAKE_LEDs_C_TLBR[s+((fake))]] = lightchaseColorOne;}
      if (chaseMode == 6) {LEDs[FAKE_LEDs_C_CSIN[s+((fake))]] = lightchaseColorOne;}
      if (chaseMode == 7) {LEDs[FAKE_LEDs_C_TRBL[s+((fake))]] = lightchaseColorOne;}
      if (chaseMode == 8) {LEDs[FAKE_LEDs_C_OUTS[s+((fake))]] = lightchaseColorOne;}
      if (chaseMode == 9) {LEDs[FAKE_LEDs_C_VERT[s+((fake))]] = lightchaseColorOne;}
      if (chaseMode == 10) {LEDs[FAKE_LEDs_C_OUTS2[s+((fake))]] = lightchaseColorOne;}
      if (chaseMode == 11) {LEDs[FAKE_LEDs_C_VERT2[s+((fake))]] = lightchaseColorOne;}
    }
    FastLED.show();
  for( int d = 0; d < 40; d++) {server.handleClient(); }  //delay to speed, but so the web buttons still work
  // delay(1);
  }  
  for (int i = SPECTRUM_PIXELS-1; i > -1; --i) {   //draw backwards
    for (byte s=0; s<LEDS_PER_SEGMENT; s++ ){              // 7 LEDs per segment
   int fake =  i * LEDS_PER_SEGMENT;
      if (chaseMode == 0) {LEDs[FAKE_LEDs_C_BLTR[s+((fake))]] = lightchaseColorTwo;}
      if (chaseMode == 1) {LEDs[FAKE_LEDs_C_BRTL[s+((fake))]] = lightchaseColorTwo;}
      if (chaseMode == 2) {LEDs[FAKE_LEDs_C_BMUP[s+((fake))]] = lightchaseColorTwo;}
      if (chaseMode == 3) {LEDs[FAKE_LEDs_C_TMDN[s+((fake))]] = lightchaseColorTwo;}
      if (chaseMode == 4) {LEDs[FAKE_LEDs_C_CMOT[s+((fake))]] = lightchaseColorTwo;}
      if (chaseMode == 5) {LEDs[FAKE_LEDs_C_TLBR[s+((fake))]] = lightchaseColorTwo;}
      if (chaseMode == 6) {LEDs[FAKE_LEDs_C_CSIN[s+((fake))]] = lightchaseColorTwo;}
      if (chaseMode == 7) {LEDs[FAKE_LEDs_C_TRBL[s+((fake))]] = lightchaseColorTwo;}
      if (chaseMode == 8) {LEDs[FAKE_LEDs_C_OUTS[s+((fake))]] = lightchaseColorTwo;}
      if (chaseMode == 9) {LEDs[FAKE_LEDs_C_VERT[s+((fake))]] = lightchaseColorTwo;}
      if (chaseMode == 10) {LEDs[FAKE_LEDs_C_OUTS2[s+((fake))]] = lightchaseColorTwo;}
      if (chaseMode == 11) {LEDs[FAKE_LEDs_C_VERT2[s+((fake))]] = lightchaseColorTwo;}
    }
    FastLED.show();
  for( int d = 0; d < 40; d++) {server.handleClient(); }  //delay to speed, but so the web buttons still work
  //  delay(1);
  }  
  if (clockMode != 5) { allBlank(); }
} //end of chase


 
void Twinkles() {
  int audio_input = analogRead(MIC_IN_PIN); 
  int Level = map(audio_input, 100, 2000, 50, 210);
  if (audio_input < 100){  Level = 50;  }
  if (audio_input > 2000){  Level = 210;  }
  int seed = random(Level);         // A random number. Higher number => fewer twinkles. Use random16() for values >255.
  int i = random(SPECTRUM_PIXELS);         // A random number. Higher number => fewer twinkles. Use random16() for values >255.
  if (seed > 46) {
    CRGB color = CRGB::Black;
    if (pastelColors == 0){ color = CHSV(random(0, 255), 255, 255); }
    if (pastelColors == 1){ color = CRGB(random(0, 255), random(0, 255), random(0, 255)); }
          int fake =  i * LEDS_PER_SEGMENT;
          for (byte s=0; s<LEDS_PER_SEGMENT; s++ ){              // 7 LEDs per segment
            LEDs[FAKE_LEDs_C_BLTR[s+((fake))]] = color;
          }
  }
  
  for (int j = 0; j < SPECTRUM_PIXELS; j++) {
          int fake =  j * LEDS_PER_SEGMENT;
          for (byte s=0; s<LEDS_PER_SEGMENT; s++ ){              // 7 LEDs per segment
            LEDs[FAKE_LEDs_C_BLTR[s+((fake))]].fadeToBlackBy(20);
          }
  }
  if (clockMode != 5) { allBlank(); }
} // twinkles



void Rainbow() {
  //fill_gradient_RGB(LEDs, NUM_LEDS, CRGB::Red, CRGB::Yellow, CRGB::Green, CRGB::Blue);
    for (byte j = 0; j < SPECTRUM_PIXELS; j++) {
      int fake =  j * LEDS_PER_SEGMENT;
      for (byte s=0; s<LEDS_PER_SEGMENT; s++ ){              // 7 LEDs per segment
        LEDs[FAKE_LEDs_C_BMUP[s+((fake))]] = colorWheel(((255) + colorWheelPosition) % 256);
      }
    }
  EVERY_N_MILLISECONDS(150) {
    colorWheelPosition = colorWheelPosition - colorWheelSpeed; // SPEED OF COLOR WHEEL
  if (colorWheelPosition < 0) // RESET COLOR WHEEL
    colorWheelPosition = 255;
  }
  if (clockMode != 5) { allBlank(); }
} //end of Rainbow



void GreenMatrix() {
  EVERY_N_MILLISECONDS(65) {
    updateMatrix();
    FastLED.show();
 }
  EVERY_N_MILLISECONDS(30) {
    changeMatrixpattern();
  }
} //loop

void changeMatrixpattern () {
  int rand1 = random16 (SPECTRUM_PIXELS);
  int rand2 = random16 (SPECTRUM_PIXELS);
  if ((greenMatrix[rand1] == 1) && (greenMatrix[rand2] == 0) )   //simple get two random dot 1 and 0 and swap it,
  {
    greenMatrix[rand1] = 0;  //this will not change total number of dots
    greenMatrix[rand2] = 1;
  }
  if (clockMode != 5) { allBlank(); }
} //changeMatrixpattern

void initGreenMatrix() {                               //init array of dots. run once
  for (int i = 0; i < SPECTRUM_PIXELS; i++) {
    if (random8(20) == 0) {
      greenMatrix[i] = 1;  //random8(20) number of dots. decrease for more dots
    }
    else {
      greenMatrix[i] = 0;
    }
  }
} //initGreenMatrix

void updateMatrix() {
    for (byte j = 0; j < SPECTRUM_PIXELS; j++) {
      byte layer = greenMatrix[((j + lightshowSpeed + random8(2) + SPECTRUM_PIXELS) % SPECTRUM_PIXELS)];   //fake scroll based on shift coordinate
      // random8(2) add glitchy look
      if (layer) {
          int fake =  j * LEDS_PER_SEGMENT;
          for (byte s=0; s<LEDS_PER_SEGMENT; s++ ){              // 7 LEDs per segment
            //LEDs[FAKE_LEDs_C_RAIN[s+((fake))]] = CHSV(110, 255, 255);
            LEDs[FAKE_LEDs_C_RAIN[s+((fake))]] = CRGB::Green;
          }
      }
    }
  lightshowSpeed ++;
    for (byte j = 0; j < SPECTRUM_PIXELS; j++) {
      int fake =  j * LEDS_PER_SEGMENT;
      for (byte s=0; s<LEDS_PER_SEGMENT; s++ ){              // 7 LEDs per segment
        LEDs[FAKE_LEDs_C_RAIN[s+((fake))]].fadeToBlackBy( 60 );
      }
    }
} //updateMatrix



void blueRain() {
    int audio_input = analogRead(MIC_IN_PIN); 
  int Level = map(audio_input, 100, 2000, 0, 400);
  if (audio_input < 100){  Level = 0;  }
  if (audio_input > 2000){  Level = 400;  }
    Serial.println(Level);
    int delayRain = 400-Level;
  EVERY_N_MILLIS_I( thistimer, 400 ) { // initial period = 100ms
thistimer.setPeriod(delayRain);
    updaterain();
 //   FastLED.show();
  }
  if (clockMode != 5) { allBlank(); }
} //loop

void raininit() {                               //init array of dots. run once
  for (int i = 0; i < SPECTRUM_PIXELS; i++) {
    if (random8(24) == 0) {  //30?
      rain[i] = 1;  //random8(20) number of dots. decrease for more dots
    }
    else {
      rain[i] = 0;
    }
  }
} //raininit

void updaterain() {
    for (byte j = 0; j < SPECTRUM_PIXELS; j++) {
      byte layer = rain[((j + lightshowSpeed + 2 + SPECTRUM_PIXELS) % SPECTRUM_PIXELS)];   //fake scroll based on shift coordinate
      // random8(2) add glitchy look
      if (layer) {
          int fake =  j * LEDS_PER_SEGMENT;
          for (byte s=0; s<LEDS_PER_SEGMENT; s++ ){              // 7 LEDs per segment
            //LEDs[FAKE_LEDs_C_RAIN[s+((fake))]] = CHSV(110, 255, 255);
            LEDs[FAKE_LEDs_C_RAIN[s+((fake))]] = CRGB::Blue;
          }
      }
    }
  lightshowSpeed ++;
  for (byte j = 0; j < SPECTRUM_PIXELS; j++) {
      int fake =  j * LEDS_PER_SEGMENT;
      for (byte s=0; s<LEDS_PER_SEGMENT; s++ ){              // 7 LEDs per segment
        LEDs[FAKE_LEDs_C_RAIN[s+((fake))]].fadeToBlackBy( 128 );
      }
  }

}



void Fire2021() {
// Array of temperature readings at each simulation cell
  static byte heat[SPECTRUM_PIXELS];

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < SPECTRUM_PIXELS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, 30));  //55-cooling
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= SPECTRUM_PIXELS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }

    // Step 3.  Randomly ignite new 'sparks' of heat to the bottom center 3
    if( random8() < 128 ) {  //128-sparkling
      int y = random8(0,2);
      heat[y] = qadd8( heat[y], random8(155,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < SPECTRUM_PIXELS; j++) {
      CRGB color = HeatColor( heat[j]);
      int fake =  j * LEDS_PER_SEGMENT;
      for (byte s=0; s<LEDS_PER_SEGMENT; s++ ){              // 7 LEDs per segment
        LEDs[FAKE_LEDs_C_FIRE[s+((fake))]] = color;
      }

    }
  if (clockMode != 5) { allBlank(); }
}



void Snake() {  //real random snake mode with random food changing its color
  int move = 0;
  int fadeby = 130;
  int pickOne = random(3);
  //just write out every possible move, that's all, oh and make it random
  if (snakePosition == 0 && snakeLastDirection == 0 && move == 0)  {if (pickOne == 3) {snakeLastDirection = 3; snakePosition = 4; move = 1;} else {snakePosition = 5; move = 1;}}
  if (snakePosition == 0 && snakeLastDirection == 2 && move == 0)  {snakeLastDirection = 3; snakePosition = 1; move = 1;}
  if (snakePosition == 1 && snakeLastDirection == 3 && move == 0)  {if (pickOne == 0) {snakeLastDirection = 0; snakePosition = 3; move = 1;} else {snakePosition = 2; move = 1;}}
  if (snakePosition == 1 && snakeLastDirection == 1 && move == 0)  {snakeLastDirection = 0; snakePosition = 0; move = 1;}
  if (snakePosition == 2 && snakeLastDirection == 3 && move == 0)  {if (pickOne == 0) {snakeLastDirection = 0; snakePosition = 9; move = 1;} else {snakePosition = 10; move = 1;}}
  if (snakePosition == 2 && snakeLastDirection == 1 && move == 0)  {if (pickOne == 0) {snakeLastDirection = 0; snakePosition = 3; move = 1;} else {snakePosition = 1; move = 1;}}
  if (snakePosition == 3 && snakeLastDirection == 0 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 1; snakePosition = 4; move = 1;} else if (pickOne == 3) {snakeLastDirection = 3; snakePosition = 8; move = 1;} else {snakePosition = 7; move = 1;}}
  if (snakePosition == 3 && snakeLastDirection == 2 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 1; snakePosition = 1; move = 1;} else {snakeLastDirection = 3; snakePosition = 2; move = 1;}}
  if (snakePosition == 4 && snakeLastDirection == 1 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 2; snakePosition = 0; move = 1;} else {snakeLastDirection = 0; snakePosition = 5; move = 1;}}
  if (snakePosition == 4 && snakeLastDirection == 3 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 0; snakePosition = 7; move = 1;} else if (pickOne == 3) {snakeLastDirection = 2; snakePosition = 3; move = 1;} else {snakePosition = 8; move = 1;}}
  if (snakePosition == 5 && snakeLastDirection == 0 && move == 0)  {snakeLastDirection = 3; snakePosition = 6; move = 1;}
  if (snakePosition == 5 && snakeLastDirection == 2 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 3; snakePosition = 4; move = 1;} else {snakeLastDirection = 2; snakePosition = 0; move = 1;}}
  if (snakePosition == 6 && snakeLastDirection == 3 && move == 0)  {if (pickOne == 3) {snakeLastDirection = 2; snakePosition = 7; move = 1;} else {snakePosition = 15; move = 1;}}
  if (snakePosition == 6 && snakeLastDirection == 1 && move == 0)  {snakeLastDirection = 2; snakePosition = 5; move = 1;}
  if (snakePosition == 7 && snakeLastDirection == 0 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 1; snakePosition = 6; move = 1;} else {snakeLastDirection = 3; snakePosition = 15; move = 1;}} 
  if (snakePosition == 7 && snakeLastDirection == 2 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 1; snakePosition = 4; move = 1;} else if (pickOne == 3) {snakeLastDirection = 3; snakePosition = 8; move = 1;} else {snakePosition = 3; move = 1;}}
  if (snakePosition == 8 && snakeLastDirection == 1 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 2; snakePosition = 3; move = 1;} else if (pickOne == 3) {snakeLastDirection = 0; snakePosition = 7; move = 1;} else {snakePosition = 4; move = 1;}}
  if (snakePosition == 8 && snakeLastDirection == 3 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 0; snakePosition = 14; move = 1;} else if (pickOne == 3) {snakeLastDirection = 2; snakePosition = 9; move = 1;} else {snakePosition = 13; move = 1;}}
  if (snakePosition == 9 && snakeLastDirection == 0 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 1; snakePosition = 8; move = 1;} else if (pickOne == 3) {snakeLastDirection = 3; snakePosition = 13; move = 1;} else {snakePosition = 14; move = 1;}}
  if (snakePosition == 9 && snakeLastDirection == 2 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 1; snakePosition = 2; move = 1;} else {snakeLastDirection = 3; snakePosition = 10; move = 1;}}
  if (snakePosition == 10 && snakeLastDirection == 3 && move == 0)  {if (pickOne == 0) {snakeLastDirection = 0; snakePosition = 12; move = 1;} else {snakePosition = 11; move = 1;}}
  if (snakePosition == 10 && snakeLastDirection == 1 && move == 0)  {if (pickOne == 0) {snakeLastDirection = 0; snakePosition = 9; move = 1;} else {snakePosition = 2; move = 1;}}
  if (snakePosition == 11 && snakeLastDirection == 3 && move == 0)  {if (pickOne == 0) {snakeLastDirection = 0; snakePosition = 19; move = 1;} else {snakePosition = 20; move = 1;}}
  if (snakePosition == 11 && snakeLastDirection == 1 && move == 0)  {if (pickOne == 0) {snakeLastDirection = 0; snakePosition = 12; move = 1;} else {snakePosition = 10; move = 1;}}
  if (snakePosition == 12 && snakeLastDirection == 0 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 1; snakePosition = 13; move = 1;} else if (pickOne == 3) {snakeLastDirection = 3; snakePosition = 18; move = 1;} else {snakePosition = 17; move = 1;}}
  if (snakePosition == 12 && snakeLastDirection == 2 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 1; snakePosition = 10; move = 1;} else {snakeLastDirection = 3; snakePosition = 11; move = 1;}}
  if (snakePosition == 13 && snakeLastDirection == 1 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 2; snakePosition = 9; move = 1;} else if (pickOne == 3) {snakeLastDirection = 0; snakePosition = 14; move = 1;} else {snakePosition = 8; move = 1;}}
  if (snakePosition == 13 && snakeLastDirection == 3 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 0; snakePosition = 17; move = 1;} else if (pickOne == 3) {snakeLastDirection = 2; snakePosition = 12; move = 1;} else {snakePosition = 18; move = 1;}}
  if (snakePosition == 14 && snakeLastDirection == 0 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 1; snakePosition = 15; move = 1;} else {snakeLastDirection = 3; snakePosition = 16; move = 1;}} 
  if (snakePosition == 14 && snakeLastDirection == 2 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 1; snakePosition = 8; move = 1;} else if (pickOne == 3) {snakeLastDirection = 3; snakePosition = 13; move = 1;} else {snakePosition = 9; move = 1;}}
  if (snakePosition == 15 && snakeLastDirection == 3 && move == 0)  {if (pickOne == 3) {snakeLastDirection = 2; snakePosition = 14; move = 1;} else {snakePosition = 16; move = 1;}}
  if (snakePosition == 15 && snakeLastDirection == 1 && move == 0)  {if (pickOne == 3) {snakeLastDirection = 2; snakePosition = 7; move = 1;} else {snakeLastDirection = 1; snakePosition = 6; move = 1;}}
  if (snakePosition == 16 && snakeLastDirection == 3 && move == 0)  {if (pickOne == 3) {snakeLastDirection = 2; snakePosition = 17; move = 1;} else {snakePosition = 25; move = 1;}}
  if (snakePosition == 16 && snakeLastDirection == 1 && move == 0)  {if (pickOne == 3) {snakeLastDirection = 2; snakePosition = 14; move = 1;} else {snakeLastDirection = 1; snakePosition = 15; move = 1;}}
  if (snakePosition == 17 && snakeLastDirection == 0 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 1; snakePosition = 16; move = 1;} else {snakeLastDirection = 3; snakePosition = 25; move = 1;}} 
  if (snakePosition == 17 && snakeLastDirection == 2 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 1; snakePosition = 13; move = 1;} else if (pickOne == 3) {snakeLastDirection = 3; snakePosition = 18; move = 1;} else {snakePosition = 12; move = 1;}}
  if (snakePosition == 18 && snakeLastDirection == 1 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 2; snakePosition = 12; move = 1;} else if (pickOne == 3) {snakeLastDirection = 0; snakePosition = 17; move = 1;} else {snakePosition = 13; move = 1;}}
  if (snakePosition == 18 && snakeLastDirection == 3 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 0; snakePosition = 24; move = 1;} else if (pickOne == 3) {snakeLastDirection = 2; snakePosition = 19; move = 1;} else {snakePosition = 23; move = 1;}}
  if (snakePosition == 19 && snakeLastDirection == 0 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 1; snakePosition = 18; move = 1;} else if (pickOne == 3) {snakeLastDirection = 3; snakePosition = 23; move = 1;} else {snakePosition = 24; move = 1;}}
  if (snakePosition == 19 && snakeLastDirection == 2 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 1; snakePosition = 11; move = 1;} else {snakeLastDirection = 3; snakePosition = 20; move = 1;}}
  if (snakePosition == 20 && snakeLastDirection == 3 && move == 0)  {if (pickOne == 0) {snakeLastDirection = 0; snakePosition = 22; move = 1;} else {snakePosition = 21; move = 1;}}
  if (snakePosition == 20 && snakeLastDirection == 1 && move == 0)  {if (pickOne == 0) {snakeLastDirection = 0; snakePosition = 19; move = 1;} else {snakePosition = 11; move = 1;}}
  if (snakePosition == 21 && snakeLastDirection == 3 && move == 0)  {if (pickOne == 0) {snakeLastDirection = 0; snakePosition = 29; move = 1;} else {snakePosition = 30; move = 1;}}
  if (snakePosition == 21 && snakeLastDirection == 1 && move == 0)  {if (pickOne == 0) {snakeLastDirection = 0; snakePosition = 22; move = 1;} else {snakePosition = 20; move = 1;}}
  if (snakePosition == 22 && snakeLastDirection == 0 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 1; snakePosition = 23; move = 1;} else if (pickOne == 3) {snakeLastDirection = 3; snakePosition = 28; move = 1;} else {snakePosition = 27; move = 1;}}
  if (snakePosition == 22 && snakeLastDirection == 2 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 1; snakePosition = 20; move = 1;} else {snakeLastDirection = 3; snakePosition = 21; move = 1;}}
  if (snakePosition == 23 && snakeLastDirection == 1 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 2; snakePosition = 19; move = 1;} else if (pickOne == 3) {snakeLastDirection = 0; snakePosition = 24; move = 1;} else {snakePosition = 18; move = 1;}}
  if (snakePosition == 23 && snakeLastDirection == 3 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 0; snakePosition = 27; move = 1;} else if (pickOne == 3) {snakeLastDirection = 2; snakePosition = 22; move = 1;} else {snakePosition = 28; move = 1;}}
  if (snakePosition == 24 && snakeLastDirection == 0 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 1; snakePosition = 25; move = 1;} else {snakeLastDirection = 3; snakePosition = 26; move = 1;}} 
  if (snakePosition == 24 && snakeLastDirection == 2 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 1; snakePosition = 18; move = 1;} else if (pickOne == 3) {snakeLastDirection = 3; snakePosition = 23; move = 1;} else {snakePosition = 19; move = 1;}}
  if (snakePosition == 25 && snakeLastDirection == 3 && move == 0)  {if (pickOne == 3) {snakeLastDirection = 2; snakePosition = 24; move = 1;} else {snakePosition = 26; move = 1;}}
  if (snakePosition == 25 && snakeLastDirection == 1 && move == 0)  {if (pickOne == 3) {snakeLastDirection = 2; snakePosition = 17; move = 1;} else {snakeLastDirection = 1; snakePosition = 16; move = 1;}}
  if (snakePosition == 26 && snakeLastDirection == 3 && move == 0)  {if (pickOne == 3) {snakeLastDirection = 2; snakePosition = 27; move = 1;} else {snakePosition = 34; move = 1;}}
  if (snakePosition == 26 && snakeLastDirection == 1 && move == 0)  {if (pickOne == 3) {snakeLastDirection = 2; snakePosition = 24; move = 1;} else {snakeLastDirection = 1; snakePosition = 25; move = 1;}}
  if (snakePosition == 27 && snakeLastDirection == 0 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 1; snakePosition = 26; move = 1;} else {snakeLastDirection = 3; snakePosition = 34; move = 1;}} 
  if (snakePosition == 27 && snakeLastDirection == 2 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 1; snakePosition = 23; move = 1;} else if (pickOne == 3) {snakeLastDirection = 3; snakePosition = 28; move = 1;} else {snakePosition = 22; move = 1;}}
  if (snakePosition == 28 && snakeLastDirection == 1 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 2; snakePosition = 22; move = 1;} else if (pickOne == 3) {snakeLastDirection = 0; snakePosition = 27; move = 1;} else {snakePosition = 23; move = 1;}}
  if (snakePosition == 28 && snakeLastDirection == 3 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 0; snakePosition = 33; move = 1;} else if (pickOne == 3) {snakeLastDirection = 2; snakePosition = 29; move = 1;} else {snakePosition = 32; move = 1;}}
  if (snakePosition == 29 && snakeLastDirection == 0 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 1; snakePosition = 28; move = 1;} else if (pickOne == 3) {snakeLastDirection = 3; snakePosition = 32; move = 1;} else {snakePosition = 33; move = 1;}}
  if (snakePosition == 29 && snakeLastDirection == 2 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 1; snakePosition = 21; move = 1;} else {snakeLastDirection = 3; snakePosition = 30; move = 1;}}
  if (snakePosition == 30 && snakeLastDirection == 3 && move == 0)  {snakeLastDirection = 0; snakePosition = 31; move = 1;}
  if (snakePosition == 30 && snakeLastDirection == 1 && move == 0)  {if (pickOne == 0) {snakeLastDirection = 0; snakePosition = 29; move = 1;} else {snakePosition = 21; move = 1;}}
  if (snakePosition == 31 && snakeLastDirection == 0 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 1; snakePosition = 32; move = 1;} else {snakePosition = 36; move = 1;}}
  if (snakePosition == 31 && snakeLastDirection == 2 && move == 0)  {snakeLastDirection = 1; snakePosition = 30; move = 1;}
  if (snakePosition == 32 && snakeLastDirection == 1 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 2; snakePosition = 29; move = 1;} else if (pickOne == 3) {snakeLastDirection = 0; snakePosition = 33; move = 1;} else {snakePosition = 28; move = 1;}}
  if (snakePosition == 32 && snakeLastDirection == 3 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 0; snakePosition = 36; move = 1;} else if (pickOne == 3) {snakeLastDirection = 2; snakePosition = 31; move = 1;}}
  if (snakePosition == 33 && snakeLastDirection == 0 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 1; snakePosition = 34; move = 1;} else {snakeLastDirection = 3; snakePosition = 35; move = 1;}} 
  if (snakePosition == 33 && snakeLastDirection == 2 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 1; snakePosition = 28; move = 1;} else if (pickOne == 3) {snakeLastDirection = 3; snakePosition = 32; move = 1;} else {snakePosition = 29; move = 1;}}
  if (snakePosition == 34 && snakeLastDirection == 3 && move == 0)  {if (pickOne == 3) {snakeLastDirection = 2; snakePosition = 33; move = 1;} else {snakePosition = 35; move = 1;}}
  if (snakePosition == 34 && snakeLastDirection == 1 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 2; snakePosition = 27; move = 1;} else {snakeLastDirection = 1; snakePosition = 26; move = 1;}}
  if (snakePosition == 35 && snakeLastDirection == 3 && move == 0)  {snakeLastDirection = 2; snakePosition = 36; move = 1;}
  if (snakePosition == 35 && snakeLastDirection == 1 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 2; snakePosition = 33; move = 1;} else {snakeLastDirection = 1; snakePosition = 34; move = 1;}}
  if (snakePosition == 36 && snakeLastDirection == 0 && move == 0)  {snakeLastDirection = 1; snakePosition = 35; move = 1;}
  if (snakePosition == 36 && snakeLastDirection == 2 && move == 0)  {if (pickOne == 1) {snakeLastDirection = 1; snakePosition = 32; move = 1;} else {snakePosition = 31; move = 1;}}
  if (snakePosition == foodSpot) { oldsnakecolor = spotcolor;  snakeWaiting = 1; foodSpot = 40;}  //did snake find the food, change snake color
  if (snakeWaiting > 0) {snakeWaiting = snakeWaiting + 1;} //counting while waiting
  int audio_input = analogRead(MIC_IN_PIN); 
  int Level = map(audio_input, 100, 2000, 1, 10);
  if (audio_input < 100){  Level = 1;  }
  if (audio_input > 2000){  Level = 10;  }
  if (snakeWaiting > random((30/Level),(600/Level))) {snakeWaiting = 0; foodSpot = random(36); spotcolor = CHSV(random(0, 255), 255, 255); if (getSlower > 3){  getSlower =  getSlower / 3;  }}  //waiting time is up, pick new spot, new color, reset speed
  if (getSlower > 1000){  getSlower = 1000;  }
  int fake =  foodSpot * LEDS_PER_SEGMENT;   //draw food
  for (byte s=0; s<LEDS_PER_SEGMENT; s++ ){              // 7 LEDs per segment
    if (foodSpot != 40)     {  LEDs[FAKE_LEDs_C_BRTL[s+((fake))]] = spotcolor; }  //draw food, but not while waiting
  }
  int fake2 =  snakePosition * LEDS_PER_SEGMENT;   //draw snake
  for (byte s=0; s<LEDS_PER_SEGMENT; s++ ){              // 7 LEDs per segment
    LEDs[FAKE_LEDs_C_BRTL[s+((fake2))]] = oldsnakecolor;
  } 
  for (byte j = 0; j < SPECTRUM_PIXELS; j++) {  //slowly erase snake
    int fake3 =  j * LEDS_PER_SEGMENT;
    for (byte s=0; s<LEDS_PER_SEGMENT; s++ ){              // 7 LEDs per segment
      LEDs[FAKE_LEDs_C_BRTL[s+((fake3))]].fadeToBlackBy( fadeby );
     }
  }
  getSlower = getSlower + 40;
  if (clockMode != 5) { allBlank(); }
} //snake

void Cylon() {
  int fake = 0;
   const uint8_t CYLON[12] = {4,8,13,18,23,28,32,28,23,18,13,8};
   
   if (cylonPosition >=12) { cylonPosition = 0;}
   fake = CYLON[cylonPosition] * LEDS_PER_SEGMENT;
      for (byte s=0; s<LEDS_PER_SEGMENT; s++ ){LEDs[FAKE_LEDs_SNAKE[s+((fake))]] = CRGB::Red;}
  cylonPosition++;
  
   if (cylonPosition >=12) { cylonPosition = 0;}
    for (byte j = 0; j < SPECTRUM_PIXELS; j++) {
      int fake =  j * LEDS_PER_SEGMENT;
      for (byte s=0; s<LEDS_PER_SEGMENT; s++ ){              // 7 LEDs per segment
        LEDs[FAKE_LEDs_SNAKE[s+((fake))]].fadeToBlackBy( 120 );
      }
    }
  if (clockMode != 5) { allBlank(); }
} //Cylon


void loadSetupSettings(){  //setting stored in preffs and loaded at boot
  preferences.begin("shelfclock", false);
  gmtOffset_sec = preferences.getLong("gmtOffset_sec", -28800);
  DSTime = preferences.getBool("DSTime", 0);
  cd_r_val = preferences.getInt("cd_r_val", 0);
  cd_g_val = preferences.getInt("cd_g_val", 255);
  cd_b_val = preferences.getInt("cd_b_val", 0);
  r0_val = preferences.getInt("r0_val", 193);
  g0_val = preferences.getInt("g0_val", 204);
  b0_val = preferences.getInt("b0_val", 78);
  r1_val = preferences.getInt("r1_val", 255);
  g1_val = preferences.getInt("g1_val", 0);
  b1_val = preferences.getInt("b1_val", 0);
  r2_val = preferences.getInt("r2_val", 255);
  g2_val = preferences.getInt("g2_val", 0);
  b2_val = preferences.getInt("b2_val", 0);
  r3_val = preferences.getInt("r3_val", 255);
  g3_val = preferences.getInt("g3_val", 0);
  b3_val = preferences.getInt("b3_val", 0);
  r4_val = preferences.getInt("r4_val", 255);
  g4_val = preferences.getInt("g4_val", 0);
  b4_val = preferences.getInt("b4_val", 0);
  r5_val = preferences.getInt("r5_val", 255);
  g5_val = preferences.getInt("g5_val", 0);
  b5_val = preferences.getInt("b5_val", 0);
  r6_val = preferences.getInt("r6_val", 255);
  g6_val = preferences.getInt("g6_val", 0);
  b6_val = preferences.getInt("b6_val", 0);
  r7_val = preferences.getInt("r7_val", 255);
  g7_val = preferences.getInt("g7_val", 0);
  b7_val = preferences.getInt("b7_val", 0);
  r8_val = preferences.getInt("r8_val", 255);
  g8_val = preferences.getInt("g8_val", 0);
  b8_val = preferences.getInt("b8_val", 0);
  r9_val = preferences.getInt("r9_val", 255);
  g9_val = preferences.getInt("g9_val", 0);
  b9_val = preferences.getInt("b9_val", 0);
  r10_val = preferences.getInt("r10_val", 255);
  g10_val = preferences.getInt("g10_val", 0);
  b10_val = preferences.getInt("b10_val", 0);
  r11_val = preferences.getInt("r11_val", 255);
  g11_val = preferences.getInt("g11_val", 0);
  b11_val = preferences.getInt("b11_val", 0);
  r12_val = preferences.getInt("r12_val", 255);
  g12_val = preferences.getInt("g12_val", 0);
  b12_val = preferences.getInt("b12_val", 0);
  r13_val = preferences.getInt("r13_val", 255);
  g13_val = preferences.getInt("g13_val", 0);
  b13_val = preferences.getInt("b13_val", 0);
  r14_val = preferences.getInt("r14_val", 255);
  g14_val = preferences.getInt("g14_val", 0);
  b14_val = preferences.getInt("b14_val", 0);
  r15_val = preferences.getInt("r15_val", 255);
  g15_val = preferences.getInt("g15_val", 0);
  b15_val = preferences.getInt("b15_val", 0);
  r16_val = preferences.getInt("r16_val", 255);
  g16_val = preferences.getInt("g16_val", 255);
  b16_val = preferences.getInt("b16_val", 255);
  r17_val = preferences.getInt("r17_val", 0);
  g17_val = preferences.getInt("g17_val", 0);
  b17_val = preferences.getInt("b17_val", 0);
  clockMode = preferences.getInt("clockMode", 0);
  pastelColors = preferences.getInt("pastelColors", 0);
  temperatureSymbol = preferences.getInt("temperatureSym", 39);
  ClockColorSettings = preferences.getInt("ClockColorSet", 0);
  DateColorSettings = preferences.getInt("DateColorSet", 0);
  tempColorSettings = preferences.getInt("tempColorSet", 0);
  humiColorSettings = preferences.getInt("humiColorSet", 0);
  tempDisplayType = preferences.getInt("tempDisplayType", 0);
  humiDisplayType = preferences.getInt("humiDisplayType", 0);
  temperatureCorrection = preferences.getInt("tempCorrection", 0);
  colonType = preferences.getInt("colonType", 0);
  ColorChangeFrequency = preferences.getInt("ColorChangeFreq", 0);
  scrollText = preferences.getString("scrollText", "dAdS ArE tHE bESt");
  clockDisplayType = preferences.getInt("clockDispType", 3);
  dateDisplayType = preferences.getInt("dateDisplayType", 5);
  colorchangeCD = preferences.getBool("colorchangeCD", 1);
  useAudibleAlarm = preferences.getBool("alarmCD", 0);
  spectrumMode = preferences.getInt("spectrumMode", 0);
  realtimeMode = preferences.getInt("realtimeMode", 0);
  spectrumColorSettings = preferences.getInt("spectrumColor", 2);
  spectrumBackgroundSettings = preferences.getInt("spectrumBkgd", 0);
  spotlightsColorSettings = preferences.getInt("spotlightsCoSe", 0);
  brightness = preferences.getInt("brightness", 10);
  useSpotlights = preferences.getBool("useSpotlights", 1);
  scrollColorSettings = preferences.getInt("scrollColorSet", 0);
  scrollFrequency = preferences.getInt("scrollFreq", 1);
  randomSpectrumMode = preferences.getBool("randSpecMode", 0);
  scrollOverride = preferences.getBool("scrollOverride", 1);
  scrollOptions1 = preferences.getBool("scrollOptions1", 0);
  scrollOptions2 = preferences.getBool("scrollOptions2", 0);
  scrollOptions3 = preferences.getBool("scrollOptions3", 0);
  scrollOptions4 = preferences.getBool("scrollOptions4", 0);
  scrollOptions5 = preferences.getBool("scrollOptions5", 0);
  scrollOptions6 = preferences.getBool("scrollOptions6", 0);
  scrollOptions7 = preferences.getBool("scrollOptions7", 0);
  scrollOptions8 = preferences.getBool("scrollOptions8", 0);
  lightshowMode = preferences.getInt("lightshowMode", 0);
  suspendFrequency = preferences.getInt("suspendFreq", 1);
  suspendType = preferences.getInt("suspendType", 0);

  Serial.println("-----------");
  Serial.println(tempConfig.outdoor_enable);
  Serial.println(tempConfig.outdoor_lat);
  Serial.println(tempConfig.outdoor_long);
  Serial.println(tempConfig.outdoor_apikey);
  Serial.println("-----------");


  preferences.getBytes("temperature", &tempConfig, preferences.getBytesLength("temperature"));

  Serial.println("-----------");
  Serial.println(tempConfig.outdoor_enable);
  Serial.println(tempConfig.outdoor_lat);
  Serial.println(tempConfig.outdoor_long);
  Serial.println(tempConfig.outdoor_apikey);
  Serial.println("-----------");

  //   ssid = preferences.getChar("ssid");
  //    password = preferences.getChar("password");
}

void getpreset1(){
    gmtOffset_sec = preferences.getLong("p1-gmtOffset", -28800);
    DSTime = preferences.getBool("p1-DSTime", 0);
    cd_r_val = preferences.getInt("p1-cd_r_val", 0);
    cd_g_val = preferences.getInt("p1-cd_g_val", 255);
    cd_b_val = preferences.getInt("p1-cd_b_val", 0);
    r0_val = preferences.getInt("p1-r0_val", 193);
    g0_val = preferences.getInt("p1-g0_val", 204);
    b0_val = preferences.getInt("p1-b0_val", 78);
    r1_val = preferences.getInt("p1-r1_val", 255);
    g1_val = preferences.getInt("p1-g1_val", 0);
    b1_val = preferences.getInt("p1-b1_val", 0);
    r2_val = preferences.getInt("p1-r2_val", 255);
    g2_val = preferences.getInt("p1-g2_val", 0);
    b2_val = preferences.getInt("p1-b2_val", 0);
    r3_val = preferences.getInt("p1-r3_val", 255);
    g3_val = preferences.getInt("p1-g3_val", 0);
    b3_val = preferences.getInt("p1-b3_val", 0);
    r4_val = preferences.getInt("p1-r4_val", 255);
    g4_val = preferences.getInt("p1-g4_val", 0);
    b4_val = preferences.getInt("p1-b4_val", 0);
    r5_val = preferences.getInt("p1-r5_val", 255);
    g5_val = preferences.getInt("p1-g5_val", 0);
    b5_val = preferences.getInt("p1-b5_val", 0);
    r6_val = preferences.getInt("p1-r6_val", 255);
    g6_val = preferences.getInt("p1-g6_val", 0);
    b6_val = preferences.getInt("p1-b6_val", 0);
    r7_val = preferences.getInt("p1-r7_val", 255);
    g7_val = preferences.getInt("p1-g7_val", 0);
    b7_val = preferences.getInt("p1-b7_val", 0);
    r8_val = preferences.getInt("p1-r8_val", 255);
    g8_val = preferences.getInt("p1-g8_val", 0);
    b8_val = preferences.getInt("p1-b8_val", 0);
    r9_val = preferences.getInt("p1-r9_val", 255);
    g9_val = preferences.getInt("p1-g9_val", 0);
    b9_val = preferences.getInt("p1-b9_val", 0);
    r10_val = preferences.getInt("p1-r10_val", 255);
    g10_val = preferences.getInt("p1-g10_val", 0);
    b10_val = preferences.getInt("p1-b10_val", 0);
    r11_val = preferences.getInt("p1-r11_val", 255);
    g11_val = preferences.getInt("p1-g11_val", 0);
    b11_val = preferences.getInt("p1-b11_val", 0);
    r12_val = preferences.getInt("p1-r12_val", 255);
    g12_val = preferences.getInt("p1-g12_val", 0);
    b12_val = preferences.getInt("p1-b12_val", 0);
    r13_val = preferences.getInt("p1-r13_val", 255);
    g13_val = preferences.getInt("p1-g13_val", 0);
    b13_val = preferences.getInt("p1-b13_val", 0);
    r14_val = preferences.getInt("p1-r14_val", 255);
    g14_val = preferences.getInt("p1-g14_val", 0);
    b14_val = preferences.getInt("p1-b14_val", 0);
    r15_val = preferences.getInt("p1-r15_val", 255);
    g15_val = preferences.getInt("p1-g15_val", 0);
    b15_val = preferences.getInt("p1-b15_val", 0);
    r16_val = preferences.getInt("p1-r16_val", 255);
    g16_val = preferences.getInt("p1-g16_val", 255);
    b16_val = preferences.getInt("p1-b16_val", 255);
    r17_val = preferences.getInt("p1-r17_val", 0);
    g17_val = preferences.getInt("p1-g17_val", 0);
    b17_val = preferences.getInt("p1-b17_val", 0);
    clockMode = preferences.getInt("p1-clockMode", 0);
    pastelColors = preferences.getInt("p1-pastelColors", 0);
    temperatureSymbol = preferences.getInt("p1-temperatSym", 39);
    ClockColorSettings = preferences.getInt("p1-CckColrSet", 0);
    DateColorSettings = preferences.getInt("p1-DateColorSet", 0);
    tempColorSettings = preferences.getInt("p1-tempColorSet", 0);
    humiColorSettings = preferences.getInt("p1-humiColorSet", 0);
    tempDisplayType = preferences.getInt("p1-tempDisType", 0);
    humiDisplayType = preferences.getInt("p1-humiDisType", 0);
    temperatureCorrection = preferences.getInt("p1-tempCorrec", 0);
    colonType = preferences.getInt("p1-colonType", 0);
    ColorChangeFrequency = preferences.getInt("p1-ClrChaFreq", 0);
    scrollText = preferences.getString("p1-scrollText", "PrESEt 1");
    clockDisplayType = preferences.getInt("p1-clkDisType", 3);
    dateDisplayType = preferences.getInt("p1-datDisType", 5);
    colorchangeCD = preferences.getBool("p1-clrchanCD", 1);
    useAudibleAlarm = preferences.getBool("p1-alarmCD", 0);
    randomSpectrumMode = preferences.getBool("p1-randSpecMode", 0);
    spectrumMode = preferences.getInt("p1-spectrumMode", 0);
    realtimeMode = preferences.getInt("p1-realtmMode", 0);
    spectrumColorSettings = preferences.getInt("p1-spectrumClr", 2);
    spectrumBackgroundSettings = preferences.getInt("p1-spectrumBkg", 0);
    spotlightsColorSettings = preferences.getInt("p1-spotlColor", 0);
    brightness = preferences.getInt("p1-brightness", 10);
    useSpotlights = preferences.getBool("p1-useSL", 1);
    scrollColorSettings = preferences.getInt("p1-scrolClrSet", 0);
    scrollFrequency = preferences.getInt("p1-scrollFreq", 1);
    scrollOverride = preferences.getBool("p1-scrollOvRd", 1);
    scrollOptions1 = preferences.getBool("p1-scrollOpts1", 0);
    scrollOptions2 = preferences.getBool("p1-scrollOpts2", 0);
    scrollOptions3 = preferences.getBool("p1-scrollOpts3", 0);
    scrollOptions4 = preferences.getBool("p1-scrollOpts4", 0);
    scrollOptions5 = preferences.getBool("p1-scrollOpts5", 0);
    scrollOptions6 = preferences.getBool("p1-scrollOpts6", 0);
    scrollOptions7 = preferences.getBool("p1-scrollOpts7", 0);
    scrollOptions8 = preferences.getBool("p1-scrollOpts8", 0);
    lightshowMode = preferences.getInt("p1-lsMode", 0);
    suspendFrequency = preferences.getInt("p1-suspendFreq", 1);
    suspendType = preferences.getInt("p1-suspendType", 0);
}

void setpreset1(){
  preferences.putLong("p1-gmtOffset", gmtOffset_sec);
  preferences.putBool("p1-DSTime", DSTime);
  preferences.putInt("p1-cd_b_val", cd_b_val);
  preferences.putInt("p1-cd_g_val", cd_g_val);
  preferences.putInt("p1-cd_r_val", cd_r_val);
  preferences.putInt("p1-b0_val", b0_val);
  preferences.putInt("p1-b1_val", b1_val);
  preferences.putInt("p1-b2_val", b2_val);
  preferences.putInt("p1-b3_val", b3_val);
  preferences.putInt("p1-b4_val", b4_val);
  preferences.putInt("p1-b5_val", b5_val);
  preferences.putInt("p1-b6_val", b6_val);
  preferences.putInt("p1-b7_val", b7_val);
  preferences.putInt("p1-b8_val", b8_val); 
  preferences.putInt("p1-b9_val", b9_val);
  preferences.putInt("p1-b10_val", b10_val);
  preferences.putInt("p1-b11_val", b11_val);
  preferences.putInt("p1-b12_val", b12_val);
  preferences.putInt("p1-b13_val", b13_val);
  preferences.putInt("p1-b14_val", b14_val);
  preferences.putInt("p1-b15_val", b15_val);
  preferences.putInt("p1-b16_val", b16_val);
  preferences.putInt("p1-b17_val", b17_val);
  preferences.putInt("p1-g0_val", g0_val);
  preferences.putInt("p1-g1_val", g1_val);
  preferences.putInt("p1-g2_val", g2_val);
  preferences.putInt("p1-g3_val", g3_val);
  preferences.putInt("p1-g4_val", g4_val);
  preferences.putInt("p1-g5_val", g5_val);
  preferences.putInt("p1-g6_val", g6_val);
  preferences.putInt("p1-g7_val", g7_val);
  preferences.putInt("p1-g8_val", g8_val);
  preferences.putInt("p1-g9_val", g9_val);
  preferences.putInt("p1-g10_val", g10_val);
  preferences.putInt("p1-g11_val", g11_val);
  preferences.putInt("p1-g12_val", g12_val);
  preferences.putInt("p1-g13_val", g13_val);
  preferences.putInt("p1-g14_val", g14_val);
  preferences.putInt("p1-g15_val", g15_val);
  preferences.putInt("p1-g16_val", g16_val);
  preferences.putInt("p1-g17_val", g17_val);
  preferences.putInt("p1-r0_val", r0_val);
  preferences.putInt("p1-r1_val", r1_val);
  preferences.putInt("p1-r2_val", r2_val);
  preferences.putInt("p1-r3_val", r3_val);
  preferences.putInt("p1-r4_val", r4_val);
  preferences.putInt("p1-r5_val", r5_val);
  preferences.putInt("p1-r6_val", r6_val);
  preferences.putInt("p1-r7_val", r7_val);
  preferences.putInt("p1-r8_val", r8_val);
  preferences.putInt("p1-r9_val", r9_val);
  preferences.putInt("p1-r10_val", r10_val);
  preferences.putInt("p1-r11_val", r11_val);
  preferences.putInt("p1-r12_val", r12_val);
  preferences.putInt("p1-r13_val", r13_val);
  preferences.putInt("p1-r14_val", r14_val);
  preferences.putInt("p1-r15_val", r15_val);
  preferences.putInt("p1-r16_val", r16_val);
  preferences.putInt("p1-r17_val", r17_val);
  preferences.putInt("p1-clockMode", clockMode);
  preferences.putInt("p1-pastelColors", pastelColors);
  preferences.putInt("p1-temperatSym", temperatureSymbol);
  preferences.putInt("p1-CckColrSet", ClockColorSettings);
  preferences.putInt("p1-DateColorSet", DateColorSettings);
  preferences.putInt("p1-tempColorSet", tempColorSettings);
  preferences.putInt("p1-humiColorSet", humiColorSettings);
  preferences.putInt("p1-tempDisType", tempDisplayType);
  preferences.putInt("p1-humiDisType", humiDisplayType);
  preferences.putInt("p1-tempCorrec", temperatureCorrection);
  preferences.putInt("p1-colonType", colonType);
  preferences.putInt("p1-ClrChaFreq", ColorChangeFrequency);
  preferences.putString("p1-scrollText", scrollText);
  preferences.putInt("p1-clkDisType", clockDisplayType);
  preferences.putInt("p1-datDisType", dateDisplayType);
  preferences.putBool("p1-clrchanCD", colorchangeCD);
  preferences.putBool("p1-alarmCD", useAudibleAlarm);
  preferences.putBool("p1-randSpecMode", randomSpectrumMode);
  preferences.putInt("p1-spectrumMode", spectrumMode); 
  preferences.putInt("p1-realtmMode", realtimeMode);  
  preferences.putInt("p1-spectrumClr", spectrumColorSettings);
  preferences.putInt("p1-spectrumBkg", spectrumBackgroundSettings);
  preferences.putInt("p1-spotlColor", spotlightsColorSettings);
  preferences.putInt("p1-brightness", brightness);
  preferences.putBool("p1-useSL", useSpotlights);
  preferences.putInt("p1-scrolClrSet", scrollColorSettings);
  preferences.putInt("p1-scrollFreq", scrollFrequency);
  preferences.putBool("p1-scrollOvRd", scrollOverride);
  preferences.putBool("p1-scrollOpts1", scrollOptions1);
  preferences.putBool("p1-scrollOpts2", scrollOptions2);
  preferences.putBool("p1-scrollOpts3", scrollOptions3);
  preferences.putBool("p1-scrollOpts4", scrollOptions4);
  preferences.putBool("p1-scrollOpts5", scrollOptions5);
  preferences.putBool("p1-scrollOpts6", scrollOptions6);
  preferences.putBool("p1-scrollOpts7", scrollOptions7);
  preferences.putBool("p1-scrollOpts8", scrollOptions8);
  preferences.putInt("p1-lsMode", lightshowMode); 
  preferences.putInt("p1-suspendFreq", suspendFrequency);
  preferences.putInt("p1-suspendType", suspendType);
}

void getpreset2(){
    gmtOffset_sec = preferences.getLong("p2-gmtOffset", -28800);
    DSTime = preferences.getBool("p2-DSTime", 0);
    cd_r_val = preferences.getInt("p2-cd_r_val", 0);
    cd_g_val = preferences.getInt("p2-cd_g_val", 255);
    cd_b_val = preferences.getInt("p2-cd_b_val", 0);
    r0_val = preferences.getInt("p2-r0_val", 193);
    g0_val = preferences.getInt("p2-g0_val", 204);
    b0_val = preferences.getInt("p2-b0_val", 78);
    r1_val = preferences.getInt("p2-r1_val", 255);
    g1_val = preferences.getInt("p2-g1_val", 0);
    b1_val = preferences.getInt("p2-b1_val", 0);
    r2_val = preferences.getInt("p2-r2_val", 255);
    g2_val = preferences.getInt("p2-g2_val", 0);
    b2_val = preferences.getInt("p2-b2_val", 0);
    r3_val = preferences.getInt("p2-r3_val", 255);
    g3_val = preferences.getInt("p2-g3_val", 0);
    b3_val = preferences.getInt("p2-b3_val", 0);
    r4_val = preferences.getInt("p2-r4_val", 255);
    g4_val = preferences.getInt("p2-g4_val", 0);
    b4_val = preferences.getInt("p2-b4_val", 0);
    r5_val = preferences.getInt("p2-r5_val", 255);
    g5_val = preferences.getInt("p2-g5_val", 0);
    b5_val = preferences.getInt("p2-b5_val", 0);
    r6_val = preferences.getInt("p2-r6_val", 255);
    g6_val = preferences.getInt("p2-g6_val", 0);
    b6_val = preferences.getInt("p2-b6_val", 0);
    r7_val = preferences.getInt("p2-r7_val", 255);
    g7_val = preferences.getInt("p2-g7_val", 0);
    b7_val = preferences.getInt("p2-b7_val", 0);
    r8_val = preferences.getInt("p2-r8_val", 255);
    g8_val = preferences.getInt("p2-g8_val", 0);
    b8_val = preferences.getInt("p2-b8_val", 0);
    r9_val = preferences.getInt("p2-r9_val", 255);
    g9_val = preferences.getInt("p2-g9_val", 0);
    b9_val = preferences.getInt("p2-b9_val", 0);
    r10_val = preferences.getInt("p2-r10_val", 255);
    g10_val = preferences.getInt("p2-g10_val", 0);
    b10_val = preferences.getInt("p2-b10_val", 0);
    r11_val = preferences.getInt("p2-r11_val", 255);
    g11_val = preferences.getInt("p2-g11_val", 0);
    b11_val = preferences.getInt("p2-b11_val", 0);
    r12_val = preferences.getInt("p2-r12_val", 255);
    g12_val = preferences.getInt("p2-g12_val", 0);
    b12_val = preferences.getInt("p2-b12_val", 0);
    r13_val = preferences.getInt("p2-r13_val", 255);
    g13_val = preferences.getInt("p2-g13_val", 0);
    b13_val = preferences.getInt("p2-b13_val", 0);
    r14_val = preferences.getInt("p2-r14_val", 255);
    g14_val = preferences.getInt("p2-g14_val", 0);
    b14_val = preferences.getInt("p2-b14_val", 0);
    r15_val = preferences.getInt("p2-r15_val", 255);
    g15_val = preferences.getInt("p2-g15_val", 0);
    b15_val = preferences.getInt("p2-b15_val", 0);
    r16_val = preferences.getInt("p2-r16_val", 255);
    g16_val = preferences.getInt("p2-g16_val", 255);
    b16_val = preferences.getInt("p2-b16_val", 255);
    r17_val = preferences.getInt("p2-r17_val", 0);
    g17_val = preferences.getInt("p2-g17_val", 0);
    b17_val = preferences.getInt("p2-b17_val", 0);
    clockMode = preferences.getInt("p2-clockMode", 0);
    pastelColors = preferences.getInt("p2-pastelColors", 0);
    temperatureSymbol = preferences.getInt("p2-temperatSym", 39);
    ClockColorSettings = preferences.getInt("p2-CckColrSet", 0);
    DateColorSettings = preferences.getInt("p2-DateColorSet", 0);
    tempColorSettings = preferences.getInt("p2-tempColorSet", 0);
    humiColorSettings = preferences.getInt("p2-humiColorSet", 0);
    tempDisplayType = preferences.getInt("p2-tempDisType", 0);
    humiDisplayType = preferences.getInt("p2-humiDisType", 0);
    temperatureCorrection = preferences.getInt("p2-tempCorrec", 0);
    colonType = preferences.getInt("p2-colonType", 0);
    ColorChangeFrequency = preferences.getInt("p2-ClrChaFreq", 0);
    scrollText = preferences.getString("p2-scrollText", "PrESEt 2");
    clockDisplayType = preferences.getInt("p2-clkDisType", 3);
    dateDisplayType = preferences.getInt("p2-datDisType", 5);
    colorchangeCD = preferences.getBool("p2-clrchanCD", 1);
    useAudibleAlarm = preferences.getBool("p2-alarmCD", 0);
    randomSpectrumMode = preferences.getBool("p2-randSpecMode", 0);
    spectrumMode = preferences.getInt("p2-spectrumMode", 0);
    realtimeMode = preferences.getInt("p2-realtmMode", 0);
    spectrumColorSettings = preferences.getInt("p2-spectrumClr", 2);
    spectrumBackgroundSettings = preferences.getInt("p2-spectrumBkg", 0);
    spotlightsColorSettings = preferences.getInt("p2-spotlColor", 0);
    brightness = preferences.getInt("p2-brightness", 10);
    useSpotlights = preferences.getBool("p2-useSL", 1);
    scrollColorSettings = preferences.getInt("p2-scrolClrSet", 0);
    scrollFrequency = preferences.getInt("p2-scrollFreq", 1);
    scrollOverride = preferences.getBool("p2-scrollOvRd", 1);
    scrollOptions1 = preferences.getBool("p2-scrollOpts1", 0);
    scrollOptions2 = preferences.getBool("p2-scrollOpts2", 0);
    scrollOptions3 = preferences.getBool("p2-scrollOpts3", 0);
    scrollOptions4 = preferences.getBool("p2-scrollOpts4", 0);
    scrollOptions5 = preferences.getBool("p2-scrollOpts5", 0);
    scrollOptions6 = preferences.getBool("p2-scrollOpts6", 0);
    scrollOptions7 = preferences.getBool("p2-scrollOpts7", 0);
    scrollOptions8 = preferences.getBool("p2-scrollOpts8", 0);
    lightshowMode = preferences.getInt("p2-lsMode", 0);
    suspendFrequency = preferences.getInt("p2-suspendFreq", 1);
    suspendType = preferences.getInt("p2-suspendType", 0);
}

void setpreset2(){
  preferences.putLong("p2-gmtOffset", gmtOffset_sec);
  preferences.putBool("p2-DSTime", DSTime);
  preferences.putInt("p2-cd_b_val", cd_b_val);
  preferences.putInt("p2-cd_g_val", cd_g_val);
  preferences.putInt("p2-cd_r_val", cd_r_val);
  preferences.putInt("p2-b0_val", b0_val);
  preferences.putInt("p2-b1_val", b1_val);
  preferences.putInt("p2-b2_val", b2_val);
  preferences.putInt("p2-b3_val", b3_val);
  preferences.putInt("p2-b4_val", b4_val);
  preferences.putInt("p2-b5_val", b5_val);
  preferences.putInt("p2-b6_val", b6_val);
  preferences.putInt("p2-b7_val", b7_val);
  preferences.putInt("p2-b8_val", b8_val); 
  preferences.putInt("p2-b9_val", b9_val);
  preferences.putInt("p2-b10_val", b10_val);
  preferences.putInt("p2-b11_val", b11_val);
  preferences.putInt("p2-b12_val", b12_val);
  preferences.putInt("p2-b13_val", b13_val);
  preferences.putInt("p2-b14_val", b14_val);
  preferences.putInt("p2-b15_val", b15_val);
  preferences.putInt("p2-b16_val", b16_val);
  preferences.putInt("p2-b17_val", b17_val);
  preferences.putInt("p2-g0_val", g0_val);
  preferences.putInt("p2-g1_val", g1_val);
  preferences.putInt("p2-g2_val", g2_val);
  preferences.putInt("p2-g3_val", g3_val);
  preferences.putInt("p2-g4_val", g4_val);
  preferences.putInt("p2-g5_val", g5_val);
  preferences.putInt("p2-g6_val", g6_val);
  preferences.putInt("p2-g7_val", g7_val);
  preferences.putInt("p2-g8_val", g8_val);
  preferences.putInt("p2-g9_val", g9_val);
  preferences.putInt("p2-g10_val", g10_val);
  preferences.putInt("p2-g11_val", g11_val);
  preferences.putInt("p2-g12_val", g12_val);
  preferences.putInt("p2-g13_val", g13_val);
  preferences.putInt("p2-g14_val", g14_val);
  preferences.putInt("p2-g15_val", g15_val);
  preferences.putInt("p2-g16_val", g16_val);
  preferences.putInt("p2-g17_val", g17_val);
  preferences.putInt("p2-r0_val", r0_val);
  preferences.putInt("p2-r1_val", r1_val);
  preferences.putInt("p2-r2_val", r2_val);
  preferences.putInt("p2-r3_val", r3_val);
  preferences.putInt("p2-r4_val", r4_val);
  preferences.putInt("p2-r5_val", r5_val);
  preferences.putInt("p2-r6_val", r6_val);
  preferences.putInt("p2-r7_val", r7_val);
  preferences.putInt("p2-r8_val", r8_val);
  preferences.putInt("p2-r9_val", r9_val);
  preferences.putInt("p2-r10_val", r10_val);
  preferences.putInt("p2-r11_val", r11_val);
  preferences.putInt("p2-r12_val", r12_val);
  preferences.putInt("p2-r13_val", r13_val);
  preferences.putInt("p2-r14_val", r14_val);
  preferences.putInt("p2-r15_val", r15_val);
  preferences.putInt("p2-r16_val", r16_val);
  preferences.putInt("p2-r17_val", r17_val);
  preferences.putInt("p2-clockMode", clockMode);
  preferences.putInt("p2-pastelColors", pastelColors);
  preferences.putInt("p2-temperatSym", temperatureSymbol);
  preferences.putInt("p2-CckColrSet", ClockColorSettings);
  preferences.putInt("p2-DateColorSet", DateColorSettings);
  preferences.putInt("p2-tempColorSet", tempColorSettings);
  preferences.putInt("p2-humiColorSet", humiColorSettings);
  preferences.putInt("p2-tempDisType", tempDisplayType);
  preferences.putInt("p2-humiDisType", humiDisplayType);
  preferences.putInt("p2-tempCorrec", temperatureCorrection);
  preferences.putInt("p2-colonType", colonType);
  preferences.putInt("p2-ClrChaFreq", ColorChangeFrequency);
  preferences.putString("p2-scrollText", scrollText);
  preferences.putInt("p2-clkDisType", clockDisplayType);
  preferences.putInt("p2-datDisType", dateDisplayType);
  preferences.putBool("p2-clrchanCD", colorchangeCD);
  preferences.putBool("p2-alarmCD", useAudibleAlarm);
  preferences.putBool("p2-randSpecMode", randomSpectrumMode);
  preferences.putInt("p2-spectrumMode", spectrumMode); 
  preferences.putInt("p2-realtmMode", realtimeMode);  
  preferences.putInt("p2-spectrumClr", spectrumColorSettings);
  preferences.putInt("p2-spectrumBkg", spectrumBackgroundSettings);
  preferences.putInt("p2-spotlColor", spotlightsColorSettings);
  preferences.putInt("p2-brightness", brightness);
  preferences.putBool("p2-useSL", useSpotlights);
  preferences.putInt("p2-scrolClrSet", scrollColorSettings);
  preferences.putInt("p2-scrollFreq", scrollFrequency);
  preferences.putBool("p2-scrollOvRd", scrollOverride);
  preferences.putBool("p2-scrollOpts1", scrollOptions1);
  preferences.putBool("p2-scrollOpts2", scrollOptions2);
  preferences.putBool("p2-scrollOpts3", scrollOptions3);
  preferences.putBool("p2-scrollOpts4", scrollOptions4);
  preferences.putBool("p2-scrollOpts5", scrollOptions5);
  preferences.putBool("p2-scrollOpts6", scrollOptions6);
  preferences.putBool("p2-scrollOpts7", scrollOptions7);
  preferences.putBool("p2-scrollOpts8", scrollOptions8);
  preferences.putInt("p2-lsMode", lightshowMode); 
  preferences.putInt("p2-suspendFreq", suspendFrequency);
  preferences.putInt("p2-suspendType", suspendType);

}


void loadWebPageHandlers() {
 

// Index.html Webpage Handlers


  server.on("/goClockMode", HTTP_POST, []() {     
    allBlank();  
    clockMode = 0; 
    preferences.putInt("clockMode", clockMode);   
    realtimeMode = 0;   
    preferences.putInt("realtimeMode", realtimeMode);  
    server.send(200, "text/json", "{\"result\":\"ok\"}");
    printLocalTime(); 
  }); 

  server.on("/goCountdownMode", HTTP_POST, []() {    
    countdownMilliSeconds = server.arg("ms").toInt();     
    if (countdownMilliSeconds < 1000) {countdownMilliSeconds = 1000;}
    if (countdownMilliSeconds > 86400000) {countdownMilliSeconds = 86400000;} 
    endCountDownMillis = millis() + countdownMilliSeconds;
    if (currentMode == 0) {currentMode = clockMode; currentReal = realtimeMode;}
    allBlank(); 
    clockMode = 1;  
    //preferences.putInt("clockMode", clockMode);  
    realtimeMode = 0;   
    //preferences.putInt("realtimeMode", realtimeMode);  
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });

  server.on("/goTemperatureMode", HTTP_POST, []() {   
    allBlank();
    clockMode = 2;    
    preferences.putInt("clockMode", clockMode); 
    realtimeMode = 0;   
    preferences.putInt("realtimeMode", realtimeMode);  
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });  

  server.on("/goScoreboardMode", HTTP_POST, []() {   
    scoreboardLeft = server.arg("left").toInt();
    if (scoreboardLeft < 0) {scoreboardLeft = 0;}
    if (scoreboardLeft > 99) {scoreboardLeft = 99;}
    scoreboardRight = server.arg("right").toInt();
    if (scoreboardRight < 0) {scoreboardRight = 0;}
    if (scoreboardRight > 99) {scoreboardRight = 99;}
    allBlank();
    clockMode = 3;   
    preferences.putInt("clockMode", clockMode);  
    realtimeMode = 0;   
    preferences.putInt("realtimeMode", realtimeMode);  
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });  
   
  server.on("/goStopwatchMode", HTTP_POST, []() {    
    CountUpMillis = millis(); // TODO: set starting condition properly
    countdownMilliSeconds = server.arg("ms").toInt();    
    if (countdownMilliSeconds < 1000) {countdownMilliSeconds = 1000;}
    if (countdownMilliSeconds > 86400000) {countdownMilliSeconds = 86400000;} 
    endCountDownMillis = millis() + countdownMilliSeconds;
    if (currentMode == 0) {currentMode = clockMode; currentReal = realtimeMode;}
    allBlank(); 
    clockMode = 4;    
    //preferences.putInt("clockMode", clockMode); 
    realtimeMode = 0;   
    //preferences.putInt("realtimeMode", realtimeMode);  
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });
  
  server.on("/goLightshowMode", HTTP_POST, []() {  
    allBlank(); 
    lightshowMode = server.arg("lightshowMode").toInt();
    oldsnakecolor = CRGB::Green;
    getSlower = 180;
    clockMode = 5;    
    preferences.putInt("lightshowMode", lightshowMode); 
    preferences.putInt("clockMode", clockMode); 
    realtimeMode = 1;   
    preferences.putInt("realtimeMode", realtimeMode);  
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  }); 
  
  server.on("/goDateMode", HTTP_POST, []() {     
    allBlank();   
    clockMode = 7;     
    preferences.putInt("clockMode", clockMode);
    realtimeMode = 0;   
    preferences.putInt("realtimeMode", realtimeMode);  
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  }); 
  
  server.on("/goHumidityMode", HTTP_POST, []() {     
    allBlank();   
    clockMode = 8;     
    preferences.putInt("clockMode", clockMode);
    realtimeMode = 0;   
    preferences.putInt("realtimeMode", realtimeMode);  
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  }); 

  server.on("/goSpectrumMode", HTTP_POST, []() {  
    allBlank(); 
    spectrumMode = server.arg("spectrumMode").toInt();
    clockMode = 9;    
    preferences.putInt("spectrumMode", spectrumMode); 
    preferences.putInt("clockMode", clockMode); 
    realtimeMode = 1;   
    preferences.putInt("realtimeMode", realtimeMode);  
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  }); 
  
  server.on("/goDisplayOffMode", HTTP_POST, []() {     
    allBlank();   
    clockMode = 10;     
    preferences.putInt("clockMode", clockMode);
    realtimeMode = 0;   
    preferences.putInt("realtimeMode", realtimeMode);  
    server.send(200, "text/json", "{\"result\":\"ok\"}");
    rtttl::stop();
    breakOutSet = 1;   
  }); 

  server.on("/goScrollingMode", HTTP_POST, []() {     
    allBlank();   
    clockMode = 11;     
    preferences.putInt("clockMode", clockMode);
    realtimeMode = 0;   
    preferences.putInt("realtimeMode", realtimeMode);  
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  }); 

// Get/Load Presets Handlers
  server.on("/getPreset1", HTTP_POST, []() {   
    getpreset1();   
    GetBrightnessLevel();        
    allBlank(); 
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });
  
  server.on("/getPreset2", HTTP_POST, []() {   
    getpreset2();   
    GetBrightnessLevel();        
    allBlank(); 
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });

// Settings.html Webpage Handlers

// Spotlight Settings
 
  
  server.on("/updateuseSpotlights", HTTP_POST, []() {   
    if ( server.arg("useSpotlights") == "true") {useSpotlights = 1;}
    if ( server.arg("useSpotlights") == "false") {useSpotlights = 0;}
    preferences.putBool("useSpotlights", useSpotlights);
    ShelfDownLights(); 
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });

// Clock Mode Settings    
  server.on("/updateDSTime", HTTP_POST, []() {   
    if ( server.arg("DSTime") == "true") {DSTime = 1;}
    if ( server.arg("DSTime") == "false") {DSTime = 0;}
    configTime(gmtOffset_sec, (daylightOffset_sec * DSTime), ntpServer);
    if(!getLocalTime(&timeinfo)){Serial.println("Error, no NTP Server found!");}
    int tempyear = (timeinfo.tm_year +1900);
    int tempmonth = (timeinfo.tm_mon + 1);
    rtc.adjust(DateTime(tempyear, tempmonth, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
    preferences.putBool("DSTime", DSTime);
    if (clockMode == 0) { allBlank(); } 
    printLocalTime(); 
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });
  

// Countdown/Stopwatch Mode Settings 
  server.on("/updatecolorchangeCD", HTTP_POST, []() {   
    if ( server.arg("colorchangeCD") == "true") {colorchangeCD = 1;}
    if ( server.arg("colorchangeCD") == "false") {colorchangeCD = 0;}
    preferences.putBool("colorchangeCD", colorchangeCD);
    if ((clockMode == 1) || (clockMode == 4)) { allBlank(); } 
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });
  
  server.on("/updatealarmCD", HTTP_POST, []() {   
    if ( server.arg("alarmCD") == "true") {useAudibleAlarm = 1;}
    if ( server.arg("alarmCD") == "false") {useAudibleAlarm = 0;}
    preferences.putBool("alarmCD", useAudibleAlarm);
    if ((clockMode == 1) || (clockMode == 4)) { allBlank(); } 
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });
 

//Spectrum Mode Settings
  server.on("/updaterandomSpectrumMode", HTTP_POST, []() {   
    if ( server.arg("randomSpectrumMode") == "true") {randomSpectrumMode = 1;}
    if ( server.arg("randomSpectrumMode") == "false") {randomSpectrumMode = 0;}
    preferences.putBool("randSpecMode", randomSpectrumMode);
    if (clockMode == 9) { allBlank(); } 
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });
  

  
// Scrolling-text Mode settings
  server.on("/updatescrollOverride", HTTP_POST, []() {   
    if ( server.arg("scrollOverride") == "true") {scrollOverride = 1;}
    if ( server.arg("scrollOverride") == "false") {scrollOverride = 0;}
    preferences.putBool("scrollOverride", scrollOverride);
    if (clockMode == 11) { allBlank(); } 
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });
  
  server.on("/updatescrollOptions1", HTTP_POST, []() {   
    if ( server.arg("scrollOptions1") == "true") {scrollOptions1 = 1;}
    if ( server.arg("scrollOptions1") == "false") {scrollOptions1 = 0;}
    preferences.putBool("scrollOptions1", scrollOptions1);
    if (clockMode == 11) { allBlank(); } 
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });
  
  server.on("/updatescrollOptions2", HTTP_POST, []() {   
    if ( server.arg("scrollOptions2") == "true") {scrollOptions2 = 1;}
    if ( server.arg("scrollOptions2") == "false") {scrollOptions2 = 0;}
    preferences.putBool("scrollOptions2", scrollOptions2);
    if (clockMode == 11) { allBlank(); } 
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });
  
  server.on("/updatescrollOptions3", HTTP_POST, []() {   
    if ( server.arg("scrollOptions3") == "true") {scrollOptions3 = 1;}
    if ( server.arg("scrollOptions3") == "false") {scrollOptions3 = 0;}
    preferences.putBool("scrollOptions3", scrollOptions3);
    if (clockMode == 11) { allBlank(); } 
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });
  
  server.on("/updatescrollOptions4", HTTP_POST, []() {   
    if ( server.arg("scrollOptions4") == "true") {scrollOptions4 = 1;}
    if ( server.arg("scrollOptions4") == "false") {scrollOptions4 = 0;}
    preferences.putBool("scrollOptions4", scrollOptions4);
    if (clockMode == 11) { allBlank(); } 
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });
  
  server.on("/updatescrollOptions5", HTTP_POST, []() {   
    if ( server.arg("scrollOptions5") == "true") {scrollOptions5 = 1;}
    if ( server.arg("scrollOptions5") == "false") {scrollOptions5 = 0;}
    preferences.putBool("scrollOptions5", scrollOptions5);
    if (clockMode == 11) { allBlank(); } 
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });
  
  server.on("/updatescrollOptions6", HTTP_POST, []() {   
    if ( server.arg("scrollOptions6") == "true") {scrollOptions6 = 1;}
    if ( server.arg("scrollOptions6") == "false") {scrollOptions6 = 0;}
    preferences.putBool("scrollOptions6", scrollOptions6);
    if (clockMode == 11) { allBlank(); } 
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });
  
  server.on("/updatescrollOptions7", HTTP_POST, []() {   
    if ( server.arg("scrollOptions7") == "true") {scrollOptions7 = 1;}
    if ( server.arg("scrollOptions7") == "false") {scrollOptions7 = 0;}
    preferences.putBool("scrollOptions7", scrollOptions7);
    if (clockMode == 11) { allBlank(); } 
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });
  
  server.on("/updatescrollOptions8", HTTP_POST, []() {   
    if ( server.arg("scrollOptions8") == "true") {scrollOptions8 = 1;}
    if ( server.arg("scrollOptions8") == "false") {scrollOptions8 = 0;}
    preferences.putBool("scrollOptions8", scrollOptions8);
    if (clockMode == 11) { allBlank(); } 
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });
  
  server.on("/updatescrollText", HTTP_POST, []() {   
    scrollText = server.arg("scrollText");
    if ( scrollText == "") {scrollText = "dAdS ArE tHE bESt";}
    preferences.putString("scrollText", scrollText);
    if (clockMode == 11) { allBlank(); } 
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });


// Save Preset Handles
  server.on("/setpreset1", HTTP_POST, []() {   
    setpreset1();   
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });
  
  server.on("/setpreset2", HTTP_POST, []() {   
    setpreset2();   
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });

  server.on("/gethome", []() {
    DynamicJsonDocument json(500);
    String output;

    json["scoreboardLeft"] = scoreboardLeft;
    json["scoreboardRight"] = scoreboardRight;

    serializeJson(json, output);
    server.send(200, "application/json", output);
  });

  server.on("/getsettings", []() {
    DynamicJsonDocument json(1500);
    String output;
    char spotlightcolor[8];
    char colorHour[8], colorMin[8], colorColon[8];
    char dayColor[8], monthColor[8], separatorColor[8];
    char tempColor[8], typeColor[8], degreeColor[8];
    char humiColor[8], humiDecimalColor[8], humiSymbolColor[8];
    char colorCD[8], scoreboardColorLeft[8], scoreboardColorRight[8];
    char spectrumColor[8], spectrumBackgroundColor[8], scrollingColor[8];

    json["ColorPalette"] = pastelColors;
    json["ColorChangeFrequency"] = ColorChangeFrequency;
    json["rangeBrightness"] = brightness;
    json["suspendType"] = suspendType;
    json["suspendFrequency"] = suspendFrequency;
    json["useSpotlights"] = useSpotlights;
    
    sprintf(spotlightcolor, "#%02X%02X%02X", r0_val, g0_val, b0_val);
    json["spotlightcolor"] = spotlightcolor;

    json["spotlightsColorSettings"] = spotlightsColorSettings;
    json["ClockDisplayType"] = clockDisplayType;
    json["ColonType"] = colonType;
    json["TimezoneSetting"] = gmtOffset_sec;
    json["DSTime"] = DSTime;
    json["ClockColorSettings"] = ClockColorSettings;

    sprintf(colorHour, "#%02X%02X%02X", r1_val, g1_val, b1_val);
    json["colorHour"] = colorHour;
    sprintf(colorMin, "#%02X%02X%02X", r2_val, g2_val, b2_val);
    json["colorMin"] = colorMin;
    sprintf(colorColon, "#%02X%02X%02X", r3_val, g3_val, b3_val);
    json["colorColon"] = colorColon;

    json["DateDisplayType"] = dateDisplayType;
    json["DateColorSettings"] = DateColorSettings;

    sprintf(dayColor, "#%02X%02X%02X", r4_val, g4_val, b4_val);
    json["dayColor"] = dayColor;
    sprintf(monthColor, "#%02X%02X%02X", r5_val, g5_val, b5_val);
    json["monthColor"] = monthColor;
    sprintf(separatorColor, "#%02X%02X%02X", r6_val, g6_val, b6_val);
    json["separatorColor"] = separatorColor;

    json["TempType"] = temperatureSymbol;
    json["CorrectionSelect"] = temperatureCorrection;
    json["TempDisplayType"] = tempDisplayType;
    json["TempColorSettings"] = tempColorSettings;

    sprintf(tempColor, "#%02X%02X%02X", r7_val, g7_val, b7_val);
    json["TempColor"] = tempColor;
    sprintf(typeColor, "#%02X%02X%02X", r8_val, g8_val, b8_val);
    json["TypeColor"] = typeColor;
    sprintf(degreeColor, "#%02X%02X%02X", r9_val, g9_val, b9_val);
    json["DegreeColor"] = degreeColor;

    json["HumiDisplayType"] = humiDisplayType;
    json["HumiColorSettings"] = humiColorSettings;

    sprintf(humiColor, "#%02X%02X%02X", r10_val, g10_val, b10_val);
    json["HumiColor"] = humiColor;
    sprintf(humiDecimalColor, "#%02X%02X%02X", r11_val, g11_val, b11_val);
    json["HumiDecimalColor"] = humiDecimalColor;
    sprintf(humiSymbolColor, "#%02X%02X%02X", r12_val, g12_val, b12_val);
    json["HumiSymbolColor"] = humiSymbolColor;
    json["useAudibleAlarm"] = useAudibleAlarm;
    json["colorchangeCD"] = colorchangeCD;

    sprintf(colorCD, "#%02X%02X%02X", cd_r_val, cd_g_val, cd_b_val);
    json["colorCD"] = colorCD;
    sprintf(scoreboardColorLeft, "#%02X%02X%02X", r13_val, g13_val, b13_val);
    json["scoreboardColorLeft"] = scoreboardColorLeft;
    sprintf(scoreboardColorRight, "#%02X%02X%02X", r14_val, g14_val, b14_val);
    json["scoreboardColorRight"] = scoreboardColorRight;

    json["randomSpectrumMode"] = randomSpectrumMode;
    sprintf(spectrumColor, "#%02X%02X%02X", r15_val, g15_val, b15_val);
    json["spectrumColor"] = spectrumColor;
    sprintf(spectrumBackgroundColor, "#%02X%02X%02X", r17_val, g17_val, b17_val);
    json["spectrumBackgroundColor"] = spectrumBackgroundColor;

    json["spectrumBackgroundSettings"] = spectrumBackgroundSettings;
    json["spectrumColorSettings"] = spectrumColorSettings;
    json["scrollFrequency"] = scrollFrequency;
    json["scrollOptions1"] = scrollOptions1;
    json["scrollOptions2"] = scrollOptions2;
    json["scrollOptions3"] = scrollOptions3;
    json["scrollOptions4"] = scrollOptions4;
    json["scrollOptions5"] = scrollOptions5;
    json["scrollOptions6"] = scrollOptions6;
    json["scrollOptions7"] = scrollOptions7;
    json["scrollOptions8"] = scrollOptions8;
    json["scrollText"] = scrollText;
    json["scrollOverride"] = scrollOverride;

    sprintf(scrollingColor, "#%02X%02X%02X", r16_val, g16_val, b16_val);
    json["scrollColor"] = scrollingColor;

    json["scrollColorSettings"] = scrollColorSettings;
     
    serializeJson(json, output);
    server.send(200, "application/json", output);

  });

  server.on("/updateanything", HTTP_POST, []() {
    DynamicJsonDocument json(1500);
    if(server.args() > 0) {
      String body = server.arg(0);
      Serial.println("----");
      Serial.println(body);
      Serial.println("----");
      deserializeJson(json, server.arg(0));

      // ColorPalette
      if (!json["ColorPalette"].isNull()){
        pastelColors = (int)json["ColorPalette"];
        preferences.putInt("pastelColors", pastelColors);
        allBlank();
      }

      // SuspendType
      if (!json["suspendType"].isNull()) {
        suspendType = (int)json["suspendType"];
        preferences.putInt("suspendType", suspendType);
      }

      // spotlightsColorSettings
      if (!json["spotlightsColorSettings"].isNull()) {
        spotlightsColorSettings = (int)json["spotlightsColorSettings"];
        preferences.putInt("spotlightsCoSe", spotlightsColorSettings);
        ShelfDownLights(); 
      }

      // ClockDisplayType
      if (!json["ClockDisplayType"].isNull()) {
        clockDisplayType = (int)json["ClockDisplayType"];
        preferences.putInt("clockDispType", clockDisplayType);
        if (clockMode == 0) { allBlank(); }  
      }

      // ColonType
      if (!json["ColonType"].isNull()) {
        colonType = (int)json["ColonType"];
        preferences.putInt("colonType", colonType);
        if (clockMode == 0) { allBlank(); } 
      }

      // ClockColorSettings
      if (!json["ClockColorSettings"].isNull()) {
        ClockColorSettings = (int)json["ClockColorSettings"];
        preferences.putInt("ClockColorSet", ClockColorSettings);
        if (clockMode == 0) { allBlank(); } 
      }

      // DateDisplayType
      if (!json["DateDisplayType"].isNull()) {
        ClockColorSettings = (int)json["DateDisplayType"];
        preferences.putInt("dateDisplayType", dateDisplayType);
        if (clockMode == 7) { allBlank(); }
      }

      // DateColorSettings
      if (!json["DateColorSettings"].isNull()) {
        DateColorSettings = (int)json["DateColorSettings"];
        preferences.putInt("DateColorSet", DateColorSettings);
        if (clockMode == 7) { allBlank(); } 
      }

      // TempType
      if (!json["TempType"].isNull()) {
        temperatureSymbol = (int)json["TempType"];
        preferences.putInt("temperatureSym", temperatureSymbol);
        if (clockMode == 2) { allBlank(); } 
      }

      // TempDisplayType
      if (!json["TempDisplayType"].isNull()) {
        tempDisplayType = (int)json["TempDisplayType"];
        preferences.putInt("tempDisplayType", tempDisplayType);
        if (clockMode == 2) { allBlank(); } 
      }

      // TempColorSettings
      if (!json["TempColorSettings"].isNull()) {
        tempColorSettings = (int)json["TempColorSettings"];
        preferences.putInt("tempColorSet", tempColorSettings);
        if (clockMode == 2) { allBlank(); }
      }

      // HumiDisplayType
      if (!json["HumiDisplayType"].isNull()) {
        humiDisplayType = (int)json["HumiDisplayType"];
        preferences.putInt("humiDisplayType", humiDisplayType);
        if (clockMode == 8) { allBlank(); } 
      }

      // HumiColorSettings
      if (!json["HumiColorSettings"].isNull()) {
        humiColorSettings = (int)json["HumiColorSettings"];
        preferences.putInt("humiColorSet", humiColorSettings);
        if (clockMode == 8) { allBlank(); } 
      }

      // spectrumBackgroundSettings
      if (!json["spectrumBackgroundSettings"].isNull()) {
        spectrumBackgroundSettings = (int)json["spectrumBackgroundSettings"];
        preferences.putInt("spectrumBkgd", spectrumBackgroundSettings);
        if (clockMode == 9) { allBlank(); } 
      }

      // spectrumColorSettings
      if (!json["spectrumColorSettings"].isNull()) {
        spectrumColorSettings = (int)json["spectrumColorSettings"];
        preferences.putInt("spectrumColor", spectrumColorSettings);
        if (clockMode == 9) { allBlank(); } 
      }

      // scrollColorSettings
      if (!json["scrollColorSettings"].isNull()) {
        scrollColorSettings = (int)json["scrollColorSettings"];
        preferences.putInt("scrollColorSet", scrollColorSettings);
        if (clockMode == 11) { allBlank(); } 
      }

      // ColorChangeFrequency
      if (!json["ColorChangeFrequency"].isNull()) {
        ColorChangeFrequency = (int)json["ColorChangeFrequency"];
        preferences.putInt("ColorChangeFreq", ColorChangeFrequency);
        allBlank(); 
      }

      // suspendFrequency
      if (!json["suspendFrequency"].isNull()) {
        suspendFrequency = (int)json["suspendFrequency"];
        preferences.putInt("suspendFreq", suspendFrequency);
      }

      // TimezoneSetting
      if (!json["TimezoneSetting"].isNull()) {
        gmtOffset_sec = (int)json["TimezoneSetting"];
        configTime(gmtOffset_sec, (daylightOffset_sec * DSTime), ntpServer);
        if(!getLocalTime(&timeinfo)){Serial.println("Error, no NTP Server found!");}
        int tempyear = (timeinfo.tm_year +1900);
        int tempmonth = (timeinfo.tm_mon + 1);
        rtc.adjust(DateTime(tempyear, tempmonth, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
        preferences.putLong("gmtOffset_sec", gmtOffset_sec);
        if (clockMode == 0) { allBlank(); } 
        printLocalTime(); 
      }

      // CorrectionSelect
      if (!json["CorrectionSelect"].isNull()) {
        temperatureCorrection = (int)json["CorrectionSelect"];
        preferences.putInt("tempCorrection", temperatureCorrection);
        if (clockMode == 2) { allBlank(); } 
      }

      // scrollFrequency
      if (!json["scrollFrequency"].isNull()) {
        scrollFrequency = (int)json["scrollFrequency"];
        preferences.putInt("scrollFreq", scrollFrequency);
        if (clockMode == 11) { allBlank(); } 
      }

      // rangeBrightness
      if (!json["rangeBrightness"].isNull()) {
        brightness = (int)json["rangeBrightness"];
        preferences.putInt("brightness", brightness);
        ShelfDownLights();
      }
      
      // spotlightcolor
      if (!json["spotlightcolor"].isNull()) {
        r0_val = (int)json["spotlightcolor"]["r"];
        g0_val = (int)json["spotlightcolor"]["g"];
        b0_val = (int)json["spotlightcolor"]["b"];
        preferences.putInt("r0_val", r0_val);
        preferences.putInt("g0_val", g0_val);
        preferences.putInt("b0_val", b0_val);
        ShelfDownLights();
      }

      // colorHour
      if (!json["colorHour"].isNull()) {
        r1_val = (int)json["colorHour"]["r"];
        g1_val = (int)json["colorHour"]["g"];
        b1_val = (int)json["colorHour"]["b"];
        preferences.putInt("r1_val", r1_val);
        preferences.putInt("g1_val", g1_val);
        preferences.putInt("b1_val", b1_val);
        if (clockMode == 0) { allBlank(); } 
      }

      // colorMin
      if (!json["colorMin"].isNull()) {
        r2_val = (int)json["colorMin"]["r"];
        g2_val = (int)json["colorMin"]["g"];
        b2_val = (int)json["colorMin"]["b"];
        preferences.putInt("r2_val", r2_val);
        preferences.putInt("g2_val", g2_val);
        preferences.putInt("b2_val", b2_val);
        if (clockMode == 0) { allBlank(); } 
      }

      // colorColon
      if (!json["colorColon"].isNull()) {
        r3_val = (int)json["colorColon"]["r"];
        g3_val = (int)json["colorColon"]["g"];
        b3_val = (int)json["colorColon"]["b"];
        preferences.putInt("r3_val", r3_val);
        preferences.putInt("g3_val", g3_val);
        preferences.putInt("b3_val", b3_val);
        if (clockMode == 0) { allBlank(); } 
      }

      // dayColor
      if (!json["dayColor"].isNull()) {
        r4_val = (int)json["dayColor"]["r"];
        g4_val = (int)json["dayColor"]["g"];
        b4_val = (int)json["dayColor"]["b"];
        preferences.putInt("r4_val", r4_val);
        preferences.putInt("g4_val", g4_val);
        preferences.putInt("b4_val", b4_val);
        if (clockMode == 7) { allBlank(); } 
      }

      // monthColor
      if (!json["monthColor"].isNull()) {
        r5_val = (int)json["monthColor"]["r"];
        g5_val = (int)json["monthColor"]["g"];
        b5_val = (int)json["monthColor"]["b"];
        preferences.putInt("r5_val", r5_val);
        preferences.putInt("g5_val", g5_val);
        preferences.putInt("b5_val", b5_val);
        if (clockMode == 7) { allBlank(); } 
      }

      // separatorColor
      if (!json["separatorColor"].isNull()) {
        r6_val = (int)json["separatorColor"]["r"];
        g6_val = (int)json["separatorColor"]["g"];
        b6_val = (int)json["separatorColor"]["b"];
        preferences.putInt("r6_val", r6_val);
        preferences.putInt("g6_val", g6_val);
        preferences.putInt("b6_val", b6_val);
        if (clockMode == 7) { allBlank(); } 
      }
      
      // TempColor
      if (!json["TempColor"].isNull()) {
        r7_val = (int)json["TempColor"]["r"];
        g7_val = (int)json["TempColor"]["g"];
        b7_val = (int)json["TempColor"]["b"];
        preferences.putInt("r7_val", r7_val);
        preferences.putInt("g7_val", g7_val);
        preferences.putInt("b7_val", b7_val);
        if (clockMode == 2) { allBlank(); } 
      }

      // TypeColor
      if (!json["TypeColor"].isNull()) {
        r8_val = (int)json["TypeColor"]["r"];
        g8_val = (int)json["TypeColor"]["g"];
        b8_val = (int)json["TypeColor"]["b"];
        preferences.putInt("r8_val", r8_val);
        preferences.putInt("g8_val", g8_val);
        preferences.putInt("b8_val", b8_val);
        if (clockMode == 2) { allBlank(); } 
      }

      // DegreeColor
      if (!json["DegreeColor"].isNull()) {
        r9_val = (int)json["DegreeColor"]["r"];
        g9_val = (int)json["DegreeColor"]["g"];
        b9_val = (int)json["DegreeColor"]["b"];
        preferences.putInt("r9_val", r9_val);
        preferences.putInt("g9_val", g9_val);
        preferences.putInt("b9_val", b9_val);
        if (clockMode == 2) { allBlank(); } 
      }

      // HumiColor
      if (!json["HumiColor"].isNull()) {
        r10_val = (int)json["HumiColor"]["r"];
        g10_val = (int)json["HumiColor"]["g"];
        b10_val = (int)json["HumiColor"]["b"];
        preferences.putInt("r10_val", r10_val);
        preferences.putInt("g10_val", g10_val);
        preferences.putInt("b10_val", b10_val);
        if (clockMode == 8) { allBlank(); } 
      }

       // HumiDecimalColor
      if (!json["HumiDecimalColor"].isNull()) {
        r11_val = (int)json["HumiDecimalColor"]["r"];
        g11_val = (int)json["HumiDecimalColor"]["g"];
        b11_val = (int)json["HumiDecimalColor"]["b"];
        preferences.putInt("r11_val", r11_val);
        preferences.putInt("g11_val", g11_val);
        preferences.putInt("b11_val", b11_val);
        if (clockMode == 8) { allBlank(); } 
      }

      // HumiSymbolColor
      if (!json["HumiSymbolColor"].isNull()) {
        r12_val = (int)json["HumiSymbolColor"]["r"];
        g12_val = (int)json["HumiSymbolColor"]["g"];
        b12_val = (int)json["HumiSymbolColor"]["b"];
        preferences.putInt("r12_val", r12_val);
        preferences.putInt("g12_val", g12_val);
        preferences.putInt("b12_val", b12_val);
        if (clockMode == 8) { allBlank(); } 
      }

      // colorCD
      if (!json["colorCD"].isNull()) {
        cd_r_val = (int)json["colorCD"]["r"];
        cd_g_val = (int)json["colorCD"]["g"];
        cd_b_val = (int)json["colorCD"]["b"];
        preferences.putInt("cd_r_val", cd_r_val);
        preferences.putInt("cd_g_val", cd_g_val);
        preferences.putInt("cd_b_val", cd_b_val);
        if ((clockMode == 1) || (clockMode == 4)) { allBlank(); } 
      }

      // scoreboardColorLeft
      if (!json["scoreboardColorLeft"].isNull()) {
        r13_val = (int)json["scoreboardColorLeft"]["r"];
        g13_val = (int)json["scoreboardColorLeft"]["g"];
        b13_val = (int)json["scoreboardColorLeft"]["b"];
        preferences.putInt("r13_val", r13_val);
        preferences.putInt("g13_val", g13_val);
        preferences.putInt("b13_val", b13_val);
        if (clockMode == 3) { allBlank(); } 
      }

      // scoreboardColorRight
      if (!json["scoreboardColorRight"].isNull()) {
        r14_val = (int)json["scoreboardColorRight"]["r"];
        g14_val = (int)json["scoreboardColorRight"]["g"];
        b14_val = (int)json["scoreboardColorRight"]["b"];
        preferences.putInt("r14_val", r14_val);
        preferences.putInt("g14_val", g14_val);
        preferences.putInt("b14_val", b14_val);
        if (clockMode == 3) { allBlank(); } 
      }

      // spectrumColor
      if (!json["spectrumColor"].isNull()) {
        r15_val = (int)json["spectrumColor"]["r"];
        g15_val = (int)json["spectrumColor"]["g"];
        b15_val = (int)json["spectrumColor"]["b"];
        preferences.putInt("r15_val", r15_val);
        preferences.putInt("g15_val", g15_val);
        preferences.putInt("b15_val", b15_val);
        if (clockMode == 9) { allBlank(); }
      }

      // scrollColor
      if (!json["scrollColor"].isNull()) {
        r16_val = (int)json["scrollColor"]["r"];
        g16_val = (int)json["scrollColor"]["g"];
        b16_val = (int)json["scrollColor"]["b"];
        preferences.putInt("r16_val", r16_val);
        preferences.putInt("g16_val", g16_val);
        preferences.putInt("b16_val", b16_val);
        if (clockMode == 11) { allBlank(); } 
      }

      // spectrumBackgroundColor
      if (!json["spectrumBackgroundColor"].isNull()) {
        r17_val = (int)json["spectrumBackgroundColor"]["r"];
        g17_val = (int)json["spectrumBackgroundColor"]["g"];
        b17_val = (int)json["spectrumBackgroundColor"]["b"];
        preferences.putInt("r17_val", r17_val);
        preferences.putInt("g17_val", g17_val);
        preferences.putInt("b17_val", b17_val);
        if (clockMode == 9) { allBlank(); } 
      }

      // outdoor temperature
      if (!json["temperature"].isNull()) {
        preferences.getBytes("temperature", &tempConfig, preferences.getBytesLength("temperature"));        
        tempConfig.outdoor_enable = json["temperature"]["outdoor_enable"].isNull() ? tempConfig.outdoor_enable : json["temperature"]["outdoor_enable"];
        if (!json["temperature"]["outdoor_lat"].isNull()) strncpy(tempConfig.outdoor_lat, json["temperature"]["outdoor_lat"], sizeof(tempConfig.outdoor_lat));
        if (!json["temperature"]["outdoor_long"].isNull()) strncpy(tempConfig.outdoor_long, json["temperature"]["outdoor_long"], sizeof(tempConfig.outdoor_long));
        if (!json["temperature"]["outdoor_apikey"].isNull()) strncpy(tempConfig.outdoor_apikey, json["temperature"]["outdoor_apikey"], sizeof(tempConfig.outdoor_apikey));
      }
      server.send(200, "text/json", "{\"result\":\"ok\"}");
    } 
    server.send(401);
  });


// Manually update time from browser

  server.on("/setdate", HTTP_POST, []() { 
    int yeararg = server.arg("year").toInt();
    int montharg = server.arg("month").toInt();
    int dayarg = server.arg("day").toInt();
    int hourarg = server.arg("hour").toInt();
    int minarg = server.arg("min").toInt();
    int secarg = server.arg("sec").toInt();
    rtc.adjust(DateTime(yeararg, montharg, dayarg, hourarg, minarg, secarg));   //set time on the RTC of the DS3231
    struct tm tm;
    tm.tm_year = yeararg - 1900;
    tm.tm_mon = montharg - 1;
    tm.tm_mday = dayarg;
    tm.tm_hour = hourarg;
    tm.tm_min = minarg;
    tm.tm_sec = secarg;
    time_t t = mktime(&tm);
    struct timeval now1 = { .tv_sec = t };
    settimeofday(&now1, NULL);    //set time on the RTC of the ESP32
    printLocalTime(); 
    server.send(200, "text/json", "{\"result\":\"ok\"}");
  });




//debug page
   server.on("/debugpage", []() {    
    char webString[4096]="";  //uses about 3600 characters at startup without color info
    char tempString[255]="";
    char tempRTC[64]="";
    char tempRTCE[64]="";
    DateTime now = rtc.now();
    char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    char monthsOfTheYear[12][12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
    int humidTemp = dht.readHumidity();        // read humidity
    float sensorTemp = dht.readTemperature();     // read temperature
    //DS3231 RTC    
    sprintf(tempString, "%s", daysOfTheWeek[now.dayOfTheWeek()]);strcpy(tempRTC, tempString);
    strcat(tempRTC, ", ");
    sprintf(tempString, "%s", monthsOfTheYear[now.month()-1]);strcat(tempRTC, tempString);
    strcat(tempRTC, " ");
    sprintf(tempString, "%02d", now.day());strcat(tempRTC, tempString);
    strcat(tempRTC, " ");
    sprintf(tempString, "%d", now.year());strcat(tempRTC, tempString);
    strcat(tempRTC, " ");
    sprintf(tempString, "%02d", now.hour());strcat(tempRTC, tempString);
    strcat(tempRTC, ":");
    sprintf(tempString, "%02d", now.minute());strcat(tempRTC, tempString);
    strcat(tempRTC, ":");
    sprintf(tempString, "%02d", now.second());strcat(tempRTC, tempString);
    //ESP32 RTC
    sprintf(tempString, "%s", daysOfTheWeek[timeinfo.tm_wday]);strcpy(tempRTCE, tempString);
    strcat(tempRTCE, ", ");
    sprintf(tempString, "%s", monthsOfTheYear[timeinfo.tm_mon]);strcat(tempRTCE, tempString);
    strcat(tempRTCE, " ");
    sprintf(tempString, "%02d", timeinfo.tm_mday);strcat(tempRTCE, tempString);
    strcat(tempRTCE, " ");
    sprintf(tempString, "%d", timeinfo.tm_year+1900);strcat(tempRTCE, tempString);
    strcat(tempRTCE, " ");
    sprintf(tempString, "%02d", timeinfo.tm_hour);strcat(tempRTCE, tempString);
    strcat(tempRTCE, ":");
    sprintf(tempString, "%02d", timeinfo.tm_min);strcat(tempRTCE, tempString);
    strcat(tempRTCE, ":");
    sprintf(tempString, "%02d", timeinfo.tm_sec);strcat(tempRTCE, tempString);
    //HTML header
    strcpy(webString, "<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, shrink-to-fit=no\"><script type=\"text/javascript\" src=\"js/jquery-3.5.1.min.js\"></script><script type=\"text/javascript\" src=\"js/bootstrap.min.js\"></script><link rel=\"stylesheet\" href=\"css/bootstrap.min.css\"><title>Shelf Clock</title></head><body><nav class=\"navbar navbar-expand-md navbar-dark bg-dark mb-4\"><a class=\"navbar-brand\" href=\"./index.html\">Shelf Clock</a><button class=\"navbar-toggler\" type=\"button\" data-toggle=\"collapse\" data-target=\"#navbarCollapse\"aria-controls=\"navbarCollapse\" aria-expanded=\"false\" aria-label=\"Toggle navigation\"><span class=\"navbar-toggler-icon\"></span></button><div class=\"collapse navbar-collapse\" id=\"navbarCollapse\"><ul class=\"navbar-nav mr-auto\"><li class=\"nav-item\"><a class=\"nav-link active\" href=\"./index.html\">Home <span class=\"sr-only\">(current)</span></a></li><li class=\"nav-item\"><a class=\"nav-link\" href=\"./settings.html\">Settings</a></li></div></nav><style type=\"text/css\">body {font: .75rem Inconsolata, monospace;}</style>");

    /*
    strcat(webString, "<br>cd_r_val=");sprintf(tempString, "%d", cd_r_val);strcat(webString, tempString);
    strcat(webString, "<br>cd_g_val=");sprintf(tempString, "%d", cd_g_val);strcat(webString, tempString);
    strcat(webString, "<br>cd_b_val=");sprintf(tempString, "%d", cd_b_val);strcat(webString, tempString);
    strcat(webString, "<br>r0_val=");sprintf(tempString, "%d", r0_val);strcat(webString, tempString);
    strcat(webString, "<br>g0_val=");sprintf(tempString, "%d", g0_val);strcat(webString, tempString);
    strcat(webString, "<br>b0_val=");sprintf(tempString, "%d", b0_val);strcat(webString, tempString);
    strcat(webString, "<br>r1_val=");sprintf(tempString, "%d", r1_val);strcat(webString, tempString);
    strcat(webString, "<br>g1_val=");sprintf(tempString, "%d", g1_val);strcat(webString, tempString);
    strcat(webString, "<br>b1_val=");sprintf(tempString, "%d", b1_val);strcat(webString, tempString);
    strcat(webString, "<br>r2_val=");sprintf(tempString, "%d", r2_val);strcat(webString, tempString);
    strcat(webString, "<br>g2_val=");sprintf(tempString, "%d", g2_val);strcat(webString, tempString);
    strcat(webString, "<br>b2_val=");sprintf(tempString, "%d", b2_val);strcat(webString, tempString);
    strcat(webString, "<br>r3_val=");sprintf(tempString, "%d", r3_val);strcat(webString, tempString);
    strcat(webString, "<br>g3_val=");sprintf(tempString, "%d", g3_val);strcat(webString, tempString);
    strcat(webString, "<br>b3_val=");sprintf(tempString, "%d", b3_val);strcat(webString, tempString);
    strcat(webString, "<br>r4_val=");sprintf(tempString, "%d", r4_val);strcat(webString, tempString);
    strcat(webString, "<br>g4_val=");sprintf(tempString, "%d", g4_val);strcat(webString, tempString);
    strcat(webString, "<br>b4_val=");sprintf(tempString, "%d", b4_val);strcat(webString, tempString);
    strcat(webString, "<br>r5_val=");sprintf(tempString, "%d", r5_val);strcat(webString, tempString);
    strcat(webString, "<br>g5_val=");sprintf(tempString, "%d", g5_val);strcat(webString, tempString);
    strcat(webString, "<br>b5_val=");sprintf(tempString, "%d", b5_val);strcat(webString, tempString);
    strcat(webString, "<br>r6_val=");sprintf(tempString, "%d", r6_val);strcat(webString, tempString);
    strcat(webString, "<br>g6_val=");sprintf(tempString, "%d", g6_val);strcat(webString, tempString);
    strcat(webString, "<br>b6_val=");sprintf(tempString, "%d", b6_val);strcat(webString, tempString);
    strcat(webString, "<br>r7_val=");sprintf(tempString, "%d", r7_val);strcat(webString, tempString);
    strcat(webString, "<br>g7_val=");sprintf(tempString, "%d", g7_val);strcat(webString, tempString);
    strcat(webString, "<br>b7_val=");sprintf(tempString, "%d", b7_val);strcat(webString, tempString);
    strcat(webString, "<br>r8_val=");sprintf(tempString, "%d", r8_val);strcat(webString, tempString);
    strcat(webString, "<br>g8_val=");sprintf(tempString, "%d", g8_val);strcat(webString, tempString);
    strcat(webString, "<br>b8_val=");sprintf(tempString, "%d", b8_val);strcat(webString, tempString);
    strcat(webString, "<br>r9_val=");sprintf(tempString, "%d", r9_val);strcat(webString, tempString);
    strcat(webString, "<br>g9_val=");sprintf(tempString, "%d", g9_val);strcat(webString, tempString);
    strcat(webString, "<br>b9_val=");sprintf(tempString, "%d", b9_val);strcat(webString, tempString);
    strcat(webString, "<br>r10_val=");sprintf(tempString, "%d", r10_val);strcat(webString, tempString);
    strcat(webString, "<br>g10_val=");sprintf(tempString, "%d", g10_val);strcat(webString, tempString);
    strcat(webString, "<br>b10_val=");sprintf(tempString, "%d", b10_val);strcat(webString, tempString);
    strcat(webString, "<br>r11_val=");sprintf(tempString, "%d", r11_val);strcat(webString, tempString);
    strcat(webString, "<br>g11_val=");sprintf(tempString, "%d", g11_val);strcat(webString, tempString);
    strcat(webString, "<br>b11_val=");sprintf(tempString, "%d", b11_val);strcat(webString, tempString);
    strcat(webString, "<br>r12_val=");sprintf(tempString, "%d", r12_val);strcat(webString, tempString);
    strcat(webString, "<br>g12_val=");sprintf(tempString, "%d", g12_val);strcat(webString, tempString);
    strcat(webString, "<br>b12_val=");sprintf(tempString, "%d", b12_val);strcat(webString, tempString);
    strcat(webString, "<br>r13_val=");sprintf(tempString, "%d", r13_val);strcat(webString, tempString);
    strcat(webString, "<br>g13_val=");sprintf(tempString, "%d", g13_val);strcat(webString, tempString);
    strcat(webString, "<br>b13_val=");sprintf(tempString, "%d", b13_val);strcat(webString, tempString);
    strcat(webString, "<br>r14_val=");sprintf(tempString, "%d", r14_val);strcat(webString, tempString);
    strcat(webString, "<br>g14_val=");sprintf(tempString, "%d", g14_val);strcat(webString, tempString);
    strcat(webString, "<br>b14_val=");sprintf(tempString, "%d", b14_val);strcat(webString, tempString);
    strcat(webString, "<br>r15_val=");sprintf(tempString, "%d", r15_val);strcat(webString, tempString);
    strcat(webString, "<br>g15_val=");sprintf(tempString, "%d", g15_val);strcat(webString, tempString);
    strcat(webString, "<br>b15_val=");sprintf(tempString, "%d", b15_val);strcat(webString, tempString);
    strcat(webString, "<br>r16_val=");sprintf(tempString, "%d", r16_val);strcat(webString, tempString);
    strcat(webString, "<br>g16_val=");sprintf(tempString, "%d", g16_val);strcat(webString, tempString);
    strcat(webString, "<br>b16_val=");sprintf(tempString, "%d", b16_val);strcat(webString, tempString);
    strcat(webString, "<br>r17_val=");sprintf(tempString, "%d", r17_val);strcat(webString, tempString);
    strcat(webString, "<br>g17_val=");sprintf(tempString, "%d", g17_val);strcat(webString, tempString);
    strcat(webString, "<br>b17_val=");sprintf(tempString, "%d", b17_val);strcat(webString, tempString);
    strcat(webString, "<br>cd_b_val=");sprintf(tempString, "%d", cd_b_val);strcat(webString, tempString);
    strcat(webString, "<br>cd_g_val=");sprintf(tempString, "%d", cd_g_val);strcat(webString, tempString);
    strcat(webString, "<br>cd_r_val=");sprintf(tempString, "%d", cd_r_val);strcat(webString, tempString);
    */

    strcat(webString, "<br>DS-3231 RTC=");strcat(webString, tempRTC);
    strcat(webString, "<br>ESP32-NTP RTC=");strcat(webString, tempRTCE);
    strcat(webString, "<br>DHT11 C Temp=");sprintf(tempString, "%.1f", sensorTemp);strcat(webString, tempString);
    strcat(webString, "<br>DHT11 F Temp=");sprintf(tempString, "%.1f", (sensorTemp*1.8000)+32);strcat(webString, tempString);
    strcat(webString, "<br>DHT11 Humidity=");sprintf(tempString, "%02d", humidTemp);strcat(webString, tempString);
    strcat(webString, "<br>WiFi IP=");sprintf(tempString, "%s", WiFi.localIP().toString().c_str());strcat(webString, tempString);
    strcat(webString, "<br>analogRead(PHOTORESISTER_PIN)=");sprintf(tempString, "%04d", analogRead(PHOTORESISTER_PIN));strcat(webString, tempString);
    strcat(webString, "<br>analogRead(MIC_IN_PIN)=");sprintf(tempString, "%04d", analogRead(MIC_IN_PIN));strcat(webString, tempString);
    strcat(webString, "<br>digitalRead(AUDIO_GATE_PIN)=");sprintf(tempString, "%d", digitalRead(AUDIO_GATE_PIN));strcat(webString, tempString);
    strcat(webString, "<br>AUDIO_GATE_PIN=");sprintf(tempString, "%d", AUDIO_GATE_PIN);strcat(webString, tempString);
    strcat(webString, "<br>averageAudioInput=");sprintf(tempString, "%d", averageAudioInput);strcat(webString, tempString);
    strcat(webString, "<br>breakOutSet=");sprintf(tempString, "%d", breakOutSet);strcat(webString, tempString);
    strcat(webString, "<br>brightness=");sprintf(tempString, "%d", brightness);strcat(webString, tempString);
    strcat(webString, "<br>BUZZER_PIN=");sprintf(tempString, "%d", BUZZER_PIN);strcat(webString, tempString);
    strcat(webString, "<br>clearOldLeds=");sprintf(tempString, "%d", clearOldLeds);strcat(webString, tempString);
    strcat(webString, "<br>ClockColorSettings=");sprintf(tempString, "%d", ClockColorSettings);strcat(webString, tempString);
    strcat(webString, "<br>clockDisplayType=");sprintf(tempString, "%d", clockDisplayType);strcat(webString, tempString);
    strcat(webString, "<br>clockMode=");sprintf(tempString, "%d", clockMode);strcat(webString, tempString);
    strcat(webString, "<br>colonType=");sprintf(tempString, "%d", colonType);strcat(webString, tempString);
    strcat(webString, "<br>colorchangeCD=");sprintf(tempString, "%d", colorchangeCD);strcat(webString, tempString);
    strcat(webString, "<br>ColorChangeFrequency=");sprintf(tempString, "%d", ColorChangeFrequency);strcat(webString, tempString);
    strcat(webString, "<br>colorWheelPosition=");sprintf(tempString, "%d", colorWheelPosition);strcat(webString, tempString);
    strcat(webString, "<br>colorWheelPositionTwo=");sprintf(tempString, "%d", colorWheelPositionTwo);strcat(webString, tempString);
    strcat(webString, "<br>colorWheelSpeed=");sprintf(tempString, "%d", colorWheelSpeed);strcat(webString, tempString);
    strcat(webString, "<br>countdownMilliSeconds=");sprintf(tempString, "%d", countdownMilliSeconds);strcat(webString, tempString);
    strcat(webString, "<br>CountUpMillis=");sprintf(tempString, "%d", CountUpMillis);strcat(webString, tempString);
    strcat(webString, "<br>countupMilliSeconds=");sprintf(tempString, "%d", countupMilliSeconds);strcat(webString, tempString);
    strcat(webString, "<br>currentMode=");sprintf(tempString, "%d", currentMode);strcat(webString, tempString);
    strcat(webString, "<br>currentReal=");sprintf(tempString, "%d", currentReal);strcat(webString, tempString);
    strcat(webString, "<br>cylonPosition=");sprintf(tempString, "%d", cylonPosition);strcat(webString, tempString);
    strcat(webString, "<br>DateColorSettings=");sprintf(tempString, "%d", DateColorSettings);strcat(webString, tempString);
    strcat(webString, "<br>dateDisplayType=");sprintf(tempString, "%d", dateDisplayType);strcat(webString, tempString);
    strcat(webString, "<br>daylightOffset_sec=");sprintf(tempString, "%d", daylightOffset_sec);strcat(webString, tempString);
    strcat(webString, "<br>daysUptime=");sprintf(tempString, "%d", daysUptime);strcat(webString, tempString);
    strcat(webString, "<br>decay_check=");sprintf(tempString, "%d", decay_check);strcat(webString, tempString);
    strcat(webString, "<br>decay=");sprintf(tempString, "%d", decay);strcat(webString, tempString);
    strcat(webString, "<br>DHT_PIN=");sprintf(tempString, "%d", DHT_PIN);strcat(webString, tempString);
    strcat(webString, "<br>DHTTYPE=");sprintf(tempString, "%d", DHTTYPE);strcat(webString, tempString);
    strcat(webString, "<br>dotsOn=");sprintf(tempString, "%d", dotsOn);strcat(webString, tempString);
    strcat(webString, "<br>DSTime=");sprintf(tempString, "%d", DSTime);strcat(webString, tempString);
    strcat(webString, "<br>endCountDownMillis=");sprintf(tempString, "%d", endCountDownMillis);strcat(webString, tempString);
    strcat(webString, "<br>FAKE_NUM_LEDS=");sprintf(tempString, "%d", FAKE_NUM_LEDS);strcat(webString, tempString);
    strcat(webString, "<br>fakeclockrunning=");sprintf(tempString, "%d", fakeclockrunning);strcat(webString, tempString);
    strcat(webString, "<br>foodSpot=");sprintf(tempString, "%d", foodSpot);strcat(webString, tempString);
    strcat(webString, "<br>getSlower=");sprintf(tempString, "%d", getSlower);strcat(webString, tempString);
    strcat(webString, "<br>gmtOffset_sec=");sprintf(tempString, "%d", gmtOffset_sec);strcat(webString, tempString);
    strcat(webString, "<br>host=");sprintf(tempString, "%s", host);strcat(webString, tempString);
    strcat(webString, "<br>humiColorSettings=");sprintf(tempString, "%d", humiColorSettings);strcat(webString, tempString);
    strcat(webString, "<br>humiDisplayType=");sprintf(tempString, "%d", humiDisplayType);strcat(webString, tempString);
    strcat(webString, "<br>isAsleep=");sprintf(tempString, "%d", isAsleep);strcat(webString, tempString);
    strcat(webString, "<br>LED_PIN=");sprintf(tempString, "%d", LED_PIN);strcat(webString, tempString);
    strcat(webString, "<br>LEDS_PER_DIGIT=");sprintf(tempString, "%d", LEDS_PER_DIGIT);strcat(webString, tempString);
    strcat(webString, "<br>LEDS_PER_SEGMENT=");sprintf(tempString, "%d", LEDS_PER_SEGMENT);strcat(webString, tempString);
    strcat(webString, "<br>lightSensorValue=");sprintf(tempString, "%d", lightSensorValue);strcat(webString, tempString);
    strcat(webString, "<br>lightshowMode=");sprintf(tempString, "%d", lightshowMode);strcat(webString, tempString);
    strcat(webString, "<br>lightshowSpeed=");sprintf(tempString, "%d", lightshowSpeed);strcat(webString, tempString);
    strcat(webString, "<br>MIC_IN_PIN=");sprintf(tempString, "%d", MIC_IN_PIN);strcat(webString, tempString);
    strcat(webString, "<br>MILLI_AMPS=");sprintf(tempString, "%d", MILLI_AMPS);strcat(webString, tempString);
    strcat(webString, "<br>ntpServer=");sprintf(tempString, "%s", ntpServer);strcat(webString, tempString);
    strcat(webString, "<br>NUM_LEDS=");sprintf(tempString, "%d", NUM_LEDS);strcat(webString, tempString);
    strcat(webString, "<br>NUMBER_OF_DIGITS=");sprintf(tempString, "%d", NUMBER_OF_DIGITS);strcat(webString, tempString);
    strcat(webString, "<br>pastelColors=");sprintf(tempString, "%d", pastelColors);strcat(webString, tempString);
    strcat(webString, "<br>PHOTO_SAMPLES=");sprintf(tempString, "%d", PHOTO_SAMPLES);strcat(webString, tempString);
    strcat(webString, "<br>photoresisterReadings[0]=");sprintf(tempString, "%d", photoresisterReadings[0]);strcat(webString, tempString);
    strcat(webString, "<br>photoresisterReadings[1]=");sprintf(tempString, "%d", photoresisterReadings[1]);strcat(webString, tempString);
    strcat(webString, "<br>photoresisterReadings[2]=");sprintf(tempString, "%d", photoresisterReadings[2]);strcat(webString, tempString);
    strcat(webString, "<br>photoresisterReadings[3]=");sprintf(tempString, "%d", photoresisterReadings[3]);strcat(webString, tempString);
    strcat(webString, "<br>photoresisterReadings[4]=");sprintf(tempString, "%d", photoresisterReadings[4]);strcat(webString, tempString);
    strcat(webString, "<br>photoresisterReadings[5]=");sprintf(tempString, "%d", photoresisterReadings[5]);strcat(webString, tempString);
    strcat(webString, "<br>photoresisterReadings[6]=");sprintf(tempString, "%d", photoresisterReadings[6]);strcat(webString, tempString);
    strcat(webString, "<br>photoresisterReadings[7]=");sprintf(tempString, "%d", photoresisterReadings[7]);strcat(webString, tempString);
    strcat(webString, "<br>photoresisterReadings[8]=");sprintf(tempString, "%d", photoresisterReadings[8]);strcat(webString, tempString);
    strcat(webString, "<br>photoresisterReadings[9]=");sprintf(tempString, "%d", photoresisterReadings[9]);strcat(webString, tempString);
    strcat(webString, "<br>photoresisterReadings[10]=");sprintf(tempString, "%d", photoresisterReadings[10]);strcat(webString, tempString);
    strcat(webString, "<br>photoresisterReadings[11]=");sprintf(tempString, "%d", photoresisterReadings[11]);strcat(webString, tempString);
    strcat(webString, "<br>photoresisterReadings[12]=");sprintf(tempString, "%d", photoresisterReadings[12]);strcat(webString, tempString);
    strcat(webString, "<br>photoresisterReadings[13]=");sprintf(tempString, "%d", photoresisterReadings[13]);strcat(webString, tempString);
    strcat(webString, "<br>PHOTORESISTER_PIN=");sprintf(tempString, "%d", PHOTORESISTER_PIN);strcat(webString, tempString);
    strcat(webString, "<br>post_react=");sprintf(tempString, "%d", post_react);strcat(webString, tempString);
    strcat(webString, "<br>pre_react=");sprintf(tempString, "%d", pre_react);strcat(webString, tempString);
    strcat(webString, "<br>previousTimeDay=");sprintf(tempString, "%d", previousTimeDay);strcat(webString, tempString);
    strcat(webString, "<br>previousTimeHour=");sprintf(tempString, "%d", previousTimeHour);strcat(webString, tempString);
    strcat(webString, "<br>previousTimeMin=");sprintf(tempString, "%d", previousTimeMin);strcat(webString, tempString);
    strcat(webString, "<br>previousTimeMonth=");sprintf(tempString, "%d", previousTimeMonth);strcat(webString, tempString);
    strcat(webString, "<br>previousTimeWeek=");sprintf(tempString, "%d", previousTimeWeek);strcat(webString, tempString);
    strcat(webString, "<br>prevTime=");sprintf(tempString, "%d", prevTime);strcat(webString, tempString);
    strcat(webString, "<br>prevTime2=");sprintf(tempString, "%d", prevTime2);strcat(webString, tempString);
    strcat(webString, "<br>randomDayPassed=");sprintf(tempString, "%d", randomDayPassed);strcat(webString, tempString);
    strcat(webString, "<br>randomHourPassed=");sprintf(tempString, "%d", randomHourPassed);strcat(webString, tempString);
    strcat(webString, "<br>randomMinPassed=");sprintf(tempString, "%d", randomMinPassed);strcat(webString, tempString);
    strcat(webString, "<br>randomMonthPassed=");sprintf(tempString, "%d", randomMonthPassed);strcat(webString, tempString);
    strcat(webString, "<br>randomSpectrumMode=");sprintf(tempString, "%d", randomSpectrumMode);strcat(webString, tempString);
    strcat(webString, "<br>randomWeekPassed=");sprintf(tempString, "%d", randomWeekPassed);strcat(webString, tempString);
    strcat(webString, "<br>react=");sprintf(tempString, "%d", react);strcat(webString, tempString);
    strcat(webString, "<br>readIndex=");sprintf(tempString, "%d", readIndex);strcat(webString, tempString);
    strcat(webString, "<br>realtimeMode=");sprintf(tempString, "%d", realtimeMode);strcat(webString, tempString);
    strcat(webString, "<br>scoreboardLeft=");sprintf(tempString, "%d", scoreboardLeft);strcat(webString, tempString);
    strcat(webString, "<br>scoreboardRight=");sprintf(tempString, "%d", scoreboardRight);strcat(webString, tempString);
    strcat(webString, "<br>scrollColorSettings=");sprintf(tempString, "%d", scrollColorSettings);strcat(webString, tempString);
    strcat(webString, "<br>scrollFrequency=");sprintf(tempString, "%d", scrollFrequency);strcat(webString, tempString);
    strcat(webString, "<br>scrollOptions1=");sprintf(tempString, "%d", scrollOptions1);strcat(webString, tempString);
    strcat(webString, "<br>scrollOptions2=");sprintf(tempString, "%d", scrollOptions2);strcat(webString, tempString);
    strcat(webString, "<br>scrollOptions3=");sprintf(tempString, "%d", scrollOptions3);strcat(webString, tempString);
    strcat(webString, "<br>scrollOptions4=");sprintf(tempString, "%d", scrollOptions4);strcat(webString, tempString);
    strcat(webString, "<br>scrollOptions5=");sprintf(tempString, "%d", scrollOptions5);strcat(webString, tempString);
    strcat(webString, "<br>scrollOptions6=");sprintf(tempString, "%d", scrollOptions6);strcat(webString, tempString);
    strcat(webString, "<br>scrollOptions7=");sprintf(tempString, "%d", scrollOptions7);strcat(webString, tempString);
    strcat(webString, "<br>scrollOptions8=");sprintf(tempString, "%d", scrollOptions8);strcat(webString, tempString);
    strcat(webString, "<br>scrollOverride=");sprintf(tempString, "%d", scrollOverride);strcat(webString, tempString);
    strcat(webString, "<br>scrollText=");sprintf(tempString, "%s", scrollText.c_str());strcat(webString, tempString);
    strcat(webString, "<br>SEGMENTS_PER_NUMBER=");sprintf(tempString, "%d", SEGMENTS_PER_NUMBER);strcat(webString, tempString);
    strcat(webString, "<br>sleepTimerCurrent=");sprintf(tempString, "%d", sleepTimerCurrent);strcat(webString, tempString);
    strcat(webString, "<br>snakeLastDirection=");sprintf(tempString, "%d", snakeLastDirection);strcat(webString, tempString);
    strcat(webString, "<br>snakePosition=");sprintf(tempString, "%d", snakePosition);strcat(webString, tempString);
    strcat(webString, "<br>snakeWaiting=");sprintf(tempString, "%d", snakeWaiting);strcat(webString, tempString);
    strcat(webString, "<br>SPECTRUM_PIXELS=");sprintf(tempString, "%d", SPECTRUM_PIXELS);strcat(webString, tempString);
    strcat(webString, "<br>spectrumBackgroundSettings=");sprintf(tempString, "%d", spectrumBackgroundSettings);strcat(webString, tempString);
    strcat(webString, "<br>spectrumColorSettings=");sprintf(tempString, "%d", spectrumColorSettings);strcat(webString, tempString);
    strcat(webString, "<br>spectrumMode=");sprintf(tempString, "%d", spectrumMode);strcat(webString, tempString);
    strcat(webString, "<br>spotlightsColorSettings=");sprintf(tempString, "%d", spotlightsColorSettings);strcat(webString, tempString);
    strcat(webString, "<br>suspendFrequency=");sprintf(tempString, "%d", suspendFrequency);strcat(webString, tempString);
    strcat(webString, "<br>suspendType=");sprintf(tempString, "%d", suspendType);strcat(webString, tempString);
    strcat(webString, "<br>tempColorSettings=");sprintf(tempString, "%d", tempColorSettings);strcat(webString, tempString);
    strcat(webString, "<br>tempDisplayType=");sprintf(tempString, "%d", tempDisplayType);strcat(webString, tempString);
    strcat(webString, "<br>temperatureCorrection=");sprintf(tempString, "%d", temperatureCorrection);strcat(webString, tempString);
    strcat(webString, "<br>temperatureSymbol=");sprintf(tempString, "%d", temperatureSymbol);strcat(webString, tempString);
    strcat(webString, "<br>useSpotlights=");sprintf(tempString, "%d", useSpotlights);strcat(webString, tempString);
    strcat(webString, "<br>useAudibleAlarm=");sprintf(tempString, "%d", useAudibleAlarm);strcat(webString, tempString);
    //HTML footer
    strcat(webString, "</body></html>"); 
    server.send(200, "text/html", webString ); 
 });

  
}

  

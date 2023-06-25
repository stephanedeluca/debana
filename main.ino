/**
 * ﷽
 * 
 * @abstract Magic Apps CANbus, RS-485 and UART communications sniffer
 * @file magicAppsCommSniffer.ino
 * @module Magic Apps Comm Sniffer
 *
 * @summary Magic Apps CANbus, RS-485 and UART communications sniffer
 *
 * @Copyright © 2022 Magic Apps sarlau - All rights reserved
 * @author SDL 
 * @created by Sdl on Monday January 2nd 2022 23:42am @ Sdl's in Montreuil, France.
*/
// https://github.com/adafruit/Adafruit_INA219

#include <Wire.h>
#include <Adafruit_INA219.h>


#if false
#define BUZZER_PIN 2
#include <Tone.h>
// ========================================

/// Buzzer
Tone tone;

struct MusicScore {
	const char *name;
	const int tempo;
	const int notesCount;
	const int *notes;
};


class Buzzer {
	private:
	bool playedFullyCharged = false;

	public:
	Buzzer() {
    tone.begin(BUZZER_PIN);
  }

	/// Plays the given melody
	void playMelody(MusicScore melody);

	/// Plays the alarm melody when an attempt to write a config message over RS485
	void playAttemptToWriteConfigMessageToRs485Alarm();
	/// BLE connection jingle
	void playBleConnectedMelody();
	/// BLE disconnection jingle
	void playBleDisconnectedMelody();


  private:
  void tone(unsigned int frequency, unsigned long duration) {
      tone.play(frequency, duration);
  }

  void noTone(uint8_t pin, uint8_t channel) {
      tone.stop();
  }
};

Buzzer buzzer;

/// La la - la la - (alarm)
const int _alarm1 [] = {
	LA2,Two,	LA2,Two,	0,One,	
	LA2,Two,	LA2,Two,	0,One,
	LA2,Two,	LA2,Two,	0,One,
};

const MusicScore alarm1 = MusicScore {
	"Alarm",
	200,
	sizeof(_alarm1)/sizeof(*_alarm1)/2,
	_alarm1
};





/// La do mi (ble connected)
const int _bleConnected [] = {
	LA4,Third,	DO5,Third,	MI5,Third,	
	//MI5,Half,	 MI5,Half,	 
	0,One,
};

const MusicScore bleConnected = MusicScore {
	"Ble connected",
	400,
	sizeof(_bleConnected)/sizeof(*_bleConnected)/2,
	_bleConnected
};




/// La do mi (ble connected)
const int _bleDisconnected [] = {
	MI5,Third,	DO5,Third,	LA4,Third,	
	//LA4,Half,	 LA4,Half,	
	0,One,
};

const MusicScore bleDisconnected = MusicScore {
	"Ble disconnected",
	400,
	sizeof(_bleDisconnected)/sizeof(*_bleDisconnected)/2,
	_bleDisconnected
};

// Daf punk https://musescore.com/user/5024241/scores/4819838
const int _dafpunk [] = {
	MI4,Half, FAd5, Half, LA5, Half, FAd5,Half, DOd6,Half, SI5, Half, LA5, Half, FAd5,Half,
	MI4,Half, FAd5, Half, LA5, Half, FAd5,Half, DOd6,Half, SI5, Half, LA5, Half, FAd5,Half,
	LA4,Half, LA4, Half, FAd4, Half, FAd4,Half, LA4,Half, LA4, Half, FAd4,Half, FAd4,Half,
	LA4,Half, LA4, Half, FAd4, Half, FAd4,Half, LA4,Half, LA4, Half, FAd4,Half, FAd4,Half,
	0,One
	// Left hand
	//SOL4,One, SOL4,One, FA4,One, FA4,One, MI4,One, MI4,One, FAd4,One, FAd4,One, 0,One
};

const MusicScore dafPunk = MusicScore {
	"Dafpunk harder, stronger…",
	132,
	sizeof(_dafpunk)/sizeof(*_dafpunk)/2,
	_dafpunk
};



/// Plays the alarm melody when an attempt to write a config message over RS485
void Buzzer :: playAttemptToWriteConfigMessageToRs485Alarm() {

	playMelody(alarm1);

}


void Buzzer :: playMelody(MusicScore melody) {
	if (disabled) {
		warn("Buzzer is disabled");
		return;
	}
	resetPwn();

	Serial.printf("Playing melody \"%s\": starting for %d notes on tempo %d…\n", melody.name, melody.notesCount, melody.tempo);
	const int duration = 4*Scale*110/melody.tempo;

	 // iterate over the notes of the melody:
	for (int thisNote = 0; thisNote < melody.notesCount-1; thisNote++) {

		// to calculate the note duration, take one second divided by the note type.
		//e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.

		//log("Playing melody \"%s\": playing note #%d",  melody.name, thisNote);
		int t = melody.notes[1+thisNote*2]; 
		if (t==0) {
			Serial.printf("Playing melody \"%s\": note %d/%d duration is zero, exiting\n",  melody.name, thisNote, melody.notesCount);
			break;
		} 
		int noteDuration = 110*duration / t;
		tone( melody.notes[0+thisNote*2], noteDuration);

		// to distinguish the notes, set a minimum time between them.
		// the note's duration + 30% seems to work well:
		int pauseBetweenNotes = noteDuration * 1.30;
		delay(pauseBetweenNotes);
		// stop the tone playing:
		noTone();
	}

	noTone();

	//log("Playing melody \"%s\": done!", melody.name);
}
#endif
// ========================================

Adafruit_INA219 ina219;

float shuntvoltage = 0;
float busvoltage = 0;
float current_mA = 0;
float power_mW = 0;

/// Support for RS-485?
#define ENABLE_RS485 false

#if ENABLE_RS485
/// If true, will only write a config and wait for the response
#define RS485_CONFIG_WRITE_ONE_SHOT true

#endif

/// Support for CANbus?
#define ENABLE_CANBUS true
#define ENABLE_CANBUS_TRANSMIT false

#define SUPPORT_ASK_BATTERY true

#define SUPPORT_CANBUS_IFOLLOW true // 500KBPS otherwise 250KBPS
#if SUPPORT_CANBUS_IFOLLOW
#undef SUPPORT_ASK_BATTERY
#define SUPPORT_ASK_BATTERY false
#endif

#define PROTOCOL_RS485  1
#define PROTOCOL_CANBUS 2

/// What protocol to use to ask the SoC next time? Default to RS-485, if supported
int protocol = 
  #if ENABLE_RS485
  PROTOCOL_RS485
  #else
  PROTOCOL_CANBUS
  #endif
;



/// Screen support
#define BLACK 0x0000
#define WHITE 0xFFFF
#define GREY  0x5AEB
#define RED   0xF000
#define GREEN 0x0F00
#define BLUE  0x00F0

#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

#define DISPLAY_W 128
#define DISPLAY_H 160

const int16_t w = DISPLAY_W;
const int16_t h = DISPLAY_H;

/// Ask for SoC every askingDelay ms
const int askingDelay = 5000;

float soc  = -1;
void showSoc();

// == two possible dat source: UART or RS-485 transciver
#define DATA_SOURCE_RS485 true


#if DATA_SOURCE_RS485
  // == RS-485 pinouts setup (same as SBM IoT REV3)

  #define RS485_TX          21 // DI
  #define RS485_RX          19 // RO
  #define RS485_DE_RE_MODE  33 // Control pin

#else
  // == UART pintouts setup
  #define SERIAL_USE_SERIAL1 false
  #define SERIAL_USE_SERIAL2 false

  #if SERIAL_USE_SERIAL1
      #define RS485_TX 10
      #define RS485_RX 9
  #elif SERIAL_USE_SERIAL2
  //#if SERIAL_USE_SERIAL2
      #define RS485_TX 17
      #define RS485_RX 16
  #else
      #define RS485_TX 12
      #define RS485_RX 13
  #endif
#endif

// == CANbus pinouts setup which matches the SBM IoT wiring 
#define CAN_RX GPIO_NUM_4 //GPIO_NUM_5
#define CAN_TX GPIO_NUM_5 //GPIO_NUM_4

#define CAN_ENABLE_ALERTS false

/// CANbus surpport and other system stuffs
#include <driver/can.h>
#include <driver/gpio.h>
#include <esp_system.h>
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"


#include <SoftwareSerial.h>


// == Setup a new serial port

#if SERIAL_USE_SERIAL1
#define bmsSerial Serial1
#elif SERIAL_USE_SERIAL2
//#if SERIAL_USE_SERIAL2
#define bmsSerial Serial2
#else
SoftwareSerial bmsSerial;
#endif









/// Daly message ID (see SDL's DBC for the list)
#define BATTERY_SOC_VOLTAGE_CURRENT   0x90    
#define BATTERY_MIN_MAX_TEMPERATURES  0x91


#define MESSAGE_FLAGS (CAN_MSG_FLAG_EXTD)
// | CAN_MSG_FLAG_SS 
  //| CAN_MSG_FLAG_RTR; 


// define your CAN messages here, OR you can define them locally...
// standard 11-bit frame ID = 0
// extended 29-bit frame ID = 1
// format:   can_message_t (name of your message) = {std/ext frame, message ID, message DLC, {data bytes here}};

uint32_t previousMillis;
const uint32_t interval = 5000;

#define FONT_SMALLEST 1
#define FONT_SMALL 2


//#define log Serial
//#define log tft

#if false
 
  // Fill screen with grey so we can see the effect of printing with and without 
  // a background colour defined
  tft.fillScreen(TFT_GREY);
  
  // Set "cursor" at top left corner of display (0,0) and select font 2
  // (cursor will move to next line automatically during printing with 'tft.println'
  //  or stay on the line is there is room for the text with tft.print)
  tft.setCursor(0, 0, 2);
  // Set the font colour to be white with a black background, set text size multiplier to 1
  tft.setTextColor(TFT_WHITE,TFT_BLACK);  tft.setTextSize(1);
  // We can now plot text on screen using the "print" class
  tft.println("Hello World!");
  
  // Set the font colour to be yellow with no background, set to font 7
  tft.setTextColor(TFT_YELLOW); tft.setTextFont(2);
  tft.println(1234.56);
  
  // Set the font colour to be red with black background, set to font 4
  tft.setTextColor(TFT_RED,TFT_BLACK);    tft.setTextFont(4);
  tft.println((long)3735928559, HEX); // Should print DEADBEEF

  // Set the font colour to be green with black background, set to font 2
  tft.setTextColor(TFT_GREEN,TFT_BLACK);
  tft.setTextFont(2);
  tft.println("Groop");

  // Test some print formatting functions
  float fnumber = 123.45;
   // Set the font colour to be blue with no background, set to font 2
  tft.setTextColor(TFT_BLUE);    tft.setTextFont(2);
  tft.print("Float = "); tft.println(fnumber);           // Print floating point number
  tft.print("Binary = "); tft.println((int)fnumber, BIN); // Print as integer value in binary
  tft.print("Hexadecimal = "); tft.println((int)fnumb
#endif

#define TERMINAL_LOG_TO_SERIAL false


/// Enables for thread-safe (does not yet work)
#define TERMINAL_USE_SEMAPHORE true

/// Enables for debugging terminal screen and scrolling
#define TERMINAL_DEBUG_SCREEN false

/// Screen horizontal size in smallest font characters
#define DISPLAY_CW 21

/// Screen vertical size in smallest font characters
#define DISPLAY_CH 20

/// Screen size in smallest font characters
#define DISPLAY_SIZE (DISPLAY_CW*DISPLAY_CH)



//================================================
/// The scrolling terminal
class Screen {
  public: 
  static const float scaleX;
  static const float scaleY;

  /// Moves the cursor to (x,y) in smallest characters units
  void setCursor(int x, int y) {
    tft.setCursor(x*scaleX, y*scaleY, FONT_SMALLEST);
  }

  //void printf(int x, int y, char *format)
};

const float Screen::scaleX = (float)DISPLAY_W/(float)DISPLAY_CW;
const float Screen::scaleY = (float)DISPLAY_H/(float)DISPLAY_CH;

Screen screen;

#define DISPLAY_Y_TITLE 0
#define DISPLAY_Y_SOC 1
#define DISPLAY_Y_WAITING 2
#define DISPLAY_Y_ERROR 3
#define DISPLAY_Y_TERMINAL_WINDOW 4



//================================================
/// The scrolling terminal
class Terminal {

  /// Cursor horizontal position
  int x = 0;

  /// Cursor vertical position
  int y = 0;

  int scrollFrom = 4;
  int scrollTo = DISPLAY_CH;


  #define SCREEN0_CANARY "SDL0SDL0SDL0SDL0SDL0S"
  #define SCREEN1_CANARY "SDL1SDL1SDL1SDL1SDL1S"
  
  /// Flipping buffer 0
  static char screen0[DISPLAY_SIZE+1
  
  #if TERMINAL_DEBUG_SCREEN
  +DISPLAY_CW+1
  #endif
  ];
//  static char screen0Canary[DISPLAY_CW+1];

  /// Flipping buffer 1
  static char screen1[DISPLAY_SIZE+1];
  #if TERMINAL_DEBUG_SCREEN
  static char screen1Canary[DISPLAY_CW+1];
  #endif

  /// Current screen
  static char *screen;
  static int screenId;

  /// Currently scrolling?
  bool scrolling = false;



  public:

  #if TERMINAL_USE_SEMAPHORE
    static SemaphoreHandle_t semaphore;
  #endif
  
  Terminal(int from, int to) {
    scrollFrom = from>=0 ? from : 0;
    scrollTo = to<DISPLAY_CH-1 ? to : DISPLAY_CH-1;
    
    y = scrollFrom;
  }

  /// Setup terminal
  void setup() {


    clear();
  }

  /// Clear the terminal
  void clear() {
    memset(screen0, ' ', DISPLAY_SIZE); screen0[DISPLAY_SIZE] = 0;
    memset(screen1, ' ', DISPLAY_SIZE); screen1[DISPLAY_SIZE] = 0;


    #if TERMINAL_DEBUG_SCREEN
    strncpy(&screen0[DISPLAY_SIZE+1], SCREEN0_CANARY, DISPLAY_CW);
    logScreen("After strcpy canary 0");

    strncpy(&screen1[DISPLAY_SIZE+1], SCREEN1_CANARY, DISPLAY_CW);
    logScreen("After strcpy canary 1");
    #endif

    x = 0;
    y = scrollFrom;
  }

  /// Scrolls one line up
  void scroll();

  /// Actually logs the message to the terminal and possibly scrolls up
  void log(char *format, ...);

  #if TERMINAL_DEBUG_SCREEN
  void logScreen(char *message) {
    // Log screen
    char line[DISPLAY_CW+1];
    line[DISPLAY_CW] = 0;
    
    Serial.printf("Screen ID %d: ", screenId); Serial.println(message);

    for(int j=0; j<DISPLAY_CH; j++) {
      memcpy(line, &screen[j*DISPLAY_CW], DISPLAY_CW);
      //tft.println(line);
      Serial.printf("%d-%02d: |",screenId, j); Serial.print(line); Serial.println("|");
    }
  }
  #endif
};

/// Flipping buffer 0
char Terminal::screen0[
 DISPLAY_SIZE+1

#if TERMINAL_DEBUG_SCREEN
+DISPLAY_CW+1
#endif
];

/// Flipping buffer 1
char Terminal::screen1[DISPLAY_SIZE+1];

#if TERMINAL_DEBUG_SCREEN
char Terminal::screen1Canary[DISPLAY_CW+1] = SCREEN1_CANARY;
#endif

/// Current screen
char*Terminal::screen = Terminal::screen0;
int  Terminal::screenId = 0;

/// RS-485 window
Terminal terminal(DISPLAY_Y_TERMINAL_WINDOW, DISPLAY_CH);


/// Scrolls the screen one line up
void Terminal::scroll() {
  const int startLineAddress        = scrollFrom        * DISPLAY_CW;
  const int firstScrollLineAddress  = startLineAddress  + DISPLAY_CW;
  const int lastScrollLineAddress   = (scrollTo-1)      * DISPLAY_CW;

  const int scrollSize = (scrollTo - scrollFrom - 1)    * DISPLAY_CW;

  #if TERMINAL_DEBUG_SCREEN
  Serial.printf("Will scroll from %d for %d chars and erase last line %d (scrollFrom: %d, scrollTo: %d)", startLineAddress, scrollSize, lastScrollLineAddress, scrollFrom, scrollTo); Serial.println();
  #endif

  // scroll and flip
  if (screenId==0) {
    #if TERMINAL_DEBUG_SCREEN
    if (strncmp(&screen0[DISPLAY_SIZE+1], SCREEN0_CANARY, DISPLAY_CW)!=0) {
      Serial.print("*** SCREEN CANARY 0 smashed before scroll as: [");
      Serial.printf("%21s",&screen0[DISPLAY_SIZE+1]);
      Serial.println("]");
    }
    #endif

    memcpy(&screen1[startLineAddress], &screen0[firstScrollLineAddress], scrollSize);

    #if TERMINAL_DEBUG_SCREEN
    if (strncmp(&screen0[DISPLAY_SIZE+1], SCREEN0_CANARY, DISPLAY_CW)!=0) {
      Serial.print("*** SCREEN CANARY 0 smashed after scroll as: [");
      Serial.printf("%21s",&screen0[DISPLAY_SIZE+1]);
      Serial.println("]");
    }
    #endif
    screen = screen1;
    screenId = 1;
  }
  else {
    memcpy(&screen0[startLineAddress], &screen1[firstScrollLineAddress], scrollSize);

    #if TERMINAL_DEBUG_SCREEN
    if (strncmp(screen1Canary, SCREEN1_CANARY, sizeof(screen1Canary))!=0) Serial.println("*** SCREEN CANARY 1 smashed");
    #endif

    screen = screen0;
    screenId = 0;
  }

  // Clear the last line
  memset(&screen[lastScrollLineAddress], ' ', DISPLAY_CW);

  // Scroll cursor and set the char
  y = scrollTo;
}


/// Actually logs the message to the terminal and possibly scrolls up
void Terminal::log(char *format, ...) {
  #if TERMINAL_USE_SEMAPHORE

  if (!xSemaphoreTake(semaphore, (TickType_t)100)) {
    Serial.println("Unable to take the semaphore");
    return;
  }
  #endif

  va_list vargs;
  va_start(vargs, format);

  char buffer[256];
  const int l = vsnprintf(buffer, sizeof(buffer), format, vargs);
  va_end(vargs);


  const bool newLine = *format=='>'? false : true;
  //Serial.printf("log: [%s]=>%s (%d) / new line =%d\n", format, buffer, l, newLine?1:0);
#if TERMINAL_DEBUG_SCREEN
#else
  if (newLine) {
    Serial.println(buffer);
  }
  else 
    Serial.print(&buffer[1]);
#endif

  #if TERMINAL_LOG_TO_SERIAL
  return;
  #endif

  for(int i=newLine?0:1; i<l; i++) {
    char c = buffer[i];

    if (c=='\t') c = '>';
    else if (c<' ' && c>127) c = '?';



    // Line feed?
    if (x>=DISPLAY_CW) {
      // should we scroll?
      if (y >= scrollTo) 
        scroll();
      /*if (scrolling) {
        y = scrollTo-1; 
      }*/ 
      else { 
        y++;
      }
      x=0;
      #if TERMINAL_DEBUG_SCREEN
      Serial.println("NEW LINE");
      #endif
    }

    // Skip starting space in wrapped lines
    if (i>0 && c==' ' && x == 0) continue;

 /*
    // should we scroll?
    if (!scrolling && y>=scrollTo) {
      scrolling = true;
      scroll();
      scrolling = false;
    }
    */
    //Serial.printf("- %c @ (%d, %d)\n",c, x, y);
    screen[x+y*DISPLAY_CW] = c;
    x++;

    delay(10);
  }

  if (newLine) {
    if (scrolling) y = scrollTo-1; else y++;
    x=0;
    //Serial.println("NEW LINE (en of line)");
  }


  //scrolling = false;

  // Display screen
  
  #if TERMINAL_DEBUG_SCREEN
  // Log screen
  logScreen("Debug current screen");
  #endif

  ::screen.setCursor(0,scrollFrom);
  tft.println(screen);


  // Wait a bit
  //delay(10);
  #if TERMINAL_USE_SEMAPHORE
  if (xSemaphoreGive(semaphore) != pdPASS) {
    Serial.println("Unable to give the semaphore");
  }
  //else {
  //  Serial.println("Gave the semaphore");
  //}
  #endif
}

#if TERMINAL_USE_SEMAPHORE
SemaphoreHandle_t Terminal::semaphore = xSemaphoreCreateMutex();
#endif


//================================================
void canRx(void *arg);
void canTx(void *arg);
#if CAN_ENABLE_ALERTS
void canAlerts(void *arg);
#endif

/// CANbus BMS communication object
/// @link https://docs.espressif.com/projects/esp-idf/en/release-v3.3/api-reference/peripherals/can.html
/// Product: https://www.waveshare.com/sn65hvd230-can-board.htm
/// Schematics: https://www.waveshare.com/wiki/SN65HVD230_CAN_Board

class CanBusBms {
  public:


  /// The CANbus error messages
  static /*volatile*/ String errorMessage;

  /// The CANBus debug messages 
  static /*volatile*/ String debugMessage;
  
  can_message_t myMessageToSend, chipIdMessage;

  int counter = 0;

  /// CANbus window
  //Terminal terminal = Terminal(DISPLAY_CH/2, DISPLAY_CH);

  /// Logs the message 
  void logMessage(can_message_t * message, bool transmit) {

    const uint8_t type = message->identifier>>24;
    const uint8_t dataId = message->identifier>>16;
    const char *sTransmit =  transmit ? "to" : "from";
    if (type == 0x18)
      terminal.log(">CAN %s [BATTERY], ", sTransmit);
    else
      terminal.log(">CAN %s [%02x], ", sTransmit, type);



    terminal.log(">ID [%08x]: ", message->identifier);
    //const char bytes[8*4];
    
    for (int i=0; i<8; i++) {
      terminal.log(">%02x ", message->data[i]);
    }
    
    terminal.log("-");

    char buffer[] = "01234567";  // trailing zero included

    uint16_t rawVoltage = 0;
    uint16_t rawCurrent = 0;
    uint16_t rawSoc = 0;

    for (int i=0; i<8; i++) {
      byte b = message->data[i];
      buffer[i] = (b < 32 || b > 127) ? '?' : b;

      // == If we log a RX message, decompress data
      if (!transmit) {
        switch(dataId) {
        case BATTERY_SOC_VOLTAGE_CURRENT:
          switch(i) {
            case 0: rawVoltage = b<<8; break;
            case 1: rawVoltage |= b; break;

            case 4: rawCurrent = b<<8; break;
            case 5: rawCurrent |= b; break;

            case 6: rawSoc = b<<8; break;
            case 7: rawSoc |= b;
              const float voltage = rawVoltage*0.1;
              const float current = (rawCurrent-30000)*0.1;
              soc = rawSoc*0.1;
              
              terminal.log("CAN: %.1fV | %.2fA | SoC: %.1f%%", voltage, current, soc);
              showSoc();
            break;
          }
          break;
        }
      }
    }
    //terminal.log("\tASCII: %s", buffer);
  }




  /// Transmits a message over the CANbus
  void transmit(can_message_t * message) {    
    #if ENABLE_CANBUS_TRANSMIT
    logMessage(message, true);

    // Actually transmit
    const int r = can_transmit(message, pdMS_TO_TICKS(100));

    if (r == ESP_OK) {
      terminal.log("CAN: OK TX, queued");
    } 
    else {
      terminal.log("CAN: KO TX, failed to queue %x", r);
    }
    #endif
  }




  /// Asks the battery to send a data back: (aka transmit a message taking care of the message ID formatting) 
  void askBattery(uint8_t dataId) {
    /// Two possible source/target
    #define BATTERY     0x01
    #define SBM         0x40

    /// Message formatting
    #define TO(a)       (((a)&0xff)<<8)
    #define FROM(a)     ( (a)&0xff)
    #define DATA_ID(a)  (((a)&0xff)<<16)
    
    #if ENABLE_CANBUS_TRANSMIT

    const uint32_t messageId = 0x18000000 | DATA_ID(dataId) | TO(BATTERY) | FROM(SBM);

    can_message_t message;
    message.identifier = messageId;
    message.flags = MESSAGE_FLAGS | CAN_MSG_FLAG_RTR;

    if (message.flags& CAN_MSG_FLAG_RTR) {

    } else {
      message.data_length_code = 8;
    
      // All data is zero
      for (int i = 0; i < message.data_length_code; i++) message.data[i] = 0;
    }

    transmit(&message);
    #endif
  }



  void transmitSbmDisplay() {
    uint8_t payload[] = {0x65, 0xc2, 0x73, 0xF5, 0x03, 0xe8, 0x00, 0x32};
    transmitPayload(0x11, payload); 
  }

  void transmitBoard() {
    uint8_t payload[] = {0x02, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    transmitPayload(0x21, payload); 
  }


  void transmitPayload(uint8_t dataId, uint8_t payload[]) {
    /// Two possible source/target
    #define BATTERY     0x01
    #define CLIENT      0x40

    const uint32_t messageId = 0x18000000 | DATA_ID(dataId) | TO(CLIENT) | FROM(BATTERY);

    Serial.printf("CAN: ==> transmitting messge ID %04x to client ==>", messageId);
    can_message_t message;
    message.identifier = messageId;

    message.data_length_code = 8;
    message.flags = MESSAGE_FLAGS;
    
    // All data is zero
    for (int i = 0; i < message.data_length_code; i++) message.data[i] = payload[i];
    
    transmit(&message);

  }
  
  /// Transmit the given data
  void relayMessage(uint8_t dataId, uint8_t payload[]) {

    /// Two possible source/target
    #define BATTERY     0x01
    #define CLIENT      0x40

    /// Message formatting
    #define TO(a)       ((a)<<8)
    #define FROM(a)     (a)
    #define DATA_ID(a)  ((a)<<16)

    #if ENABLE_CANBUS_TRANSMIT
    const uint32_t messageId = 0x18000000 | DATA_ID(dataId) | TO(CLIENT) | FROM(BATTERY);

    Serial.printf("CAN: ==> Relaying message ID %04x to client ==>", messageId);
    can_message_t message;
    message.identifier = messageId;

    message.data_length_code = 8;
    message.flags = MESSAGE_FLAGS;
    
    // All data is zero
    for (int i = 0; i < message.data_length_code; i++) message.data[i] = payload[i];
    
    transmit(&message);
    #endif
  }
  

  void setup_can_driver(){
    // Initialize CANbus driver @ 250KBPS on standard 4/5 IO
    can_general_config_t general_config = {
      .mode = 
      //CAN_MODE_LISTEN_ONLY,
      //CAN_MODE_NO_ACK,
      CAN_MODE_NORMAL,
      .tx_io = (gpio_num_t)CAN_TX,
      .rx_io = (gpio_num_t)CAN_RX,
      .clkout_io = (gpio_num_t)CAN_IO_UNUSED,
      .bus_off_io = (gpio_num_t)CAN_IO_UNUSED,
      .tx_queue_len = 100,
      .rx_queue_len = 65,
      .alerts_enabled = CAN_ALERT_NONE,
      .clkout_divider = 0
    };
    #if SUPPORT_CANBUS_IFOLLOW
    can_timing_config_t timing_config = CAN_TIMING_CONFIG_500KBITS();
    #else
    can_timing_config_t timing_config = CAN_TIMING_CONFIG_250KBITS();
    #endif

    terminal.log("CAN: speed %d", timing_config);

    can_filter_config_t filter_config = CAN_FILTER_CONFIG_ACCEPT_ALL();
    
    esp_err_t error = can_driver_install(&general_config, &timing_config, &filter_config);
    if (error == ESP_OK) {
      terminal.log("CAN: driver installation success...");
    }
    else {
      terminal.log("CAN: driver installation FAILED...");
      return;
    }

    // = Start CAN driver
    error = can_start();
    if (error == ESP_OK) {
      terminal.log("CAN: driver start success...");
    }
    else {
      terminal.log("CAN: driver start FAILED...");
      return;
    }


    #if CAN_ENABLE_ALERTS
    // = Reconfigure alerts to detect Error Passive and Bus-Off error states
    const uint32_t alerts_to_enable = CAN_ALERT_ERR_PASS | CAN_ALERT_BUS_OFF; // CAN_ALERT_ALL makes the firmware to be puzzled
    if (can_reconfigure_alerts(alerts_to_enable, NULL) == ESP_OK) {
      terminal.log("CAN: Alerts reconfigured");
    } else {
      terminal.log("CAN: Failed to reconfigure alerts");
    }
    #endif

  }




  void setupCanbus() {

    // Setup and start CANbus commms
    setup_can_driver();

  // Init the message we gonna send as Extended frame and single shot comm.
    myMessageToSend.identifier = 0xDEADDEAD;
    myMessageToSend.data_length_code = 8;
    myMessageToSend.flags = MESSAGE_FLAGS;
    
    // Init our 16-bit counter to zero
    myMessageToSend.data[0] = 0;
    myMessageToSend.data[1] = 0;
    #if 0
    char *msg = "hello!";
  #else
    char *msg = "goodbye";
  #endif
    for (int i=0; i<6; i++) myMessageToSend.data[2+i] = msg[i];

  // Init the message we gonna send as Extended frame and single shot comm.
    chipIdMessage.identifier = 0xC419;
    chipIdMessage.data_length_code = 8;
    chipIdMessage.flags = MESSAGE_FLAGS;

    const uint64_t chipId = ESP.getEfuseMac();
    
    // Init our 16-bit counter to zero
    chipIdMessage.data[0] = chipId>>48;
    chipIdMessage.data[1] = chipId>>32;
    chipIdMessage.data[2] = chipId>>24;
    chipIdMessage.data[3] = chipId>>16;
    chipIdMessage.data[4] = chipId>> 8;
    chipIdMessage.data[5] = chipId;
    chipIdMessage.data[6] = '-';
    chipIdMessage.data[7] = '-';



    //xTaskCreatePinnedToCore(&canTx, "CAN_tx", 4096, NULL, tskIDLE_PRIORITY/*8*/, NULL, tskNO_AFFINITY);
    //xTaskCreatePinnedToCore(&canRx, "CAN_rx", 4096, NULL, tskIDLE_PRIORITY/*9*/, NULL, tskNO_AFFINITY);
    #if CAN_ENABLE_ALERTS
    xTaskCreatePinnedToCore(&canAlerts, "CAN_alerts", 4096, NULL, tskIDLE_PRIORITY/*9*/, NULL, tskNO_AFFINITY);
    #endif
  }

  /// Receive: read pending message, if any
  void receive() {
    terminal.log("CAN: looking for messages");

    can_message_t rx_frame;
    if (can_receive(&rx_frame, pdMS_TO_TICKS(100)) == ESP_OK) {
      // Receive
      const int id = rx_frame.identifier;
      terminal.log("CAN <== RX 0x%04x", id);
      logMessage(&rx_frame, false);

      //delay(1000);
    }
    else {
      terminal.log("CAN: no pending messages");
    }
  }
};

CanBusBms canBusBms;

/*volatile*/ String CanBusBms::errorMessage = "";
/*volatile*/ String CanBusBms::debugMessage = "";

#if CAN_ENABLE_ALERTS

void canAlerts(void *arg) {
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0') 


  int counter = 1;
  for(;;) {
    // = Block indefinitely until an alert occurs
    uint32_t alerts_triggered;
    can_read_alerts(&alerts_triggered, portMAX_DELAY);

    terminal.log("CANbus alerts #%d: 0x%04x occurred ("BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN" "BYTE_TO_BINARY_PATTERN")",
      counter,
      alerts_triggered,
      BYTE_TO_BINARY(alerts_triggered>>24),
      BYTE_TO_BINARY(alerts_triggered>>16),
      BYTE_TO_BINARY(alerts_triggered>> 8),
      BYTE_TO_BINARY(alerts_triggered    )
    );


    /// Alert: Description
    // CAN_ALERT_TX_IDLE: No more messages queued for transmission
    // CAN_ALERT_TX_SUCCESS: The previous transmission was successful
    // CAN_ALERT_BELOW_ERR_WARN: Both error counters have dropped below error warning limit
    // CAN_ALERT_ERR_ACTIVE: CAN controller has become error active
    // CAN_ALERT_RECOVERY_IN_PROGRESS: CAN controller is undergoing bus recovery
    // CAN_ALERT_BUS_RECOVERED: CAN controller has successfully completed bus recovery
    // CAN_ALERT_ARB_LOST: The previous transmission lost arbitration
    // CAN_ALERT_ABOVE_ERR_WARN: One of the error counters have exceeded the error warning limit
    // CAN_ALERT_BUS_ERROR: A (Bit, Stuff, CRC, Form, ACK) error has occurred on the bus
    // CAN_ALERT_TX_FAILED: The previous transmission has failed
    // CAN_ALERT_RX_QUEUE_FULL: The RX queue is full causing a received frame to be lost
    // CAN_ALERT_ERR_PASS: CAN controller has become error passive
    // CAN_ALERT_BUS_OFF: Bus-off condition occurred. CAN controller can no longer influence bus

    CanBusBms::debugMessage = "";
    CanBusBms::errorMessage = "";
    
    // == Debug messages
    if (alerts_triggered&CAN_ALERT_TX_IDLE)               CanBusBms::debugMessage += "TX_IDLE\n";
    if (alerts_triggered&CAN_ALERT_TX_SUCCESS)            CanBusBms::debugMessage += "TX_SUCCESS\n";

    // == Error messages    
    if (alerts_triggered&CAN_ALERT_BELOW_ERR_WARN)        CanBusBms::errorMessage += "BELOW_ERR_WARN\n";
    if (alerts_triggered&CAN_ALERT_ERR_ACTIVE)            CanBusBms::errorMessage += "ERR_ACTIVE\n";
    if (alerts_triggered&CAN_ALERT_RECOVERY_IN_PROGRESS)  CanBusBms::errorMessage += "RECOVERY_IN_PROGRESS\n";
    if (alerts_triggered&CAN_ALERT_BUS_RECOVERED)         CanBusBms::errorMessage += "BUS_RECOVERED\n";
    if (alerts_triggered&CAN_ALERT_ARB_LOST)              CanBusBms::errorMessage += "ARB_LOST\n";
    if (alerts_triggered&CAN_ALERT_ABOVE_ERR_WARN)        CanBusBms::errorMessage += "ABOVE_ERR_WARN\n";
    if (alerts_triggered&CAN_ALERT_BUS_ERROR)             CanBusBms::errorMessage += "BUS_ERROR\n";
    if (alerts_triggered&CAN_ALERT_TX_FAILED)             CanBusBms::errorMessage += "TX_FAILED\n";
    if (alerts_triggered&CAN_ALERT_RX_QUEUE_FULL)         CanBusBms::errorMessage += "RX_QUEUE_FULL\n";
    if (alerts_triggered&CAN_ALERT_ERR_PASS)              CanBusBms::errorMessage += "ERR_PASS\n";
    if (alerts_triggered&CAN_ALERT_BUS_OFF)               CanBusBms::errorMessage += "BUS_OFF\n";

    Serial.printf("CAN: alert %s", CanBusBms::errorMessage.c_str()); Serial.println();
    Serial.printf("CAN: aldebugert %s", CanBusBms::debugMessage.c_str()); Serial.println();

    delay(3000);
  }
}  
#endif

/*
void canRx(void *arg) {
  for(;;) {

    canBusBms.receive();
    delay(1000);
  }
}


void canTx(void *arg) {
  for(;;) {
    // == Transmit: Every second, sends out a CANbus frame w/ our counter embedded
    if (millis() - previousMillis > interval) { 
      previousMillis = millis();

      //canBusBms.
      terminal.log("CAN ==> TX");
      canBusBms.askBattery(BATTERY_SOC_VOLTAGE_CURRENT);
      terminal.log("=====================");
      delay(10000);
    } 
  }
}
*/

//================================================
/// Display

// A few test variables used during debugging
bool change_colour = true;
bool selected = true;

void setupDisplay() {
  // Setup the TFT display
  tft.init();
  tft.setRotation(0); // Must be setRotation(0) for this sketch to work correctly
  tft.fillScreen(TFT_BLACK);


  //screen.setCursor(0, DISPLAY_Y_TITLE);
  //tft.setRotation(1);

  tft.fillScreen(BLACK);

  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  //tft.fillRect(0,0,Screen::scale*DISPLAY_CW,Screen::scale*1, TFT_RED);
  tft.drawCentreString("Baterm UART/RS-485/CAN", DISPLAY_W/2, Screen::scaleY*DISPLAY_Y_TITLE, FONT_SMALLEST);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
}



//================================================
/// RS-485 BMS communication object
class Rs485Bms {
  public:
  void setup() {



#if DATA_SOURCE_RS485
    bmsSerial.begin(9600, SWSERIAL_8N1, RS485_RX, RS485_TX, false);
    bmsSerial.setTransmitEnablePin(RS485_DE_RE_MODE);
    //pinMode(TX, OUTPUT);
      
#else
  #if SERIAL_USE_SERIAL1
      bmsSerial.begin(9600, SERIAL_8N1, RS485_RX, TRS485_RX);
  #elif SERIAL_USE_SERIAL2
  //#if SERIAL_USE_SERIAL2
      bmsSerial.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX);
  #else
      //pinMode(TX, OUTPUT);
      //pinMode(RX, INPUT);
      bmsSerial.begin(9600, SWSERIAL_8N1, RS485_RX, RS485_TX, false);
  #endif
#endif

  
    if (!bmsSerial) { // If the object did not initialize, then its configuration is invalid
      terminal.log("Invalid BMS serial pin configuration, check config, stopped!"); 
      while (1) { // Don't continue with invalid configuration
        delay (1000);
      }
    }
    

    terminal.log("BMS serial setup successful wiring is: RX=%d TX=%d ", RS485_RX, RS485_TX); 
  }

  uint8_t checksum(uint8_t buffer[]) {
    uint8_t sum = 0;    
    for (int i=0; i<12; i++) {
      sum += buffer[i];
    }
    return sum;
  }

  void ask90SocCurrentvoltage() {
    askBattery(BATTERY_SOC_VOLTAGE_CURRENT);
  }

  void askBattery(uint8_t dataId) {
    //                  Header       Data ID     0     1     2     3     4     5     6     7     
    //                          To BMS      Length                                                checksum
    uint8_t message[] = {0xA5,  0x40, dataId, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    message[12] = checksum(message); 
    
    #if DATA_SOURCE_RS485
    terminal.log(">==> RS-485 TX ");
    #else
    terminal.log(">==> UART TX ");
    #endif

    for (int i=0; i<13; i++) {
      terminal.log(">%02x ", message[i]);
    }
    terminal.log("...");
    
    #if DATA_SOURCE_RS485
    bmsSerial.enableTx(true); delay(15);
    #endif

    bmsSerial.write(message, sizeof(message));
    
    #if DATA_SOURCE_RS485
    bmsSerial.enableTx(false); delay(15);
    #endif

    showSoc();
  }








  #if RS485_CONFIG_WRITE_ONE_SHOT

  /// Writes a config message to the battery
  void writeBattery(uint8_t dataId, const uint8_t payload[]) {
  //                  Header       Data ID     0     1     2     3     4     5     6     7     
    //                          To BMS      Length                                                checksum
    uint8_t message[] = {0xA5,  0x40, dataId, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    Serial.println("[[[[[[ WRITING CONFIG AS FOLLOWS ]]]]]]");
    // Write payload
    for (int i=0; i<8; i++) message[4+i] = payload[i];

    message[12] = checksum(message); 
    
    #if DATA_SOURCE_RS485
    terminal.log(">==> RS-485 TX ");
    #else
    terminal.log(">==> UART TX ");
    #endif

    for (int i=0; i<13; i++) {
      terminal.log(">%02x ", message[i]);
    }
    terminal.log("...");
    
    Serial.println("[[[[[[ ------------------------- ]]]]]]");

    #if DATA_SOURCE_RS485
    bmsSerial.enableTx(true); delay(15);
    #endif

    bmsSerial.write(message, sizeof(message));
    
    #if DATA_SOURCE_RS485
    bmsSerial.enableTx(false); delay(15);
    #endif
  }    
  
  #endif  
  
};

Rs485Bms rs485Bms;

byte rxBuffer[1024];
int rxBufferOffset = 0;





//================================================
/// Setup

void setup() {
	Serial.begin(115200);


  // Setup terminal
  terminal.setup();

  // Setup the display
  setupDisplay();

  showSoc();  

  //terminal.log("");
	//terminal.log("Magic Apps Communications Sniffer");
	terminal.log("---------------------");



  // == Start serial commuications with the BMS
  #if ENABLE_RS485
  terminal.log("Now, listening to the RS485 serial @ 9600bps 8N1:");
  rs485Bms.setup();
  #endif

  // == Setup CANbus
  #if ENABLE_CANBUS
  terminal.log("Now, listening to the CANbus");
  canBusBms.setupCanbus();
  #endif


  Wire.begin(21, 22);


  // Coulomb meter: initialize ina219 with default measurement range of 32V, 2A
  if (ina219.begin()) {
    ina219.setCalibration_32V_2A();    // set measurement range to 32V, 2A  (do not exceed 26V!)
    //ina219.setCalibration_32V_1A();    // set measurement range to 32V, 1A  (do not exceed 26V!)
    //ina219.setCalibration_16V_400mA(); // set measurement range to 16V, 400mA
    terminal.log("Coulomb meter setup");
  }
  Serial.println("exiting setup");
}




//================================================
/// Loop

int lines = 0;
int lastLog = 0;

int progress = 0;
int lastProgress = 0;
const char progressChars[] = {"|/-\\"};

/// When should we look for receiving CANbus messages?
int canBusLastReception = 0;
/// When we last transmitted SBM display CANbus message?
int canBusLastSbmTransmit = 0;
/// When we last transmitted board CANbus message?
int canBusLastBoardTransmit = 0;

/// When should we look for receiving RS-485 messages?
int rs485LastReception = 0;

/// Should we show the potential error?
int canBusLastShowError = 0;

int loopCount = 0;

void showSoc() {
  tft.startWrite();
  screen.setCursor(0,DISPLAY_Y_SOC);
  tft.print("SoC:");
  
  const int offsetX = Screen::scaleX*4;
  const int dw = w - offsetX;
  const int sw = soc >= 0 ? soc*dw/100 : dw/2;
  const int sh = Screen::scaleY*1;
  const int sy = Screen::scaleY*DISPLAY_Y_SOC;

  const int g = soc >= 0 ? TFT_GREEN : TFT_WHITE;
  
  // Quick way to draw a line
  tft.setAddrWindow(offsetX+0, sy, sw, sh);
  tft.pushColor(g, sw*sh); // push remaining capacity

  tft.setAddrWindow(offsetX+sw, sy, w-sw, sh);
  tft.pushColor(TFT_RED, (w-sw)*sh); // push total capacity

  tft.endWrite();  
}

void loop(void) {
  //loopCount++;
  //Serial.printf("looping %d", loopCount); Serial.println();
  //delay(5000);

  const int now = millis();

  // Show progress
  if (now - lastProgress > 100) {
    lastProgress = now;
    if (progress>=sizeof(progressChars)) progress = 0;
    const char c = progressChars[progress++];
    tft.drawChar(c, 0,0, FONT_SMALLEST);
  }


  
  #if ENABLE_CANBUS
  /// == Should read CANbus?
  if (now - canBusLastReception > 15000) {
    canBusLastReception = now;    
    canBusBms.receive();
  }

  #if ENABLE_CANBUS_TRANSMIT
  if (now - canBusLastSbmTransmit> 6000) {
    canBusLastSbmTransmit = now;
    canBusBms.transmitSbmDisplay();
  }
  
  if (now - canBusLastBoardTransmit> 10000) {
    canBusLastBoardTransmit = now;
    canBusBms.transmitBoard();
  }
  #endif


  /// == Should read CANbus?
  #if CAN_ENABLE_ALERTS  
  if (now - canBusLastShowError > 1000) {
    screen.setCursor(0, DISPLAY_Y_ERROR);

    if (CanBusBms::errorMessage.length()>0) {
      char *msg = (char*)("CAN: Alert "+CanBusBms::errorMessage).c_str();
      //tft.setTextColor(TFT_WHITE, TFT_RED);
      //tft.printf(msg);
      Serial.println(msg);
    }
    else {
      char *msg = (char*)("CAN: OK "+CanBusBms::debugMessage).c_str();
      //tft.setTextColor(TFT_WHITE, TFT_GREEN);
      //tft.printf(msg);
      Serial.println(msg);
    }
  }
  #endif
  

  #endif



  // == Should read RS-485
#if ENABLE_RS485
  if (now - rs485LastReception > 1000) {
    rs485LastReception = now;    

    // == Empty serial, if byte available
    #if DATA_SOURCE_RS485
    bmsSerial.enableRx(true); delay(15);
    #endif

    Serial.printf("Looking for data available on serial (bmsSerial==%d): available is %d...", !!bmsSerial, bmsSerial.available()); Serial.println();

    if (bmsSerial.available()) {

      tft.setTextColor(TFT_GREEN, TFT_BLACK); 

      // Assume it is the start byte (which it might not be the case with burst messages)
      rxBufferOffset = 0;
      char * errorMsg = "";      

      do {
        const byte b = bmsSerial.read();


        // == Record the byte      
        if (rxBufferOffset < sizeof(rxBuffer)) {
          rxBuffer[rxBufferOffset] = b;
        }

        errorMsg = "";      
        
        /// Make sure we have a Daly message
        switch(rxBufferOffset) {
          case 0: 
            if (b==0xa5) 
              rxBufferOffset++;
            else {
              rxBufferOffset = 0;
              errorMsg = ": not A5 => not Daly msg, skip";
            }            
          break;

          case 1: 
            if (b==0x01) 
              rxBufferOffset++;
            else {
              rxBufferOffset = 0;
              errorMsg = ": not 01 => not a Daly response msg, reset";
            }
          break;
          
          case 3: 
            if (b==8) 
              rxBufferOffset++;
            else {
              rxBufferOffset = 0;
              errorMsg = ": not 1 => wrong payload length for Daly response msg, reset";
            }
          break;

          default: 
            rxBufferOffset++; 
          break;
              
        }

        if (rxBufferOffset == 0) {
          tft.setTextColor(TFT_WHITE, TFT_RED);
          screen.setCursor(0, DISPLAY_Y_ERROR);
          tft.printf("%02x%s", b, errorMsg);
          
          Serial.printf("%02x%s", b, errorMsg); 
          Serial.println();
        }
        else {
          if (rxBufferOffset == 1) terminal.log ("><==");

          if (b>=' ' && b<=127)
            terminal.log(">%02x (%c) ", b,b);
          else 
            terminal.log(">%02x ", b);        
        }

        /*
        // If it is a CR or we are near end of line then scroll one line
        if (data == '\r' || xPos>231) {
          xPos = 0;
          yDraw = scroll_line(); // It can take 13ms to scroll and blank 16 pixel lines
        }
        if (data > 31 && data < 128) {
          xPos += tft.drawChar(data,xPos,yDraw,2);
          blank[(18+(yStart-TOP_FIXED_AREA)/TEXT_HEIGHT)%19]=xPos; // Keep a record of line lengths
        }
        */

        // == End of Daly packet?
        if (rxBufferOffset==13) {
          break;
        }

      } while (bmsSerial.available());

      if (rxBufferOffset > 0) terminal.log("-");

      #if DATA_SOURCE_RS485
      bmsSerial.enableRx(false); delay(15);
      #endif

      /// Decoding some known messages
      // == Decode Daly 0x90?
      if (rxBuffer[0] == 0xa5 && rxBuffer[1] == 0x01 && rxBuffer[3] == 8 && rxBufferOffset == 13) {

        if (rxBuffer[2] == 0x90) {
          int i = 4;
          const uint16_t rawVoltage = (rxBuffer[i++]<<8) | rxBuffer[i++]; const float voltage =  0.1 * rawVoltage;
          i += 2;
          const uint16_t rawCurrent = (rxBuffer[i++]<<8) | rxBuffer[i++]; const float current = 0.1 * (rawCurrent  - 30000);
          const uint16_t rawSoc     = (rxBuffer[i++]<<8) | rxBuffer[i++]; soc     = 0.1 * rawSoc;

          const int roundtrip = now-lastLog;
          terminal.log("BMS: %.1fV | %.1fA | SoC %.1f%% (in %dms)",  voltage, current, soc, roundtrip);
        }


        #if ENABLE_CANBUS
        // == Now routing the data over CANbus

        uint8_t dataId = rxBuffer[2];
        uint8_t *payload =  &rxBuffer[4];
        terminal.log("RS-485: relaying data Id %02x over CANbus...", dataId);
        canBusBms.relayMessage(dataId, payload);
        #endif

      }
      // == Decode Baiway coulometer
      else if (rxBuffer[0] == 0xa5 && rxBufferOffset == 16) {
        int i = 1;
        const uint8_t rawSoc     = rxBuffer[i++];                       soc     =(float)rawSoc;
        const uint16_t rawVoltage = (rxBuffer[i++]<<8) | rxBuffer[i++]; const float voltage =  0.01 * rawVoltage;
        terminal.log("SBM Display: %.1fV | SoC %.1f%%",  voltage, soc);
      }
    }
  }
  #endif
  

#if SUPPORT_ASK_BATTERY
  
  // == Ask for SoC?
  if(lastLog == 0 
  || now-lastLog > askingDelay) {
    lastLog = now;
    tft.setTextColor(TFT_WHITE, TFT_BLACK); 
    screen.setCursor(1, DISPLAY_Y_TITLE);
    unsigned int uptime = millis() / 1000;  // secs
    const unsigned int t = uptime;
    const unsigned int days   = uptime / (24 * 3600);
    uptime -= days * 24 * 3600;
    const unsigned int hours  = uptime / 3600;
    uptime -= hours * 3600;
    const unsigned int mins   = uptime / 60;
    uptime -= mins * 60;
    const unsigned int secs   = uptime % 60;

    tft.printf("Uptime %dd %dh %d:%d | %d loops", days, hours, mins, secs, loopCount++);



      static uint8_t dataIds[] = {
        //BATTERY_SOC_VOLTAGE_CURRENT,
        0,
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
        BATTERY_SOC_VOLTAGE_CURRENT, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
      };

    // == Ask SoC by using the current protocol
    if (protocol == PROTOCOL_RS485) {
      #if RS485_CONFIG_WRITE_ONE_SHOT
      // Write 
      static bool written = false;
      if (!written) {
        written = true;
      
        const uint8_t dataId = 0x13;  // Construction information
        const uint16_t operationMode = 0; 
        const uint8_t year = 2023-2000;
        const uint8_t month = 4;
        const uint8_t day = 11;
        const uint16_t sleepAfter = 60; // seconde (MSB-LSB)
        const uint8_t batteryType = 0; // LFP
        
        const uint8_t payload[] = {
          operationMode>>8,
          operationMode,
          
          year,
          month,
          day,
          
          sleepAfter>>8,
          sleepAfter,
          
          batteryType
        };

        rs485Bms.writeBattery(dataId, payload);
      }
      #else      
      static int currentDataIdIndex = 0;
      rs485Bms.askBattery(dataIds[currentDataIdIndex]);
      currentDataIdIndex++;
      
      // Should we roll over?
      if (currentDataIdIndex>=sizeof(dataIds)/sizeof(*dataIds)) currentDataIdIndex = 0;
      #endif
    }
    else {

      static int currentDataIdIndex = 0;
      canBusBms.askBattery(dataIds[currentDataIdIndex]);
      currentDataIdIndex++;
      
      // Should we roll over?
      if (currentDataIdIndex>=sizeof(dataIds)/sizeof(*dataIds)) currentDataIdIndex = 0;
    }
    
    // == Toggle if we support multiple protocols
    #if ENABLE_RS485
    #if ENABLE_CANBUS
    protocol = protocol == PROTOCOL_CANBUS ? PROTOCOL_RS485 : PROTOCOL_CANBUS;
    #endif
    #endif
  }
  else {
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    screen.setCursor(0, DISPLAY_Y_WAITING);
    tft.printf("%s: request in %2ds", protocol == PROTOCOL_CANBUS ? "CAN":"RS485", (askingDelay - (now-lastLog))/1000);
    delay(50);
  }
  #endif


  // === Measuring current
  
  static uint32_t lastMillis = 0;
  if (millis() - lastMillis > 500) {

    // === Read current: from ina219
    shuntvoltage = ina219.getShuntVoltage_mV();
    busvoltage = ina219.getBusVoltage_V();
    current_mA = ina219.getCurrent_mA();
    power_mW = ina219.getPower_mW();


    screen.setCursor(0,DISPLAY_CH-5);
    tft.printf("Coulomb meter: Bus: %.1fV|Shunt: %.1fmV|Current: %.1fmA|Power: %.1fmW", busvoltage, shuntvoltage, current_mA, power_mW);
  }
  //delay(20);
}














#if false

// ##############################################################################################
// Call this function to scroll the display one text line
// ##############################################################################################
int scroll_line() {
  int yTemp = yStart; // Store the old yStart, this is where we draw the next line
  // Use the record of line lengths to optimise the rectangle size we need to erase the top line
  tft.fillRect(0,yStart,blank[(yStart-TOP_FIXED_AREA)/TEXT_HEIGHT],TEXT_HEIGHT, TFT_BLACK);

  // Change the top of the scroll area
  yStart+=TEXT_HEIGHT;
  // The value must wrap around as the screen memory is a circular buffer
  if (yStart >= YMAX - BOT_FIXED_AREA) yStart = TOP_FIXED_AREA + (yStart - YMAX + BOT_FIXED_AREA);
  // Now we can scroll the display
  scrollAddress(yStart);
  return  yTemp;
}

// ##############################################################################################
// Setup a portion of the screen for vertical scrolling
// ##############################################################################################
// We are using a hardware feature of the display, so we can only scroll in portrait orientation
void setupScrollArea(uint16_t tfa, uint16_t bfa) {
  tft.writecommand(ST7735_VSCRDEF);// ILI9341_VSCRDEF); // Vertical scroll definition
  tft.writedata(tfa >> 8);           // Top Fixed Area line count
  tft.writedata(tfa);
  tft.writedata((YMAX-tfa-bfa)>>8);  // Vertical Scrolling Area line count
  tft.writedata(YMAX-tfa-bfa);
  tft.writedata(bfa >> 8);           // Bottom Fixed Area line count
  tft.writedata(bfa);
}

// ##############################################################################################
// Setup the vertical scrolling start address pointer
// ##############################################################################################
void scrollAddress(uint16_t vsp) {
  tft.writecommand(ILI9341_VSCRSADD); // Vertical scrolling pointer
  tft.writedata(vsp>>8);
  tft.writedata(vsp);
}







/*
 * Pong
 * Original Code from https://github.com/rparrett/pongclock
 *
 */


int dly = 10;

int16_t paddle_h = 25;
int16_t paddle_w = 2;

int16_t lpaddle_x = 0;
int16_t rpaddle_x = w - paddle_w;

int16_t lpaddle_y = 0;
int16_t rpaddle_y = h - paddle_h;

int16_t lpaddle_d = 1;
int16_t rpaddle_d = -1;

int16_t lpaddle_ball_t = w - w / 4;
int16_t rpaddle_ball_t = w / 4;

int16_t target_y = 0;

int16_t ball_x = 2;
int16_t ball_y = 2;
int16_t oldball_x = 2;
int16_t oldball_y = 2;

int16_t ball_dx = 1;
int16_t ball_dy = 1;

int16_t ball_w = 4;
int16_t ball_h = 4;

int16_t dashline_h = 4;
int16_t dashline_w = 2;
int16_t dashline_n = h / dashline_h;
int16_t dashline_x = w / 2 - 1;
int16_t dashline_y = dashline_h / 2;

int16_t lscore = 12;
int16_t rscore = 4;

void setupGame() {
  
  randomSeed(analogRead(0)*analogRead(1));
   
  tft.init();

  tft.setRotation(1);

  tft.fillScreen(BLACK);
  
  initgame();


  delay(2000);
  
}

void loopGame() {
  delay(dly);

  lpaddle();
  rpaddle();

  midline();

  ball();
}

void initgame() {
  lpaddle_y = random(0, h - paddle_h);
  rpaddle_y = random(0, h - paddle_h);

  // ball is placed on the center of the left paddle
  ball_y = lpaddle_y + (paddle_h / 2);
  
  calc_target_y();

  midline();

  tft.fillRect(0,h-26,w,h-1,BLACK);

  tft.setTextDatum(TC_DATUM);
  //tft.setTextColor(WHITE);
  tft.setTextColor(WHITE, RED);

  tft.drawString("SDL pong", w/2, h-26 , 2);
}

void midline() {

  // If the ball is not on the line then don't redraw the line
  if ((ball_x<dashline_x-ball_w) && (ball_x > dashline_x+dashline_w)) return;

  tft.startWrite();

  // Quick way to draw a dashed line
  tft.setAddrWindow(dashline_x, 0, dashline_w, h);
  
  for(int16_t i = 0; i < dashline_n; i+=2) {
    tft.pushColor(WHITE, dashline_w*dashline_h); // push dash pixels
    tft.pushColor(BLACK, dashline_w*dashline_h); // push gap pixels
  }

  tft.endWrite();
}

void lpaddle() {
  
  if (lpaddle_d == 1) {
    tft.fillRect(lpaddle_x, lpaddle_y, paddle_w, 1, BLACK);
  } 
  else if (lpaddle_d == -1) {
    tft.fillRect(lpaddle_x, lpaddle_y + paddle_h - 1, paddle_w, 1, BLACK);
  }

  lpaddle_y = lpaddle_y + lpaddle_d;

  if (ball_dx == 1) lpaddle_d = 0;
  else {
    if (lpaddle_y + paddle_h / 2 == target_y) lpaddle_d = 0;
    else if (lpaddle_y + paddle_h / 2 > target_y) lpaddle_d = -1;
    else lpaddle_d = 1;
  }

  if (lpaddle_y + paddle_h >= h && lpaddle_d == 1) lpaddle_d = 0;
  else if (lpaddle_y <= 0 && lpaddle_d == -1) lpaddle_d = 0;

  tft.fillRect(lpaddle_x, lpaddle_y, paddle_w, paddle_h, WHITE);
}

void rpaddle() {
  
  if (rpaddle_d == 1) {
    tft.fillRect(rpaddle_x, rpaddle_y, paddle_w, 1, BLACK);
  } 
  else if (rpaddle_d == -1) {
    tft.fillRect(rpaddle_x, rpaddle_y + paddle_h - 1, paddle_w, 1, BLACK);
  }

  rpaddle_y = rpaddle_y + rpaddle_d;

  if (ball_dx == -1) rpaddle_d = 0;
  else {
    if (rpaddle_y + paddle_h / 2 == target_y) rpaddle_d = 0;
    else if (rpaddle_y + paddle_h / 2 > target_y) rpaddle_d = -1;
    else rpaddle_d = 1;
  }

  if (rpaddle_y + paddle_h >= h && rpaddle_d == 1) rpaddle_d = 0;
  else if (rpaddle_y <= 0 && rpaddle_d == -1) rpaddle_d = 0;

  tft.fillRect(rpaddle_x, rpaddle_y, paddle_w, paddle_h, WHITE);
}

void calc_target_y() {
  int16_t target_x;
  int16_t reflections;
  int16_t y;

  if (ball_dx == 1) {
    target_x = w - ball_w;
  } 
  else {
    target_x = -1 * (w - ball_w);
  }

  y = abs(target_x * (ball_dy / ball_dx) + ball_y);

  reflections = floor(y / h);

  if (reflections % 2 == 0) {
    target_y = y % h;
  } 
  else {
    target_y = h - (y % h);
  }
}

void ball() {
  ball_x = ball_x + ball_dx;
  ball_y = ball_y + ball_dy;

  if (ball_dx == -1 && ball_x == paddle_w && ball_y + ball_h >= lpaddle_y && ball_y <= lpaddle_y + paddle_h) {
    ball_dx = ball_dx * -1;
    dly = random(5); // change speed of ball after paddle contact
    calc_target_y(); 
  } else if (ball_dx == 1 && ball_x + ball_w == w - paddle_w && ball_y + ball_h >= rpaddle_y && ball_y <= rpaddle_y + paddle_h) {
    ball_dx = ball_dx * -1;
    dly = random(5); // change speed of ball after paddle contact
    calc_target_y();
  } else if ((ball_dx == 1 && ball_x >= w) || (ball_dx == -1 && ball_x + ball_w < 0)) {
    dly = 5;
  }

  if (ball_y > h - ball_w || ball_y < 0) {
    ball_dy = ball_dy * -1;
    ball_y += ball_dy; // Keep in bounds
  }

  //tft.fillRect(oldball_x, oldball_y, ball_w, ball_h, BLACK);
  tft.drawRect(oldball_x, oldball_y, ball_w, ball_h, BLACK); // Less TFT refresh aliasing than line above for large balls
  tft.fillRect(   ball_x,    ball_y, ball_w, ball_h, WHITE);
  oldball_x = ball_x;
  oldball_y = ball_y;
}







#endif

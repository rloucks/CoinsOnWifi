#include <FastLED.h>

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>


// -----[ Coins on Wifi ]----------------------------------    //
// This uses an crypto stock API, BearSSL, ESP8266, lights     //
// solder and time.                                            //
//                                                             //
// Segments are created, and then each light is added based    //
// on the call being made to the doL(letter) and doNum(number) //
// you can edit the letters/numbers as you like to create the  //
// font look you desire. each segment is based on a 3x5 square //
// in order to achive a long board and use only 100 lights.    //
//                                                             //
// You can technically make a bigger segments, but the segment // 
// will look as such, using leds[ XY(col + segX, row)]         //
// ------------------------------------------------------------//
// 0 1 2   (Row 0)      o o o                                  //
// 0 1 2   (Row 1)      o . o                                  //
// 0 1 2   (Row 2)      o o o   Makes an A                     // 
// 0 1 2   (Row 3)      o . o                                  //
// 0 1 2   (Row 4)      o . o                                  //
///////////////////////////////////////////////////////////////// 


// I wouldn't touch these
#define COLOR_ORDER GRB // Set the Color Order
#define CHIPSET     WS2811 // We all know 2812 is better ;)
#define LED_PIN  6 // The data pin, one of the 3 pins to connect. 

// Play with these all you like ----------------------------------------// 
#define BRIGHTNESS 150 // the total brightness
#define SEG1 0 // the start led for the first segment (see above)
#define SEG2 4 // so on.. and so forth...
#define SEG3 8
#define SEG4 12
#define SEG5 16 // add more if you like...

#define LAST_VISIBLE_LED 99 // don't trust arrays to do the dirty work. This is your last LED. Arrays start at 0 if you didn't remember.


// Params for width and height - this sets how 
const uint8_t kMatrixWidth = 20; // how wide is your matrix? (that's what she said)
const uint8_t kMatrixHeight = 5; // how high is your matrix? (no comment...)

// What coins do you want  to see? 
int coinsCount = 5; // make sure to add the count of coins...
const char* coins[] = {"DOGE", "BTT", "XRP", "BTC", "ETH"}; // the coin call signs
String currency_code = "usd"; // currency of the crypto. Only so many are supported 


// Play around only if you know what your are doing, this makes you matrix work ;)
#define NUM_LEDS (kMatrixWidth * kMatrixHeight) // count how many LEDs you potentially have. Hopefully they all work!
CRGB leds[ NUM_LEDS ]; // Setup that old fashion strip code...
uint8_t XY (uint8_t x, uint8_t y) {
  // any out of bounds address maps to the first hidden pixel
  if ( (x >= kMatrixWidth) || (y >= kMatrixHeight) ) {
    return (LAST_VISIBLE_LED + 1);
  }


// This is your table of LEDs. Go here: https://macetech.github.io/FastLED-XY-Map-Generator/
  const uint8_t XYTable[] = {
    19,  18,  17,  16,  15,  14,  13,  12,  11,  10,   9,   8,   7,   6,   5,   4,   3,   2,   1,   0,
    20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,
    59,  58,  57,  56,  55,  54,  53,  52,  51,  50,  49,  48,  47,  46,  45,  44,  43,  42,  41,  40,
    60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
    99,  98,  97,  96,  95,  94,  93,  92,  91,  90,  89,  88,  87,  86,  85,  84,  83,  82,  81,  80
  };

  uint8_t i = (y * kMatrixWidth) + x;
  uint8_t j = XYTable[i];
  return j;
}

// WIFI Network name and password
#define WIFI_SSID "SSID"
#define WIFI_PASS "123456"

// Fingerprint for api.cryptonator.com - expires 6 Feb 2022 - either you update it then or wait for an update to the script. 
const uint8_t fingerprint_crypto[20] = {0x10, 0x76, 0x19, 0x6B, 0xE9, 0xE5, 0x87, 0x5A, 0x26, 0x12, 0x15, 0xDE, 0x9F, 0x7D, 0x3B, 0x92, 0x9A, 0x7F, 0x30, 0x13};


// Okay - Stop editing. 
const String apiUrlstart = "https://api.cryptonator.com/api/ticker/";
const String apiUrlend = "-" + currency_code;


// don't touch this, unless you want to just mess everything up. 
// this sets the inital compare price for the lastprice. unless you are magical...
int lastPrice = 0;
String coinLast[] = {"0","0","0","0","0"};
 

ESP8266WiFiMulti WiFiMulti;
void loop()
{
  Serial.println("Starting the main loop...");
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    Serial.println("Running wifi https commands...");
    uint8_t fingerprint[20];
    memcpy(fingerprint, fingerprint_crypto, 20); std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure); client->setFingerprint(fingerprint);
    
    Serial.println("SSL fingerprint...");
    int loopDo = 0;
    Serial.println("Starting the loopDo...");
    while (loopDo < coinsCount) {
      Serial.print("Current loopDo:"); Serial.println(loopDo); Serial.print("Current Coin:"); Serial.println(coins[loopDo]);
      String apiUrl = apiUrlstart + coins[loopDo] + apiUrlend;
      Serial.print("Current Url"); Serial.println(apiUrl);
      HTTPClient https;
      if (https.begin(*client, apiUrl)) {  // HTTPS
        int httpCode = https.GET();

        if (httpCode > 0) {
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {

            String payload = https.getString();
            float price;
            
            Serial.print("Payload is: ");
            Serial.println(payload);
            Serial.println("---------------------------------");
            Serial.print("Converting Price: ");
            Serial.println(payload.substring(payload.indexOf("price") + 8, payload.indexOf("volume") - 3));
            Serial.println("---------------------------------");
            float currPrice = payload.substring(payload.indexOf("price") + 8, payload.indexOf("volume") - 3).toFloat();
            double testNum = currPrice;
            String theNum(testNum,6);
            float testNum2 = coinLast[loopDo].toFloat();
              
              String lastPrice(testNum2,6);
              String tmpNum = theNum.substring(0,1);
              String numOne(tmpNum); Serial.print(numOne); tmpNum = theNum.substring(1,2);
              String numTwo(tmpNum); Serial.print(numTwo); tmpNum = theNum.substring(2,3);
              String numThree(tmpNum); Serial.print(numThree); tmpNum = theNum.substring(3,4);
              String numFour(tmpNum); Serial.print(numFour); tmpNum = theNum.substring(4,5);
              String numFive(tmpNum); Serial.print(numFive); tmpNum = theNum.substring(5,6);
              String numSix(tmpNum); Serial.println(numSix); //special use only

            int aColor = 0;
            int bColor = 0;
        String theArrow = "";
        if (lastPrice.toDouble() > currPrice) {aColor = 255; Serial.println("Its Down!"); theArrow = "down";} else {aColor = 0;}
        if (lastPrice.toDouble() < currPrice) {bColor = 150;Serial.println("Its Up!"); theArrow = "up";} else {bColor = 0;}
        if (lastPrice.toDouble() == currPrice) {aColor = 100; bColor = 100;Serial.println("Its the same!"); theArrow = "s"; } 
      
            Serial.print("aColor:");Serial.println(aColor);
            Serial.print("bColor:");Serial.println(bColor);
            Serial.print("lastPrice used: "); Serial.println(lastPrice.toDouble(),6);

String coinName = coins[loopDo];
            delay(500);
            doL(SEG1, coinName.substring(0,1), 0, 0, 255);
            doL(SEG2, coinName.substring(1,2), 0, 0, 255);
            doL(SEG3, coinName.substring(2,3), 0, 0, 255);
            doL(SEG4, coinName.substring(3,4), 0, 0, 255);
            doArrow(SEG5, theArrow);
            FastLED.show();
            delay(3000);      
            FastLED.clear();
 
      
        if (numTwo == ".") {doNum(SEG1, numOne, aColor, bColor); doNum(SEG1, numTwo, aColor, bColor); doNum(SEG2, numThree, aColor, bColor); doNum(SEG3, numFour, aColor, bColor); doNum(SEG4, numFive, aColor, bColor); doNum(SEG5, numSix, aColor, bColor);}
        if (numThree == ".") {doNum(SEG1, numOne, aColor, bColor); doNum(SEG2, numTwo, aColor, bColor); doNum(SEG2, numThree, aColor, bColor); doNum(SEG3, numFour, aColor, bColor); doNum(SEG4, numFive, aColor, bColor); doNum(SEG5, numSix, aColor, bColor);}
        if (numFour == ".") {doNum(SEG1, numOne, aColor, bColor); doNum(SEG2, numTwo, aColor, bColor); doNum(SEG3, numThree, aColor, bColor); doNum(SEG3, numFour, aColor, bColor); doNum(SEG4, numFive, aColor, bColor); doNum(SEG5, numSix, aColor, bColor);}
        if (numFive == ".") {doNum(SEG1, numOne, aColor, bColor); doNum(SEG2, numTwo, aColor, bColor); doNum(SEG3, numThree, aColor, bColor); doNum(SEG4, numFour, aColor, bColor); doNum(SEG4, numFive, aColor, bColor); doNum(SEG5, numSix, aColor, bColor);}
        if (numSix == ".") {doNum(SEG1, numOne, aColor, bColor); doNum(SEG2, numTwo, aColor, bColor); doNum(SEG3, numThree, aColor, bColor); doNum(SEG4, numFour, aColor, bColor); doNum(SEG5, numFive, aColor, bColor); doNum(SEG5, numSix, aColor, bColor);}
       
        FastLED.show();
        
            Serial.println("----------last price set-----------------------");
            //coinLast[loopDo] = payload.substring(payload.indexOf("price") + 8, payload.indexOf("volume") - 3).toFloat();
            coinLast[loopDo] = currPrice;

            //String showLast(coinLast[loopDo], 6).toString());
            //showLast = showLast(6);
            Serial.print(coins[loopDo]); Serial.print(": "); Serial.print(coinLast[loopDo]);
            delay(5000);
            FastLED.clear();
          }
        } else { Serial.print(https.errorToString(httpCode).c_str()); }
        https.end();
        loopDo++;
      }
      else { Serial.println("Unable to connect to API"); }
    } //loopDo
  }
}

void doNum(int segX, String daNum, int daColorR, int daColorG) {

  if (daNum == "") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(3 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 1)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 1)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(3 + segX, 1)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(3 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    
    leds[ XY(0 + segX, 3)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 3)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 3)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(3 + segX, 3)] = CRGB( daColorR, daColorG, 0);
    
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 4)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(3 + segX, 4)] = CRGB( daColorR, daColorG, 0);
    }


  if (daNum == ".") {
    leds[ XY(3 + segX, 4)] = CRGB( 0, 0, 100);
  }



  if (daNum == "0") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 1)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(0 + segX, 3)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 3)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 4)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, 0);
  }



  if (daNum == "1") {
    leds[ XY(1 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 1)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 3)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 4)] = CRGB( daColorR, daColorG, 0);
  }

  if (daNum == "2") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 1)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(0 + segX, 3)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 4)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, 0);
  }

  if (daNum == "3") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 1)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 3)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 4)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, 0);
  }

  if (daNum == "4") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 1)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 3)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, 0);
  }

  if (daNum == "5") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 3)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 4)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, 0);
  }

  if (daNum == "6") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(0 + segX, 3)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 3)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 4)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, 0);
  }

  if (daNum == "7") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 1)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 3)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, 0);
  }

  if (daNum == "8") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 1)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(0 + segX, 3)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 3)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 4)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, 0);
  }


  if (daNum == "9") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 1)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(1 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 3)] = CRGB( daColorR, daColorG, 0);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, 0);
  }

}

void doL(int segX, String daLetter, int daColorR, int daColorG, int daColorB) {

  if (daLetter == "A") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
  }
  if (daLetter == "B") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    //leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
  }
  if (daLetter == "C") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
  }
  if (daLetter == "D") {

    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
   }
  if (daLetter == "E") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
  }
  if (daLetter == "F") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    }
  if (daLetter == "G") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
  }
  if (daLetter == "H") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
  }
  if (daLetter == "I") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
  }
  if (daLetter == "J") {
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
  }
  if (daLetter == "K") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
  }
  if (daLetter == "L") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
  }
  if (daLetter == "M") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
  }
    if (daLetter == "N") {
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
  }
  if (daLetter == "O") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
  }
  if (daLetter == "P") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
   }
  if (daLetter == "Q") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
  }
  if (daLetter == "R") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
  }
  if (daLetter == "S") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
  }
  if (daLetter == "T") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
   }
  if (daLetter == "U") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
  }
  if (daLetter == "V") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    }
  if (daLetter == "W") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
  }
  if (daLetter == "X") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);    
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    }
  if (daLetter == "Y") {
    leds[ XY(0 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 0)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
     }
  if (daLetter == "Z") {
    leds[ XY(0 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 2)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(0 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(2 + segX, 4)] = CRGB( daColorR, daColorG, daColorB);
  }
  if (daLetter == ":") {
    leds[ XY(1 + segX, 1)] = CRGB( daColorR, daColorG, daColorB);
    leds[ XY(1 + segX, 3)] = CRGB( daColorR, daColorG, daColorB);
     }
}

void doArrow(int segX, String direct) {

  if (direct == "up") {
    leds[ XY(0 + segX, 1)] = CRGB( 0, 255, 0);
    leds[ XY(2 + segX, 1)] = CRGB( 0, 255, 0);
    leds[ XY(0 + segX, 3)] = CRGB( 0, 255, 0);
    leds[ XY(2 + segX, 3)] = CRGB( 0, 255, 0);
    leds[ XY(0 + segX, 4)] = CRGB( 0, 255, 0);
    leds[ XY(1 + segX, 4)] = CRGB( 0, 255, 0);
    leds[ XY(2 + segX, 4)] = CRGB( 0, 255, 0);    
    }

  if (direct == "down") {
    leds[ XY(0 + segX, 1)] = CRGB( 255, 0, 0);
    leds[ XY(2 + segX, 1)] = CRGB( 255, 0, 0);
    leds[ XY(0 + segX, 3)] = CRGB( 255, 0, 0);
    leds[ XY(1 + segX, 3)] = CRGB( 255, 0, 0);
    leds[ XY(2 + segX, 3)] = CRGB( 255, 0, 0);
    leds[ XY(0 + segX, 4)] = CRGB( 255, 0, 0);
    leds[ XY(2 + segX, 4)] = CRGB( 255, 0, 0);
    FastLED.show();
    delay(500);
    leds[ XY(2 + segX, 2)] = CRGB( 0, 0, 255);
    FastLED.show();
    delay(500);
    leds[ XY(2 + segX, 2)] = CRGB( 0, 0, 0);
    leds[ XY(2 + segX, 3)] = CRGB( 0, 0, 255);
    FastLED.show();
    delay(500);
    leds[ XY(2 + segX, 3)] = CRGB( 255, 0, 0);
    leds[ XY(2 + segX, 4)] = CRGB( 0, 0, 255);
    FastLED.show();
    delay(500);
    leds[ XY(2 + segX, 4)] = CRGB( 255, 0, 0);
        
    }
  if (direct == "s") {
    leds[ XY(0 + segX, 2)] = CRGB( 50, 50, 50);
    leds[ XY(1 + segX, 2)] = CRGB( 50, 50, 50);
    leds[ XY(2 + segX, 2)] = CRGB( 50, 50, 50);
    }
}


void setup() {
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness( BRIGHTNESS );
  Serial.begin(9600);
  // Add Wifi network to ESP
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(WIFI_SSID, WIFI_PASS);
  Serial.println("--- Start Serial Monitor SEND_RCVE ---");

doL(SEG1, "C", 255, 0, 255);
doL(SEG2, "O", 0, 255, 255);
doL(SEG3, "I", 255, 255, 0);
doL(SEG4, "N", 0, 0, 255);
doL(SEG5, "S", 255, 0, 0);
FastLED.show(); 
delay(1000);
FastLED.clear();
doL(SEG1, "O", 0, 0, 255);
doL(SEG2, "N", 0, 0, 255);
FastLED.show(); 
delay(1000);
FastLED.clear();
doL(SEG1, "W", 0, 50, 150);
doL(SEG2, "I", 0, 50, 150);
doL(SEG3, "F", 0, 50, 150);
doL(SEG4, "I", 0, 50, 150);
FastLED.show(); 
delay(1000);
FastLED.clear();
Serial.println("Starting up... moving on...");

// This is to test all your LEDs if you like, lights 4x5 segments different colors. 
//doNum(SEG1, "", 0, 100);
//doNum(SEG2, "", 100, 0);
//doNum(SEG3, "", 50, 50);
//doNum(SEG4, "", 75, 20);
//doNum(SEG5, "", 200, 200);
//FastLED.show(); 
//delay(10000);
//FastLED.clear();

}

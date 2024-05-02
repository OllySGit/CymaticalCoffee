#include "MCP4725.h"
#include "Wire.h"
#include <RotaryEncoder.h>
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

const int TFT_CS = 6;
const int TFT_RST = 5;
const int TFT_DC = 9;
uint16_t freq = 20;
uint32_t period = 0;
uint32_t halvePeriod = 0;
int8_t thevol = 20;
volatile bool volumeChanged = false;

#define MAX9744_I2CADDR 0x4B
#define MAX9744_I2CADDR2 0x4A
#define RGB_PIN 5         // Pin number to which NeoPixels are connected
#define NUM_RGBPIXELS 24  // Number of NeoPixels

// Initialize NeoPixel object with number of pixels and pin number
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_RGBPIXELS, RGB_PIN, NEO_GRB + NEO_KHZ800);
uint32_t color1 = pixels.Color(0, 255, 0);
uint32_t color2 = pixels.Color(255, 0, 0);

uint16_t count;
uint32_t lastTime = 0;

//Declare encoders
RotaryEncoder *volEncoder = new RotaryEncoder(12, 13, RotaryEncoder::LatchMode::FOUR3);
RotaryEncoder *freqEncoder = new RotaryEncoder(10, 11, RotaryEncoder::LatchMode::FOUR3);
//declare screen
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

MCP4725 MCP1(0x62);
MCP4725 MCP2(0x63);
MCP4725 MCP3(0x64);
MCP4725 MCP4(0x65);
uint16_t sine[361];
uint16_t cosWave[361];

enum Rotation {
  ROTATION_0,
  ROTATION_90,
  ROTATION_180,
  ROTATION_270
};

void setup() {
  // put your setup code here, to run once:
  tft.begin();
  tft.setRotation(ROTATION_270);  // Adjust rotation as needed
  tft.fillScreen(ILI9341_WHITE);  // Set background to white

  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(2.5);
  String title = "Coffee Art by Olly";
  int16_t titleWidth = title.length() * 6;  // Estimate width (6 pixels per character)
  tft.setCursor(10, 10);
  tft.println(title);

  // Draw boxes and labels centered
  drawBox((tft.width() - 220) / 2, 50, "Frequency", freq);
  drawBox((tft.width() - 220) / 2, 120, "Volume", thevol);
  drawBox((tft.width() - 220) / 2, 190, "Phase", 10);

  updateValue(30, 100, freq);
  updateValue(30, 170, thevol);
  updateValue(30, 220, 10);

  MCP1.begin();
  MCP2.begin();
  MCP3.begin();
  MCP4.begin();
  Wire.begin();
  setvolume(0);
  pixels.begin();
  for (int i = 0; i < 361; i++) {
    sine[i] = 2047 + round(2047 * sin(i * PI / 180));
    cosWave[i] = 2047 + round(2047 * cos(i * PI / 180));
  }
  attachInterrupt(12, volEncoderInterrupt, CHANGE);  //interrupt triggered on any change
  attachInterrupt(13, volEncoderInterrupt, CHANGE);
  attachInterrupt(10, freqEncoderInterrupt, CHANGE);  //interrupt triggered on any change
  attachInterrupt(11, freqEncoderInterrupt, CHANGE);
  Serial.begin(9600);
  Wire.setClock(2000000);

  volEncoder->setPosition(thevol);
  freqEncoder->setPosition(freq);

  for (int i = 0; i < 12; i++) {
    pixels.setPixelColor((2*i), color1);
    pixels.setPixelColor((2*i + 1), color2);
  }
  pixels.show();  // Display the updated colors on the pixels
  Serial.print("Frequency: ");
  Serial.println(freq);
  period = 1e6 / freq;
  halvePeriod = period / 2;

  if (!setvolume(thevol)) {
    Serial.println("Failed to set volume, MAX9744 not found!");
  }
}

void loop() {
  if (volumeChanged) {
    volumeChanged = false;
    if (!setvolume(thevol)) {
      Serial.println("Failed to set volume, MAX9744 not found!");
    }
  }
  yield();
  uint32_t now = micros();
  count++;
  if (now - lastTime > 100000) {
    lastTime = now;
    count = 0;
    period = 1e6 / freq;
    halvePeriod = period / 2;
    //Serial.print("freq: ");
    //Serial.println(freq);
  }
  uint32_t t = now % period;
  int idx = (360 * t) / period;
  int value = sine[idx];  //  fetch from lookup table
  MCP1.setValue(value);
  MCP2.setValue(value);
  MCP3.setValue(value);
  MCP4.setValue(value);
}

void volEncoderInterrupt() {

  volEncoder->tick();
  thevol = volEncoder->getPosition();

  // Ensure freq is at least 1
  if (thevol < 10) {
    thevol = 10;
    volEncoder->setPosition(10);
  }

  // Limit freq to 64
  if (thevol > 63) {
    thevol = 63;
    volEncoder->setPosition(63);
  }


  volumeChanged = true;

  updateValue(30, 170, thevol);
}

boolean setvolume(int8_t v) {
  // cant be higher than 63 or lower than 0

  Serial.print("Setting volume to ");
  Serial.println(v);
  Wire.beginTransmission(MAX9744_I2CADDR);
  Wire.write(v);
  Wire.endTransmission(MAX9744_I2CADDR);
  Wire.beginTransmission(MAX9744_I2CADDR2);
  Wire.write(v);
  Wire.endTransmission(MAX9744_I2CADDR2);
  return true;
}

void freqEncoderInterrupt() {
  freqEncoder->tick();
  freq = freqEncoder->getPosition();
  if (freq < 1) {
    freq = 1;
    freqEncoder->setPosition(1);
  }

  // Limit freq to 64

  //Serial.print("Frequency: ");
  //Serial.println(freq);

  updateValue(30, 100, freq);
}

void drawBox(int x, int y, String label, int value) {
  tft.fillRect(x, y, 220, 50, ILI9341_BLUE);  // Draw box
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(x + 10, y + 15);
  tft.println(label);
}

void updateValue(int x, int y, float value) {
  tft.fillRect(x, y, 220, 30, ILI9341_WHITE);  // Clear previous value
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(x + 10, y + 5);
  tft.println(value);
}
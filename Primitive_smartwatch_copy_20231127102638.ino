#include <U8glib.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <RtcDS3231.h>

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);

#define PIN_NEOPIXEL 6
#define NUM_PIXELS 24   // Define the number of pixels in your NeoPixel ring
#define PIXEL_BRIGHTNESS 255 // Adjust brightness (0-255)

// KY-039 Pulse Sensor (Analog)
#define KY039_SENSOR A0

// Temperature sensor pin
#define PIN_TEMP_SENSOR A1

// RTC Clock module
RtcDS3231<TwoWire> Rtc(Wire);

#define GRAPH_WIDTH 44  // Width of the graph on the OLED display
#define GRAPH_HEIGHT 22 // Height of the graph on the OLED display (top half)
#define GRAPH_X_OFFSET 4 // X offset of the graph on the OLED display
#define GRAPH_Y_OFFSET 1 // Y offset of the graph on the OLED display
#define GRAPH_SCALE_FACTOR 1 // Adjust this value to scale the graph

int heartRateData[GRAPH_WIDTH] = {0}; // Array to store heart rate data

// NeoPixel initialization
Adafruit_NeoPixel neopixel = Adafruit_NeoPixel(NUM_PIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// Variables for pulse sensor calibration
int sensorMin = 1023;  // Minimum sensor value
int sensorMax = 0;     // Maximum sensor value

void setup() {
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  Rtc.SetDateTime(compiled);
  Serial.begin(9600);

  // Initialize pulse sensor calibration
  while (millis() < 2000) {
    int pulseSensorValue = analogRead(KY039_SENSOR);

    // Record the maximum sensor value
    if (pulseSensorValue > sensorMax) {
      sensorMax = pulseSensorValue;
    }

    // Record the minimum sensor value
    if (pulseSensorValue < sensorMin) {
      sensorMin = pulseSensorValue;
    }
  }

  // Neopixel Initialization
  neopixel.begin();
  neopixel.show();

  // OLED Display Initialization
  u8g.setFont(u8g_font_unifont);
  u8g.setColorIndex(1);

  // RTC Initialization
  Rtc.Begin();
}

void loop() {
  // Read the analog data from KY-039 Pulse Sensor
  int pulseSensorValue = analogRead(KY039_SENSOR);
  // Apply calibration to the sensor reading
  pulseSensorValue = constrain(pulseSensorValue, sensorMin, sensorMax);
  int heartRate = map(pulseSensorValue, sensorMin, sensorMax, 60, 200);

  // Get the current RTC time
  RtcDateTime now = Rtc.GetDateTime();

  // Get the temperature from the RTC module
  RtcTemperature rtcTemp = Rtc.GetTemperature();
  float rtcTemperatureC = rtcTemp.AsFloatDegC();

  // Update the heart rate data array with the latest value
  for (int i = 0; i < GRAPH_WIDTH - 1; i++) {
    heartRateData[i] = heartRateData[i + 1];
  }
  heartRateData[GRAPH_WIDTH - 1] = heartRate;

  // Clear the OLED screen
  u8g.firstPage();
  do {
    u8g.setFont(u8g_font_unifont);
    // Display temperature
    u8g.drawStr(0, 40, "Temp: ");
    u8g.setPrintPos(60, 40);
    u8g.print(rtcTemperatureC);
    u8g.print("C");

    // Display time
    u8g.drawStr(0, 56, "Time: ");
    u8g.setPrintPos(60, 56);
    u8g.print(now.Hour());
    u8g.print(":");
    u8g.print(now.Minute());

    // Display the heart rate graph on the OLED display (top half)
    for (int i = 0; i < GRAPH_WIDTH - 1; i++) {
      int x1 = i + GRAPH_X_OFFSET;
      int y1 = GRAPH_HEIGHT - map(heartRateData[i], 60, 200, 0, GRAPH_HEIGHT / GRAPH_SCALE_FACTOR) + GRAPH_Y_OFFSET;
      int x2 = i + 1 + GRAPH_X_OFFSET;
      int y2 = GRAPH_HEIGHT - map(heartRateData[i + 1], 60, 200, 0, GRAPH_HEIGHT / GRAPH_SCALE_FACTOR) + GRAPH_Y_OFFSET;
      u8g.drawLine(x1, y1, x2, y2);
    }

    // Send heart rate data to serial plotter
    Serial.println(heartRate);

  } while (u8g.nextPage());

  // Update NeoPixel colors based on heart rate
  for (int i = 0; i < NUM_PIXELS; i++) {
    // Set color based on heart rate (adjust colors as needed)
    uint32_t color = getColorForHeartRate(heartRate);
    neopixel.setPixelColor(i, color);
  }
  neopixel.show();
}

// Define a function to map heart rate to a color
uint32_t getColorForHeartRate(int heartRate) {
  if (heartRate < 80) {
    return neopixel.Color(0, 255, 0);  // Green for low heart rate
  } else if (heartRate < 120) {
    return neopixel.Color(255, 165, 0); // Orange for moderate heart rate
  } else {
    return neopixel.Color(255, 0, 0);   // Red for high heart rate
  }
}

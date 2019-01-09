#include <LiquidCrystal.h>
#include <TimeLib.h>
#include <dht.h>


LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

dht DHT;

#define DHT11_PIN 7
// For creating time request
#define TIME_HEADER  "T"   // Header tag for serial time sync message
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message

// State variable will keep track on the display view
// 0 for "Home View, current humidity and temp"
// 1 for Historical average
int viewState = 0;
// Button press state
int switchState = 0;

// Measured and calculated values
struct MeasurementValues
{
  float currentTemp;
  float currentHumidity;
  float averageTemp;
  float averageHum;
};

// Measurement values from last 30 days
struct AverageValues
{
  float temperatures[30];
  float humidities[30];
};

// Initialize all to zero
MeasurementValues values = {0.0, 0.0, 0.0, 0.0};
// Values set for debugging
AverageValues averages = { {21.0, 17.9, 23.2, 19.8, 22.1}, {37.2, 28.4, 19.9, 27.3, 25.0} };


void updateCurrentValues()
{
  // Get new measurements and update
  int measurement = DHT.read11(DHT11_PIN);
  delay(1500);
  values.currentTemp = DHT.temperature;
  values.currentHumidity = DHT.humidity;
}


void calculateAverages()
{
  float tempTotal;
  float humTotal;
  
  int tempValidCount = 0;
  int humValidCount = 0;
  
  for (int i = 0; i < 30; i++) 
  {
    if (averages.temperatures[i] != 0)
    {
      tempTotal += averages.temperatures[i];
      tempValidCount++;
    }
    
    if (averages.humidities[i] != 0)
    {
      humTotal += averages.humidities[i];
      humValidCount++;
    }
  }
  
  values.averageTemp = tempTotal / tempValidCount;
  values.averageHum = humTotal / humValidCount;
}


void printCurrentValues()
{
  // Update the LCD values
  lcd.setCursor(0,0); 
  lcd.print("Temp: ");
  lcd.print(values.currentTemp);
  //lcd.print(char(223));
  lcd.print(" °C");

  lcd.setCursor(0,1); 
  lcd.print("Humidity: ");
  lcd.print(values.currentHumidity);
  lcd.print("%");
}


void printAverages()
{
  Serial.println();
  
  lcd.setCursor(0,0); 
  lcd.print("Avg.Temp: ");
  lcd.print(values.averageTemp);
  //lcd.print(char(223));
  lcd.print("C");

  lcd.setCursor(0,1); 
  lcd.print("Avg.Hum: ");
  lcd.print(values.averageHum);
  lcd.print("%");
}


// For setting up time
// Copied directly from library example
void processSyncMessage()
{
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

  if (Serial.find(TIME_HEADER))
  {
     pctime = Serial.parseInt();
     if( pctime >= DEFAULT_TIME)
     { // check the integer is a valid time (greater than Jan 1 2013)
       setTime(pctime); // Sync Arduino clock to the time received on the serial port
     }
  }
}


// Request for pc to get the time
// Copied directly from library example
time_t requestSync()
{
  Serial.write(TIME_REQUEST);  
  return 0; // the time will be sent later in response to serial mesg
}


void setup()
{
  Serial.begin(9600);
  pinMode(2, INPUT);

  // For setting up the time
  setSyncProvider(requestSync);  // set function to call when sync required
  Serial.println("Waiting for sync message");

  lcd.begin(16, 2);
  
  updateCurrentValues();
  calculateAverages();
  printCurrentValues();
}


void loop()
{
  // Booleans for determining if new values need to be fetched
  static bool validCurrents = false;
  static bool validAverages = false;
  static bool clearedScreen = false;
  static bool timeSynced = false;
  static int previousHour = 0;
  static int prevSecond = 0;

  // Check if the time has been synced by the pc
  if (!timeSynced)
  {
    if (Serial.available())
    {
      // Sync needs to happen only once
      Serial.println("Processing sync message");
      processSyncMessage();
      previousHour = hour();
      timeSynced = true;
      Serial.println("Time has been synced to the board");
    }
  }

  if (prevSecond - second() > 15 || second() - prevSecond > 15)
  {
    updateCurrentValues();
    prevSecond = second();
  }

  // TODO - get measurements during the day -> add to the array later that day
  if (previousHour != hour())
  {
    // Take measurement
    // Update to the arrays for the current day
    previousHour = hour();
  }
  
  // Check the switch position
  switchState = digitalRead(6);


  // Check if button is pressed
  // This triggers the change for a differen view in LCD
  if (switchState == HIGH)
  {
    if (viewState == 0)
    {
      viewState = 1;
      lcd.clear();
    } else {
      viewState = 0;
      lcd.clear();
    }
  }
  
  
  // Current temperatures will be displayed if state is 0
  if (viewState == 0)
  {
    if (!validCurrents)
    {
      updateCurrentValues();
      validCurrents = true;
    }
    
    printCurrentValues();
  }
  
  // Average values from the last 30 days will be printed
  if (viewState == 1)
  {
    if (!validAverages)
    {
      calculateAverages();
      validAverages = true;
    }
    
    printAverages();
  }
  
}
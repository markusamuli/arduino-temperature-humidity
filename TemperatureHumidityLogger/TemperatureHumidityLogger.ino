// Link for getting the 10 digit time for doing the serial time sync
// https://www.unixtimestamp.com/index.php

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
  // Arrays for storing the values for the month
  float temperatures[30];
  float humidities[30];

  // Variables for tracking the measurements during the day
  float tempsForTheDay[24];
  float humidsForTheDay[24];
  int hourUpdateCounter;
};

// Initialize all to zero
MeasurementValues values = {0.0, 0.0, 0.0, 0.0};
// Values set for debugging
AverageValues averages = { {0}, {0}, {0}, {0}, 0 };


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

  if (tempValidCount == 0 || humValidCount == 0)
  {
    values.averageTemp = 0;
    values.averageHum = 0;
  } else {
    values.averageTemp = tempTotal / tempValidCount;
    values.averageHum = humTotal / humValidCount;
  }
  
}


void calculateDayAverages(int* arrayIndex)
{
  float tempTotal;
  float humTotal;
  
  int tempValidCount = 0;
  int humValidCount = 0;
  
  for (int i = 0; i < 24; i++) 
  {
    if (averages.tempsForTheDay[i] != 0)
    {
      tempTotal += averages.tempsForTheDay[i];
      tempValidCount++;
    }
    
    if (averages.humidsForTheDay[i] != 0)
    {
      humTotal += averages.humidsForTheDay[i];
      humValidCount++;
    }
  }

  if (tempValidCount == 0 || humValidCount == 0)
  {
    averages.temperatures[*arrayIndex] = 0;
    averages.humidities[*arrayIndex] = 0;
  } else {
    averages.temperatures[*arrayIndex] = tempTotal / tempValidCount;
    averages.humidities[*arrayIndex] = humTotal / humValidCount; 
  }
}


void printCurrentValues()
{
  // Update the LCD values
  lcd.setCursor(0,0); 
  lcd.print("Temp: ");
  lcd.print(values.currentTemp);
  //lcd.print(char(223));
  lcd.print(" C");

  lcd.setCursor(0,1); 
  lcd.print("Humidity: ");
  lcd.print(values.currentHumidity);
  lcd.print("%");
}


void printAverages()
{
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

  // Values for tracking the time
  static int measurementInterval = 10;
  static int dayIndex = 0;
  static int prevDay = 0;
  static int previousHour = 0;
  static int prevSecond = 0;

  // Check if the time has been synced by the pc
  // Sync needs to happen only once
  // Copied from the library example
  if (!timeSynced)
  {
    if (Serial.available())
    {
      Serial.println("Processing sync message");
      processSyncMessage();
      previousHour = hour();
      timeSynced = true;
      Serial.println("Time has been synced to the board");
    }
  }

  // If day is different, calculate new averages
  if (prevDay != day())
  {
    calculateAverages();
    prevDay = day();
  }

  // Check if the measurement values have been updated recently
  if (prevSecond - second() > measurementInterval 
      || second() - prevSecond > measurementInterval)
  {
    updateCurrentValues();
    prevSecond = second();
  }

  // TODO - get measurements during the day -> add to the array later that day
  if (previousHour != hour())
  {
    // Check if all the values for the day are collected
    // If yes, mean for those values can be calculated
    if (averages.hourUpdateCounter == 24)
    {
      calculateDayAverages(&dayIndex);
      if (dayIndex == 29)
      {
        dayIndex = 0;
      } else {
        dayIndex++;
      }
      averages.hourUpdateCounter = 0;
    } else {
      // Take measurement and add for the measurements for the current day
      averages.tempsForTheDay[hour()-1] = values.currentTemp;
      averages.humidsForTheDay[hour()-1] = values.currentHumidity;
      averages.hourUpdateCounter++;
    }
    previousHour = hour();
  }
  
  // Check the switch position
  switchState = digitalRead(6);


  // Check if button is pressed
  // This triggers the change for a different view in LCD
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
    printCurrentValues();
  }
  
  // Average values from the last 30 days will be printed
  else if (viewState == 1)
  {    
    printAverages();
  }
  
}

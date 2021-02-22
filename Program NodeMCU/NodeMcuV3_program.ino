#include <Wire.h>
#include <SoftwareSerial.h>

SoftwareSerial hc06(3,1);

#define timerIntervalReg 100
#define timerIntervalTemp 1000000
#define numberOfSteps 100

const int voltagePin = A0;
const int pwmPin = 0;

// D3 -> 0
// D8 -> 15
// D7 -> 13

int voltageLevel = 0; 
int dutyCycle = 0;
int waveStep = 0;

int temperatureAdd = 72;
int temperatureCelcius = 0;

unsigned long currentMillis = 0;
unsigned long previousMillisReg = 0;
unsigned long previousMillisTemp = 0;
unsigned long previousMillisFun = 0;

char rxBuff[100];
char rxData = 0;
int rxIndex = 0;
bool transferFlag = false;

double functionPeriod = 0;
double functionStep = 0;

int frequencyBT = 10;
int pwmBT = 50;
int signalTypeBT = 2;

// 0 - dc
// 1 - sine
// 2 - triangle
// 3 - square

int calculatePWM(int pwmBT)
{
  if (pwmBT < 0) pwmBT = 0;
  if (pwmBT > 100) pwmBT = 100;
  return int((pwmBT * 1024) / 100);
}

void readData()
{
  if (rxIndex == 0) 
  {
    for (int i = 0; i < 100; i++)
    {
      rxBuff[i] = 0;
    }
  }
  rxData = (char)hc06.read();
  if (rxData != 64) 
  {
    rxBuff[rxIndex] = rxData;
    rxIndex++;
    if (rxIndex > 100) rxIndex = 0;
  }
  else 
  {
    rxIndex = 0;
    //transferFlag = true;
    hc06.flush();
  }
}

double calculatePeriod(int freqBT)
{
  return 1 / freqBT;
}

void setup()
{
  Serial.begin(9600);  
  Wire.begin();
  hc06.begin(9600);
  pinMode(pwmPin, OUTPUT);
  pinMode(voltagePin, INPUT);
}

void loop()
{
  if (hc06.available()) readData();
  currentMillis = micros();
  if (currentMillis - previousMillisReg >= timerIntervalReg)
  {
    previousMillisReg = currentMillis;
    voltageLevel = analogRead(voltagePin);
    dutyCycle = calculatePWM(pwmBT);
    functionPeriod = calculatePeriod(frequencyBT);
    functionStep = functionPeriod / numberOfSteps;
    if (signalTypeBT == 0)
    {
      analogWrite(pwmPin, dutyCycle); 
    }
    else if (signalTypeBT == 1)
    {
      if (currentMillis - previousMillisFun >= functionStep)
      {
        previousMillisFun = currentMillis;
        analogWrite(pwmPin, dutyCycle * sin((2 * PI * waveStep) / numberOfSteps) + dutyCycle);
        waveStep++;
      }
    }
    else if (signalTypeBT == 2)
    {
      if (currentMillis - previousMillisFun >= functionStep)
      {
        previousMillisFun = currentMillis;
        if ((waveStep >= 0) && (waveStep < 51))
        {
          analogWrite(pwmPin, (dutyCycle * waveStep) / (numberOfSteps / 2));
        }
        if ((waveStep >= 51) && (waveStep < 101))
        {
          analogWrite(pwmPin, (dutyCycle * (100 - waveStep)) / (numberOfSteps / 2));
        }
        waveStep++;
      }
    }
    else if (signalTypeBT == 3)
    {
      if (currentMillis - previousMillisFun >= functionStep)
      {
        previousMillisFun = currentMillis;
        if ((waveStep >= 0) && (waveStep < 51))
        {
          analogWrite(pwmPin, dutyCycle);
        }
        if ((waveStep >= 51) && (waveStep < 101))
        {
          analogWrite(pwmPin, 0);
        }
      }
    }
    if (waveStep >= numberOfSteps) waveStep = 0;
  }
  if (currentMillis - previousMillisTemp >= timerIntervalTemp)
  {
    previousMillisTemp = currentMillis;
    Wire.beginTransmission(temperatureAdd);
    Wire.write(0);
    Wire.requestFrom(temperatureAdd, 1);
    while (Wire.available() == 0);
    temperatureCelcius = Wire.read(); 
    char temp[2];
    temp[1] = char(temperatureCelcius % 10) + '0';
    temp[0] = char((temperatureCelcius - (temperatureCelcius % 10)) / 10) + '0';
    hc06.write(temp);
  }
  if (1)
  {
    if ((rxBuff[0] == 'X') && (rxBuff[1] == 'D') && (rxBuff[2] == 'U') && (rxBuff[3] == 'A') && (rxBuff[4] == 'D'))
    {
      sscanf(rxBuff, "XDUAD%d-%d-%d", &pwmBT, &frequencyBT, &signalTypeBT);
      //transferFlag = false;
      if (frequencyBT == 0) frequencyBT = 1;
    }
  }
}
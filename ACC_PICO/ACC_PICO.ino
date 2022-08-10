#define PIN_SD_MOSI       PIN_SPI0_MOSI
#define PIN_SD_MISO       PIN_SPI0_MISO
#define PIN_SD_SCK        PIN_SPI0_SCK
#define PIN_SD_SS         PIN_SPI0_SS

#define _RP2040_SD_LOGLEVEL_       0

#include <SPI.h>
#include <FreeRTOS.h>
#include <RP2040_SD.h>

#define Xpin 26
#define Ypin 27
#define Zpin 28

#define LED 2


String dataToSend = "";

//Thread synchronization
bool dataReady = false;
bool dataSend = true;

//Actual values of x,y,z acceleration
unsigned int x = 0;
unsigned int y = 0;
unsigned int z = 0;

//File organization
int fileCount = 1;
int fileNumber = 1;


void readAcc(unsigned int *x, unsigned int *y, unsigned int *z)
{
  int xs = 0;
  int ys = 0;
  int zs = 0;

  int i = 0;
  while(i<10)
  {
    xs += analogRead(Xpin);
    ys += analogRead(Ypin);
    zs += analogRead(Zpin);

    i++;
  }
  *x = xs/10;
  *y = ys/10;
  *z = zs/10;
}

String fileName()
{
  return String(fileCount)+".txt";
}

//Setup of 1st thread
void setup() {

  pinMode(Xpin, INPUT);
  pinMode(Ypin, INPUT);
  pinMode(Zpin, INPUT);
  analogReadResolution(12);

  // see if the card is present and can be initialized:
  if (!SD.begin(PIN_SD_SS)) 
    {
      return;
    }

  delay(10);

  // see if other files exist and write down the file number
  while(SD.exists(fileName()))
  {
    fileCount++;
  }
  fileNumber = fileCount;

  // write header to file
  File dataFile = SD.open(fileName(), FILE_WRITE);
  if (dataFile) {
    dataFile.println("Pomiar nr " + String(fileCount));
    dataFile.println("");
    dataFile.close();
  }

}

// loop of 1st thread:
// value measurements
void loop() {
  if(dataSend){

      dataSend = false;
      int i=0;
      String dataString = "";
      while(i<600)
      {
        unsigned long czas = micros();
        readAcc(&x,&y,&z);
        dataString += String(czas) + ";" + String(x) + ";" + String(y) + ";" + String(z)+ "\n";
        delayMicroseconds(200);
        
        i++;
      }
      dataToSend = dataString;
      dataReady = true;
  }
}

// setup of 2nd thread
void setup1(){
  pinMode(LED, OUTPUT);
}

unsigned long lastDiodeTime=0;
unsigned long lastFileNameChangedTime=0;
int period = 1000;
int periodName = 1800000;
bool diodeState = false;

// loop of 2nd thread:
// saving data on a memory card
void loop1()
{

  if(dataReady){
    dataSend = true;
    dataReady = false;
    dataToSend.remove(dataToSend.length()-1);

    File dataFile = SD.open(fileName(), FILE_WRITE);

    // if the file is available, write to it
    if (dataFile) {
      dataFile.println(dataToSend);
      dataFile.close();
      dataToSend = "";
      period = 1000;
    
    }
    else {
      period = 140;
    }

  }
  else // signaling the correct operation of the device
  {
    unsigned long actualTime = millis();
    if(lastDiodeTime + period < actualTime)
    {
      lastDiodeTime = actualTime;
      if(diodeState)
      {
        digitalWrite(LED,LOW);
        diodeState = false;
      }
      else
      {
        digitalWrite(LED,HIGH);
        diodeState = true;
      }

      if(lastFileNameChangedTime + periodName < actualTime) // new file on every +- 30 minutes
      {
        lastFileNameChangedTime = actualTime;

        fileCount++;

        File dataFile = SD.open(fileName(), FILE_WRITE);

        if (dataFile) {
          dataFile.println("cd... " + String(fileNumber));
          dataFile.println("");
          dataFile.close();
        }
      }
    }
  }
}








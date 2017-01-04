
/*
Project developed by Angad Daryani for IBDP Computer Science Internal Assessment 2016. 
School : Jamnabai Narsee International School          March 2016
Air quality measuring device with onboard LCD and SD card datalogging. 
Code developed through the reference of Open Source examples online. 
 
 Hardware and pins used : 
 - Arduino Mega R3 2560
 - TinyRTC Module 
 - Adafruit Micro SD card breakout board
 - 16x2 Character Parallel Liquid Crystal display
 - Push Button
 - SHARP GP2Y1010AU0F compact optical dust sensor
 - MQ7B Carbon Monoxide Sensor

 Use a separate functions to open SD file, sort the elements in 3 arrays and then send each array to the linear search function

 RTC : 
 SDA - A4
 SCL - A5

 SD Card:
 CLK - D52
 DO - D50
 DI - D51
 CS - D53
 CD - D13

 Button - D30

 LCD:
  4 - D2
  6 - D3
  11 - D4
  12 - D5
  13 - D6
  14 - D7
 
 CO Sensor - A1 

 Dust Sensor:
 LED - D31
 Data - A0
 
 */

#include <Time.h> // library for keeping track of time
#include <TimeLib.h> // library for keeping track of time
#include <LiquidCrystal.h> // library for running the LCD display
#include <SD.h> // library to read and write data from the SD card
#include <Wire.h> // library to read/write time and date from RTC
#include "RTClib.h" //library to interface with the TinyRTC
#include <SPI.h> // library to use Serial Peripheral Interface

// list of variables
int maxc, maxi, maxd , maxm; // max carbonmonoxide, time for max carbonmonoxide , max dust value, time for max dust value
int dustled = 31; //connected to digital pin 31.
int dustval = 0; // value stored is 0, open to change.
int dustPin = 0; // connected to analog pin 0
String timen = ""; // time from RTC in one string
String daten = ""; // date from RTC in one string
String timedate =""; // date and time together for file name for SD Card

//delay time for various detection cycles of the dust sensor
int delayTime1 = 280; // for how long the first LED has to stay on
int delayTime2 = 40; // for how long the LED has to stay off
float Offtime = 9680; //time for the sensor to refresh

RTC_DS1307 RTC; // create an object named RTC for the RTC to run it through the library.
String year , month, day, hour , minute , second, time, date; 

//SD Card
const int chipSelect = 53; // defining the SD card write pin to digital pin 53;
int cardinsert = 13; //detect if a card is inserted. With pull up resistor, it will naturally show HIGH and is connected to pin 13 on the arduino. Although I should change the pin
File myFile; // Create a variable for file reading/writing/etc
String s = ""; // String which will save the data being read in the sorter() function and willl help save data into individual arrays.
char ch; // character which will run across the string to check for space and will save accordingly
int sctr = 0; // count the number of spaces
int s1loc, s2loc; // location of the first and second spaces

//button
int buttonpin = 30; // defining the pin (D30) to which the button is connected
int buttonstate = 0; //reading the state of the button
int lastbuttonstate = 0; // previous button state
int ctr = 0; // counter to count the number of times the button was pressed

//LCD
LiquidCrystal lcd(2,3,4,5,6,7); // initializing the library by defining the interface pins for the LCD

//CO Sensor
int autovoltagepin = 8; // the auto voltage regulator pin would be connected to digital pin 8
int COsensorpin = 1; // the input pin for this sensor is connected to analog pin 1
long CO5time = 60000; // time for which 5V should be supplied to the heater - in milliseconds
long CO14time = 90000; // time for which 1.4V should be supplied to the heater - in milliseconds
long COreadtime = 1000;  // time for which data can be read from the sensor before it needs to be heated again
unsigned long COval; // value of the data read from the sensor
unsigned long startMillis; // time at which the program is running a certain line may be a very large number in milliseconds hence it is an unsigned long
unsigned long switchTimeMillis; // time in milliseconds after which a switch in mode took place
boolean heaterInHighPhase; // if the heater is on at 5v (TRUE) or 1.4V(FALSE)

//------------------------------------------------------------------------------------------------------------------------------

void setup() // defining the input and outputs
{
  pinMode(dustled,OUTPUT);// defining the refracting LEDs in the dust sensor as an output
  pinMode(dustPin, INPUT); // accessing data from the Analog input  
  Wire.begin(); // start the wire library
  RTC.begin(); // start the RTC library
  if (!RTC.isrunning()) 
  {
    lcd.print("RTC off"); // print error message
    RTC.adjust(DateTime(__DATE__, __TIME__)); // this line sets the date and time from the time of code uploading
  }
  pinMode(chipSelect, OUTPUT); // defining the SD card write pin
  pinMode(cardinsert,INPUT); // defining the check SD card presence pin
  pinMode(buttonpin,INPUT); // defining the button pin
  lcd.begin(16,2) // defining the LCD to have 32 character divided over 2 rows. 
  pinMode (COsensorpin,INPUT); // reading data from the CO sensor
  pinMode (autovoltagepin , OUTPUT); // controlling the auto voltage IC for heating the sensor plate
  if(digitalRead(cardinsert = HIGH)) // if card is not inserted, the pin would naturally be high due to the pullup resistor
  {
   lcd.setCursor(0,1); // got to first position on the LCD. 
   lcd.print ("Card not present"); // print error message
   delay(1000); // let it be there for 1s
   return; // quit program
  }
  if(!SD.begin(53)) // if data cannot be read/written to the SD card
   {
    lcd.print("SD FAILED"); // print error message
    return; // quit program 
   }
}
//--------------------------------------------------------------------------------------------------------------------------------  

int Button()
{
  while ( buttonstate <=2) // till button is not pressed twice
  {
    digitalRead(buttonpin);
    if(buttonpin = HIGH) // if button is pressed
      buttonstate == buttonstate+1;
    while(ctr<=2) // while button is pressed less than 3 times
    {
      if(buttonstate ==1 && lastbuttonstate ==0) // if pressed once
     {
       lastbuttonstate = 1;
       ctr = ctr+1;  // increment counter
     }
    
   else if( buttonstate == 2 && lastbuttonstate ==1) // if button is pressed twice
     {
       lastbuttonstate = 2;
       ctr = 2; // increment counter
     }
     return ctr; // output the counter value
   }
   delay(20); // wait for 20 milliseconds so that more values are not immediately recorded
 }
 if(ctr ==2) // refresh counter to become 0, so that it can be used again. 
  ctr = 0;
}

//---------------------------------------------------------------------------------------------------------------------------

String LCD()
{
  Button(); //inheriting button function
  COsensor(); //inheriting COSensor function
  DustSensor(); //inheriting DustSensor function
  RTC(); //inheriting RTC function
  LinSearchDust(); // inheriting linear search function for dust sensor
  LinSearchCO(); // inheriting linear search funcion for CO sensor
  
  while(ctr==0)
  {
    lcd.setCursor(0,1); // go to the first position in the first row
    lcd.print("DS:");// DS indicating dust sensor
    lcd.print(dustval); // dust sensor value
    lcd.print("   "); // 3 spaces
    lcd.print("CO:"); // CO representing carbon monoxide
    lcd.print(COval); // CO sensor value
    lcd.setCursor(0,2); // go to first position in the second row
    lcd.print("Time:"); // print time at that instant
    lcd.print(timen); // time from RTC
  }
  
  if(ctr == 1) // if button is pressed once, start recording
  {
    lcd.clear(); // clear out lcd and return to 0,1
    lcd.print("Recording data"); // print message
  }
  else if(ctr == 2) // if butotn is pressed twice, stop recording
  {
    lcd.clear(); // clear LCD screen 
    lcd.setCursor(0,1); // go to 0,1
    lcd.print("Data Recorded"); // print message
    lcd.setCursor(0,2); // go to next line
    lcd.print("@"); // stands for AT
    lcd.print(timen); // print time 
    delay(100); // wait for 1 second 
    lcd.clear(); // clear LCD 
    lcd.setCursor(0,1); // return to 0,1
    lcd.print("maxd:"); // print max dust value
    lcd.print(maxd); 
    lcd.print("@");
    lcd.print(timea(maxm)); // print time at which max value for dust took place
    lcd.setCursor(0,2); // go to next line
    lcd.print("maxc"); // print max CO value
    lcd.print(maxc);
    lcd.print("@");
    lcd.print(timea(maxi)); // print time at which max value for CO took place.
  } 
}

//----------------------------------------------------------------------------------------------------------------------------

int COsensor()
{
  /* this sensor works due to a chemical reaction of the crystalline heater plate once it goes through a heating cycle 
     of 60s HIGH and 90s LOW, with Carbon Monoxide in the air. Since this sensor requires 150mA of current for its heater,
     it's more than what the arduino can provide on each pin, hence we can use an external power source to make this sensor
     work effectively. However, since the recommended automatic voltage regulation IC was not locally available, I did not 
     use it and continued to do my IA since the only problem would be inconsistent data. However, since this was a proof of 
     concept and there would be many more future iterations to the same, the inconsistent data was acceptable to my client. 
  */

  if(heaterInHighPhase) // if heater is on
  {
    if(millis()>switchTimeMillis) // and if the heater has been on for longer that the switch time
    {
      digitalWrite(autovoltagepin, HIGH); // switch the heater mode
      heaterInPhase = false;  // switch the state to 1.4
      swichTimeMillis = millis()+ CO14time; // time at which the switch took place - MILLIS keeps time in milliseconds since the command was called
    }
  } 
    else 
    {
      if (millis() > switchTimeMillis) 
       {
          digitalWrite(autovoltagepin,LOW); // switch the heater mode
          heaterInPhase = true; // switch the state to 5v
          switchTimeMillis = millis() + CO5time; // time at which the switch took place again
       }
    }
    
    COval = analogRead(COsensorpin); // read data from the sensor
    delay (COreadtime);  // read for COreadtime duration
    return COval; // return the sensor value
}

//-----------------------------------------------------------------------------------------------------------------------------
int DustSensor()
{
  /*
   As mentioned in the data sheet of the sensor, the LED inside the sensor is switched on using a 
   PNP transistor (Positive Negative Positive), hence it would turn on, when there is low voltage
   provided to it. Thus, in order to switch on the LED, I have used a LOW (Off) command and to switch
   it off, I have used a HIGH (On) command.
  */
  
  digitalWrite(dustled,LOW); // switch on LED
  delayMicroseconds(delayTime1); // keep it on for 280 microseconds
  dustval=analogRead(dustPin); // read the input value from the dust sensor
  delayMicroseconds(delayTime2); // read the value for 40 microseconds
  digitalWrite(dustled,HIGH); // switch off the LED
  delayMicroseconds(Offtime); // keep it off for 9680 microseconds
  delay(3000); // pause for 3000 milliseconds
  return(dustval); // output the dust value
}

//--------------------------------------------------------------------------------------------------------------------------------

String RTC()
{
   DateTime datetime = RTC.now(); // create object datetime and 
   year = String (datetime.year(),DEC); // fetch the year from the RTC
   month = String (datetime.month(),DEC); // fetch the month from the RTC
   day = String (datetime.day(),DEC); // fetch the date from the RTC
   hour = String (datetime.hour(),DEC); // fetch the hour from the RTC
   minute = String (datetime.minute(),DEC); // fetch the minute from the RTC
   second = String(datetime.second(),DEC); // fetch the second from the RTC
   time = (hour + ":" + minute + ":" + second); // in order to output 1 string with time 
   date = (day + "/" + month + "/" + year); // output 1 string with date
   timedate = (time + "," + date); // time and date so that it can be used for file names
   delay(1000); // wait for 1 second 
   return time, date;
}

//---------------------------------------------------------------------------------------------------------------------------------
int LinSearchCO() // finding the maximum CO value
{
  sorter(); // import sorter function
  for(int i = 0; i<arr.length();i++)
  {
    maxc = arr(i);
    for(int j = i+1 ; j<=arr.length();j++)
    {
      if(arr(j)>maxc)
      maxc= arr(j);
      maxi = j;
    }
  }
  
  return maxc, maxi;
}

//-----------------------------------------------------------------------------------------------------------------------------------

int LinSearchDust() // finding the maximum dust value
{
  sorter(); // import sorter function
   
  for(int i = 0; i<arr1.length();i++)
  {
    maxd = arr1(i);
    for(int j = i+1 ; j<=arr1.length();j++)
    {
      if(arr1(j)>maxc)
      maxd= arr1(j);
      maxm = j;
    }
  }
  
  return maxd, maxm;
  
}

//-----------------------------------------------------------------------------------------------------------------------------------

String SDCard()
{
  RTC(); // inherit the RTC function
  Button(); // inherit the Button function
  COsensor(); //inherit the COSensor function
  DustSensor(); // inherit the DustSensor function
  LCD(); // inherit the LCD function
  
  myFile = SD.open("time.txt" , FILE_WRITE); // open a new file with the file name as the time.txt

  do // do while the button is pressed only once.
   {
    if(myFile) // if data is successfully being written to the file
     myFile.println( dustval + " " + COval + " " + timen); // write data to file line by line first..
     // ..with dust value, then space then CO value then space and then the time. 
   } while(ctr == 1 && ctr!=2);

   if(ctr ==2) // if the button is pressed twice, close the file
    myFile.close(); 
    
   else if(ctr !=0 && ctr!=1 && ctr!=2) // if the ctr value doesn't even show a 0, there must be an error
   { 
    lcd.print("Error in File"); //print error message and quit the program
    return;
   }
}
//---------------------------------------------------------------------------------------------------------------------------------------------------

 String sorter() //sorts the data in the SD card into 3 separate arrays
 {
  SDCard(); // inherit SD card function
  myFile = SD.open("time.txt");  // open the file
  int arr1[]= myFile.size(); // array for dust sensor data
  int arr[] = myFile.size(); // array for CO sensor data
  int timea[]= myFile.size(); // array for time data 
  
  if(myFile) // if the file exists
  {
    while(myFile.available()) // if file is available for reading and writing
    {
      for(int i = 0; i<myFile.size() ; i++) // run a for loop from 0 to less than the 
      //file size which is the same as the total number of elements
      {
        s = myFile.position(i); // the first string is the one at position 0
        
        for(int j = 0; i< s.length(); j++)  // check character by character for a space
        {
          ch = s.charAt(j); // start from the 1st position
          if ( s.charAt(j) == ' ' && sctr == 0) // if no space is detected before AND 
          //the character at this position is a space
           {
            s1loc = j; //store the index of the character in the string
            sctr = sctr+1; // increment the space counter
           }
          
         else if (s.charAt(j) == ' ' && sctr ==1) // if one space is already detected before and new space is detected
           {
            s2loc = j; // store index of the character
            break; // stop searching
           }

            for(int n = 0; n <s1loc; n++) // save all chars from the first position in the string till the space in arr 
             arr[j] = s.charAt(n);
            for(int m = s1loc; m <s2loc; m++) // save all chars from first space till the second space in arr1
             arr1[j] = s.charAt(m);
            for(int k = s2loc+1; k<s.length();k++)  // save all chars from 2nd space till the last in the string in timea
             timea[j] = s.charAt(k);
       }   
     }   
   }
  }

  return arr,arr1,timea; // return 3 arrays
 } 
  
//------------------------------------------------------------------------------------------------------------------------------------

void loop() // this loop repeats infintely inside the microcontroller
{
  RTC();// run the RTC function
  LCD(); // run the LCD function 
  Button(); //run the Button function
  SDCard(); // run the SDCard function
  sorter(); // sorts the data in the SD card into 3 separate arrays
  COsensor(); // run the Carbon Monoxide Sensor function
  DustSensor(); // run the Dust Sensor function
  LinSearchDust(); // run a Linear search for the Dust sensor
  LinSearchCO(); // run a linear search for the Carbon Monoxide Sensor
}

#include <LiquidCrystal.h>
#include <Wire.h>
#include <DS1307new.h>
#include <EEPROM.h>
#include <AnalogButtons.h>
#include <avr/pgmspace.h>

int buttonPressed=0;

prog_char clock_0[] PROGMEM = "Year: ";  
prog_char clock_1[] PROGMEM = "Month: ";
prog_char clock_2[] PROGMEM = "Day: ";
prog_char clock_3[] PROGMEM = "Hour: ";
prog_char clock_4[] PROGMEM = "Minutes: ";
prog_char clock_5[] PROGMEM = "Seconds: ";

PROGMEM const char *config_table1[] = 
{   
  clock_0,
  clock_1,
  clock_2,
  clock_3,
  clock_4,
  clock_5};

prog_char alarm_0[] PROGMEM = "Hour: ";  
prog_char alarm_1[] PROGMEM = "Minute: ";
prog_char alarm_2[] PROGMEM = "Duration: ";
prog_char alarm_3[] PROGMEM = "Enabled: ";

PROGMEM const char *config_table2[] = 
{   
  alarm_0,
  alarm_1,
  alarm_2,
  alarm_3,};

char buffer[30];    // make sure this is large enough for the largest string it must hold

byte alarmsOnIcon[8] = {
  B00010,
  B00110,
  B11110,
  B11110,
  B11110,
  B00110,
  B00010,
};
byte soundIcon[8] = {
  B00100,
  B10010,
  B01010,
  B01010,
  B01010,
  B10010,
  B00100,
};

#define ANALOG_PIN 0

int settingMode=0;

long previousMillis = 0;
long interval = 1000;

AnalogButtons analogButtons(ANALOG_PIN, 30, &handleButtons);
Button buttonMode = Button(1, 200,300);
Button buttonUp = Button(2, 400, 600);
Button buttonDown = Button(3, 650, 800);

int addr = 0;
//Use to populate the EEPROM settings the first time
/*int myAlarms[]={
 6,0,23,0,0,59,5,1,10,0,0,1,
 7,0,23,0,0,59,5,1,10,0,0,1,
 8,0,23,0,0,59,5,1,10,0,0,1,
 9,0,23,0,0,59,5,1,10,0,0,1,
 10,0,23,0,0,59,5,1,10,0,0,1,
 11,0,23,0,0,59,5,1,10,0,0,1,
 12,0,23,0,0,59,5,1,10,0,0,1,
 13,0,23,0,0,59,5,1,10,0,0,1,
 14,0,23,0,0,59,5,1,10,0,0,1,
 15,0,23,0,0,59,5,1,10,0,0,1,
 16,0,23,0,0,59,5,1,10,0,0,1,
 17,0,23,0,0,59,5,1,10,0,0,1,
 18,0,23,0,0,59,5,1,10,0,0,1,
 19,0,23,0,0,59,5,1,10,0,0,1,
 20,0,23,0,0,59,5,1,10,0,0,1
 };*/

int buzzerDuration = 5;

LiquidCrystal lcd(2, 3, 4, 5, 6, 7, 8);

boolean alarmFlag = false;

#define MON_BUF_LEN 128

char mon_buf[MON_BUF_LEN];
uint8_t mon_buf_cnt = 0;
char *mon_curr;

uint8_t mon_is_date;
uint16_t mon_year;
uint8_t mon_month;
uint8_t mon_day;

uint8_t mon_is_time;
uint8_t mon_hour;
uint8_t mon_minute;
uint8_t mon_second;

void setup()  
{
  Serial.begin(9600);
  pinMode(9, OUTPUT);
  digitalWrite(9, HIGH); //turn on backlight
  pinMode(A1, OUTPUT);
  digitalWrite(A1, HIGH); //Vcc for keyboard
  pinMode(A2, OUTPUT);
  digitalWrite(A2, LOW); //GND for keyboard
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.setCursor(0,0);
  lcd.write("Menno Industries");
  lcd.setCursor(0,1);
  lcd.write("   Engineering");
  delay(3000);
  lcd.createChar(0, alarmsOnIcon);
  lcd.createChar(1, soundIcon);
  lcd.clear();
  analogButtons.addButton(buttonMode);
  analogButtons.addButton(buttonUp);
  analogButtons.addButton(buttonDown); 
  //RTC.fillByYMD(2013,05,31);
  //RTC.fillByHMS(7,11,30);
  //RTC.setTime();
  //RTC.startClock();

  //Use to populate the EEPROM settings the first time
  //for(int x = 0; x < 15*12; x++){
  //  EEPROM.write(x, myAlarms[x]);
  //}

}

void loop()
{

  analogButtons.checkButtons();
  if(buttonPressed==4){//wait for long press on any button to enter settings mode
    buttonPressed=0;
    clockSettings();
    alarmSettings();
  }

  unsigned long currentMillis = millis();

  if(currentMillis - previousMillis > interval) { //update alarms and screen once every second
    previousMillis = currentMillis;   
    displayDate(3,0);
    displayTime(4,1);
    checkAlarms();
    displayAlarmStatus();

  }
  //Serial.print("[memCheckMainLoop]");
  //Serial.println(freeRam());
}

void displayDate(int col, int row){
  RTC.getTime();
  lcd.setCursor(col,row);
  if(RTC.year<10){
    lcd.print(" ");
  }
  lcd.print(RTC.year, DEC);
  lcd.setCursor(col+4,row);
  lcd.print("/");
  if(RTC.month<10){
    lcd.print("0");
  }
  lcd.print(RTC.month, DEC);
  lcd.setCursor(col+7,row);
  lcd.print("/");
  if(RTC.day<10){
    lcd.print("0");
  }
  lcd.print(RTC.day, DEC);
}

void displayTime(int col, int row){

  RTC.getTime();
  lcd.setCursor(col,row);
  if(RTC.hour<10){
    lcd.print(" ");
  }
  lcd.print(RTC.hour, DEC);
  lcd.setCursor(col+2,row);
  lcd.print(":");
  if(RTC.minute<10){
    lcd.print("0");
  }
  lcd.print(RTC.minute, DEC);
  lcd.setCursor(col+5,row);
  lcd.print(":");
  if(RTC.second<10){
    lcd.print("0");
  }
  lcd.print(RTC.second, DEC);

}

void displayAlarmStatus(){

  if(alarmFlag==true){

    lcd.setCursor(14,1);
    lcd.write(byte(0));
    lcd.write(byte(1));

  }
  else {
    lcd.setCursor(14,1);
    lcd.write(byte(0));
    lcd.print(" ");
  }
  alarmFlag=false;
}

void checkAlarms(){

  //read in the alarms
  int myAlarms[15*12];
  for(int x = 0; x < 15*12; x=x++){
    myAlarms[x]=EEPROM.read(x);
  }

  for(int x=0; x<15*12; x=x+12){

    int h=myAlarms[x];
    int m=myAlarms[x+3];
    int d=myAlarms[x+6];
    int enable=myAlarms[x+9];
    
    RTC.getTime();

    if ( RTC.hour == h && RTC.minute == m && RTC.second < d && RTC.dow != 0 && RTC.dow != 6 && enable >0){
      alarmFlag=true;
    } 
  }
  //Serial.print("[memCheckcheckAlarms]");
  //Serial.println(freeRam());
}

void handleButtons(int id, boolean held)
{  
  if (held) {
    buttonPressed=4; 
  } 
  else{
    buttonPressed=id;
  }
}

void clockSettings(){
  boolean nextMenuFlag=false;
  RTC.getTime();
  int settingArray[]={
    RTC.year,2000,2050,RTC.month,1,12,RTC.day,1,31,RTC.hour,1,24,RTC.minute,0,59,RTC.second,0,59
  };
  lcd.clear();
  lcd.home();
  for(int x = 0; x < 6; x++){
    strcpy_P(buffer, (char*)pgm_read_word(&(config_table1[x]))); 
    nextMenuFlag=false;
    lcd.clear();
    lcd.home();
    lcd.print(buffer);
    lcd.print(settingArray[3*x]);
    int minValue=settingArray[3*x+1];
    int maxValue=settingArray[3*x+2];
    while(nextMenuFlag==false){
      analogButtons.checkButtons();
      switch (buttonPressed) {
      case 1:
        nextMenuFlag=true;
        buttonPressed=0;
        break;
      case 2:
        if(settingArray[3*x]>=maxValue){
          settingArray[3*x]=settingArray[3*x];
        }
        else{
          settingArray[3*x]=settingArray[3*x]+1;
        }
        lcd.clear();
        lcd.home();
        lcd.print(buffer);
        lcd.print(settingArray[3*x]);
        buttonPressed=0;
        break;
      case 3:
        if(settingArray[3*x]<=minValue){
          settingArray[3*x]=settingArray[3*x];
        }
        else{
          settingArray[3*x]=settingArray[3*x]-1;
        }
        lcd.clear();
        lcd.home();
        lcd.print(buffer);
        lcd.print(settingArray[3*x]);
        buttonPressed=0;
        break;
      default: 
        nextMenuFlag=false;
        buttonPressed=0;
      }
    }
    //Serial.print("[memCheckclockSettings]");
    //Serial.println(freeRam());
  }
  // Saving settings to RTC
  RTC.fillByYMD(settingArray[0*3],settingArray[1*3],settingArray[2*3]);
  RTC.fillByHMS(settingArray[3*3],settingArray[4*3],settingArray[5*3]);
  RTC.setTime();
  RTC.startClock();
  lcd.clear();
  lcd.home();
  lcd.print("Settings Saved");
  delay(1000);
  lcd.clear();
  lcd.home();

}

void alarmSettings(){
  boolean nextMenuFlag=false;
  //read in the alarms
  int settingArray[15*12];
  for(int x = 0; x < 15*12; x++){
    settingArray[x]=EEPROM.read(x);
  }
  lcd.clear();
  lcd.home();
  for(int x = 0; x < 15; x++){
    for(int y = 0; y < 4; y++){
      strcpy_P(buffer, (char*)pgm_read_word(&(config_table2[y]))); 
      nextMenuFlag=false;
      lcd.clear();
      lcd.home();
      lcd.print("Alarm ");
      lcd.print(x);
      lcd.setCursor(0,1);
      lcd.print(buffer);
      lcd.print(settingArray[(12*x)+(3*y)]);
      int minValue=settingArray[(12*x)+(3*y)+1];
      int maxValue=settingArray[(12*x)+(3*y)+2];
      while(nextMenuFlag==false){
        analogButtons.checkButtons();
        switch (buttonPressed) {
        case 1:
          nextMenuFlag=true;
          buttonPressed=0;
          break;
        case 2:
          if(settingArray[(12*x)+(3*y)]>=maxValue){
            settingArray[(12*x)+(3*y)]=settingArray[(12*x)+(3*y)];
          }
          else{
            settingArray[(12*x)+(3*y)]=settingArray[(12*x)+(3*y)]+1;
          }
          lcd.clear();
          lcd.home();
          lcd.print("Alarm ");
          lcd.print(x);
          lcd.setCursor(0,1);
          lcd.print(buffer);
          lcd.print(settingArray[(12*x)+(3*y)]);
          buttonPressed=0;
          break;
        case 3:
          if(settingArray[(12*x)+(3*y)]<=minValue){
            settingArray[(12*x)+(3*y)]=settingArray[(12*x)+(3*y)];
          }
          else{
            settingArray[(12*x)+(3*y)]=settingArray[(12*x)+(3*y)]-1;
          }
          lcd.clear();
          lcd.home();
          lcd.print("Alarm ");
          lcd.print(x);
          lcd.setCursor(0,1);
          lcd.print(buffer);
          lcd.print(settingArray[(12*x)+(3*y)]);
          buttonPressed=0;
          break;
        default: 
          nextMenuFlag=false;
          buttonPressed=0;
        }
      }
    }
    //Serial.print("[memCheckalarmSettings]");
    //Serial.println(freeRam());
  }
  // Saving settings
  for(int x = 0; x < 15*12; x++){
    EEPROM.write(x, settingArray[x]);
  }
  lcd.clear();
  lcd.home();
  lcd.print("Settings Saved");
  delay(1000);
  lcd.clear();
  lcd.home();

}
int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}















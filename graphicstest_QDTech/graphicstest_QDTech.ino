
//#define sclk 13  // Нельзя изменять
//#define mosi 11  //  Нельзя изменять
#define cs_  9
#define dc_   10
#define rst_  12  // you can also connect this to the Arduino reset
#define sm_step 7
#define sm_dir 4
#define sm_enable 8
#define holl 14

#define btn1 15
#define btn2 16
#define btn3 17
//#define btn4 18


#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
//#include <AccelStepper.h>
#include <SPI.h>
#include"timer-api.h"

Adafruit_ST7735 tft = Adafruit_ST7735(cs_, dc_, rst_);  // Invoke custom library

unsigned long _lastStepTime = 0;
unsigned long _stepInterval = 1000000;
unsigned long _lastSpeedTime = 0;
unsigned long _speedInterval = 5;

int pin;
unsigned long count = 0;

int max_speed = 800;
int min_speed = 400;
int current_speed = 1;
int stepper_status = 0;
int old_speed;

typedef enum{dir_back = 0, dir_forw = 1} direct_t; 
int direct = dir_forw;

double len;
double max_len = 47.5;


typedef enum {mode_stop, mode_start, mode_stoping, mode_forward, mode_backward} mode_t;
mode_t mode = mode_stop;

//AccelStepper stepper(1, sm_step, sm_dir); // use functions to step

String utf8rus(String source)
{
  int i, k;
  String target;
  unsigned char n;
  char m[2] = { '0', '\0' };

  k = source.length(); i = 0;

  //while (i < 256) {
  while (i < k) {
    n = source[i]; i++;

    if (n >= 0xC0) {
      switch (n) {
        case 0xD0: {
            n = source[i]; i++;
            if (n == 0x81) {
              n = 0xA8;
              break;
            }
            if (n >= 0x90 && n <= 0xBF) n = n + 0x30 - 1;
            break;
          }
        case 0xD1: {
            n = source[i]; i++;
            if (n == 0x91) {
              n = 0xB8;
              break;
            }
            if (n >= 0x80 && n <= 0x8F) n = n + 0x70 - 1;
            break;
          }
      }
    }
    m[0] = n; target = target + String(m);

    //m[0] = i; target = target + String(m);
  }
  return target;
}

void tftSetCursorToLine(int line)
{
  enum {lineHeight = 16};
  tft.setCursor (1, 1 + line*lineHeight);

  tft.fillRect (0, line*lineHeight, 128, lineHeight, ST7735_BLACK);
}

struct
{
  double len; // 
  int count;
  int spd;
  mode_t mode;
} indicStr; // отображаемые данные на дисплее

void indicationRefr(void)
{ 
  
  //tftSetCursorToLine(0);
  //tft.fillRect (0, 0, 128, 36, ST7735_BLACK);
  
  tftSetCursorToLine(0);
  tft.print("Len:");
  tft.print(indicStr.len);
  tft.println("m");

  //tftSetCursorToLine(1);
  //tft.println(indicStr.count);

  tftSetCursorToLine(2);

  switch(indicStr.mode)
  {
    case mode_stop:
      tft.println("STOP");
      break;
    case mode_start:
      tft.println("START");
      break;
    case mode_stoping:
      tft.println("STOPING");
      break;
    case mode_forward:
      tft.println("FORWARD");
      break;
    case mode_backward:
      tft.println("BACKWARD");
      break;
  }

  //tftSetCursorToLine(3);
  //tft.print("Spd ");
  //tft.println(indicStr.spd);
}

void newMode(mode_t newMode)
{
  mode = newMode;
  indicStr.mode = newMode;
  indicationRefr();
}

void set_speed(int new_speed)
{
  if (new_speed != old_speed) {
    _stepInterval = 1000000 / new_speed;
    old_speed = new_speed;
    
    indicStr.spd = new_speed;
    Serial.print("speed ");
    Serial.println(new_speed);
  }
}

void setup(void) {
  Serial.begin(9600);
  Serial.println("setup");

  pinMode(holl, INPUT_PULLUP); //Датчик холла
  pinMode(btn1, INPUT_PULLUP); 
  pinMode(btn2, INPUT_PULLUP); 
  pinMode(btn3, INPUT_PULLUP); 
  //pinMode(btn4, INPUT_PULLUP); 
  pinMode(sm_enable, OUTPUT); //Enable
  pinMode(sm_dir, OUTPUT);
  pinMode(sm_step, OUTPUT);
  digitalWrite(sm_enable, 0);


  tft.initR(INITR_BLACKTAB);

  tft.setRotation(0);	// 0 - Portrait, 1 - Lanscape
  tft.fillScreen(ST7735_BLACK);
  tft.setTextWrap(true);

  tft.setTextColor (ST7735_WHITE);
  tft.setTextSize (2);
  pin = digitalRead(holl);
  len = 0;

  indicStr.len = len;
  indicationRefr();

  _lastSpeedTime = millis();  
}

void stopMotor(void)
{
  timer_stop_ISR(TIMER_DEFAULT);  
  newMode(mode_stop);
  Serial.println("stop motor!");
}

void startMotor(int dir)
{
  direct = dir;
  timer_init_ISR_50KHz(TIMER_DEFAULT);
  Serial.println("start motor!");
}

void newCount(int cnt)
{
  len = count / 6.0;
  
  indicStr.len = len;
  indicStr.count = count;
  indicationRefr();
        
  Serial.print("sensor ");
  Serial.print(count);
  Serial.print(" len ");
  Serial.println(len);
}


void clickStartStop(void)
{
   if(mode_stop == mode)
   {
     Serial.println("click start!");
        
     newMode(mode_start);

     set_speed (1);
     startMotor(dir_forw);   

     count = 0;
     newCount(count);
  }else
  if(mode_start == mode)
  {
     Serial.println("click stop!");
        
     newMode(mode_stoping); 
  }
}

void clickForward (void)
{
  Serial.println("click Forward!");
  if(mode_stop == mode)
  {
    newMode(mode_forward);
    set_speed (min_speed);
    startMotor(dir_forw);   
  }
}

void clickBackward (void)
{
  Serial.println("click Backward!");
  if(mode_stop == mode)
  {
    newMode(mode_backward);
    set_speed (min_speed);
    startMotor(dir_back); 
  }
}

void stopManualMode(void)
{
  stopMotor();
}

void loop() {
  int new_pin = digitalRead(holl);

  if((mode_start == mode)||(mode_stoping == mode))
  {
    if (pin != new_pin)  
    {
      if (new_pin == 0) 
      {
        count++;
        newCount(count);
      }  
      pin = new_pin;
    }
  }

// кнопка старт стоп
  int new_pinBtn1 = digitalRead(btn1);
  static int cntBtn1 = 0;
  if (0 == new_pinBtn1)
  {
    ++cntBtn1;
    if (25 == cntBtn1) 
    {      
      clickStartStop();
    }
  }
  else
  {
    cntBtn1 = 0;
  }

// кнопка вперед
  int new_pinBtn2 = digitalRead(btn2);
  static int cntBtn2 = 0;
  if (0 == new_pinBtn2)
  {
    ++cntBtn2;
    if (25 == cntBtn2) 
    {      
      clickForward();
    }
  }
  else
  {
    cntBtn2 = 0;
    if (mode_forward == mode)
    {
      stopManualMode();
    }
  }

// кнопка назад
  int new_pinBtn3 = digitalRead(btn3);
  static int cntBtn3 = 0;
  if (0 == new_pinBtn3)
  {
    ++cntBtn3;
    if (25 == cntBtn3) 
    {      
      clickBackward();
    }
  }
  else
  {
    cntBtn3 = 0;
    if (mode_backward == mode)
    {
      stopManualMode();
    }
  }

  if (mode_start == mode)
  {
      unsigned long time = millis();
      if (time - _lastSpeedTime > _speedInterval) 
      {
        if (len + 2.0 < max_len) 
        {
          if (current_speed < max_speed) 
          {
            current_speed += 5;
            set_speed(current_speed);
          }
       } 
       else if ((len + 2.0 >= max_len) && (len < max_len)) 
       {
          if (current_speed > min_speed) 
          {
            current_speed -= 2;
            set_speed(current_speed);
          }
        } 
        else 
        {
          stopMotor();          
        }
        _lastSpeedTime = time;
      }
  }
  else if (mode_stoping == mode)
  {
    if (current_speed > min_speed) 
    {
      current_speed -= 2;
      set_speed(current_speed);
    }
    else
    {
      stopMotor();  
      current_speed = 1;
      set_speed(current_speed);
      indicationRefr();
    }
  }
}

void timer_handle_interrupts(int timer) {
  unsigned long time = micros();
  if (mode_stop != mode)
  {
    if (time - _lastStepTime >= _stepInterval) {
      _lastStepTime = time;
      
      digitalWrite(sm_dir, direct);
      digitalWrite(sm_step, 1);
      delayMicroseconds(1);
      digitalWrite(sm_step, 0);
    }
  }
}

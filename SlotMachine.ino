#define DEBUG
  
#include <LedControl.h>
#include <TimerFreeTone.h>                                                    // https://bitbucket.org/teckel12/arduino-timer-free-tone/wiki/Home
#include "Wheel.h"
#include "Piano.h"
#include <LED.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <TM1637Display.h>

#define DIGITS 3  //Number of MAX7219
#define WHEELS 3  //Number of wheels on machine

// - MAX7219 Dot Matrix Module
#define DIGIT(a) (a)

// Button
#define BUTTON1_PIN 7
#define BUTTON2_PIN 12

// Matrix
#define DIN 15
#define CLK 17
#define LOAD 16

// 7 segment
#define S_CLK 10
#define S_DIO 9

// Buzzer
#define TONE_PIN 3

#define BUZZER_DDR  DDRD                                      
#define BUZZER_PORT PORTD
#define BUZZER_PIN  DDD3

LedControl lc=LedControl(DIN,CLK,LOAD,DIGITS);

LiquidCrystal_I2C lcd(0x3F,  16, 2);

TM1637Display display = TM1637Display(S_CLK, S_DIO);

#define BRIGHTNESS 2  //0 to 15

//Close Encounters
#define NUM_NOTES 5
const int closeEncounters[] = {                             // notes in the melody:
    NOTE_A2, NOTE_B2, NOTE_G2, NOTE_G1, NOTE_D2                     // "Close Encounters" tones
};
  
//- Payout Table
/*  Probabilities based on a 1 credit wager
    Three spaceships:     1 / (25 * 25 * 25)    = 0.000064
    Any three symbols:            24 / 15625    = 0.001536
    Two spaceships:         (24 * 3) / 15625    = 0.004608
    One spaceship:      (24 * 24 * 3)/ 15625    = 0.110592
    Two symbols match: (23 * 3 * 24) / 15625    = 0.105984
    House win, 1 minus sum of all probabilities = 0.777216
    _
                                                   P   R   O   O   F
                                                   Actual    Actual    
        Winning Combination Payout   Probablility  Count     Probability
        =================== ======   ============  ========  ===========*/
#define THREE_SEVENS_PAYOUT 600 //    0.000064            0.00006860
#define THREE_SYMBOL_PAYOUT    122 //    0.001536            0.00151760
#define TWO_SEVENS_PAYOUT    50 //    0.004608            0.00468740
#define ONE_SEVEN_PAYOUT     3 //    0.110592            0.11064389
#define TWO_SYMBOL_PAYOUT        2 //    0.105984            0.10575249

/* Timing constants that control how the reels spin */
#define START_DELAY_TIME 10
#define INCREMENT_DELAY_TIME 5
#define PAUSE_TIME 1000
#define MAX_DELAY_BEFORE_STOP 100
#define MIN_SPIN_TIME 500
#define MAX_SPIN_TIME 1500
#define FLASH_REPEAT 10
#define FLASH_TIME 100
#define DIGIT_DELAY_TIME 5

/* spinDigit holds the information for each wheel */
struct spinDigit 
{
  unsigned long delayTime;
  unsigned long spinTime;
  unsigned long frameTime;
  uint8_t row;
  uint8_t symbol;
  bool stopped;
};

spinDigit spin[WHEELS]; 

#define STARTING_CREDIT_BALANCE 1000    // Number of credits you have at "factory reset".
#define DEFAULT_HOLD            0       // default hold is zero, over time the machine pays out whatever is wagered
#define MINIMUM_WAGER           5       // 
#define WAGER_INCREMENT         5       //

/* global variables to store game statistics and settings */
unsigned long wagered = 0;
unsigned long plays = 0;
unsigned long payedOut = 0;
unsigned long  twoMatchCount = 0;
unsigned int  threeMatchCount = 0;
unsigned long  shipOneMatchCount = 0;
unsigned int  shipTwoMatchCount = 0;
unsigned int  shipThreeMatchCount = 0;
long creditBalance = STARTING_CREDIT_BALANCE;
int hold = DEFAULT_HOLD;
unsigned int seed = random(0, 65535);
unsigned long payout;
double owedExcess = 0;
int wager = MINIMUM_WAGER;

//--------------------------------------------------------------------------------------------

void setup() 
{
  #ifdef DEBUG
    Serial.begin(9600);
  #endif

  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(TONE_PIN, OUTPUT);

  //Set up a random seed 
  randomSeed(analogRead(A0));

  display.setBrightness(5);

  //initialize lcd screen
  lcd.init();
  // turn on the backlight
  lcd.backlight();
  lcd.print("Initializing...");
  
  //Set up matrixes
  for (uint8_t j = 0; j < DIGITS; j++) {
    lc.shutdown(j,false); //Wake up
    lc.setIntensity(j, BRIGHTNESS);
    lc.clearDisplay(j);

    if (j < WHEELS)
    {
      spin[j].row = random(0, SYMBOLS) << 3;  //Start each wheel on a random symbol
    }
  }

  //Show credit balance
  display.showNumberDec(STARTING_CREDIT_BALANCE);

  //Play splash screen
  playSplashScreen();

  //Show the opening set of wheels
  for (uint8_t i = 0; i < WHEELS; i++)
  {
    displayWheelSymbol(i);
  }

  //Scroll display
  for (uint8_t i = 0; i < 16; i++) {
    lcd.scrollDisplayLeft();
    delay(50);
  }
  lcd.clear();

  // Display the current credit balance
  displayWager();
}

//--------------------------------------------------------------------------------------------

//Main loop spins the wheels
void loop()
{
  waitOnButtonPress();

  spinTheWheels();

  #ifdef DEBUG
    for (uint8_t i = 0; i < WHEELS; i++)
    {
      Serial.print(String(spin[i].symbol) + " ");
    }
    Serial.println(" - Payout is " + String(payout));
  #endif
  
  delay(PAUSE_TIME / 2);

  //All stopped, time to pay out
  double winnings = wager * (payout - (payout * (hold / 100.0))); //winnings are the amount wagered times the payout minus the hold.
  long roundWinnings = (long) round(winnings);
  owedExcess += winnings - roundWinnings;                                 // owedExcess is the change; credits between -1 and 1.
  if (owedExcess >= 1 || owedExcess <= -1) 
  {                                                                       // if we can pay out some excess
    int roundOwedExcess = (int) round(owedExcess);
    roundWinnings += roundOwedExcess;                                     // add the rounded portion to the winnings
    owedExcess -= roundOwedExcess;                                        // subtract out what we added to continue to track the excess
  } 
  
  if (roundWinnings > 0) {
    lcd.clear();
    String s = "You won " + String(roundWinnings);
    lcd.print(s);
    lcd.setCursor(0,1);
    lcd.print("Double? Yes  No");
    roundWinnings = waitOnButtonPressDouble(roundWinnings);
    Serial.println(roundWinnings);
  }

  roundWinnings -= wager;
  payout += roundWinnings;
  wagered += wagered;

  Serial.println("Updating Balance with: " + String(roundWinnings));
  adjustCreditBalance(creditBalance + roundWinnings);
  displayWager();
  beepDigit();
}

//--------------------------------------------------------------------------------------------

//Spins all the wheels and updates payout
void spinTheWheels()
{
  //Reset wheels for the spin
  unsigned long totalTime = millis();
  for (uint8_t j = 0; j < WHEELS; j++)
  {
    totalTime = totalTime + random(MIN_SPIN_TIME, MAX_SPIN_TIME);
    spin[j].delayTime = START_DELAY_TIME;
    spin[j].spinTime = totalTime;
    spin[j].frameTime = millis() + spin[j].delayTime;
    spin[j].stopped = false;
  }
  
  bool allStopped = false;
  while (!allStopped)
  {
    //Scroll each symbol up
    for (uint8_t j = 0; j < WHEELS; j++)
    {
      if (!spin[j].stopped && millis() > spin[j].frameTime)
      {
        spin[j].frameTime = millis() + spin[j].delayTime;

        displayWheelSymbol(j);
        spin[j].row = (spin[j].row + 1) % TOTAL_SYMBOL_ROWS;

        beepWheel();
        
        if (millis() > spin[j].spinTime)
        {
          //Stop if delayTime exceeds MAX_DELAY_BEFORE_STOP
          //Only stop on complete symbol
          if (spin[j].delayTime > MAX_DELAY_BEFORE_STOP && (spin[j].row % 8) == 1)
          {
            spin[j].stopped = true;
            spin[j].symbol = spin[j].row >> 3;
            if (j == (WHEELS - 1))
            {
              //All wheels are now stopped
              allStopped = true;
              highlightWinAndCalculatePayout();
            }
          }
          else if (spin[j].delayTime <= MAX_DELAY_BEFORE_STOP)
          {
            spin[j].delayTime = spin[j].delayTime + INCREMENT_DELAY_TIME;
          }
        }
      }
    }
    yield();
  }
}

//--------------------------------------------------------------------------------------------

//Display the current symbol of the specified wheel
void displayWheelSymbol(int wheel)
{
  for (int8_t i = 7; i >= 0; i--)
  {
    lc.setRow(DIGIT(wheel), i, getReelRow((spin[wheel].row + i) % TOTAL_SYMBOL_ROWS));
  }
}

//--------------------------------------------------------------------------------------------

//Work out if the player has one anything
//If they have, flash winning sequence and update payout multiplier
void highlightWinAndCalculatePayout()
{
  payout = 0;
  uint8_t matches = 0;
  uint8_t symbol = 0;
  uint8_t sevens = 0;
  uint8_t wild = 0;
  if (spin[0].symbol == spin[1].symbol && spin[1].symbol == spin[2].symbol && spin[0].symbol == spin[2].symbol) {
    if (spin[0].symbol == SEVEN) {
      sevens = 3;
    }
    else {
      matches = 3;
    }
  }
  else {
    for (uint8_t y = 0; y < WHEELS; y++)
    {
      if (spin[y].symbol == SEVEN)
      {
        sevens++;
      }
      else
      {
        for (uint8_t x = 0; x < WHEELS; x++)
        {
          if (spin[y].symbol == spin[x].symbol && y != x)
          {
            matches++;
            symbol = spin[y].symbol;
          }
        }
      }
    }
  }
  if (matches > 0 || sevens > 0)
  {
    if ((sevens == 1 && matches == 2) || (sevens == 2))
    {
      Serial.print("Wild payout");
      for (uint8_t x = 0; x < WHEELS; x++)
        {
          if (spin[x].symbol != SEVEN)
          {
            symbol = spin[x].symbol;
          }
        }
      payout = THREE_SYMBOL_PAYOUT; threeMatchCount++; winSound(4);
      wild = 1;
    }
    else {
    switch (sevens)
    {
      case 3: payout = THREE_SEVENS_PAYOUT; shipThreeMatchCount++; winSound(5); symbol = SEVEN; break;
      case 2: payout = TWO_SEVENS_PAYOUT; shipTwoMatchCount++; winSound(3); symbol = SEVEN; break;
      case 1: payout = ONE_SEVEN_PAYOUT; shipOneMatchCount++; winSound(2); symbol = SEVEN; break;
      default:
        switch (matches)
        {
          case 3: payout = THREE_SYMBOL_PAYOUT; threeMatchCount++; winSound(4); break;
          case 2: payout = TWO_SYMBOL_PAYOUT; twoMatchCount++; winSound(1); break;
        }
        break;
    }
    }
    if (wild == 1) {
      flashSymbol(symbol, true);
    } else {
      flashSymbol(symbol, false);
    }
  }
  //Count every spin
  plays++;
}

//---------------------------------------------------------------------------

//Flashes any wheel that is showing the specified symbol
void flashSymbol(uint8_t symbol, bool seven)
{
  bool on = true;
  uint8_t row = symbol << 3;
  for (uint8_t r = 0; r < FLASH_REPEAT; r++)
  {
    for (uint8_t j = 0; j < WHEELS; j++)
    {
      if (spin[j].symbol == symbol)
      {
        for (int8_t i = 7; i >= 0; i--)
        {
          if (on)
          {
            lc.setRow(DIGIT(j), i, 0);
          }
          else
          {
            lc.setRow(DIGIT(j), i, getReelRow((row + i) % TOTAL_SYMBOL_ROWS));
          }
        }
      }
      if (spin[j].symbol != symbol && seven) {
        for (int8_t i = 7; i >= 0; i--)
        {
          if (on)
          {
            lc.setRow(DIGIT(j), i, 0);
          }
          else
          {
            lc.setRow(DIGIT(j), i, getReelRow((row + i) % TOTAL_SYMBOL_ROWS));
          }
        }
      }
    }
    on = !on;
    delay(FLASH_TIME);
  }
}

//---------------------------------------------------------------------------

//Play the opening anaimation
void playSplashScreen() 
{
  //Show aliens walking
  for (uint8_t k = 0; k < 1; k++)
  {
    for (int8_t i = 7; i >= 0; i--)
    {
      lc.setRow(DIGIT(0), i, getReelRow((P << 3) + i));
    }
    delay(250);
    for (int8_t i = 7; i >= 0; i--)
    {
      lc.setRow(DIGIT(1), i, getReelRow((M << 3) + i));
    }
    delay(250);
    for (int8_t i = 7; i >= 0; i--)
    {
      lc.setRow(DIGIT(2), i, getReelRow((HEART << 3) + i));
    }
    playMelody();
  }

  //Move ship from right to left clearing out the aliens
  for (int p = WHEELS * 8 - 1; p > -8; p--)
  {
    for (uint8_t c = 0; c < 8; c++)
    {
      //Display each column at position p
      int pc = p + c;
      if (pc >= 0 && pc < (WHEELS << 3))
      {
        for (uint8_t r = 0; r < 8; r++)
        {
          lc.setLed(DIGIT((pc >> 3)), r, pc & 0x07, getReelRow((HEART << 3) + r) & (1 << c));
        }
      }
      //Clear last column
      pc = pc + 1;
      if (pc >= 0 && pc < (WHEELS << 3))
      {
        lc.setColumn(DIGIT((pc >> 3)), pc & 07, 0);
      }
    }
    delay(10);
  }
  lc.clearDisplay(DIGIT(0));
  delay(2000);
}

//-----------------------------------------------------------------------------------

//Read a row from the reels either from FLASH memory or RAM
uint8_t getReelRow(uint8_t row)
{
  return reel[row];
}

//-----------------------------------------------------------------------------------

//Turn on and off buzzer quickly
void beepWheel() 
{                                                                   // Beep and flash LED green unless STATE_AUTO
  BUZZER_PORT |= (1 << BUZZER_PIN);                                           // turn on buzzer
  delay(10);
  BUZZER_PORT &= ~(1 << BUZZER_PIN);
}

//-----------------------------------------------------------------------------------

//Turn on and off buzzer quickly
void beepDigit() 
{                                                                   // Beep and flash LED green unless STATE_AUTO
  BUZZER_PORT |= (1 << BUZZER_PIN);                                           // turn on buzzer
  delay(5);
  BUZZER_PORT &= ~(1 << BUZZER_PIN);                                          // turn off the buzzer
}

//-----------------------------------------------------------------------------------

void doubleSuccessSound() {
    TimerFreeTone(TONE_PIN, NOTE_C4, 150);
    delay(200);
    TimerFreeTone(TONE_PIN, NOTE_E4, 150);
    delay(200);
    TimerFreeTone(TONE_PIN, NOTE_G4, 150);
    delay(200);
}

//-----------------------------------------------------------------------------------

void doubleFailSound() {
    TimerFreeTone(TONE_PIN, NOTE_C4, 300);
    delay(200);
    TimerFreeTone(TONE_PIN, NOTE_F3, 300);
    delay(200);
    TimerFreeTone(TONE_PIN, NOTE_B2, 300);
    delay(200);
}

//Play the winning siren multiple times
void winSound(uint8_t repeat)
{
  for (uint8_t i = 0; i < repeat; i++)
  {
    playSiren();
  }
}

//-----------------------------------------------------------------------------------

//Play the siren sound
void playSiren() 
{
  #define MAX_NOTE                4978                                         // Maximum high tone in hertz. Used for siren.
  #define MIN_NOTE                31                                           // Minimum low tone in hertz. Used for siren.
  
  for (int note = MIN_NOTE; note <= MAX_NOTE; note += 5)
  {                       
    TimerFreeTone(TONE_PIN, note, 1);
  }
}

//-----------------------------------------------------------------------------------

//Play the "Close Encounters" melody
void playMelody() 
{
  for (int thisNote = 0; thisNote < NUM_NOTES; thisNote++) 
  {
    // to calculate the note duration, take one second divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 500;
    TimerFreeTone(TONE_PIN, closeEncounters[thisNote] * 3, noteDuration); 
    delay(100);
  }
}

//-----------------------------------------------------------------------------------

//animate the change in credit balance
void adjustCreditBalance(long newBalance)
{
  unsigned int difference;
  int8_t direction;
  if (creditBalance != newBalance)
  {
    if (creditBalance > newBalance)
    {
      difference = creditBalance - newBalance;
      direction = -1;
    }
    else
    {
      difference = newBalance - creditBalance;
      direction = 1;
    }
    
    for (unsigned int i = 0; i < difference; i++)
    {
      creditBalance += direction;
      display.showNumberDec(creditBalance);
      beepDigit();
      delay(DIGIT_DELAY_TIME);
    }
  }
}

//-----------------------------------------------------------------------------------

//Display a number on the LCD display
void displayWager()
{
  bool negative = (wager < 0);
  wager = abs(wager);
  String numberString = String(wager); // Convert the number to a string
  lcd.clear();
  lcd.print("Current wager:");
  lcd.setCursor(0, 1);
  lcd.print(numberString); // Print the number string to the LCD
}


//-----------------------------------------------------------------------------------

// Wait until player presses the button
void waitOnButtonPress()
{
  bool released = false;
  while (!released)
  {
    if (digitalRead(BUTTON1_PIN) == LOW)
    {
      // If BUTTON1_PIN is pressed, start spinning the wheels
      while (digitalRead(BUTTON1_PIN) == LOW)
      {
        yield();
        delay(10);
      }
      released = true;
    }
    else if (digitalRead(BUTTON2_PIN) == LOW)
    {
      // If BUTTON2_PIN is pressed, increase the wager by 5
      while (digitalRead(BUTTON2_PIN) == LOW)
      {
        yield();
        delay(10);
      }
      wager += WAGER_INCREMENT;
      if (wager > 25) {
        wager = MINIMUM_WAGER;
      }
      displayWager();
      delay(200); // Debounce delay
    }
    yield();
    delay(10);
  }
}


long waitOnButtonPressDouble(long roundWinnings) {
  bool released = false;
  int buttonPressed = 0;
  while (!released) {
    if (digitalRead(BUTTON1_PIN) == LOW) {
      while (digitalRead(BUTTON1_PIN) == LOW) {
        yield();
        delay(10);
      }
      released = true;
      buttonPressed = 1;
    } else if (digitalRead(BUTTON2_PIN) == LOW) {
      while (digitalRead(BUTTON2_PIN) == LOW) {
        yield();
        delay(10);
      }
      released = true;
      buttonPressed = 2;
    }
  }
  if (buttonPressed == 1) {
    long randomNumber = random(1, 65535);
    Serial.print(randomNumber);
    if (randomNumber % 2 == 0) {
      roundWinnings *= 2;
      doubleSuccessSound();
      lcd.clear();
      lcd.print("Congrats!You won:");
      lcd.setCursor(0, 1);
      lcd.print(String(roundWinnings));
      delay(3000);
    }
    else {
      roundWinnings = 0;
      doubleFailSound();
      lcd.clear();
      lcd.print("Better luck");
      lcd.setCursor(0, 1);
      lcd.print("next time!");
      delay(3000);
    }
    return roundWinnings;
  }
  return roundWinnings;
}


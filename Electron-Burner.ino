//#include <Boards.h>
//#include <Firmata.h>

#include <avr/wdt.h>
#include <TFT_HX8357.h>
#include <User_Setup.h>


#include <TFT_HX8357.h> // Hardware-specific library
#include <Encoder.h>

TFT_HX8357 tft = TFT_HX8357();       // Invoke custom library

#define BIT0    1
#define BIT1    2
#define BIT2    4
#define BIT3    8
#define BIT4    0x10
#define BIT5    0x20
#define BIT6    0x40
#define BIT7    0x80
#define BIT8    0x100
#define BIT9    0x200
#define BIT10   0x400
#define BIT11   0x800
#define BIT12   0x1000
#define BIT13   0x2000
#define BIT14   0x4000
#define BIT15   0x8000

uint32_t updateTime = 0;       // time for next update
uint32_t Time500ms  = 0;
uint32_t Time50ms   = 0;
uint32_t Time1000ms = 0;

//#define NO_STOP_VOLTS_DETECT  1

#define CEL_STOP_MAX    410                       // 4.1 V
#define CEL_STOP_MIN    360                       // 3.6 V

#define MAX_CONV_SPANNIG  340                     // 34.0 V
#define MAX_CONV_STROOM   100                     // 10.0 A
#define MAX_ADC_STROOM_COUNTS 409
#define MIN_CEL_VOLTAGE_OK  30                    // 3V Minimum cel voltage

int meting_volts_ch1 = 0;
int meting_volts_ch2 = 0;
int meting_amps_ch1 = 0;
int meting_amps_ch2 = 0;
int mah_ch1 = 0;
int mah_ch2 = 0;
float mah_ch1_calc = 0;
float mah_ch2_calc = 0;
int meet_adc_index = 0;
int cel_stop_voltage_ch1 = 382;
int cel_stop_voltage_ch2 = 382;
int cel_stop_voltage_ch1_edit = 2 * 382;
int cel_stop_voltage_ch2_edit = 2 * 382;
int run_timer_ch1 = 0;
int run_timer_ch2 = 0;
int lock_enter_1_out = 0;
int lock_enter_2_out = 0;

int raw_adc[14] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int balancer_volts[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int old_plot_pointer[12] = { -1, -1, -1, -1, -1, -1, -1, -1, 1, -1, -1, -1};
int d = 0;

#define METER_VOLT_CH1  0
#define METER_VOLT_CH2  1

#define ENCODER_1_INT1  2
#define ENCODER_1_INT2  3
#define ENCODER_1_ENTER 6
#define ENCODER_2_INT1  20
#define ENCODER_2_INT2  21
#define ENCODER_2_ENTER 7
#define ENCODER_1_LED_RED 8
#define ENCODER_1_LED_GREEN 9
#define ENCODER_1_LED_BLUE  10
#define ENCODER_2_LED_RED 11
#define ENCODER_2_LED_GREEN 12
#define ENCODER_2_LED_BLUE  13
#define fet_ch1     5
#define fet_ch2     4
//
//Mega2560
// external interrupt int.0    int.1    int.2   int.3   int.4   int.5            
// pin                  2         3      21      20      19      18

Encoder knobch1(ENCODER_1_INT2, ENCODER_1_INT1);
Encoder knobch2(ENCODER_2_INT2, ENCODER_2_INT1);

long positionch1  = -999;
long positionch2  = -999; 

typedef struct
{
  float ltx;    // Saved x coord of bottom of needle
  uint16_t osx;
  uint16_t osy; // Saved x & y coords
  int   old_analog;
  int   old_run_timer;
  int   old_spanning;
  int   old_amps;
  int   old_stop_voltage;
  int   old_mah;
  int   detected_cells;
  int   old_detected_cells;
  byte  meter_nr;
  
} METER_DATA;

uint16_t run_flags = 0;
#define RUN_FLAGS_START_CH1         BIT0
#define RUN_FLAGS_START_CH2         BIT1
#define RUN_FLAGS_ADC_DIAG          BIT2
#define RUN_FLAGS_AUTO_STOPPPED_CH1 BIT3
#define RUN_FLAGS_AUTO_STOPPPED_CH2 BIT4

METER_DATA meters[2];

#define KEY_NONE                0
#define KEY_APPL_START_CH1      BIT0
#define KEY_APPL_START_CH2      BIT1
#define KEY_APPL_STOP_CH1       BIT2
#define KEY_APPL_STOP_CH2       BIT3
#define KEY_APPL_PLUS_CH1       BIT4
#define KEY_APPL_MIN_CH1        BIT5
#define KEY_APPL_PLUS_CH2       BIT6
#define KEY_APPL_MIN_CH2        BIT7
#define KEY_APPL_ADC_DIAG       BIT8
  


void setup(void)
{
//  wdt_enable(WDTO_1S);
  
  tft.init();
  tft.setRotation(1);
  Serial.begin(115200); // For debug
  
  pinMode(fet_ch1, OUTPUT);
  pinMode(fet_ch2, OUTPUT);
  digitalWrite(fet_ch1, LOW);
  digitalWrite(fet_ch2, LOW);

  pinMode(ENCODER_1_LED_RED, OUTPUT);
  pinMode(ENCODER_1_LED_GREEN, OUTPUT);
  pinMode(ENCODER_1_LED_BLUE, OUTPUT);
  pinMode(ENCODER_2_LED_RED, OUTPUT);
  pinMode(ENCODER_2_LED_GREEN, OUTPUT);
  pinMode(ENCODER_2_LED_BLUE, OUTPUT);

  digitalWrite(ENCODER_1_LED_RED, HIGH);
  digitalWrite(ENCODER_1_LED_GREEN, HIGH);
  digitalWrite(ENCODER_1_LED_BLUE, HIGH);
  digitalWrite(ENCODER_2_LED_RED, HIGH);
  digitalWrite(ENCODER_2_LED_GREEN, HIGH);
  digitalWrite(ENCODER_2_LED_BLUE, HIGH);

  pinMode(ENCODER_1_ENTER, INPUT);
  pinMode(ENCODER_2_ENTER, INPUT);
 
 analogReference(DEFAULT);
  analogRead(A0);                                                                 // Start adc vRef
  Build_Main_Screen();
  
  updateTime = millis(); // Next update time
  Time500ms = millis();
  Time50ms = millis();
  Time1000ms = millis();
}

void Build_Main_Screen(void)
{
  byte d = 40;
  
  tft.fillScreen(TFT_NAVY);

  InitMeter(METER_VOLT_CH1, &meters[METER_VOLT_CH1]);     // Draw analog meter
  InitMeter(METER_VOLT_CH2, &meters[METER_VOLT_CH2]);     // Draw analog meter

 
  analogMeter(&meters[METER_VOLT_CH1]);                                       // Draw analog meter
  analogMeter(&meters[METER_VOLT_CH2]);                                       // Draw analog meter

  // Draw 6 linear meters

  plotLinear("C1", 0, 168);
  plotLinear("C2", 1 * d, 168);
  plotLinear("C3", 2 * d, 168);
  plotLinear("C4", 3 * d, 168);
  plotLinear("C5", 4 * d, 168);
  plotLinear("C6", 5 * d, 168);
  
  plotLinear("C1", 6 * d, 168);
  plotLinear("C2", 7 * d, 168);
  plotLinear("C3", 8 * d, 168);
  plotLinear("C4", 9 * d, 168);
  plotLinear("C5", 10 * d, 168);
  plotLinear("C6", 11 * d, 168);
  updateTime = millis(); // Next update time

}

void Build_Adc_Diag_Screen(void)
{
  tft.fillScreen(TFT_BLACK);
}


void loop()
{
    int adc_val;
    int key;
    int i;

  long enc_ch1, enc_ch2;

//  wdt_reset();      
  
  if (Time50ms <= millis())
  {
    enc_ch1 = knobch1.read();
    enc_ch2 = knobch2.read();
  
    if (enc_ch1 != positionch1 || enc_ch2 != positionch2)
    {
      key = KEY_NONE;
      if( enc_ch1 < positionch1) key = KEY_APPL_MIN_CH1; 
      if( enc_ch1 > positionch1) key = KEY_APPL_PLUS_CH1; 
      Decode_Key_action(key);
      
      key = KEY_NONE;
      if( enc_ch2 < positionch2) key = KEY_APPL_MIN_CH2; 
      if( enc_ch2 > positionch2) key = KEY_APPL_PLUS_CH2; 
      Decode_Key_action(key);

      positionch1 = enc_ch1;
      positionch2 = enc_ch2;
    }

     if(!lock_enter_1_out && (digitalRead(ENCODER_1_ENTER) == HIGH) && (digitalRead(ENCODER_2_ENTER) == HIGH))
     {
        Decode_Key_action(KEY_APPL_ADC_DIAG); 
        lock_enter_1_out = 8;
     }
     else
     {
       if( !(run_flags & RUN_FLAGS_ADC_DIAG))
       {
         if(!lock_enter_1_out && (digitalRead(ENCODER_1_ENTER) == HIGH))
         {
            if( run_flags & RUN_FLAGS_START_CH1)
             Decode_Key_action(KEY_APPL_STOP_CH1);
            else
              Decode_Key_action(KEY_APPL_START_CH1);
            lock_enter_1_out = 4;
         }
         
         if(!lock_enter_2_out && (digitalRead(ENCODER_2_ENTER) == HIGH))
         {
            if( run_flags & RUN_FLAGS_START_CH2)
             Decode_Key_action(KEY_APPL_STOP_CH2);
            else
              Decode_Key_action(KEY_APPL_START_CH2);
            lock_enter_2_out = 4;
         }
    
         if( lock_enter_1_out)
         {
            if( digitalRead(ENCODER_1_ENTER) == LOW)                            // WAIT on enter release
              lock_enter_1_out--;
         }
         
         if( lock_enter_2_out)
         {
            if( digitalRead(ENCODER_2_ENTER) == LOW)                            // WAIT on enter release
              lock_enter_2_out--;
         }
       }
       else
       {
         if( lock_enter_1_out)
         {
            if(( digitalRead(ENCODER_1_ENTER) == LOW) && (digitalRead(ENCODER_2_ENTER) == LOW))                            // WAIT on enter release
              lock_enter_1_out--;
         }       
       }
     }
       
    switch( meet_adc_index)
    {
      case 0 : adc_val = analogRead(A0);
               balancer_volts[0] = map(adc_val, 0, 1023, 0, MAX_CONV_SPANNIG);
               raw_adc[0] = adc_val;
               
               adc_val = analogRead(A1);
               balancer_volts[1] = map(adc_val, 0, 1023, 0, MAX_CONV_SPANNIG);
               raw_adc[1] = adc_val;
               break;
               
      case 1 : adc_val = analogRead(A2);
               balancer_volts[2] = map(adc_val, 0, 1023, 0, MAX_CONV_SPANNIG);
               raw_adc[2] = adc_val;
               
               adc_val = analogRead(A3);
               balancer_volts[3] = map(adc_val, 0, 1023, 0, MAX_CONV_SPANNIG);
               raw_adc[3] = adc_val;
               break;
               
      case 2 : adc_val = analogRead(A4);
               balancer_volts[4] = map(adc_val, 0, 1023, 0, MAX_CONV_SPANNIG);
               raw_adc[4] = adc_val;
               
               adc_val = analogRead(A5);
               balancer_volts[5] = map(adc_val, 0, 1023, 0, MAX_CONV_SPANNIG);
               meting_volts_ch1 = Pack_Voltage(0, meters[METER_VOLT_CH1].detected_cells);
               raw_adc[5] = adc_val;
               break;
               
      case 3 : adc_val = analogRead(A7);
               balancer_volts[6] = map(adc_val, 0, 1023, 0, MAX_CONV_SPANNIG);
               raw_adc[6] = adc_val;
               
               adc_val = analogRead(A8);
               balancer_volts[7] = map(adc_val, 0, 1023, 0, MAX_CONV_SPANNIG);
               raw_adc[7] = adc_val;
               break;
               
      case 4 : adc_val = analogRead(A9);
               balancer_volts[8] = map(adc_val, 0, 1023, 0, MAX_CONV_SPANNIG);
               raw_adc[8] = adc_val;
               
               adc_val = analogRead(A10);
               balancer_volts[9] = map(adc_val, 0, 1023, 0, MAX_CONV_SPANNIG);
               raw_adc[9] = adc_val;
               break;
               
      case 5 : adc_val = analogRead(A11);
               balancer_volts[10] = map(adc_val, 0, 1023, 0, MAX_CONV_SPANNIG);
               raw_adc[10] = adc_val;
               
               adc_val = analogRead(A12);
               balancer_volts[11] = map(adc_val, 0, 1023, 0, MAX_CONV_SPANNIG);
               meting_volts_ch2 = Pack_Voltage(1, meters[METER_VOLT_CH2].detected_cells) ;
               raw_adc[11] = adc_val;
               break;
               
      case 6 : adc_val = analogRead(A6);
               if( adc_val > MAX_ADC_STROOM_COUNTS)
                adc_val = MAX_ADC_STROOM_COUNTS;
               meting_amps_ch1 = map(adc_val, 0, MAX_ADC_STROOM_COUNTS, 0, MAX_CONV_STROOM);
               raw_adc[12] = adc_val;
               
               adc_val = analogRead(A13);
               if( adc_val > MAX_ADC_STROOM_COUNTS)
                adc_val = MAX_ADC_STROOM_COUNTS;
               meting_amps_ch2 = map(adc_val, 0, MAX_ADC_STROOM_COUNTS, 0, MAX_CONV_STROOM);
               raw_adc[13] = adc_val;
               break;
    }
    meet_adc_index++;
    if( meet_adc_index > 6)
      meet_adc_index = 0;

    Auto_Detect_Cells();
      
    Time50ms = millis() + 50;

#ifndef NO_STOP_VOLTS_DETECT
    if( run_flags & RUN_FLAGS_START_CH1)
    {
      if( meting_volts_ch1 < (cel_stop_voltage_ch1 * meters[METER_VOLT_CH1].detected_cells))
      {
        run_flags |= RUN_FLAGS_AUTO_STOPPPED_CH1;
        Start_Stop_Discharge(0, 0);     
      }
    }
     
    if( run_flags & RUN_FLAGS_START_CH2)
    {
      if( meting_volts_ch2 < (cel_stop_voltage_ch2 * meters[METER_VOLT_CH2].detected_cells))
      {
        run_flags |= RUN_FLAGS_AUTO_STOPPPED_CH2;
        Start_Stop_Discharge(1, 0);           
      }
    }
#endif
     
  }
  
  if (updateTime <= millis())
  {
    if( !(run_flags & RUN_FLAGS_ADC_DIAG))
    {
      plotPointer();
     
      plotNeedle(meting_volts_ch1, meting_amps_ch1, mah_ch1, 0, &meters[METER_VOLT_CH1], cel_stop_voltage_ch1, run_timer_ch1);
      plotNeedle(meting_volts_ch2, meting_amps_ch2, mah_ch2, 0, &meters[METER_VOLT_CH2], cel_stop_voltage_ch2, run_timer_ch2);
    }

    if( run_flags & RUN_FLAGS_ADC_DIAG)
    {
      plot_adc_diag(0);
    }
    
    updateTime = millis() + 200;
  }

  if (Time500ms <= millis())
  {
    Blink_Encoder_Leds();
    Time500ms = millis() + 500;
  }
  
  if (Time1000ms <= millis())
  {

    if( run_flags & RUN_FLAGS_START_CH1)
    {
      Calc_Mah(0);
      run_timer_ch1++;
    }
      
    if( run_flags & RUN_FLAGS_START_CH2)
    {
      Calc_Mah(1);
      run_timer_ch2++;
    }
      
    Time1000ms = millis() + 1000;
  }
}

void Blink_Encoder_Leds(void)
{
  if( !(run_flags & RUN_FLAGS_START_CH1))
  {
     if( run_flags & RUN_FLAGS_AUTO_STOPPPED_CH1)
     {
       digitalWrite(ENCODER_1_LED_BLUE, LOW);
       digitalWrite(ENCODER_1_LED_GREEN, HIGH);
       digitalWrite(ENCODER_1_LED_RED, HIGH);
     }
     else
     { 
       if( meters[METER_VOLT_CH1].detected_cells >= 3)
       {
        if( digitalRead(ENCODER_1_LED_GREEN))
        {
          digitalWrite(ENCODER_1_LED_GREEN, LOW);
        }
        else
        {
          digitalWrite(ENCODER_1_LED_GREEN, HIGH);       
        }

        digitalWrite(ENCODER_1_LED_RED, HIGH);                  
       }
       else
       {
        if( digitalRead(ENCODER_1_LED_RED))
          digitalWrite(ENCODER_1_LED_RED, LOW);
        else
          digitalWrite(ENCODER_1_LED_RED, HIGH);
                      
        digitalWrite(ENCODER_1_LED_GREEN, HIGH);
       }
       digitalWrite(ENCODER_1_LED_BLUE, HIGH);
     }     
  }
  else
  {
     if( digitalRead(ENCODER_1_LED_BLUE))
      digitalWrite(ENCODER_1_LED_BLUE, LOW);
     else
      digitalWrite(ENCODER_1_LED_BLUE, HIGH);     

    digitalWrite(ENCODER_1_LED_GREEN, HIGH);      
    digitalWrite(ENCODER_1_LED_RED, HIGH);    
  }
  
  if( !(run_flags & RUN_FLAGS_START_CH2))
  {
     if( run_flags & RUN_FLAGS_AUTO_STOPPPED_CH2)
     {
       digitalWrite(ENCODER_2_LED_BLUE, LOW);
       digitalWrite(ENCODER_2_LED_GREEN, HIGH);
       digitalWrite(ENCODER_2_LED_RED, HIGH);
     }
     else
     { 
       if( meters[METER_VOLT_CH2].detected_cells >= 3)
       {
        if( digitalRead(ENCODER_2_LED_GREEN))
          digitalWrite(ENCODER_2_LED_GREEN, LOW);
        else
          digitalWrite(ENCODER_2_LED_GREEN, HIGH);     

        digitalWrite(ENCODER_2_LED_RED, HIGH);                  
       }
       else
       {
        if( digitalRead(ENCODER_2_LED_RED))
          digitalWrite(ENCODER_2_LED_RED, LOW);
        else
          digitalWrite(ENCODER_2_LED_RED, HIGH);
                      
        digitalWrite(ENCODER_2_LED_GREEN, HIGH);
       }
       digitalWrite(ENCODER_2_LED_BLUE, HIGH);
     }     
  }
  else
  {
     if( digitalRead(ENCODER_2_LED_BLUE))
      digitalWrite(ENCODER_2_LED_BLUE, LOW);
     else
      digitalWrite(ENCODER_2_LED_BLUE, HIGH);     

    digitalWrite(ENCODER_2_LED_GREEN, HIGH);      
    digitalWrite(ENCODER_2_LED_RED, HIGH);    
  }

}

void Start_Stop_Discharge(int channel, int mode)
{
    if( !channel)
    {
        if( mode)
        {
          run_flags |= RUN_FLAGS_START_CH1;
          run_flags &= (~RUN_FLAGS_AUTO_STOPPPED_CH1);
          digitalWrite(fet_ch1, HIGH);
          run_timer_ch1 = 0;
          mah_ch1 = 0;
          mah_ch1_calc = 0;
        }
        else
        {
          run_flags &= (~RUN_FLAGS_START_CH1);
          digitalWrite(fet_ch1, LOW);
        }
        return;                               
    }
    else
    {
        if( mode)
        {
          run_flags |= RUN_FLAGS_START_CH2;
          run_flags &= (~RUN_FLAGS_AUTO_STOPPPED_CH2);
          digitalWrite(fet_ch2, HIGH);
          run_timer_ch2 = 0;
          mah_ch2 = 0;
          mah_ch2_calc = 0;
        }
        else
        {
          run_flags &= (~RUN_FLAGS_START_CH2);
          digitalWrite(fet_ch2, LOW);
        }      
    }  
}

void Calc_Mah(int channel)
{
    float  hulp;
    
    if( !channel)
    {
       hulp = meting_amps_ch1 * 100;                                                      // Get into Mah value
       hulp /= 3600;
       mah_ch1_calc += hulp;
       mah_ch1 = mah_ch1_calc;
    }
    else
    {      
       hulp = meting_amps_ch2 * 100;                                                      // Get into Mah value
       hulp /= 3600;
       mah_ch2_calc += hulp;
       mah_ch2 = mah_ch2_calc;
    }
}


void Decode_Key_action(int key)
{
    int i;
    
  if( key != KEY_NONE)
  {
      switch(key)
      {
        case KEY_APPL_START_CH1 : if( !(run_flags & RUN_FLAGS_START_CH1))
                                  {
                                    if( meters[METER_VOLT_CH1].detected_cells >= 3)
                                    {
                                      Start_Stop_Discharge(0, 1);
                                      meters[0].old_run_timer = -999;
                                    }
                                  }
                                  break;
                                  
        case KEY_APPL_START_CH2 : if( !(run_flags & RUN_FLAGS_START_CH2))
                                  {
                                    if( meters[METER_VOLT_CH2].detected_cells >= 3)
                                    {
                                      Start_Stop_Discharge(1, 1);
                                      meters[1].old_run_timer = -999;
                                    }
                                  }
                                  break;
                                  
        case KEY_APPL_STOP_CH1 : if( run_flags & RUN_FLAGS_START_CH1)
                                  {
                                    Start_Stop_Discharge(0, 0);
                                  }
                                  break;
                                  
        case KEY_APPL_STOP_CH2 : if( run_flags & RUN_FLAGS_START_CH2)
                                  {
                                    Start_Stop_Discharge(1, 0);
                                  }
                                  break;
  
       case KEY_APPL_PLUS_CH1 :
                                  if( cel_stop_voltage_ch1 < CEL_STOP_MAX)
                                  {
                                    cel_stop_voltage_ch1_edit++;
                                    cel_stop_voltage_ch1 = cel_stop_voltage_ch1_edit / 2;                                    
                                  }
                                  break;
       case KEY_APPL_PLUS_CH2 :
                                  if( cel_stop_voltage_ch2 < CEL_STOP_MAX)
                                  {
                                    cel_stop_voltage_ch2_edit++;
                                    cel_stop_voltage_ch2 = cel_stop_voltage_ch2_edit / 2;                                    
                                  }
                                  break;
                                  
       case KEY_APPL_MIN_CH1 :
                                  if( cel_stop_voltage_ch1 > CEL_STOP_MIN)
                                  {
                                    cel_stop_voltage_ch1_edit--;
                                    cel_stop_voltage_ch1 = cel_stop_voltage_ch1_edit / 2;                                    
                                  }
                                  break;
                                  
       case KEY_APPL_MIN_CH2 :
                                  if( cel_stop_voltage_ch2 > CEL_STOP_MIN)
                                  {
                                    cel_stop_voltage_ch2_edit--;
                                    cel_stop_voltage_ch2 = cel_stop_voltage_ch2_edit / 2;                                    
                                  }
                                  break;
  
       case KEY_APPL_ADC_DIAG :  if( !(run_flags & RUN_FLAGS_ADC_DIAG))
                                  {
                                    Start_Stop_Discharge(0, 1);
                                    Start_Stop_Discharge(1, 1);
                                    run_flags |= RUN_FLAGS_ADC_DIAG;
                                    run_flags &= (~(RUN_FLAGS_START_CH1 + RUN_FLAGS_START_CH1));
                                    Build_Adc_Diag_Screen();
                                  }
                                  else
                                  {
                                    Start_Stop_Discharge(0, 0);
                                    Start_Stop_Discharge(1, 0);

                                    for( i = 0; i < 12; i++)
                                      old_plot_pointer[i] = -1;
                                      
                                    run_flags &= (~RUN_FLAGS_ADC_DIAG);
                                    Build_Main_Screen();
                                  }
                                  break;
      }
  }
}

void Auto_Detect_Cells(void)
{
  if( !(run_flags & RUN_FLAGS_START_CH1))
    meters[METER_VOLT_CH1].detected_cells = Cell_Count(0);
    
  if( !(run_flags & RUN_FLAGS_START_CH2))
    meters[METER_VOLT_CH2].detected_cells = Cell_Count(6);
}

int Cell_Count(int index)
{
   int  cells = 0;
   int  i;

   for( i = 0; i < 6; i++)
   {
      if( balancer_volts[i + index] > MIN_CEL_VOLTAGE_OK)
        cells++;    
   }

   return( cells);
}

int Pack_Voltage(int nr, int cell_count)
{
  int index = 0;
  int val;

  if( !cell_count)
    cell_count = 6;

  if( nr)
    cell_count += 6;
  val = balancer_volts[cell_count -1];

  return( val);
  
}


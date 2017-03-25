
#define TFT_GREY 0x5AEB

#define MAX_CONV_SPANNIG_METER            280                     // 26.0 V

void InitMeter(byte meter_nr, METER_DATA *mtr)
{
  mtr->ltx = 0;    // Saved x coord of bottom of needle
  mtr->osx = 120;
  mtr->osy = 120;
  mtr->meter_nr = meter_nr;  
  mtr->old_analog = -999;
  mtr->old_run_timer = -999;
  mtr->old_spanning = -999;
  mtr->old_amps = -999;
  mtr->old_stop_voltage = -999;
  mtr->old_mah = -9999;
  mtr->detected_cells = 0;
  mtr->old_detected_cells = -999;
}

// #########################################################################
//  Draw the analogue meter on the screen
// #########################################################################
void analogMeter(METER_DATA *mtr)
{
    int startx = 0;

    // Long scale tick length
    int tl = 15;
    float sx;
    float sy;
    uint16_t x0;
    uint16_t y0;
    uint16_t x1;
    uint16_t y1;

    // Coordinates of next tick for zone fill
    float sx2;
    float sy2;
    int x2;
    int y2;
    int x3;
    int y3;

  if( mtr->meter_nr == METER_VOLT_CH2)
    startx = 240;
  // Meter outline
  tft.fillRect(0 + startx, 0, 239 + startx, 126, TFT_GREY);
  tft.fillRect(5 + startx, 3, 230 + startx, 119, TFT_WHITE);

  tft.setTextColor(TFT_BLACK);  // Text colour

  // Draw ticks every 5 degrees from -50 to +50 degrees (100 deg. FSD swing)
  for (int i = -50; i < 51; i += 5)
  {
    // Coodinates of tick to draw
    sx = cos((i - 90) * 0.0174532925);
    sy = sin((i - 90) * 0.0174532925);
    x0 = sx * (100 + tl) + 120+ startx;
    y0 = sy * (100 + tl) + 140;
    x1 = sx * 100 + 120 + startx;
    y1 = sy * 100 + 140;

    // Coordinates of next tick for zone fill
    sx2 = cos((i + 5 - 90) * 0.0174532925);
    sy2 = sin((i + 5 - 90) * 0.0174532925);
    x2 = sx2 * (100 + tl) + 120 + startx;
    y2 = sy2 * (100 + tl) + 140;
    x3 = sx2 * 100 + 120 + startx;
    y3 = sy2 * 100 + 140;

    // Yellow zone limits
    if (i >= -50 && i < 0)
    {
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_RED);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_RED);
    }

    // Green zone limits
    if (i >= 0 && i < 25)
    {
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_YELLOW);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_YELLOW);
    }

    // Orange zone limits
    if (i >= 25 && i < 50)
    {
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_GREEN);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_GREEN);
    }

    // Short scale tick length
    if (i % 25 != 0)
      tl = 8;

    // Recalculate coords incase tick lenght changed
    x0 = sx * (100 + tl) + 120 + startx;
    y0 = sy * (100 + tl) + 140;
    x1 = sx * 100 + 120 + startx;
    y1 = sy * 100 + 140;

    // Draw tick
    tft.drawLine(x0, y0, x1, y1, TFT_BLACK);

    // Check if labels should be drawn, with position tweaks
    if (i % 25 == 0)
    {
      // Calculate label positions
      x0 = sx * (100 + tl + 10) + 120 + startx;
      y0 = sy * (100 + tl + 10) + 140;
      switch (i / 25)
      {
        case -2: tft.drawCentreString("0", x0, y0 - 12, 2); break;
        case -1: tft.drawCentreString("10", x0, y0 - 9, 2); break;
        case 0: tft.drawCentreString("20", x0, y0 - 6, 2); break;
        case 1: tft.drawCentreString("24", x0, y0 - 9, 2); break;
        case 2: tft.drawCentreString("28", x0, y0 - 12, 2); break;
      }
    }

    // Now draw the arc of the scale
    sx = cos((i + 5 - 90) * 0.0174532925);
    sy = sin((i + 5 - 90) * 0.0174532925);
    x0 = sx * 100 + 120 + startx;
    y0 = sy * 100 + 140;
    // Draw scale arc, don't draw the last part
    if (i < 50) tft.drawLine(x0, y0, x1, y1, TFT_BLACK);
  }

    if( mtr->meter_nr == METER_VOLT_CH2)
      tft.drawString("CH2", 5 + 230 - 40 + startx, 119 - 20, 2); // Units at bottom right
    else
      tft.drawString("CH1", 5 + 230 - 40 + startx, 119 - 20, 2); // Units at bottom right
  tft.drawCentreString("Volt", 120 + startx, 70, 4); // Comment out to avoid font 4
  
  Meter_dsp_cells(startx, mtr);
  
  tft.drawRect(5, 3 + startx, 230, 119, TFT_BLACK); // Draw bezel line

  plotNeedle(0, 0, 0, 0, mtr, 0, 0); // Put meter needle at 0
}

void Meter_dsp_cells(int startx, METER_DATA *mtr)
{
  char buf[8];  

  if( mtr->detected_cells < 3)
    tft.setTextColor(TFT_RED, TFT_WHITE);
  else
    tft.setTextColor(TFT_BLACK, TFT_WHITE);

  buf[0] = mtr->detected_cells + 0x30;
  buf[1] = 0x20;
  buf[2] = 'S';
  buf[3] = 0; 
  tft.drawCentreString(&buf[0], 120 + startx, 90, 4); // Comment out to avoid font 4
  
}

// #########################################################################
// Update needle position
// This function is blocking while needle moves, time depends on ms_delay
// 10ms minimises needle flicker if text is drawn within needle sweep area
// Smaller values OK if text not in sweep area, zero for instant movement but
// does not look realistic... (note: 100 increments for full scale deflection)
// #########################################################################
void plotNeedle(int value, int amps, int mah, byte ms_delay, METER_DATA *mtr, int cel_stop_voltage, int run_timer)
{
    int startx = 0;
    char buf[16];  
    float spanning = value;
    float stop_voltage = cel_stop_voltage;
    float amps_d = amps;
    int run_mode = 0;
    
    if( mtr->meter_nr == METER_VOLT_CH2)
    {
      startx = 240;
      if(run_flags & RUN_FLAGS_START_CH2) 
        run_mode = 1;
    }
    else
    {
      if(run_flags & RUN_FLAGS_START_CH1) 
        run_mode = 1;
    }

  if( mtr->detected_cells != mtr->old_detected_cells)
  {
    mtr->old_detected_cells = mtr->detected_cells;  
    Meter_dsp_cells(startx, mtr);
  }
  
  if( value != mtr->old_spanning)
  {
    spanning /= 10;
    mtr->old_spanning = value;
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    dtostrf(spanning, 3, 1, &buf[0]);
    tft.drawRightString(&buf[0], 40 + startx, 119 - 20, 2);
  }

  if( value > MAX_CONV_SPANNIG_METER) value = MAX_CONV_SPANNIG_METER;
  if( value <= 200)
    value = map(value, 0, 200, 0, 50);
  else
  {
    if( value <= 240)
    {
      value = map((value - 200), 0, 40, 0, 25);
      value += 50;
    }
    else
    {    
      value = map((value - 240), 0, 40, 0, 25);
      value += 75;
    }
  }
  
  if (value < -10) value = -10; // Limit value to emulate needle end stops
  if (value > 110) value = 110;

  if( amps != mtr->old_amps)
  {
    mtr->old_amps = amps;
    amps_d /= 10;
    buf[0] = 0;
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    dtostrf(amps_d, 2, 1, buf);
    strcat(buf, " A");
    tft.drawString(buf, 5 + startx, 125, 4); // // Comment out to avoid font 4
  }

  if( cel_stop_voltage |= mtr->old_stop_voltage)
  {
    mtr->old_stop_voltage = cel_stop_voltage;
    buf[0] = 0;
    tft.setTextColor(TFT_CYAN, TFT_NAVY);
    stop_voltage /= 100;
    dtostrf(stop_voltage, 4, 2, buf);
    strcat(buf, " V");
    tft.drawString(buf, 5 + startx, 146, 4); // // Comment out to avoid font 4
  }

  if( mah != mtr->old_mah)
  { 
    mtr->old_mah = mah;
    buf[0] = 0;
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    dtostrf(mah, 6, 0, buf);
    strcat(buf, " mAh");
    tft.drawRightString(buf, 235 + startx, 125, 4); // // Comment out to avoid font 4
  }

  buf[0] = 0;    
  if( !run_mode)
  {
    tft.setTextColor(TFT_GREEN, TFT_NAVY);
    strcat(buf, "Stopped");
  }
  else
  {
    tft.setTextColor(TFT_YELLOW, TFT_NAVY);
    strcat(buf, "Running");
  }
  tft.drawCentreString(buf, 125 + startx, 146, 4);

  if( run_timer != mtr->old_run_timer)
  {  
      mtr->old_run_timer = run_timer;  
      buf[0] = 0;
      tft.setTextColor(TFT_WHITE, TFT_NAVY);
      sprintf(buf, "%02d:%02d:%02d", run_timer / 3600, (run_timer / 60) % 60, run_timer % 60);
      tft.drawRightString(buf, 235 + startx, 150, 2); // // Comment out to avoid font 4
  }

  
  // Move the needle util new value reached
  while (!(value == mtr->old_analog))
  {
    if (mtr->old_analog < value)
      mtr->old_analog++;
    else
      mtr->old_analog--;

    if (ms_delay == 0)
      mtr->old_analog = value; // Update immediately id delay is 0

    float sdeg = map(mtr->old_analog, -10, 110, -150, -30); // Map value to angle
    // Calcualte tip of needle coords
    float sx = cos(sdeg * 0.0174532925);
    float sy = sin(sdeg * 0.0174532925);

    // Calculate x delta of needle start (does not start at pivot point)
    float tx = tan((sdeg + 90) * 0.0174532925);

    // Erase old needle image
    tft.drawLine(120 + 20 * mtr->ltx - 1 + startx, 140 - 20, mtr->osx - 1 + startx, mtr->osy, TFT_WHITE);
    tft.drawLine(120 + 20 * mtr->ltx + startx, 140 - 20, mtr->osx + startx, mtr->osy, TFT_WHITE);
    tft.drawLine(120 + 20 * mtr->ltx + 1 + startx, 140 - 20, mtr->osx + 1 + startx, mtr->osy, TFT_WHITE);

    // Re-plot text under needle
    tft.setTextColor(TFT_BLACK);
    tft.drawCentreString("Volt", 120 + startx, 70, 4); // // Comment out to avoid font 4

    // Store new needle end coords for next erase
    mtr->ltx = tx;
    mtr->osx = sx * 98 + 120;
    mtr->osy = sy * 98 + 140;

    // Draw the needle in the new postion, magenta makes needle a bit bolder
    // draws 3 lines to thicken needle
    tft.drawLine(120 + 20 * mtr->ltx - 1 + startx, 140 - 20, mtr->osx - 1 + startx, mtr->osy, TFT_RED);
    tft.drawLine(120 + 20 * mtr->ltx + startx, 140 - 20, mtr->osx + startx, mtr->osy, TFT_MAGENTA);
    tft.drawLine(120 + 20 * mtr->ltx + 1 + startx, 140 - 20, mtr->osx + 1 + startx, mtr->osy, TFT_RED);

    // Slow needle down slightly as it approaches new postion
    if (abs(mtr->old_analog - value) < 10) ms_delay += ms_delay / 5;

    // Wait before next update
    delay(ms_delay);
  }
}

// #########################################################################
//  Draw a linear meter on the screen
// #########################################################################
void plotLinear(char *label, int x, int y)
{
  int w = 36;
  tft.drawRect(x, y, w, 155, TFT_GREY);
  tft.fillRect(x+2, y + 19, w-3, 155 - 38, TFT_WHITE);
  
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawCentreString(label, x + w / 2, y + 2, 2);

  for (int i = 0; i < 110; i += 10)
  {
    tft.drawFastHLine(x + 20, y + 27 + i, 6, TFT_BLACK);
  }

  for (int i = 0; i < 110; i += 50)
  {
    tft.drawFastHLine(x + 20, y + 27 + i, 9, TFT_BLACK);
  }
  
  //tft.fillTriangle(x+3, y + 127, x+3+16, y+127, x + 3, y + 127 - 5, TFT_RED);
  //tft.fillTriangle(x+3, y + 127, x+3+16, y+127, x + 3, y + 127 + 5, TFT_RED);
  
  tft.drawCentreString("---", x + w / 2, y + 155 - 18, 2);
}

// #########################################################################
//  Adjust 6 linear meter pointer positions
// #########################################################################
void plotPointer(void)
{
  int dy = 195;
  byte pw = 8;
  char buf[8];
  int  dx;
  int  startx = 240;
  float volts;
  int   plot_val;
  
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);

// Convert channels 1--6 to display
  for (int i = 0; i < 6; i++)
  {
    if( i)                                                        // Calc difference for cell spanning.
      plot_val = balancer_volts[i] - balancer_volts[i - 1];
    else
      plot_val = balancer_volts[i];
      
    if( plot_val < 0) plot_val = 0;
    if( plot_val > 50) plot_val = 50; 
    
    volts = plot_val;
    volts /= 10; 
    dtostrf(volts, 3, 1, buf);
    tft.drawRightString(buf, i * 40 + 36 - 5, 195 - 27 + 155 - 18, 2);

    dx = 3 + 40 * i;
    
    plot_val *= 2;

    while (plot_val != old_plot_pointer[i])
    {
      dy = 195 + 100 - old_plot_pointer[i];
      if (old_plot_pointer[i] > plot_val)
      {
        tft.drawLine(dx, dy - 5, dx + pw, dy, TFT_WHITE);
        old_plot_pointer[i]--;
        tft.drawLine(dx, dy + 6, dx + pw, dy + 1, TFT_RED);
      }
      else
      {
        tft.drawLine(dx, dy + 5, dx + pw, dy, TFT_WHITE);
        old_plot_pointer[i]++;
        tft.drawLine(dx, dy - 6, dx + pw, dy - 1, TFT_RED);
      }
    }
  }

// Convert channels 7--12 to display
  
  for (int i = 0; i < 6; i++)
  {
    if( i)                                                        // Calc difference for cell spanning.
      plot_val = balancer_volts[i + 6] - balancer_volts[(i - 1) + 6];
    else
      plot_val = balancer_volts[i + 6];
      
    if( plot_val < 0) plot_val = 0;
    if( plot_val > 50) plot_val = 50; 
      
    volts = plot_val;
    volts /= 10;   
    dtostrf(volts, 3, 1, buf);
    tft.drawRightString(buf, ( i * 40 + 36 - 5) + startx, 195 - 27 + 155 - 18, 2);

    dx = 3 + 40 * i + startx;
    plot_val *= 2;

    while (plot_val != old_plot_pointer[i + 6])
    {
      dy = 195 + 100 - old_plot_pointer[i + 6];
      if (old_plot_pointer[i + 6] > plot_val)
      {
        tft.drawLine(dx, dy - 5, dx + pw, dy, TFT_WHITE);
        old_plot_pointer[i + 6]--;
        tft.drawLine(dx, dy + 6, dx + pw, dy + 1, TFT_RED);
      }
      else
      {
        tft.drawLine(dx, dy + 5, dx + pw, dy, TFT_WHITE);
        old_plot_pointer[i + 6]++;
        tft.drawLine(dx, dy - 6, dx + pw, dy - 1, TFT_RED);
      }
    }
  }
}

void plot_adc_diag(int base)
{
  int dy = 195;
  char buf[8];
  char line[32];
  float volts;
  int   i;
  
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);

  // Move the 6 pointers one pixel towards new value
  for (i = 0; i < 12; i++)
  {
    volts = map(raw_adc[i], 0, 1023, 0, MAX_CONV_SPANNIG);
    volts /= 10; 
    dtostrf(volts, 3, 1, buf);
    if( i > 5)
      sprintf(&line[0], "channel %d raw %d conv %s V", i + 2, raw_adc[i], &buf[0]);
    else
      sprintf(&line[0], "channel %d raw %d conv %s V", i + 1, raw_adc[i], &buf[0]);
    tft.drawRightString(&line[0], 5, 5 + (i * 20), 2);
  }

    volts = meting_amps_ch1;
    volts /= 10; 
    dtostrf(volts, 3, 1, buf);
    sprintf(&line[0], "channel %d raw %d conv %s A", 7, raw_adc[12], &buf[0]);
    tft.drawRightString(&line[0], 5, 5 + (12 * 20), 2);

    volts = meting_amps_ch2;
    volts /= 10; 
    dtostrf(volts, 3, 1, buf);
    sprintf(&line[0], "channel %d raw %d conv %s A", 14, raw_adc[13], &buf[0]);
    tft.drawRightString(&line[0], 5, 5 + (13 * 20), 2);
}


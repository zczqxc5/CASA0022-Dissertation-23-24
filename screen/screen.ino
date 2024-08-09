#include <st7789v2.h>
#include "SPI.h"
#include "seeed.h"

st7789v2 Display;

void setup() {
  // put your setup code here, to run once:
  Display.SetRotate(180);
  Display.Init();
  Display.SetBacklight(100);
  Display.Clear(WHITE);
    Display.DrawLine(15, 65, 65, 65, MAGENTA, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
  Display.DrawLine(15, 70, 80, 70, MAGENTA, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
  
  Display.DrawRectangle(15, 80, 225, 150, GRAY, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
  
  Display.DrawCircle(10, 10, 25, MAGENTA, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
  Display.DrawCircle(10, 10, 20, GRAYBLUE, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
  Display.DrawCircle(10, 10, 15, MAGENTA, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
  Display.DrawCircle(10, 10, 10, GRAYBLUE, DOT_PIXEL_2X2, DRAW_FILL_FULL);

  Display.DrawCircle(230, 10, 25, MAGENTA, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
  Display.DrawCircle(230, 10, 20, GRAYBLUE, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
  Display.DrawCircle(230, 10, 15, MAGENTA, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
  Display.DrawCircle(230, 10, 10, GRAYBLUE, DOT_PIXEL_2X2, DRAW_FILL_FULL);

  Display.DrawCircle(10, 270, 25, MAGENTA, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
  Display.DrawCircle(10, 270, 20, GRAYBLUE, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
  Display.DrawCircle(10, 270, 15, MAGENTA, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
  Display.DrawCircle(10, 270, 10, GRAYBLUE, DOT_PIXEL_2X2, DRAW_FILL_FULL);

  Display.DrawCircle(230, 270, 25, MAGENTA, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
  Display.DrawCircle(230, 270, 20, GRAYBLUE, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
  Display.DrawCircle(230, 270, 15, MAGENTA, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
  Display.DrawCircle(230, 270, 10, GRAYBLUE, DOT_PIXEL_2X2, DRAW_FILL_FULL);

  Display.DrawLine(195, 160, 225, 160, GRAYBLUE, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
  Display.DrawLine(175, 165, 225, 165, GRAYBLUE, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
  
  Display.DrawString_EN(20, 180, "By: Xin Cheng", &Font20, WHITE, BLACK);

}

void loop() {
  // put your main code here, to run repeatedly:
//  Display.SetPixel(100, 100, RED);
//  Display.DrawPoint(50, 50, YELLOW, DOT_PIXEL_8X8, DOT_FILL_AROUND);

 



  
}
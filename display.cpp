#include "display.h"
#include <bb_spi_lcd.h>
#include "ble_manager.h"
#include "jk_data.h"

BB_SPI_LCD lcd;

void displayBegin()
{
  lcd.begin(DISPLAY_CYD_22C);
  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_GREY, TFT_BLACK);
  // lcd.print("Starting..");
  lcd.setRotation(270);
  lcd.setFont(FONT_12x16);
  lcd.setCursor(17, 8); // Set cursor position
  lcd.println("el'Komunist");

  lcd.setTextColor(TFT_GREY, TFT_BLACK);
  lcd.setCursor(170, 40); // Set cursor position
  lcd.println("Remain:");
  lcd.setCursor(15, 135); // Set cursor position
  lcd.println("Power:");
  lcd.setCursor(170, 135); // Set cursor position
  lcd.println("Cell:");
}

void displayUpdate() {
  bool connected = bleConnected();
  // top status
  lcd.setCursor(175, 8); // Set cursor position
  if (!connected) {
      lcd.setTextColor(TFT_RED, TFT_BLACK);
      lcd.println("Connecting    ");
    }
    else {
      lcd.setTextColor(TFT_BLUE, TFT_BLACK);
      //char deg = 248;
      //lcd.println(String(jkData.temperature2, 0) + deg + " C       ");
      lcd.println(String(jkData.temperature2, 1) + " C         ");
      // lcd.println("Connected   ");
    }

  lcd.setFont(FONT_16x32);
  // top left
  lcd.setCursor(15, 60); // Set cursor position

  if (!connected) {
      lcd.setTextColor(TFT_GREY, TFT_BLACK);
    }
    else if (jkData.totalVoltage >= 52) {
      lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    }
    else if (jkData.totalVoltage <= 50) {
      lcd.setTextColor(TFT_RED, TFT_BLACK);
    }
    else {
      lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
    }
  lcd.println(String(jkData.totalVoltage) + " V  "); // Display the voltage

  // top right
  lcd.setCursor(170, 60); // Set cursor position
  if (!connected) {
      lcd.setTextColor(TFT_GREY, TFT_BLACK);
    }
    else if (jkData.stateOfCharge >= 66) {
      lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    }
    else if (jkData.stateOfCharge <= 33) {
      lcd.setTextColor(TFT_RED, TFT_BLACK);
    }
    else {
      lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
    }
  lcd.println(String(jkData.stateOfCharge, 0) + " %  "); // Display the percentage
  lcd.setCursor(170, 90); // Set cursor position
  lcd.setFont(FONT_12x16);
  lcd.println(String(jkData.remainingAh, 1) + " Ah  "); // Display the Ah

  // lower left
  lcd.setFont(FONT_16x32);
  lcd.setCursor(15, 155); // Set cursor position
  if (!connected) {
      lcd.setTextColor(TFT_GREY, TFT_BLACK);
    }
    else if (jkData.current <= -19) {
      lcd.setTextColor(TFT_RED, TFT_BLACK);
    }
    else if (jkData.current <= -12) {
      lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
    }
    else {
      lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    }
  lcd.println(String(jkData.current, 1) + " A   "); // Display the amps
  lcd.setCursor(15, 185); // Set cursor position
  lcd.setFont(FONT_12x16);
  lcd.println(String(jkData.power, 0) + " W   "); // Display the watt

  lcd.setCursor(15, 215); // Set cursor position
  //lcd.setFont(FONT_12x16);
  if (jkData.alarms) {
      lcd.setTextColor(TFT_RED, TFT_BLACK);
      //lcd.println("Error: " + String(jkData.alarms));
      lcd.println("Error: " + String(jkData.alarms) + " " + jkData.alarmText );
    }
    else if (!connected) {
      //lcd.setTextColor(TFT_GREY, TFT_BLACK);
      lcd.println("           "); // power bar
    }
    else if (jkData.current <= -20) {
      lcd.println("# # # # #> "); // power bar
    }
    else if (jkData.current <= -15) {
      lcd.println("# # # #>   "); // power bar
    }
    else if (jkData.current <= -10) {
      lcd.println("# # #>     "); // power bar
    }
    else if (jkData.current <= -5) {
      lcd.println("# #>       "); // power bar
    }
    else if (jkData.current <= -1) {
      lcd.println("#>         "); // power bar
    }
    else {
      lcd.println(">           "); // power bar
    }
  lcd.setFont(FONT_16x32);

  // lower right
  lcd.setCursor(170, 155); // Set cursor position
  if (!connected) {
      lcd.setTextColor(TFT_GREY, TFT_BLACK);
    }
    else if (jkData.averageCellVoltage <= 3.1) {
      lcd.setTextColor(TFT_RED, TFT_BLACK);
    }
    else if (jkData.averageCellVoltage <= 3.23) {
      lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
    }
    else {
      lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    }
  lcd.println("Ave:" + String(jkData.averageCellVoltage) + " V "); // Display the cell ave
  
  lcd.setCursor(170, 185); // Set cursor position
  if (jkData.cellDiff >= 0.07) {
      lcd.setTextColor(TFT_RED, TFT_BLACK);
    }
    else if (!connected) {
      lcd.setTextColor(TFT_GREY, TFT_BLACK);
    }
    else {
      lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    }
  lcd.setFont(FONT_12x16);
  int delta_cell = int(jkData.cellDiff * 1000);  // temporary local variable.
  lcd.println("Del: " + String(delta_cell) + " mV   "); // Display the cell delta

}

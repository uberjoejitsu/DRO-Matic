/*
*  DROMatic.ino
*  DROMatic OS Core
*  Devin R. Olsen - July 4th, 2017
*  devin@devinrolsen.com
*/
#include <Wire.h>    //Include the software serial library for white sheild
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal.h> //lib for interfacing with LCD screen
#include <SPI.h> //Suppoting lib for SD card
#include <SD.h> //SD card API
#include <StandardCplusplus.h> //STD
#include <StandardCplusplus\vector> //Vectors
#include <StandardCplusplus\ctime> //Time helper
#include <ArduinoJson\ArduinoJson.h> //Arduno Json (aka epic)
#include <DS3231.h> //Real time clock lib
 
#include "Globals.h" //All temp and PROGMEM global variables
#include "Core.h" //All core functions and variables
#include "Crops.h" //All crop functions and variables
#include "Pumps.h" //All pump functions and variables
#include "Regimens.h" //All session functions and variables
#include "Screens.h" //All screen functions and variables
#include "Menus.h" //All menu functions and variables
#include "DatesTime.h" //All date & time functions and variables
#include "Irrigation.h" //All irrigation related functions and variables
#include "Timers.h" //All timer related functions and variables

//OS main setup
void setup()
{
	lcd.createChar(0, upArrow);
	lcd.createChar(1, downArrow);
	lcd.begin(16, 2);
	pixels.begin(); // This initializes the NeoPixel library.

	//If SD Card is not found, we can't proceed
	if (!SD.begin(53)){
		lcd.print(F("SD Card Required"));
		lcd.setCursor(0, 1);
		lcd.print(F("Insert And Reset"));
		screenName = "REQUIREDSD";
	} else {//if SD card is found, proceed with crop load or setup
		rtc.begin(); //Realtime Clock setup AFTER having built and loaded crop
		captureDateTime(); //Capture current time from real time clock

		//Setup Flow Sensor Pins
		pinMode(FlowPinIn, INPUT);	//irrigation "in" flow meter
		digitalWrite(FlowPinIn, HIGH);
		pinMode(FlowPinOut, INPUT);	//irrigation "out" flow meter
		digitalWrite(FlowPinOut, HIGH);

		//irrigation flow meters being hooked into flow counter methods
		attachInterrupt(digitalPinToInterrupt(FlowPinIn), countRsvrFill, FALLING);
		attachInterrupt(digitalPinToInterrupt(FlowPinOut), countRsvrDrain, FALLING);

		//Setup Relay Pins
		pinMode(RELAY1, OUTPUT);	//perstaltic pump 1
		digitalWrite(RELAY1, HIGH);
		pinMode(RELAY2, OUTPUT);	//perstaltic pump 2
		digitalWrite(RELAY2, HIGH);
		pinMode(RELAY3, OUTPUT);	//perstaltic pump 3
		digitalWrite(RELAY3, HIGH);
		pinMode(RELAY4, OUTPUT);	//perstaltic pump 4
		digitalWrite(RELAY4, HIGH);
		pinMode(RELAY5, OUTPUT);	//perstaltic pump 5
		digitalWrite(RELAY5, HIGH);
		pinMode(RELAY6, OUTPUT);	//perstaltic pump 6
		digitalWrite(RELAY6, HIGH);
		pinMode(RELAY7, OUTPUT);	//perstaltic pump 7
		digitalWrite(RELAY7, HIGH);
		pinMode(RELAY8, OUTPUT);	//perstaltic pump 8
		digitalWrite(RELAY8, HIGH);
		pinMode(RELAY9, OUTPUT);	//perstaltic pump 9
		digitalWrite(RELAY9, HIGH);
		pinMode(RELAY10, OUTPUT);	//perstaltic pump 10
		digitalWrite(RELAY10, HIGH);
		pinMode(RELAY11, OUTPUT);	//irrigation "in" valve
		digitalWrite(RELAY11, HIGH);
		pinMode(RELAY12, OUTPUT);	//irrigation "out" valve
		digitalWrite(RELAY12, HIGH);

		pinMode(RELAY13, OUTPUT);	//High voltage power receptical 1
		digitalWrite(RELAY13, HIGH);
		pinMode(RELAY14, OUTPUT);	//High voltage power receptical 2
		digitalWrite(RELAY14, HIGH);
		pinMode(RELAY15, OUTPUT);	//High voltage power receptical 3
		digitalWrite(RELAY15, HIGH);
		pinMode(RELAY16, OUTPUT);	//High voltage power receptical 4
		digitalWrite(RELAY16, HIGH);

		//Serial.begin(9600);
		Wire.begin();   // enable I2C port.
		coreInit();		//Loads or Creates Crops
	}
}

//Our runtime loop function
void loop()
{
	Key = analogRead(0);

	//Reset home screen and menu timers to current miliseconds after any interaction with LCD keys
	if (Key >= 0 && Key <= 650){
		homeMillis = menuMillis = millis();
	}

	//60 seconds has passed - Check logic for action
	if (previousMinute != rtc.getTime().min && cropStatus != 0) {
		previousMinute = rtc.getTime().min;
		if (screenName == "" && (millis() - menuMillis >= 10000)) {
			correctRsvrPH();
			correctPlantPH();
			correctPlantEC();
			//checkRecepticals();
		}
	}
	
	//if 2 seconds has passed, reprint home screen
	//10 seconds has passed since interacting with menus start printing home screen
	if ((millis() - homeMillis) >= 2000 && (millis() - menuMillis >= 10000)) {
		if (screenName == ""){//of course non of this happens while we have a menu open
			homeMillis = millis(); //reset home millis so 2 seconds can happen again.
			printHomeScreen(); //finally we call the print home method
		}
	}

	//1000 miliseconds has passed - Realtime UI Menus update
	if ((millis() - homeMillis) >= 1000){
		checkFlowRates();	//OS monitors change to in and out water flow directions
		//doseCurrentRegimen();
		if (screenName == "RSVRVOL"){
			tmpFloats[0] += (flowInRate / 60) * 1000;
			float liters = tmpFloats[0] / 1000;
			float USgallons = tmpFloats[0] / 4546.091879;
			float UKgallons = USgallons * 0.83267384;

			lcd.clear();
			lcd.home();
			lcd.print(USgallons, 1);
			lcd.print(F("lqd/"));
			lcd.print(UKgallons, 1);
			lcd.print(F("gal"));
			lcd.setCursor(0, 1);
			lcd.print(F("<back>      <ok>"));
			lcd.setCursor(cursorX, cursorY);
			lcd.blink();
			homeMillis = millis();
		}
	}

	//UI Menus
	if (Key == 0 || Key == 408){
		//Left & Right
		if (screenName == "DATETIME"){
			matrix = {
				{ { 1, 1 }, { 4, 4 }, { 10, 10 }, { 13, 13 } },
				{ { 3, 3 }, { 6, 6 }, { 13, 13 } }
			};
		}
		if (screenName == "NEW"){
			matrix = {
				{ { 0, 8 } },
				{ { 11, 11 } }
			};
			if (Key == 0){
				cropRename(NULL);
				lcd.blink();
			}
		}
		if (screenName == "OPEN"){
			matrix = {
				{ { 0, 0 } },
				{ { 1, 1 }, { 9, 9 } }
			};
		}
		if (screenName == "RESET"){
			matrix = {
				{ {2, 2} },
				{ { 1, 1 }, { 9, 9 } }
			};
		}
		if (screenName == "DELETE") {}
		if (screenName == "STATUS") {
			matrix = {
				{ { 8, 8 } },
				{ { 1, 1 }, { 13, 13 } }
			};
		}
		if (screenName == "EC") {
			matrix = {
				{ { 3, 3 }, { 8, 8 }, { 15, 15 } },
				{ { 1, 1 }, { 11, 11 } }
			};
		}
		if (screenName == "PH"){
			matrix = {
				{ { 3, 3 }, {9, 9} },
				{ { 1, 1 }, { 13, 13 } }
			};
		}
		if (screenName == "RSVRVOL"){
			matrix = {
				{ { 0, 0 } },
				{ { 1, 1 }, { 13, 13 } }
			};
		}
		if (screenName == "DOSES"){
			matrix = {
				{ { 1, 1 } },
				{ { 1, 1 }, { 13, 13 } }
			};
		}
		if (screenName == "PUMPCAL"){
			matrix = {
				{ { 2, 2 } },
				{ { 1, 1 }, { 13, 13 } }
			};
		}
		if (screenName == "PUMPDLY"){
			matrix = {
				{ { 2, 2 } },
				{ { 1, 1 }, { 13, 13 } }
			};
		}
		if (screenName == "AMOUNT"){
			matrix = {
				{ { 12, 12 } },
				{ { 1, 1 }, { 11, 11 } }
			};
		}
		if (screenName == "TPFCCNT"){
			matrix = {
				{ { 0, 0 } },
				{ { 1, 1 }, { 13, 13 } }
			};
		}
		if (screenName == "TPFAMT"){
			matrix = {
				{ { 0, 0 } },
				{ { 1, 1 }, { 13, 13 } }
			};
		}
		if (screenName == "TPFDLY"){
			matrix = {
				{ { 2, 2 } },
				{ { 1, 1 }, { 13, 13 } }
			};
		}
		if (screenName == "DRNTIME"){
			matrix = {
				{ { 1, 1 } },
				{ { 1, 1 }, { 13, 13 } }
			};
		}
		if (screenName == "PRIME"){
			matrix = {
				{ { 0, 0 } },
				{ { 11, 11 } }
			};
		}
		if (screenName == "STARTEND"){
			matrix = {
				{ { 0, 0 }, { 3, 3 }, { 5, 5 }, { 8, 8 }, { 13, 13 } },
				{ { 1, 1 }, { 13, 13 } }
			};
		}
		if (screenName == "RECEP01" || screenName == "RECEP02" || screenName == "RECEP03" || screenName == "RECEP04"){
			matrix = {
				{ { 1, 1 }, { 6, 6 }, { 11, 11 }, { 14, 14 }, },
				{ { 1, 1 }, { 13, 13 } }
			};
		}
		if (screenName == "FLOWCAL"){
			matrix = {
				{ { 5, 5 }, { 13, 13 } },
				{ { 1, 1 }, { 13, 13 } }
			};
		}
		if (screenName == "PHCAL" || screenName == "PHCALLOW" || screenName == "PHCALHI"){
			matrix = {
				{ { 0, 0 } },
				{ { 1, 1 }, { 11, 11 } }
			};
		}
		if (screenName == "ECCAL" || screenName == "ECCALLOW" || screenName == "ECCALHI"){
			matrix = {
				{ { 10, 10 }, { 11, 11 }, { 12, 12 }, { 13, 13 }, { 14, 14 }, { 15, 15 } },
				{ { 1, 1 }, { 11, 11 } }
			};
		}
		if (screenName == "MANFLUSH"){
			matrix = {
				{ { 0, 0 } },
				{ { 1, 1 }, { 5, 5 }, {11, 11} }
			};
		}

		cursorX = (Key == 0) ? cursorX + 1 : cursorX - 1;
		if (Key == 408 && screenName == ""){
			menusHistory.pop_back();
			menuIndex = 0;
			tmpFile = SD.open("dromatic/" + cropName + "/" + getMenuHistory());
			getDirectoryMenus(tmpFile);
			tmpFile.close();
			lcd.clear();
			printScreenNames(menus.front());
			printScrollArrows();
		}
		if (Key == 0 || Key == 408 && screenName != ""){
			screenMatrix();
		}
		delay(250);
	}
	if (Key == 99 || Key == 255) {
		//Up & Down
		int dir = (Key == 99) ? 1 : -1;
		if (screenName == "DATETIME"){
			if (cursorY == 0){
				printDateTime(dir);
			} else if (cursorX == 1){
				printDateTime(dir);
			}
		}
		if (screenName == "NEW"){
			cropRename(dir);
		}
		if (screenName == "OPEN"){
			if (cursorX == 0 && cursorY == 0){
				lcd.clear();
				scrollMenus(dir);
				lcd.setCursor(0, 1);
				lcd.print(F("<back>  <open>"));
				lcd.home();
			}
		}
		if (screenName == "RESET") {}
		if (screenName == "DELETE") {}
		if (screenName == "STATUS") {
			if (cursorX == 8 && cursorY == 0){
				printStatus(dir);
			}
		}
		if (screenName == "RSVRVOL") {
			printReservoirVolume(dir);
		}
		if (screenName == "DOSES"){
			if (cursorX == 1 && cursorY == 0){
				printRegimenNumber(dir);
			}
		}
		if (screenName == "PUMPCAL"){
			if (cursorX == 2 && cursorY == 0){
				printPumpCalibration(dir);
			}
		}
		if (screenName == "PUMPDLY"){
			if (cursorX == 2 && cursorY == 0){
				printPumpDelay(dir);
			}
		}
		if (screenName == "AMOUNT"){
			if (cursorX == 12 && cursorY == 0){
				printRegimenAmount(dir);
			}
		}
		if (screenName == "TPFCCNT") {
			if (cursorX == 0 && cursorY == 0){
				printTopOffConcentrate(dir);
			}
		}
		if (screenName == "TPFAMT"){
			if (cursorX == 0 && cursorY == 0){
				printTopOffAmount(dir);
			}
		}
		if (screenName == "TPFDLY"){
			if (cursorX == 2 && cursorY == 0){
				printTopOffDelay(dir);
			}
		}
		if (screenName == "DRNTIME") {
			if (cursorX == 1 && cursorY == 0){
				printDrainTime(dir);
			}
		}
		if (screenName == "PRIME") {
			if (cursorX == 0 && cursorY == 0){
				primePump(dir);
			}
		}
		if (screenName == "STARTEND") {
			if (cursorX == 0 || cursorX == 3 || cursorX == 5 || cursorX == 8 || cursorX == 13 && cursorY == 0){
				printTimerStartEnd(dir);
			}
		}
		if (screenName == "RECEP01" || screenName == "RECEP02" || screenName == "RECEP03" || screenName == "RECEP04") {
			if (cursorX == 1 || cursorX == 6 || cursorX == 11 || cursorX == 14 && cursorY == 0){
				printTimerStartEnd(dir);
			}

		}
		if (screenName == "FLOWCAL") {
			if (cursorX == 5 || cursorX == 13 && cursorY == 0){
				printFlowCalibration(dir);
			}
		}
		if (screenName == "EC"){
			if (cursorX == 3 || cursorX == 8 || cursorX == 15 && cursorY == 0){
				printECRange(dir);
			}
		}
		if (screenName == "PH"){
			if (cursorX == 3 || cursorX == 9 && cursorY == 0){
				printPHRange(dir);
			}
		}
		if (screenName == "ECCAL" || screenName == "ECCALLOW" || screenName == "ECCALHI"){
			if ((cursorX >=10 || cursorX <=15) && cursorY == 0){
				if (screenName == "ECCAL"){
					printECCalibrations("DRY", dir);
				}
				if (screenName == "ECCALLOW"){
					printECCalibrations("LOW", dir);
				}
				if (screenName == "ECCALHI"){
					printECCalibrations("HIGH", dir);
				}
			}
		}
		if (screenName == ""){
			scrollMenus(dir);
		}
		delay(250);
	}
	if (Key == 639) {
		//Select
		if (screenName == ""){
			lcd.clear();
			String history = getMenuHistory();
			tmpFile = SD.open("dromatic/" + cropName + "/" + history + "/" + menus[menuIndex]);
			menusHistory.push_back(menus[menuIndex]);
			getDirectoryMenus(tmpFile);
			tmpFile.close();
			if (menus.size() > 0){
				menuIndex = 0;
				tmpFile.close();
				printScreenNames(menus.front());
				printScrollArrows();
			}
			else {
				screenName = menusHistory.back();
				lcd.blink();
				lcd.home();
				if (screenName == "DATETIME"){
					captureDateTime();
					printDateTime();
					cursorX = 1;
					cursorY = 0;
				}
				if (screenName == "NEW"){
					menus.clear();
					menusHistory.clear();
					currentAlphaIndex = 0;
					currentPumpIndex = 0;
					currentRegimenIndex = 0;
					menuIndex = 0;
					cursorX = cursorY = 0;
					cropCreate();
				}
				if (screenName == "OPEN"){
					lcd.setCursor(0, 1);
					lcd.print(F("<back>  <open>"));
					lcd.home();
					cursorX = cursorY = 0;
					tmpFile = SD.open("dromatic/");
					getDirectoryMenus(tmpFile);
					menuIndex = 0;
					lcd.print(menus[menuIndex]);
					printScrollArrows();
					lcd.home();
				}
				if (screenName == "RESET"){
					lcd.clear();
					lcd.home();
					lcd.print(F(" CONFIRM RESET "));
					lcd.setCursor(0, 1);
					lcd.print(F("<back>  <reset>"));
					printScrollArrows();
				}
				if (screenName == "DELETE"){

				}
				if (screenName == "STATUS") {
					printStatus();
				}
				if (screenName == "RSVRVOL"){
					tmpFloats[0] = 0;
				}
				if (screenName == "DOSES"){
					cursorX = 1;
					cursorY = 0;
					StaticJsonBuffer<cropBufferSize> buffer;
					JsonObject& data = getCropData(buffer);
					tmpInts[0] = data["maxRegimens"];
					String totalDisplay;
					totalDisplay = (tmpInts[0] < 10) ? "0" + String(tmpInts[0]) : String(tmpInts[0]);
					lcd.print(totalDisplay + F(" REGIMEN DOSES"));
					lcd.setCursor(0, 1);
					lcd.print(F("<back>      <ok>"));
					lcd.setCursor(cursorX, cursorY);
				}
				if (screenName == "PUMPCAL"){
					tmpInts[0] = pumpCalibration;
					printPumpCalibration();
				}
				if (screenName == "PUMPDLY"){
					tmpInts[0] = pumpDelay;
					printPumpDelay();
				}
				if (screenName == "AMOUNT"){
					String amountDisplay;
					StaticJsonBuffer<regimenBufferSize> sessionBuffer;
					JsonObject& data = getRegimenData(sessionBuffer);
					tmpFloats[0] = data["ml"];
					tmpInts[0] = 1;

					StaticJsonBuffer<cropBufferSize> cropBuffer;
					JsonObject& cropData = getCropData(cropBuffer);
					tmpInts[1] = cropData["maxRegimens"];

					if (tmpFloats[0] > 1000){
						amountDisplay = F("");
					}else if (tmpFloats[0] > 100){
						amountDisplay = F("0");
					}else if (tmpFloats[0] > 10){
						amountDisplay = F("00");
					} else if (tmpFloats[0] > 0.1){
						amountDisplay = F("000");
					} else {
						amountDisplay = F("0000.00");
					}

					lcd.print(F("REGI "));
					lcd.print(tmpInts[0]);
					lcd.print(F(" "));
					lcd.print(amountDisplay);
					lcd.print(F("ml"));
					lcd.setCursor(0, 1);
					lcd.print(F("<back>    <next>"));
					cursorX = 12;
					cursorY = 0;
					lcd.setCursor(cursorX, cursorY);
				}
				if (screenName == "TPFCCNT"){
					tmpInts[0] = topOffConcentrate;
					printTopOffConcentrate();
				}
				if (screenName == "TPFAMT"){
					tmpInts[0] = topOffAmount;
					printTopOffAmount();
				}
				if (screenName == "TPFDLY"){
					tmpInts[0] = topOffDelay;
					printTopOffDelay();
				}
				if (screenName == "DRNTIME"){
					tmpInts[0] = drainTime;
					printDrainTime();
				}
				if (screenName == "FLOWCAL"){
					tmpFloats[0] = flowMeterConfig[0];
					tmpFloats[1] = flowMeterConfig[1];
					printFlowCalibration();
				}
				if (screenName == "PRIME"){
					lcd.write(byte(0));
					lcd.print(F(" TO PRIME CH0"));
					lcd.print(String(currentPumpIndex));
					lcd.setCursor(0, 1);
					lcd.print(F("          <done>"));
					cursorX = 0;
					cursorY = 0;
					lcd.setCursor(cursorX, cursorY);
				}
				if (screenName == "STARTEND"){
					StaticJsonBuffer<timerBufferSize> buffer;
					JsonObject& data = getTimerSessionData(buffer);
					JsonArray& start = data["start"].asArray();
					JsonArray& end = data["end"].asArray();

					tmpInts[0] = start[0];
					tmpInts[1] = start[1];
					tmpInts[2] = start[2];
					tmpInts[3] = end[0];
					tmpInts[4] = end[1];
					tmpInts[5] = end[2];

					String startAMPM = (tmpInts[2] == 0) ? F("AM") : F("PM");
					String endAMPM = (tmpInts[2] == 0) ? F("AM") : F("PM");

					lcd.print(String(tmpInts[0]) + F(":") + String(tmpInts[1]) + startAMPM + F("-") + String(tmpInts[3]) + F(":") + String(tmpInts[4]) + endAMPM);
					lcd.setCursor(0, 1);
					lcd.print(F("<back>      <ok>"));
					cursorX = 3;
					cursorY = 0;
					lcd.setCursor(cursorX, cursorY);
				}
				if (screenName == "RECEP01" || screenName == "RECEP02" || screenName == "RECEP03" || screenName == "RECEP04"){
					String startDisplay, endDisplay;
					String AMPM1, AMPM2;
					StaticJsonBuffer<timerSessionBufferSize> buffer;
					if (currentTimerSessionIndex < 1){ currentTimerSessionIndex = 1; }
					JsonObject& data = getTimerSessionData(buffer, currentTimerIndex, currentTimerSessionIndex);

					JsonArray& times = data["times"];

					tmpInts[0] = times[0][0]; //start hour
					tmpInts[1] = times[0][1]; //end hour

					//start
					if (cursorX == 1){
						if (tmpInts[0] > 23){
							tmpInts[0] = 0;
						}
						if (tmpInts[0] < 0) {
							tmpInts[0] = 23;
						}
					}

					if (tmpInts[0] > 12){
						startDisplay = ((tmpInts[0] - 12) < 10) ? "0" + String(tmpInts[0] - 12) : String(tmpInts[0] - 12);
					}
					else{
						if (tmpInts[0] == 0){
							startDisplay = String(12);
						}
						else{
							startDisplay = (tmpInts[0] < 10) ? "0" + String(tmpInts[0]) : String(tmpInts[0]);
						}
					}

					//END
					if (cursorX == 6){
						if (tmpInts[1] > 23){
							tmpInts[1] = 0;
						}
						if (tmpInts[1] < 0) {
							tmpInts[1] = 23;
						}
					}

					if (tmpInts[1] > 12){
						endDisplay = ((tmpInts[1] - 12) < 10) ? "0" + String(tmpInts[1] - 12) : String(tmpInts[1] - 12);
					}
					else{
						if (tmpInts[1] == 0){
							endDisplay = String(12);
						}
						else{
							endDisplay = (tmpInts[1] < 10) ? "0" + String(tmpInts[1]) : String(tmpInts[1]);
						}
					}

					AMPM1 = (tmpInts[0] > 11) ? "PM" : "AM";
					AMPM2 = (tmpInts[1] > 11) ? "PM" : "AM";

					lcd.print(startDisplay);
					lcd.print(AMPM1);
					lcd.print(F("-"));
					lcd.print(endDisplay);
					lcd.print(AMPM2);
					lcd.print(F(" Su/01"));
					lcd.setCursor(0, 1);
					lcd.print(F("<back>      <ok>"));
					cursorX = 6;
					cursorY = 0;
					lcd.setCursor(cursorX, cursorY);
				}
				if (screenName == "EC"){
					printECRange(0); //0 = first print version
				}
				if (screenName == "PH"){
					printPHRange(0); //0 = first print version
				}
				if (screenName == "ECCAL"){
					printECCalibrations("DRY");
				}
				if (screenName == "PHCAL"){
					printPHCalibrations("LOW", 4);
				}
				if (screenName == "MANFLUSH"){
					printFullFlushing();
				}
			}
			delay(350);
		}
		
		//Saves
		if (screenName == "DATETIME"){
			saveDateTime();
		}
		if (screenName == "NEW"){
			if (cursorX == 11 && cursorY == 1){
				String nameConfirm;
				for (int i = 0; i < 15; i++){
					nameConfirm = nameConfirm + nameArry[i];
				}
				if (nameConfirm == ""){
					lcd.clear();
					lcd.home();
					lcd.print(F("Sorry, No Blank"));
					lcd.setCursor(0, 1);
					lcd.print(F("Crop Names"));
					delay(3000);
					lcd.clear();
					lcd.setCursor(0, 1);
					lcd.print(F("Crop Name <done>"));
					cursorX = cursorY = 0;
					lcd.home();
				}
				else if (SD.exists("dromatic/" + nameConfirm)){
					lcd.clear();
					lcd.home();
					lcd.print(F("Sorry, Crop"));
					lcd.setCursor(0, 1);
					lcd.print(F("Already Exists"));
					delay(3000);
					lcd.clear();
					lcd.print(nameConfirm);
					lcd.setCursor(0, 1);
					lcd.print(F("Crop Name <done>"));
					cursorX = cursorY = 0;
					lcd.home();
				}
				else {
					cropBuild();
				}
			}
		}
		if (screenName == "OPEN"){
			if (cursorX == 9 && cursorY == 1){
				if (menus[menuIndex] != cropName){
					cropChange();
				}
				else{
					exitScreen();
				}
			}
			if (cursorX == 1 && cursorY == 1){
				exitScreen();
			}
		}
		if (screenName == "RESET"){
			if (cursorX == 13 && cursorY == 1){
				lcd.clear();
				lcd.print(F("RESETTING"));
				lcd.setCursor(1, 0);
				lcd.print(F("  PLEASE HOLD  "));

				for (byte i = 1; i < 8; i++){
					StaticJsonBuffer<pumpBufferSize> pumpBuffer;
					JsonObject& pumpData = getPumpData(pumpBuffer, i);
					byte totalSessions = pumpData["totalSessions"];

					for (byte j = 1; j < totalSessions; j++){
						StaticJsonBuffer<regimenBufferSize> regimenBuffer;
						JsonObject& regimenData = getRegimenData(regimenBuffer, i, j);
						JsonArray& regimenDate = regimenData["date"].asArray();
						regimenData["repeated"] = 0;
						regimenData["expired"] = false;
						regimenDate[0] = rtc.getTime().year;
					}
				}
			}
			if (cursorX == 1 || cursorX == 9 && cursorY == 1){
				tmpInts[1] = tmpInts[0] = 0;
				exitScreen();
			}
		}
		if (screenName == "DELETE") {}
		if (screenName == "STATUS") {
			saveStatus();
		}
		if (screenName == "RSVRVOL") {
			saveReservoirVolume();
		}
		if (screenName == "DOSES"){
			if (cursorX == 13 && cursorY == 1){
				lcd.clear();
				lcd.home();
				StaticJsonBuffer<cropBufferSize> buffer;
				JsonObject& data = getCropData(buffer);

				if (data["maxRegimens"] > tmpInts[0]){ //we are trimming sessions
					lcd.print(F("TRIM REGIMENS"));
					lcd.setCursor(0, 1);
					lcd.print(F(" PLEASE HOLD... "));
					trimRegimens(data["maxRegimens"], tmpInts[0]);
				}
				else if (data["maxRegimens"] < tmpInts[0]){ //we are adding sessions
					lcd.print(F("ADDING REGIMENS"));
					lcd.setCursor(0, 1);
					lcd.print(F(" PLEASE HOLD... "));
					addRegimens(data["maxRegimens"], tmpInts[0]);
				}
				data["maxRegimens"] = tmpInts[0]; //update pump's session total
				maxRegimens = tmpInts[0];
				setCropData(data, false);
			}
			if (cursorX == 1 && cursorY == 1 || cursorX == 13 && cursorY == 1){
				tmpInts[0] = 0;
				exitScreen();
			}
		}
		if (screenName == "PUMPCAL"){
			savePumpCalibration();
		}
		if (screenName == "PUMPDLY"){
			savePumpDelay();
		}
		if (screenName == "AMOUNT"){
			saveRegimenAmount();
		}
		if (screenName == "TPFCCNT"){
			saveTopOffConcentrate();
		}
		if (screenName == "TPFAMT"){
			saveTopOffAmount();
		}
		if (screenName == "TPFDLY") {
			saveTopOffDelay();
		}
		if (screenName == "FLOWCAL"){
			saveFlowCalibration();
		}
		if (screenName == "DRNTIME") {
			saveDrainTime();
		}
		if (screenName == "STARTEND"){
			saveStartEnd();
		}
		if (screenName == "RECEP01" || screenName == "RECEP02" || screenName == "RECEP03" || screenName == "RECEP04") {
			if (cursorX == 13 && cursorY == 1){
				lcd.clear();
				lcd.home();
				StaticJsonBuffer<timerSessionBufferSize> saveBuffer;
				JsonObject& saveData = getTimerSessionData(saveBuffer, currentTimerIndex, currentTimerSessionIndex);

				saveData["times"].asArray()[currentTimerSessionDayIndex].asArray()[0] = tmpInts[0]; //start hour
				saveData["times"].asArray()[currentTimerSessionDayIndex].asArray()[1] = tmpInts[1]; //end hour
				setTimerSessionData(saveData);
				checkRecepticals();
			}
			if (cursorX == 1 || cursorX == 13 && cursorY == 1){
				tmpInts[0] = tmpInts[1] = tmpInts[2] = tmpInts[3] = 0;
				exitScreen();
			}
		}
		if (screenName == "PRIME"){
			if (cursorX == 11 && cursorY == 1){
				exitScreen();
			}
		}
		if (screenName == "EC"){
			saveECData();
		}
		if (screenName == "PH"){
			savePHData();
		}
		if (screenName == "PHCAL"){
			if (cursorX == 1 && cursorY == 1){ //back
				exitScreen();
			}
			if (cursorX == 11 && cursorY == 1){ //forward
				setPHWaterProbeCalibration(112, 4.0, 'low'); //ph probe 1
				setPHWaterProbeCalibration(114, 4.0, 'low'); //ph probe 2
				printPHCalibrations("MID", 7);
				screenName = "PHCALMID";
			}
		}
		if (screenName == "PHCALMID"){
			if (cursorX == 1 && cursorY == 1){ //back
				printPHCalibrations("LOW", 4);
				screenName = "PHCALLOW";
			}
			if (cursorX == 11 && cursorY == 1){ //forward
				setPHWaterProbeCalibration(112, 7.0, 'mid'); //ph probe 1
				setPHWaterProbeCalibration(114, 7.0, 'mid'); //ph probe 2
				printPHCalibrations("HI", 10);
				screenName = "PHCALHI";
			}
		}
		if (screenName == "PHCALHI"){
			if (cursorX == 1 && cursorY == 1){ //back
				printPHCalibrations("MID", 7);
				screenName = "PHCALMID";
			}
			if (cursorX == 11 && cursorY == 1){ //forward
				setPHWaterProbeCalibration(112, 10.0, 'high'); //ph probe 1
				setPHWaterProbeCalibration(114, 10.0, 'high'); //ph probe 2
				lcd.clear();
				lcd.home();
				lcd.print(F("PH CALIBRATION"));
				lcd.setCursor(0, 1);
				lcd.print(F("NOW FINISHED!"));
				delay(5000);
				exitScreen();
			}
		}
		if (screenName == "ECCAL"){
			if (cursorX == 11 && cursorY == 1){ //moving forward
				setECWaterProbeCalibration(111, tmpIntsToInt(5), 'dry'); //ec probe 1
				setECWaterProbeCalibration(113, tmpIntsToInt(5), 'dry'); //ec probe 2
				printECCalibrations("LOW");
				screenName = "ECCALLOW";
			}
			if ((cursorX == 1 && cursorY == 1)){ //cancel calibration
				exitScreen();
			}
		}
		if (screenName == "ECCALLOW"){
			if (cursorX == 11 && cursorY == 1){ //going forward
				setECWaterProbeCalibration(111, tmpIntsToInt(5), 'low'); //ec probe 1
				setECWaterProbeCalibration(113, tmpIntsToInt(5), 'low'); //ec probe 2
				printECCalibrations("HIGH");
				screenName = "ECCALHI";
			}
			if ((cursorX == 1 && cursorY == 1)){ //going back
				printECCalibrations("DRY");
				screenName = "ECCAL";
			}
		}
		if (screenName == "ECCALHI"){
			if (cursorX == 11 && cursorY == 1){ //going forward
				tmpInts[0] = tmpInts[1] = tmpInts[2] = tmpInts[3] = tmpInts[4] = tmpInts[5] = 0; //reset tmpInts.
				setECWaterProbeCalibration(111, tmpIntsToInt(5), 'high'); //ec probe 1
				setECWaterProbeCalibration(113, tmpIntsToInt(5), 'high'); //ec probe 2
				lcd.clear();
				lcd.home();
				lcd.print(F("EC CALIBRATION"));
				lcd.setCursor(0, 1);
				lcd.print(F("NOW FINISHED!"));
				delay(5000);
				exitScreen();
			}
			if ((cursorX == 1 && cursorY == 1)){ //going back
				printECCalibrations("LOW");
				screenName = "ECCALLOW";
			}
		}
		if (screenName == "MANFLUSH"){
			if (cursorX == 1 && cursorY == 1){
				RelayToggle(11, true);
				RelayToggle(12, false);
			}
			if (cursorX == 5 && cursorY == 1){
				RelayToggle(11, false);
				RelayToggle(12, true);
			}
			if (cursorX == 11 && cursorY == 1){
				RelayToggle(11, false);
				RelayToggle(12, false);
				exitScreen();
			}
		}
	}
}
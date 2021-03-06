/*
 Arduino_LCD_Menu Library
Copyright Dustin Andrews, David Andrews 2012
Licensed under the follwing license:

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the distribution.
The name of the author may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *   Pinouts:
                                      +-----+
         +----[PWR]-------------------| USB |--+
         |                            +-----+  |
         |         GND/RST2  [ ][ ]            |
         |       MOSI2/SCK2  [ ][ ]  A5/SCL[ ] |   C5
         |          5V/MISO2 [ ][ ]  A4/SDA[ ] |   C4
         |                             AREF[ ] |
         |                              GND[ ] |
         | [ ]N/C                    SCK/13[ ] |  PB5
         | [ ]IOREF                 MISO/12[ ] |   .
         | [ ]RST                   MOSI/11[ ]~|   .
         | [ ]3V3    +---+               10[ ]~|   .
         | [ ]5v    -| A |-               9[ ]~|   .
         | [ ]GND   -| R |-               8[ ] |  PB0
         | [ ]GND   -| D |-                    |
         | [ ]Vin   -| U |-               7[ ] |  PD7
         |          -| I |-               6[ ]~|   .
     PC0 | [ ]A0    -| N |-               5[ ]~|   .
      .  | [ ]A1    -| O |-               4[ ] |   .
      .  | [ ]A2     +---+           INT1/3[ ]~|   .
      .  | [ ]A3                     INT0/2[ ] |   .
      .  | [ ]A4/SDA  RST SCK MISO     TX>1[ ] |   .
     PC5 | [ ]A5/SCL  [ ] [ ] [ ]      RX<0[ ] |  PD0
         |            [ ] [ ] [ ]              |
         |  UNO_R3    GND MOSI 5V  ____________/
          \_______________________/

		  http://busyducks.com/ascii-art-arduinos
 *
 */


#include <LiquidCrystal.h>
#include "MenuEntry.h"
#include "MenuLCD.h"
#include "MenuManager.h"

//This example is a Stopwatch and Timer.  Although it is mostly functional, it might not be the best
// user interface.  The layout was created more to provide examples of a stopwatch/timer.

const int Led1 =  13;         //przypisanie aliasów do pinów portów
const int Led2 =  12;
const int Led3 =  11;
const int Led4 =  10;
const int SW1  =  3;
const int SW2  =  2;
const int SW3  =  1;
const int SW4  =  0;
const int Buzzer =  A5;

//Edit your particular hardware setup here - See LiquidCrystal documentation for details
const int LCDD7 = 7;
const int LCDD6 = 6;
const int LCDD5 = 5;
const int LCDD4 = 4;
const int LCDE  = 9;
const int LCDRS = 8;
//LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
//Now create the MenuLCD and MenuManager classes.
MenuLCD g_menuLCD( LCDRS, LCDE, LCDD4, LCDD5, LCDD6, LCDD7, 16, 2);
MenuManager g_menuManager( &g_menuLCD);//pass the g_menuLCD object to the g_menuManager with the & operator.

//Global variables used by the sample application
//when the display is showing user results (e.g. time elapsed), the next "select" should send it back into the menu.
unsigned int g_isDisplaying = false;
int g_timerTime = 23;
long g_timerRunning = false;
long g_timerTarget = 0;
long g_autoReset = false;
long g_stopMillis = 0;
long g_startMillis = 0;

byte g_smiley[8] = {
		B00000,
		B10001,
		B00000,
		B00000,
		B10001,
		B01110,
		B00000,
};

byte g_frown[8] = {
		B00000,
		B10001,
		B00000,
		B00000,
		B00000,
		B01110,
		B10001,
};


//end Global variables section

//setupMenus
//This function is called during setup to populate the menu with the tree of nodes
//This can be a bit brain-bending to write.  If you draw a tree you want for your menu first
// this code can be a little easier to write.  Add the nodes in a depth-first order for
// the easiest code and least amount of temporary variables.
// http://en.wikipedia.org/wiki/Depth-first
// The MenuManager code is the same for building the menu as for selecting it via inputs.
// You create the menu entries and then move about the menu structure using Up, Down, Select as if you
// were selecting them via the inputs, and then use either AddChild or Add sibling from the selected node
// to create your menu.
//
//  This sample code is a simple stopwatch.  Our menu will look like this:
//  Stopwatch
//  |-Start
//  |-Stop
//  |-Reset
//  Timer
//  |-Set Time
//  |-AutoReset
//  | |-On
//  | |-Off
//  |-Start
//  |-Stop
//  Credits


void setupMenus()
{
	g_menuLCD.MenuLCDSetup();
	//Add nodes via a depth-first traversal order
	MenuEntry * p_menuEntryRoot;
	//Add root node
	//MenuEntry( char * menuText, void * userData/*=0*/, MENU_ACTION_CALLBACK_FUNC func);
	p_menuEntryRoot = new MenuEntry("Stopwatch", NULL, NULL);
	g_menuManager.addMenuRoot( p_menuEntryRoot );
	g_menuManager.addChild( new MenuEntry("Stopwatch Start", NULL, WatchStartCallback) );
	g_menuManager.addChild( new MenuEntry("Stopwatch Stop", NULL, WatchStopCallback ) );
	g_menuManager.addChild( new MenuEntry("Reset", NULL, WatchResetCallback) );
	g_menuManager.addChild( new MenuEntry("Back", (void *) &g_menuManager, MenuEntry_BackCallbackFunc) );

	g_menuManager.addSibling( new MenuEntry("Timer", NULL, NULL ) );
	//Now we want to select the "Timer" entry so we can add children under that node
	//"Timer" is one down from "Stopwatch", so we issue the down command
	g_menuManager.MenuDown();
	g_menuManager.addChild( new MenuEntry("Set Time", NULL, SetTimeCallback ) );
	//now move down to the "Time" node to add children by selecting the "Timer" node
	g_menuManager.MenuSelect();
	//Add "time"'s sibling "AutoReset" and select it
	g_menuManager.addSibling( new MenuEntry( "AutoReset", NULL, NULL) );
	g_menuManager.MenuDown();

	//Add "AutoReset"'s children
	//Use the built-in BOOL setting callbacks from MenuEntry.h: MenuEntry_Bool*CallbackFunc
	g_menuManager.addChild( new MenuEntry( "Turn Reset On",  (void *) (&g_autoReset), MenuEntry_BoolTrueCallbackFunc ) );
	g_menuManager.addChild( new MenuEntry( "Turn Reset Off", (void *) (&g_autoReset), MenuEntry_BoolFalseCallbackFunc ) );
	g_menuManager.addChild( new MenuEntry("Back", (void *) &g_menuManager, MenuEntry_BackCallbackFunc) );

	//Add timer start and stop
	g_menuManager.addSibling( new MenuEntry( "Countdown Start", NULL, TimerStartCallback) );
	g_menuManager.addSibling( new MenuEntry( "Countdown Stop", NULL, TimerStopCallback) );
	g_menuManager.addSibling( new MenuEntry("Back", (void *) &g_menuManager, MenuEntry_BackCallbackFunc) );

	//Get the selection state back to the root for startup and to add the last entry
	g_menuManager.SelectRoot();
	g_menuManager.addSibling( new MenuEntry( "Credits", NULL, CreditsCallback) );
	//Make sure the menu is drawn correctly after all changes are done
	g_menuManager.DrawMenu();

	g_menuManager.addSibling( new MenuEntry( "Draw Smiley", NULL, SmileyCallback) );

	g_menuLCD.getLCD()->createChar( 0, g_smiley );
	g_menuLCD.getLCD()->createChar( 1, g_frown );
}


//This is a sample callback funtion for when a menu item with no children (aka command) is selected
void WatchStartCallback( char* pMenuText, void *pUserData )
{
	g_startMillis = millis();
	char *pTextLines[2] = {"Clock Started", "" };
	g_menuLCD.PrintMenu( pTextLines, 2, 3 );
	g_isDisplaying = true;
}


//This is a sample callback funtion for when a menu item with no children (aka command) is selected
void WatchStopCallback( char* pMenuText, void *pUserData )
{
	g_stopMillis = millis();

	char strSeconds[50];
	dtostrf( ((float)(g_stopMillis-g_startMillis))/1000, 1, 2, strSeconds );
	char *pTextLines[2] = {"Elapsed Time", strSeconds };
	g_menuLCD.PrintMenu( pTextLines, 2, 3 );
	g_isDisplaying = true;
}

//This is a sample callback funtion for when a menu item with no children (aka command) is selected
void WatchResetCallback( char* pMenuText, void *pUserData )
{
	g_startMillis = 0;
	g_stopMillis = 0;
	char *pTextLines[2] = {"Clock reset", "" };
	g_menuLCD.PrintMenu( pTextLines, 2, 3 );
}

//This callback uses the built-in Int Input routine in MenuManager.h to request input of a integer number from the
//user.  Control will pass to the DoIntInput function until the user finishes.  the g_timerTime will be set to the
//value the user selects.
void SetTimeCallback( char* pMenuText, void *pUserData )
{
	char *pLabel = "Timer seconds";
	int iNumLabelLines = 1;
	int iMin = 1;
	int iMax = 1000;
	int iStart = 60;
	//Each user input action (such as a turn of rotary enocoder or push of button
	//will step this amount
	int iStep = 5;

	g_menuManager.DoIntInput( iMin, iMax, iStart, iStep, &pLabel, iNumLabelLines, &g_timerTime );
	Serial.print("Timer time" );
	Serial.println( g_timerTime );
}
//This is a sample callback funtion for when a menu item with no children (aka command) is selected
void TimerStartCallback( char* pMenuText, void *pUserData )
{
	g_timerTarget = millis() + (g_timerTime * 1000);//This is buggy- doesn't handle wrap-around of the millis output.  Too bad :(
	Serial.print( "timer target = ");
	Serial.println( g_timerTarget );
	Serial.print( "time = " );
	Serial.println( millis());
	g_timerRunning = true;
	char strSeconds[50];
	itoa( g_timerTime, strSeconds, 10 );
	char *pTextLines[2] = {"Go!", strSeconds };
	g_menuLCD.PrintMenu( pTextLines, 2, 3 );
	g_isDisplaying = true;
}


//This is a sample callback funtion for when a menu item with no children (aka command) is selected
void TimerStopCallback( char* pMenuText, void *pUserData )
{
	g_timerRunning = false;
}

void CreditsCallback( char* pMenuText, void *pUserData )
{
	char *pTextLines[2] = {"David Andrews ", "Dustin Andrews" };
	g_menuLCD.PrintMenu( pTextLines, 2, 1 );
	delay(5000);
	char *pTextLines2[2] = {"http://authenti","cinvention.com"};
	g_menuLCD.PrintMenu( pTextLines2, 2, 1 );
	g_isDisplaying = true;
}

void SmileyCallback( char* pMenuText, void *pUserData )
{
	for( int i = 0; i < 10 ; ++i )
	{
		g_menuLCD.ClearLCD();
		g_menuLCD.getLCD()->setCursor( 8,0 );
		g_menuLCD.getLCD()->print( (char)0 );
		delay(500);
		g_menuLCD.ClearLCD();
		g_menuLCD.getLCD()->setCursor( 8,0 );
		g_menuLCD.getLCD()->print( (char)1 );
		delay(500);
	}
}



void setup()
{
	pinMode(Led1, OUTPUT);    //Konfiguracja linii do ktorych sa dolaczone diody jako wyjscia
	pinMode(Led2, OUTPUT);
	pinMode(Led3, OUTPUT);
	pinMode(Led4, OUTPUT);
	pinMode(Buzzer, OUTPUT);    //konfiguracja linii do ktorej jest dolaczony brzeczyk jako wyjscie
	pinMode(SW1, INPUT);        //konfiguracja linii do ktorych sa dolaczone przyciski jako wejscia
	pinMode(SW2, INPUT);
	pinMode(SW3, INPUT);
	pinMode(SW4, INPUT);
	digitalWrite(SW1, HIGH);   //dolaczenie do linii do ktorych sa dolaczone przyciski rezystorow podciagajacych co wymusi na nich domyslnie stan wysoki
	digitalWrite(SW2, HIGH);
	digitalWrite(SW3, HIGH);
	digitalWrite(SW4, HIGH);
	digitalWrite(Led1, HIGH);  //domyslne wylaczenie diod LED
	digitalWrite(Led2, HIGH);
	digitalWrite(Led3, HIGH);
	digitalWrite(Led4, HIGH);
	digitalWrite(Buzzer, HIGH);  //domyslne wylaczenie brzeczyka
	Serial.begin(115200);
	Serial.print("Ready.");
	setupMenus();
}

char incomingByte = 'r';
int SW1value = 0;
int SW2value = 0;
int SW3value = 0;
int SW4value = 0;

void readSW4()
{
	Serial.println(incomingByte);
	SW1value = digitalRead(SW1);
	SW2value = digitalRead(SW2);
	SW3value = digitalRead(SW3);
	SW4value = digitalRead(SW4);
	Serial.print(SW1value);
	Serial.print(SW2value);
	Serial.print(SW3value);
	Serial.println(SW4value);
	//TU COŚ SENSOWNEGO DODAJEMY, ABY UZYSKAĆ STEROWANIE GORA I DÓŁ PO MENU
	incomingByte = 's';
	SW1value = 1;
	SW2value = 1;
	SW3value = 1;
	SW4value = 1;
}

void readSW3()
{
	Serial.println(incomingByte);
	SW1value = digitalRead(SW1);
	SW2value = digitalRead(SW2);
	SW3value = digitalRead(SW3);
	SW4value = digitalRead(SW4);
	Serial.print(SW1value);
	Serial.print(SW2value);
	Serial.print(SW3value);
	Serial.println(SW4value);
	//TU COŚ SENSOWNEGO DODAJEMY, ABY UZYSKAĆ STEROWANIE GORA I DÓŁ PO MENU
	incomingByte = 'b';
	SW1value = 1;
	SW2value = 1;
	SW3value = 1;
	SW4value = 1;
}


void loop()
{
	//The example shows using bytes on the serial port to move the menu. You can hook up your buttons or other controls.
	if (Serial.available() > 0)
	{
		incomingByte = Serial.read();
	}
	//Serial.println(incomingByte);
	SW1value = 1;
	SW2value = 1;
	SW3value = 1;
	SW4value = 1;
	attachInterrupt(SW3, readSW3, FALLING);
	attachInterrupt(SW4, readSW4, FALLING);

	/*
	SW4value = digitalRead(SW4);
	if (SW4value == LOW) {         //Sprawdzenie czy nacisniety przycisk S4
		incomingByte = 'd';
	}
	else
		incomingByte = 'b';
	if(SW4value == LOW)
	{
		Serial.println(SW4value);
		Serial.println(incomingByte);
	}

	int key_val = 256;
	if (incomingByte = 'e')
	{
		key_val = analogRead(A0);
		if(key_val >= 127 && key_val <= 255)
			incomingByte = 'd';
		else
			incomingByte = 'u';
	 }

	 */

	switch( incomingByte )
	{
	case 'u':
		g_menuManager.DoMenuAction( MENU_ACTION_UP );
		break;
	case 'd':
		g_menuManager.DoMenuAction( MENU_ACTION_DOWN );
		break;
	case 's':
		if( g_isDisplaying )
		{
			g_isDisplaying = false;
			g_menuManager.DrawMenu();
		}
		else
		{
			g_menuManager.DoMenuAction( MENU_ACTION_SELECT );
		}
		break;
	case 'b':
		g_menuManager.DoMenuAction( MENU_ACTION_BACK );
		break;
	default:
		break;
	}
	incomingByte = 'e';
	if( g_timerRunning && g_timerTarget < millis())
	{
		long time = millis();

		Serial.print( "time = " );
		Serial.println( time );

		Serial.println("Timer Goes Off HERE!");
		if( g_autoReset)
		{
			Serial.print( "timer target was ");
			Serial.println( g_timerTarget );
			g_timerTarget = time + ( g_timerTime *1000 );
			Serial.print( "timer target is now ");
			Serial.println( g_timerTarget );
			Serial.print( "timerTime is " );
			Serial.println( g_timerTime );
			Serial.println( "--------" );
		}
		else
		{
			g_timerRunning = false;
		}
	}
}

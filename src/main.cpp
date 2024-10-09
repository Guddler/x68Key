// Sharp X68000 to USB keyboard and mouse converter
// by Zuofu
//
// Supports true USB protocol (even through hub), so all types of USB keyboards and mice may be used.
// Combo devices (e.g. keyboard/mice combos with a single wireless receiver) are supported.
// Note that the X68000 keyboard has many more keys than a standard US 104 key, so feel free to adjust keymap
// to suit your needs (e.g. I don't know if software uses XF1-6 keys in a way that their shifted mapping is awkward
//
// Provided free of charge and without warranty by Zuofu, you may use this code
// for either non-commercial or commercial purposes (e.g. sell on eBay), but please
// give credit.
//
// Requirements
// -Arduino Mini Pro (3.3V/8MHz): https://smile.amazon.com/dp/B004RF9LB8 (or equivalent)
// -Mini USB Host: https://smile.amazon.com/dp/B01EWW9R1E (or equivalent)
//  note: above USB host board should have USB VBUS->3.3V connection removed and replaced with a connection from VBUS to RAW for full 5V USB compatibility
// -Mini DIN-7 connector: https://smile.amazon.com/dp/B00PZSCGPY
// -Some way to program above Arduino (typically a USB->Serial adapter)
// -Add USB Host Shield 2.0 library to Arduino (Tools->Manage Libraries...)
//
//  SHARP X68000 PIN CONNECTIONS:
//  Mini-DIN            Mini Pro
//  -----------------------------
//  pin1  +5V          	RAW
//  pin2  MOUSE        	D5
//  pin3  RXD          	D1 (TXO)
//  pin4  TXD          	D0 (RXI) - note this is technically over-voltage, can put a diode/resistor in series
//  pin5  READY        	N/C
//  pin6  REMOTE      	N/C
//  pin7  GND          	GND
//
//  References:
//  https://ixsvr.dyndns.org/ps2ms68k
//  https://geekhack.org/index.php?topic=29060.0
//  Sharp X68000 Technical Databook (Blue book)

//  Special keys default mapping:
//  {FULL WIDTH} 	-> RIGHT ALT
//  {HIRAGANA}  	-> LEFT ALT
//  {KANA}      	-> PRINT SCREEN
//  {ROMANJI}    	-> SCROLL LOCK
//  {CODE ENTRY} 	-> PAUSE/BREAK
//  BREAK        	-> F11
//  COPY        	-> F12
//  UNDO        	-> END
//  CLR          	-> NUM LOCK
//  OPT1        	-> MENU
//  OPT2        	-> RIGHT CTRL
//  CTRL        	-> LEFT CTRL

//  The following requires holding down Left 'Windows/GUI'
//  {SYMBOL INPUT}	-> WIN + NUM /
//  {REGISTER}    	-> WIN + NUM *
//  HELP          	-> WIN + NUM -
//  XFn            	-> WIN + Fn

// Updated version by M White, October 2024. Please see main README for details of
// all the changes from the original code. NB: Please don't pay too much attention
// to the auto-repeat code, it could probably be much better, but it got the job done!

/* FTDI
	CTS and DTS are not present on all FTDI boards but if they are present, you'll likely need to connect them as below
	GND	-> GND
	CTS	-> GND
	VCC -> VCC (3.3) can go on the pin between the Arduino and the USB host if you've soldered to the programming vcc pin
	TX 	-> RX
	RX	-> TX
	DTS	-> DTS
*/

#include <hidboot.h>
#include <usbhub.h>
#include <SoftwareSerial.h>

#include <SPI.h>

// Uncomment only one layout - compile will fail with more than one
#include "layout_uk.h"
// #include "layout_uk_mac.h"
// #include "layout_us.h"

// Adjust this accordingly to your mouse DPI if you have issues, 4 is a good number for 300/600 modern mice, may need to increase
// if using a high-DPI 'gamer' mouse
#define MOUSE_DIVIDER 0x03

// This will likely break keyboard functionality but give a small insight into the data received from the X60000
// WARNING: Who knows what commands the system will think it's receiving. No promises you won't lose disk data!
// #define DEBUG

// NB: This is temporary while I work on issues with auto-repeat
//#define REPEAT_ENABLED 

#define PRESS & ~0x80
#define RELEASE | 0x80
bool SHIFTED = false;
bool KEY_ENABLE = false;

#ifdef REPEAT_ENABLED
// Auto-repeat related stuff
uint16_t repeatDelay = 500;		// Period before we repear
uint16_t repeatInterval = 110;	// Delay between repeats
uint32_t currentMillis = 0;
uint32_t previousMillis = 0;
uint32_t delayMillis = 0;
uint8_t lastKey = 0;			// Track the last key pressed since we are going to fake key presses
bool initialDelay = true;		// Know if we are obeying the initial delay or the shorter repeat delay
bool repeatTriggered = false; 	// As we're calling our own callbacks, track if it was called, or triggered
#endif

SoftwareSerial x68kSerMouse(4, 5); // RX, TX (RX unused)

uint8_t rxbyte, last_rxbyte;
uint8_t mouse_left, mouse_right;
uint8_t leftGUI = 0;
int16_t mouse_dx, mouse_dy;

class MouseRptParser : public MouseReportParser
{
protected:
	void OnMouseMove(MOUSEINFO *mi);
	void OnLeftButtonUp(MOUSEINFO *mi);
	void OnLeftButtonDown(MOUSEINFO *mi);
	void OnRightButtonUp(MOUSEINFO *mi);
	void OnRightButtonDown(MOUSEINFO *mi);
	void OnMiddleButtonUp(MOUSEINFO *mi);
	void OnMiddleButtonDown(MOUSEINFO *mi);
};
void MouseRptParser::OnMouseMove(MOUSEINFO *mi)
{
	mouse_dx += (mi->dX) / MOUSE_DIVIDER;
	mouse_dy += (mi->dY) / MOUSE_DIVIDER;
};
void MouseRptParser::OnLeftButtonUp(MOUSEINFO *mi)
{
	mouse_left = 0;
};
void MouseRptParser::OnLeftButtonDown(MOUSEINFO *mi)
{
	mouse_left = 1;
};
void MouseRptParser::OnRightButtonUp(MOUSEINFO *mi)
{
	mouse_right = 0;
};
void MouseRptParser::OnRightButtonDown(MOUSEINFO *mi)
{
	mouse_right = 1;
};
void MouseRptParser::OnMiddleButtonUp(MOUSEINFO *mi) {
	// Middle mouse unused
};
void MouseRptParser::OnMiddleButtonDown(MOUSEINFO *mi) {
	// Middle mouse unused
};

class KbdRptParser : public KeyboardReportParser
{
	public:
		void OnKeyDown(uint8_t mod, uint8_t key);
		void OnKeyUp(uint8_t mod, uint8_t key);

	protected:
		void OnControlKeysChanged(uint8_t before, uint8_t after);
		void OnKeyPressed(uint8_t key);

	private:
		uint8_t doShiftOrUnshift(uint8_t *list, unsigned int size, uint8_t key);
};

// If the keycode is found in the list, return the scancode that should be sent to x68k
uint8_t KbdRptParser::doShiftOrUnshift(uint8_t *list, unsigned int size, uint8_t key)
{
	for (unsigned int i = 0; i < size; i += 2)
	{
		if (list[i] == key)
			return list[i + 1];
	}
	return 0;
}

void KbdRptParser::OnKeyDown(uint8_t mod, uint8_t key)
{
	if (leftGUI)
	{
		// if the GUI key is down, check the list of 'alternative keys'
		for (unsigned int i = 0; i < sizeof(altKeysUSB); i++)
			if (key == altKeysUSB[i])
			{ // found in list, so send keycode and break
				Serial.write(altKeyCodes[i] PRESS);
				break;
			}
	}
	else if (key < sizeof(keymapping))
	{
		if (SHIFTED)
		{
			uint8_t unshiftKey = doShiftOrUnshift(unshifted_keys, sizeof(unshifted_keys), key);
			if (unshiftKey > 0)
			{
				// These are keys that need to be sent where the shifted state is different to the state
				// of the physical keyboard. An example would be the ^. On a PC keyboard, this is shift 6
				// but on an X68000 it is unshifted (the ^~ key).
				Serial.write(SHIFT_SCAN RELEASE); // undo shift
				Serial.write(unshiftKey PRESS);
				Serial.write(SHIFT_SCAN PRESS); // reapply shift
			}
			else
			{
				if (shifted_keymapping[key] != 0)
				{
					// This is an override - if you want to send a different scan code for a shifted key than the one
					// you would natively for that key then set it to something other than 0 in the shifted_keymapping
					// array. This is an extreme example, but if when user presses <SHIFT><A> you really want to send
					// a capital B, define the keycode for B in the array slot for A. What this is actually useful for
					// is to allow us to match up the symbols on the keyboard with what is sent to the X68K. Ordinarily
					// the left bracket is <SHIFT><8> for example but on western keyboards it is on <SHIFT><9>. There
					// manhy that are different.
					Serial.write(shifted_keymapping[key] PRESS);
				}
				else
				{
					Serial.write(keymapping[key] PRESS);
				}
			}
		}
		else
		{
			uint8_t shiftKey = doShiftOrUnshift(shifted_keys, sizeof(shifted_keys), key);
			if (shiftKey > 0)
			{
				// See the note above about keys that don't match the physical shift state, except these are
				// for keys where we don't have the shift key held on the physical keyboard but we need to
				// send the scancode of a shifted key to get the right character.
				Serial.write(SHIFT_SCAN PRESS); // apply shift
				Serial.write(shiftKey PRESS);
				Serial.write(SHIFT_SCAN RELEASE); // undo shift
			}
			else
			{
				Serial.write(keymapping[key] PRESS);
			}
		}
#ifdef REPEAT_ENABLED
		lastKey = key;
		delayMillis = 0;
		previousMillis = millis();
#endif
	}
}

void KbdRptParser::OnControlKeysChanged(uint8_t before, uint8_t after)
{

	MODIFIERKEYS beforeMod;
	*((uint8_t *)&beforeMod) = before;

	MODIFIERKEYS afterMod;
	*((uint8_t *)&afterMod) = after;

	if (beforeMod.bmLeftCtrl != afterMod.bmLeftCtrl)
	{
		if (!afterMod.bmLeftCtrl)
			Serial.write(CTRL_SCAN RELEASE);
		else
			Serial.write(CTRL_SCAN PRESS);
	}
	if (beforeMod.bmLeftShift != afterMod.bmLeftShift)
	{
		if (!afterMod.bmLeftShift)
		{
			SHIFTED = false;
			Serial.write(SHIFT_SCAN RELEASE);
		}
		else
		{
			SHIFTED = true;
			Serial.write(SHIFT_SCAN PRESS);
		}
	}
	if (beforeMod.bmLeftAlt != afterMod.bmLeftAlt)
	{
		if (!afterMod.bmLeftAlt)
			Serial.write(HIRA_SCAN RELEASE);
		else
			Serial.write(HIRA_SCAN PRESS);
	}
	if (beforeMod.bmLeftGUI != afterMod.bmLeftGUI)
	{
		leftGUI = afterMod.bmLeftGUI;
	}

	if (beforeMod.bmRightCtrl != afterMod.bmRightCtrl)
	{
		if (!afterMod.bmRightCtrl)
			Serial.write(OPT2_SCAN RELEASE);
		else
			Serial.write(OPT2_SCAN PRESS);
	}
	if (beforeMod.bmRightShift != afterMod.bmRightShift)
	{
		if (!afterMod.bmRightShift)
		{
			SHIFTED = false;
			Serial.write(SHIFT_SCAN RELEASE);
		}
		else
		{
			SHIFTED = true;
			Serial.write(SHIFT_SCAN PRESS);
		}
	}
	if (beforeMod.bmRightAlt != afterMod.bmRightAlt)
	{
		if (!afterMod.bmRightAlt)
			Serial.write(WIDTH_SCAN RELEASE);
		else
			Serial.write(WIDTH_SCAN PRESS);
	}
	if (beforeMod.bmRightGUI != afterMod.bmRightGUI)
	{
		// right Windows/GUI key unused, since my keyboard doesn't have one
	}
}

void KbdRptParser::OnKeyUp(uint8_t mod, uint8_t key)
{
	// onKeyUp(mod, key);
	if (leftGUI)
	{
		// if the GUI key is down, check the list of 'alternative keys'
		for (unsigned int i = 0; i < sizeof(altKeysUSB); i++)
			if (key == altKeysUSB[i])
			{ // found in list, so send keycode and break
				Serial.write(altKeyCodes[i] RELEASE);
				break;
			}
	}
	else if (key < sizeof(keymapping))
	{
#ifdef REPEAT_ENABLED
		if (!repeatTriggered) {
			lastKey = 0;
			initialDelay = true;
		}
#endif

		// We need all the same logic here as when keys are pressed otherwise we can leave keys hanging
		// and the machine seems to get really upset. DOS doesn't seem to care too much but programs like
		// LHES and ED certainly do!
		if (SHIFTED)
		{
			uint8_t unshiftKey = doShiftOrUnshift(unshifted_keys, sizeof(unshifted_keys), key);
			if (unshiftKey > 0)
			{
				Serial.write(unshiftKey RELEASE);
			}
			else
			{
				if (shifted_keymapping[key] != 0)
				{
					Serial.write(shifted_keymapping[key] RELEASE);
				}
				else
				{
					Serial.write(keymapping[key] RELEASE);
				}
			}
		}
		else
		{
			uint8_t shiftKey = doShiftOrUnshift(shifted_keys, sizeof(shifted_keys), key);
			if (shiftKey > 0)
			{
				Serial.write(shiftKey RELEASE);
			}
			else
			{
				Serial.write(keymapping[key] RELEASE);
			}
		}
	}
}

void KbdRptParser::OnKeyPressed(uint8_t key) {
	// This callback unused
};

USB Usb;
USBHub Hub(&Usb);

HIDBoot<USB_HID_PROTOCOL_KEYBOARD | USB_HID_PROTOCOL_MOUSE> HidComposite(&Usb);
HIDBoot<USB_HID_PROTOCOL_KEYBOARD> HidKeyboard(&Usb);
HIDBoot<USB_HID_PROTOCOL_MOUSE> HidMouse(&Usb);

KbdRptParser KbdPrs;
MouseRptParser MousePrs;

void resetRepeat()
{
#ifdef REPEAT_ENABLED
	initialDelay = true;
	delayMillis = 0;
	currentMillis = 0;
	previousMillis = 0;
	lastKey = 0;
	repeatTriggered = false;
#endif
}

void setup()
{
	Serial.begin(2400);
#if !defined(__MIPSEL__)
	while (!Serial)
		; // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif
	x68kSerMouse.begin(4800);

	if (Usb.Init() == -1)
	{
		// Serial.println("OSC did not start.");
		delay(200);
	}

	HidComposite.SetReportParser(0, &KbdPrs);
	HidComposite.SetReportParser(1, &MousePrs);
	HidKeyboard.SetReportParser(0, &KbdPrs);
	HidMouse.SetReportParser(0, &MousePrs);

	resetRepeat();
}

void loop()
{
	if (KEY_ENABLE) {
#ifdef REPEAT_ENABLED 
		repeatTriggered = false;
#endif
		Usb.Task(); // Poll USB

#ifdef REPEAT_ENABLED
		if (lastKey > 0) {
			// Sort out delay / repeat
			currentMillis = millis();
			delayMillis = currentMillis - previousMillis;
#ifdef DEBUG
			Serial.print("DelayMillis: ");
			Serial.println(delayMillis);
#endif
			if ((delayMillis >= repeatDelay && initialDelay) || (delayMillis >= repeatInterval && !initialDelay))
			{
				// We don't use mod in key up/down so just fake it
				repeatTriggered = true;
				KbdPrs.OnKeyUp(0, lastKey);
				KbdPrs.OnKeyDown(0, lastKey);
				delayMillis = 0;
				initialDelay = false;
			}
		}
#endif
	}

	while (Serial.available() > 0) // Read from Keyboard serial port - to determine whether to poll mouse or not
	{
		last_rxbyte = rxbyte;
		rxbyte = Serial.read();

#ifdef DEBUG
		char msg[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#endif
		// Interpret the commands sent from the computer
		switch (rxbyte & 0xF0) {
			case 0x40:
				if (rxbyte == 0x40 && last_rxbyte == 0x41) // MSCTRL Toggle H->L
				{
					uint8_t mouse_packet[3];
					uint8_t x_ovp, x_ovn, y_ovp, y_ovn = 0;

					// detect overflow conditions
					if (mouse_dx > 127)
						x_ovp = 1;
					if (mouse_dx < -128)
						x_ovn = 1;
					if (mouse_dy > 127)
						y_ovp = 1;
					if (mouse_dy < -128)
						y_ovn = 1;

					mouse_packet[0] = (y_ovn << 7) | (y_ovp << 6) | (x_ovn << 5) | (x_ovp << 4) | (mouse_right << 1) | mouse_left;
					mouse_packet[1] = mouse_dx;
					mouse_packet[2] = mouse_dy;
					mouse_dx = 0;
					mouse_dy = 0;
					for (int i = 0; i < 3; i++)
						x68kSerMouse.write(mouse_packet[i]);
				}
				else if (rxbyte == 0x48)
				{ // Keyboard disable
					KEY_ENABLE = false;
					// TODO: What happens with keyboard repeat while the keyboard is disabled?
#ifdef DEBUG
					sprintf(msg, "Keyboard Disable");
#endif
				}
				else if (rxbyte == 0x49)
				{
					// Keyboard emable
					KEY_ENABLE = true;
#ifdef DEBUG
					sprintf(msg, "\nKeyboard Enable");
#endif
				}
				break;
			case 0x50:
				// We don't care about any of these
				break;
			case 0x60:
#ifdef REPEAT_ENABLED
				// Repeat delay - 200 + (0x0n * 100) in ms
				repeatDelay = ((rxbyte & 0x0F) * 100) + 200;
#endif
#ifdef DEBUG
						sprintf(msg, "Delay: %dms", repeatDelay);
#endif
				break;
			case 0x70:
#ifdef REPEAT_ENABLED
				// Repeat interval = 30 + (rep time squared) * 5 in ms
				repeatInterval = (((rxbyte & 0x0F) * (rxbyte & 0x0F)) * 5) + 30;
#endif
#ifdef DEBUG
					sprintf(msg, "Repeat: %dms", repeatInterval);
#endif
				break;
			case 0x80:
				// LED status - we don't care
				break;
			case 0xF0:
				// FD, DF handshake at poweron, FF at power off
				if (rxbyte == 0xFD)
				{
					resetRepeat();
#ifdef DEBUG
					sprintf(msg, "Hello keyboard");
#endif
				}
				else if (rxbyte == 0xFF)
				{
					resetRepeat();
#ifdef DEBUG
					sprintf(msg, "Bye for now");
#endif
				}
				break;
		}

#ifdef DEBUG
		if (msg[0] != 0)
			Serial.println(msg);
#endif
	}

}

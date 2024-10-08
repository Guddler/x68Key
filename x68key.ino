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
//  pin1  +5V          RAW
//  pin2  MOUSE        D5
//  pin3  RXD          D1 (TXO)
//  pin4  TXD          D0 (RXI) - note this is technically over-voltage, can put a diode/resistor in series
//  pin5  READY        N/C
//  pin6  REMOTE      N/C
//  pin7  GND          GND
//
//  References:
//  https://ixsvr.dyndns.org/ps2ms68k
//  https://geekhack.org/index.php?topic=29060.0
//  Sharp X68000 Technical Databook (Blue book)

// Special keys default mapping:
//{FULL WIDTH} -> RIGHT ALT
//{HIRAGANA}  -> LEFT ALT
//{KANA}      -> PRINT SCREEN
//{ROMANJI}    -> SCROLL LOCK
//{CODE ENTRY} -> PAUSE/BREAK
// BREAK        -> F11
// COPY        -> F12
// UNDO        -> END
// CLR          -> NUM LOCK
// OPT1        -> MENU
// OPT2        -> RIGHT CTRL
// CTRL        -> LEFT CTRL

// The following requires holding down Left 'Windows/GUI'
//{SYMBOL INPUT} -> WIN + NUM /
//{REGISTER}    -> WIN + NUM *
// HELP          -> WIN + NUM -
// XFn            -> WIN + Fn

#include <hidboot.h>
#include <usbhub.h>
#include <SoftwareSerial.h>

// Satisfy IDE, which only needs to see the include statment in the ino.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

// Adjust this accordingly to your mouse DPI if you have issues, 4 is a good number for 300/600 modern mice, may need to increase
// if using a high-DPI 'gamer' mouse
#define MOUSE_DIVIDER 0x04

#define CTRL_SCAN 0x71
#define SHIFT_SCAN 0x70
#define HIRA_SCAN 0x56
#define WIDTH_SCAN 0x60
#define OPT1_SCAN 0x72
#define OPT2_SCAN 0x73

SoftwareSerial x68kSerMouse(4, 5); // RX, TX (RX unused)

// KEY CODE TO X68K---- -0-----1-----2-----3-----4-----5-----6-----7-----8-----9-----A-----B-----C-----D-----E-----F--
uint8_t keymapping[] = {0x00, 0x00, 0x00, 0x00, 0x1E, 0x2E, 0x2C, 0x20, 0x13, 0x21, 0x22, 0x23, 0x18, 0x24, 0x25, 0x26,	 // 0x
						0x30, 0x2F, 0x19, 0x1A, 0x11, 0x14, 0x1F, 0x15, 0x17, 0x2D, 0x12, 0x2B, 0x16, 0x2A, 0x02, 0x03,	 // 1x
						0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x1D, 0x01, 0x0F, 0x10, 0x35, 0x0D, 0x0C, 0x1C,	 // 2x
						0x29, 0x0E, 0x34, 0x27, 0x28, 0x5F, 0x31, 0x32, 0x33, 0x5D, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,	 // 3x
						0x69, 0x6A, 0x6B, 0x6C, 0x61, 0x62, 0x5A, 0x5B, 0x5C, 0x5E, 0x36, 0x38, 0x37, 0x3A, 0x39, 0x3D,	 // 4x
						0x3B, 0x3E, 0x3C, 0x3F, 0x40, 0x41, 0x42, 0x46, 0x4E, 0x4B, 0x4C, 0x4D, 0x47, 0x48, 0x49, 0x43,	 // 5x
						0x44, 0x45, 0x4F, 0x51, 0x0E, 0x72, 0x4A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // 6x

uint8_t
shifted_keymapping[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	 // 0x
						0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 1x
						0x00, 0x00, 0x00, 0x00, 0x07, 0x28, 0x09, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 2x
						0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 3x
						0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 4x
						0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 5x
						0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // 6x

// USB keycodes for 'alternative keys' (holding down left-GUI) and their corresponding X68000 mappings
// Note that arrays must be the same size
uint8_t altKeysUSB[] = {0x54, 0x55, 0x56, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E};
uint8_t altKeyCodes[] = {0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59};

uint8_t rxbyte, last_rxbyte;
uint8_t mouse_left, mouse_right;
uint8_t leftGUI = 0;
int16_t mouse_dx, mouse_dy;

bool _SHIFTED = false;

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
protected:
	void OnControlKeysChanged(uint8_t before, uint8_t after);
	void OnKeyDown(uint8_t mod, uint8_t key);
	void OnKeyUp(uint8_t mod, uint8_t key);
	void OnKeyPressed(uint8_t key);
};

void KbdRptParser::OnKeyDown(uint8_t mod, uint8_t key)
{
	if (leftGUI)
	{
		// if the GUI key is down, check the list of 'alternative keys'
		for (int i = 0; i < sizeof(altKeysUSB); i++)
			if (key == altKeysUSB[i])
			{ // found in list, so send keycode and break
				Serial.write(altKeyCodes[i] & ~0x80);
				break;
			}
	}
	else if (key < sizeof(keymapping)) 
	{
		// This is an override - if you want to send a different scan code for a shifted key than the one
		// you would natively for that key then set it to something other than 0 in the shifted_keymapping
		// array. This is an extreme example, but if when user presses <SHIFT><A> you really want to send
		// a capital B, define the keycode for B in the array slot for A. What this is actually useful for
		// is to allow us to match up the symbols on the keyboard with what is sent to the X68K. Ordinarily
		// the left bracket is <SHIFT><8> for example but on western keyboards it is on <SHIFT><9>. There
		// manhy that are different.
		if (_SHIFTED && shifted_keymapping[key] != 0)
			Serial.write(shifted_keymapping[key] & ~0x80);
		else
			Serial.write(keymapping[key] & ~0x80);
	}
}

void KbdRptParser::OnControlKeysChanged(uint8_t before, uint8_t after)
{

	MODIFIERKEYS beforeMod;
	*((uint8_t *)&beforeMod) = before;

	MODIFIERKEYS afterMod;
	*((uint8_t *)&afterMod) = after;

	// & = Pressed
	// | = Released

	if (beforeMod.bmLeftCtrl != afterMod.bmLeftCtrl)
	{
		if (!afterMod.bmLeftCtrl)
			Serial.write(CTRL_SCAN | 0x80);
		else
			Serial.write(CTRL_SCAN & ~0x80);
	}
	if (beforeMod.bmLeftShift != afterMod.bmLeftShift)
	{
		if (!afterMod.bmLeftShift)
    {
			_SHIFTED = false;
			Serial.write(SHIFT_SCAN | 0x80);
    }
		else
    {
			_SHIFTED = true;
			Serial.write(SHIFT_SCAN & ~0x80);
    }
	}
	if (beforeMod.bmLeftAlt != afterMod.bmLeftAlt)
	{
		if (!afterMod.bmLeftAlt)
			Serial.write(HIRA_SCAN | 0x80);
		else
			Serial.write(HIRA_SCAN & ~0x80);
	}
	if (beforeMod.bmLeftGUI != afterMod.bmLeftGUI)
	{
		leftGUI = afterMod.bmLeftGUI;
	}

	if (beforeMod.bmRightCtrl != afterMod.bmRightCtrl)
	{
		if (!afterMod.bmRightCtrl)
			Serial.write(OPT2_SCAN | 0x80);
		else
			Serial.write(OPT2_SCAN & ~0x80);
	}
	if (beforeMod.bmRightShift != afterMod.bmRightShift)
	{
		if (!afterMod.bmRightShift)
    {
			_SHIFTED = false;
			Serial.write(SHIFT_SCAN | 0x80);
    }
		else
    {
			_SHIFTED = true;
			Serial.write(SHIFT_SCAN & ~0x80);
    }
	}
	if (beforeMod.bmRightAlt != afterMod.bmRightAlt)
	{
		if (!afterMod.bmRightAlt)
			Serial.write(WIDTH_SCAN | 0x80);
		else
			Serial.write(WIDTH_SCAN & ~0x80);
	}
	if (beforeMod.bmRightGUI != afterMod.bmRightGUI)
	{
		// right Windows/GUI key unused, since my keyboard doesn't have one
	}
}

void KbdRptParser::OnKeyUp(uint8_t mod, uint8_t key)
{
	if (leftGUI)
	{
		// if the GUI key is down, check the list of 'alternative keys'
		for (int i = 0; i < sizeof(altKeysUSB); i++)
			if (key == altKeysUSB[i])
			{ // found in list, so send keycode and break
				Serial.write(altKeyCodes[i] | 0x80);
				break;
			}
	}
	else if (key < sizeof(keymapping)) 
	{
		if (_SHIFTED && shifted_keymapping[key] != 0)
			Serial.write(shifted_keymapping[key] | 0x80);
		else
			Serial.write(keymapping[key] | 0x80);
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
}

void loop()
{
	Usb.Task(); // Poll USB

	while (Serial.available() > 0) // Read from Keyboard serial port - to determine whether to poll mouse or not
	{
		last_rxbyte = rxbyte;
		rxbyte = Serial.read();
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
				y_ovp = 1;

			mouse_packet[0] = (y_ovn << 7) | (y_ovp << 6) | (x_ovn << 5) | (x_ovp << 4) | (mouse_right << 1) | mouse_left;
			mouse_packet[1] = mouse_dx;
			mouse_packet[2] = mouse_dy;
			mouse_dx = 0;
			mouse_dy = 0;
			for (int i = 0; i < 3; i++)
				x68kSerMouse.write(mouse_packet[i]);
		}
	}
}

// Bit 8 - 1 Released
//		   0 Pressed

//      IN	 OUT
// \ = 0x64	0x00 | 0x80 = Unmapped
// # = 0x31 0x0E | 0x8E = \|
// ` = 0x35 0x5F | 0x = ひらがな - Working OK

// #~ (non US) = 0x32
// \| (non US) = 0x64
// ` = 0x35

/**
 * Keyboard layout info moved from the original main file since we are now catering for 2 or more 
 * different layouts. For original info and attribution please see the header in main.cpp
 * 
 * This layout has been developed and tested against a UK layout Logi K270 Keyboard from the MK270 set.
 * 
 * @file layout_uk.h
 * @author Martin White
 * @brief Keyboard layout info.
 * @version 1.0
 */

#ifndef _LAYOUT_H_INCLUDED
#define _LAYOUT_H_INCLUDED

// Silence IDE without needing to include files
typedef unsigned char uint8_t;

#define CTRL_SCAN 0x71
#define SHIFT_SCAN 0x70
#define HIRA_SCAN 0x56
#define WIDTH_SCAN 0x60
#define OPT1_SCAN 0x72
#define OPT2_SCAN 0x73

// KEY CODE TO X68K---- -0-----1-----2-----3-----4-----5-----6-----7-----8-----9-----A-----B-----C-----D-----E-----F--
uint8_t keymapping[] = {0x00, 0x00, 0x00, 0x00, 0x1E, 0x2E, 0x2C, 0x20, 0x13, 0x21, 0x22, 0x23, 0x18, 0x24, 0x25, 0x26,	 // 0x
						0x30, 0x2F, 0x19, 0x1A, 0x11, 0x14, 0x1F, 0x15, 0x17, 0x2D, 0x12, 0x2B, 0x16, 0x2A, 0x02, 0x03,	 // 1x
						0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x1D, 0x01, 0x0F, 0x10, 0x35, 0x0C, 0x00, 0x1C,	 // 2x
						0x29, 0x0E, 0x34, 0x27, 0x28, 0x5F, 0x31, 0x32, 0x33, 0x5D, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,	 // 3x
						0x69, 0x6A, 0x6B, 0x6C, 0x61, 0x62, 0x5A, 0x5B, 0x5C, 0x5E, 0x36, 0x39, 0x37, 0x3A, 0x38, 0x3D,	 // 4x
						0x3B, 0x3E, 0x3C, 0x3F, 0x40, 0x41, 0x42, 0x46, 0x4E, 0x4B, 0x4C, 0x4D, 0x47, 0x48, 0x49, 0x43,	 // 5x
						0x44, 0x45, 0x4F, 0x51, 0x0E, 0x72, 0x4A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // 6x

uint8_t
shifted_keymapping[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	 // 0x
						0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	 // 1x
						0x00, 0x00, 0x00, 0x00, 0x07, 0x28, 0x09, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x27, 0x00,	 // 2x
						0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	 // 3x
						0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	 // 4x
						0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	 // 5x
						0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // 6x

// Keys that would normally be unshifted but will be shifted to insert the correct symbol.
// These must be specified in pairs. Key, then Mapped symbol
//
// NB: In both the below arrays, the comments (key & emit) might not be right, it's really 
// confusing! It's tested and working fine though, whatever the comments say :-)
uint8_t shifted_keys[] = {
	0x34, 0x08,	// ' key, emit @
	0x32, 0x04,	// # key, emit 3
	0x2E, 0x0C	// = key, emit -
};

// Keys that would normally be shifted but will be unshifted to insert the correct place symbol.
// These must be specified in pairs. Key, then Mapped symbol
uint8_t unshifted_keys[] = { 
	0x23, 0x0D,	// 6 key, emit ^
	0x33, 0x28,	// ; key, emit :
	0x34, 0x1B	// ' key, emit @
};

// USB keycodes for 'alternative keys' (holding down left-GUI) and their corresponding X68000 mappings
// Note that arrays must be the same size
uint8_t altKeysUSB[] = {0x54, 0x55, 0x56, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E};
uint8_t altKeyCodes[] = {0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59};

#endif

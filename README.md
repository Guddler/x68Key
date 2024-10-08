X68Key
==

An arduino based USB keyboard adapter for the Sharp X68000

**DISCLAIMER**: While I have tested this before uploading it and am using it myself, please consider this 
initial commit of the modified code as Beta. **I cannot be held responsible if the keyboard
suddenly decides to send a key combination you didn't type and wipes your drive.** Clearly
I'm very confident this can't happen or I wouldn't be putting the code here! But, you know, "caveat emptor"
and all that (look it up if you need to)...

Introduction
--

This is the firmware code to drive a USB keyboard adapter for the Sharp X68000 range of computers.
The code was originally written by Zuofu and posted to the nfg.forums with the permission to use
in commercial or non-commercial form provided proper credit was given. Respectfully all credit is
given to them for the initial code. I haven't explicitly sought permission for the code to be
posted to GitHub so I reserve the right to pull the repo without notice.

Please see the extensive comments at the top of main.cpp for all the original information including
links to purchase the parts to make the hardware.

Changes
--
Updated version by Martin White, Oct 2024. This updated version maintains all of Zuofu's code, with
minimal changes but adds the following features:

- allows all the keys on a PC keyboard to be mapped as they are printed on the keyboard
- comes with a fully mapped UK layout that has been tested on the Logi MK270 wireless key / mouse combo
- page up / page down seemed to be reversed (IMO) so I switched them - other tweaks may be needed?
- default mouse DPI speed increased as I felt it was a bit slow (as before, adjust to taste)
- enables or disables the keyboard as requested by the system
- receives keyboard repeat delay rates from the system when sent (as set in SRAM using switch.x).
- implements auto-repeat, this will obey the timings requested by the system as best it can, but it is
  inevitable that it will only be approximate since we are sometimes sending more than one byte of serial 
  data to acheive the correct key mapping where the original keyboard would have only sent a single byte.
  I really doubt you'll notice if the repeat rates are not exact.

Allowing the keyboard to be fully mapped, matching the printed keys on the PC kayboard wasn't a simple case
of changing the mapping as there are cases where you need to send an unshifted keypress while the user is
actually holding shift and vice-versa.

Building / Uploading
--

One change I made was to move from the default Arduino IDE to VSCode and PlatformIO. If you are also using 
PlatformIO then I would assume you know what to do. The code will still work just fine in Arduino IDE if 
you simply rename this file to something like "x68key.ino" and place it in a folder of the same name 
together with the layout file(s).

Below is an example directory structure if you want to use the Arduino IDE.

	<project dir>
		|__ libraries
		|		|__ USB_Host_Shield_2.0
		|__ x68key
				|__ x68key.ino
				|__ layout_uk.h
				|__ layout_us.h
				|__ <etc.>

Ensure that you have included the layout file you want within main.cpp (now called x68key.ino)

WARNING
-
**NB**: This updated version hasn't been extensively tested with games (as of 08-10-2024) but I did test it
with StarForce and it worked fine. I would assume most games would implement their own keyboard
handlers and even if they don't then I don't think we are doing anything any different to the
original X68000 keyboards. We just don't send any display control data so you cannot control an
original screen from a PC keyboard. It could be done, but we'd probably struggle to find enough
keys to do it. That and I don't have one, so it can't be me. I also assume that anyone with an 
original monitor probably has a keyboard too and won't be needing this adapter!

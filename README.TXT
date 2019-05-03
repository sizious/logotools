-------------------------------------------------------------------------------
pngtomr and logoinsert
-------------------------------------------------------------------------------

pngtomr: convert a png to the format used in the dreamcast ip.bin

logoinsert: insert a file from pngtomr into ip.bin at the right place


* You need to have libpng to build pngtomr.

* The logo you insert with these tools appears in the bottom right corner of 
  the screen while the SEGA license screen is showing.

* Your image should be 320x90 or smaller, and have no more than 128 colors.

* The "normal" ip.bin has room for 8192 bytes for the image - more space can
  be had by rewriting / rearranging ip.bin.

* This code has not been well tested - it seems to be working though.

* Thanks to the Bleem guys for showing that a 3rd logo like this was possible
  by having done it in Bleemcast.

-------------------------------------------------------------------------------

adk/napalm 2001
andrewk@napalm-x.com
http://www.napalm-x.com











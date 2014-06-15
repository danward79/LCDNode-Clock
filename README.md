LCDNode - Clock
=======

Since I coded the origincal LCDNode, I moved country. In my new home I can't monitor my power due tot he location of my power feed, etc. This version of the LCD display changes the core function to being a clock and a display for my climate RF nodes. I have removed the power history and display functions.

-------

**Features**

1.  Large clock display, with flashing colon to indicate seconds.
2.  Outside temperature with min/max of the day.
3.  Inside temperature (local).
4.  LED backlight turned off late at night and on early in morning.
5.  Boring weather sensor display page.
6.  Receives time updates from base station via RFM12B.
7.  Broadcasts local light and temp readings.

**TODO:**

1.  Need to decide what to use the 2 x bicolour LEDs for yet.
2.  Would like to add capability to listed for sunset / sunrise packets to control (optionally?) the backlight.

**Credits**

1.  Makes use of Jeelabs RFM12 and GLCD libraries.
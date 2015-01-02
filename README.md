Arduino-IR-Multiplexer
======================

For use with an Arduino IR Multiplexer shield. (https://oshpark.com/shared_projects/mLu4CjXF)

This library utilizes the IRRemote Arduino Library by Ken Shirriff.
The IRRemoteInt.h file in that library must be edited to tell the library to use pin 9 rather than pin 3. Look for the lines like:

// Arduino Duemilanove, Diecimila, LilyPad, Mini, Fio, etc
#else
  #define IR_USE_TIMER1   // tx = pin 9
  //#define IR_USE_TIMER2     // tx = pin 3
#endif

These lines should be around line 68. Make sure that, as above, the TIMER1 line is uncommented and the TIMER2 line is commented.

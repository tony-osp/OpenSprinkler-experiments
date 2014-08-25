This is my other OpenSprinkler-type experiment - an alternative sprinkler control program, running on the standard Arduino hardware.

The core sprinkler control program was created by Richard Zimmerman, and is available at his Github repository:
https://github.com/rszimm/sprinklers_pi

I added local UI module to this control program to allow it to operate similar to my verion of the OpenSprinkler - UI style
is inspired by the original OpenSprinkler UI as well as by typical UIs found in battery-operated sprinkler timers.

Hardware: I'm using standard Arduino Mega, with the standard Ethernet on W5100 chip, and standard 1602 LCD. Input is provided by
          four buttons connected to pins A0-A3. The code also includes support for analog input, allowing use of the typical
          1602 LCD shield (it uses analog levels for 6 input buttons). Input selection is done by controlling appropriate #define symbol.



Software license: The situation with license is not very clear because core piece of the software created by Richard Zimmerman did not
                  include explicit license with the code.
                  However Richard published his code on Github for public access, and in e-mail conversation he stated that he intend
                  this code to be freely available for non-commercial/private use, including code modifications and derivative work.

                  Modules I created (right now it is LocalUI.cpp, LocalUI.h and keys.cpp) are published with Apache v2 license.
                  You can use these modules for any purpose as long as there is acknowledgement (in "About" box, readme etc).
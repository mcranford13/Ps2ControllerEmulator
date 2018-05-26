# Ps2ControllerEmulator
Ps2 Controller Emulator for Arduino


What is this?
----------------
This code turns an arduino into a ps2 controller, created with building a portable Ps2 in mind. 
For more information check out: https://bitbuilt.net

Status
--------------
No support for Analog Sticks or Rumble (for now), but everything else works great.
No support for Guitar Hero Guitar, but that would be very simple to add.

External Circuitry / GPIO
--------------------------
Because of the arduino's lack of a Open-Drain configuration on gpio, you will need to add it externally on the MISO and ACK lines.

What I have set up is: MISO <---4.7K resistor ---> B Transistor, E Tranistor <--> GND, C Transistor <-- 10K Resistor --> 3.3v .

NOTE: Due to the transistor not being able to switch instantaneously, some glitching on the MISO line may occur.

Also due to the Arduino Uno's lack of GPIO, an Arduino Mega is recommended to handle all the buttons and analog.
28 GPIOs are required for full controller functionality.


Credit
---------------
This would not have been possible for the following, and I highly recommend looking at their stuff for more information:

1. Scanlime - For documenting the ps2 controller protocol, https://gist.github.com/scanlime/5042071
2. CuriousInventor - Creating a very easy to follow reference, http://store.curiousinventor.com/guides/PS2/
3. Aurelio Mannara - Guidance, and Mentoring.











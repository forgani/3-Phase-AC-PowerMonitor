# Monitoring the current of the 3-Phase AC load 

Building a Home Energy Monitor with ESP-12E and  3 X CT Sensors

A system to control the energy usage and monitoring the current of the 3-Phase AC load of your home in real time.

This system helps you to remotely monitor the used energy for each of the three-phases.
You can also monitor the three-phase line for unbalanced voltage consumption and also ideally equally distribute the load with the consumption over the 3 phases.

The most important components are obviously the ESP-12E microcontroller, 74HC4051 Multiplexer and the 3 x CT sensor (Current Transformer).

I’m using the AC current transformer to measure the AC current indirectly and finally, monitor the data by using Blynk app.

Components:

ESP-12E microcontroller + 74HC4051 Multiplexer
3 x CT Sensors AC current transformer which has an AC output. (You can use any standard current transformer)
(This sensors clamp over the main cable and transforms the magnetic field around the cable into a ac voltage.)
Blynk Server & Blynk app builder
(This tool allows you to make apps for your projects by using various widgets. It is available for Android and iOS platforms.)
Software Used:

Arduino IDE
Blynk Mobile App (for Android or iOS)
Libraries: EmonLib, BlynkSimpleEsp32, NTPClient, Time
 

Here's the wiring and circuit diagram
![image](https://user-images.githubusercontent.com/25223934/136688666-8dd04bfc-4330-48f5-b588-237fb34aa6b9.png)


So as we know the ESP-12E has only one single analog input, with an input range of 0 – 1.0V.

The ESP-12E ADC (analog to digital converter) has a resolution of 10 bits and the analog input measures varying voltage levels between 0V to 1.0V.
The measured voltage will assigned to a value between 0 and 1023 bits, in which 0V corresponds to 0, and 1.0V corresponds to 1023 bits.
To measure the 3 phases we need 3x ADC input. So I’m increasing the number of analog inputs to three by using a 74HC4051 multiplexer.
With the 74HC4051 we can connect up to 8 analog devices to the single analog pin on the ESP-12E (It uses 3 digital pins for addressing, from which two of them will cover my purpose).

The AC current transformer which I’m using has 2000 windings and can measure up to 28 Amps.
We are supposed to adjust the output of the CT in connection with a burden resistor so that the AC output voltage is exactly 1V sine wave.
The formulas Here and the Tool for calculating burden resistor size  helped me in my case.
So for example for a CT with 2000 windings and a burden resistor of 25Ω  I measured a max primary current of about 28A. (35Ω for 20Amps).

Burden Resistor (ohms) = (AREF * CT TURNS) / (2√2 * max primary current)
The value of burden resistor depends on the maximum primary current that we want to measure.
It should be calculated for ESP-12E so that the maximum of the CT output AC voltage is exactly 1V.


To visualize the data, you create a simple Blynk app by using blynk mobile visual interface.
![image](https://user-images.githubusercontent.com/25223934/136706706-1ac96483-67ae-4911-89af-8a0cc76f3ae1.png)


for more info: https://www.forgani.com/electronics-projects/home-energy-monitor/


![3-AC-blynk](https://user-images.githubusercontent.com/25223934/136781403-09f9b3c4-f09b-4351-9c72-1826d17253d3.jpg)



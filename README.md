# FreqGen
**Wave generator and frequency meter on ESP32 with RemoteXY**

This application is a square wave frequency generator on a ESP32 MCU with a BLE 
connection to the Android app 'RemoteXY'. It can both generate and measure frequencies. 

The ESP32 is remotely controlled via Bluetooth and the RemoteXY app. In the app the
output and measured frequencies are displayed. The user interface shows a slider 
where the output frequency can be set in 201 steps  ranging from 0.1 Hz to 15 MHz.
All duty cycles are 50%.

When the ESP32 is started the initial 1 Hz frequency is visible on the internal 
LED (like blink). The Bluetooth connection is named ‘FreqGen’.


__By: Erikjvl 2023-05-05__


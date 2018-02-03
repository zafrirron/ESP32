# ESP32 Home Gate Bell+Open Lock Manager
# Using: Async Web Server; DigitalPin interrupts; Sonos http play; http logger logger

A very simple straight forward home gate management:

1. Detecting a gate bell button press (hooked to ESP digital pin).
2. Play sound on my Home Sonos System (Server runnig on local network - (https://github.com/jishi/node-sonos-http-api).
3. Listen on local http server for gate open request (ESP local server).
4. Close a relay attached to my gate lock to open a gate (hooked to ESP digital pin).
5. Log the bell and gate open events on local network simple logger (Simple http logger local server - https://github.com/divhide/node-log-server).

Trying to keep it simple and event driven coding style.

SDK used: Arduino + ESP32 toolchain

Libraries and functions Used:
1. Wifi: Used to connect to local wifi (fixed IP), and use wifi events to detect connection and wifi connection lost events.
2. HTTPClient: Used to send HTTP requests to Sonos service and Logger service 
3. ESPAsyncWebServer: Used to run local asynd server, using evetns to manage http request to open the gate (https://github.com/me-no-dev/ESPAsyncWebServer)
4. digitalPinToInterrupt: used to manage a bell press as event (https://techtutorialsx.com/2017/09/30/esp32-arduino-external-interrupts/)
5. NTP time sync: Used to log all evetns with time tag (Arduino Examples-ESP32-Time) 

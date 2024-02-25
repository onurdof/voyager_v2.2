| Supported Targets | ESP32-S2 | ESP32-S3 |
| ----------------- | -------- | -------- |

# Voyager (A BICYCLE SIMULATOR PROJECT )

Human interface devices (HID) are one of the most common USB devices, it is implemented in various devices such as keyboards, mice, game controllers, sensors and alphanumeric display devices.
In this project, we implement USB gamepads.
Upon connection to USB host (PC), the project send simulator data as HID gamepad and receive data from the host using UART for now. (It will changed later to USB PID)





### Hardware Required

Any ESP board that have USB-OTG supported.

#### Pin Assignment

For each component there is a different assignments. (Check components/ component_name/component_name.h file)
### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```bash
idf.py -p PORT flash monitor
```
PORT : /dev/ttyUSBx in linux

(Replace PORT with the name of the serial port to use.)

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.


## Comm package

```bash
You should check dof_gamecomm.py and specs.json file for how the comm(reading data from the game and send it to the esp32 chip) works with pc . Also you should create a txt file and add txt file's path to the specs.json

```
```bash
-> dof.ico is  new icon
-> logo.ico is old icon
```
-> Logos for creating exe for windows


## TODO

There is a list of features that will implemented for improving efficiency.

```
-> Use interrupt method for buttons and switches 
-> Add error checking mechanism for functions
-> UART should be change to USB force feedback (check PID documentation) 
-> Add kconfig for motor speed and some other peripherals like USB name USB PID number , etc.. 
->pyinstaller --onefile --noconsole --icon=dof.ico dof_gamecomm.py
```



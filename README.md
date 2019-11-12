# IoT-clock-project
Concept:
Main idea behind project was create device which will gather 
temperature data from three sensors and display it on LCD. Selected
temperature source value will be stored in FRAM memory. 

Device will be used also as clock so near temperature on main 
window will be display curent hour. Device will be use WiFi 
interface to data synchronization(time and temperature measurement). 

Project phases:
1. I created PC driver which use FTDI chips and emulate interfaces of 
microntroller like GPIO, I2C, SPI and UART. I chosed LPC111X as target
microcontroller. I planed build final project on LPC1115 microcontroller.

2. Using emulated microcontroller interfaces I wrote drivers for LCD 
with ILI9325 controller, driver for touch screen with ADS7843 controller,
Si7021-A20 temperature sensor and ESP-01 module.

3) I created clock project with uGui library with first concept of 
application GUI. during development windows sdl library was used to 
display GUI. On final step of this phase test with real LCD was
performed and project work correctly.

4) I prepared code to run on real hardware (lpcxpresso board with 
LPC1114. Unfortunetly code exceed microcontroller flash limit. 
Code was modified to flash microcontroller and check basic 
functionality. After flashed all things work correctly. New 
microcontroller was chose to fulfil memory requirements it was 
LPC11E68. Evaluation board with this microcontroler was performed. 
Driver was ported to new microcontroller and all things work without
problem.

5) I prepared first release of clock board pcb with necessary
thing. Clock board contain FRAM (FM25W256), three I2C expander 
bus(PCA9507), footprint for ESP-07. The same code from my eval 
board run without any problem on clock board.

6) FRAM driver was added and store clockstate and temperature.
GUI functionality was added: set backlight of LCD, calibrate touch 
screen, factory reset, display temperature graph, calendar 
functionality, alarm animation. WiFi GUI concept was added.
uGui lobrary was modified to run faster.

First release of source code

7) WiFi support was be added. 

Second release of code.

8) New version of hardware with fixes.

9) Added support to monochrome bitmap in uGui. Added monochrome bitmap 
in GUI code. Added Options fields in bitmap.

Third release of code.
First official release of HW.

10) Code refactoring is necessary to delete thing which isn't necessary 
and improve code readability.

Fourth release of code.

11) Prepare android application to control clock via Wifi.
12) Prepare document with clock specification.

License of code project:
  Below drivers is licensed under BSD 3-claus license:
  -BacklightControl
  -BuzzerControl
  -ESP_Layer
  -FRAM_Driver
  -GPIO_Driver
  -I2C_Driver
  -LCD
  -SOMEIP_Layer
  -SPI_Driver
  -TemperatureSensor
  -TouchPanel
  -UART_Driver
  
  Below files is licensed under GPL license:
  -ClockControl
  -GUI_Clock
  -Thread
  -WIFI_InteractionLayer
  

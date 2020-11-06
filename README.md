# Camera Slider with stepper motor drive and analog stick control

* Using Bresenham multi-dimensional as illustrated at http://members.chello.at/easyfilter/bresenham.html
* Analog-Stick-Version - No auto-range at startup. All positions are user responsibility. 
* Microcontrollers supported: 
  * Arduino Nano or clone
  * ESP32 breakout board e.g. Doit Devkit

## Hardware used:

### Input:
* Analog 3-axis stick with button, e.g. "JH-D400X-R4 10kOhm 4D sealed rocker"
* digital rotating encoder
* 3 buttons: 
  * start/stop the motors by button on top of analog stick, 
  * config select by encoder button 
  * skip to next target by simple momentary button

### Output: 
  * 1602 display module with I2C connector
    * for Nano: 5 Volt version
    * for ESP32: either 5 Volt version and 2-channel bidir level shifter 3.3 <-> 5, or 3.3 V module
  * Stepper drivers: TMC2208

### Power supply:
  * 10+ Volts DC of any kind (Lipo, AC adapter, Li-ion battery pack of power tools ...)
  * recommended: additional step-down regulator to deliver 5 volts for display, TMC driver logic side and controller module

### Steppers:
* e.g. MINEBEA 17PM-K077BP01CN (cheap, rather low power consumption, 10.2 Volts nominal, max. 300 mA but works fine for this purpose)

### wiring @todo
* perfboard sample pictures (incomplete) see: https://www.instagram.com/p/CFacepfKM1g/

## Menu structure (rotation/push encoder button)
* MODE SELECT 
  * -> (Click) -> STICK DRIVE | STICK SLOW | PINGPONG | ONEWAY | ONEWAY_TRIG -> (click =  select + return)
* SET START 
  * -> (Click) -> set coordinate index 0, back to menu
* SET STOP
  * -> (Click) -> set coordinate index 1, back to menu
* SET STEP DELAY 
  * -> (Click) -> Turn right: increase value (slow down) | Turn Left: decrease value (speed up) -> (Click = Return)
* OPTIONS 
  * -> (Click) -> Recalibrate | Reboot/Reset? (TODO...) 

### Button B:
While driving in ... mode:
* STICK DRIVE -> no action
* STICK SLOW -> no action
* PINGPONG -> always skip current target and start moving to next target
* ONEWAY -> always skip current target and start moving to next target
* ONEWAY_TRIG -> If running, skip to next target | if waiting, start the run
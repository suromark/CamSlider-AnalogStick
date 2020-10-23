Camera Slider with stepper motor drive and analog stick control

Using Bresenham multi-dimensional as illustrated at http://members.chello.at/easyfilter/bresenham.html

Analog-Stick-Version - No auto-range at startup. All positions are user responsibility. 

Startup -> current position = Zero

Input:
Analog 3-axis stick with button, digital rotating knob, 3 button options: start/stop, config cycle, skip2next
Analog stick model e.g. JH-D400X-R4 10kOhm 4D sealed rocker

Steppers: e.g. MINEBEA 17PM-K077BP01CN (cheap, rather low power consumption, 10.2 Volts nominal, max. 300 mA (but works fine at 100 mA already), comes with GT2 14T gear premounted) 

Menu structure (rotation/click encoder knob)

MODE -> (Click) -> STICK DRIVE | STICK SLOW | PINGPONG | ONEWAY | ONEWAY_TRIG -> (click =  select + return)
 | 
POINT 1 -> (Click) -> set coordinate index 0, back to menu
 |
POINT 2 -> (Click) -> set coordinate index 1, back to menu
 |
STEP DELAY -> (Click) -> Turn right: UP | Turn Left: DOWN -> (Click = Return)
 |
OPTIONS -> (Click) -> Recalibrate | Reboot?

Analog stick top button: Start / Stop motors

Button B:
While driving in ... mode:
 STICK DRIVE -> no action
 STICK SLOW -> no action
 PINGPONG -> Skip to next target
 ONEWAY -> Return to start
 ONEWAY_TRIG -> Return to start
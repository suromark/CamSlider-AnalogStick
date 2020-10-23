Camera Slider with stepper motor drive and analog stick control

Using Bresenham multi-dimensional as illustrated at http://members.chello.at/easyfilter/bresenham.html

Analog-Stick-Version - No auto-range at startup. All positions are user responsibility. 

Ideas:
Startup -> position = Zero

Input:
Analog 3-axis stick with button, digital rotating knob, 3 button options: start/stop, config cycle, skip2next

Menu structure (rotation/click knob)

MODE -> (Click) -> STICK DRIVE | STICK SLOW | PINGPONG | ONEWAY | ONEWAY_TRIG -> (click =  select + return)
 | 
POINT 1 -> (Click) -> set coordinate index 0, back to menu
 |
POINT 2 -> (Click) -> set coordinate index 1, back to menu
 |
STEP DELAY -> (Click) -> Turn right: UP | Turn Left: DOWN -> (Click = Return)
 |
OPTIONS -> (Click) -> Recalibrate | Reboot?

Button Analog-Top: Start / Stop motors

Button B:
While driving in ... mode:
 STICK DRIVE -> no action
 STICK SLOW -> no action
 PINGPONG -> Skip to next target
 ONEWAY -> Return to start
 ONEWAY_TRIG -> Return to start
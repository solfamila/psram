Hardware requirements
=====================
- Micro USB cable
- MIMXRT700-EVK board
- Personal Computer
- Oscilloscope

Board settings
==============
No special is needed.
Use oscilloscope to measure the output 48KHz PWM signal pin at J5-8 pin.

Prepare the Demo
================
1. Connect a USB cable between the PC host and the MCU-LINK USB port on the board.
2. Open a serial terminal with these settings:
    - 115200 baud rate
    - 8 data bits
    - No parity
    - One stop bit
    - No flow control
3. Download the program to the target board.
4. Either press the reset button on your board or launch the debugger in your IDE to begin running the example.

Running the demo
================
When the example runs successfully, if the input duty cycle range is 0~100,
You can see similar information from the terminal as follows:
~~~~~~~~~~~~
FLEXIO_PWM demo start.

Please input a value (0 - 100) to set duty cycle: 10
Input value is 10

PWM duty cycle is: 10
PWM leave is: 0

Please input a value (0 - 100) to set duty cycle: 150
Input value is 150
Your value is output of range.
Set pwm output to IDLE.

Please input pwm idle status (0 or 1): 1
Input IDLE state value is 1

PWM leave is: 1 

Please input a value (0 - 100) to set duty cycle: 
......
~~~~~~~~~~~~
Note:
1. The duty cycle of the PWM is variable, except for the idle state, 0 and 100.
2. If the input duty cycle range exceeds 100, the pwm will be set to idle state
3. If the set pwm idle value is out of range, it will output "Your value is output of range."
   and then return to set the duty cycle.

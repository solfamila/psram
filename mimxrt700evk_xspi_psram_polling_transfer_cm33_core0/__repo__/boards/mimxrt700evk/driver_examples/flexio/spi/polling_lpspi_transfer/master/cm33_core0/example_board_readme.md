Hardware requirements
=====================
- Micro USB cable
- MIMXRT700-EVK Board
- Personal Computer

Board settings
==============
To make the example work, connections needed to be as follows:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 SLAVE(LPSPI16) connect to               MASTER(FlexIO0)
Pin Name   Board Location            Pin Name  Board Location
SOUT        J22 pin 6                SIN       J5 pin 3
SIN         J22 pin 5                SOUT      J5 Pin 6
SCK         J22 pin 4                SCK       J5 pin 4
PCS0        J22 pin 3                PCS       J5 pin 5
GND         J22 pin 8                GND       J6 pin 7
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Prepare the Demo
================
1.  Connect a USB cable between the host PC and the MCU-LINK USB port on the board.
2.  Open a serial terminal with the following settings:
    - 115200 baud rate
    - 8 data bits
    - No parity
    - One stop bit
    - No flow control
3. Download the program to the target board.
4. Either press the reset button on your board or launch the debugger in your IDE to begin running
   the demo.

Running the demo
================
You can see the similar message shows following in the terminal if the example runs successfully.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~
FLEXIO Master - LPSPI Slave polling example start.
This example use one flexio spi as master and one lpspi instance as slave on one board.
Master uses polling and slave uses interrupt way.
Please make sure you make the correct line connection. Basically, the connection is:
FLEXIO_SPI_master -- LPSPI_slave
      CLK        --    CLK
      PCS        --    PCS
      SOUT       --    SIN
      SIN        --    SOUT

This is LPSPI slave call back.
FLEXIO SPI master <-> LPSPI slave transfer all data matched!
End of example.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

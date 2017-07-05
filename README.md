# stacker-clone

A clone of the merchandiser "STACKER" for an Arduino Nano (or similar) device, using a LED matrix as the display.

## Getting Started

The project sketch depends on the following 3rd-party libraries.

* `DMD` - "A library for driving the Freetronics 512 pixel dot matrix LED display "DMD"". Available [here](https://github.com/freetronics/DMD).
* `arduino-timerone` - A collection of functions used to simplify timer setup. Available [here](https://code.google.com/archive/p/arduino-timerone/downloads).

After these are added to the local `Arduino/libraries` directory, the sketch `stacker-clone.ino` may be programmed on an Arduino device in the usual method using the Arduino IDE.

The LED matrix used in this project is the Techbrands XC4622 available and described [here](https://www.jaycar.com.au/white-led-dot-matrix-display-for-arduino/p/XC4622). The included sketch requires the following circuit to be constructed. 

| Arduino Pin Number | Pin Label | XC4622 Pin Number | Pin Label | 
| --- | --- | --- | --- |   
| - | +5V | - | Vcc\* |  
| - | GND | 15 | - |  
| 6 | D6 | 2 | A-D6 |  
| 7 | D7 | 4 | B-D7 |  
| 8 | D8 | 10 | SCLK-D9 |  
| 9 | D9 | 1 | OE-D9 |  
| 11 | D11 | 12 | DATA-D11 |  
| 13 | D13 | 8 | CLK-D13 |  

\* Where Vcc is either of the centre connectors on the back of the XC4622.

### Running

After setup, a programmed Arduino device will display the game board in its starting state, with three blocks sliding left and right on the bottom row. The tower grows every time the current row's blocks are stopped while supported by the lower row. As the tower grows, the blocks move faster. The objective is to build the tallest tower. 

Before every block movement, the Arduino device checks for serial communication (at Baud rate 9600). If any character was received since the last movement, the current block position is locked in.

When a block position is locked in, any blocks without support fall away. The game is over when the blocks were stopped without overlap with the lower row. Once a game has finished, a quick animation is displayed before restarting. In subsequent rounds, the maximum height achieved since start up is displayed by disabling the border LEDs of that row.

### Game Pad

The included Python script `GamePad.py` was written to simplify interaction with the device. This script generates a simple GUI with just a single button. This button (or any keyboard letter or number key) sends a single character to the Arduino device.

To run `GamePad.py`, the host machine must have the following installed:
* `Python3` - The programming language in which the script was written. Available [here](https://www.python.org/).
* `Tkinter` - The Python library required for the game pad user interface.\*

\*Included with the standard Python3 installation on Windows and MacOS, requires separate installation on Linux. For Debian-based systems, this is achieved through the following command:
`apt-get install python3-tk`

The game pad may be started with the command  
`python3 GamePad.py`

Before displaying the GUI, a command line prompt for the Arduino device's COM port appears. Simply submit the COM port's name (e.g. "COM3") to connect and start the game.

## Authors

* **Marc Katzef** - [mkatzef](https://github.com/mkatzef)

## Acknowledgements

* **Jaycar Electronics** - Part supplier and author of [this guide](https://www.jaycar.co.nz/diy-arduino-clock) showing how to interact with the LED matrix display.

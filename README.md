## ir_receiver<br>
C code to receive NEC IR codes from a demodulator connected to a GPIO pin<br>
<br>
Written by Larry Bank<br>
Copyright (c) 2018 BitBank Software, Inc.<br>
Project started 4/15/2018<br>
<br>
bitbank@pobox.com<br>

The purpose of this project is to show how to receive NEC IR codes in the
simplest manner possible and without the use of any kernel drivers or external
libraries. To access GPIO pins in a convient way, I make use of my ArmbianIO
library (https://github.com/bitbank2/ArmbianIO).<br>

<br>
Building the program<br>
--------------------<br>
make<br>
<br>
Running the program<br>
-------------------<br>
sudo ./ir<br>
<br>
The program will wait for codes to be received and try to match them to the
included key list. I used an inexpensive IR remote that I received with some
other gear for testing. The key table maps buttons 0-9 to the 0-9 keys on the
keyboard. The window with the focus will receive the keypresses. If running
remotely (e.g. through SSH/TTY), then the keys will not be sent since I make
use of the uinput device to simulate key presses on the local terminal.
<br>

![My Setup](/setup.jpg?raw=true "My Setup")


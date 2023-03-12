# Credit to Roel Jordans
I used Roel's work https://github.com/rjordans/libopencm3-u8g2-test as a starting point. His repo worked great with u8x8 but I wanted to use u8g2 functions. This is the result

Roel's original readme: still applicable

# Materials

 - 1x "blue-pill" board (STM32F103C8)
 - 1x 0.96" OLED module, I2C version
 - STlink programmer
 - Some wires
 - (if needed) 2x 4.7k resistor for I2C pull-up


# Wiring I2C

Connect I2C1 pins to OLED module.  Pin B6 to SCK and pin B7 to SDA.  Don't forget to also connect the power and ground.  In my case the I2C pull-up resistors were installed on the OLED module, if yours hasn't got them make sure to add pull-up resistors.  4.7k or so to the 3v3 line should do (the exact value isn't critical).

# Software
If you have already got the libopencm3 examples running then you should have everything you need.

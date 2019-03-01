# SD_CARD_via_GPIO
External SD CARD I/O Library for RaspberryPi/OrangePi.   
You can access external SD CARD using GPIO.   

I ported from here.   
http://blogsmayan.blogspot.jp/p/interfacing-sd-card.html   


---

# Wirering for External SD CARD module   

|SD CARD||Rpi/OPI|
|:-:|:-:|:-:|
|GND|--|GND|
|VCC|--|5V|
|MISO|--|Pin#21(SPI MISO)|
|MOSI|--|Pin#19(SPI MOSI)|
|SCK|--|Pin#23(SPI SCLK)|
|CS|--|Pin#24(SPI CE0)(*)|
|CS|--|Pin#26(SPI CE1)(*)|

\*Note:   
Rpi have 2 SPI.   
CE0 and GPIO10.   
CE1 and GPIO11.   

Opi-PC have only 1 SPI.   
CE0 and GPIO10.   

Opi-ZERO have only 1 SPI.   
CE1 and GPIO10.   

You must to chack mmcbb_gpio.c   

```
//GPIO10 as CE
#define 	CE		10
//GPIO11 as CE
//#define 	CE		11
```

---

# SD CARD format   

You have to format sd card with FAT32.   

---

# How to use   

**Using bcm2835 Library(RPi Only)**   
This is original Library.  
Thank you Rajiv for your good code.   

cc -o RpiSDCard main.c ff.c mmcbb.c -lbcm2835   
sudo ./RpiSDCard   



**Using wiringPi Library(RPi/OPi)**   

cc -o RpiSDCard_gpio main.c ff.c mmcbb_gpio.c -lwiringPi   
sudo ./RpiSDCard_gpio   



```
$ sudo ./RpiSDCard_gpio

Open an existing file (HELLO.TXT).
f_open rc=0

Type the file content.
Hello world!
Goodbye world.
121
253
199

Close the file.

Create a new file (HELLO.TXT).

Write a text data. (Hello world!)
14 bytes written.
16 bytes written.
1 bytes written.
1 bytes written.
1 bytes written.
2 bytes written.
1 bytes written.
1 bytes written.
1 bytes written.
2 bytes written.
1 bytes written.
1 bytes written.
1 bytes written.
2 bytes written.

Close the file.

Open root directory.

Directory listing...
   <dir>  SYSTEM~1
      88  TEST.TXT
      45  HELLO.TXT

Test completed.
```



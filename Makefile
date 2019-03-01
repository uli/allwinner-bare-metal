all : RpiSDCard RpiSDCard_gpio

RpiSDCard : main.c ff.c mmcbb.c
	cc -o RpiSDCard main.c ff.c mmcbb.c -lbcm2835

RpiSDCard_gpio : main.c ff.c mmcbb_gpio.c
	cc -o RpiSDCard_gpio main.c ff.c mmcbb_gpio.c -lwiringPi

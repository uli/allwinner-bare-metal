#include <display.h>
#include <usb.h>
#include <fs.h>
#include <ports.h>
#include <system.h>
#include <stdio.h>
#include <../../uart.h>

void game_tick(uint32_t tick_counter);
void game_start();

extern int sample_count;
void game_tick_next() {
  display_swap_buffers();
  display_clear_active_buffer();
  game_tick(tick_counter);
  printf("%d samples\n", sample_count);
}

void main(void)
{
  game_start();
    uart_print("Hello world \n");

  // Go back to sleep
  while(1) {
    static unsigned int cframe;
    asm("wfi");
    usb_task();
    sd_detect();
    if (cframe != tick_counter) {
    	game_tick_next();
    	cframe = tick_counter;
    }
    //audio_queue_samples();
  }
}

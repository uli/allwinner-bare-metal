#include <usb.h>
#include <fs.h>
#include <stdio.h>

void game_tick(uint32_t tick_counter);
void game_start();

extern int sample_count;
void game_tick_next() {
  buffer_swap();
  game_tick(tick_counter);
  printf("%d samples\n", sample_count);
}

int sd_detect;

int main(int argc, char **argv)
{
  game_start();

  // Go back to sleep
  while(1) {
    static unsigned int cframe;
    asm("wfi");
    usb_task();
    if (!sd_detect && !get_pin_data(PORTF, 6)) {
      printf("card in\n");
      if (!fs_init())
        sd_detect = 1;
    } else if (sd_detect && get_pin_data(PORTF, 6)) {
      printf("card out\n");
      fs_deinit();
      sd_detect = 0;
    }
    if (cframe != tick_counter) {
    	game_tick_next();
    	cframe = tick_counter;
    }
    //audio_queue_samples();
  }
}

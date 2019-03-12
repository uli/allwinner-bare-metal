#include <stdint.h>
#include "spritelayers.h"
#include "display.h"
#include "demo_data.h"
#include "uart.h"

// Define the scrolling background pattern
uint32_t* pattern[100*100];
struct sprite_layer background;

void game_start() {
  // Populate the pattern
  for(int n=0;n<100*100;n++)
    pattern[n] = demo_sprite;

  // Configure the background to use the pattern
  background.pattern = pattern;
  background.x_size = 100;
  background.y_size = 100;
  background.x_offset = 0;
  background.y_offset = 0;
}

void game_tick(uint32_t tick_counter) {
  // Scroll the background
  background.x_offset = -(tick_counter%16);
  // ...and render it 20 times for fun
  // This approximately represents the performance limit, 10,000 sprites
  for(int n=0;n<20;n++)
    render_layer(&background);
}

#include <stdint.h>
#include "spritelayers.h"
#include "display.h"

inline void render_raw(uint32_t* pattern, volatile uint32_t* destination);

void render_layer(struct sprite_layer* layer) {
  uint32_t** pattern = layer->pattern;
  // Iterate over sprites
  int32_t y_position = layer->y_offset;
  for(uint32_t y=0; y < layer->y_size; y++) {
    y_position += 16;
    int32_t x_position = layer->x_offset;
    for(uint32_t x=0; x < layer->x_size; x++) {
      x_position += 16;
      // Don't render anything outside the buffer area
      if(*pattern &&
         x_position > 0 && x_position < 496 &&
         y_position > 0 && y_position < 286)
        render_raw(*pattern, active_buffer + y_position * 512 + x_position);
      // }
      pattern++;
    }
  }
}

void render_sprite(uint32_t* pattern, int32_t x_offset, int32_t y_offset){
  // Add 16 to the offset such that 0,0 is the upper-left of the visible display
  volatile uint32_t* destination = active_buffer + (y_offset + 16) * 512 + (x_offset + 16);
  render_raw(pattern, destination);
}


inline void render_raw(uint32_t* pattern, volatile uint32_t* destination) {
  for(int y=0; y<16; y++) {
    for(int x=0; x<16; x++) {
      if(*pattern) {
        *destination = *pattern;
      }
      pattern++;
      destination++;
    }
    destination += 512-16;
  }
}

// TODO: An additionl function to render individual sprites with full 8-bit opacity

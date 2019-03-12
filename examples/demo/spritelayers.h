#include <stdint.h>

struct sprite_layer {
  uint32_t** pattern;
  uint32_t x_size;
  uint32_t y_size;
  int32_t x_offset;
  int32_t y_offset;
};

void render_layer(struct sprite_layer* layer);
void render_sprite(uint32_t* pattern, int32_t x, int32_t y);

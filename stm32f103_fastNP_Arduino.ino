#include <string.h>

/* The ws2812 refresh code is implemented in C, so we need the 'extern C' here */
extern "C" {
#include "ws2812_led.h"
void GPIO_PortB_Output_Init();
}

/*
 * NOTE: The CPU speed needs to be set for 72MHz (Nominal)
 * The Optimization Level must be at least -O1 (recommend -O3)
 * Edit ws2812_led.h to change the channel-to-pin assignments, and to set how many channels you want
 */

/* Framebuffers don't have to be of fixed length.
 * We're just using a fixed length for the demo animation, to keep the animation code simple.
 */
#define FRAMEBUFFER_SIZE        256

/* The order of R, G, B is important, because that's the order these get clocked out.
 * The LEDs expect the green channel first, then red, then blue.
 */
struct pixel {
    uint8_t g;
    uint8_t r;
    uint8_t b;
};

void make_pretty_colors(struct pixel *framebuffer, int channel, int state)
{
    int red_offset, green_offset, blue_offset, i;

    red_offset = 0;
    green_offset = (FRAMEBUFFER_SIZE / 4) * ((channel) & 0x03);
    blue_offset = (FRAMEBUFFER_SIZE / 4) * ((channel >> 2) & 0x03);

    /* Just generate a different-looking psychedelic-looking pattern for each channel, to
     * test/prove that each channel is receiving a unique pattern
     */
    for (i = 0; i < FRAMEBUFFER_SIZE / 2; i++) {
        framebuffer[(i + red_offset + state) % FRAMEBUFFER_SIZE].r = i / 8;
        framebuffer[(i + green_offset + state) % FRAMEBUFFER_SIZE].g = i / 8;
        framebuffer[(i + blue_offset + state) % FRAMEBUFFER_SIZE].b = i / 8;
    }
}

struct pixel channel_framebuffers[WS2812_NUM_CHANNELS][FRAMEBUFFER_SIZE];
struct led_channel_info led_channels[WS2812_NUM_CHANNELS];

void setup() {
  // Set all GPIOB pins as outputs. We will be sending LED data there
/*
  pinMode(18, OUTPUT);  // B0
  pinMode(19, OUTPUT);  // B1
  // Arduino does not bring out pin B2, but it exists.
  // On the Blue Pill board, it's shared with the yellow jumpers, through a resistor I think
  pinMode(39, OUTPUT);  // B3
  pinMode(40, OUTPUT);  // B4
  pinMode(41, OUTPUT);  // B5
  pinMode(42, OUTPUT);  // B6
  pinMode(43, OUTPUT);  // B7
  pinMode(45, OUTPUT);  // B8
  pinMode(46, OUTPUT);  // B9

  pinMode(21, OUTPUT);  // B10
  pinMode(22, OUTPUT);  // B11  
  pinMode(25, OUTPUT);  // B12
  pinMode(26, OUTPUT);  // B13
  pinMode(27, OUTPUT);  // B14
  pinMode(28, OUTPUT);  // B15
*/

  /* Okay I don't know why I can't set all the Arduino PortB pins as outputs, so I'm just gonna
   * set them all to output using the STM32 GPIO HAL. See gpio_outputs.c
   * 
   * THIS WILL SET EVERY PBxx GPIO AS AN OUTPUT (and we'll send LED data there)
   */
  GPIO_PortB_Output_Init();

  /* Populate the led_channel_info structure. Set the framebuffer pointer for each channel,
   * and the length of the framebuffer for that channel. Lengths are in bytes, NOT in pixels.
   * We allow some framebuffers to be shorter than others (ie, if your LED strips are of
   * unequal lengths) without wasting memory. Removing the length check from ws2812_refresh()
   * will improve performance, but we are within spec as-is.
   */
  for (int i = 0; i < WS2812_NUM_CHANNELS; i++) {
      led_channels[i].framebuffer = (uint8_t*) channel_framebuffers[i];
      led_channels[i].length = FRAMEBUFFER_SIZE * sizeof(struct pixel);
  }

  ws2812_init();
}

/* This just tracks the position of our animation, so that we draw something slightly different
 *  on every loop.
 */
int animation_state = 0;

void loop() {

  /* Draw a pretty (but different) pattern for each channel.
   * We can trivially send the same framebuffer to each channel, but we won't do that, just
   * to prove that we can truly refresh 16 unique framebuffers at once.
   */
  for (int ch = 0; ch < WS2812_NUM_CHANNELS; ch++)
      make_pretty_colors(channel_framebuffers[ch], ch, animation_state);

  animation_state++;

  /* Now that we drew something pretty for every channel, actually send our data to our LEDs */
  __disable_irq();
  ws2812_refresh(led_channels, GPIOB);
  __enable_irq();
}

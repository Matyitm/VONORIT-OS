#ifndef VONORIT_INPUT_H
#define VONORIT_INPUT_H

#include "framebuffer.h"

void input_initialize(framebuffer_t* framebuffer);
void input_irq1_handler(void);
void input_irq12_handler(void);

#endif

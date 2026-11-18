#ifndef VONORIT_IDT_H
#define VONORIT_IDT_H

#include "framebuffer.h"

void idt_initialize(framebuffer_t* framebuffer);
void idt_exception_dispatch(uint64_t* frame);

#endif

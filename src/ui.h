#ifndef VONORIT_UI_H
#define VONORIT_UI_H

#include "framebuffer.h"

typedef enum
{
	UI_THEME_CHICAGO = 0,
	UI_THEME_WHISTLER = 1,
	UI_THEME_LONGHORN = 2
} ui_theme_t;

void ui_initialize(framebuffer_t* framebuffer);
void ui_set_theme(ui_theme_t theme);
ui_theme_t ui_get_theme(void);
void ui_draw(void);
void ui_redraw(void);
int ui_start_contains(int x, int y);
int ui_start_menu_hit(int x, int y);
int ui_theme_menu_hit(int x, int y);
void ui_handle_click(int x, int y);
void ui_terminal_append(char character);
void ui_terminal_clear(void);
void ui_set_uptime(uint64_t seconds);
void ui_set_datetime(uint8_t hour, uint8_t minute, uint8_t second);
void ui_update_mouse(int x, int y, int left_button, int left_pressed);
void ui_update_clock(void);
void ui_present(void);
void ui_open_notepad(const char* name);
void ui_notepad_save(void);
void ui_notepad_append(char character);
void ui_notepad_backspace(void);
int ui_notepad_is_active(void);
void ui_open_winver(void);
void ui_open_user_window(const char* title, const char* body);
int ui_winver_is_active(void);
void ui_render_cursor(int x, int y);
void ui_restore_cursor(int x, int y);
void ui_put_pixel(framebuffer_t* framebuffer, uint32_t x, uint32_t y,
	uint8_t red, uint8_t green, uint8_t blue);
void ui_fill_rect(framebuffer_t* framebuffer, uint32_t x, uint32_t y,
	uint32_t width, uint32_t height, uint8_t red, uint8_t green, uint8_t blue);
void ui_draw_text(framebuffer_t* framebuffer, uint32_t x, uint32_t y,
	const char* text, uint8_t scale, uint8_t red, uint8_t green, uint8_t blue);

#endif

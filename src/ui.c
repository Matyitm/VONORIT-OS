#include <stdint.h>
#include <stddef.h>

#include "ui.h"
#include "vfs.h"

static uint8_t backbuffer[800u*600u*3u];
static framebuffer_t* ui_fb;
static char notepad_text[VFS_CONTENT_SIZE];
static char notepad_name[VFS_MAX_NAME];
static uint16_t notepad_length;
static ui_theme_t ui_theme = UI_THEME_CHICAGO;
static int start_open;
static char terminal_text[512];
static uint16_t terminal_len;
static char ui_clock_text[12]="00:00:00";
static uint64_t ui_clock_seconds;
static uint32_t cursor_under[16u*16u];
static int cursor_saved = 0;
static int cursor_x;
static int cursor_y;

#define UI_WINDOW_TERMINAL 0
#define UI_WINDOW_FILES 1
#define UI_WINDOW_ABOUT 2
#define UI_WINDOW_NOTEPAD 3
#define UI_WINDOW_COUNT 4

typedef struct
{
	int x;
	int y;
	int width;
	int height;
	int visible;
	int minimized;
	int z_order;
	const char* title;
	const char* body;
} ui_window_t;

static ui_window_t ui_windows[UI_WINDOW_COUNT];
static int ui_active_window=-1;
static int ui_next_z=4;
static int ui_drag_window=-1;
static int ui_drag_offset_x;
static int ui_drag_offset_y;

static void invalidate_cursor(void)
{
	cursor_saved=0;
}

void ui_put_pixel(framebuffer_t* fb, uint32_t x, uint32_t y,
	uint8_t red, uint8_t green, uint8_t blue)
{
	if (fb == NULL || x >= fb->width || y >= fb->height || x>=800u || y>=600u) return;
	uint32_t offset=(y*800u+x)*3u;
	backbuffer[offset]=(uint8_t)blue;
	backbuffer[offset+1u]=(uint8_t)green;
	backbuffer[offset+2u]=(uint8_t)red;
	(void)fb;
}

void ui_present(void)
{
	if(ui_fb==NULL) return;
	uint64_t flags;
	__asm__ volatile("pushfq\npopq %0":"=r"(flags));
	__asm__ volatile("cli");
	uint32_t row_bytes=ui_fb->width*(ui_fb->bits_per_pixel/8u);
	for(uint32_t y=0;y<ui_fb->height && y<600u;++y)
		for(uint32_t x=0;x<ui_fb->width && x<800u;++x)
		{
			uint32_t src=(y*800u+x)*3u;
			volatile uint8_t* dst=ui_fb->address+y*ui_fb->pitch+x*(ui_fb->bits_per_pixel/8u);
			if(ui_fb->bits_per_pixel==24u)
			{
				dst[0]=backbuffer[src];dst[1]=backbuffer[src+1u];dst[2]=backbuffer[src+2u];
			}
			else
			{
				uint32_t pixel=((uint32_t)backbuffer[src+2u]<<ui_fb->red_position)|
					((uint32_t)backbuffer[src+1u]<<ui_fb->green_position)|
					((uint32_t)backbuffer[src]<<ui_fb->blue_position);
				*(volatile uint32_t*)dst=pixel;
			}
		}
	(void)row_bytes;
	if((flags&0x200u)!=0u) __asm__ volatile("sti");
}

void ui_fill_rect(framebuffer_t* fb, uint32_t x, uint32_t y,
	uint32_t width, uint32_t height, uint8_t red, uint8_t green, uint8_t blue)
{
	for (uint32_t row = 0; row < height; ++row)
		for (uint32_t column = 0; column < width; ++column)
			ui_put_pixel(fb, x + column, y + row, red, green, blue);
}

static void border(framebuffer_t* fb, uint32_t x, uint32_t y, uint32_t w, uint32_t h, int raised)
{
	uint8_t t1 = raised ? 255u : 128u, t2 = raised ? 128u : 255u;
	ui_fill_rect(fb, x, y, w, 1u, t1, t1, t1);
	ui_fill_rect(fb, x, y, 1u, h, t1, t1, t1);
	ui_fill_rect(fb, x, y + h - 1u, w, 1u, t2, t2, t2);
	ui_fill_rect(fb, x + w - 1u, y, 1u, h, t2, t2, t2);
}

static const uint8_t* glyph(char character)
{
	static const uint8_t blank[5] = {0,0,0,0,0};
	static const uint8_t glyphs[128][5] = {
		['A']={0x1F,0x11,0x11,0x1F,0x11},
		['B']={0x1E,0x11,0x1E,0x11,0x1E},
		['C']={0x0F,0x10,0x10,0x10,0x0F},
		['D']={0x1E,0x11,0x11,0x11,0x1E},
		['E']={0x1F,0x10,0x1E,0x10,0x1F},
		['F']={0x1F,0x10,0x1E,0x10,0x10},
		['G']={0x0F,0x10,0x17,0x11,0x0F},
		['H']={0x11,0x11,0x1F,0x11,0x11},
		['I']={0x1F,0x04,0x04,0x04,0x1F},
		['J']={0x01,0x01,0x01,0x11,0x0E},
		['K']={0x11,0x12,0x1C,0x12,0x11},
		['L']={0x10,0x10,0x10,0x10,0x1F},
		['M']={0x11,0x1B,0x15,0x11,0x11},
		['N']={0x11,0x19,0x15,0x13,0x11},
		['O']={0x0E,0x11,0x11,0x11,0x0E},
		['P']={0x1E,0x11,0x1E,0x10,0x10},
		['Q']={0x0E,0x11,0x11,0x15,0x0F},
		['R']={0x1E,0x11,0x1E,0x12,0x11},
		['S']={0x0F,0x10,0x0E,0x01,0x1E},
		['T']={0x1F,0x04,0x04,0x04,0x04},
		['U']={0x11,0x11,0x11,0x11,0x0E},
		['V']={0x11,0x11,0x11,0x0A,0x04},
		['W']={0x11,0x11,0x15,0x15,0x0A},
		['X']={0x11,0x0A,0x04,0x0A,0x11},
		['Y']={0x11,0x0A,0x04,0x04,0x04},
		['Z']={0x1F,0x02,0x04,0x08,0x1F},
		['0']={0x0E,0x11,0x11,0x11,0x0E},
		['1']={0x04,0x0C,0x04,0x04,0x0E},
		['2']={0x0E,0x11,0x02,0x04,0x1F},
		['3']={0x1E,0x01,0x06,0x01,0x1E},
		['4']={0x02,0x06,0x0A,0x1F,0x02},
		['5']={0x1F,0x10,0x1E,0x01,0x1E},
		['6']={0x06,0x08,0x1E,0x11,0x0E},
		['7']={0x1F,0x01,0x02,0x04,0x04},
		['8']={0x0E,0x11,0x0E,0x11,0x0E},
		['9']={0x0E,0x11,0x0F,0x01,0x0E},
		[':']={0,0x04,0,0x04,0},
		['.']={0,0,0,0,0x04},
		['-']={0,0,0x1F,0,0},
		['_']={0,0,0,0,0x1F},
		['/']={0x01,0x02,0x04,0x08,0x10},
		['>']={0x10,0x08,0x04,0x08,0x10},
		['[']={0x0E,0x08,0x08,0x08,0x0E},
		[']']={0x0E,0x02,0x02,0x02,0x0E},
		['(']={0x02,0x04,0x08,0x04,0x02},
		[')']={0x08,0x04,0x02,0x04,0x08},
		['!']={0x04,0x04,0x04,0,0x04},
		['?']={0x0E,0x11,0x02,0,0x04},
		[' ']={0,0,0,0,0}
	};
	if(character >= 'a' && character <= 'z') character=(char)(character-'a'+'A');
	unsigned char glyph_index=(unsigned char)character;
	if(glyph_index < 128u) return glyphs[glyph_index];
	return blank;
}

void ui_draw_text(framebuffer_t* fb, uint32_t x, uint32_t y,
	const char* text, uint8_t scale, uint8_t red, uint8_t green, uint8_t blue)
{
	uint32_t origin=x;
	for (size_t i=0;text[i]!=0;++i)
	{
		if(text[i]=='\n'){x=origin;y+=8u*scale;continue;}
		const uint8_t* bitmap=glyph(text[i]);
		for(uint32_t r=0;r<5;++r) for(uint32_t c=0;c<5;++c)
			if(bitmap[r]&(1u<<(4u-c))) ui_fill_rect(fb,x+c*scale,y+r*scale,scale,scale,red,green,blue);
		x+=6u*scale;
	}
}

static void title_gradient(uint32_t x, uint32_t y, uint32_t w, int active)
{
	for(uint32_t c=0;c<w;++c)
	{
		uint8_t red,green,blue;
		if(ui_theme==UI_THEME_CHICAGO){red=active?(uint8_t)(18+c/8):128u;green=active?(uint8_t)(56+c/5):128u;blue=active?170u:128u;}
		else if(ui_theme==UI_THEME_WHISTLER){red=active?(uint8_t)(20+c/12):170u;green=active?(uint8_t)(80+c/5):190u;blue=active?220u:215u;}
		else {red=active?(uint8_t)(35+c/10):75u;green=active?(uint8_t)(65+c/8):95u;blue=active?(uint8_t)(145+c/5):135u;}
		ui_fill_rect(ui_fb,x+c,y,1u,26u,red,green,blue);
	}
}

static void draw_window(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
	const char* title, const char* body, int active)
{
	if(w<100u) w=100u;
	ui_fill_rect(ui_fb,x+4u,y+4u,w,h,45u,50u,55u);
	uint8_t wr=192u,wg=192u,wb=192u;
	if(ui_theme==UI_THEME_WHISTLER){wr=235u;wg=240u;wb=248u;}
	if(ui_theme==UI_THEME_LONGHORN){wr=210u;wg=220u;wb=232u;}
	ui_fill_rect(ui_fb,x,y,w,h,wr,wg,wb);
	border(ui_fb,x,y,w,h,1);
	title_gradient(x+3u,y+3u,w-6u,active);
	ui_draw_text(ui_fb,x+10u,y+8u,title,2u,255u,255u,255u);
	uint32_t button_y=y+6u;
	for(uint32_t i=0;i<3;++i)
	{
		uint32_t bx=x+w-70u+i*20u;
		ui_fill_rect(ui_fb,bx,button_y,16u,16u,192u,192u,192u);
		border(ui_fb,bx,button_y,16u,16u,1);
		if(i==0) ui_draw_text(ui_fb,bx+5u,button_y+5u,"_",1u,20u,20u,20u);
		if(i==1) ui_fill_rect(ui_fb,bx+4u,button_y+4u,8u,8u,40u,40u,40u);
		if(i==2) ui_draw_text(ui_fb,bx+5u,button_y+4u,"X",1u,30u,30u,30u);
	}
	border(ui_fb,x+10u,y+34u,w-20u,h-44u,0);
	ui_draw_text(ui_fb,x+18u,y+48u,body,2u,25u,35u,55u);
}

static void draw_taskbar(void)
{
	uint32_t task_h=42u;
	uint32_t y=ui_fb->height-task_h;
	uint8_t r=192u,g=192u,b=192u;
	if(ui_theme==UI_THEME_WHISTLER){r=218u;g=232u;b=246u;}
	if(ui_theme==UI_THEME_LONGHORN){r=44u;g=74u;b=112u;b=150u;}
	ui_fill_rect(ui_fb,0u,y,ui_fb->width,task_h,r,g,b);
	border(ui_fb,0u,y,ui_fb->width,task_h,1);

	uint32_t start_w=82u;
	ui_fill_rect(ui_fb,7u,y+7u,start_w,28u,30u,135u,70u);
	border(ui_fb,7u,y+7u,start_w,28u,1);
	ui_draw_text(ui_fb,23u,y+13u,"START",2u,255u,255u,255u);

	for(int index=0;index<UI_WINDOW_COUNT;++index)
	{
		uint32_t task_x=100u+(uint32_t)index*150u;
		uint8_t r=220u,g=220u,b=220u;
		if(ui_windows[index].visible && ui_active_window==index){r=240u;g=240u;b=240u;}
		ui_fill_rect(ui_fb,task_x,y+7u,150u,28u,r,g,b);
		border(ui_fb,task_x,y+7u,150u,28u,0);
		ui_draw_text(ui_fb,task_x+10u,y+13u,ui_windows[index].title,2u,20u,30u,45u);
	}

	uint32_t tray_w=130u;
	ui_fill_rect(ui_fb,ui_fb->width-tray_w-8u,y+7u,tray_w,28u,155u,175u,195u);
	border(ui_fb,ui_fb->width-tray_w-8u,y+7u,tray_w,28u,0);
	ui_draw_text(ui_fb,ui_fb->width-tray_w+18u,y+13u,ui_clock_text,2u,20u,30u,45u);
}

static void draw_start_menu(void)
{
	if(!start_open) return;
	uint32_t y=ui_fb->height-42u-250u;
	uint32_t x=6u,w=220u,h=250u;
	ui_fill_rect(ui_fb,x,y,w,h,210u,220u,230u);
	border(ui_fb,x,y,w,h,1);
	ui_fill_rect(ui_fb,x,y,36u,h,30u,80u,150u);
	ui_draw_text(ui_fb,x+48u,y+18u,"PROGRAMS",2u,20u,30u,50u);
	ui_draw_text(ui_fb,x+48u,y+48u,"TERMINAL",2u,20u,30u,50u);
	ui_draw_text(ui_fb,x+48u,y+78u,"FILES",2u,20u,30u,50u);
	ui_draw_text(ui_fb,x+48u,y+108u,"ABOUT",2u,20u,30u,50u);
	ui_draw_text(ui_fb,x+48u,y+138u,"NOTEPAD",2u,20u,30u,50u);
	ui_draw_text(ui_fb,x+48u,y+168u,"THEME: CHICAGO",2u,20u,30u,50u);
	ui_draw_text(ui_fb,x+48u,y+198u,"THEME: WHISTLER",2u,20u,30u,50u);
	ui_draw_text(ui_fb,x+48u,y+228u,"THEME: LONGHORN",2u,20u,30u,50u);
}

static void draw_watermark(void)
{
	uint32_t right=ui_fb->width-8u;
	uint32_t line1_width=30u*6u*2u;
	uint32_t x=line1_width<right?right-line1_width:4u;
	uint32_t y=ui_fb->height-82u;
	ui_draw_text(ui_fb,x,y,"VONORIT OS HYBRID EDITION",1u,210u,220u,235u);
	ui_draw_text(ui_fb,x,y+10u,"Evaluation copy. Build 2026.x86_64",1u,210u,220u,235u);
}

static int ui_cursor_overlaps(uint32_t x,uint32_t y,uint32_t width,uint32_t height)
{
	if(!cursor_saved) return 0;
	int left=(int)x;
	int top=(int)y;
	int right=(int)(x+width);
	int bottom=(int)(y+height);
	int cursor_right=cursor_x+16;
	int cursor_bottom=cursor_y+16;
	return cursor_right>=left && cursor_x<=right && cursor_bottom>=top && cursor_y<=bottom;
}

void ui_set_uptime(uint64_t seconds)
{
	ui_clock_seconds=seconds;
}

void ui_set_datetime(uint8_t hour, uint8_t minute, uint8_t second)
{
	ui_clock_text[0]=(char)('0'+hour/10u);
	ui_clock_text[1]=(char)('0'+hour%10u);
	ui_clock_text[2]=':';
	ui_clock_text[3]=(char)('0'+minute/10u);
	ui_clock_text[4]=(char)('0'+minute%10u);
	ui_clock_text[5]=':';
	ui_clock_text[6]=(char)('0'+second/10u);
	ui_clock_text[7]=(char)('0'+second%10u);
	ui_clock_text[8]=0;
}

static void ui_bring_to_front(int index)
{
	ui_active_window=index;
	ui_windows[index].z_order=ui_next_z++;
}

static int ui_window_at(int x,int y)
{
	for(int z=ui_next_z-1;z>=1;--z)
	{
		for(int index=0;index<UI_WINDOW_COUNT;++index)
		{
			ui_window_t* window=&ui_windows[index];
			if(window->visible && !window->minimized && window->z_order==z &&
				x>=window->x && x<window->x+window->width &&
				y>=window->y && y<window->y+window->height)
				return index;
		}
	}
	return -1;
}

static void ui_restore_window(int index)
{
	ui_windows[index].visible=1;
	ui_windows[index].minimized=0;
	ui_bring_to_front(index);
	invalidate_cursor();
	ui_redraw();
}

static void ui_close_window(int index)
{
	ui_windows[index].visible=0;
	ui_windows[index].minimized=0;
	if(ui_active_window==index) ui_active_window=-1;
	invalidate_cursor();
	ui_redraw();
}

static void ui_minimize_window(int index)
{
	ui_windows[index].minimized=1;
	if(ui_active_window==index) ui_active_window=-1;
	invalidate_cursor();
	ui_redraw();
}

void ui_open_notepad(const char* name)
{
	const char* content=vfs_read(name);
	uint16_t i=0u;
	while(content[i]!=0 && i<VFS_CONTENT_SIZE-1u){notepad_text[i]=content[i];++i;}
	notepad_text[i]=0;
	notepad_length=i;
	uint16_t n=0u;
	while(name[n]!=0 && n<VFS_MAX_NAME-1u){notepad_name[n]=name[n];++n;}
	notepad_name[n]=0;
	ui_windows[UI_WINDOW_NOTEPAD].visible=1;
	ui_windows[UI_WINDOW_NOTEPAD].minimized=0;
	ui_bring_to_front(UI_WINDOW_NOTEPAD);
	ui_redraw();
}

void ui_notepad_save(void)
{
	if(notepad_name[0]!=0) (void)vfs_write(notepad_name,notepad_text);
}

void ui_notepad_append(char character)
{
	if(!ui_notepad_is_active()) return;
	if(character=='\n') character=' ';
	if(notepad_length<VFS_CONTENT_SIZE-1u)
	{
		notepad_text[notepad_length++]=character;
		notepad_text[notepad_length]=0;
		ui_redraw();
	}
}

void ui_notepad_backspace(void)
{
	if(!ui_notepad_is_active()) return;
	if(notepad_length>0u){--notepad_length;notepad_text[notepad_length]=0;}
	ui_redraw();
}

int ui_notepad_is_active(void)
{
	return ui_active_window==UI_WINDOW_NOTEPAD && ui_windows[UI_WINDOW_NOTEPAD].visible!=0;
}

void ui_update_mouse(int x,int y,int left_button,int left_pressed)
{
	if(left_pressed)
	{
		ui_window_t* save_window=&ui_windows[UI_WINDOW_NOTEPAD];
		if(save_window->visible && !save_window->minimized && x>=save_window->x+save_window->width-80 && x<save_window->x+save_window->width-20 && y>=save_window->y+34 && y<save_window->y+50)
		{
			ui_notepad_save();
			return;
		}
		ui_window_t* files_window=&ui_windows[UI_WINDOW_FILES];
		if(files_window->visible && !files_window->minimized && x>=files_window->x && x<files_window->x+files_window->width && y>=files_window->y+64 && y<files_window->y+64+(int)(vfs_count()*16u))
		{
			uint8_t file_index=(uint8_t)((y-files_window->y-64)/16);
			const vfs_file_t* file=vfs_file(file_index);
			if(file!=0){ui_open_notepad(file->name);return;}
		}
		int index=ui_window_at(x,y);
		if(index>=0)
		{
			ui_window_t* window=&ui_windows[index];
			ui_bring_to_front(index);
			uint32_t title_height=32u;
			if(y>=window->y && y<window->y+(int)title_height)
			{
				int minimize_x=window->x+window->width-70;
				int close_x=window->x+window->width-30;
				if(x>=minimize_x && x<minimize_x+16 && y>=window->y+6 && y<window->y+22)
					ui_minimize_window(index);
				else if(x>=close_x && x<close_x+16 && y>=window->y+6 && y<window->y+22)
					ui_close_window(index);
				else
				{
					ui_drag_window=index;
					ui_drag_offset_x=x-window->x;
					ui_drag_offset_y=y-window->y;
					invalidate_cursor();
					ui_redraw();
				}
				return;
			}
			invalidate_cursor();
			ui_redraw();
		}
	}
	else if(left_button && ui_drag_window>=0)
	{
		ui_window_t* window=&ui_windows[ui_drag_window];
		window->x=x-ui_drag_offset_x;
		window->y=y-ui_drag_offset_y;
		if(window->x<0) window->x=0;
		if(window->y<0) window->y=0;
		if(window->x+window->width>(int)ui_fb->width) window->x=(int)ui_fb->width-window->width;
		if(window->y+window->height>(int)ui_fb->height-42) window->y=(int)ui_fb->height-42-window->height;
		if(window->x<0) window->x=0;
		if(window->y<0) window->y=0;
		invalidate_cursor();
		ui_redraw();
	}
	else if(!left_button)
	{
		ui_drag_window=-1;
	}
}

static void ui_draw_window_state(ui_window_t* window)
{
	if(!window->visible || window->minimized) return;
	int active=ui_active_window==(int)(window-ui_windows);
	if(window==&ui_windows[UI_WINDOW_NOTEPAD])
	{
		draw_window((uint32_t)window->x,(uint32_t)window->y,(uint32_t)window->width,
			(uint32_t)window->height,window->title,"",active);
		ui_fill_rect(ui_fb,(uint32_t)window->x+10u,(uint32_t)window->y+34u,
			(uint32_t)window->width-20u,16u,220u,220u,220u);
		ui_draw_text(ui_fb,(uint32_t)window->x+window->width-80u,(uint32_t)window->y+38u,"[SAVE]",1u,20u,20u,20u);
		ui_draw_text(ui_fb,(uint32_t)window->x+18u,(uint32_t)window->y+58u,notepad_text,1u,25u,35u,55u);
		return;
	}
	if(window==&ui_windows[UI_WINDOW_FILES])
	{
		draw_window((uint32_t)window->x,(uint32_t)window->y,(uint32_t)window->width,
			(uint32_t)window->height,window->title,"",active);
		uint32_t y=(uint32_t)window->y+48u;
		ui_draw_text(ui_fb,(uint32_t)window->x+18u,y,"HOME DOCUMENTS PROGRAMS",1u,25u,35u,55u);
		for(uint8_t index=0u;index<vfs_count();++index)
		{
			const vfs_file_t* file=vfs_file(index);
			if(file!=0) ui_draw_text(ui_fb,(uint32_t)window->x+18u,y+16u*(index+1u),file->name,1u,25u,35u,55u);
		}
		return;
	}
	draw_window((uint32_t)window->x,(uint32_t)window->y,(uint32_t)window->width,
		(uint32_t)window->height,window->title,window->body,active);
}

static void ui_initialize_window_states(void)
{
	uint32_t margin=ui_fb->width>=800u?24u:8u;
	uint32_t gap=16u;
	uint32_t window_width=(ui_fb->width-2u*margin-2u*gap)/3u;
	if(window_width>280u) window_width=280u;
	uint32_t window_y=88u;
	uint32_t window_h=(ui_theme==UI_THEME_LONGHORN)?200u:205u;
	const char* titles[4]={"TERMINAL","FILES","ABOUT","NOTEPAD"};
	const char* bodies[4]={terminal_text[0]?terminal_text:"VONORIT SHELL\nREADY","HOME\nDOCUMENTS\nPROGRAMS","VONORIT OS\nHYBRID DESKTOP",""};
	for(int index=0;index<UI_WINDOW_COUNT;++index)
	{
		ui_windows[index].x=(int)(index==UI_WINDOW_NOTEPAD?margin+gap:margin+(window_width+gap)*index);
		ui_windows[index].y=(int)window_y;
		ui_windows[index].width=(int)window_width;
		ui_windows[index].height=(int)window_h;
		ui_windows[index].visible=(index==UI_WINDOW_NOTEPAD)?0:1;
		ui_windows[index].minimized=0;
		ui_windows[index].z_order=index+1;
		ui_windows[index].title=titles[index];
		ui_windows[index].body=bodies[index];
	}
	ui_active_window=UI_WINDOW_TERMINAL;
	ui_next_z=5;
}

static void ui_draw_window_states(void)
{
	for(int z=1;z<ui_next_z;++z)
		for(int index=0;index<UI_WINDOW_COUNT;++index)
			if(ui_windows[index].z_order==z) ui_draw_window_state(&ui_windows[index]);
}

static uint64_t ui_save_flags(void);
static void ui_restore_flags(uint64_t flags);

void ui_update_clock(void)
{
	if(ui_fb==NULL) return;
	uint64_t flags=ui_save_flags();

	uint32_t task_y=ui_fb->height-42u;
	uint32_t tray_w=130u;
	uint32_t tray_x=ui_fb->width-tray_w-8u;
	uint32_t tray_y=task_y+7u;

	int task_overlap=ui_cursor_overlaps(tray_x,tray_y,tray_w,28u);
	if(task_overlap && cursor_saved) ui_restore_cursor(cursor_x,cursor_y);

	ui_fill_rect(ui_fb,tray_x,tray_y,tray_w,28u,155u,175u,195u);
	border(ui_fb,tray_x,tray_y,tray_w,28u,0);
	ui_draw_text(ui_fb,tray_x+18u,tray_y+6u,ui_clock_text,2u,20u,30u,45u);

	if(ui_theme==UI_THEME_LONGHORN)
	{
		uint32_t sidebar=170u;
		uint32_t sidebar_x=ui_fb->width-sidebar;
		uint32_t clock_y=108u;
		int sidebar_overlap=ui_cursor_overlaps(sidebar_x+18u,clock_y,120u,10u);
		if(sidebar_overlap && cursor_saved) ui_restore_cursor(cursor_x,cursor_y);
		ui_fill_rect(ui_fb,sidebar_x+18u,clock_y,120u,20u,35u,55u,85u);
		ui_draw_text(ui_fb,sidebar_x+18u,clock_y,ui_clock_text,2u,240u,240u,255u);
		if(sidebar_overlap) ui_render_cursor(cursor_x,cursor_y);
	}

	if(task_overlap) ui_render_cursor(cursor_x,cursor_y);
	ui_restore_flags(flags);
	ui_present();
}

void ui_initialize(framebuffer_t* framebuffer)
{
	ui_fb=framebuffer;
	start_open=0;
	terminal_len=0;
	terminal_text[0]=0;
	ui_initialize_window_states();
	ui_draw();
}

void ui_sync_window_theme_layout(void)
{
	ui_initialize_window_states();
}

void ui_set_theme(ui_theme_t theme){invalidate_cursor();ui_theme=theme;ui_redraw();}
ui_theme_t ui_get_theme(void){return ui_theme;}

void ui_draw(void)
{
	invalidate_cursor();
	if(ui_fb==NULL) return;
	if(ui_theme==UI_THEME_CHICAGO) ui_fill_rect(ui_fb,0u,0u,ui_fb->width,ui_fb->height-42u,0u,128u,128u);
	else if(ui_theme==UI_THEME_WHISTLER) ui_fill_rect(ui_fb,0u,0u,ui_fb->width,ui_fb->height-42u,55u,155u,225u);
	else
	{
		ui_fill_rect(ui_fb,0u,0u,ui_fb->width,ui_fb->height-42u,20u,43u,73u);
		uint32_t sidebar=170u;
		ui_fill_rect(ui_fb,ui_fb->width-sidebar,56u,sidebar,ui_fb->height-98u,35u,55u,85u);
		border(ui_fb,ui_fb->width-sidebar,56u,sidebar,ui_fb->height-98u,1);
		ui_draw_text(ui_fb,ui_fb->width-sidebar+18u,76u,"SYSTEM",2u,220u,235u,255u);
		ui_draw_text(ui_fb,ui_fb->width-sidebar+18u,108u,ui_clock_text,2u,240u,240u,255u);
	}

	ui_draw_window_states();
	draw_watermark();
	draw_taskbar();
	draw_start_menu();
}

void ui_redraw(void){ui_draw();ui_present();}

int ui_start_contains(int x,int y){uint32_t t=ui_fb->height-42u;return x>=7&&x<=89&&y>=((int)t+7)&&y<=((int)t+35);}
int ui_start_menu_hit(int x,int y){return start_open && x>=6 && x<=226 && y>=((int)ui_fb->height-292) && y<=((int)ui_fb->height-42);}
int ui_theme_menu_hit(int x,int y){
	if(!start_open) return 0;
	uint32_t ybase=ui_fb->height-292u;
	if(y<(int)ybase+130||y>(int)ybase+230||x<54||x>220) return 0;
	return 1;
}
void ui_handle_click(int x,int y)
{
	invalidate_cursor();
	if(ui_start_contains(x,y)){start_open=!start_open;ui_redraw();return;}
	uint32_t task_y=ui_fb->height-42u;
	if(y>=((int)task_y+7) && y<=((int)task_y+35))
	{
		for(int index=0;index<UI_WINDOW_COUNT;++index)
		{
			uint32_t task_x=100u+(uint32_t)index*150u;
			if(x>=((int)task_x)&&x<((int)task_x+150))
			{
				ui_restore_window(index);
				start_open=0;
				return;
			}
		}
	}
	if(start_open)
	{
		uint32_t base=ui_fb->height-292u;
		if(y>=((int)base+48)&&y<=((int)base+68)){ui_restore_window(UI_WINDOW_TERMINAL);start_open=0;return;}
		if(y>=((int)base+78)&&y<=((int)base+98)){ui_restore_window(UI_WINDOW_FILES);start_open=0;return;}
		if(y>=((int)base+108)&&y<=((int)base+128)){ui_restore_window(UI_WINDOW_ABOUT);start_open=0;return;}
		if(y>=((int)base+138)&&y<=((int)base+158)){ui_open_notepad("NOTES.TXT");start_open=0;return;}
		if(y>=((int)base+168)&&y<=((int)base+188)){ui_set_theme(UI_THEME_CHICAGO);start_open=0;return;}
		if(y>=((int)base+198)&&y<=((int)base+218)){ui_set_theme(UI_THEME_WHISTLER);start_open=0;return;}
		if(y>=((int)base+228)&&y<=((int)base+248)){ui_set_theme(UI_THEME_LONGHORN);start_open=0;return;}
		start_open=0;ui_redraw();
	}
}

void ui_terminal_append(char character)
{
	invalidate_cursor();
	if(character=='\n' && terminal_len<sizeof(terminal_text)-1u)
		terminal_text[terminal_len++]='\n';
	else if(character!='\n' && terminal_len<sizeof(terminal_text)-1u)
		terminal_text[terminal_len++]=character;
	terminal_text[terminal_len]=0;
	ui_redraw();
}

void ui_terminal_clear(void){invalidate_cursor();terminal_len=0;terminal_text[0]=0;ui_redraw();}

static uint64_t ui_save_flags(void)
{
	uint64_t flags;
	__asm__ volatile("pushfq\npopq %0":"=r"(flags));
	__asm__ volatile("cli");
	return flags;
}

static void ui_restore_flags(uint64_t flags)
{
	if((flags&0x200u)!=0u) __asm__ volatile("sti");
}

void ui_restore_cursor(int x,int y)
{
	uint64_t flags=ui_save_flags();
	if(!cursor_saved || ui_fb==NULL)
	{
		ui_restore_flags(flags);
		return;
	}
	for(int dy=0;dy<16;++dy)
	{
		for(int dx=0;dx<16;++dx)
		{
			int px=x+dx;
			int py=y+dy;
			if(px<0||py<0||px>=((int)ui_fb->width)||py>=((int)ui_fb->height)) continue;
			volatile uint8_t* dst=ui_fb->address+(uint32_t)py*ui_fb->pitch+(uint32_t)px*(ui_fb->bits_per_pixel/8u);
			uint32_t pixel=cursor_under[dy*16u+(uint32_t)dx];
			if(ui_fb->bits_per_pixel==24u)
			{
				dst[0]=(uint8_t)pixel;
				dst[1]=(uint8_t)(pixel>>8);
				dst[2]=(uint8_t)(pixel>>16);
			}
			else *(volatile uint32_t*)dst=pixel;
		}
	}
	cursor_saved=0;
	ui_restore_flags(flags);
}

void ui_render_cursor(int x,int y)
{
	uint64_t flags=ui_save_flags();
	if(ui_fb==NULL)
	{
		ui_restore_flags(flags);
		return;
	}
	cursor_x=x;
	cursor_y=y;
	for(int dy=0;dy<16;++dy)
	{
		for(int dx=0;dx<16;++dx)
		{
			int px=x+dx;
			int py=y+dy;
			if(px<0||py<0||px>=((int)ui_fb->width)||py>=((int)ui_fb->height))
			{
				cursor_under[dy*16u+(uint32_t)dx]=0u;
				continue;
			}
			volatile uint8_t* source=ui_fb->address+(uint32_t)py*ui_fb->pitch+(uint32_t)px*(ui_fb->bits_per_pixel/8u);
			if(ui_fb->bits_per_pixel==24u)
				cursor_under[dy*16u+(uint32_t)dx]=(uint32_t)source[0]|((uint32_t)source[1]<<8)|((uint32_t)source[2]<<16);
			else cursor_under[dy*16u+(uint32_t)dx]=*(volatile uint32_t*)source;
		}
	}
	cursor_saved=1;

	for(int dy=0;dy<16;++dy)
	{
		for(int dx=0;dx<16;++dx)
		{
			int px=x+dx;
			int py=y+dy;
			if(px<0||py<0||px>=((int)ui_fb->width)||py>=((int)ui_fb->height)) continue;
			int inside=(dx==0 && dy<=14);
			if(dx>=1 && dx<=dy && dy<=12) inside=1;
			if(dx>dy && dx<=dy+2 && dy<=12) inside=1;
			if(dy>12 && dx>=0 && dx<=dy-11) inside=1;
			if(inside)
			{
				volatile uint8_t* dst=ui_fb->address+(uint32_t)py*ui_fb->pitch+(uint32_t)px*(ui_fb->bits_per_pixel/8u);
				if(ui_fb->bits_per_pixel==24u){dst[0]=255u;dst[1]=255u;dst[2]=255u;}
				else *(volatile uint32_t*)dst=0x00FFFFFFu;
			}
		}
	}
	ui_restore_flags(flags);
}

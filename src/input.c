#include <stdint.h>

#include "input.h"
#include "ui.h"
#include "vfs.h"

static uint8_t keyboard_ctrl;
static char terminal_command[128];
static uint16_t terminal_command_length;

static void terminal_append_text(const char* text)
{
	while(*text!=0){ui_terminal_append(*text++);}
}

static uint8_t terminal_starts(const char* left,const char* right)
{
	uint16_t i=0u;
	while(right[i]!=0)
	{
		if(left[i]==0 || left[i]!=right[i]) return 0u;
		++i;
	}
	return 1u;
}

static void terminal_print_vfs(void)
{
	for(uint8_t i=0u;i<16u;++i)
	{
		const vfs_file_t* file=vfs_file(i);
		if(file!=0)
		{
			ui_terminal_append('\n');terminal_append_text(file->name);
		}
	}
}

static void terminal_parse_command(void)
{
	if(terminal_starts(terminal_command,"ls") || terminal_starts(terminal_command,"dir"))
	{
		terminal_print_vfs();
	}
	else if(terminal_starts(terminal_command,"cat "))
	{
		char name[32];uint16_t i=4u,j=0u;
		while(terminal_command[i]!=0 && j<31u) name[j++]=terminal_command[i++];
		name[j]=0;terminal_append_text("\n");terminal_append_text(vfs_read(name));
	}
	else if(terminal_starts(terminal_command,"write "))
	{
		char name[32];uint16_t i=6u,j=0u;
		while(terminal_command[i]!=0 && terminal_command[i]!=' ' && j<31u) name[j++]=terminal_command[i++];
		name[j]=0;if(terminal_command[i]==' ') ++i;
		char text[1024];j=0u;
		while(terminal_command[i]!=0 && j<1023u) text[j++]=terminal_command[i++];
		text[j]=0;(void)vfs_write(name,text);
	}
	else if(terminal_starts(terminal_command,"notepad"))
	{
		ui_open_notepad("NOTES.TXT");
	}
}

static framebuffer_t* input_fb;
static int mouse_x=400,mouse_y=300;
static uint8_t mouse_packet[4];
static uint8_t mouse_index;
static int mouse_buttons;
static uint8_t mouse_previous_left;
static uint8_t mouse_left_pressed;
static uint8_t mouse_left_released;
static uint8_t keyboard_shift;
static uint8_t keyboard_caps_lock;

static char to_lower_ascii(char character)
{
	if(character>= 'A' && character<= 'Z') return (char)(character-'A'+'a');
	return character;
}

static int command_equals(const char* left, const char* right)
{
	uint16_t index=0;
	while(left[index]!=0 && right[index]!=0)
	{
		if(to_lower_ascii(left[index])!=to_lower_ascii(right[index])) return 0;
		++index;
	}
	return left[index]==0 && right[index]==0;
}

static void execute_command(void)
{
	terminal_command[terminal_command_length]=0;
	terminal_parse_command();
	if(terminal_starts(terminal_command,"ls") || terminal_starts(terminal_command,"dir") || terminal_starts(terminal_command,"cat ") || terminal_starts(terminal_command,"write ") || terminal_starts(terminal_command,"notepad"))
	{
		terminal_command_length=0;terminal_command[0]=0;return;
	}
	if(command_equals(terminal_command,"help"))
	{
		ui_terminal_append('\n');
		ui_terminal_append('C'); ui_terminal_append('O'); ui_terminal_append('M'); ui_terminal_append('M');
		ui_terminal_append('A'); ui_terminal_append('N'); ui_terminal_append('D'); ui_terminal_append('S');
		ui_terminal_append(':'); ui_terminal_append(' '); ui_terminal_append('H'); ui_terminal_append('E');
		ui_terminal_append('L'); ui_terminal_append('P'); ui_terminal_append(' '); ui_terminal_append('V');
		ui_terminal_append('E'); ui_terminal_append('R'); ui_terminal_append(' '); ui_terminal_append('C');
		ui_terminal_append('L'); ui_terminal_append('E'); ui_terminal_append('A'); ui_terminal_append('R');
	}
	else if(command_equals(terminal_command,"ver"))
	{
		ui_terminal_append('\n');
		ui_terminal_append('V'); ui_terminal_append('O'); ui_terminal_append('N'); ui_terminal_append('O');
		ui_terminal_append('R'); ui_terminal_append('I'); ui_terminal_append('T'); ui_terminal_append(' ');
		ui_terminal_append('O'); ui_terminal_append('S'); ui_terminal_append(' ');
		ui_terminal_append('2'); ui_terminal_append('0'); ui_terminal_append('2'); ui_terminal_append('6');
	}
	else if(command_equals(terminal_command,"clear") || command_equals(terminal_command,"cls"))
	{
		ui_terminal_clear();
	}
	else if(command_equals(terminal_command,"uptime") || command_equals(terminal_command,"time"))
	{
		ui_terminal_append('\n');
		ui_terminal_append('U'); ui_terminal_append('P'); ui_terminal_append('T'); ui_terminal_append('I'); ui_terminal_append('M'); ui_terminal_append('E');
		ui_terminal_append(' '); ui_terminal_append('C'); ui_terminal_append('O'); ui_terminal_append('U'); ui_terminal_append('N'); ui_terminal_append('T'); ui_terminal_append('E'); ui_terminal_append('R');
	}
	else if(command_equals(terminal_command,"theme chicago"))
	{
		ui_set_theme(UI_THEME_CHICAGO);
	}
	else if(command_equals(terminal_command,"theme whistler"))
	{
		ui_set_theme(UI_THEME_WHISTLER);
	}
	else if(command_equals(terminal_command,"theme longhorn"))
	{
		ui_set_theme(UI_THEME_LONGHORN);
	}
	terminal_command_length=0;
	terminal_command[0]=0;
}

static char apply_keyboard_modifiers(char character)
{
	if(character>= 'a' && character<= 'z' && (keyboard_shift!=0u || keyboard_caps_lock!=0u))
		return (char)(character-'a'+'A');
	if(keyboard_shift!=0u)
	{
		if(character=='1') return '!';
		if(character=='2') return '@';
		if(character=='3') return '#';
		if(character=='4') return '$';
		if(character=='-') return '_';
	}
	return character;
}

static void append_terminal_character(char character)
{
	if(terminal_command_length>=sizeof(terminal_command)-1u) return;
	terminal_command[terminal_command_length++]=character;
	terminal_command[terminal_command_length]=0;
	ui_terminal_append(character);
}

static inline void outb(uint16_t port,uint8_t value){__asm__ volatile("outb %0,%1"::"a"(value),"Nd"(port));}
static inline uint8_t inb(uint16_t port){uint8_t value;__asm__ volatile("inb %1,%0":"=a"(value):"Nd"(port));return value;}
static void wait_output(void){for(uint32_t i=0;i<100000u;++i){if((inb(0x64)&1u)!=0u)return;}}
static void wait_input(void){for(uint32_t i=0;i<100000u;++i){if((inb(0x64)&2u)==0u)return;}}

static char scancode_to_ascii(uint8_t scancode)
{
	static const char set[128] = {
		0,27,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t',
		'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
		'a','s','d','f','g','h','j','k','l',';','\'', '`',0,'\\',
		'z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',0
	};
	return scancode<128u?set[scancode]:0;
}

void input_irq1_handler(void)
{
	uint8_t status=inb(0x64);
	if((status&1u)==0u)return;
	uint8_t scancode=inb(0x60);
	if(scancode==0xE0u)return;
	if(scancode==0x1Du){keyboard_ctrl=1u;return;}
	if(scancode==0x9Du){keyboard_ctrl=0u;return;}
	if((scancode&0x80u)!=0u){if(scancode==0x9Du) keyboard_ctrl=0u;return;}
	if(keyboard_ctrl!=0u && scancode==0x1Fu){ui_notepad_save();return;}
	if(ui_notepad_is_active())
	{
		char character=scancode_to_ascii(scancode);
		if(character=='\b') ui_notepad_backspace();
		else if(character!=0 && character!='\n') ui_notepad_append(apply_keyboard_modifiers(character));
		return;
	}
	if(scancode==0xAAu || scancode==0xB6u){keyboard_shift=0u;return;}
	if(scancode&0x80u)return;
	if(scancode==0x2Au || scancode==0x36u){keyboard_shift=1u;return;}
	if(scancode==0x3Au){keyboard_caps_lock=(uint8_t)(keyboard_caps_lock^1u);return;}
	char character=scancode_to_ascii(scancode);
	if(character=='\n')
	{
		ui_terminal_append('\n');
		execute_command();
		ui_terminal_append('\n');
		ui_terminal_append('>'); ui_terminal_append(' ');
	}
	else if(character=='\b')
	{
		if(terminal_command_length>0u)
		{
			--terminal_command_length;
			terminal_command[terminal_command_length]=0;
		}
	}
	else if(character!=0)
	{
		append_terminal_character(apply_keyboard_modifiers(character));
	}
}

void input_irq12_handler(void)
{
	if((inb(0x64)&0x21u)!=0x21u)return;
	uint8_t packet=inb(0x60);
	if(mouse_index==0u && (packet&8u)==0u)return;
	mouse_packet[mouse_index++]=packet;
	if(mouse_index==3u)
	{
		mouse_index=0u;
		mouse_buttons=mouse_packet[0]&7u;
		uint8_t left_button=(uint8_t)(mouse_buttons&1u);
		mouse_left_pressed=(uint8_t)(left_button!=0u && mouse_previous_left==0u);
		mouse_left_released=(uint8_t)(left_button==0u && mouse_previous_left!=0u);
		mouse_previous_left=left_button;
		int dx=(int)(int8_t)mouse_packet[1];
		int dy=(int)(int8_t)mouse_packet[2];
		int prev_x=mouse_x;
		int prev_y=mouse_y;
		mouse_x+=dx;
		mouse_y-=dy;
		if(mouse_x<0) mouse_x=0;
		if(mouse_y<0) mouse_y=0;
		if(mouse_x>=((int)input_fb->width))mouse_x=(int)input_fb->width-1;
		if(mouse_y>=((int)input_fb->height))mouse_y=(int)input_fb->height-1;
		ui_restore_cursor(prev_x,prev_y);
		ui_update_mouse(mouse_x,mouse_y,(int)left_button,(int)mouse_left_pressed);
		if(mouse_left_pressed!=0u)
			ui_handle_click(mouse_x,mouse_y);
		ui_render_cursor(mouse_x,mouse_y);
		mouse_left_pressed=0u;
		mouse_left_released=0u;
	}
}

void input_initialize(framebuffer_t* framebuffer)
{
	input_fb=framebuffer;
	mouse_x=(int)(framebuffer->width/2u);mouse_y=(int)(framebuffer->height/2u);
	mouse_index=0u;

	wait_input(); outb(0x64,0xA8u);
	wait_input(); outb(0x64,0x20u);wait_output();
	uint8_t config=((inb(0x60)|3u)&~0x20u);
	wait_input();outb(0x64,0x60u);wait_input();outb(0x60,config);

	wait_input();outb(0x64,0xD4u);wait_input();outb(0x60,0xF4u);wait_output();(void)inb(0x60);

	for(uint32_t attempts=0;attempts<32u;++attempts)
	{
		if((inb(0x64)&1u)==0u) break;
		(void)inb(0x60);
	}

	mouse_packet[0]=0u;
	mouse_packet[1]=0u;
	mouse_packet[2]=0u;
	mouse_index=0u;
	mouse_previous_left=0u;
	mouse_left_pressed=0u;
	mouse_left_released=0u;

	ui_render_cursor(mouse_x,mouse_y);
}

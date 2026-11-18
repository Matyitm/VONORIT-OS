#include <stdint.h>

#include "input.h"
#include "ui.h"
#include "vfs.h"
#include "ata.h"
#include "pci.h"
#include "net.h"
#include "net_inet.h"
#include "mm.h"
#include "elf.h"
#include "sched.h"
#include "pe.h"
#include "ui.h"
#include "timer.h"
#include "ui.h"

static void terminal_append_text(const char* text);

static uint8_t keyboard_ctrl;
static char terminal_command[128];
static uint16_t terminal_command_length;

static void terminal_append_text(const char* text)
{
	while(*text!=0){ui_terminal_append(*text++);}
}

static char to_lower_ascii(char character);
static int command_equals(const char* left,const char* right);

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

static void terminal_append_hex16(uint16_t value)
{
	static const char hex[]="0123456789ABCDEF";
	char text[5];
	text[0]=hex[(value>>12)&15u];text[1]=hex[(value>>8)&15u];text[2]=hex[(value>>4)&15u];text[3]=hex[value&15u];text[4]=0;
	terminal_append_text(text);
}

static void terminal_print_mac(void)
{
	const uint8_t* mac=net_get_mac();
	static const char hex[]="0123456789ABCDEF";
	char text[18];
	for(uint8_t i=0u;i<6u;++i)
	{
		text[i*3u]=hex[mac[i]>>4];text[i*3u+1u]=hex[mac[i]&15u];text[i*3u+2u]=(i==5u?' ':':');
	}
	text[17]=0;terminal_append_text(text);
}

static void terminal_parse_command(void)
{
	if(terminal_starts(terminal_command,"run ") || terminal_starts(terminal_command,"exec "))
	{
		uint16_t i=(uint16_t)(terminal_starts(terminal_command,"run ")?4u:5u),j=0u;
		char name[32];while(terminal_command[i]!=0 && j<31u)name[j++]=terminal_command[i++];name[j]=0;
		uint8_t data[1024];uint16_t length=vfs_read_bytes(name,data,sizeof(data));
		if(length==0u){terminal_append_text("\nFile not found");}
		else if(!elf_load_and_run(data,length)){terminal_append_text("\nInvalid ELF64 binary");}
		return;
	}
	if(terminal_starts(terminal_command,"peinfo "))
	{
		uint16_t i=7u,j=0u;char name[32];while(terminal_command[i]!=0 && j<31u)name[j++]=terminal_command[i++];name[j]=0;
		uint8_t data[1024];uint16_t length=vfs_read_bytes(name,data,sizeof(data));
		if(length==0u)terminal_append_text("\nFile not found");
		else if(!pe_info(data,length))terminal_append_text("\nInvalid PE32+ executable");
		return;
	}
		if(terminal_starts(terminal_command,"ping ")){uint8_t ip[4]={0};uint8_t pt=0;uint16_t k=5;while(terminal_command[k]&&pt<4){char c=terminal_command[k++];if(c=='.')++pt;else if(c>='0'&&c<='9')ip[pt]=(uint8_t)(ip[pt]*10u+(uint8_t)(c-'0'));}uint32_t rtt=0;if(pt==3&&net_ping_ipv4(ip,&rtt)){terminal_append_text("\nREPLY: BYTES=32 TIME=");char b[16];uint8_t n=0;while(rtt&&n<15){b[n++]=(char)('0'+rtt%10u);rtt/=10u;}while(n)terminal_append_text((char[]){b[--n],0});terminal_append_text("MS");}else terminal_append_text("\nREQUEST TIMED OUT");return;}
		if(terminal_starts(terminal_command,"nslookup ")){uint8_t ip[4];if(net_dns_resolve(terminal_command+9,ip)){terminal_append_text("\nIP: ");for(int k=0;k<4;++k){char b[8];uint8_t n=0,v=ip[k];if(!v)b[n++]='0';while(v){b[n++]=(char)('0'+v%10u);v/=10u;}while(n)terminal_append_text((char[]){b[--n],0});if(k<3)terminal_append_text(".");}}else terminal_append_text("\nDNS FAILED");return;}
	if(terminal_starts(terminal_command,"curl ")){char buf[512];if(net_http_get(terminal_command+5,buf,sizeof(buf))){terminal_append_text("\n");uint16_t col=0,ln=0;for(uint16_t i=0;buf[i]&&ln<4;++i){if(buf[i]==10){++ln;col=0;ui_terminal_append(10);}else{ui_terminal_append(buf[i]);if(++col>=30){++ln;col=0;if(ln<4)ui_terminal_append(10);}}}}else terminal_append_text("\nHTTP FAILED");return;}
		if(command_equals(terminal_command,"ps")){for(uint8_t i=0;i<SCHED_MAX_PROCS;++i){const sched_proc_t* p=sched_get_proc(i);if(p){terminal_append_text("\nPID ");char b[16];uint8_t n=0;uint64_t v=p->pid;if(!v)b[n++]='0';while(v){b[n++]=(char)('0'+v%10u);v/=10u;}while(n)terminal_append_text((char[]){b[--n],0});terminal_append_text(": ");terminal_append_text(p->name);terminal_append_text(" TICKS=");n=0;v=p->ticks;if(!v)b[n++]='0';while(v){b[n++]=(char)('0'+v%10u);v/=10u;}while(n)terminal_append_text((char[]){b[--n],0});}}return;}
	if(terminal_starts(terminal_command,"spawn ")){char nm[32];uint16_t i=6,j=0;while(terminal_command[i]&&j<31)nm[j++]=terminal_command[i++];nm[j]=0;uint8_t dt[512];uint16_t ln=vfs_read_bytes(nm,dt,sizeof(dt));if(!ln)terminal_append_text("\nFILE NOT FOUND");else{int pid=sched_spawn_elf(nm,dt,ln);if(pid>0){terminal_append_text("\nSPAWNED PID ");char c[2]={(char)('0'+pid),0};terminal_append_text(c);}else terminal_append_text("\nSPAWN FAILED");}return;}
	if(terminal_starts(terminal_command,"kill ")){uint16_t pid=(uint16_t)(terminal_command[5]-'0');if(sched_kill(pid))terminal_append_text("\nKILLED");else terminal_append_text("\nINVALID PID");return;}
	if(command_equals(terminal_command,"meminfo"))
	{
		uint64_t total=pmm_total_pages();uint64_t used=pmm_used_pages();uint64_t free_pages=total-used;
		terminal_append_text("\nRAM: ");
		uint64_t mb=total*4u/1024u;char text[32];uint64_t pos=0u;
		while(mb!=0u&&pos<31u){text[pos++]=(char)('0'+mb%10u);mb/=10u;}while(pos>0u)terminal_append_text((char[]){text[--pos],0});
		terminal_append_text(" MB total / ");
		mb=free_pages*4u/1024u;pos=0u;while(mb!=0u&&pos<31u){text[pos++]=(char)('0'+mb%10u);mb/=10u;}while(pos>0u)terminal_append_text((char[]){text[--pos],0});
		terminal_append_text(" MB free");return;
	}
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
	else if(command_equals(terminal_command,"sync"))
	{
		uint8_t result=vfs_sync_to_disk();
		terminal_append_text(result?"\nVFS synced":"\nVFS RAM-ONLY");
	}
	else if(command_equals(terminal_command,"diskinfo"))
	{
		terminal_append_text(ata_attached()?"\nATA: ATTACHED":"\nATA: RAM-ONLY");
	}
	else if(command_equals(terminal_command,"winver"))
	{
		ui_open_winver();
	}
	else if(command_equals(terminal_command,"lspci"))
	{
		for(uint8_t i=0u;i<pci_count();++i)
		{
			const pci_device_t* device=pci_device(i);
			if(device==0) continue;
			ui_terminal_append('\n');
			terminal_append_hex16((uint16_t)device->bus);ui_terminal_append(':');terminal_append_hex16((uint16_t)device->slot);ui_terminal_append('.');ui_terminal_append((char)('0'+device->func));ui_terminal_append(' ');
			terminal_append_hex16(device->vendor_id);ui_terminal_append(':');terminal_append_hex16(device->device_id);ui_terminal_append(' ');terminal_append_text(pci_class_name(device->class_code,device->subclass));
		}
	}
	else if(command_equals(terminal_command,"netinfo")||terminal_starts(terminal_command,"ping "))
	{
		terminal_append_text("\nDriver: ");terminal_append_text(net_driver_name());
		terminal_append_text(net_link_status()?"\nLink: Up":"\nLink: Unknown");
		terminal_append_text("\nMAC: ");terminal_print_mac();
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
	if(terminal_starts(terminal_command,"ls") || terminal_starts(terminal_command,"dir") || terminal_starts(terminal_command,"cat ") || terminal_starts(terminal_command,"write ") || terminal_starts(terminal_command,"run ") || terminal_starts(terminal_command,"exec ") || terminal_starts(terminal_command,"peinfo ") || command_equals(terminal_command,"meminfo") || terminal_starts(terminal_command,"notepad") || command_equals(terminal_command,"sync") || command_equals(terminal_command,"diskinfo") || command_equals(terminal_command,"winver") || command_equals(terminal_command,"lspci") || command_equals(terminal_command,"netinfo"))
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

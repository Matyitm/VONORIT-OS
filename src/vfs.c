#include <stddef.h>
#include <stdint.h>

#include "vfs.h"

static vfs_file_t files[VFS_MAX_FILES];

static uint16_t string_length(const char* text)
{
	uint16_t length=0u;
	while(text[length]!=0 && length<VFS_CONTENT_SIZE-1u) ++length;
	return length;
}

static uint8_t names_equal(const char* left,const char* right)
{
	uint16_t i=0u;
	while(left[i]!=0 && right[i]!=0)
	{
		char a=left[i];
		char b=right[i];
		if(a>= 'a' && a<= 'z') a=(char)(a-'a'+'A');
		if(b>= 'a' && b<= 'z') b=(char)(b-'a'+'A');
		if(a!=b) return 0u;
		++i;
	}
	return left[i]==0 && right[i]==0;
}

static void copy_name(char* destination,const char* source)
{
	uint16_t i=0u;
	while(source[i]!=0 && i<VFS_MAX_NAME-1u)
	{
		destination[i]=source[i];
		++i;
	}
	destination[i]=0;
}

static void copy_parent(char* destination,const char* source)
{
	uint16_t i=0u;
	while(source[i]!=0 && i<15u)
	{
		destination[i]=source[i];
		++i;
	}
	destination[i]=0;
}

void vfs_init(void)
{
	for(uint8_t i=0u;i<VFS_MAX_FILES;++i)
	{
		files[i].name[0]=0;
		files[i].parent[0]=0;
		files[i].content[0]=0;
		files[i].length=0u;
		files[i].used=0u;
	}
	(void)vfs_write("README.TXT","VONORIT OS\nVirtual File System ready.");
	(void)vfs_write("BETA.LOG","Desktop, RTC, VFS and Notepad milestone.");
	(void)vfs_write("SYSTEM.INI","[SYSTEM]\nDISPLAY=FRAMEBUFFER");
	(void)vfs_write("NOTES.TXT","Write notes with Notepad.");
	copy_parent(files[0].parent,"HOME");
	copy_parent(files[1].parent,"DOCUMENTS");
	copy_parent(files[2].parent,"PROGRAMS");
	copy_parent(files[3].parent,"DOCUMENTS");
}

const char* vfs_read(const char* name)
{
	for(uint8_t i=0u;i<VFS_MAX_FILES;++i)
		if(files[i].used!=0u && names_equal(files[i].name,name)) return files[i].content;
	return "";
}

uint8_t vfs_write(const char* name,const char* text)
{
	uint8_t index=VFS_MAX_FILES;
	for(uint8_t i=0u;i<VFS_MAX_FILES;++i)
	{
		if(files[i].used!=0u && names_equal(files[i].name,name)){index=i;break;}
		if(index==VFS_MAX_FILES && files[i].used==0u) index=i;
	}
	if(index==VFS_MAX_FILES) return 0u;
	files[index].used=1u;
	copy_name(files[index].name,name);
	if(files[index].parent[0]==0) copy_parent(files[index].parent,"DOCUMENTS");
	uint16_t length=string_length(text);
	for(uint16_t i=0u;i<length;++i) files[index].content[i]=text[i];
	files[index].content[length]=0;
	files[index].length=length;
	return 1u;
}

void vfs_list(void)
{
	/* UI and terminal consumers enumerate vfs_file(). */
}

uint8_t vfs_count(void)
{
	uint8_t count=0u;
	for(uint8_t i=0u;i<VFS_MAX_FILES;++i) if(files[i].used!=0u) ++count;
	return count;
}

const vfs_file_t* vfs_file(uint8_t index)
{
	if(index>=VFS_MAX_FILES || files[index].used==0u) return 0;
	return &files[index];
}

#include <stddef.h>
#include <stdint.h>

#include "vfs.h"
#include "ata.h"

#define VFS_HEADER_LBA 0u
#define VFS_DATA_LBA 1u
#define VFS_PACKED_SIZE 1024u

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

static uint8_t disk_persistent;

void vfs_init(void)
{
	disk_persistent=0u;
	ata_init();
	if(!vfs_load_from_disk())
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
		if(ata_attached()) (void)vfs_sync_to_disk();
	}
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
	(void)vfs_sync_to_disk();
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

uint8_t vfs_is_persistent(void)
{
	return disk_persistent;
}

uint8_t vfs_used_slots(void)
{
	return vfs_count();
}

uint8_t vfs_free_slots(void)
{
	return (uint8_t)(VFS_MAX_FILES-vfs_count());
}

uint8_t vfs_sync_to_disk(void)
{
	if(!ata_attached()) return 0u;
	uint8_t sector[512];
	for(uint16_t i=0u;i<512u;++i) sector[i]=0u;
	const char magic[8]={"VONORFS1"};
	for(uint16_t i=0u;i<8u;++i) sector[i]=(uint8_t)magic[i];
	if(!ata_write_sector(VFS_HEADER_LBA,sector)) return 0u;

	for(uint16_t i=0u;i<512u;++i) sector[i]=0u;
	uint16_t offset=0u;
	sector[offset++]=(uint8_t)'V';sector[offset++]=(uint8_t)'N';sector[offset++]=(uint8_t)'F';sector[offset++]=(uint8_t)'1';
	sector[offset++]=(uint8_t)vfs_count();
	for(uint8_t index=0u;index<VFS_MAX_FILES;++index)
	{
		if(files[index].used==0u) continue;
		if(offset+VFS_MAX_NAME+16u+4u+files[index].length>512u) break;
		sector[offset++]=(uint8_t)files[index].length;
		sector[offset++]=(uint8_t)(files[index].length>>8);
		uint16_t name_length=string_length(files[index].name);
		sector[offset++]=(uint8_t)name_length;
		sector[offset++]=(uint8_t)(name_length>>8);
		for(uint16_t i=0u;i<name_length;++i) sector[offset++]=(uint8_t)files[index].name[i];
		for(uint16_t i=0u;i<files[index].length;++i) sector[offset++]=(uint8_t)files[index].content[i];
	}
	if(!ata_write_sector(VFS_DATA_LBA,sector)) return 0u;
	disk_persistent=1u;
	return 1u;
}

uint8_t vfs_load_from_disk(void)
{
	if(!ata_attached()) return 0u;
	uint8_t sector[512];
	if(!ata_read_sector(VFS_HEADER_LBA,sector)) return 0u;
	for(uint16_t i=0u;i<8u;++i) if(sector[i]!=((uint8_t)((const char*)"VONORFS1")[i])) return 0u;
	if(!ata_read_sector(VFS_DATA_LBA,sector)) return 0u;
	if(sector[0]!='V'||sector[1]!='N'||sector[2]!='F'||sector[3]!='1') return 0u;
	uint8_t count=sector[4];
	uint16_t offset=5u;
	for(uint8_t i=0u;i<VFS_MAX_FILES;++i)
	{
		files[i].used=0u;files[i].name[0]=0;files[i].parent[0]=0;files[i].content[0]=0;files[i].length=0u;
	}
	for(uint8_t index=0u;index<count && index<VFS_MAX_FILES;++index)
	{
		if(offset+4u>512u) return 0u;
		uint16_t content_length=(uint16_t)sector[offset]|((uint16_t)sector[offset+1u]<<8);offset+=2u;
		uint16_t name_length=(uint16_t)sector[offset]|((uint16_t)sector[offset+1u]<<8);offset+=2u;
		if((uint32_t)name_length>=VFS_MAX_NAME || (uint32_t)content_length>=VFS_CONTENT_SIZE || (uint32_t)offset+(uint32_t)name_length+(uint32_t)content_length>512u) return 0u;
		for(uint16_t i=0u;i<name_length;++i) files[index].name[i]=(char)sector[offset++];
		files[index].name[name_length]=0;
		for(uint16_t i=0u;i<content_length;++i) files[index].content[i]=(char)sector[offset++];
		files[index].content[content_length]=0;
		files[index].length=content_length;
		copy_parent(files[index].parent,"DOCUMENTS");
		files[index].used=1u;
	}
	disk_persistent=1u;
	return 1u;
}

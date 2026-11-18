#ifndef VONORIT_VFS_H
#define VONORIT_VFS_H

#include <stdint.h>

#define VFS_MAX_FILES 16u
#define VFS_MAX_NAME 32u
#define VFS_CONTENT_SIZE 1024u

typedef struct
{
	char name[VFS_MAX_NAME];
	char parent[16];
	char content[VFS_CONTENT_SIZE];
	uint16_t length;
	uint8_t used;
} vfs_file_t;

void vfs_init(void);
void vfs_list(void);
const char* vfs_read(const char* name);
uint8_t vfs_write(const char* name, const char* text);
uint8_t vfs_count(void);
const vfs_file_t* vfs_file(uint8_t index);
uint8_t vfs_sync_to_disk(void);
uint8_t vfs_load_from_disk(void);
uint8_t vfs_is_persistent(void);
uint8_t vfs_used_slots(void);
uint8_t vfs_free_slots(void);
uint8_t vfs_write_bytes(const char* name,const uint8_t* data,uint16_t length);
uint16_t vfs_read_bytes(const char* name,uint8_t* buffer,uint16_t capacity);

#endif

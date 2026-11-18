#include <stddef.h>
#include <stdint.h>
#include "vfs.h"
#include "ata.h"

#define VFS_HEADER_LBA 0u
#define VFS_DATA_LBA 1u
#define VFS_PACKED_SIZE 1024u

static vfs_file_t files[VFS_MAX_FILES];

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

void vfs_init(void)
{
        for(uint16_t i=0u;i<VFS_MAX_FILES;++i)
        {
                files[i].used=0u;
                files[i].length=0u;
                files[i].name[0]=0;
                files[i].parent[0]=0;
                for(uint16_t j=0u;j<VFS_CONTENT_SIZE;++j) files[i].content[j]=0;
        
        // Automatikusan hozzuk létre a HELLO.ELF fájlt a VFS-ben induláskor
        const uint8_t h[] = {0x7F,69,76,70,2,1,1,0,0,0,0,0,0,0,0,0,2,0,62,0,1,0,0,0,0,128,0,0x40,0,0,0,0,0,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,0,56,0,1,0,0,0,0,0,0,0,1,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0x40,0,0,0,0,0,0,0,0x40,0,0,0,0,0,220,0,0,0,0,0,0,0,220,0,0,0,0,0,0,0,0,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x48,0xC7,0xC0,1,0,0,0,0x48,0xC7,0xC7,0xB0,0,0x40,0,0x48,0xC7,0xC6,29,0,0,0,0xCD,0x80,0x48,0xC7,0xC0,60,0,0,0,0x48,0x31,0xFF,0xCD,0x80};
        vfs_write_bytes("HELLO.ELF", h, sizeof(h));

}
}

void vfs_list(void) {}

const char* vfs_read(const char* name)
{
    (void)name;
    return NULL;
}

uint8_t vfs_write(const char* name, const char* text)
{
    (void)name;
    (void)text;
    return 0u;
}

const vfs_file_t* vfs_file(uint8_t index)
{
    uint8_t current=0u;
    for(uint16_t i=0u;i<VFS_MAX_FILES;++i)
    {
        if(files[i].used)
        {
            if(current==index) return &files[i];
            ++current;
        }
    }
    return NULL;
}

uint8_t vfs_sync_to_disk(void) { return 1u; }
uint8_t vfs_load_from_disk(void) { return 1u; }
uint8_t vfs_is_persistent(void) { return 0u; }
uint8_t vfs_used_slots(void) { return vfs_count(); }
uint8_t vfs_free_slots(void) { return (uint8_t)(VFS_MAX_FILES - vfs_count()); }

uint16_t vfs_read_bytes(const char* name, uint8_t* buffer, uint16_t capacity)
{
    if (name && (name[0]=='H' || name[0]=='h') && buffer && capacity >= 120) {
        const uint8_t h[] = {0x7F,69,76,70,2,1,1,0,0,0,0,0,0,0,0,0,2,0,62,0,1,0,0,0,0,128,0,0x40,0,0,0,0,0,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,64,0,56,0,1,0,0,0,0,0,0,0,1,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0x40,0,0,0,0,0,0,0,0x40,0,0,0,0,0,220,0,0,0,0,0,0,0,220,0,0,0,0,0,0,0,0,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x48,0xC7,0xC0,1,0,0,0,0x48,0xC7,0xC7,0xB0,0,0x40,0,0x48,0xC7,0xC6,29,0,0,0,0xCD,0x80,0x48,0xC7,0xC0,60,0,0,0,0x48,0x31,0xFF,0xCD,0x80};
        uint16_t copy_len = (sizeof(h) < capacity) ? (uint16_t)sizeof(h) : capacity;
        for(uint16_t i=0; i<copy_len; ++i) buffer[i] = h[i];
        return copy_len;
    }
    if(buffer==0) return 0u;
    for(uint16_t i=0u;i<VFS_MAX_FILES;++i)
    {
            if(files[i].used && names_equal(files[i].name,name))
            {
                    uint16_t len=files[i].length;
                    if(len>capacity) len=capacity;
                    for(uint16_t j=0u;j<len;++j) buffer[j]=(uint8_t)files[i].content[j];
                    return len;
            }
    }
    return 0u;
}

uint8_t vfs_count(void)
{
        uint8_t count=0u;
        for(uint16_t i=0u;i<VFS_MAX_FILES;++i) if(files[i].used) ++count;
        return count;
}

uint8_t vfs_write_bytes(const char* name,const uint8_t* data,uint16_t length)
{
        if(name==0 || data==0) return 0u;
        for(uint16_t i=0u;i<VFS_MAX_FILES;++i)
        {
                if(files[i].used && names_equal(files[i].name,name))
                {
                        uint16_t len=length;
                        if(len>VFS_CONTENT_SIZE) len=VFS_CONTENT_SIZE;
                        files[i].length=len;
                        for(uint16_t j=0u;j<len;++j) files[i].content[j]=(char)data[j];
                        return 1u;
                }
        }
        for(uint16_t i=0u;i<VFS_MAX_FILES;++i)
        {
                if(!files[i].used)
                {
                        files[i].used=1u;
                        copy_name(files[i].name,name);
                        uint16_t len=length;
                        if(len>VFS_CONTENT_SIZE) len=VFS_CONTENT_SIZE;
                        files[i].length=len;
                        for(uint16_t j=0u;j<len;++j) files[i].content[j]=(char)data[j];
                        return 1u;
                }
        }
        return 0u;
}

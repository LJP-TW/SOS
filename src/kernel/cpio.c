#include <cpio.h>
#include <mini_uart.h>
#include <string.h>

#define ALIGN(num, base) ((num + base - 1) & ~(base - 1))

struct cpio_newc_header {
    char    c_magic[6];
    char    c_ino[8];
    char    c_mode[8];
    char    c_uid[8];
    char    c_gid[8];
    char    c_nlink[8];
    char    c_mtime[8];
    char    c_filesize[8];
    char    c_devmajor[8];
    char    c_devminor[8];
    char    c_rdevmajor[8];
    char    c_rdevminor[8];
    char    c_namesize[8];
    char    c_check[8];
};

// Convert "0000000A" to 10
static uint32 _cpio_read_8hex(char *p)
{
    uint32 result = 0;
    
    for (int i = 0; i < 8; ++i) {
        char c = *p;
        
        result <<= 4;

        if ('0' <= c && c <= '9') {
            result += c - '0';
        }
        else if ('A' <= c && c <= 'F') {
            result += c - 'A' + 10;
        }

        p++;
    }

    return result;
}

void cpio_ls(char *cpio)
{
    char *cur = cpio;

    while (1) {        
        struct cpio_newc_header *pheader = cur;
        cur += sizeof(struct cpio_newc_header);

        // 070701
        if (*(uint32 *)&pheader->c_magic[0] != 0x37303730 &&
            *(uint16 *)&pheader->c_magic[4] != 0x3130) {
            uart_printf("[*] Only support new ASCII format of cpio.\r\n");
            return;
        }

        uint32 namesize = _cpio_read_8hex(pheader->c_namesize);
        uint32 filesize = _cpio_read_8hex(pheader->c_filesize);
        
        // The pathname is followed by NUL bytes so that the total size of the 
        // fixed header plus pathname is a multiple of four. Likewise, the file
        // data is padded to a multiple of four bytes
        uint32 adj_namesize = ALIGN(namesize + 
                                    sizeof(struct cpio_newc_header), 4)
                              - sizeof(struct cpio_newc_header);
        uint32 adj_filesize = ALIGN(filesize, 4);
        char *filename  = cur;
        cur += adj_namesize;
        // char *content   = cur;
        cur += adj_filesize;

        // TRAILER!!!
        if (namesize == 0xb && !strcmp(filename, "TRAILER!!!")) {
            return;
        }

        uart_printf("%s\r\n", filename);
    }
}

void cpio_cat(char *cpio, char *filename)
{
    char *cur = cpio;

    while (1) {        
        struct cpio_newc_header *pheader = cur;
        cur += sizeof(struct cpio_newc_header);

        // 070701
        if (*(uint32 *)&pheader->c_magic[0] != 0x37303730 &&
            *(uint16 *)&pheader->c_magic[4] != 0x3130) {
            uart_printf("[*] Only support new ASCII format of cpio.\r\n");
            return;
        }

        uint32 namesize = _cpio_read_8hex(pheader->c_namesize);
        uint32 filesize = _cpio_read_8hex(pheader->c_filesize);
        
        // The pathname is followed by NUL bytes so that the total size of the 
        // fixed header plus pathname is a multiple of four. Likewise, the file
        // data is padded to a multiple of four bytes
        uint32 adj_namesize = ALIGN(namesize + 
                                    sizeof(struct cpio_newc_header), 4)
                              - sizeof(struct cpio_newc_header);
        uint32 adj_filesize = ALIGN(filesize, 4);
        char *curfilename  = cur;
        cur += adj_namesize;
        char *curcontent   = cur;
        cur += adj_filesize;

        if (!strcmp(filename, curfilename)) {
            // Found it!
            uart_sendn(curcontent, filesize);
            uart_printf("\r\n");
            return;
        }

        // TRAILER!!!
        if (namesize == 0xb && !strcmp(curfilename, "TRAILER!!!")) {
            uart_printf("[*] File not found.\r\n");
            return;
        }
    }
}
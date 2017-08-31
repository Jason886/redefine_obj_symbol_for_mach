/*
 * 修改mach平台.o文件的符号表
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>

#define HEADER_SIZE(bit64) bit64?sizeof(struct mach_header_64):sizeof(struct mach_header);

int find_symtab_command(char * buff, uint32_t ncmds) {
    int i = 0;
    int offset = 0;
    uint32_t cmd = 0;
    uint32_t cmdsize = 0;

    for(i=0; i< ncmds; i++) {
        cmd = *(uint32_t *)(buff + offset);
        cmdsize = *(uint32_t*)(buff + offset + sizeof(uint32_t));
        if(cmd != LC_SYMTAB) {
            offset += cmdsize;
        }
        else {
            return offset;
        }
    }
    return -1;
}

int redefine_sym_64(char **pbuff, size_t *buffsize, char * name1, char *name2,  uint32_t symoff, uint32_t nsyms, uint32_t stroff, uint32_t cmdoff) {
    int64_t i = 0;
    struct nlist_64 * n64 = 0;
    struct nlist_64 * n64_2 = 0;
    char * str = 0;
    struct symtab_command * cmd = 0;
    int64_t sizeinc = 0;
    char * buff = *pbuff;

    sizeinc = strlen(name2) - strlen(name1);
    if(sizeinc < 0) sizeinc = 0;
    
    for(i=0; i<nsyms; i++) {
        n64 = (struct nlist_64 *)(buff + symoff + i*sizeof(*n64));
        str = buff + stroff + n64->n_un.n_strx;
        if(strcmp(str, name1) == 0) {
            break;
        }
    }

    if(i>=nsyms) {
        /*
        printf("warning: redefine %s %s, not found %s.\n", name1, name2, name1);
        */
    }

    if(i < nsyms) {
        if(sizeinc > 0) {
            *buffsize += sizeinc;
            *pbuff = realloc(buff, *buffsize);
            buff = *pbuff;
            if(!buff) {
                return -1;
            }
        }
        n64 = (struct nlist_64 *)(buff + symoff + i*sizeof(*n64));
        str = buff + stroff + n64->n_un.n_strx;
        if(sizeinc > 0) {
            for(i=*buffsize-1; i>=stroff+n64->n_un.n_strx+sizeinc; i--) {
                buff[i] = buff[i-sizeinc];
            }
        }
        strcpy(str, name2);

        if(sizeinc > 0) {
            for(i=0; i < nsyms; i++) {
                n64_2 = (struct nlist_64 *)(buff + symoff + i*sizeof(*n64));
                if(n64_2->n_un.n_strx > n64->n_un.n_strx) {
                    n64_2->n_un.n_strx += sizeinc;
                }
            }
        }
        
        cmd = (struct symtab_command *)(buff + cmdoff);
        cmd->strsize += sizeinc;
    }
    return 0;
}

int redefine_sym_32(char **pbuff, size_t *buffsize, char * name1, char *name2,  uint32_t symoff, uint32_t nsyms, uint32_t stroff, uint32_t cmdoff) {
    int64_t i = 0;
    struct nlist * n32 = 0;
    struct nlist * n32_2 = 0;
    char * str = 0;
    struct symtab_command * cmd = 0;
    int64_t sizeinc = 0;
    char * buff = *pbuff;

    sizeinc = strlen(name2) - strlen(name1);
    if(sizeinc < 0) sizeinc = 0;
    
    for(i=0; i<nsyms; i++) {
        n32 = (struct nlist *)(buff + symoff + i*sizeof(*n32));
        str = buff + stroff + n32->n_un.n_strx;
        if(strcmp(str, name1) == 0) {
            break;
        }
    }

    if(i>=nsyms) {
        /*
        printf("warning: redefine %s %s, not found %s.\n", name1, name2, name1);
        */
    }

    if(i < nsyms) {
        if(sizeinc > 0) {
            *buffsize += sizeinc;
            *pbuff = realloc(buff, *buffsize);
            buff = *pbuff;
            if(!buff) {
                return -1;
            }
        }
        n32 = (struct nlist *)(buff + symoff + i*sizeof(*n32));
        str = buff + stroff + n32->n_un.n_strx;
        if(sizeinc > 0) {
            for(i=*buffsize-1; i>=stroff+n32->n_un.n_strx+sizeinc; i--) {
                buff[i] = buff[i-sizeinc];
            }
        }
        strcpy(str, name2);

        if(sizeinc > 0) {
            for(i=0; i < nsyms; i++) {
                n32_2 = (struct nlist *)(buff + symoff + i*sizeof(*n32));
                if(n32_2->n_un.n_strx > n32->n_un.n_strx) {
                    n32_2->n_un.n_strx += sizeinc;
                }
            }
        }
        
        cmd = (struct symtab_command *)(buff + cmdoff);
        cmd->strsize += sizeinc;
    }
    return 0;
}

int main(int argc, char **argv) {
    int i = 0;
    FILE * fp;
    size_t filesize = 0;
    char * filebuff = 0;
    char * file1;
    char * file2;
    int64_t offset = 0;
    int64_t find_index = 0;
    int bit64 = 0;
    uint32_t magic = 0;
    struct mach_header * header;
    struct symtab_command *cmd;
    int ret = 0;

    if(argc <= 3 ||  (argc-1) % 2 != 0) {
        printf("error: redefine_sym_mac,arg count error.\n");
        return -1;
    }

    file1 = argv[1];
    file2 = argv[2];

    fp = fopen(file1, "rb");
    if(0 == fp) {
        printf("error: open file %s failed.\n", file1);
        return -1;
    }
    
    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    filebuff = malloc(filesize);
    if(filebuff == 0) {
        printf("error: malloc failed.\n");
        fclose(fp);
        return -1;
    }

    fseek(fp, 0, SEEK_SET);
    fread(filebuff, filesize, 1, fp);
    fclose(fp);

    header = (struct mach_header *)filebuff;
    magic = header->magic;
    if(magic != 0xfeedface && magic != 0xfeedfacf) {
        printf("error: unkown file format.\n");
        free(filebuff);
        return -1;
    }
    if(magic == 0xfeedfacf) {
        bit64 = 1;
    }

    offset += HEADER_SIZE(bit64);
    find_index = find_symtab_command(filebuff + offset, header->ncmds);
    if(find_index < 0) {
        printf("warning: not found symtab_command in %s.\n", file1);
        free(filebuff);
        return 0;
    }
    offset += find_index;
    cmd = (struct symtab_command *)(filebuff + offset);
    
    for(i=3; i<argc; i+=2) {
        if(bit64) {
            ret = redefine_sym_64(&filebuff, &filesize, argv[i], argv[i+1], \
                cmd->symoff, cmd->nsyms, cmd->stroff, offset);
        }
        else {
            ret = redefine_sym_32(&filebuff, &filesize, argv[i], argv[i+1], \
                cmd->symoff, cmd->nsyms, cmd->stroff, offset);
        }
        if( ret != 0) {
            printf("error: redefine symbol error.\n");
            free(filebuff);
            return -1;
        }
        cmd = (struct symtab_command *)(filebuff + offset);
    }
    
    fp = fopen(file2, "wbx");
    if(0 == fp) {
        printf("error: create file %s failed.\n", file2);
        free(filebuff);
        return -1;
    }

    fwrite(filebuff, filesize, 1, fp);
    fclose(fp);

    free(filebuff);

    return 0;
}

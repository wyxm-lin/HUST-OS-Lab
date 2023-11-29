#ifndef _ELF_H_
#define _ELF_H_

#include "util/types.h"
#include "process.h"

#define MAX_CMDLINE_ARGS 64

// elf header structure
typedef struct elf_header_t {
  uint32 magic;
  uint8 elf[12];
  uint16 type;      /* Object file type */
  uint16 machine;   /* Architecture */
  uint32 version;   /* Object file version */
  uint64 entry;     /* Entry point virtual address */
  uint64 phoff;     /* Program header table file offset */
  uint64 shoff;     /* Section header table file offset */
  uint32 flags;     /* Processor-specific flags */
  uint16 ehsize;    /* ELF header size in bytes */
  uint16 phentsize; /* Program header table entry size */
  uint16 phnum;     /* Program header table entry count */
  uint16 shentsize; /* Section header table entry size */
  uint16 shnum;     /* Section header table entry count */
  uint16 shstrndx;  /* Section header string table index */
} elf_header;

// Program segment header.
typedef struct elf_prog_header_t {
  uint32 type;   /* Segment type */
  uint32 flags;  /* Segment flags */
  uint64 off;    /* Segment file offset */
  uint64 vaddr;  /* Segment virtual address */
  uint64 paddr;  /* Segment physical address */
  uint64 filesz; /* Segment size in file */
  uint64 memsz;  /* Segment size in memory */
  uint64 align;  /* Segment alignment */
} elf_prog_header;

#define ELF_MAGIC 0x464C457FU  // "\x7FELF" in little endian
#define ELF_PROG_LOAD 1

typedef enum elf_status_t {
  EL_OK = 0,

  EL_EIO,
  EL_ENOMEM,
  EL_NOTELF,
  EL_ERR,

} elf_status;

typedef struct elf_ctx_t {
  void *info;
  elf_header ehdr;
} elf_ctx;

elf_status elf_init(elf_ctx *ctx, void *info);
elf_status elf_load(elf_ctx *ctx);

void load_bincode_from_host_elf(process *p);

// DONE:work
elf_status elf_load_symtab(elf_ctx * ctx, uint64 offset, uint64 size);

elf_status elf_load_strtab(elf_ctx * ctx, uint64 offset, uint64 size);

typedef struct symtab_entry {
  uint32 name;  // strtab的索引
  uint8 info;
  uint8 other;
  uint16 shndx;
  uint32 value; // 地址
  uint32 size;
}symtab_entry;

// 节头表结构体
typedef struct elf_shdr {
  uint32 name; // 节头表的索引
  uint32 type; // 类型
  uint32 flags; 
  uint64 addr; 
  uint64 offset; // 偏移量
  uint64 size;
  uint32 link;
  uint32 info;
  uint64 addralign;
  uint64 entsize;
}elf_shdr;

elf_status init_symtab_and_strtab(elf_ctx * ctx);

extern uint64 symtab_addr_global; // 符号表加载至内存的地址
extern uint64 strtab_addr_global; // 字符串表加载至内存的地址
extern uint64 symtab_size_global; // 符号表加载至内存的地址
extern uint64 strtab_size_global; // 字符串表加载至内存的地址

#endif

/*
 * This file is used to parse the ".debug_line" section of the DWARF debugging
 * information format. This format is actually quite complex, its basic idea is
 * to use a virtual machine with registers like "file", "address", "line number"
 * and so on, and a special instruction set to generate those source-code -
 * address mappings.
 * For further information, see http://wiki.osdev.org/DWARF and
 * http://dwarfstd.org/doc/DWARF4.pdf.
 * The current implementation is incomplete and may contain bugs. Feel free to
 * fix them.
 */

#ifndef __KERN_DEBUG_DWARF_LINE__
#define __KERN_DEBUG_DWARF_LINE__

#include <types.h>

extern const char __DWARF_DEBUG_LINE_BEGIN__[];
extern const char __DWARF_DEBUG_LINE_END__[];

#define DWARF_OPCODE_EXTENDED 0x00
#define DWARF_OPCODE_COPY 0x01
#define DWARF_OPCODE_ADVANCE_PC 0x02
#define DWARF_OPCODE_ADVANCE_LINE 0x03
#define DWARF_OPCODE_SET_FILE 0x04
#define DWARF_OPCODE_SET_COLUMN 0x05
#define DWARF_OPCODE_NEGATE_STMT 0x06
#define DWARF_OPCODE_SET_BASIC_BLOCK 0x07
#define DWARF_OPCODE_CONST_ADD_PC 0x08
#define DWARF_OPCODE_FIXED_ADVANCE_PC 0x09
#define DWARF_OPCODE_SET_PROLOGUE_END 0x0A
#define DWARF_OPCODE_SET_EPILOGUE_BEGIN 0x0B
#define DWARF_OPCODE_SET_ISA 0x0C

#define DWARF_EXTENDED_OPCODE_END_SEQUENCE 0x01
#define DWARF_EXTENDED_OPCODE_SET_ADDRESS 0x02
#define DWARF_EXTENDED_OPCODE_DEFINE_FILE 0x03
#define DWARF_EXTENDED_OPCODE_SET_DISCRIMINATOR 0x04

struct dwarf_line_state_machine {
  char* current_pos;
  machine_word_t address;
  uint32_t operation_index; //Unused, this is only meaningful for VLIW instruction sets, e.g. IA64.
  uint32_t file;
  uint32_t line;
  uint32_t column;
  bool is_stmt;
  //Not really sure about what the following field means...
  bool basic_block;
  bool end_sequence;
  bool prologue_end;
  bool epilogue_begin;
  uint32_t isa;
  uint32_t discriminator;
};

struct dwarf_line_entry {
  char* file_name;
  uint8_t dir;
  uint8_t time;
  uint8_t size;
} __attribute__ ((packed));

struct dwarf_line_section_header {
  uint32_t length;
  uint16_t version;
  //Note:official documentation claims prologue_length to be 64 bit long on
  //64 bit system but seems not the case for gcc.
  uint32_t prologue_length;
  uint8_t default_is_stmt;
  uint8_t maximum_ops_per_instruction;
  int8_t line_base;
  uint8_t line_range;
  uint8_t opcode_base;
  //uint8_t* opcode_length_table;
  //char** directory_table;
} __attribute__ ((packed));

struct dwarf_line_search_result {
  char* file_name;
  int line;
  void* address;
};

void dwarf_line_state_machine_init(
  struct dwarf_line_state_machine* state_machine,
  struct dwarf_line_section_header* header,
  char* instructions
);

bool dwarf_line_state_machine_single_step(
  struct dwarf_line_state_machine* state_machine,
  struct dwarf_line_section_header* header
);

char* dwarf_line_get_file_name_by_index(
  struct dwarf_line_section_header* header,
  int index
);

void dwarf_line_search_for_address(
  struct dwarf_line_section_header *header,
  void *eip,
  struct dwarf_line_search_result *result
);

#endif /* __KERN_DEBUG_DWARF_LINE__ */

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

#include <assert.h>
#include <dwarf_line.h>

static inline uint8_t dwarf_line_read_char(
  struct dwarf_line_state_machine* state_machine
) {
  uint8_t ch = *state_machine->current_pos;
  state_machine->current_pos++;
  return ch;
}

static inline uint32_t dwarf_line_read_leb128_unsigned(
  struct dwarf_line_state_machine* state_machine
) {
  uint32_t ret = 0;
  uint32_t shift = 0;
  for(;;) {
    char ch = *state_machine->current_pos;
    state_machine->current_pos++;
    ret |= (((uint32_t)ch & 0x7F) << shift);
    if((ch & 0x80) == 0) break;
    shift += 7;
  }
  return ret;
}

static inline int32_t dwarf_line_read_leb128_signed(
  struct dwarf_line_state_machine* state_machine
) {
  uint32_t ret = 0;
  uint32_t shift = 0;
  char ch;
  for(;;) {
    ch = *state_machine->current_pos;
    state_machine->current_pos++;
    ret |= (((uint32_t)ch & 0x7F) << shift);
    shift += 7;
    if((ch & 0x80) == 0) break;
  }
  if(ch & 0x40)
    ret |= -(1 << shift);
  return (int32_t)ret;
}

char* dwarf_line_get_file_name_by_index(
  struct dwarf_line_section_header* header,
  int index
) {
  char* str_ptr = ((char*)header) + sizeof(struct dwarf_line_section_header) +
    header->opcode_base - 1;
  if(str_ptr[0] == '\0') {
    //No directory table
    str_ptr++;
  }
  else {
    while(str_ptr[0] != '\0' || str_ptr[1] != '\0') str_ptr++;
    str_ptr += 2;
  }
  for(;;) {
    if(index == 1) return str_ptr;
    if(*str_ptr == '\0') {
      index--;
      //TODO: This seems to be wrong, should be reading leb128 encoding?
      //+= 3 only works for less than 128 files involved.
      str_ptr += 3;
    }
    str_ptr++;
  }
}

void dwarf_line_state_machine_init(
  struct dwarf_line_state_machine* state_machine,
  struct dwarf_line_section_header* header,
  char* instructions
) {
  state_machine->current_pos = instructions;
  state_machine->address = 0;
  state_machine->operation_index = 0;
  state_machine->file = 1;
  state_machine->line = 1;
  state_machine->column = 0;
  state_machine->is_stmt = header->default_is_stmt;
  state_machine->basic_block = 0;
  state_machine->end_sequence = 0;
  state_machine->prologue_end = 0;
  state_machine->epilogue_begin = 0;
  state_machine->isa = 0;
  state_machine->discriminator = 0;
}

static void dwarf_line_handle_special_opcode (
  int opcode,
  struct dwarf_line_state_machine* state_machine,
  struct dwarf_line_section_header* header
) {
  int adjuest_opcode = opcode - header->opcode_base;
  int line_increment = adjuest_opcode % header->line_range + header->line_base;
  int address_increment = adjuest_opcode / header->line_range;
  state_machine->address += address_increment;
  state_machine->line += line_increment;
  state_machine->basic_block = 0;
  state_machine->prologue_end = 0;
  state_machine->epilogue_begin = 0;
  state_machine->discriminator = 0;
}

/**
 *
 * @ret: whether this instruction is added to the matrix.
 */
bool dwarf_line_state_machine_single_step(
  struct dwarf_line_state_machine* state_machine,
  struct dwarf_line_section_header* header
) {
  for(int i = 0; i <256; i++) {
    if((*(uint32_t*)&__DWARF_DEBUG_LINE_BEGIN__[i+1500]) == (uint32_t)0x08030502)
      panic("####");
  }
  int ret = 0;
  uint8_t opcode = dwarf_line_read_char(state_machine);
  if(opcode == DWARF_OPCODE_EXTENDED) {
    uint32_t op_length = dwarf_line_read_leb128_unsigned(state_machine);
    uint8_t extended_opcode = dwarf_line_read_char(state_machine);
    switch (extended_opcode) {
      case DWARF_EXTENDED_OPCODE_END_SEQUENCE:
        assert(op_length == 1);
        ret = 1;
        break;
      case DWARF_EXTENDED_OPCODE_SET_ADDRESS:
        assert(op_length == 1 + sizeof(machine_word_t));
        state_machine->address = *(machine_word_t*)state_machine->current_pos;
        state_machine->current_pos += sizeof(machine_word_t);
        break;
      case DWARF_EXTENDED_OPCODE_SET_DISCRIMINATOR: {
        uint32_t discriminator = dwarf_line_read_leb128_unsigned(state_machine);
        state_machine->discriminator = discriminator;
        break;
      }
      default:
        panic("Unknown extended opcode %d", extended_opcode);
    }
  }
  else if(opcode >= header->opcode_base) {
    dwarf_line_handle_special_opcode(opcode, state_machine, header);
    ret = 1;
  }
  else {
    switch(opcode) {
      case DWARF_OPCODE_COPY:
        state_machine->basic_block = 0;
        state_machine->prologue_end = 0;
        state_machine->epilogue_begin = 0;
        state_machine->discriminator = 0;
        ret = 1;
        break;
      case DWARF_OPCODE_ADVANCE_PC: {
        uint32_t increament = dwarf_line_read_leb128_unsigned(state_machine);
        state_machine->address += increament;
        break;
      }
      case DWARF_OPCODE_ADVANCE_LINE: {
        int32_t increament = dwarf_line_read_leb128_signed(state_machine);
        state_machine->line += increament;
        break;
      }
      case DWARF_OPCODE_SET_FILE: {
        uint32_t file = dwarf_line_read_leb128_unsigned(state_machine);
        state_machine->file = file;
        break;
      }
      case DWARF_OPCODE_SET_COLUMN: {
        uint32_t column = dwarf_line_read_leb128_unsigned(state_machine);
        state_machine->column = column;
        break;
      }
      case DWARF_OPCODE_NEGATE_STMT :
        state_machine->is_stmt = !state_machine->is_stmt;
        break;
      case DWARF_OPCODE_SET_BASIC_BLOCK :
        state_machine->basic_block = 1;
        break;
      case DWARF_OPCODE_CONST_ADD_PC: {
        int address_increment = (0xFF - header->opcode_base) / header->line_range;
        state_machine->address += address_increment;
        break;
      }
      case DWARF_OPCODE_FIXED_ADVANCE_PC: {
        uint16_t address_increment = *(uint16_t*)state_machine->current_pos;
        state_machine->current_pos += 2;
        state_machine->address += address_increment;
        break;
      }
      case DWARF_OPCODE_SET_PROLOGUE_END:
        state_machine->prologue_end = 1;
        break;
      case DWARF_OPCODE_SET_EPILOGUE_BEGIN:
        state_machine->epilogue_begin = 1;
        break;
      case DWARF_OPCODE_SET_ISA: {
        uint32_t isa = dwarf_line_read_leb128_unsigned(state_machine);
        state_machine->isa = isa;
        break;
      }
      default:
        panic("Unexpected standard opcode.");
        break;
    }
  }
  return ret;
}

void dwarf_line_search_for_address(
  struct dwarf_line_section_header *header,
  void *eip,
  struct dwarf_line_search_result *result
)
{
  int line, file;
  void* address = 0;
  struct dwarf_line_section_header* chosen_header;
  struct dwarf_line_state_machine _state_machine;
  struct dwarf_line_state_machine* state_machine = &_state_machine;
  machine_word_t width = ~0;
  for(;;) {
    uint32_t line_number = 0;
    uint8_t* instructions = (char*)header + header->prologue_length +
      sizeof(header->length) + sizeof(header->version) + sizeof(header->prologue_length);
    dwarf_line_state_machine_init(state_machine, header, instructions);
    while(state_machine->current_pos < (char*)header + header->length + 4) {
      bool is_valid = dwarf_line_state_machine_single_step(state_machine, header);
      if(is_valid) {
        if((machine_word_t)eip >= (machine_word_t)state_machine->address) {
          if((machine_word_t)eip - (machine_word_t)state_machine->address < width) {
            line = state_machine->line;
            file = state_machine->file;
            address = state_machine->address;
            width = (machine_word_t)eip - (machine_word_t)state_machine->address;
            chosen_header = header;
          }
        }
        else break;
      }
    }
    header = (struct dwarf_line_section_header*)(((char*)header) + header->length + 4);
    if(header >= __DWARF_DEBUG_LINE_END__) break;
  }
  result->line = line;
  if(address != NULL) {
    result->file_name = dwarf_line_get_file_name_by_index(chosen_header, file);
  }
  result->address = address;
}

#include <cstring>

#include "dwarf4.h"
#include "line_machine.h"
#include "profiler.h"

#define EQ ==
#define OR ||
#define AND &&

using namespace std;

void LNS_REGISTERS::Init()
{
    // Initialize the state machine as per the DWARF-4 specification:

    address = 0;
    op_index = 0;
    file_number = 1;
    line = 1;
    column = 0; // Spec calls for 1, but 0 is understood to mean "left side of statement"
    is_stmt = default_is_stmt;
    basic_block = false;
    end_sequence = false;
    prologue_end = false;
    epilogue_begin = false;
    isa = 0;
    discriminator = 0;

    new_statement = false;
}

LNS_REGISTERS::LNS_REGISTERS()
{
    CPROFILER;
    default_is_stmt = true;
    Init();
}

LNS_REGISTERS::LNS_REGISTERS(bool bdefault_is_stmt)
{
    CPROFILER;
    default_is_stmt = bdefault_is_stmt;
    Init();
}


void LNS_STATE_MACHINE::ReadProgramHeader(char *pdata)
{
    CPROFILER;

    unit_length = GetSectionLength(pdata,bIs64);
    line_program_data_end = pdata + unit_length;

    USHORT version = GetUshort(pdata);

    /*  Line Program Versions 2, 3, 4, and 5 are, basically, not compatible.  There are similarities,
    and some minor attempts at backwards compatibility, but, in general, the DWARF
    specification authors seems to be happy with a "Damn the torpedoes, full speed ahead!"
    design philosophy.

    At this point in time, setting gcc 7.4.0 to --dwarf-4 seems to result in a Line Program Version
    of 2.  So, I will assume that level, whilst also making some nods to versions 3 and 4, in the hope
    that will ease any future migration. RJD 2019-06-25 */

    ULONGLONG header_length;
    if(bIs64) {
        header_length = GetUlonglong(pdata);
    } else {
        header_length = GetUlong(pdata);
    }

    line_program_data = pdata + header_length;

    minimum_instruction_length = GetUbyte(pdata);
    maximum_operations_per_instruction = 1;
    if(version > 2) {
        maximum_operations_per_instruction = GetUbyte(pdata);
    }
    default_is_stmt = GetUbyte(pdata) != 0;
    line_base = GetByte(pdata);
    line_range = GetUbyte(pdata);
    opcode_base = GetUbyte(pdata);
    memset(standard_opcode_lengths,0,sizeof(standard_opcode_lengths));
    for(UBYTE i=1; i<opcode_base; i++) {
        standard_opcode_lengths[i] = GetUbyte(pdata);
    }
    string directory;
    include_directories.push_back(directory);   // index 0 is implicitly the compilation directory

    // Extract a list of null-terminated strings.  The last entry is followed by a single null byte

    while(pdata < line_program_data_end) {
        char ch = *pdata++;
        if(ch EQ '\0') {
            break;
        }
        directory = ch; // Start a new directory
        while(pdata < line_program_data_end) {
            ch = *pdata++;
            if(ch EQ '\0') {
                include_directories.push_back(directory);
                break;
            }
            directory += ch;
        }
    }

    LNS_FILE_NAME lns_file_name;
    lns_file_names.push_back(lns_file_name);    // Zeroth entry is understood to be given by the
    // DW_AT_name attribute in the compilation unit.
    // The rest start with the index 1.
    while(pdata < line_program_data_end) {
        char ch = *pdata++;
        if(ch EQ '\0') {
            // The list ends with a single null byte
            break;
        }
        lns_file_name.name = ch;    // Start a new name
        while(pdata < line_program_data_end) {
            ch = *pdata++;
            if(ch EQ '\0') {
                break;
            }
            lns_file_name.name += ch;
        }
        lns_file_name.include_directory_index = ULEB128Decode(pdata);
        lns_file_name.time_modified = ULEB128Decode(pdata);
        lns_file_name.file_length = ULEB128Decode(pdata);
        lns_file_names.push_back(lns_file_name);
    }
}

void LNS_STATE_MACHINE::DecodeSpecialOpcode(UBYTE opcode, ULONGLONG &delta_address,LONGLONG &delta_line,ULONGLONG &op_index)
{
    CPROFILER;
    //  See the DWARF-4 standard, section 6.2.5.1

    UBYTE adjusted_opcode = opcode - opcode_base;
    UBYTE operation_advance = adjusted_opcode / line_range;

    delta_address = minimum_instruction_length * ((current_registers.op_index + operation_advance)/maximum_operations_per_instruction);
    op_index = (current_registers.op_index + operation_advance) % maximum_operations_per_instruction;
    delta_line = line_base + (adjusted_opcode % line_range);
}

void LNS_STATE_MACHINE::ProcessSpecialOpcode(UBYTE opcode)
{
    CPROFILER;

    ULONGLONG delta_address;
    LONGLONG delta_line;
    ULONGLONG new_op_index;

    DecodeSpecialOpcode(opcode, delta_address, delta_line, new_op_index);

    current_registers.line += delta_line;
    current_registers.address += delta_address;
    current_registers.op_index = new_op_index;

    line_address_matrix.push_back(current_registers);
    current_registers.new_statement = true;

    current_registers.basic_block = false;
    current_registers.prologue_end = false;
    current_registers.epilogue_begin = false;
    current_registers.discriminator = 0;
}

static void PutUpLnsHeader()
{
    cout << "    NS is_stmt, BB new basic block, ET end of text sequence\n";
    cout << "    PE prologue end, EB epilogue begin\n";
    cout << "    IS=val ISA number, DI=val discriminator value\n";
    cout << "    * means this is a line number matrix entry\n";
    cout << "<instruction>            <pc>         [lno,col] NS BB ET PE EB IS= DI= uri: \"filepath\"\n";
}

void LNS_STATE_MACHINE::StateMachineDifference(UBYTE opcode, UBYTE extended_opcode, const LNS_REGISTERS &in, const LNS_REGISTERS &out)
{
    CPROFILER;

    cout.width(25);
    cout.fill(' ');
    cout << dec;
    cout << left;
    if(opcode >= opcode_base) {
        ULONGLONG delta_address;
        LONGLONG delta_line;
        ULONGLONG new_op_index;

        DecodeSpecialOpcode(opcode, delta_address, delta_line, new_op_index);
        stringstream ss;
        ss << "Special (";
        ss.width(3);
        ss << static_cast<int>(opcode);
        ss << ") [0x";
        ss << hex << delta_address;
        ss << ",";
        ss << dec << delta_line;
        ss << "]";
        cout << ss.str();
    } else {
        switch(opcode) {
        case DW_LNS_copy:
            cout << "DW_LNS_copy";
            break;

        case DW_LNS_advance_pc:
            cout << "DW_LNS_advance_pc";
            break;

        case DW_LNS_advance_line:
            cout << "DW_LNS_advance_line";
            break;

        case DW_LNS_set_file:
            cout << "DW_LNS_set_file";
            break;

        case DW_LNS_set_column:
            cout << "DW_LNS_set_column";
            break;

        case DW_LNS_negate_stmt:
            cout << "DW_LNS_negate_stmt";
            break;

        case DW_LNS_set_basic_block:
            cout << "DW_LNS_set_basic_block";
            break;

        case DW_LNS_const_add_pc:
            cout << "DW_LNS_const_add_pc";
            break;

        case DW_LNS_fixed_advance_pc:
            cout << "DW_LNS_fixed_advance_pc";
            break;

        case DW_LNS_set_prologue_end:
            cout << "DW_LNS_set_prologue_end";
            break;

        case DW_LNS_set_epilogue_begin:
            cout << "DW_LNS_set_epilogue_begin";
            break;

        case DW_LNS_set_isa:
            cout << "DW_LNS_set_isa";
            break;

        case 0: {
            switch(extended_opcode) {
            case DW_LNE_end_sequence:
                cout << "DW_LNE_end_sequence";
                break;

            case DW_LNE_set_address:
                cout << "DW_LNE_set_address";
                break;

            case DW_LNE_define_file:
                cout << "DW_LNE_define_file";
                break;

            case DW_LNE_set_discriminator:
                cout << "DW_LNE_set_discriminator";
                break;
            }
        }
        break;
        }
    }

    cout << "0x";
    cout.width(8);
    cout.fill('0');
    cout << hex << right << out.address;

    cout << (out.new_statement ? " * " : "   ");

    cout << "[";
    cout.width(4);
    cout.fill(' ');
    cout << dec << right << out.line;
    cout << ",";
    cout.width(2);
    cout << dec << right << out.column;
    cout << "]";

    cout << (out.is_stmt ? " NS" : "");

    if(in.file_number != out.file_number) {
        cout << " url: ";
        cout << lns_file_names[(size_t)out.file_number].name;
    }

    if(out.new_statement) {
        if(in.discriminator != 0) {
            cout << " DI=0x";
            cout.width(0);
            cout << hex << in.discriminator;
        }
        if(out.end_sequence) {
            cout << " ET";
        }
    } else {
        if(in.end_sequence != out.end_sequence) {
            cout << " DI=0x";
            cout.width(0);
            cout << hex << out.discriminator;
        }
        if(in.end_sequence != out.end_sequence AND out.end_sequence) {
            cout << " ET";
        }
    }


    cout << endl;
}

void LNS_STATE_MACHINE::Run(char *line_data)
{
    CPROFILER;

    static const bool show_your_work = false;

    line_address_matrix.clear();    // Make sure our output is empty

    // Get hold of the DWARF Version 4 documentation.  Look for section 6.2.4
    ReadProgramHeader(line_data);
    current_registers.SetDefaultIsStmt(default_is_stmt);

    if(show_your_work) {
        PutUpLnsHeader();
    }

    bool first_time = true;
    while(line_program_data < line_program_data_end) {
        LNS_REGISTERS starting_registers = current_registers;
        current_registers.new_statement = false;
        if(first_time) {
            first_time = false;
            // Change the .file value to force the display of the file's URI
            starting_registers.file_number -= 1;
        }
        UBYTE opcode = GetUbyte(line_program_data);
        UBYTE extended_opcode = 0;
        ULONGLONG extended_opcode_length=0;

        bool just_did_end_sequence = false;
        if(opcode >= opcode_base) {
            ProcessSpecialOpcode(opcode);
        } else {
            ULONGLONG operation_advance;
            LONGLONG advance_line;
            ULONGLONG file_number;
            ULONGLONG column_number;
            USHORT address_increment;
            ULONGLONG new_isa;
            ULONGLONG relocatable_address;
            ULONGLONG new_discriminator;
            switch(opcode) {
            case DW_LNS_copy:
                line_address_matrix.push_back(current_registers);
                current_registers.new_statement = true;
                current_registers.basic_block = false;
                current_registers.prologue_end = false;
                current_registers.epilogue_begin = false;
                current_registers.discriminator = 0;
                break;

            case DW_LNS_advance_pc:
                operation_advance = ULEB128Decode(line_program_data);
                current_registers.address += minimum_instruction_length * ((current_registers.op_index + operation_advance)/maximum_operations_per_instruction);
                current_registers.op_index = (current_registers.op_index + operation_advance) % maximum_operations_per_instruction;
                break;

            case DW_LNS_advance_line:
                advance_line = LEB128Decode(line_program_data);
                current_registers.line += advance_line;
                break;

            case DW_LNS_set_file:
                file_number = ULEB128Decode(line_program_data);
                current_registers.file_number = file_number;
                break;

            case DW_LNS_set_column:
                column_number = ULEB128Decode(line_program_data);
                current_registers.column = column_number;
                break;

            case DW_LNS_negate_stmt:
                current_registers.is_stmt = !current_registers.is_stmt;
                break;

            case DW_LNS_set_basic_block:
                current_registers.basic_block = true;
                break;

            case DW_LNS_const_add_pc: {
                ULONGLONG delta_address;
                LONGLONG delta_line;
                ULONGLONG new_op_index;
                DecodeSpecialOpcode(255,delta_address,delta_line,new_op_index);
                current_registers.address += delta_address;
            }
            break;

            case DW_LNS_fixed_advance_pc:
                address_increment = GetUshort(line_program_data);
                current_registers.address += address_increment;
                break;

            case DW_LNS_set_prologue_end:
                current_registers.prologue_end = true;
                break;

            case DW_LNS_set_epilogue_begin:
                current_registers.epilogue_begin = true;
                break;

            case DW_LNS_set_isa:
                new_isa = ULEB128Decode(line_program_data);
                current_registers.isa = new_isa;
                break;

            case 0: {
                extended_opcode_length = ULEB128Decode(line_program_data);
                extended_opcode = GetUbyte(line_program_data);
                switch(extended_opcode) {
                case DW_LNE_end_sequence:
                    just_did_end_sequence = true;
                    current_registers.end_sequence = true;
                    line_address_matrix.push_back(current_registers);

                    current_registers.new_statement = true;
                    if(show_your_work) {
                        StateMachineDifference(opcode, extended_opcode, starting_registers, current_registers);
                    }

                    current_registers.Init();
                    break;

                case DW_LNE_set_address:
                    // This is actually moot; because this is a relocatable module, the address we are
                    // about to pick up is going to be zero.  But I suppose I might be wrong:

                    // Assume 64-bit addressing:

                    relocatable_address = *(ULONGLONG *)(line_program_data);
                    line_program_data += 8;

                    current_registers.address = relocatable_address;
                    current_registers.op_index = 0;
                    break;

                case DW_LNE_define_file: {
                    LNS_FILE_NAME lns_file_name;
                    while(line_program_data < line_program_data_end) {
                        char ch = *line_program_data++;
                        lns_file_name.name = ch;    // Start a new name
                        while(line_program_data < line_program_data_end) {  // 12.
                            ch = *line_program_data++;
                            if(ch EQ '\0') {
                                break;
                            }
                            lns_file_name.name += ch;
                        }
                        lns_file_name.include_directory_index = ULEB128Decode(line_program_data);
                        lns_file_name.time_modified = ULEB128Decode(line_program_data);
                        lns_file_name.file_length = ULEB128Decode(line_program_data);
                        lns_file_names.push_back(lns_file_name);
                    }
                }
                break;

                case DW_LNE_set_discriminator:
                    new_discriminator = ULEB128Decode(line_program_data);
                    current_registers.discriminator = new_discriminator;
                    break;

                default:
                    cerr << "Unknown extended opcode in LNS_STATE_MACHINE::Run(): " << (int)extended_opcode << "\n";
                    ::exit(1);
                    break;
                }
            }
            break;
            default:
                cerr << "Unknown opcode in LNS_STATE_MACHINE::Run()\n";
                ::exit(1);
                break;
            }
        }
        if(show_your_work) {
            if(!just_did_end_sequence) {
                StateMachineDifference(opcode, extended_opcode, starting_registers, current_registers);
            }
        }
    }
}

void LNS_BUILDER::SetParams(UBYTE ml, UBYTE mopi, BYTE lb, UBYTE lr, UBYTE ob, bool def_is_stmt, const UBYTE *sol)
{
    minimum_instruction_length = ml;
    maximum_operations_per_instruction = mopi;
    line_base = lb;
    line_range = lr;
    opcode_base = ob;
    default_is_stmt = def_is_stmt;
    memcpy(standard_opcode_lengths,sol,sizeof(standard_opcode_lengths));
}

int LNS_BUILDER::AddFile(const std::string &filename)
{
    LNS_FILE_NAME lfn;
    if(lns_file_names.size() EQ 0) {
        // The zeroth entry is empty; it implies the compilation directory
        lns_file_names.push_back(lfn);
    }
    int retval = -1;
    VLNS_FILE_NAMES::const_iterator it = lns_file_names.begin();
    while(it != lns_file_names.end()) {
        if(it->name EQ filename) {
            break;
        }
        it++;
    }
    if(it EQ lns_file_names.end()) {
        // This is a never-before-seen filename, so append it.
        lfn.name = filename;
        lfn.include_directory_index = 0;
        lfn.time_modified = 0;
        lfn.file_length = 0;
        lns_file_names.push_back(lfn);
        retval = (int)lns_file_names.size() - 1;
    } else {
        retval = (int)(it - lns_file_names.begin());
    }
    return retval;
}

void LNS_BUILDER::AddToAddressLineList(ULONGLONG address, int file_number, ULONGLONG line)
{
    ADDRESS_LINE_PAIR alp;
    alp.file_number = file_number;
    alp.address = address;
    alp.line = line;
    address_line_pairs.push_back(alp);
}

void LNS_BUILDER::PushValues(const UBYTE *p,size_t length)
{
    while(length-- > 0) {
        line_program.push_back(*p++);
    }
}

void LNS_BUILDER::PushValues(vector<UBYTE>vals)
{
    size_t i = 0;
    while( i < vals.size() ) {
        line_program.push_back(vals[i++]);
    }
}

template<typename T>void LNS_BUILDER::Overwrite(ULONGLONG location,T v)
{
    const UBYTE *src = (const UBYTE *)(&v);
    for(size_t i=0; i<sizeof(T); i++) {
        line_program[(size_t)location++] = *src++;
    }
}

template<typename T>void LNS_BUILDER::PushValue(T v)
{
    PushValues((const UBYTE *)(&v),sizeof(T));
}

void LNS_BUILDER::PushULEB128(ULONGLONG v)
{
    vector<UBYTE>vals = ULEB128Encode(v);
    PushValues(vals);
}

void LNS_BUILDER::PushLEB128(LONGLONG v)
{
    vector<UBYTE>vals = LEB128Encode(v);
    PushValues(vals);
}

vector<UBYTE> LNS_BUILDER::BuildProgram()
{
    CPROFILER;
    line_program.clear();

    // Referring to section 6.2.4 of the DWARF-2 line program format....

    // Stash the location of the total_length, so we can fill it in later
    size_t total_length_address = line_program.size();
    PushValue((ULONG)0);    // total_length.  Assume for now, files less than 2^32 bytes in length

    // Assume we are doing version 2 line address programming
    PushValue((USHORT)(2)); // As I said:  Version 2

    // Stash the location of the prologue length; we'll be filling it in later
    size_t prologue_length_address = line_program.size();  // We'll fill this length in later
    PushValue((ULONG)0);    // Prologue length

    // Use the minimum instruction length we were given
    PushValue(minimum_instruction_length);

    // DWARF-2 assumes that maximum_operations_per_instruction is 1
    // DWARF-3 introduces that value as something other than 1.

    // Establish the default is_stmt value
    PushValue( (UBYTE)(default_is_stmt ? 1 : 0) );

    // The line_base, line_range and opcode_base, each one byte in length
    PushValue(line_base);
    PushValue(line_range);
    PushValue(opcode_base);

    // Pass along the standard_opcode_lengths we read from the file in the first place
    for(int i=1; i<opcode_base; i++) {
        PushValue(standard_opcode_lengths[i]);
    }

    // Next up: include directories.

    // We don't have any. If we did have any, they go here, as a series of
    // null-terminated strings.

    // The last entry is followed by a single null byte.
    PushValue((UBYTE)0);

    // Next, we have the source file names.  We probably have just the one at index [1]; the
    // one at index [0] is a null entry, since DWARF does its numbering from 1.
    for(size_t i=1; i<lns_file_names.size(); i++) {
        for(size_t j=0; j<lns_file_names[i].name.size(); j++) {
            PushValue(lns_file_names[i].name[j]);
        }
        PushValue((UBYTE)0);    // This terminates the string
        PushULEB128(lns_file_names[i].include_directory_index);
        PushULEB128(lns_file_names[i].time_modified);
        PushULEB128(lns_file_names[i].file_length);
    }

    // When GDB loads, it defaults to showing you the last file in the list.
    // We (probably) want that to be the first, so if we have more than one
    // (recalling the first is a dummy) duplicate [1] at the end.

    if(lns_file_names.size() > 2) {
        for(size_t j=0; j<lns_file_names[1].name.size(); j++) {
            PushValue(lns_file_names[1].name[j]);
        }
        PushValue((UBYTE)0);    // This terminates the string
        PushULEB128(lns_file_names[1].include_directory_index);
        PushULEB128(lns_file_names[1].time_modified);
        PushULEB128(lns_file_names[1].file_length);
    }
    PushValue((UBYTE)0);    // This terminates the list of files

    // Determine the maximum address jump we can do with a special instruction:
    ULONGLONG const_add_pc_delta = (255-opcode_base) / line_range;
    ULONGLONG addr_threshold = const_add_pc_delta*2;        // See the DWARF spec notes on DW_LNS_const_add_pc in section 6.2.5.2
    LONGLONG line_threshold = line_base + line_range - 1;   // Beyond line_threshold, we have to use DW_LNS_advance_line

    LNS_REGISTERS registers_current(default_is_stmt);
    LNS_REGISTERS registers_target(default_is_stmt);

    // We are about to start writing the actual program.  That means we know what we need to
    // update the prologue_length.  The prologue length gives the number
    // of bytes *after* the prologue length field, which explains the -4 in the
    // next line:

    ULONG prologue_length = static_cast<ULONG>(line_program.size() - prologue_length_address - 4);
    Overwrite(prologue_length_address,prologue_length);

    // We now have everything we need to create the list of state machine instructions:
    // Prime the pump with the entry for the address relocation
    PushValue((UBYTE)0);                    // Extended opcode sequence
    PushValue((UBYTE)9);                    // Nine bytes to follow
    PushValue((UBYTE)DW_LNE_set_address);   // The extended opcode

    // Stash the location of the relocatable address;  Whoever called us is going
    // to need it to adjust the related entry in .rela_debug_line
    relocation_address_offset = line_program.size();
    PushValue(ULONGLONG(0));    // Eight-byte relocatable address
    // We are assuming that the relocatable address is zero when we process it
    // If that turns out to be untrue, then much of what we are doing here won't
    // work.

    //  At this point in development, we are going to do nothing except bump along the addresses and line count.
    //  Nonetheless, I am going to make the code a little more complex than that
    //  I am making the code a little more complex than that, in anticipation that someday we might do a little
    //  more than that.  Just in case.

    size_t next_pair = 0;

    if(address_line_pairs.size() >= 1) {
        // The while() logic immediately below doesn't process the final entry in
        // the address_line_pairs vector.  To get around that, I duplicate the
        // final entry here.
        address_line_pairs.push_back(address_line_pairs.back());

        // Prime the pump.
        registers_target.address = address_line_pairs[next_pair].address;
        registers_target.file_number = address_line_pairs[next_pair].file_number;
        registers_target.line = address_line_pairs[next_pair].line;
        next_pair += 1;
    }

    while(next_pair < address_line_pairs.size()) {
        // First, check all of the flags and values that might have to change
        // before doing a DW_LNS_copy

        // (Note: As of 2019-06-27, there actually is no provision, in this program,
        //  for changing any of those flags.  But I figured I'd put the code in while
        //  it was fresh in my mind.  (Note further that this inevitably means none
        //  of it has been tested yet.  -- RJD))

        if(registers_current.basic_block != registers_target.basic_block) {
            PushValue((UBYTE)DW_LNS_set_basic_block);
            registers_current.basic_block = registers_target.basic_block;
            continue;
        }
        if(registers_current.column != registers_target.column) {
            PushValue((UBYTE)DW_LNS_set_column);
            PushULEB128(registers_target.column);
            registers_current.column = registers_target.column;
            continue;
        }
        if(registers_current.discriminator != registers_target.discriminator) {
            vector<UBYTE>vub = ULEB128Encode(registers_target.discriminator);
            ULONGLONG size = 1 + vub.size();
            PushValue((UBYTE)0);
            PushULEB128(size);
            PushValue((UBYTE)DW_LNE_set_discriminator);
            PushULEB128(registers_target.discriminator);
            registers_current.discriminator = registers_target.discriminator;
            continue;
        }
        if(registers_current.epilogue_begin != registers_target.epilogue_begin) {
            PushValue((UBYTE)DW_LNS_set_epilogue_begin);
            registers_current.epilogue_begin = registers_target.epilogue_begin;
            continue;
        }
        if(registers_current.file_number != registers_target.file_number) {
            PushValue((UBYTE)DW_LNS_set_file);
            PushULEB128(registers_target.file_number);
            registers_current.file_number = registers_target.file_number;
            continue;
        }
        if(registers_current.isa != registers_target.isa) {
            PushValue((UBYTE)DW_LNS_set_isa);
            PushULEB128(registers_target.isa);
            registers_current.isa = registers_target.isa;
            continue;
        }
        if(registers_current.is_stmt != registers_target.is_stmt) {
            PushValue((UBYTE)DW_LNS_negate_stmt);
            registers_current.is_stmt = registers_target.is_stmt;
            continue;
        }
        if(registers_current.prologue_end != registers_target.prologue_end) {
            PushValue((UBYTE)DW_LNS_set_prologue_end);
            registers_current.prologue_end = registers_target.prologue_end;
            continue;
        }

        // After all the flags, we now look at .address and .line.  If, at this point,
        // the target matches current, it's because we've gone through an exercise that
        // didn't involve special opcodes.  That means it's now time to DW_LNS_copy:

        if(     registers_current.address EQ registers_target.address
                AND registers_current.line    EQ registers_target.line      ) {
            PushValue((UBYTE)DW_LNS_copy);

            // Having done that DW_LNS_copy, we need to reset some of the registers:

            registers_current.isa            = registers_target.isa             = 0;
            registers_current.basic_block    = registers_target.basic_block     = false;
            registers_current.prologue_end   = registers_target.prologue_end    = false;
            registers_current.epilogue_begin = registers_target.epilogue_begin  = false;

            // And we need to establish the new address/line target:
            registers_target.address = address_line_pairs[next_pair].address;
            registers_target.file_number = address_line_pairs[next_pair].file_number;
            registers_target.line = address_line_pairs[next_pair].line;
            next_pair += 1;
            continue;
        }

        // At this point, the current/target flags are all the same, but the address or line
        // must be different.
        LONGLONG delta_line = registers_target.line - registers_current.line;

        if(delta_line > line_threshold) {
            PushValue((UBYTE)DW_LNS_advance_line);
            PushLEB128(delta_line);
            registers_current.line += delta_line; // After the addition, current should be the same as target
            continue;
        }

        ULONGLONG delta_addr = registers_target.address - registers_current.address;

        LONGLONG opcode = (delta_line - line_base) + line_range * delta_addr + opcode_base;

        if(opcode >= opcode_base AND opcode <= 255) {
            // We have us a special opcode situation:
            PushValue((UBYTE)opcode);

            // special opcode operations implicitly perform a DW_LNS_copy:

            // Update the current address/line values:
            registers_current.address = registers_target.address ;
            registers_current.line    = registers_target.line    ;

            // Having done that DW_LNS_copy, we need to reset some of the registers:

            registers_current.isa            = registers_target.isa             = 0;
            registers_current.basic_block    = registers_target.basic_block     = false;
            registers_current.prologue_end   = registers_target.prologue_end    = false;
            registers_current.epilogue_begin = registers_target.epilogue_begin  = false;

            // And we need to establish the new address/line target:
            registers_target.address = address_line_pairs[next_pair].address;
            registers_target.file_number = address_line_pairs[next_pair].file_number;
            registers_target.line = address_line_pairs[next_pair].line;
            next_pair += 1;
            continue;
        }

        // We didn't handle the new address/line with a special opcode, which means
        // we need to proceed with standard instructions:

        if(delta_addr != 0 AND delta_addr < addr_threshold) {
            // It's appropriate to do a DW_LNS_const_add_pc
            PushValue((UBYTE)DW_LNS_const_add_pc);
            registers_current.address += const_add_pc_delta;
            continue;
        }
        if(delta_addr != 0) {
            // Actually, the if() shouldn't be necessary.  The only possibility left is that
            // the delta_address is big enough to require DW_LNS_advance_pc
            PushValue((UBYTE)DW_LNS_advance_pc);
            PushULEB128(delta_addr);
            registers_current.address += delta_addr; // After the addition, current should be the same as target
            continue;
        }

        // The upshot is that arriving here should be an impossibility

        cerr << "This cannot be!  Figure out what's happening in LNS_BUILDER::BuildProgram()\n";
        exit(1);
    }

    // We have hit the end of the road:
    PushValue((UBYTE)0);                    // Extended opcode sequence
    PushValue((UBYTE)1);                    // Nine bytes to follow
    PushValue((UBYTE)DW_LNE_end_sequence);  // The extended opcode

    // Back at the beginning of the main while() loop, we duplicated the final entry.
    // Restore the vector to the way it was when this routine was entered. I don't know
    // why that would matter, but it seems only courteous.
    address_line_pairs.pop_back();

    // We can now update the total length field back at the beginning:
    // (Note that total_length_address should be zero.  I put it into a variable
    //  for self-documenting clarity.  It says here.)
    ULONG total_length = static_cast<ULONG>(line_program.size() -  total_length_address - 4); // Not including the total_length field itself
    Overwrite(total_length_address,total_length);

    return line_program;
}

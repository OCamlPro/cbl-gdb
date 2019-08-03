#pragma once

#include <string>

typedef enum {
    DW_TAG_NONE = 0x00,
    DW_TAG_array_type = 0x01,
    DW_TAG_class_type = 0x02,
    DW_TAG_entry_point = 0x03,
    DW_TAG_enumeration_type = 0x04,
    DW_TAG_formal_parameter = 0x05,
    DW_TAG_imported_declaration = 0x08,
    DW_TAG_label = 0x0a,
    DW_TAG_lexical_block = 0x0b,
    DW_TAG_member = 0x0d,
    DW_TAG_pointer_type = 0x0f,
    DW_TAG_reference_type = 0x10,
    DW_TAG_compile_unit = 0x11,
    DW_TAG_string_type = 0x12,
    DW_TAG_structure_type = 0x13,
    DW_TAG_subroutine_type = 0x15,
    DW_TAG_typedef = 0x16,
    DW_TAG_union_type = 0x17,
    DW_TAG_unspecified_parameters = 0x18,
    DW_TAG_variant = 0x19,
    DW_TAG_common_block = 0x1a,
    DW_TAG_common_inclusion = 0x1b,
    DW_TAG_inheritance = 0x1c,
    DW_TAG_inlined_subroutine = 0x1d,
    DW_TAG_module = 0x1e,
    DW_TAG_ptr_to_member_type = 0x1f,
    DW_TAG_set_type = 0x20,
    DW_TAG_subrange_type = 0x21,
    DW_TAG_with_stmt = 0x22,
    DW_TAG_access_declaration = 0x23,
    DW_TAG_base_type = 0x24,
    DW_TAG_catch_block = 0x25,
    DW_TAG_const_type = 0x26,
    DW_TAG_constant = 0x27,
    DW_TAG_enumerator = 0x28,
    DW_TAG_file_type = 0x29,
    DW_TAG_friend = 0x2a,
    DW_TAG_namelist = 0x2b,
    DW_TAG_namelist_item = 0x2c,
    DW_TAG_packed_type = 0x2d,
    DW_TAG_subprogram = 0x2e,
    DW_TAG_template_type_parameter = 0x2f,
    DW_TAG_template_value_parameter = 0x30,
    DW_TAG_thrown_type = 0x31,
    DW_TAG_try_block = 0x32,
    DW_TAG_variant_part = 0x33,
    DW_TAG_variable = 0x34,
    DW_TAG_volatile_type = 0x35,
    DW_TAG_dwarf_procedure = 0x36,
    DW_TAG_restrict_type = 0x37,
    DW_TAG_interface_type = 0x38,
    DW_TAG_namespace = 0x39,
    DW_TAG_imported_module = 0x3a,
    DW_TAG_unspecified_type = 0x3b,
    DW_TAG_partial_unit = 0x3c,
    DW_TAG_imported_unit = 0x3d,
    DW_TAG_condition = 0x3f,
    DW_TAG_shared_type = 0x40,
    DW_TAG_type_unit = 0x41,
    DW_TAG_rvalue_reference_type = 0x42,
    DW_TAG_template_alias = 0x43,
    DW_TAG_lo_user = 0x4080,
    DW_TAG_hi_user = 0xffff,
} DW_TAG;

typedef enum {
    DW_CHILDREN_no  = 0x00,
    DW_CHILDREN_yes	= 0x01,
} DW_CHILDREN;

typedef enum {
    DW_AT_sibling = 0x01,                   //  reference
    DW_AT_location = 0x02,                  //  exprloc, loclistptr
    DW_AT_name = 0x03,                      //  string
    DW_AT_ordering = 0x09,                  //  constant
    DW_AT_byte_size = 0x0b,                 //  constant, exprloc, reference
    DW_AT_bit_offset = 0x0c,                //  constant, exprloc, reference
    DW_AT_bit_size = 0x0d,                  //  constant, exprloc, reference
    DW_AT_stmt_list = 0x10,                 //  lineptr
    DW_AT_low_pc = 0x11,                    //  address
    DW_AT_high_pc = 0x12,                   //  address, constant
    DW_AT_language = 0x13,                  //  constant
    DW_AT_discr = 0x15,                     //  reference
    DW_AT_discr_value = 0x16,               //  constant
    DW_AT_visibility = 0x17,                //  constant
    DW_AT_import = 0x18,                    //  reference
    DW_AT_string_length = 0x19,             //  exprloc, loclistptr
    DW_AT_common_reference = 0x1a,          //  reference
    DW_AT_comp_dir = 0x1b,                  //  string
    DW_AT_const_value = 0x1c,               //  block, constant, string
    DW_AT_containing_type = 0x1d,           //  reference
    DW_AT_default_value = 0x1e,             //  reference
    DW_AT_inline = 0x20,                    //  constant
    DW_AT_is_optional = 0x21,               //  flag
    DW_AT_lower_bound = 0x22,               //  constant, exprloc, reference
    DW_AT_producer = 0x25,                  //  string
    DW_AT_prototyped = 0x27,                //  flag
    DW_AT_return_addr = 0x2a,               //  exprloc, loclistptr
    DW_AT_start_scope = 0x2c,               //  Constant, rangelistptr
    DW_AT_bit_stride = 0x2e,                //  constant, exprloc, reference
    DW_AT_upper_bound = 0x2f,               //  constant, exprloc, reference
    DW_AT_abstract_origin = 0x31,           //  reference
    DW_AT_accessibility = 0x32,             //  constant
    DW_AT_address_class = 0x33,             //  constant
    DW_AT_artificial = 0x34,                //  flag
    DW_AT_base_types = 0x35,                //  reference
    DW_AT_calling_convention = 0x36,        //  constant
    DW_AT_count = 0x37,                     //  constant, exprloc, reference
    DW_AT_data_member_location = 0x38,      //  constant, exprloc, loclistptr
    DW_AT_decl_column = 0x39,               //  constant
    DW_AT_decl_file = 0x3a,                 //  constant
    DW_AT_decl_line = 0x3b,                 //  constant
    DW_AT_declaration = 0x3c,               //  flag
    DW_AT_discr_list = 0x3d,                //  block
    DW_AT_encoding = 0x3e,                  //  constant
    DW_AT_external = 0x3f,                  //  flag
    DW_AT_frame_base = 0x40,                //  exprloc, loclistptr
    DW_AT_friend = 0x41,                    //  reference
    DW_AT_identifier_case = 0x42,           //  constant
    DW_AT_macro_info = 0x43,                //  macptr
    DW_AT_namelist_item = 0x44,             //  reference
    DW_AT_priority = 0x45,                  //  reference
    DW_AT_segment = 0x46,                   //  exprloc, loclistptr
    DW_AT_specification = 0x47,             //  reference
    DW_AT_static_link = 0x48,               //  exprloc, loclistptr
    DW_AT_type = 0x49,                      //  reference
    DW_AT_use_location = 0x4a,              //  exprloc, loclistptr
    DW_AT_variable_parameter = 0x4b,        //  flag
    DW_AT_virtuality = 0x4c,                //  constant
    DW_AT_vtable_elem_location = 0x4d,      //  exprloc, loclistptr
    DW_AT_allocated = 0x4e,                 //  constant, exprloc, reference
    DW_AT_associated = 0x4f,                //  constant, exprloc, reference
    DW_AT_data_location = 0x50,             //  exprloc
    DW_AT_byte_stride = 0x51,               //  constant, exprloc, reference
    DW_AT_entry_pc = 0x52,                  //  address
    DW_AT_use_UTF8 = 0x53,                  //  flag
    DW_AT_extension = 0x54,                 //  reference
    DW_AT_ranges = 0x55,                    //  rangelistptr
    DW_AT_trampoline = 0x56,                //  address, flag, reference, string
    DW_AT_call_column = 0x57,               //  constant
    DW_AT_call_file = 0x58,                 //  constant
    DW_AT_call_line = 0x59,                 //  constant
    DW_AT_description = 0x5a,               //  string
    DW_AT_binary_scale = 0x5b,              //  constant
    DW_AT_decimal_scale = 0x5c,             //  constant
    DW_AT_small = 0x5d,                     //  reference
    DW_AT_decimal_sign = 0x5e,              //  constant
    DW_AT_digit_count = 0x5f,               //  constant
    DW_AT_picture_string = 0x60,            //  string
    DW_AT_mutable = 0x61,                   //  flag
    DW_AT_threads_scaled = 0x62,            //  flag
    DW_AT_explicit = 0x63,                  //  flag
    DW_AT_object_pointer = 0x64,            //  reference
    DW_AT_endianity = 0x65,                 //  constant
    DW_AT_elemental = 0x66,                 //  flag
    DW_AT_pure = 0x67,                      //  flag
    DW_AT_recursive = 0x68,                 //  flag
    DW_AT_signature = 0x69,                 //  reference
    DW_AT_main_subprogram = 0x6a,           //  flag
    DW_AT_data_bit_offset = 0x6b,           //  constant
    DW_AT_const_expr = 0x6c,                //  flag
    DW_AT_enum_class = 0x6d,                //  flag
    DW_AT_linkage_name = 0x6e,              //  string
    DW_AT_lo_user = 0x2000,                 //  ---
    DW_AT_hi_user = 0x3fff,                 //  ---
} DW_AT;

typedef enum {
    DW_FORM_NONE = 0,
    DW_FORM_addr = 0x01,           //  address
    DW_FORM_block2 = 0x03,         //  block
    DW_FORM_block4 = 0x04,         //  block
    DW_FORM_data2 = 0x05,          //  constant
    DW_FORM_data4 = 0x06,          //  constant
    DW_FORM_data8 = 0x07,          //  constant
    DW_FORM_string = 0x08,         //  string
    DW_FORM_block = 0x09,          //  block
    DW_FORM_block1 = 0x0a,         //  block
    DW_FORM_data1 = 0x0b,          //  constant
    DW_FORM_flag = 0x0c,           //  flag
    DW_FORM_sdata = 0x0d,          //  constant
    DW_FORM_strp = 0x0e,           //  string
    DW_FORM_udata = 0x0f,          //  constant
    DW_FORM_ref_addr = 0x10,       //  reference
    DW_FORM_ref1 = 0x11,           //  reference
    DW_FORM_ref2 = 0x12,           //  reference
    DW_FORM_ref4 = 0x13,           //  reference
    DW_FORM_ref8 = 0x14,           //  reference
    DW_FORM_ref_udata = 0x15,      //  reference
    DW_FORM_indirect = 0x16,       //  (see Section 7.5.3)
    DW_FORM_sec_offset = 0x17,     //  lineptr, loclistptr, macptr, rangelistptr
    DW_FORM_exprloc = 0x18,        //  exprloc
    DW_FORM_flag_present = 0x19,   //  flag
    DW_FORM_ref_sig8 = 0x20,       //  reference
} DW_FORM;

typedef enum {
    DW_OP_addr = 0x03,
    DW_OP_deref = 0x06,
    DW_OP_const1u = 0x08,
    DW_OP_const1s = 0x09,
    DW_OP_const2u = 0x0a,
    DW_OP_const2s = 0x0b,
    DW_OP_const4u = 0x0c,
    DW_OP_const4s = 0x0d,
    DW_OP_const8u = 0x0e,
    DW_OP_const8s = 0x0f,
    DW_OP_constu = 0x10,
    DW_OP_consts = 0x11,
    DW_OP_dup = 0x12,
    DW_OP_drop = 0x13,
    DW_OP_over = 0x14,
    DW_OP_pick = 0x15,
    DW_OP_swap = 0x16,
    DW_OP_rot = 0x17,
    DW_OP_xderef = 0x18,
    DW_OP_abs = 0x19,
    DW_OP_and = 0x1a,
    DW_OP_div = 0x1b,
    DW_OP_minus = 0x1c,
    DW_OP_mod = 0x1d,
    DW_OP_mul = 0x1e,
    DW_OP_neg = 0x1f,
    DW_OP_not = 0x20,
    DW_OP_or = 0x21,
    DW_OP_plus = 0x22,
    DW_OP_plus_uconst = 0x23,
    DW_OP_shl = 0x24,
    DW_OP_shr = 0x25,
    DW_OP_shra = 0x26,
    DW_OP_xor = 0x27,
    DW_OP_skip = 0x2f,
    DW_OP_bra = 0x28,
    DW_OP_eq = 0x29,
    DW_OP_ge = 0x2a,
    DW_OP_gt = 0x2b,
    DW_OP_le = 0x2c,
    DW_OP_lt = 0x2d,
    DW_OP_ne = 0x2e,
    DW_OP_lit0 = 0x30,
    DW_OP_lit1 = 0x31,
    DW_OP_lit31 = 0x4f,
    DW_OP_reg0 = 0x50,
    DW_OP_reg1 = 0x51,
    DW_OP_reg31 = 0x6f,
    DW_OP_breg0 = 0x70,
    DW_OP_breg1 = 0x71,
    DW_OP_breg31 = 0x8f,
    DW_OP_regx = 0x90,
    DW_OP_fbreg = 0x91,
    DW_OP_bregx = 0x92,
    DW_OP_piece = 0x93,
    DW_OP_deref_size = 0x94,
    DW_OP_xderef_size = 0x95,
    DW_OP_nop = 0x96,
    DW_OP_push_object_address = 0x97,
    DW_OP_call2 = 0x98,
    DW_OP_call4 = 0x99,
    DW_OP_call_ref = 0x9a,
    DW_OP_form_tls_address = 0x9b,
    DW_OP_call_frame_cfa = 0x9c,
    DW_OP_bit_piece = 0x9d,
    DW_OP_implicit_value = 0x9e,
    DW_OP_stack_value = 0x9f,
    DW_OP_lo_user = 0xe0,
    DW_OP_hi_user = 0xff,
} DW_OP;

typedef enum {
    DW_ATE_address = 0x01,
    DW_ATE_boolean = 0x02,
    DW_ATE_complex_float = 0x03,
    DW_ATE_float = 0x04,
    DW_ATE_signed = 0x05,
    DW_ATE_signed_char = 0x06,
    DW_ATE_unsigned = 0x07,
    DW_ATE_unsigned_char = 0x08,
    DW_ATE_imaginary_float = 0x09,
    DW_ATE_packed_decimal = 0x0a,
    DW_ATE_numeric_string = 0x0b,
    DW_ATE_edited = 0x0c,
    DW_ATE_signed_fixed = 0x0d,
    DW_ATE_unsigned_fixed = 0x0e,
    DW_ATE_decimal_float = 0x0f,
    DW_ATE_UTF = 0x10,
    DW_ATE_lo_user = 0x80,
    DW_ATE_hi_user = 0xff,
} DW_ATE;

typedef enum {
    DW_DS_unsigned = 0x01,
    DW_DS_leading_overpunch = 0x02,
    DW_DS_trailing_overpunch = 0x03,
    DW_DS_leading_separate = 0x04,
    DW_DS_trailing_separate = 0x05,
} DW_DS;

typedef enum {
    DW_END_default = 0x00,
    DW_END_big = 0x01,
    DW_END_little = 0x02,
    DW_END_lo_user = 0x40,
    DW_END_hi_user = 0xff,
} DW_END;

typedef enum {
    DW_ACCESS_public = 0x01,
    DW_ACCESS_protected = 0x02,
    DW_ACCESS_private = 0x03,
} DW_ACCESS;

typedef enum {
    DW_VIS_local = 0x01,
    DW_VIS_exported = 0x02,
    DW_VIS_qualified = 0x03,
} DW_VIS;

typedef enum {
    DW_VIRTUALITY_none = 0x00,
    DW_VIRTUALITY_virtual = 0x01,
    DW_VIRTUALITY_pure_virtual = 0x02,
} DW_VIRTUALITY;

typedef enum {
    DW_LANG_C89 = 0x0001,
    DW_LANG_C = 0x0002,
    DW_LANG_Ada83 = 0x0003,
    DW_LANG_C_plus_plus = 0x0004,
    DW_LANG_Cobol74 = 0x0005,
    DW_LANG_Cobol85 = 0x0006,
    DW_LANG_Fortran77 = 0x0007,
    DW_LANG_Fortran90 = 0x0008,
    DW_LANG_Pascal83 = 0x0009,
    DW_LANG_Modula2 = 0x000a,
    DW_LANG_Java = 0x000b,
    DW_LANG_C99 = 0x000c,
    DW_LANG_Ada95 = 0x000d,
    DW_LANG_Fortran95 = 0x000e,
    DW_LANG_PLI = 0x000f,
    DW_LANG_ObjC = 0x0010,
    DW_LANG_ObjC_plus_plus = 0x0011,
    DW_LANG_UPC = 0x0012,
    DW_LANG_D = 0x0013,
    DW_LANG_Python = 0x0014,
    DW_LANG_lo_user = 0x8000,
    DW_LANG_hi_user = 0xffff,
} DW_LANG;

typedef enum {
    DW_ID_case_sensitive = 0x00,
    DW_ID_up_case = 0x01,
    DW_ID_down_case = 0x02,
    DW_ID_case_insensitive = 0x03,
} DW_ID;

typedef enum {
    DW_CC_normal = 0x01,
    DW_CC_program = 0x02,
    DW_CC_nocall = 0x03,
    DW_CC_lo_user = 0x40,
    DW_CC_hi_user = 0xff,
} DW_CC;

typedef enum {
    DW_INL_not_inlined = 0x00,
    DW_INL_inlined = 0x01,
    DW_INL_declared_not_inlined = 0x02,
    DW_INL_declared_inlined = 0x03,
} DW_INL;

typedef enum {
    DW_ORD_row_major = 0x00,
    DW_ORD_col_major = 0x01,
} DW_ORD;

typedef enum {
    DW_DSC_label = 0x00,
    DW_DSC_range = 0x01,
} DW_DSC;

typedef enum {
    DW_LNS_copy = 0x01,
    DW_LNS_advance_pc = 0x02,
    DW_LNS_advance_line = 0x03,
    DW_LNS_set_file = 0x04,
    DW_LNS_set_column = 0x05,
    DW_LNS_negate_stmt = 0x06,
    DW_LNS_set_basic_block = 0x07,
    DW_LNS_const_add_pc = 0x08,
    DW_LNS_fixed_advance_pc = 0x09,
    DW_LNS_set_prologue_end = 0x0a,
    DW_LNS_set_epilogue_begin = 0x0b,
    DW_LNS_set_isa = 0x0c,
} DW_LNS;

typedef enum {
    DW_LNE_end_sequence = 0x01,
    DW_LNE_set_address = 0x02,
    DW_LNE_define_file = 0x03,
    DW_LNE_set_discriminator = 0x04,
    DW_LNE_lo_user = 0x80,
    DW_LNE_hi_user = 0xff,
} DW_LNE;

typedef enum {
    DW_MACINFO_define = 0x01,
    DW_MACINFO_undef = 0x02,
    DW_MACINFO_start_file = 0x03,
    DW_MACINFO_end_file = 0x04,
    DW_MACINFO_vendor_ext = 0xff,
} DW_MACINFO;

typedef enum {
    DW_CFA_nop = 0x00,
    DW_CFA_set_loc = 0x01,
    DW_CFA_advance_loc1 = 0x02,
    DW_CFA_advance_loc2 = 0x03,
    DW_CFA_advance_loc4 = 0x04,
    DW_CFA_offset_extended = 0x05,
    DW_CFA_restore_extended = 0x06,
    DW_CFA_undefined = 0x07,
    DW_CFA_same_value = 0x08,
    DW_CFA_register = 0x09,
    DW_CFA_remember_state = 0x0a,
    DW_CFA_restore_state = 0x0b,
    DW_CFA_def_cfa = 0x0c,
    DW_CFA_def_cfa_register = 0x0d,
    DW_CFA_def_cfa_offset = 0x0e,
    DW_CFA_def_cfa_expression = 0x0f,
    DW_CFA_expression = 0x10,
    DW_CFA_offset_extended_sf = 0x11,
    DW_CFA_def_cfa_sf = 0x12,
    DW_CFA_def_cfa_offset_sf = 0x13,
    DW_CFA_val_offset = 0x14,
    DW_CFA_val_offset_sf = 0x15,
    DW_CFA_val_expression = 0x16,
    DW_CFA_lo_user = 0x1c,
    DW_CFA_hi_user = 0x3f,
    DW_CFA_advance_loc_00 = 0x40,
    DW_CFA_advance_loc_01 = 0x41,
    DW_CFA_advance_loc_02 = 0x42,
    DW_CFA_advance_loc_03 = 0x43,
    DW_CFA_advance_loc_04 = 0x44,
    DW_CFA_advance_loc_05 = 0x45,
    DW_CFA_advance_loc_06 = 0x46,
    DW_CFA_advance_loc_07 = 0x47,
    DW_CFA_advance_loc_08 = 0x48,
    DW_CFA_advance_loc_09 = 0x49,
    DW_CFA_advance_loc_0a = 0x4a,
    DW_CFA_advance_loc_0b = 0x4b,
    DW_CFA_advance_loc_0c = 0x4c,
    DW_CFA_advance_loc_0d = 0x4d,
    DW_CFA_advance_loc_0e = 0x4e,
    DW_CFA_advance_loc_0f = 0x4f,
    DW_CFA_advance_loc_10 = 0x50,
    DW_CFA_advance_loc_11 = 0x51,
    DW_CFA_advance_loc_12 = 0x52,
    DW_CFA_advance_loc_13 = 0x53,
    DW_CFA_advance_loc_14 = 0x54,
    DW_CFA_advance_loc_15 = 0x55,
    DW_CFA_advance_loc_16 = 0x56,
    DW_CFA_advance_loc_17 = 0x57,
    DW_CFA_advance_loc_18 = 0x58,
    DW_CFA_advance_loc_19 = 0x59,
    DW_CFA_advance_loc_1a = 0x5a,
    DW_CFA_advance_loc_1b = 0x5b,
    DW_CFA_advance_loc_1c = 0x5c,
    DW_CFA_advance_loc_1d = 0x5d,
    DW_CFA_advance_loc_1e = 0x5e,
    DW_CFA_advance_loc_1f = 0x5f,
    DW_CFA_advance_loc_20 = 0x60,
    DW_CFA_advance_loc_21 = 0x61,
    DW_CFA_advance_loc_22 = 0x62,
    DW_CFA_advance_loc_23 = 0x63,
    DW_CFA_advance_loc_24 = 0x64,
    DW_CFA_advance_loc_25 = 0x65,
    DW_CFA_advance_loc_26 = 0x66,
    DW_CFA_advance_loc_27 = 0x67,
    DW_CFA_advance_loc_28 = 0x68,
    DW_CFA_advance_loc_29 = 0x69,
    DW_CFA_advance_loc_2a = 0x6a,
    DW_CFA_advance_loc_2b = 0x6b,
    DW_CFA_advance_loc_2c = 0x6c,
    DW_CFA_advance_loc_2d = 0x6d,
    DW_CFA_advance_loc_2e = 0x6e,
    DW_CFA_advance_loc_2f = 0x6f,
    DW_CFA_advance_loc_30 = 0x70,
    DW_CFA_advance_loc_31 = 0x71,
    DW_CFA_advance_loc_32 = 0x72,
    DW_CFA_advance_loc_33 = 0x73,
    DW_CFA_advance_loc_34 = 0x74,
    DW_CFA_advance_loc_35 = 0x75,
    DW_CFA_advance_loc_36 = 0x76,
    DW_CFA_advance_loc_37 = 0x77,
    DW_CFA_advance_loc_38 = 0x78,
    DW_CFA_advance_loc_39 = 0x79,
    DW_CFA_advance_loc_3a = 0x7a,
    DW_CFA_advance_loc_3b = 0x7b,
    DW_CFA_advance_loc_3c = 0x7c,
    DW_CFA_advance_loc_3d = 0x7d,
    DW_CFA_advance_loc_3e = 0x7e,
    DW_CFA_advance_loc_3f = 0x7f,
    DW_CFA_offset_00 = 0x80,
    DW_CFA_offset_01 = 0x81,
    DW_CFA_offset_02 = 0x82,
    DW_CFA_offset_03 = 0x83,
    DW_CFA_offset_04 = 0x84,
    DW_CFA_offset_05 = 0x85,
    DW_CFA_offset_06 = 0x86,
    DW_CFA_offset_07 = 0x87,
    DW_CFA_offset_08 = 0x88,
    DW_CFA_offset_09 = 0x89,
    DW_CFA_offset_0a = 0x8a,
    DW_CFA_offset_0b = 0x8b,
    DW_CFA_offset_0c = 0x8c,
    DW_CFA_offset_0d = 0x8d,
    DW_CFA_offset_0e = 0x8e,
    DW_CFA_offset_0f = 0x8f,
    DW_CFA_offset_10 = 0x90,
    DW_CFA_offset_11 = 0x91,
    DW_CFA_offset_12 = 0x92,
    DW_CFA_offset_13 = 0x93,
    DW_CFA_offset_14 = 0x94,
    DW_CFA_offset_15 = 0x95,
    DW_CFA_offset_16 = 0x96,
    DW_CFA_offset_17 = 0x97,
    DW_CFA_offset_18 = 0x98,
    DW_CFA_offset_19 = 0x99,
    DW_CFA_offset_1a = 0x9a,
    DW_CFA_offset_1b = 0x9b,
    DW_CFA_offset_1c = 0x9c,
    DW_CFA_offset_1d = 0x9d,
    DW_CFA_offset_1e = 0x9e,
    DW_CFA_offset_1f = 0x9f,
    DW_CFA_offset_20 = 0xa0,
    DW_CFA_offset_21 = 0xa1,
    DW_CFA_offset_22 = 0xa2,
    DW_CFA_offset_23 = 0xa3,
    DW_CFA_offset_24 = 0xa4,
    DW_CFA_offset_25 = 0xa5,
    DW_CFA_offset_26 = 0xa6,
    DW_CFA_offset_27 = 0xa7,
    DW_CFA_offset_28 = 0xa8,
    DW_CFA_offset_29 = 0xa9,
    DW_CFA_offset_2a = 0xaa,
    DW_CFA_offset_2b = 0xab,
    DW_CFA_offset_2c = 0xac,
    DW_CFA_offset_2d = 0xad,
    DW_CFA_offset_2e = 0xae,
    DW_CFA_offset_2f = 0xaf,
    DW_CFA_offset_30 = 0xb0,
    DW_CFA_offset_31 = 0xb1,
    DW_CFA_offset_32 = 0xb2,
    DW_CFA_offset_33 = 0xb3,
    DW_CFA_offset_34 = 0xb4,
    DW_CFA_offset_35 = 0xb5,
    DW_CFA_offset_36 = 0xb6,
    DW_CFA_offset_37 = 0xb7,
    DW_CFA_offset_38 = 0xb8,
    DW_CFA_offset_39 = 0xb9,
    DW_CFA_offset_3a = 0xba,
    DW_CFA_offset_3b = 0xbb,
    DW_CFA_offset_3c = 0xbc,
    DW_CFA_offset_3d = 0xbd,
    DW_CFA_offset_3e = 0xbe,
    DW_CFA_offset_3f = 0xbf,
    DW_CFA_restore_00 = 0xc0,
    DW_CFA_restore_01 = 0xc1,
    DW_CFA_restore_02 = 0xc2,
    DW_CFA_restore_03 = 0xc3,
    DW_CFA_restore_04 = 0xc4,
    DW_CFA_restore_05 = 0xc5,
    DW_CFA_restore_06 = 0xc6,
    DW_CFA_restore_07 = 0xc7,
    DW_CFA_restore_08 = 0xc8,
    DW_CFA_restore_09 = 0xc9,
    DW_CFA_restore_0a = 0xca,
    DW_CFA_restore_0b = 0xcb,
    DW_CFA_restore_0c = 0xcc,
    DW_CFA_restore_0d = 0xcd,
    DW_CFA_restore_0e = 0xce,
    DW_CFA_restore_0f = 0xcf,
    DW_CFA_restore_10 = 0xd0,
    DW_CFA_restore_11 = 0xd1,
    DW_CFA_restore_12 = 0xd2,
    DW_CFA_restore_13 = 0xd3,
    DW_CFA_restore_14 = 0xd4,
    DW_CFA_restore_15 = 0xd5,
    DW_CFA_restore_16 = 0xd6,
    DW_CFA_restore_17 = 0xd7,
    DW_CFA_restore_18 = 0xd8,
    DW_CFA_restore_19 = 0xd9,
    DW_CFA_restore_1a = 0xda,
    DW_CFA_restore_1b = 0xdb,
    DW_CFA_restore_1c = 0xdc,
    DW_CFA_restore_1d = 0xdd,
    DW_CFA_restore_1e = 0xde,
    DW_CFA_restore_1f = 0xdf,
    DW_CFA_restore_20 = 0xe0,
    DW_CFA_restore_21 = 0xe1,
    DW_CFA_restore_22 = 0xe2,
    DW_CFA_restore_23 = 0xe3,
    DW_CFA_restore_24 = 0xe4,
    DW_CFA_restore_25 = 0xe5,
    DW_CFA_restore_26 = 0xe6,
    DW_CFA_restore_27 = 0xe7,
    DW_CFA_restore_28 = 0xe8,
    DW_CFA_restore_29 = 0xe9,
    DW_CFA_restore_2a = 0xea,
    DW_CFA_restore_2b = 0xeb,
    DW_CFA_restore_2c = 0xec,
    DW_CFA_restore_2d = 0xed,
    DW_CFA_restore_2e = 0xee,
    DW_CFA_restore_2f = 0xef,
    DW_CFA_restore_30 = 0xf0,
    DW_CFA_restore_31 = 0xf1,
    DW_CFA_restore_32 = 0xf2,
    DW_CFA_restore_33 = 0xf3,
    DW_CFA_restore_34 = 0xf4,
    DW_CFA_restore_35 = 0xf5,
    DW_CFA_restore_36 = 0xf6,
    DW_CFA_restore_37 = 0xf7,
    DW_CFA_restore_38 = 0xf8,
    DW_CFA_restore_39 = 0xf9,
    DW_CFA_restore_3a = 0xfa,
    DW_CFA_restore_3b = 0xfb,
    DW_CFA_restore_3c = 0xfc,
    DW_CFA_restore_3d = 0xfd,
    DW_CFA_restore_3e = 0xfe,
    DW_CFA_restore_3f = 0xff,
} DW_CFA;

std::string DW_TagToString(DW_TAG dw_tag);
std::string DW_TagToString(DW_CHILDREN dw_child);
std::string DW_TagToString(DW_AT dw_at);
std::string DW_TagToString(DW_FORM dw_form);
std::string DW_TagToString(DW_OP dw_tag);
std::string DW_TagToString(DW_ATE dw_tag);
std::string DW_TagToString(DW_DS dw_tag);
std::string DW_TagToString(DW_END dw_tag);
std::string DW_TagToString(DW_ACCESS dw_tag);
std::string DW_TagToString(DW_VIS dw_tag);
std::string DW_TagToString(DW_VIRTUALITY dw_tag);
std::string DW_TagToString(DW_LANG dw_tag);
std::string DW_TagToString(DW_ID dw_tag);
std::string DW_TagToString(DW_CC dw_tag);
std::string DW_TagToString(DW_INL dw_tag);
std::string DW_TagToString(DW_ORD dw_tag);
std::string DW_TagToString(DW_DSC dw_tag);
std::string DW_TagToString(DW_LNS dw_tag);
std::string DW_TagToString(DW_LNE dw_tag);
std::string DW_TagToString(DW_MACINFO dw_tag);
std::string DW_TagToString(DW_CFA dw_tag);

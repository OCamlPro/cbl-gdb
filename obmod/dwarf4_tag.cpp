#include <string>
#include "dwarf4_tags.h"

using namespace std;

string DW_TagToString(DW_TAG dw_tag)
{
    switch(dw_tag) {
    case DW_TAG_array_type                      :
        return "DW_TAG_array_type";
    case DW_TAG_class_type                      :
        return "DW_TAG_class_type";
    case DW_TAG_entry_point                     :
        return "DW_TAG_entry_point";
    case DW_TAG_enumeration_type                :
        return "DW_TAG_enumeration_type";
    case DW_TAG_formal_parameter                :
        return "DW_TAG_formal_parameter";
    case DW_TAG_imported_declaration            :
        return "DW_TAG_imported_declaration";
    case DW_TAG_label                           :
        return "DW_TAG_label";
    case DW_TAG_lexical_block                   :
        return "DW_TAG_lexical_block";
    case DW_TAG_member                          :
        return "DW_TAG_member";
    case DW_TAG_pointer_type                    :
        return "DW_TAG_pointer_type";
    case DW_TAG_reference_type                  :
        return "DW_TAG_reference_type";
    case DW_TAG_compile_unit                    :
        return "DW_TAG_compile_unit";
    case DW_TAG_string_type                     :
        return "DW_TAG_string_type";
    case DW_TAG_structure_type                  :
        return "DW_TAG_structure_type";
    case DW_TAG_subroutine_type                 :
        return "DW_TAG_subroutine_type";
    case DW_TAG_typedef                         :
        return "DW_TAG_typedef";
    case DW_TAG_union_type                      :
        return "DW_TAG_union_type";
    case DW_TAG_unspecified_parameters          :
        return "DW_TAG_unspecified_parameters";
    case DW_TAG_variant                         :
        return "DW_TAG_variant";
    case DW_TAG_common_block                    :
        return "DW_TAG_common_block";
    case DW_TAG_common_inclusion                :
        return "DW_TAG_common_inclusion";
    case DW_TAG_inheritance                     :
        return "DW_TAG_inheritance";
    case DW_TAG_inlined_subroutine              :
        return "DW_TAG_inlined_subroutine";
    case DW_TAG_module                          :
        return "DW_TAG_module";
    case DW_TAG_ptr_to_member_type              :
        return "DW_TAG_ptr_to_member_type";
    case DW_TAG_set_type                        :
        return "DW_TAG_set_type";
    case DW_TAG_subrange_type                   :
        return "DW_TAG_subrange_type";
    case DW_TAG_with_stmt                       :
        return "DW_TAG_with_stmt";
    case DW_TAG_access_declaration              :
        return "DW_TAG_access_declaration";
    case DW_TAG_base_type                       :
        return "DW_TAG_base_type";
    case DW_TAG_catch_block                     :
        return "DW_TAG_catch_block";
    case DW_TAG_const_type                      :
        return "DW_TAG_const_type";
    case DW_TAG_constant                        :
        return "DW_TAG_constant";
    case DW_TAG_enumerator                      :
        return "DW_TAG_enumerator";
    case DW_TAG_file_type                       :
        return "DW_TAG_file_type";
    case DW_TAG_friend                          :
        return "DW_TAG_friend";
    case DW_TAG_namelist                        :
        return "DW_TAG_namelist";
    case DW_TAG_namelist_item                   :
        return "DW_TAG_namelist_item";
    case DW_TAG_packed_type                     :
        return "DW_TAG_packed_type";
    case DW_TAG_subprogram                      :
        return "DW_TAG_subprogram";
    case DW_TAG_template_type_parameter         :
        return "DW_TAG_template_type_parameter";
    case DW_TAG_template_value_parameter        :
        return "DW_TAG_template_value_parameter";
    case DW_TAG_thrown_type                     :
        return "DW_TAG_thrown_type";
    case DW_TAG_try_block                       :
        return "DW_TAG_try_block";
    case DW_TAG_variant_part                    :
        return "DW_TAG_variant_part";
    case DW_TAG_variable                        :
        return "DW_TAG_variable";
    case DW_TAG_volatile_type                   :
        return "DW_TAG_volatile_type";
    case DW_TAG_dwarf_procedure                 :
        return "DW_TAG_dwarf_procedure";
    case DW_TAG_restrict_type                   :
        return "DW_TAG_restrict_type";
    case DW_TAG_interface_type                  :
        return "DW_TAG_interface_type";
    case DW_TAG_namespace                       :
        return "DW_TAG_namespace";
    case DW_TAG_imported_module                 :
        return "DW_TAG_imported_module";
    case DW_TAG_unspecified_type                :
        return "DW_TAG_unspecified_type";
    case DW_TAG_partial_unit                    :
        return "DW_TAG_partial_unit";
    case DW_TAG_imported_unit                   :
        return "DW_TAG_imported_unit";
    case DW_TAG_condition                       :
        return "DW_TAG_condition";
    case DW_TAG_shared_type                     :
        return "DW_TAG_shared_type";
    case DW_TAG_type_unit                       :
        return "DW_TAG_type_unit";
    case DW_TAG_rvalue_reference_type           :
        return "DW_TAG_rvalue_reference_type";
    case DW_TAG_template_alias                  :
        return "DW_TAG_template_alias";
    case DW_TAG_lo_user                         :
        return "DW_TAG_lo_user";
    case DW_TAG_hi_user                         :
        return "DW_TAG_hi_user";
    default                                     :
        return "<unknown DW_TAG>";
    }
}

string DW_TagToString(DW_CHILDREN dw_child)
{
    switch(dw_child) {
    case DW_CHILDREN_no                         :
        return "DW_CHILDREN_no";
    case DW_CHILDREN_yes                        :
        return "DW_CHILDREN_yes";
    default                                     :
        return "<unknown DW_CHILDREN>";
    }
}

string DW_TagToString(DW_AT dw_at)
{
    switch(dw_at) {
    case DW_AT_sibling                   :
        return "DW_AT_sibling";
    case DW_AT_location                  :
        return "DW_AT_location";
    case DW_AT_name                      :
        return "DW_AT_name";
    case DW_AT_ordering                  :
        return "DW_AT_ordering";
    case DW_AT_byte_size                 :
        return "DW_AT_byte_size";
    case DW_AT_bit_offset                :
        return "DW_AT_bit_offset";
    case DW_AT_bit_size                  :
        return "DW_AT_bit_size";
    case DW_AT_stmt_list                 :
        return "DW_AT_stmt_list";
    case DW_AT_low_pc                    :
        return "DW_AT_low_pc";
    case DW_AT_high_pc                   :
        return "DW_AT_high_pc";
    case DW_AT_language                  :
        return "DW_AT_language";
    case DW_AT_discr                     :
        return "DW_AT_discr";
    case DW_AT_discr_value               :
        return "DW_AT_discr_value";
    case DW_AT_visibility                :
        return "DW_AT_visibility";
    case DW_AT_import                    :
        return "DW_AT_import";
    case DW_AT_string_length             :
        return "DW_AT_string_length";
    case DW_AT_common_reference          :
        return "DW_AT_common_reference";
    case DW_AT_comp_dir                  :
        return "DW_AT_comp_dir";
    case DW_AT_const_value               :
        return "DW_AT_const_value";
    case DW_AT_containing_type           :
        return "DW_AT_containing_type";
    case DW_AT_default_value             :
        return "DW_AT_default_value";
    case DW_AT_inline                    :
        return "DW_AT_inline";
    case DW_AT_is_optional               :
        return "DW_AT_is_optional";
    case DW_AT_lower_bound               :
        return "DW_AT_lower_bound";
    case DW_AT_producer                  :
        return "DW_AT_producer";
    case DW_AT_prototyped                :
        return "DW_AT_prototyped";
    case DW_AT_return_addr               :
        return "DW_AT_return_addr";
    case DW_AT_start_scope               :
        return "DW_AT_start_scope";
    case DW_AT_bit_stride                :
        return "DW_AT_bit_stride";
    case DW_AT_upper_bound               :
        return "DW_AT_upper_bound";
    case DW_AT_abstract_origin           :
        return "DW_AT_abstract_origin";
    case DW_AT_accessibility             :
        return "DW_AT_accessibility";
    case DW_AT_address_class             :
        return "DW_AT_address_class";
    case DW_AT_artificial                :
        return "DW_AT_artificial";
    case DW_AT_base_types                :
        return "DW_AT_base_types";
    case DW_AT_calling_convention        :
        return "DW_AT_calling_convention";
    case DW_AT_count                     :
        return "DW_AT_count";
    case DW_AT_data_member_location      :
        return "DW_AT_data_member_location";
    case DW_AT_decl_column               :
        return "DW_AT_decl_column";
    case DW_AT_decl_file                 :
        return "DW_AT_decl_file";
    case DW_AT_decl_line                 :
        return "DW_AT_decl_line";
    case DW_AT_declaration               :
        return "DW_AT_declaration";
    case DW_AT_discr_list                :
        return "DW_AT_discr_list";
    case DW_AT_encoding                  :
        return "DW_AT_encoding";
    case DW_AT_external                  :
        return "DW_AT_external";
    case DW_AT_frame_base                :
        return "DW_AT_frame_base";
    case DW_AT_friend                    :
        return "DW_AT_friend";
    case DW_AT_identifier_case           :
        return "DW_AT_identifier_case";
    case DW_AT_macro_info                :
        return "DW_AT_macro_info";
    case DW_AT_namelist_item             :
        return "DW_AT_namelist_item";
    case DW_AT_priority                  :
        return "DW_AT_priority";
    case DW_AT_segment                   :
        return "DW_AT_segment";
    case DW_AT_specification             :
        return "DW_AT_specification";
    case DW_AT_static_link               :
        return "DW_AT_static_link";
    case DW_AT_type                      :
        return "DW_AT_type";
    case DW_AT_use_location              :
        return "DW_AT_use_location";
    case DW_AT_variable_parameter        :
        return "DW_AT_variable_parameter";
    case DW_AT_virtuality                :
        return "DW_AT_virtuality";
    case DW_AT_vtable_elem_location      :
        return "DW_AT_vtable_elem_location";
    case DW_AT_allocated                 :
        return "DW_AT_allocated";
    case DW_AT_associated                :
        return "DW_AT_associated";
    case DW_AT_data_location             :
        return "DW_AT_data_location";
    case DW_AT_byte_stride               :
        return "DW_AT_byte_stride";
    case DW_AT_entry_pc                  :
        return "DW_AT_entry_pc";
    case DW_AT_use_UTF8                  :
        return "DW_AT_use_UTF8";
    case DW_AT_extension                 :
        return "DW_AT_extension";
    case DW_AT_ranges                    :
        return "DW_AT_ranges";
    case DW_AT_trampoline                :
        return "DW_AT_trampoline";
    case DW_AT_call_column               :
        return "DW_AT_call_column";
    case DW_AT_call_file                 :
        return "DW_AT_call_file";
    case DW_AT_call_line                 :
        return "DW_AT_call_line";
    case DW_AT_description               :
        return "DW_AT_description";
    case DW_AT_binary_scale              :
        return "DW_AT_binary_scale";
    case DW_AT_decimal_scale             :
        return "DW_AT_decimal_scale";
    case DW_AT_small                     :
        return "DW_AT_small";
    case DW_AT_decimal_sign              :
        return "DW_AT_decimal_sign";
    case DW_AT_digit_count               :
        return "DW_AT_digit_count";
    case DW_AT_picture_string            :
        return "DW_AT_picture_string";
    case DW_AT_mutable                   :
        return "DW_AT_mutable";
    case DW_AT_threads_scaled            :
        return "DW_AT_threads_scaled";
    case DW_AT_explicit                  :
        return "DW_AT_explicit";
    case DW_AT_object_pointer            :
        return "DW_AT_object_pointer";
    case DW_AT_endianity                 :
        return "DW_AT_endianity";
    case DW_AT_elemental                 :
        return "DW_AT_elemental";
    case DW_AT_pure                      :
        return "DW_AT_pure";
    case DW_AT_recursive                 :
        return "DW_AT_recursive";
    case DW_AT_signature                 :
        return "DW_AT_signature";
    case DW_AT_main_subprogram           :
        return "DW_AT_main_subprogram";
    case DW_AT_data_bit_offset           :
        return "DW_AT_data_bit_offset";
    case DW_AT_const_expr                :
        return "DW_AT_const_expr";
    case DW_AT_enum_class                :
        return "DW_AT_enum_class";
    case DW_AT_linkage_name              :
        return "DW_AT_linkage_name";
    case DW_AT_lo_user                   :
        return "DW_AT_lo_user";
    case DW_AT_hi_user                   :
        return "DW_AT_hi_user";
    default                              : {
        char ach[128];
        sprintf(ach,"<unknown DW_AT 0x%4.4X>",dw_at);
        return string(ach);
    }
    };
}

string DW_TagToString(DW_FORM dw_form)
{
    switch(dw_form) {
    case DW_FORM_addr            :
        return "DW_FORM_addr";
    case DW_FORM_block2          :
        return "DW_FORM_block2";
    case DW_FORM_block4          :
        return "DW_FORM_block4";
    case DW_FORM_data2           :
        return "DW_FORM_data2";
    case DW_FORM_data4           :
        return "DW_FORM_data4";
    case DW_FORM_data8           :
        return "DW_FORM_data8";
    case DW_FORM_string          :
        return "DW_FORM_string";
    case DW_FORM_block           :
        return "DW_FORM_block";
    case DW_FORM_block1          :
        return "DW_FORM_block1";
    case DW_FORM_data1           :
        return "DW_FORM_data1";
    case DW_FORM_flag            :
        return "DW_FORM_flag";
    case DW_FORM_sdata           :
        return "DW_FORM_sdata";
    case DW_FORM_strp            :
        return "DW_FORM_strp";
    case DW_FORM_udata           :
        return "DW_FORM_udata";
    case DW_FORM_ref_addr        :
        return "DW_FORM_ref_addr";
    case DW_FORM_ref1            :
        return "DW_FORM_ref1";
    case DW_FORM_ref2            :
        return "DW_FORM_ref2";
    case DW_FORM_ref4            :
        return "DW_FORM_ref4";
    case DW_FORM_ref8            :
        return "DW_FORM_ref8";
    case DW_FORM_ref_udata       :
        return "DW_FORM_ref_udata";
    case DW_FORM_indirect        :
        return "DW_FORM_indirect";
    case DW_FORM_sec_offset      :
        return "DW_FORM_sec_offset";
    case DW_FORM_exprloc         :
        return "DW_FORM_exprloc";
    case DW_FORM_flag_present    :
        return "DW_FORM_flag_present";
    case DW_FORM_ref_sig8        :
        return "DW_FORM_ref_sig8";
    default                      : {
        char ach[128];
        sprintf(ach,"<unknown DW_FORM 0x%4.4X>",dw_form);
        return string(ach);
    }
    };
}

string DW_TagToString(DW_OP dw_tag)
{
    switch(dw_tag) {
    case DW_OP_addr :
        return "DW_OP_addr";
    case DW_OP_deref :
        return "DW_OP_deref";
    case DW_OP_const1u :
        return "DW_OP_const1u";
    case DW_OP_const1s :
        return "DW_OP_const1s";
    case DW_OP_const2u :
        return "DW_OP_const2u";
    case DW_OP_const2s :
        return "DW_OP_const2s";
    case DW_OP_const4u :
        return "DW_OP_const4u";
    case DW_OP_const4s :
        return "DW_OP_const4s";
    case DW_OP_const8u :
        return "DW_OP_const8u";
    case DW_OP_const8s :
        return "DW_OP_const8s";
    case DW_OP_constu :
        return "DW_OP_constu";
    case DW_OP_consts :
        return "DW_OP_consts";
    case DW_OP_dup :
        return "DW_OP_dup";
    case DW_OP_drop :
        return "DW_OP_drop";
    case DW_OP_over :
        return "DW_OP_over";
    case DW_OP_pick :
        return "DW_OP_pick";
    case DW_OP_swap :
        return "DW_OP_swap";
    case DW_OP_rot :
        return "DW_OP_rot";
    case DW_OP_xderef :
        return "DW_OP_xderef";
    case DW_OP_abs :
        return "DW_OP_abs";
    case DW_OP_and :
        return "DW_OP_and";
    case DW_OP_div :
        return "DW_OP_div";
    case DW_OP_minus :
        return "DW_OP_minus";
    case DW_OP_mod :
        return "DW_OP_mod";
    case DW_OP_mul :
        return "DW_OP_mul";
    case DW_OP_neg :
        return "DW_OP_neg";
    case DW_OP_not :
        return "DW_OP_not";
    case DW_OP_or :
        return "DW_OP_or";
    case DW_OP_plus :
        return "DW_OP_plus";
    case DW_OP_plus_uconst :
        return "DW_OP_plus_uconst";
    case DW_OP_shl :
        return "DW_OP_shl";
    case DW_OP_shr :
        return "DW_OP_shr";
    case DW_OP_shra :
        return "DW_OP_shra";
    case DW_OP_xor :
        return "DW_OP_xor";
    case DW_OP_skip :
        return "DW_OP_skip";
    case DW_OP_bra :
        return "DW_OP_bra";
    case DW_OP_eq :
        return "DW_OP_eq";
    case DW_OP_ge :
        return "DW_OP_ge";
    case DW_OP_gt :
        return "DW_OP_gt";
    case DW_OP_le :
        return "DW_OP_le";
    case DW_OP_lt :
        return "DW_OP_lt";
    case DW_OP_ne :
        return "DW_OP_ne";
    case DW_OP_lit0 :
        return "DW_OP_lit0";
    case DW_OP_lit1 :
        return "DW_OP_lit1";
    case DW_OP_lit31 :
        return "DW_OP_lit31";
    case DW_OP_reg0 :
        return "DW_OP_reg0";
    case DW_OP_reg1 :
        return "DW_OP_reg1";
    case DW_OP_reg31 :
        return "DW_OP_reg31";
    case DW_OP_breg0 :
        return "DW_OP_breg0";
    case DW_OP_breg1 :
        return "DW_OP_breg1";
    case DW_OP_breg31 :
        return "DW_OP_breg31";
    case DW_OP_regx :
        return "DW_OP_regx";
    case DW_OP_fbreg :
        return "DW_OP_fbreg";
    case DW_OP_bregx :
        return "DW_OP_bregx";
    case DW_OP_piece :
        return "DW_OP_piece";
    case DW_OP_deref_size :
        return "DW_OP_deref_size";
    case DW_OP_xderef_size :
        return "DW_OP_xderef_size";
    case DW_OP_nop :
        return "DW_OP_nop";
    case DW_OP_push_object_address :
        return "DW_OP_push_object_address";
    case DW_OP_call2 :
        return "DW_OP_call2";
    case DW_OP_call4 :
        return "DW_OP_call4";
    case DW_OP_call_ref :
        return "DW_OP_call_ref";
    case DW_OP_form_tls_address :
        return "DW_OP_form_tls_address";
    case DW_OP_call_frame_cfa :
        return "DW_OP_call_frame_cfa";
    case DW_OP_bit_piece :
        return "DW_OP_bit_piece";
    case DW_OP_implicit_value :
        return "DW_OP_implicit_value";
    case DW_OP_stack_value :
        return "DW_OP_stack_value";
    case DW_OP_lo_user :
        return "DW_OP_lo_user";
    case DW_OP_hi_user :
        return "DW_OP_hi_user";
    default                      : {
        char ach[128];
        sprintf(ach,"<unknown DW_OP 0x%4.4X>",dw_tag);
        return string(ach);
    }
    };
}

string DW_TagToString(DW_ATE dw_tag)
{
    switch(dw_tag) {
    case DW_ATE_address :
        return "DW_ATE_address";
    case DW_ATE_boolean :
        return "DW_ATE_boolean";
    case DW_ATE_complex_float :
        return "DW_ATE_complex_float";
    case DW_ATE_float :
        return "DW_ATE_float";
    case DW_ATE_signed :
        return "DW_ATE_signed";
    case DW_ATE_signed_char :
        return "DW_ATE_signed_char";
    case DW_ATE_unsigned :
        return "DW_ATE_unsigned";
    case DW_ATE_unsigned_char :
        return "DW_ATE_unsigned_char";
    case DW_ATE_imaginary_float :
        return "DW_ATE_imaginary_float";
    case DW_ATE_packed_decimal :
        return "DW_ATE_packed_decimal";
    case DW_ATE_numeric_string :
        return "DW_ATE_numeric_string";
    case DW_ATE_edited :
        return "DW_ATE_edited";
    case DW_ATE_signed_fixed :
        return "DW_ATE_signed_fixed";
    case DW_ATE_unsigned_fixed :
        return "DW_ATE_unsigned_fixed";
    case DW_ATE_decimal_float :
        return "DW_ATE_decimal_float";
    case DW_ATE_UTF :
        return "DW_ATE_UTF";
    case DW_ATE_lo_user :
        return "DW_ATE_lo_user";
    case DW_ATE_hi_user :
        return "DW_ATE_hi_user";
    default                      : {
        char ach[128];
        sprintf(ach,"<unknown DW_ATE 0x%4.4X>",dw_tag);
        return string(ach);
    }
    };
}

string DW_TagToString(DW_DS dw_tag)
{
    switch(dw_tag) {
    case DW_DS_unsigned :
        return "DW_DS_unsigned";
    case DW_DS_leading_overpunch :
        return "DW_DS_leading_overpunch";
    case DW_DS_trailing_overpunch :
        return "DW_DS_trailing_overpunch";
    case DW_DS_leading_separate :
        return "DW_DS_leading_separate";
    case DW_DS_trailing_separate :
        return "DW_DS_trailing_separate";
    default                      :
        return "<unknown DW_DS>";
    };
}

string DW_TagToString(DW_END dw_tag)
{
    switch(dw_tag) {
    case DW_END_default :
        return "DW_END_default";
    case DW_END_big :
        return "DW_END_big";
    case DW_END_little :
        return "DW_END_little";
    case DW_END_lo_user :
        return "DW_END_lo_user";
    case DW_END_hi_user :
        return "DW_END_hi_user";
    default                      :
        return "<unknown DW_END>";
    };
}

string DW_TagToString(DW_ACCESS dw_tag)
{
    switch(dw_tag) {
    case DW_ACCESS_public :
        return "DW_ACCESS_public";
    case DW_ACCESS_protected :
        return "DW_ACCESS_protected";
    case DW_ACCESS_private :
        return "DW_ACCESS_private";
    default                      :
        return "<unknown DW_ACCESS>";
    };
}

string DW_TagToString(DW_VIS dw_tag)
{
    switch(dw_tag) {
    case DW_VIS_local :
        return "DW_VIS_local";
    case DW_VIS_exported :
        return "DW_VIS_exported";
    case DW_VIS_qualified :
        return "DW_VIS_qualified";
    default                      :
        return "<unknown DW_VIS>";
    };
}

string DW_TagToString(DW_VIRTUALITY dw_tag)
{
    switch(dw_tag) {
    case DW_VIRTUALITY_none :
        return "DW_VIRTUALITY_none";
    case DW_VIRTUALITY_virtual :
        return "DW_VIRTUALITY_virtual";
    case DW_VIRTUALITY_pure_virtual :
        return "DW_VIRTUALITY_pure_virtual";
    default                      :
        return "<unknown DW_VIRTUALITY>";
    };
}

string DW_TagToString(DW_LANG dw_tag)
{
    switch(dw_tag) {
    case DW_LANG_C89 :
        return "DW_LANG_C89";
    case DW_LANG_C :
        return "DW_LANG_C";
    case DW_LANG_Ada83 :
        return "DW_LANG_Ada83";
    case DW_LANG_C_plus_plus :
        return "DW_LANG_C_plus_plus";
    case DW_LANG_Cobol74 :
        return "DW_LANG_Cobol74";
    case DW_LANG_Cobol85 :
        return "DW_LANG_Cobol85";
    case DW_LANG_Fortran77 :
        return "DW_LANG_Fortran77";
    case DW_LANG_Fortran90 :
        return "DW_LANG_Fortran90";
    case DW_LANG_Pascal83 :
        return "DW_LANG_Pascal83";
    case DW_LANG_Modula2 :
        return "DW_LANG_Modula2";
    case DW_LANG_Java :
        return "DW_LANG_Java";
    case DW_LANG_C99 :
        return "DW_LANG_C99";
    case DW_LANG_Ada95 :
        return "DW_LANG_Ada95";
    case DW_LANG_Fortran95 :
        return "DW_LANG_Fortran95";
    case DW_LANG_PLI :
        return "DW_LANG_PLI";
    case DW_LANG_ObjC :
        return "DW_LANG_ObjC";
    case DW_LANG_ObjC_plus_plus :
        return "DW_LANG_ObjC_plus_plus";
    case DW_LANG_UPC :
        return "DW_LANG_UPC";
    case DW_LANG_D :
        return "DW_LANG_D";
    case DW_LANG_Python :
        return "DW_LANG_Python";
    case DW_LANG_lo_user :
        return "DW_LANG_lo_user";
    case DW_LANG_hi_user :
        return "DW_LANG_hi_user";
    default                      :
        return "<unknown DW_LANG>";
    };
}

string DW_TagToString(DW_ID dw_tag)
{
    switch(dw_tag) {
    case DW_ID_case_sensitive :
        return "DW_ID_case_sensitive";
    case DW_ID_up_case :
        return "DW_ID_up_case";
    case DW_ID_down_case :
        return "DW_ID_down_case";
    case DW_ID_case_insensitive :
        return "DW_ID_case_insensitive";
    default                      :
        return "<unknown DW_ID>";
    };
}

string DW_TagToString(DW_CC dw_tag)
{
    switch(dw_tag) {
    case DW_CC_normal :
        return "DW_CC_normal";
    case DW_CC_program :
        return "DW_CC_program";
    case DW_CC_nocall :
        return "DW_CC_nocall";
    case DW_CC_lo_user :
        return "DW_CC_lo_user";
    case DW_CC_hi_user :
        return "DW_CC_hi_user";
    default                      :
        return "<unknown DW_CC>";
    };
}

string DW_TagToString(DW_INL dw_tag)
{
    switch(dw_tag) {
    case DW_INL_not_inlined :
        return "DW_INL_not_inlined";
    case DW_INL_inlined :
        return "DW_INL_inlined";
    case DW_INL_declared_not_inlined :
        return "DW_INL_declared_not_inlined";
    case DW_INL_declared_inlined :
        return "DW_INL_declared_inlined";
    default                      :
        return "<unknown DW_INL>";
    };
}

string DW_TagToString(DW_ORD dw_tag)
{
    switch(dw_tag) {
    case DW_ORD_row_major :
        return "DW_ORD_row_major";
    case DW_ORD_col_major :
        return "DW_ORD_col_major";
    default                      :
        return "<unknown DW_ORD>";
    };
}

string DW_TagToString(DW_DSC dw_tag)
{
    switch(dw_tag) {
    case DW_DSC_label :
        return "DW_DSC_label";
    case DW_DSC_range :
        return "DW_DSC_range";
    default                      :
        return "<unknown DW_OP>";
    };
}

string DW_TagToString(DW_LNS dw_tag)
{
    switch(dw_tag) {
    case DW_LNS_copy :
        return "DW_LNS_copy";
    case DW_LNS_advance_pc :
        return "DW_LNS_advance_pc";
    case DW_LNS_advance_line :
        return "DW_LNS_advance_line";
    case DW_LNS_set_file :
        return "DW_LNS_set_file";
    case DW_LNS_set_column :
        return "DW_LNS_set_column";
    case DW_LNS_negate_stmt :
        return "DW_LNS_negate_stmt";
    case DW_LNS_set_basic_block :
        return "DW_LNS_set_basic_block";
    case DW_LNS_const_add_pc :
        return "DW_LNS_const_add_pc";
    case DW_LNS_fixed_advance_pc :
        return "DW_LNS_fixed_advance_pc";
    case DW_LNS_set_prologue_end :
        return "DW_LNS_set_prologue_end";
    case DW_LNS_set_epilogue_begin :
        return "DW_LNS_set_epilogue_begin";
    case DW_LNS_set_isa :
        return "DW_LNS_set_isa";
    default                      :
        return "<unknown DW_LNS>";
    };
}

string DW_TagToString(DW_LNE dw_tag)
{
    switch(dw_tag) {
    case DW_LNE_end_sequence :
        return "DW_LNE_end_sequence";
    case DW_LNE_set_address :
        return "DW_LNE_set_address";
    case DW_LNE_define_file :
        return "DW_LNE_define_file";
    case DW_LNE_set_discriminator :
        return "DW_LNE_set_discriminator";
    case DW_LNE_lo_user :
        return "DW_LNE_lo_user";
    case DW_LNE_hi_user :
        return "DW_LNE_hi_user";
    default                      :
        return "<unknown DW_LNEP>";
    };
}

string DW_TagToString(DW_MACINFO dw_tag)
{
    switch(dw_tag) {
    case DW_MACINFO_define :
        return "DW_MACINFO_define";
    case DW_MACINFO_undef :
        return "DW_MACINFO_undef";
    case DW_MACINFO_start_file :
        return "DW_MACINFO_start_file";
    case DW_MACINFO_end_file :
        return "DW_MACINFO_end_file";
    case DW_MACINFO_vendor_ext :
        return "DW_MACINFO_vendor_ext";
    default                      :
        return "<unknown DW_MACINFO>";
    };
}

string DW_TagToString(DW_CFA dw_tag)
{
    switch(dw_tag) {
    case DW_CFA_nop :
        return "DW_CFA_nop";
    case DW_CFA_set_loc :
        return "DW_CFA_set_loc";
    case DW_CFA_advance_loc1 :
        return "DW_CFA_advance_loc1";
    case DW_CFA_advance_loc2 :
        return "DW_CFA_advance_loc2";
    case DW_CFA_advance_loc4 :
        return "DW_CFA_advance_loc4";
    case DW_CFA_offset_extended :
        return "DW_CFA_offset_extended";
    case DW_CFA_restore_extended :
        return "DW_CFA_restore_extended";
    case DW_CFA_undefined :
        return "DW_CFA_undefined";
    case DW_CFA_same_value :
        return "DW_CFA_same_value";
    case DW_CFA_register :
        return "DW_CFA_register";
    case DW_CFA_remember_state :
        return "DW_CFA_remember_state";
    case DW_CFA_restore_state :
        return "DW_CFA_restore_state";
    case DW_CFA_def_cfa :
        return "DW_CFA_def_cfa";
    case DW_CFA_def_cfa_register :
        return "DW_CFA_def_cfa_register";
    case DW_CFA_def_cfa_offset :
        return "DW_CFA_def_cfa_offset";
    case DW_CFA_def_cfa_expression :
        return "DW_CFA_def_cfa_expression";
    case DW_CFA_expression :
        return "DW_CFA_expression";
    case DW_CFA_offset_extended_sf :
        return "DW_CFA_offset_extended_sf";
    case DW_CFA_def_cfa_sf :
        return "DW_CFA_def_cfa_sf";
    case DW_CFA_def_cfa_offset_sf :
        return "DW_CFA_def_cfa_offset_sf";
    case DW_CFA_val_offset :
        return "DW_CFA_val_offset";
    case DW_CFA_val_offset_sf :
        return "DW_CFA_val_offset_sf";
    case DW_CFA_val_expression :
        return "DW_CFA_val_expression";
    case DW_CFA_lo_user :
        return "DW_CFA_lo_user";
    case DW_CFA_hi_user :
        return "DW_CFA_hi_user";
    case DW_CFA_advance_loc_00 :
        return "DW_CFA_advance_loc_00";
    case DW_CFA_advance_loc_01 :
        return "DW_CFA_advance_loc_01";
    case DW_CFA_advance_loc_02 :
        return "DW_CFA_advance_loc_02";
    case DW_CFA_advance_loc_03 :
        return "DW_CFA_advance_loc_03";
    case DW_CFA_advance_loc_04 :
        return "DW_CFA_advance_loc_04";
    case DW_CFA_advance_loc_05 :
        return "DW_CFA_advance_loc_05";
    case DW_CFA_advance_loc_06 :
        return "DW_CFA_advance_loc_06";
    case DW_CFA_advance_loc_07 :
        return "DW_CFA_advance_loc_07";
    case DW_CFA_advance_loc_08 :
        return "DW_CFA_advance_loc_08";
    case DW_CFA_advance_loc_09 :
        return "DW_CFA_advance_loc_09";
    case DW_CFA_advance_loc_0a :
        return "DW_CFA_advance_loc_0a";
    case DW_CFA_advance_loc_0b :
        return "DW_CFA_advance_loc_0b";
    case DW_CFA_advance_loc_0c :
        return "DW_CFA_advance_loc_0c";
    case DW_CFA_advance_loc_0d :
        return "DW_CFA_advance_loc_0d";
    case DW_CFA_advance_loc_0e :
        return "DW_CFA_advance_loc_0e";
    case DW_CFA_advance_loc_0f :
        return "DW_CFA_advance_loc_0f";
    case DW_CFA_advance_loc_10 :
        return "DW_CFA_advance_loc_10";
    case DW_CFA_advance_loc_11 :
        return "DW_CFA_advance_loc_11";
    case DW_CFA_advance_loc_12 :
        return "DW_CFA_advance_loc_12";
    case DW_CFA_advance_loc_13 :
        return "DW_CFA_advance_loc_13";
    case DW_CFA_advance_loc_14 :
        return "DW_CFA_advance_loc_14";
    case DW_CFA_advance_loc_15 :
        return "DW_CFA_advance_loc_15";
    case DW_CFA_advance_loc_16 :
        return "DW_CFA_advance_loc_16";
    case DW_CFA_advance_loc_17 :
        return "DW_CFA_advance_loc_17";
    case DW_CFA_advance_loc_18 :
        return "DW_CFA_advance_loc_18";
    case DW_CFA_advance_loc_19 :
        return "DW_CFA_advance_loc_19";
    case DW_CFA_advance_loc_1a :
        return "DW_CFA_advance_loc_1a";
    case DW_CFA_advance_loc_1b :
        return "DW_CFA_advance_loc_1b";
    case DW_CFA_advance_loc_1c :
        return "DW_CFA_advance_loc_1c";
    case DW_CFA_advance_loc_1d :
        return "DW_CFA_advance_loc_1d";
    case DW_CFA_advance_loc_1e :
        return "DW_CFA_advance_loc_1e";
    case DW_CFA_advance_loc_1f :
        return "DW_CFA_advance_loc_1f";
    case DW_CFA_advance_loc_20 :
        return "DW_CFA_advance_loc_20";
    case DW_CFA_advance_loc_21 :
        return "DW_CFA_advance_loc_21";
    case DW_CFA_advance_loc_22 :
        return "DW_CFA_advance_loc_22";
    case DW_CFA_advance_loc_23 :
        return "DW_CFA_advance_loc_23";
    case DW_CFA_advance_loc_24 :
        return "DW_CFA_advance_loc_24";
    case DW_CFA_advance_loc_25 :
        return "DW_CFA_advance_loc_25";
    case DW_CFA_advance_loc_26 :
        return "DW_CFA_advance_loc_26";
    case DW_CFA_advance_loc_27 :
        return "DW_CFA_advance_loc_27";
    case DW_CFA_advance_loc_28 :
        return "DW_CFA_advance_loc_28";
    case DW_CFA_advance_loc_29 :
        return "DW_CFA_advance_loc_29";
    case DW_CFA_advance_loc_2a :
        return "DW_CFA_advance_loc_2a";
    case DW_CFA_advance_loc_2b :
        return "DW_CFA_advance_loc_2b";
    case DW_CFA_advance_loc_2c :
        return "DW_CFA_advance_loc_2c";
    case DW_CFA_advance_loc_2d :
        return "DW_CFA_advance_loc_2d";
    case DW_CFA_advance_loc_2e :
        return "DW_CFA_advance_loc_2e";
    case DW_CFA_advance_loc_2f :
        return "DW_CFA_advance_loc_2f";
    case DW_CFA_advance_loc_30 :
        return "DW_CFA_advance_loc_30";
    case DW_CFA_advance_loc_31 :
        return "DW_CFA_advance_loc_31";
    case DW_CFA_advance_loc_32 :
        return "DW_CFA_advance_loc_32";
    case DW_CFA_advance_loc_33 :
        return "DW_CFA_advance_loc_33";
    case DW_CFA_advance_loc_34 :
        return "DW_CFA_advance_loc_34";
    case DW_CFA_advance_loc_35 :
        return "DW_CFA_advance_loc_35";
    case DW_CFA_advance_loc_36 :
        return "DW_CFA_advance_loc_36";
    case DW_CFA_advance_loc_37 :
        return "DW_CFA_advance_loc_37";
    case DW_CFA_advance_loc_38 :
        return "DW_CFA_advance_loc_38";
    case DW_CFA_advance_loc_39 :
        return "DW_CFA_advance_loc_39";
    case DW_CFA_advance_loc_3a :
        return "DW_CFA_advance_loc_3a";
    case DW_CFA_advance_loc_3b :
        return "DW_CFA_advance_loc_3b";
    case DW_CFA_advance_loc_3c :
        return "DW_CFA_advance_loc_3c";
    case DW_CFA_advance_loc_3d :
        return "DW_CFA_advance_loc_3d";
    case DW_CFA_advance_loc_3e :
        return "DW_CFA_advance_loc_3e";
    case DW_CFA_advance_loc_3f :
        return "DW_CFA_advance_loc_3f";
    case DW_CFA_offset_00 :
        return "DW_CFA_offset_00";
    case DW_CFA_offset_01 :
        return "DW_CFA_offset_01";
    case DW_CFA_offset_02 :
        return "DW_CFA_offset_02";
    case DW_CFA_offset_03 :
        return "DW_CFA_offset_03";
    case DW_CFA_offset_04 :
        return "DW_CFA_offset_04";
    case DW_CFA_offset_05 :
        return "DW_CFA_offset_05";
    case DW_CFA_offset_06 :
        return "DW_CFA_offset_06";
    case DW_CFA_offset_07 :
        return "DW_CFA_offset_07";
    case DW_CFA_offset_08 :
        return "DW_CFA_offset_08";
    case DW_CFA_offset_09 :
        return "DW_CFA_offset_09";
    case DW_CFA_offset_0a :
        return "DW_CFA_offset_0a";
    case DW_CFA_offset_0b :
        return "DW_CFA_offset_0b";
    case DW_CFA_offset_0c :
        return "DW_CFA_offset_0c";
    case DW_CFA_offset_0d :
        return "DW_CFA_offset_0d";
    case DW_CFA_offset_0e :
        return "DW_CFA_offset_0e";
    case DW_CFA_offset_0f :
        return "DW_CFA_offset_0f";
    case DW_CFA_offset_10 :
        return "DW_CFA_offset_10";
    case DW_CFA_offset_11 :
        return "DW_CFA_offset_11";
    case DW_CFA_offset_12 :
        return "DW_CFA_offset_12";
    case DW_CFA_offset_13 :
        return "DW_CFA_offset_13";
    case DW_CFA_offset_14 :
        return "DW_CFA_offset_14";
    case DW_CFA_offset_15 :
        return "DW_CFA_offset_15";
    case DW_CFA_offset_16 :
        return "DW_CFA_offset_16";
    case DW_CFA_offset_17 :
        return "DW_CFA_offset_17";
    case DW_CFA_offset_18 :
        return "DW_CFA_offset_18";
    case DW_CFA_offset_19 :
        return "DW_CFA_offset_19";
    case DW_CFA_offset_1a :
        return "DW_CFA_offset_1a";
    case DW_CFA_offset_1b :
        return "DW_CFA_offset_1b";
    case DW_CFA_offset_1c :
        return "DW_CFA_offset_1c";
    case DW_CFA_offset_1d :
        return "DW_CFA_offset_1d";
    case DW_CFA_offset_1e :
        return "DW_CFA_offset_1e";
    case DW_CFA_offset_1f :
        return "DW_CFA_offset_1f";
    case DW_CFA_offset_20 :
        return "DW_CFA_offset_20";
    case DW_CFA_offset_21 :
        return "DW_CFA_offset_21";
    case DW_CFA_offset_22 :
        return "DW_CFA_offset_22";
    case DW_CFA_offset_23 :
        return "DW_CFA_offset_23";
    case DW_CFA_offset_24 :
        return "DW_CFA_offset_24";
    case DW_CFA_offset_25 :
        return "DW_CFA_offset_25";
    case DW_CFA_offset_26 :
        return "DW_CFA_offset_26";
    case DW_CFA_offset_27 :
        return "DW_CFA_offset_27";
    case DW_CFA_offset_28 :
        return "DW_CFA_offset_28";
    case DW_CFA_offset_29 :
        return "DW_CFA_offset_29";
    case DW_CFA_offset_2a :
        return "DW_CFA_offset_2a";
    case DW_CFA_offset_2b :
        return "DW_CFA_offset_2b";
    case DW_CFA_offset_2c :
        return "DW_CFA_offset_2c";
    case DW_CFA_offset_2d :
        return "DW_CFA_offset_2d";
    case DW_CFA_offset_2e :
        return "DW_CFA_offset_2e";
    case DW_CFA_offset_2f :
        return "DW_CFA_offset_2f";
    case DW_CFA_offset_30 :
        return "DW_CFA_offset_30";
    case DW_CFA_offset_31 :
        return "DW_CFA_offset_31";
    case DW_CFA_offset_32 :
        return "DW_CFA_offset_32";
    case DW_CFA_offset_33 :
        return "DW_CFA_offset_33";
    case DW_CFA_offset_34 :
        return "DW_CFA_offset_34";
    case DW_CFA_offset_35 :
        return "DW_CFA_offset_35";
    case DW_CFA_offset_36 :
        return "DW_CFA_offset_36";
    case DW_CFA_offset_37 :
        return "DW_CFA_offset_37";
    case DW_CFA_offset_38 :
        return "DW_CFA_offset_38";
    case DW_CFA_offset_39 :
        return "DW_CFA_offset_39";
    case DW_CFA_offset_3a :
        return "DW_CFA_offset_3a";
    case DW_CFA_offset_3b :
        return "DW_CFA_offset_3b";
    case DW_CFA_offset_3c :
        return "DW_CFA_offset_3c";
    case DW_CFA_offset_3d :
        return "DW_CFA_offset_3d";
    case DW_CFA_offset_3e :
        return "DW_CFA_offset_3e";
    case DW_CFA_offset_3f :
        return "DW_CFA_offset_3f";
    case DW_CFA_restore_00 :
        return "DW_CFA_restore_00";
    case DW_CFA_restore_01 :
        return "DW_CFA_restore_01";
    case DW_CFA_restore_02 :
        return "DW_CFA_restore_02";
    case DW_CFA_restore_03 :
        return "DW_CFA_restore_03";
    case DW_CFA_restore_04 :
        return "DW_CFA_restore_04";
    case DW_CFA_restore_05 :
        return "DW_CFA_restore_05";
    case DW_CFA_restore_06 :
        return "DW_CFA_restore_06";
    case DW_CFA_restore_07 :
        return "DW_CFA_restore_07";
    case DW_CFA_restore_08 :
        return "DW_CFA_restore_08";
    case DW_CFA_restore_09 :
        return "DW_CFA_restore_09";
    case DW_CFA_restore_0a :
        return "DW_CFA_restore_0a";
    case DW_CFA_restore_0b :
        return "DW_CFA_restore_0b";
    case DW_CFA_restore_0c :
        return "DW_CFA_restore_0c";
    case DW_CFA_restore_0d :
        return "DW_CFA_restore_0d";
    case DW_CFA_restore_0e :
        return "DW_CFA_restore_0e";
    case DW_CFA_restore_0f :
        return "DW_CFA_restore_0f";
    case DW_CFA_restore_10 :
        return "DW_CFA_restore_10";
    case DW_CFA_restore_11 :
        return "DW_CFA_restore_11";
    case DW_CFA_restore_12 :
        return "DW_CFA_restore_12";
    case DW_CFA_restore_13 :
        return "DW_CFA_restore_13";
    case DW_CFA_restore_14 :
        return "DW_CFA_restore_14";
    case DW_CFA_restore_15 :
        return "DW_CFA_restore_15";
    case DW_CFA_restore_16 :
        return "DW_CFA_restore_16";
    case DW_CFA_restore_17 :
        return "DW_CFA_restore_17";
    case DW_CFA_restore_18 :
        return "DW_CFA_restore_18";
    case DW_CFA_restore_19 :
        return "DW_CFA_restore_19";
    case DW_CFA_restore_1a :
        return "DW_CFA_restore_1a";
    case DW_CFA_restore_1b :
        return "DW_CFA_restore_1b";
    case DW_CFA_restore_1c :
        return "DW_CFA_restore_1c";
    case DW_CFA_restore_1d :
        return "DW_CFA_restore_1d";
    case DW_CFA_restore_1e :
        return "DW_CFA_restore_1e";
    case DW_CFA_restore_1f :
        return "DW_CFA_restore_1f";
    case DW_CFA_restore_20 :
        return "DW_CFA_restore_20";
    case DW_CFA_restore_21 :
        return "DW_CFA_restore_21";
    case DW_CFA_restore_22 :
        return "DW_CFA_restore_22";
    case DW_CFA_restore_23 :
        return "DW_CFA_restore_23";
    case DW_CFA_restore_24 :
        return "DW_CFA_restore_24";
    case DW_CFA_restore_25 :
        return "DW_CFA_restore_25";
    case DW_CFA_restore_26 :
        return "DW_CFA_restore_26";
    case DW_CFA_restore_27 :
        return "DW_CFA_restore_27";
    case DW_CFA_restore_28 :
        return "DW_CFA_restore_28";
    case DW_CFA_restore_29 :
        return "DW_CFA_restore_29";
    case DW_CFA_restore_2a :
        return "DW_CFA_restore_2a";
    case DW_CFA_restore_2b :
        return "DW_CFA_restore_2b";
    case DW_CFA_restore_2c :
        return "DW_CFA_restore_2c";
    case DW_CFA_restore_2d :
        return "DW_CFA_restore_2d";
    case DW_CFA_restore_2e :
        return "DW_CFA_restore_2e";
    case DW_CFA_restore_2f :
        return "DW_CFA_restore_2f";
    case DW_CFA_restore_30 :
        return "DW_CFA_restore_30";
    case DW_CFA_restore_31 :
        return "DW_CFA_restore_31";
    case DW_CFA_restore_32 :
        return "DW_CFA_restore_32";
    case DW_CFA_restore_33 :
        return "DW_CFA_restore_33";
    case DW_CFA_restore_34 :
        return "DW_CFA_restore_34";
    case DW_CFA_restore_35 :
        return "DW_CFA_restore_35";
    case DW_CFA_restore_36 :
        return "DW_CFA_restore_36";
    case DW_CFA_restore_37 :
        return "DW_CFA_restore_37";
    case DW_CFA_restore_38 :
        return "DW_CFA_restore_38";
    case DW_CFA_restore_39 :
        return "DW_CFA_restore_39";
    case DW_CFA_restore_3a :
        return "DW_CFA_restore_3a";
    case DW_CFA_restore_3b :
        return "DW_CFA_restore_3b";
    case DW_CFA_restore_3c :
        return "DW_CFA_restore_3c";
    case DW_CFA_restore_3d :
        return "DW_CFA_restore_3d";
    case DW_CFA_restore_3e :
        return "DW_CFA_restore_3e";
    case DW_CFA_restore_3f :
        return "DW_CFA_restore_3f";
    default                      :
        return "<unknown DW_CFA>";
    };
}






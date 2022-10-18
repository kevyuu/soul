/***************************************************************************************************
 * Copyright 2022 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief      MDL module builder

#ifndef MI_NEURAYLIB_IMDL_MODULE_BUILDER_H
#define MI_NEURAYLIB_IMDL_MODULE_BUILDER_H

#include <mi/base/interface_declare.h>
#include <mi/neuraylib/itype.h>

namespace mi {

class IArray;

namespace neuraylib {

class IAnnotation_block;
class IAnnotation_list;
class IExpression;
class IExpression_list;
class IMdl_execution_context;

/** \addtogroup mi_neuray_mdl_misc
@{
*/

/// The module builder allows to create new MDL modules.
///
/// \see #mi::neuraylib::IMdl_factory::create_module_builder()
class IMdl_module_builder: public
    base::Interface_declare<0x2357f2f8,0x4428,0x47e5,0xaa,0x92,0x97,0x98,0x25,0x5d,0x26,0x57>
{
public:
    /// Adds a variant to the module.
    ///
    /// \param name                    The simple name of the material or function variant.
    /// \param prototype_name          The DB name of the prototype of the new variant.
    /// \param defaults                Default values to set. If \c NULL, the defaults of the
    ///                                original material or function are used. Feasible
    ///                                sub-expression kinds: constants and calls.
    /// \param annotations             Annotations to set. If \c NULL, the annotations of the
    ///                                original material or function are used. Pass an empty block
    ///                                to remove all annotations.
    /// \param return_annotations      Return annotations to set. If \c NULL, the annotations of
    ///                                the original material or function are used. Pass an empty
    ///                                block to remove all annotations. Materials require \c NULL or
    ///                                an empty annotation block here.
    /// \param is_exported             Indicates whether the variant will have the 'export' keyword.
    /// \param context                 The execution context can be used to pass options and to
    ///                                retrieve error and/or warning messages. Can be \c NULL.
    /// \return                        0 in case of success, or -1 in case of failure.
    virtual Sint32 add_variant(
        const char* name,
        const char* prototype_name,
        const IExpression_list* defaults,
        const IAnnotation_block* annotations,
        const IAnnotation_block* return_annotations,
        bool is_exported,
        IMdl_execution_context* context) = 0;

    /// Adds a material or function to the module.
    ///
    /// \param name                    The simple name of the material or function.
    /// \param body                    The body of the new material or function (constants, direct
    ///                                calls, and parameter references). Feasible sub-expression
    ///                                kinds: constants, direct calls, and parameter references.
    /// \param parameters              Types and names of the parameters. Can be \c NULL (treated
    ///                                like an empty parameter list).
    /// \param defaults                Default values. Can be \c NULL or incomplete. Feasible
    ///                                sub-expression kinds: constants, calls, and direct calls.
    /// \param parameter_annotations   Parameter annotations. Can be \c NULL or incomplete.
    /// \param annotations             Annotations of the material or function. Can be \c NULL.
    /// \param return_annotations      Return annotations of the function. Can be \c NULL for
    ///                                functions, must be \c NULL for materials.
    /// \param is_exported             Indicates whether the material or function will have the
    ///                                'export' keyword.
    /// \param frequency_qualifier     The frequency qualifier for functions, or
    ///                                #mi::neuraylib::IType::MK_NONE for materials.
    /// \param context                 The execution context can be used to pass options and to
    ///                                retrieve error and/or warning messages. Can be \c NULL.
    virtual Sint32 add_function(
        const char* name,
        const IExpression* body,
        const IType_list* parameters,
        const IExpression_list* defaults,
        const IAnnotation_list* parameter_annotations,
        const IAnnotation_block* annotations,
        const IAnnotation_block* return_annotations,
        bool is_exported,
        IType::Modifier frequency_qualifier,
        IMdl_execution_context* context) = 0;

    /// Adds an annotation to the module.
    ///
    /// \param name                    The simple name of the annotation.
    /// \param parameters              Types and names of the parameters. Can be \c NULL (treated
    ///                                like an empty parameter list).
    /// \param defaults                Default values. Can be \c NULL or incomplete. Feasible
    ///                                sub-expression kinds: constants, calls, and direct calls.
    /// \param parameter_annotations   Parameter annotations. Can be \c NULL or incomplete.
    /// \param annotations             Annotations of the annotation. Can be \c NULL.
    /// \param is_exported             Indicates whether the annotation will have the 'export'
    ///                                keyword.
    /// \param context                 The execution context can be used to pass options and to
    ///                                retrieve error and/or warning messages. Can be \c NULL.
    virtual Sint32 add_annotation(
        const char* name,
        const IType_list* parameters,
        const IExpression_list* defaults,
        const IAnnotation_list* parameter_annotations,
        const IAnnotation_block* annotations,
        bool is_exported,
        IMdl_execution_context* context) = 0;

    /// Adds an enum type to the module.
    ///
    /// \note Changing a particular enum type, i.e., removing it and re-adding it differently, is
    ////      \em not supported.
    ///
    /// \param name                    The simple name of the enum type.
    /// \param enumerators             Enumerators of the enum type. Must not be empty. Feasible
    ///                                sub-expression kinds: constants and direct calls.
    /// \param enumerator_annotations  Enumerator annotations. Can be \c NULL or incomplete.
    /// \param annotations             Annotations of the enum type. Can be \c NULL.
    /// \param is_exported             Indicates whether the enum type will have the 'export'
    ///                                keyword.
    /// \param context                 The execution context can be used to pass options and to
    ///                                retrieve error and/or warning messages. Can be \c NULL.
    virtual Sint32 add_enum_type(
        const char* name,
        const IExpression_list* enumerators,
        const IAnnotation_list* enumerator_annotations,
        const IAnnotation_block* annotations,
        bool is_exported,
        IMdl_execution_context* context) = 0;

    /// Adds a struct type to the module.
    ///
    /// \note Changing a particular struct type, i.e., removing it and re-adding it differently, is
    ////      \em not supported.
    ///
    /// \param name                    The simple name of the enum type.
    /// \param fields                  Fields of the struct type. Must not be empty.
    /// \param field_defaults          Defaults of the struct fields. Can be \c NULL or incomplete.
    ///                                Feasible sub-expression kinds: constants and direct calls.
    /// \param field_annotations       Field annotations of the struct type. Can be \c NULL or
    ///                                incomplete.
    /// \param annotations             Annotations of the struct type. Can be \c NULL.
    /// \param is_exported             Indicates whether the struct type will have the 'export'
    ///                                keyword.
    /// \param context                 The execution context can be used to pass options and to
    ///                                retrieve error and/or warning messages. Can be \c NULL.
    virtual Sint32 add_struct_type(
        const char* name,
        const IType_list* fields,
        const IExpression_list* field_defaults,
        const IAnnotation_list* field_annotations,
        const IAnnotation_block* annotations,
        bool is_exported,
        IMdl_execution_context* context) = 0;

    /// Adds a constant to the module.
    ///
    /// \param name                    The simple name of the constant.
    /// \param expr                    The value of the constant.
    ///                                Feasible sub-expression kinds: constants and direct calls.
    /// \param annotations             Annotations of the constant. Can be \c NULL.
    /// \param is_exported             Indicates whether the constant will have the 'export'
    ///                                keyword.
    /// \param context                 The execution context can be used to pass options and to
    ///                                retrieve error and/or warning messages. Can be \c NULL.
    virtual Sint32 add_constant(
        const char* name,
        const IExpression* expr,
        const IAnnotation_block* annotations,
        bool is_exported,
        IMdl_execution_context* context) = 0;

    /// Sets the annotations of the module itself.
    ///
    /// \param annotations             Annotations of the module. Pass \c NULL to remove existing
    ///                                annotations.
    /// \param context                 The execution context can be used to pass options and to
    ///                                retrieve error and/or warning messages. Can be \c NULL.
    virtual Sint32 set_module_annotations(
        const IAnnotation_block* annotations,
        IMdl_execution_context* context) = 0;

    /// Removes a material, function, enum or struct type from the module.
    ///
    /// \param name                    The simple name of material, function, enum or struct type to
    ///                                be removed.
    /// \param index                   The index of the function with the given name to be removed.
    ///                                Used to distinguish overloads of functions. Zero 0 for
    ///                                materials, enum or struct types.
    /// \param context                 The execution context can be used to pass options and to
    ///                                retrieve error and/or warning messages. Can be \c NULL.
    virtual Sint32 remove_entity(
        const char* name,
        Size index,
        IMdl_execution_context* context) = 0;

    /// Clears the module, i.e., removes all declarations from the module.
    virtual Sint32 clear_module( IMdl_execution_context* context) = 0;

    /// Analyzes which parameters need to be uniform.
    ///
    /// \param root_expr               Root expression of the graph, i.e., the body of the new
    ///                                material or function.
    /// \param root_expr_uniform       Indicates whether the root expression should be uniform.
    /// \param context                 The execution context can be used to pass options and to
    ///                                retrieve error and/or warning messages. Can be \c NULL.
    /// \return                        Indicates which parameters need to be uniform. The array
    ///                                might be shorter than expected if trailing parameters are
    ///                                not referenced by \p root_expr (or in case of errors).
    virtual const IArray* analyze_uniform(
        const IExpression* root_expr,
        bool root_expr_uniform,
        IMdl_execution_context* context) = 0;
};

/**@}*/ // end group mi_neuray_mdl_misc

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IMDL_MODULE_BUILDER_H

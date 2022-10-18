/***************************************************************************************************
 * Copyright 2022 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Utility class for MDL material and function definitions.

#ifndef MI_NEURAYLIB_DEFINITION_WRAPPER_H
#define MI_NEURAYLIB_DEFINITION_WRAPPER_H

#include <mi/base/handle.h>
#include <mi/neuraylib/assert.h>
#include <mi/neuraylib/iexpression.h>
#include <mi/neuraylib/ifunction_call.h>
#include <mi/neuraylib/ifunction_definition.h>
#include <mi/neuraylib/imaterial_definition.h>
#include <mi/neuraylib/imaterial_instance.h>
#include <mi/neuraylib/imdl_factory.h>
#include <mi/neuraylib/itransaction.h>
#include <mi/neuraylib/itype.h>
#include <mi/neuraylib/ivalue.h>

#include <string>

namespace mi {

namespace neuraylib {

/** \addtogroup mi_neuray_mdl_elements
@{
*/

/// A wrapper around the interface for MDL material and function definitions.
///
/// The purpose of the MDL definition wrapper is to simplify common working with MDL material and
/// function definitions. The key benefit is that it wraps API call sequences occurring in typical
/// tasks into one single method call, e.g., instance creation or obtaining the default values (as
/// long as their type is not too complex).
///
/// \note The definition wrapper does not expose the full functionality of the underlying
/// interface, but provides access to it via #get_scene_element().
///
/// See #mi::neuraylib::IFunction_definition for the underlying interface. See also
/// #mi::neuraylib::Argument_editor for a similar wrapper for MDL material instances and function
/// calls.
class Definition_wrapper
{
public:

    /// \name General methods
    //@{

    /// Constructs an MDL definition wrapper for a fixed material or function definition.
    ///
    /// \param transaction   The transaction to be used.
    /// \param name          The name of the wrapped material or function definition.
    /// \param mdl_factory   A pointer to the API component #mi::neuraylib::IMdl_factory. Needed
    ///                      only by #create_instance() if called with \c NULL as first argument,
    ///                      can be \c NULL otherwise.
    Definition_wrapper( ITransaction* transaction, const char* name, IMdl_factory* mdl_factory);

    /// Indicates whether the definition wrapper is in a valid state.
    ///
    /// The definition wrapper is valid if and only if the name passed in the constructor identifies
    /// a material or function definition. This method should be immediately called after invoking
    /// the constructor. If it returns \c false, no other methods of this class should be called.
    bool is_valid() const;

    /// Indicates whether the material or function definition referenced by this definition wrapper
    /// matches a definition in its owner module. Definitions might become invalid due to a module
    /// reload of the owner module itself or another module imported by the owner module.
    ///
    /// \param context  Execution context that can be queried for error messages
    ///                 after the operation has finished. Can be \c NULL.
    ///
    /// \return \c True, if the definition is valid, \c false otherwise.
    bool is_valid_definition( IMdl_execution_context* context) const;

    /// Indicates whether the definition wrapper acts on a material definition or on a function
    /// definition.
    ///
    /// \return    If \ref mi_mdl_materials_are_functions is enabled:
    ///            #mi::neuraylib::ELEMENT_TYPE_FUNCTION_DEFINITION, or undefined if #is_valid()
    ///            returns \c false. Otherwise:  mi::neuraylib::ELEMENT_TYPE_MATERIAL_DEFINITION,
    ///            or #mi::neuraylib::ELEMENT_TYPE_FUNCTION_DEFINITION, or undefined if #is_valid()
    ///            returns \c false.
    Element_type get_type() const;

    /// Returns the MDL name of the material or function definition.
    const char* get_mdl_definition() const;

    /// Returns the DB name of the corresponding module.
    const char* get_module() const;

    /// Indicates whether the material or function definition is exported by its module.
    bool is_exported() const;

    /// Indicates whether the definition represents a material.
    bool is_material() const;

    /// Returns the number of parameters.
    Size get_parameter_count() const;

    /// Returns the name of the parameter at \p index.
    ///
    /// \param index    The index of the parameter.
    /// \return         The name of the parameter, or \c NULL if \p index is out of range.
    const char* get_parameter_name( Size index) const;

    /// Returns the index position of a parameter.
    ///
    /// \param name     The name of the parameter.
    /// \return         The index of the parameter, or -1 if \p name is invalid.
    Size get_parameter_index( const char* name) const;

    /// Returns the types of all parameters.
    const IType_list* get_parameter_types() const;

    /// Returns the return type.
    const IType* get_return_type() const;

    /// Returns the resolved file name of the thumbnail image for this MDL definition.
    ///
    /// The function first checks for a thumbnail annotation. If the annotation is provided,
    /// it uses the 'name' argument of the annotation and resolves that in the MDL search path.
    /// If the annotation is not provided or file resolution fails, it checks for a file
    /// module_name.material_name.png next to the MDL module.
    /// In case this cannot be found either \c NULL is returned.
    const char* get_thumbnail() const;


    //@}
    /// \name Methods related to argument defaults
    //@{

    /// Returns the defaults of all parameters.
    ///
    /// \note Not all parameters have defaults. Hence, the indices in the returned expression list
    ///       do not necessarily coincide with the parameter indices of the definition. Therefore,
    ///       defaults should be retrieved via the name of the parameter instead of its index.
    const IExpression_list* get_defaults() const;

    /// Returns the default of a non-array parameter.
    ///
    /// If a literal \c 0 is passed for \p index, the call is ambiguous. You need to explicitly
    /// cast the value to #mi::Size.
    ///
    /// \note This method handles only constant defaults. Calls or parameter indices result in
    ///       error code -4.
    ///
    /// \param index        The index the parameter in question.
    /// \param[out] value   The default of the specified parameter.
    /// \return
    ///                     -  0: Success.
    ///                     - -1: #is_valid() returns \c false.
    ///                     - -2: \p index is out of range, or there is no default for this
    ///                           parameter.
    ///                     - -4: The default is not a constant.
    ///                     - -5: The type of the default does not match \p T.
    template<class T>
    Sint32 get_default( Size index, T& value) const;

    /// Returns the default of a non-array parameter.
    ///
    /// \note This method handles only constant defaults. Calls or parameter indices result in
    ///       error code -4.
    ///
    /// \param name         The name of the parameter in question.
    /// \param[out] value   The default of the specified parameter.
    /// \return
    ///                     -  0: Success.
    ///                     - -1: #is_valid() returns \c false.
    ///                     - -2: \p name is invalid, or there is no default for this parameter.
    ///                     - -4: The default is not a constant.
    ///                     - -5: The type of the default does not match \p T.
    template <class T>
    Sint32 get_default( const char* name, T& value) const;

    //@}
    /// \name Methods related to annotations
    //@{

    /// Returns the annotations for a material or function definition.
    const IAnnotation_block* get_annotations() const;

    /// Returns the annotations of all parameters.
    ///
    /// \note Not all parameters have annotations. Hence, the indices in the returned annotation
    ///       list do not necessarily coincide with the parameter indices of the definition.
    ///       Therefore, annotation blocks should be retrieved via the name of the parameter
    ///       instead of its index.
    const IAnnotation_list* get_parameter_annotations() const;

    /// Returns the annotations of the return type.
    const IAnnotation_block* get_return_annotations() const;

    /// Returns the \c enable_if conditions of all parameters.
    ///
    /// \note Not all parameters have a condition. Hence, the indices in the returned expression
    ///       list do not necessarily coincide with the parameter indices of this definition.
    ///       Therefore, conditions should be retrieved via the name of the parameter instead of
    ///       its index.
    const IExpression_list* get_enable_if_conditions() const;

    /// Returns the number of other parameters whose \c enable_if condition might depend on the
    /// argument of the given parameter.
    ///
    /// \param index    The index of the parameter.
    /// \return         The number of other parameters whose \c enable_if condition depends on this
    ///                 parameter argument.
    Size get_enable_if_users( Size index) const;

    /// Returns the index of a parameter whose \c enable_if condition might depend on the
    /// argument of the given parameter.
    ///
    /// \param index    The index of the parameter.
    /// \param u_index  The index of the enable_if user.
    /// \return         The index of a parameter whose \c enable_if condition depends on this
    ///                 parameter argument, or ~0 if indexes are out of range.
    Size get_enable_if_user( Size index, Size u_index) const;

    //@}
    /// \name Methods related to instantiation of definitions
    //@{

    /// Creates an instance of the material or function definition.
    ///
    /// \param arguments   If not \c NULL, then these arguments are used for the material instance
    ///                    or function call. If an argument is missing, then the default for that
    ///                    parameter is used. If there is no default, range annotations are used to
    ///                    obtain a suitable value, or the argument is default-constructed as last
    ///                    resort. Must not be \c NULL (or missing any arguments) in case of \ref
    ///                    mi_neuray_mdl_template_like_function_definitions.
    /// \param[out] errors An optional pointer to an #mi::Sint32 to which an error code will be
    ///                    written. The error codes have the following meaning:
    ///                    -  0: Success. If \p arguments is \c NULL, then the method always
    ///                          succeeds.
    ///                    - -1: An argument for a non-existing parameter was provided in
    ///                          \p arguments.
    ///                    - -2: The type of an argument in \p arguments does not have the correct
    ///                          type, see #get_parameter_types().
    ///                    - -3: A parameter that has no default was not provided with an argument
    ///                          value.
    ///                    - -4: The function definition is one of
    ///                          \ref mi_neuray_mdl_template_like_function_definitions and
    ///                          \p arguments is \c NULL.
    /// \return            The constructed material instance or function call, or \c NULL in case
    ///                    of errors.
    IScene_element* create_instance(
        const IExpression_list* arguments = 0, Sint32* errors = 0) const;

    /// Creates an instance of the material or function definition.
    ///
    /// \param arguments   If not \c NULL, then these arguments are used for the material instance
    ///                    or function call. If an argument is missing, then the default for that
    ///                    parameter is used. If there is no default, range annotations are used to
    ///                    obtain a suitable value, or the argument is default-constructed as last
    ///                    resort. Must not be \c NULL (or missing any arguments) in case of \ref
    ///                    mi_neuray_mdl_template_like_function_definitions.
    /// \param[out] errors An optional pointer to an #mi::Sint32 to which an error code will be
    ///                    written. The error codes have the following meaning:
    ///                    -  0: Success. If \p arguments is \c NULL, then the method always
    ///                          succeeds.
    ///                    - -1: An argument for a non-existing parameter was provided in
    ///                          \p arguments.
    ///                    - -2: The type of an argument in \p arguments does not have the correct
    ///                          type, see #get_parameter_types().
    ///                    - -3: A parameter that has no default was not provided with an argument
    ///                          value.
    ///                    - -4: The function definition is one of
    ///                          \ref mi_neuray_mdl_template_like_function_definitions and
    ///                          \p arguments is \c NULL.
    /// \return            The constructed material instance or function call, or \c NULL in case
    ///                    of errors.
    ///
    /// \tparam T          Either #mi::neuraylib::IMaterial_instance or
    ///                    #mi::neuraylib::IFunction_call.
    template <class T>
    T* create_instance( const IExpression_list* arguments = 0, Sint32* errors = 0) const
    {
        IScene_element* ptr_iscene_element = create_instance( arguments, errors);
        if(  !ptr_iscene_element)
            return 0;
        T* ptr_T = static_cast<T*>( ptr_iscene_element->get_interface( typename T::IID()));
        ptr_iscene_element->release();
        return ptr_T;
    }

    //@}
    /// \name Methods related to member access.
    //@{

    /// Get the transaction.
    ITransaction* get_transaction() const;

    /// Get the MDL factory.
    IMdl_factory* get_mdl_factory() const;

    /// Get the MDL function or material definition.
    const IScene_element* get_scene_element() const;

    /// Get the element type.
    Element_type get_element_type() const;

    /// Get the DB name of the MDL function or material definition.
    const std::string& get_name() const;

    //@}

private:

    base::Handle<ITransaction> m_transaction;
    base::Handle<const IScene_element> m_access;
    base::Handle<IMdl_factory> m_mdl_factory;
    Element_type m_type;
    std::string m_name;
};

/**@}*/ // end group mi_neuray_mdl_elements

inline Definition_wrapper::Definition_wrapper(
    ITransaction* transaction, const char* name, IMdl_factory* mdl_factory)
{
    mi_neuray_assert( transaction);
    mi_neuray_assert( name);

    m_transaction = make_handle_dup( transaction);
    m_name = name;
    m_mdl_factory = make_handle_dup( mdl_factory);
    m_access = transaction->access<IScene_element>( name);
    m_type = m_access ? m_access->get_element_type() : static_cast<Element_type>( 0);
}

inline bool Definition_wrapper::is_valid() const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    return m_access
        && (m_type == ELEMENT_TYPE_MATERIAL_DEFINITION
        ||  m_type == ELEMENT_TYPE_FUNCTION_DEFINITION);
#else // MI_NEURAYLIB_DEPRECATED_13_0
    return m_access
        && (m_type == ELEMENT_TYPE_FUNCTION_DEFINITION);
#endif // MI_NEURAYLIB_DEPRECATED_13_0
}


inline bool Definition_wrapper::is_valid_definition( IMdl_execution_context* context) const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_DEFINITION) {

        base::Handle<const IMaterial_definition> md(
            m_access->get_interface<IMaterial_definition>());
        return md->is_valid( context);
    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_DEFINITION) {

        base::Handle<const IFunction_definition> fd(
            m_access->get_interface<IFunction_definition>());
        return fd->is_valid( context);
    } else
        return false;
}

inline Element_type Definition_wrapper::get_type() const
{
    return m_type;
}

inline const char* Definition_wrapper::get_mdl_definition() const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_DEFINITION) {

        base::Handle<const IMaterial_definition> md(
            m_access->get_interface<IMaterial_definition>());
        return md->get_mdl_name();

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_DEFINITION) {

        base::Handle<const IFunction_definition> fd(
            m_access->get_interface<IFunction_definition>());
        return fd->get_mdl_name();

    } else
        return 0;
}

inline const char* Definition_wrapper::get_module() const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_DEFINITION) {

        base::Handle<const IMaterial_definition> md(
            m_access->get_interface<IMaterial_definition>());
        return md->get_module();

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_DEFINITION) {

        base::Handle<const IFunction_definition> fd(
            m_access->get_interface<IFunction_definition>());
        return fd->get_module();

    } else
        return 0;
}

inline bool Definition_wrapper::is_exported() const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_DEFINITION) {

        base::Handle<const IMaterial_definition> md(
            m_access->get_interface<IMaterial_definition>());
        return md->is_exported();

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_DEFINITION) {

        base::Handle<const IFunction_definition> fd(
            m_access->get_interface<IFunction_definition>());
        return fd->is_exported();

    } else
        return false;
}

inline bool Definition_wrapper::is_material() const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_DEFINITION) {

        return true;

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_DEFINITION) {

        base::Handle<const IFunction_definition> fd(
            m_access->get_interface<IFunction_definition>());
        return fd->is_material();

    } else
        return false;
}

inline Size Definition_wrapper::get_parameter_count() const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_DEFINITION) {

        base::Handle<const IMaterial_definition> md(
            m_access->get_interface<IMaterial_definition>());
        return md->get_parameter_count();

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_DEFINITION) {

        base::Handle<const IFunction_definition> fd(
            m_access->get_interface<IFunction_definition>());
        return fd->get_parameter_count();

    } else
        return 0;
}

inline const char* Definition_wrapper::get_parameter_name( Size index) const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_DEFINITION) {

        base::Handle<const IMaterial_definition> md(
            m_access->get_interface<IMaterial_definition>());
        return md->get_parameter_name( index);

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_DEFINITION) {

        base::Handle<const IFunction_definition> fd(
            m_access->get_interface<IFunction_definition>());
        return fd->get_parameter_name( index);

    } else
        return 0;
}

inline Size Definition_wrapper::get_parameter_index( const char* name) const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_DEFINITION) {

        base::Handle<const IMaterial_definition> md(
            m_access->get_interface<IMaterial_definition>());
        return md->get_parameter_index( name);

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_DEFINITION) {

        base::Handle<const IFunction_definition> fd(
            m_access->get_interface<IFunction_definition>());
        return fd->get_parameter_index( name);

    } else
        return 0;
}

inline const IType_list* Definition_wrapper::get_parameter_types() const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_DEFINITION) {

        base::Handle<const IMaterial_definition> md(
            m_access->get_interface<IMaterial_definition>());
        return md->get_parameter_types();

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_DEFINITION) {

        base::Handle<const IFunction_definition> fd(
            m_access->get_interface<IFunction_definition>());
        return fd->get_parameter_types();

    } else
        return 0;
}

inline const IType* Definition_wrapper::get_return_type() const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_DEFINITION) {

        return 0;

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_DEFINITION) {

        base::Handle<const IFunction_definition> fd(
            m_access->get_interface<IFunction_definition>());
        return fd->get_return_type();

    } else
        return 0;
}

inline const char* Definition_wrapper::get_thumbnail() const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_DEFINITION) {

        base::Handle<const IMaterial_definition> md(
            m_access->get_interface<IMaterial_definition>());
        return md->get_thumbnail();

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_DEFINITION) {

        base::Handle<const IFunction_definition> fd(
            m_access->get_interface<IFunction_definition>());
        return fd->get_thumbnail();

    } else
        return 0;
}

inline const IExpression_list* Definition_wrapper::get_defaults() const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_DEFINITION) {

        base::Handle<const IMaterial_definition> md(
            m_access->get_interface<IMaterial_definition>());
        return md->get_defaults();

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_DEFINITION) {

        base::Handle<const IFunction_definition> fd(
            m_access->get_interface<IFunction_definition>());
        return fd->get_defaults();

    } else
        return 0;
}

template <class T>
Sint32 Definition_wrapper::get_default( Size index, T& value) const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_DEFINITION) {

        base::Handle<const IMaterial_definition> md(
            m_access->get_interface<IMaterial_definition>());
        base::Handle<const IExpression_list> defaults( md->get_defaults());
        base::Handle<const IExpression> default_( defaults->get_expression( index));
        if( !default_)
            return -2;
        base::Handle<const IExpression_constant> default_constant(
            default_->get_interface<IExpression_constant>());
        if( !default_constant)
            return -4;
        base::Handle<const IValue> default_value( default_constant->get_value());
        Sint32 result = get_value( default_value.get(), value);
        return result == 0 ? 0 : -5;

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_DEFINITION) {

        base::Handle<const IFunction_definition> fd(
            m_access->get_interface<IFunction_definition>());
        base::Handle<const IExpression_list> defaults( fd->get_defaults());
        base::Handle<const IExpression> default_( defaults->get_expression( index));
        if( !default_)
            return -2;
        base::Handle<const IExpression_constant> default_constant(
            default_->get_interface<IExpression_constant>());
        if( !default_constant)
            return -4;
        base::Handle<const IValue> default_value( default_constant->get_value());
        Sint32 result = get_value( default_value.get(), value);
        return result == 0 ? 0 : -5;

    } else
        return -1;
}

template <class T>
Sint32 Definition_wrapper::get_default( const char* name, T& value) const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_DEFINITION) {

        base::Handle<const IMaterial_definition> md(
            m_access->get_interface<IMaterial_definition>());
        base::Handle<const IExpression_list> defaults( md->get_defaults());
        base::Handle<const IExpression> default_( defaults->get_expression( name));
        if( !default_)
            return -2;
        base::Handle<const IExpression_constant> default_constant(
            default_->get_interface<IExpression_constant>());
        if( !default_constant)
            return -4;
        base::Handle<const IValue> default_value( default_constant->get_value());
        Sint32 result = get_value( default_value.get(), value);
        return result == 0 ? 0 : -5;

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_DEFINITION) {

        base::Handle<const IFunction_definition> fd(
            m_access->get_interface<IFunction_definition>());
        base::Handle<const IExpression_list> defaults( fd->get_defaults());
        base::Handle<const IExpression> default_( defaults->get_expression( name));
        if( !default_)
            return -2;
        base::Handle<const IExpression_constant> default_constant(
            default_->get_interface<IExpression_constant>());
        if( !default_constant)
            return -4;
        base::Handle<const IValue> default_value( default_constant->get_value());
        Sint32 result = get_value( default_value.get(), value);
        return result == 0 ? 0 : -5;

    } else
        return -1;
}

inline const IAnnotation_block* Definition_wrapper::get_annotations() const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_DEFINITION) {

        base::Handle<const IMaterial_definition> md(
            m_access->get_interface<IMaterial_definition>());
        return md->get_annotations();

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_DEFINITION) {

        base::Handle<const IFunction_definition> fd(
            m_access->get_interface<IFunction_definition>());
        return fd->get_annotations();

    } else
        return 0;
}

inline const IAnnotation_list* Definition_wrapper::get_parameter_annotations() const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_DEFINITION) {

        base::Handle<const IMaterial_definition> md(
            m_access->get_interface<IMaterial_definition>());
        return md->get_parameter_annotations();

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_DEFINITION) {

        base::Handle<const IFunction_definition> fd(
            m_access->get_interface<IFunction_definition>());
        return fd->get_parameter_annotations();

    } else
        return 0;
}

inline const IAnnotation_block* Definition_wrapper::get_return_annotations() const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_DEFINITION) {

        base::Handle<const IMaterial_definition> md(
            m_access->get_interface<IMaterial_definition>());
        return md->get_return_annotations();

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_DEFINITION) {

        base::Handle<const IFunction_definition> fd(
            m_access->get_interface<IFunction_definition>());
        return fd->get_return_annotations();

    } else
        return 0;
}

inline const IExpression_list* Definition_wrapper::get_enable_if_conditions() const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_DEFINITION) {

        base::Handle<const IMaterial_definition> md(
            m_access->get_interface<IMaterial_definition>());
        return md->get_enable_if_conditions();

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_DEFINITION) {

        base::Handle<const IFunction_definition> fd(
            m_access->get_interface<IFunction_definition>());
        return fd->get_enable_if_conditions();

    } else
        return 0;
}

inline Size Definition_wrapper::get_enable_if_users( Size index) const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_DEFINITION) {

        base::Handle<const IMaterial_definition> md(
            m_access->get_interface<IMaterial_definition>());
        return md->get_enable_if_users( index);

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_DEFINITION) {

        base::Handle<const IFunction_definition> fd(
            m_access->get_interface<IFunction_definition>());
        return fd->get_enable_if_users( index);

    } else
        return Size(~0);
}

inline Size Definition_wrapper::get_enable_if_user( Size index, Size u_index) const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_DEFINITION) {

        base::Handle<const IMaterial_definition> md(
            m_access->get_interface<IMaterial_definition>());
        return md->get_enable_if_user( index, u_index);

    }
    else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_DEFINITION) {

        base::Handle<const IFunction_definition> fd(
            m_access->get_interface<IFunction_definition>());
        return fd->get_enable_if_user( index, u_index);

    } else
        return Size(~0);
}

inline IScene_element* Definition_wrapper::create_instance(
    const IExpression_list* arguments, Sint32* errors) const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_DEFINITION) {

        base::Handle<const IMaterial_definition> md(
            m_access->get_interface<IMaterial_definition>());
        if( arguments)
            return md->create_material_instance( arguments, errors);

        base::Handle<const IType_list> parameter_types( md->get_parameter_types());
        base::Handle<const IExpression_list> defaults( md->get_defaults());
        base::Handle<const IAnnotation_list> parameter_annotations(
             md->get_parameter_annotations());
        base::Handle<IValue_factory> vf( m_mdl_factory->create_value_factory( m_transaction.get()));
        base::Handle<IExpression_factory> ef(
            m_mdl_factory->create_expression_factory( m_transaction.get()));
        base::Handle<IExpression_list> local_arguments( ef->create_expression_list());

        Size count = md->get_parameter_count();
        for( Size i = 0; i < count; ++i) {
            const char* name = md->get_parameter_name( i);
            base::Handle<const IExpression> default_( defaults->get_expression( name));
            if( !default_) {
                base::Handle<const IType> type( parameter_types->get_type( i));
                base::Handle<const IAnnotation_block> block(
                    parameter_annotations->get_annotation_block( name));
                base::Handle<IValue> value( vf->create( type.get(), block.get()));
                base::Handle<IExpression> expr( ef->create_constant( value.get()));
                local_arguments->add_expression( name, expr.get());
            }
        }
        return md->create_material_instance( local_arguments.get(), errors);

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_DEFINITION) {

        base::Handle<const IFunction_definition> fd(
            m_access->get_interface<IFunction_definition>());
        if( arguments)
            return fd->create_function_call( arguments, errors);

        IFunction_definition::Semantics semantic = fd->get_semantic();
        if(    semantic == IFunction_definition::DS_INTRINSIC_DAG_ARRAY_CONSTRUCTOR
            || semantic == IFunction_definition::DS_INTRINSIC_DAG_ARRAY_LENGTH
            || semantic == IFunction_definition::DS_ARRAY_INDEX
            || semantic == IFunction_definition::DS_TERNARY
            || semantic == IFunction_definition::DS_CAST) {
            if( errors)
                *errors = -4;
            return 0;
        }

        base::Handle<const IType_list> parameter_types( fd->get_parameter_types());
        base::Handle<const IExpression_list> defaults( fd->get_defaults());
        base::Handle<const IAnnotation_list> parameter_annotations(
            fd->get_parameter_annotations());
        base::Handle<IValue_factory> vf( m_mdl_factory->create_value_factory( m_transaction.get()));
        base::Handle<IExpression_factory> ef(
            m_mdl_factory->create_expression_factory( m_transaction.get()));
        base::Handle<IExpression_list> local_arguments( ef->create_expression_list());

        Size count = fd->get_parameter_count();
        for( Size i = 0; i < count; ++i) {
                const char* name = fd->get_parameter_name( i);
            base::Handle<const IExpression> default_( defaults->get_expression( name));
            if( !default_) {
                base::Handle<const IType> type( parameter_types->get_type( i));
                base::Handle<const IAnnotation_block> block(
                    parameter_annotations->get_annotation_block( name));
                base::Handle<IValue> value( vf->create( type.get(), block.get()));
                base::Handle<IExpression> expr( ef->create_constant( value.get()));
                local_arguments->add_expression( name, expr.get());
            }
        }
        return fd->create_function_call( local_arguments.get(), errors);

    } else
        return 0;
}

inline ITransaction* Definition_wrapper::get_transaction() const
{
    m_transaction->retain();
    return m_transaction.get();
}

inline IMdl_factory* Definition_wrapper::get_mdl_factory() const
{
    m_mdl_factory->retain();
    return m_mdl_factory.get();
}

inline const IScene_element* Definition_wrapper::get_scene_element() const
{
    m_access->retain();
    return m_access.get();
}

inline Element_type Definition_wrapper::get_element_type() const
{
    return m_type;
}

inline const std::string& Definition_wrapper::get_name() const
{
    return m_name;
}

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_DEFINITION_WRAPPER_H

/***************************************************************************************************
 * Copyright 2022 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief   Utility class for MDL material instances and function calls.

#ifndef MI_NEURAYLIB_ARGUMENT_EDITOR_H
#define MI_NEURAYLIB_ARGUMENT_EDITOR_H

#include <mi/base/handle.h>
#include <mi/neuraylib/assert.h>
#include <mi/neuraylib/iexpression.h>
#include <mi/neuraylib/ifunction_call.h>
#include <mi/neuraylib/imaterial_instance.h>
#include <mi/neuraylib/imdl_factory.h>
#include <mi/neuraylib/imdl_evaluator_api.h>
#include <mi/neuraylib/itransaction.h>
#include <mi/neuraylib/itype.h>
#include <mi/neuraylib/ivalue.h>

#include <string>

namespace mi {

namespace neuraylib {

class IMdl_execution_context;

/** \addtogroup mi_neuray_mdl_elements
@{
*/

/// A wrapper around the interface for MDL material instances and function calls.
///
/// The purpose of the MDL argument editor is to simplify working with MDL material instances and
/// function calls. The key benefit is that it wraps API call sequences occurring in typical tasks
/// into one single method call, e.g., changing arguments (as long as their type is not too
/// complex): Typically this requires at least seven API calls (even more in case of arrays or if
/// you do not use #mi::neuraylib::set_value()). The argument editor offers a single method to
/// support this task.
///
/// \note The argument editor does not expose the full functionality of the underlying interface,
/// but provides access to it via #get_scene_element().
///
/// See #mi::neuraylib::IFunction_call for the underlying interface. See also
/// #mi::neuraylib::Definition_wrapper for a similar wrapper for MDL material and function
/// definitions.
class Argument_editor
{
public:

    /// \name General methods
    //@{

    /// Constructs an MDL argument editor for a fixed material instance or function call.
    ///
    /// \param transaction       The transaction to be used.
    /// \param name              The name of the wrapped material instance or function call.
    /// \param mdl_factory       A pointer to the API component #mi::neuraylib::IMdl_factory.
    ///                          Needed by all mutable methods, can be \c NULL if only const
    ///                          methods are used.
    /// \param intent_to_edit    For best performance, the parameter should be set to \c true iff
    ///                          the intention is to edit the material instance or function call.
    ///                          This parameter is for performance optimizations only; the argument
    ///                          editor will work correctly independently of the value used. The
    ///                          performance penalty for setting it incorrectly to \c true is
    ///                          usually higher than setting it incorrectly to \c false. If in
    ///                          doubt, use the default of \c false.
    Argument_editor(
        ITransaction* transaction,
        const char* name,
        IMdl_factory* mdl_factory,
        bool intent_to_edit = false);

    /// Indicates whether the argument editor is in a valid state.
    ///
    /// The argument editor is valid if and only if the name passed in the constructor identifies a
    /// material instance or function call. This method should be immediately called after invoking
    /// the constructor. If it returns \c false, no other methods of this class should be called.
    bool is_valid() const;

    /// Indicates whether the material instance or function call referenced by this argument editor
    /// is valid. A material instance or function call is valid if itself and all calls attached
    /// to its arguments point to a valid definition.
    ///
    /// \param context  Execution context that can be queried for error messages
    ///                 after the operation has finished. Can be \c NULL.
    ///
    /// \return \c True, if the instance is valid, \c false otherwise.
    bool is_valid_instance( IMdl_execution_context* context) const;

    /// Attempts to repair an invalid material instance or function call.
    ///
    /// \param flags    Repair options, see #mi::neuraylib::Mdl_repair_options.
    /// \param context  Execution context that can be queried for error messages
    ///                 after the operation has finished. Can be \c NULL.
    /// \return
    ///     -   0:   Success.
    ///     -  -1:   Repair failed. Check the \c context for details.
    mi::Sint32 repair(mi::Uint32 flags, IMdl_execution_context* context);

    /// Indicates whether the argument editor acts on a material instance or on a function call.
    ///
    /// \return    If \ref mi_mdl_materials_are_functions is enabled:
    ///            #mi::neuraylib::ELEMENT_TYPE_FUNCTION_DEFINITION, or undefined if #is_valid()
    ///            returns \c false. Otherwise: mi::neuraylib::ELEMENT_TYPE_MATERIAL_DEFINITION,
    ///            or #mi::neuraylib::ELEMENT_TYPE_FUNCTION_DEFINITION, or undefined if #is_valid()
    ///            returns \c false.
    Element_type get_type() const;

    /// Returns the DB name of the corresponding material or function definition.
    const char* get_definition() const;

    /// Returns the MDL name of the corresponding material or function definition.
    const char* get_mdl_definition() const;

    /// Indicates whether the argument editor acts on a function call that is an instance of the
    /// array constructor.
    ///
    /// \see \ref mi_neuray_mdl_arrays
    bool is_array_constructor() const;

    /// Indicates whether the argument editor acts on a material instance.
    bool is_material() const;

    /// Returns the return type.
    const IType* get_return_type() const;

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

    /// Checks the \c enable_if condition of the given parameter.
    ///
    /// \param index      The index of the parameter.
    /// \param evaluator  A pointer to the API component #mi::neuraylib::IMdl_evaluator_api.
    /// \return           \c false if the \c enable_if condition of this parameter evaluated to
    ///                   \c false, \c true otherwise
    bool is_parameter_enabled( Size index, IMdl_evaluator_api* evaluator) const;

    /// Returns all arguments.
    const IExpression_list* get_arguments() const;

    /// Returns the expression kind of an argument.
    ///
    /// This method should be used to figure out whether
    /// #mi::neuraylib::Argument_editor::get_value() or #mi::neuraylib::Argument_editor::get_call()
    /// should be used for reading an argument.
    IExpression::Kind get_argument_kind( Size parameter_index) const;

    /// Returns the expression kind of an argument.
    ///
    /// This method should be used to figure out whether
    /// #mi::neuraylib::Argument_editor::get_value() or #mi::neuraylib::Argument_editor::get_call()
    /// should be used for reading an argument.
    IExpression::Kind get_argument_kind( const char* parameter_name) const;

    //@}
    /// \name Methods related to resetting of arguments
    //@{

    /// Resets the argument at \p index.
    ///
    /// If the definition has a default for this parameter (and it does not violate a
    /// potential uniform requirement), then a clone of it is used as new argument. Otherwise, a
    /// constant expression is created, observing range annotations if present (see the overload of
    /// #mi::neuraylib::IValue_factory::create() with two arguments).
    ///
    /// \param index        The index of the argument.
    /// \return
    ///                     -   0: Success.
    ///                     -  -2: Parameter \p index does not exist.
    ///                     -  -4: The function call or material instance is immutable (because it
    ///                            appears in a default of a function or material definition).
    ///                     -  -9: The function call or material instance is not valid (see
    ///                            #is_valid()).
    Sint32 reset_argument( Size index);

    /// Sets an argument identified by name to its default.
    ///
    /// If the definition has a default for this parameter (and it does not violate a
    /// potential uniform requirement), then a clone of it is used as new argument. Otherwise, a
    /// constant expression is created, observing range annotations if present (see the overload of
    /// #mi::neuraylib::IValue_factory::create() with two arguments).
    ///
    /// \param name         The name of the parameter.
    /// \return
    ///                     -   0: Success.
    ///                     -  -1: Invalid parameters (\c NULL pointer).
    ///                     -  -2: Parameter \p name does not exist.
    ///                     -  -4: The function call or material instance is immutable (because it
    ///                            appears in a default of a function or material definition).
    ///                     -  -9: The function call or material instance is not valid (see
    ///                            #is_valid()).
    Sint32 reset_argument( const char* name);

    //@}
    /// \name Methods related to constant expressions for arguments
    //@{

    /// Returns a non-compound argument (values of constants only, no calls).
    ///
    /// If a literal \c 0 is passed for \p parameter_index, the call is ambiguous. You need to
    /// explicitly cast the value to #mi::Size.
    ///
    /// \param parameter_index  The index of the argument in question.
    /// \param[out] value       The current value of the specified argument.
    /// \return
    ///                         -  0: Success.
    ///                         - -1: #is_valid() returns \c false.
    ///                         - -2: \p parameter_index is out of range.
    ///                         - -4: The argument is not a constant.
    ///                         - -5: The type of the argument does not match \p T.
    template<class T>
    Sint32 get_value( Size parameter_index, T& value) const;

    /// Returns a non-compound argument (values of constants only, no calls).
    ///
    /// \param parameter_name   The name of the argument in question.
    /// \param[out] value       The current value of the specified argument.
    /// \return
    ///                         -  0: Success.
    ///                         - -1: #is_valid() returns \c false.
    ///                         - -2: \p parameter_name is invalid.
    ///                         - -4: The argument is not a constant.
    ///                         - -5: The type of the argument does not match \p T.
    template <class T>
    Sint32 get_value( const char* parameter_name, T& value) const;

    /// Returns a component of a compound argument (values of constants only, no calls).
    ///
    /// If a literal \c 0 is passed for \p parameter_index, the call is ambiguous. You need to
    /// explicitly cast the value to #mi::Size.
    ///
    /// \param parameter_index  The index of the argument in question.
    /// \param component_index  The index of the component in question.
    /// \param[out] value       The current value of the specified argument.
    /// \return
    ///                         -  0: Success.
    ///                         - -1: #is_valid() returns \c false.
    ///                         - -2: \p parameter_index is out of range.
    ///                         - -3: \p component_index is out of range.
    ///                         - -4: The argument is not a constant.
    ///                         - -5: The type of the argument does not match \p T.
    template<class T>
    Sint32 get_value( Size parameter_index, Size component_index, T& value) const;

    /// Returns a component of a compound argument (values of constants only, no calls).
    ///
    /// \param parameter_name   The name of the argument in question.
    /// \param component_index  The index of the component in question.
    /// \param[out] value       The current value of the specified argument.
    /// \return
    ///                         -  0: Success.
    ///                         - -1: #is_valid() returns \c false.
    ///                         - -2: \p parameter_name is invalid.
    ///                         - -3: \p component_index is out of range.
    ///                         - -4: The argument is not a constant.
    ///                         - -5: The type of the argument does not match \p T.
    template <class T>
    Sint32 get_value( const char* parameter_name, Size component_index, T& value) const;

    /// Returns a field of a struct argument (values of constants only, no calls).
    ///
    /// If a literal \c 0 is passed for \p parameter_index, the call is ambiguous. You need to
    /// explicitly cast the value to #mi::Size.
    ///
    /// \param parameter_index  The index of the argument in question.
    /// \param field_name       The name of the struct field in question.
    /// \param[out] value       The current value of the specified argument.
    /// \return
    ///                         -  0: Success.
    ///                         - -1: #is_valid() returns \c false.
    ///                         - -2: \p parameter_index is out of range.
    ///                         - -3: \p field_name is invalid.
    ///                         - -4: The argument is not a constant.
    ///                         - -5: The type of the argument does not match \p T.
    template<class T>
    Sint32 get_value( Size parameter_index, const char* field_name, T& value) const;

    /// Returns a field of a struct argument (values of constants only, no calls).
    ///
    /// \param parameter_name   The name of the argument in question.
    /// \param field_name       The name of the struct field in question.
    /// \param[out] value       The current value of the specified argument.
    /// \return
    ///                         -  0: Success.
    ///                         - -1: #is_valid() returns \c false.
    ///                         - -2: \p parameter_name is invalid.
    ///                         - -3: \p field_name is invalid.
    ///                         - -4: The argument is not a constant.
    ///                         - -5: The type of the argument does not match \p T.
    template <class T>
    Sint32 get_value( const char* parameter_name, const char* field_name, T& value) const;

    /// Sets a non-compound argument.
    ///
    /// If the current argument is a call expression, it will be replaced by a constant expression.
    ///
    /// If a literal \c 0 is passed for \p parameter_index, the call is ambiguous. You need to
    /// explicitly cast the value to #mi::Size.
    ///
    /// \param parameter_index  The index of the argument in question.
    /// \param value            The new value of the specified argument.
    /// \return
    ///                         -  0: Success.
    ///                         - -1: #is_valid() returns \c false.
    ///                         - -2: \p parameter_index is out of range.
    ///                         - -4: The material instance or function call is immutable.
    ///                         - -5: The type of the argument does not match \p T.
    template<class T>
    Sint32 set_value( Size parameter_index, const T& value);

    /// Sets a non-compound argument.
    ///
    /// If the current argument is a call expression, it will be replaced by a constant expression.
    ///
    /// \param parameter_name   The name of the argument in question.
    /// \param value            The new value of the specified argument.
    /// \return
    ///                         -  0: Success.
    ///                         - -1: #is_valid() returns \c false.
    ///                         - -2: \p parameter_name is invalid.
    ///                         - -4: The material instance or function call is immutable.
    ///                         - -5: The type of the argument does not match \p T.
    template <class T>
    Sint32 set_value( const char* parameter_name, const T& value);

    /// Sets a component of a compound argument.
    ///
    /// If the current argument is a call expression, it will be replaced by a constant expression.
    ///
    /// This method does not change the size of dynamic arrays. You need to call explicitly
    /// #mi::neuraylib::Argument_editor::set_array_size() for that.
    ///
    /// If a literal \c 0 is passed for \p parameter_index, the call is ambiguous. You need to
    /// explicitly cast the value to #mi::Size.
    ///
    /// \param parameter_index  The index of the argument in question.
    /// \param component_index  The index of the component in question.
    /// \param value            The new value of the specified argument.
    /// \return
    ///                         -  0: Success.
    ///                         - -1: #is_valid() returns \c false.
    ///                         - -2: \p parameter_index is out of range.
    ///                         - -3: \p component_index is out of range.
    ///                         - -4: The material instance or function call is immutable.
    ///                         - -5: The type of the argument does not match \p T.
    template<class T>
    Sint32 set_value( Size parameter_index, Size component_index, const T& value);

    /// Sets a component of a compound argument.
    ///
    /// If the current argument is a call expression, it will be replaced by a constant expression.
    ///
    /// This method does not change the size of dynamic arrays. You need to call explicitly
    /// #mi::neuraylib::Argument_editor::set_array_size() for that.
    ///
    /// \param parameter_name   The name of the argument in question.
    /// \param component_index  The index of the component in question.
    /// \param value            The new value of the specified argument.
    /// \return
    ///                         -  0: Success.
    ///                         - -1: #is_valid() returns \c false.
    ///                         - -2: \p parameter_name is invalid.
    ///                         - -3: \p component_index is out of range.
    ///                         - -4: The material instance or function call is immutable.
    ///                         - -5: The type of the argument does not match \p T.
    template <class T>
    Sint32 set_value( const char* parameter_name, Size component_index, const T& value);

    /// Sets a field of a struct argument.
    ///
    /// If the current argument is a call expression, it will be replaced by a constant expression.
    ///
    /// If a literal \c 0 is passed for \p parameter_index, the call is ambiguous. You need to
    /// explicitly cast the value to #mi::Size.
    ///
    /// \param parameter_index  The index of the argument in question.
    /// \param field_name       The name of the struct_field in question.
    /// \param value            The new value of the specified argument.
    /// \return
    ///                         -  0: Success.
    ///                         - -1: #is_valid() returns \c false.
    ///                         - -2: \p parameter_index is out of range.
    ///                         - -3: \p field_name is invalid.
    ///                         - -4: The material instance or function call is immutable.
    ///                         - -5: The type of the argument does not match \p T.
    template<class T>
    Sint32 set_value( Size parameter_index, const char* field_name, const T& value);

    /// Sets a field of a struct argument.
    ///
    /// If the current argument is a call expression, it will be replaced by a constant expression.
    ///
    /// \param parameter_name   The name of the argument in question.
    /// \param field_name       The name of the struct_field in question.
    /// \param value            The new value of the specified argument.
    /// \return
    ///                         -  0: Success.
    ///                         - -1: #is_valid() returns \c false.
    ///                         - -2: \p parameter_name is invalid.
    ///                         - -3: \p field_name is invalid.
    ///                         - -4: The material instance or function call is immutable.
    ///                         - -5: The type of the argument does not match \p T.
    template <class T>
    Sint32 set_value( const char* parameter_name, const char* field_name, const T& value);

    /// Returns the length of an array argument.
    ///
    /// If a literal \c 0 is passed for \p argument_index, the call is ambiguous. You need to
    /// explicitly cast the value to #mi::Uint32.
    ///
    /// \param parameter_index  The index of the argument in question.
    /// \param[out] size        The current length of the array of the specified argument.
    /// \return
    ///                         -  0: Success.
    ///                         - -1: #is_valid() returns \c false.
    ///                         - -2: \p parameter_index is out of range.
    ///                         - -4: The argument is not a constant.
    ///                         - -5: The argument is not an array.
    Sint32 get_array_length( Uint32 parameter_index, Size& size) const;

    /// Returns the length of an array argument.
    ///
    /// \param parameter_name   The name of the argument in question.
    /// \param[out] size        The current length of the array of the specified argument.
    /// \return
    ///                         -  0: Success.
    ///                         - -1: #is_valid() returns \c false.
    ///                         - -2: \p parameter_name is invalid.
    ///                         - -4: The argument is not a constant.
    ///                         - -5: The argument is not an array.
    Sint32 get_array_length( const char* parameter_name, Size& size) const;

    /// Sets the length of a dynamic array argument.
    ///
    /// If the current argument is a call expression, it will be replaced by a constant expression.
    ///
    /// If a literal \c 0 is passed for \p argument_index, the call is ambiguous. You need to
    /// explicitly cast the value to #mi::Uint32.
    ///
    /// \param parameter_index  The index of the argument in question.
    /// \param size             The new length of the array of the specified argument.
    /// \return
    ///                         -  0: Success.
    ///                         - -1: #is_valid() returns \c false.
    ///                         - -2: \p parameter_index is out of range.
    ///                         - -4: The material instance or function call is immutable.
    ///                         - -5: The argument is not a dynamic array.
    Sint32 set_array_size( Uint32 parameter_index, Size size);

    /// Sets the length of a dynamic array argument.
    ///
    /// If the current argument is a call expression, it will be replaced by a constant expression.
    ///
    /// \param parameter_name   The name of the argument in question.
    /// \param size             The new length of the array of the specified argument.
    /// \return
    ///                         -  0: Success.
    ///                         - -1: #is_valid() returns \c false.
    ///                         - -2: \p parameter_name is invalid.
    ///                         - -4: The material instance or function call is immutable.
    ///                         - -5: The argument is not a dynamic array.
    Sint32 set_array_size( const char* parameter_name, Size size);

    //@}
    /// \name Methods related to call expressions for arguments
    //@{

    /// Returns an argument (call expressions only).
    ///
    /// If a literal \c 0 is passed for \p parameter_index, the call is ambiguous. You need to
    /// explicitly cast the value to #mi::Size.
    ///
    /// \param parameter_index  The index of the argument in question.
    /// \return                 The name of the call, or \c NULL if \p parameter_index is out of
    ///                         bounds or the argument expression is not a call.
    const char* get_call( Size parameter_index) const;

    /// Returns an argument (call expressions only).
    ///
    /// \param parameter_name   The name of the argument in question.
    /// \return                 The name of the call, or \c NULL if \p parameter_name is invalid
    ///                         or the argument expression is not a call.
    const char* get_call( const char* parameter_name) const;

    /// Sets an argument (call expressions only).
    ///
    /// If a literal \c 0 is passed for \p parameter_index, the call is ambiguous. You need to
    /// explicitly cast the value to #mi::Size.
    ///
    /// \param parameter_index  The index of the argument in question.
    /// \param call_name        The name of the call to set.
    /// \return
    ///                         -  0: Success.
    ///                         - -1: #is_valid() returns \c false.
    ///                         - -2: \p parameter_index is out of range.
    ///                         - -3: The return type of \p call_name does not match the argument
    ///                               type.
    ///                         - -4: The material instance or function call is immutable.
    ///                         - -5: The parameter is uniform, but the return type of the
    ///                               call is varying.
    ///                         - -6: \p call_name is invalid.
    Sint32 set_call( Size parameter_index, const char* call_name);

    /// Sets an argument (call expressions only).
    ///
    /// \param parameter_name   The name of the argument in question.
    /// \param call_name        The name of the call to set.
    /// \return
    ///                         -  0: Success.
    ///                         - -1: #is_valid() returns \c false.
    ///                         - -2: \p parameter_name is invalid.
    ///                         - -3: The return type of \p call_name does not match the argument
    ///                               type.
    ///                         - -4: The material instance or function call is immutable.
    ///                         - -5: The parameter is uniform, but the return type of the
    ///                               call is varying.
    ///                         - -6: \p call_name is invalid.
    Sint32 set_call( const char* parameter_name, const char* call_name);

    //@}
    /// \name Methods related to member access.
    //@{

    /// Get the transaction.
    ITransaction* get_transaction() const;

    /// Get the MDL factory.
    IMdl_factory* get_mdl_factory() const;

    /// Get the value factory.
    IValue_factory* get_value_factory() const;

    /// Get the expression factory.
    IExpression_factory* get_expression_factory() const;

    /// Get the MDL function call or material instance.
    const IScene_element* get_scene_element() const;

    /// Get the MDL function call or material instance.
    IScene_element* get_scene_element();

    /// Get the element type.
    Element_type get_element_type() const;

    /// Get the DB name of the MDL function call or material instance.
    const std::string& get_name() const;

    //@}

private:
    void promote_to_edit_if_needed();

    base::Handle<ITransaction> m_transaction;
    base::Handle<IMdl_factory> m_mdl_factory;
    base::Handle<IValue_factory> m_value_factory;
    base::Handle<IExpression_factory> m_expression_factory;
    base::Handle<const IScene_element> m_access;
    base::Handle<const IScene_element> m_old_access;
    base::Handle<IScene_element> m_edit;
    Element_type m_type;
    std::string m_name;
};

/**@}*/ // end group mi_neuray_mdl_elements

inline Argument_editor::Argument_editor(
    ITransaction* transaction, const char* name, IMdl_factory* mdl_factory, bool intent_to_edit)
{
    mi_neuray_assert( transaction);
    mi_neuray_assert( name);

    m_transaction = make_handle_dup( transaction);
    m_name = name;
    m_mdl_factory = make_handle_dup( mdl_factory);
    m_value_factory
        = m_mdl_factory ? m_mdl_factory->create_value_factory( m_transaction.get()) : 0;
    m_expression_factory
        = m_mdl_factory ? m_mdl_factory->create_expression_factory( m_transaction.get()) : 0;

    if( intent_to_edit) {
        m_edit = transaction->edit<IScene_element>( name);
        m_access = m_edit;
        m_old_access = m_edit;
        m_type = m_access ? m_access->get_element_type() : static_cast<Element_type>( 0);
    } else {
        m_access = transaction->access<IScene_element>( name);
        m_type = m_access ? m_access->get_element_type() : static_cast<Element_type>( 0);
    }
}

inline bool Argument_editor::is_valid() const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    return m_access
        && (m_type == ELEMENT_TYPE_MATERIAL_INSTANCE ||  m_type == ELEMENT_TYPE_FUNCTION_CALL);
#else // MI_NEURAYLIB_DEPRECATED_13_0
    return m_access
        && (m_type == ELEMENT_TYPE_FUNCTION_CALL);
#endif // MI_NEURAYLIB_DEPRECATED_13_0
}

inline bool Argument_editor::is_valid_instance( IMdl_execution_context* context) const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<const IMaterial_instance> mi( m_access->get_interface<IMaterial_instance>());
        return mi->is_valid(context);

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<const IFunction_call> fc( m_access->get_interface<IFunction_call>());
        return fc->is_valid(context);
    }
    else
        return false;
}

inline mi::Sint32 Argument_editor::repair( mi::Uint32 flags, IMdl_execution_context* context)
{
    promote_to_edit_if_needed();

#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<IMaterial_instance> mi( m_edit->get_interface<IMaterial_instance>());
        return mi->repair(flags, context);

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<IFunction_call> fc( m_edit->get_interface<IFunction_call>());
        return fc->repair( flags, context);

    } else
        return -1;
}

inline Element_type Argument_editor::get_type() const
{
    return m_type;
}

inline const char* Argument_editor::get_definition() const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<const IMaterial_instance> mi( m_access->get_interface<IMaterial_instance>());
        return mi->get_material_definition();

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<const IFunction_call> fc( m_access->get_interface<IFunction_call>());
        return fc->get_function_definition();

    } else
        return 0;
}

inline const char* Argument_editor::get_mdl_definition() const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<const IMaterial_instance> mi( m_access->get_interface<IMaterial_instance>());
        return mi->get_mdl_material_definition();

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<const IFunction_call> fc( m_access->get_interface<IFunction_call>());
        return fc->get_mdl_function_definition();

    } else
        return 0;
}

inline bool Argument_editor::is_array_constructor() const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        return false;

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<const IFunction_call> fc( m_access->get_interface<IFunction_call>());
        return fc->is_array_constructor();

    } else
        return false;
}

inline bool Argument_editor::is_material() const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        return true;

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<const IFunction_call> fc(
            m_access->get_interface<IFunction_call>());
        return fc->is_material();

    } else
        return false;
}

inline Size Argument_editor::get_parameter_count() const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<const IMaterial_instance> mi( m_access->get_interface<IMaterial_instance>());
        return mi->get_parameter_count();

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<const IFunction_call> fc( m_access->get_interface<IFunction_call>());
        return fc->get_parameter_count();

    } else
        return 0;
}

inline const char* Argument_editor::get_parameter_name( Size parameter_index) const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<const IMaterial_instance> mi( m_access->get_interface<IMaterial_instance>());
        return mi->get_parameter_name( parameter_index);

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<const IFunction_call> fc( m_access->get_interface<IFunction_call>());
        return fc->get_parameter_name( parameter_index);

    } else
        return 0;
}

inline Size Argument_editor::get_parameter_index( const char* name) const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<const IMaterial_instance> mi( m_access->get_interface<IMaterial_instance>());
        return mi->get_parameter_index( name);

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<const IFunction_call> fc( m_access->get_interface<IFunction_call>());
        return fc->get_parameter_index( name);

    } else
        return 0;
}

inline const IType* Argument_editor::get_return_type() const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        return 0;

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<const IFunction_call> fc( m_access->get_interface<IFunction_call>());
        return fc->get_return_type();

    } else
        return 0;
}

inline const IType_list* Argument_editor::get_parameter_types() const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<const IMaterial_instance> mi( m_access->get_interface<IMaterial_instance>());
        return mi->get_parameter_types();

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<const IFunction_call> fc( m_access->get_interface<IFunction_call>());
        return fc->get_parameter_types();

    } else
        return 0;
}

inline bool Argument_editor::is_parameter_enabled( Size index, IMdl_evaluator_api* evaluator) const
{
    if( !evaluator)
        return true;

#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<const IMaterial_instance> mi( m_access->get_interface<IMaterial_instance>());
        base::Handle<const IValue_bool> b(evaluator->is_material_parameter_enabled(
            m_transaction.get(), m_value_factory.get(), mi.get(), index, /*error*/ NULL));
        if (!b)
            return true;
        return b->get_value();

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<const IFunction_call> fc( m_access->get_interface<IFunction_call>());
        base::Handle<const IValue_bool> b( evaluator->is_function_parameter_enabled(
            m_transaction.get(), m_value_factory.get(), fc.get(), index, /*error*/ NULL));
        if( !b)
            return true;
        return b->get_value();

    } else
        return true;
}

inline Sint32 Argument_editor::reset_argument( Size index)
{
    promote_to_edit_if_needed();

#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<IMaterial_instance> mi( m_edit->get_interface<IMaterial_instance>());
        return mi->reset_argument( index);

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<IFunction_call> fc( m_edit->get_interface<IFunction_call>());
        return fc->reset_argument( index);

    } else
        return -1;
}

inline Sint32 Argument_editor::reset_argument( const char* name)
{
    promote_to_edit_if_needed();

#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<IMaterial_instance> mi( m_edit->get_interface<IMaterial_instance>());
        return mi->reset_argument( name);

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<IFunction_call> fc( m_edit->get_interface<IFunction_call>());
        return fc->reset_argument( name);

    } else
        return -1;
}


inline const IExpression_list* Argument_editor::get_arguments() const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<const IMaterial_instance> mi( m_access->get_interface<IMaterial_instance>());
        return mi->get_arguments();

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<const IFunction_call> fc( m_access->get_interface<IFunction_call>());
        return fc->get_arguments();

    } else
        return 0;
}

inline IExpression::Kind Argument_editor::get_argument_kind( Size parameter_index) const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<const IMaterial_instance> mi( m_access->get_interface<IMaterial_instance>());
        base::Handle<const IExpression_list> arguments( mi->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_index));
        return argument ? argument->get_kind() : static_cast<IExpression::Kind>( 0);

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<const IFunction_call> fc( m_access->get_interface<IFunction_call>());
        base::Handle<const IExpression_list> arguments( fc->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_index));
        return argument ? argument->get_kind() : static_cast<IExpression::Kind>( 0);

    } else
        return static_cast<IExpression::Kind>( 0);
}

inline IExpression::Kind Argument_editor::get_argument_kind( const char* parameter_name) const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<const IMaterial_instance> mi( m_access->get_interface<IMaterial_instance>());
        base::Handle<const IExpression_list> arguments( mi->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_name));
        return argument ? argument->get_kind() : static_cast<IExpression::Kind>( 0);

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<const IFunction_call> fc( m_access->get_interface<IFunction_call>());
        base::Handle<const IExpression_list> arguments( fc->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_name));
        return argument ? argument->get_kind() : static_cast<IExpression::Kind>( 0);

    } else
        return static_cast<IExpression::Kind>( 0);
}

template <class T>
Sint32 Argument_editor::get_value( Size parameter_index, T& value) const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<const IMaterial_instance> mi( m_access->get_interface<IMaterial_instance>());
        base::Handle<const IExpression_list> arguments( mi->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_index));
        if( !argument)
            return -2;
        base::Handle<const IExpression_constant> argument_constant(
            argument->get_interface<IExpression_constant>());
        if( !argument_constant)
            return -4;
        base::Handle<const IValue> argument_value( argument_constant->get_value());
        Sint32 result = neuraylib::get_value( argument_value.get(), value);
        return result == 0 ? 0 : -5;

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<const IFunction_call> fc( m_access->get_interface<IFunction_call>());
        base::Handle<const IExpression_list> arguments( fc->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_index));
        if( !argument)
            return -2;
        base::Handle<const IExpression_constant> argument_constant(
            argument->get_interface<IExpression_constant>());
        if( !argument_constant)
            return -4;
        base::Handle<const IValue> argument_value( argument_constant->get_value());
        Sint32 result = neuraylib::get_value( argument_value.get(), value);
        return result == 0 ? 0 : -5;

    } else
        return -1;
}

template <class T>
Sint32 Argument_editor::get_value( const char* name, T& value) const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<const IMaterial_instance> mi( m_access->get_interface<IMaterial_instance>());
        base::Handle<const IExpression_list> arguments( mi->get_arguments());
        base::Handle<const IExpression> argument_( arguments->get_expression( name));
        if( !argument_)
            return -2;
        base::Handle<const IExpression_constant> argument_constant(
            argument_->get_interface<IExpression_constant>());
        if( !argument_constant)
            return -4;
        base::Handle<const IValue> argument_value( argument_constant->get_value());
        Sint32 result = neuraylib::get_value( argument_value.get(), value);
        return result == 0 ? 0 : -5;

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<const IFunction_call> fc(
            m_access->get_interface<IFunction_call>());
        base::Handle<const IExpression_list> arguments( fc->get_arguments());
        base::Handle<const IExpression> argument_( arguments->get_expression( name));
        if( !argument_)
            return -2;
        base::Handle<const IExpression_constant> argument_constant(
            argument_->get_interface<IExpression_constant>());
        if( !argument_constant)
            return -4;
        base::Handle<const IValue> argument_value( argument_constant->get_value());
        Sint32 result = neuraylib::get_value( argument_value.get(), value);
        return result == 0 ? 0 : -5;

    } else
        return -1;
}

template <class T>
Sint32 Argument_editor::get_value( Size parameter_index, Size component_index, T& value) const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<const IMaterial_instance> mi( m_access->get_interface<IMaterial_instance>());
        base::Handle<const IExpression_list> arguments( mi->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_index));
        if( !argument)
            return -2;
        base::Handle<const IExpression_constant> argument_constant(
            argument->get_interface<IExpression_constant>());
        if( !argument_constant)
            return -4;
        base::Handle<const IValue> argument_value( argument_constant->get_value());
        Sint32 result = neuraylib::get_value( argument_value.get(), component_index, value);
        return result == 0 ? 0 : (result == -3 ? -3 : -5);

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<const IFunction_call> fc( m_access->get_interface<IFunction_call>());
        base::Handle<const IExpression_list> arguments( fc->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_index));
        if( !argument)
            return -2;
        base::Handle<const IExpression_constant> argument_constant(
            argument->get_interface<IExpression_constant>());
        if( !argument_constant)
            return -4;
        base::Handle<const IValue> argument_value( argument_constant->get_value());
        Sint32 result = neuraylib::get_value( argument_value.get(), component_index, value);
        return result == 0 ? 0 : (result == -3 ? -3 : -5);

    } else
        return -1;
}

template <class T>
Sint32 Argument_editor::get_value( const char* parameter_name, Size component_index, T& value) const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<const IMaterial_instance> mi( m_access->get_interface<IMaterial_instance>());
        base::Handle<const IExpression_list> arguments( mi->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_name));
        if( !argument)
            return -2;
        base::Handle<const IExpression_constant> argument_constant(
            argument->get_interface<IExpression_constant>());
        if( !argument_constant)
            return -4;
        base::Handle<const IValue> argument_value( argument_constant->get_value());
        Sint32 result = neuraylib::get_value( argument_value.get(), component_index, value);
        return result == 0 ? 0 : (result == -3 ? -3 : -5);

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<const IFunction_call> fc( m_access->get_interface<IFunction_call>());
        base::Handle<const IExpression_list> arguments( fc->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_name));
        if( !argument)
            return -2;
        base::Handle<const IExpression_constant> argument_constant(
            argument->get_interface<IExpression_constant>());
        if( !argument_constant)
            return -4;
        base::Handle<const IValue> argument_value( argument_constant->get_value());
        Sint32 result = neuraylib::get_value( argument_value.get(), component_index, value);
        return result == 0 ? 0 : (result == -3 ? -3 : -5);

    } else
        return -1;
}

template <class T>
Sint32 Argument_editor::get_value(
    Size parameter_index, const char* field_name, T& value) const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<const IMaterial_instance> mi( m_access->get_interface<IMaterial_instance>());
        base::Handle<const IExpression_list> arguments( mi->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_index));
        if( !argument)
            return -2;
        base::Handle<const IExpression_constant> argument_constant(
            argument->get_interface<IExpression_constant>());
        if( !argument_constant)
            return -4;
        base::Handle<const IValue> argument_value( argument_constant->get_value());
        Sint32 result = neuraylib::get_value( argument_value.get(), field_name, value);
        return result == 0 ? 0 : (result == -3 ? -3 : -5);

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<const IFunction_call> fc( m_access->get_interface<IFunction_call>());
        base::Handle<const IExpression_list> arguments( fc->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_index));
        if( !argument)
            return -2;
        base::Handle<const IExpression_constant> argument_constant(
            argument->get_interface<IExpression_constant>());
        if( !argument_constant)
            return -4;
        base::Handle<const IValue> argument_value( argument_constant->get_value());
        Sint32 result = neuraylib::get_value( argument_value.get(), field_name, value);
        return result == 0 ? 0 : (result == -3 ? -3 : -5);

    } else
        return -1;
}

template <class T>
Sint32 Argument_editor::get_value(
    const char* parameter_name, const char* field_name, T& value) const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<const IMaterial_instance> mi( m_access->get_interface<IMaterial_instance>());
        base::Handle<const IExpression_list> arguments( mi->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_name));
        if( !argument)
            return -2;
        base::Handle<const IExpression_constant> argument_constant(
            argument->get_interface<IExpression_constant>());
        if( !argument_constant)
            return -4;
        base::Handle<const IValue> argument_value( argument_constant->get_value());
        Sint32 result = neuraylib::get_value( argument_value.get(), field_name, value);
        return result == 0 ? 0 : (result == -3 ? -3 : -5);

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<const IFunction_call> fc( m_access->get_interface<IFunction_call>());
        base::Handle<const IExpression_list> arguments( fc->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_name));
        if( !argument)
            return -2;
        base::Handle<const IExpression_constant> argument_constant(
            argument->get_interface<IExpression_constant>());
        if( !argument_constant)
            return -4;
        base::Handle<const IValue> argument_value( argument_constant->get_value());
        Sint32 result = neuraylib::get_value( argument_value.get(), field_name, value);
        return result == 0 ? 0 : (result == -3 ? -3 : -5);

    } else
        return -1;
}

template <class T>
Sint32 Argument_editor::set_value( Size parameter_index, const T& value)
{
    promote_to_edit_if_needed();

#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<IMaterial_instance> mi( m_edit->get_interface<IMaterial_instance>());
        if( mi->is_default())
            return -4;
        base::Handle<const IExpression_list> arguments( mi->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_index));
        if( !argument)
            return -2;
        base::Handle<const IType> type( argument->get_type());
        base::Handle<IValue> new_value( m_value_factory->create( type.get()));
        Sint32 result = neuraylib::set_value( new_value.get(), value);
        if( result != 0)
            return -5;
        base::Handle<IExpression> new_expression(
            m_expression_factory->create_constant( new_value.get()));
        result = mi->set_argument( parameter_index, new_expression.get());
        mi_neuray_assert( result == 0);
        return result;

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<IFunction_call> fc( m_edit->get_interface<IFunction_call>());
        if( fc->is_default())
            return -4;
        base::Handle<const IExpression_list> arguments( fc->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_index));
        if( !argument)
            return -2;
        base::Handle<const IType> type( argument->get_type());
        base::Handle<IValue> new_value( m_value_factory->create( type.get()));
        Sint32 result = neuraylib::set_value( new_value.get(), value);
        if( result != 0)
            return -5;
        base::Handle<IExpression> new_expression(
            m_expression_factory->create_constant( new_value.get()));
        result = fc->set_argument( parameter_index, new_expression.get());
        mi_neuray_assert( result == 0);
        return result;

    } else
        return -1;
}

template <class T>
Sint32 Argument_editor::set_value( const char* parameter_name, const T& value)
{
    promote_to_edit_if_needed();

#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<IMaterial_instance> mi( m_edit->get_interface<IMaterial_instance>());
        if( mi->is_default())
            return -4;
        base::Handle<const IExpression_list> arguments( mi->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_name));
        if( !argument)
            return -2;
        base::Handle<const IType> type( argument->get_type());
        base::Handle<IValue> new_value( m_value_factory->create( type.get()));
        Sint32 result = neuraylib::set_value( new_value.get(), value);
        if( result != 0)
            return -5;
        base::Handle<IExpression> new_expression(
            m_expression_factory->create_constant( new_value.get()));
        result = mi->set_argument( parameter_name, new_expression.get());
        mi_neuray_assert( result == 0);
        return result;

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<IFunction_call> fc( m_edit->get_interface<IFunction_call>());
        if( fc->is_default())
            return -4;
        base::Handle<const IExpression_list> arguments( fc->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_name));
        if( !argument)
            return -2;
        base::Handle<const IType> type( argument->get_type());
        base::Handle<IValue> new_value( m_value_factory->create( type.get()));
        Sint32 result = neuraylib::set_value( new_value.get(), value);
        if( result != 0)
            return -5;
        base::Handle<IExpression> new_expression(
            m_expression_factory->create_constant( new_value.get()));
        result = fc->set_argument( parameter_name, new_expression.get());
        mi_neuray_assert( result == 0);
        return result;

    } else
        return -1;
}

template <class T>
Sint32 Argument_editor::set_value( Size parameter_index, Size component_index, const T& value)
{
    promote_to_edit_if_needed();

#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<IMaterial_instance> mi( m_edit->get_interface<IMaterial_instance>());
        if( mi->is_default())
            return -4;
        base::Handle<const IExpression_list> arguments( mi->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_index));
        if( !argument)
            return -2;
        if( argument->get_kind() == IExpression::EK_CONSTANT) {
            // reuse existing constant expression
            base::Handle<IExpression> new_argument( m_expression_factory->clone( argument.get()));
            base::Handle<IExpression_constant> new_argument_constant(
                new_argument->get_interface<IExpression_constant>());
            base::Handle<IValue> new_value( new_argument_constant->get_value());
            Sint32 result = neuraylib::set_value( new_value.get(), component_index, value);
            if( result != 0)
                return result == -3 ? -3 : -5;
            result = mi->set_argument( parameter_index, new_argument.get());
            mi_neuray_assert( result == 0);
            return result;
        } else {
            // create new constant expression
            base::Handle<const IType> type( argument->get_type());
            base::Handle<IValue> new_value( m_value_factory->create( type.get()));
            Sint32 result = neuraylib::set_value( new_value.get(), component_index, value);
            if( result != 0)
                return result == -3 ? -3 : -5;
            base::Handle<IExpression> new_expression(
                m_expression_factory->create_constant( new_value.get()));
            result = mi->set_argument( parameter_index, new_expression.get());
            mi_neuray_assert( result == 0);
            return result;
        }

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<IFunction_call> fc( m_edit->get_interface<IFunction_call>());
        if( fc->is_default())
            return -4;
        base::Handle<const IExpression_list> arguments( fc->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_index));
        if( !argument)
            return -2;
        if( argument->get_kind() == IExpression::EK_CONSTANT) {
            // reuse existing constant expression
            base::Handle<IExpression> new_argument( m_expression_factory->clone( argument.get()));
            base::Handle<IExpression_constant> new_argument_constant(
                new_argument->get_interface<IExpression_constant>());
            base::Handle<IValue> new_value( new_argument_constant->get_value());
            Sint32 result = neuraylib::set_value( new_value.get(), component_index, value);
            if( result != 0)
                return result == -3 ? -3 : -5;
            result = fc->set_argument( parameter_index, new_argument.get());
            mi_neuray_assert( result == 0);
            return result;
        } else {
            // create new constant expression
            base::Handle<const IType> type( argument->get_type());
            base::Handle<IValue> new_value( m_value_factory->create( type.get()));
            Sint32 result = neuraylib::set_value( new_value.get(), component_index, value);
            if( result != 0)
                return result == -3 ? -3 : -5;
            base::Handle<IExpression> new_expression(
                m_expression_factory->create_constant( new_value.get()));
            result = fc->set_argument( parameter_index, new_expression.get());
            mi_neuray_assert( result == 0);
            return result;
        }

    } else
        return -1;
}

template <class T>
Sint32 Argument_editor::set_value( const char* parameter_name, Size component_index, const T& value)
{
    promote_to_edit_if_needed();

#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<IMaterial_instance> mi( m_edit->get_interface<IMaterial_instance>());
        if( mi->is_default())
            return -4;
        base::Handle<const IExpression_list> arguments( mi->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_name));
        if( !argument)
            return -2;
        if( argument->get_kind() == IExpression::EK_CONSTANT) {
            // reuse existing constant expression
            base::Handle<IExpression> new_argument( m_expression_factory->clone( argument.get()));
            base::Handle<IExpression_constant> new_argument_constant(
                new_argument->get_interface<IExpression_constant>());
            base::Handle<IValue> new_value( new_argument_constant->get_value());
            Sint32 result = neuraylib::set_value( new_value.get(), component_index, value);
            if( result != 0)
                return result == -3 ? -3 : -5;
            result = mi->set_argument( parameter_name, new_argument.get());
            mi_neuray_assert( result == 0);
            return result;
        } else {
            // create new constant expression
            base::Handle<const IType> type( argument->get_type());
            base::Handle<IValue> new_value( m_value_factory->create( type.get()));
            Sint32 result = neuraylib::set_value( new_value.get(), component_index, value);
            if( result != 0)
                return result == -3 ? -3 : -5;
            base::Handle<IExpression> new_expression(
                m_expression_factory->create_constant( new_value.get()));
            result = mi->set_argument( parameter_name, new_expression.get());
            mi_neuray_assert( result == 0);
            return result;
        }

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<IFunction_call> fc( m_edit->get_interface<IFunction_call>());
        if( fc->is_default())
            return -4;
        base::Handle<const IExpression_list> arguments( fc->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_name));
        if( !argument)
            return -2;
        if( argument->get_kind() == IExpression::EK_CONSTANT) {
            // reuse existing constant expression
            base::Handle<IExpression> new_argument( m_expression_factory->clone( argument.get()));
            base::Handle<IExpression_constant> new_argument_constant(
                new_argument->get_interface<IExpression_constant>());
            base::Handle<IValue> new_value( new_argument_constant->get_value());
            Sint32 result = neuraylib::set_value( new_value.get(), component_index, value);
            if( result != 0)
                return result == -3 ? -3 : -5;
            result = fc->set_argument( parameter_name, new_argument.get());
            mi_neuray_assert( result == 0);
            return result;
        } else {
            // create new constant expression
            base::Handle<const IType> type( argument->get_type());
            base::Handle<IValue> new_value( m_value_factory->create( type.get()));
            Sint32 result = neuraylib::set_value( new_value.get(), component_index, value);
            if( result != 0)
                return result == -3 ? -3 : -5;
            base::Handle<IExpression> new_expression(
                m_expression_factory->create_constant( new_value.get()));
            result = fc->set_argument( parameter_name, new_expression.get());
            mi_neuray_assert( result == 0);
            return result;
        }

    } else
        return -1;
}

template <class T>
Sint32 Argument_editor::set_value(
    Size parameter_index, const char* field_name, const T& value)
{
    promote_to_edit_if_needed();

#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<IMaterial_instance> mi( m_edit->get_interface<IMaterial_instance>());
        if( mi->is_default())
            return -4;
        base::Handle<const IExpression_list> arguments( mi->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_index));
        if( !argument)
            return -2;
        if( argument->get_kind() == IExpression::EK_CONSTANT) {
            // reuse existing constant expression
            base::Handle<IExpression> new_argument( m_expression_factory->clone( argument.get()));
            base::Handle<IExpression_constant> new_argument_constant(
                new_argument->get_interface<IExpression_constant>());
            base::Handle<IValue> new_value( new_argument_constant->get_value());
            Sint32 result = neuraylib::set_value( new_value.get(), field_name, value);
            if( result != 0)
                return result == -3 ? -3 : -5;
            result = mi->set_argument( parameter_index, new_argument.get());
            mi_neuray_assert( result == 0);
            return result;
        } else {
            // create new constant expression
            base::Handle<const IType> type( argument->get_type());
            base::Handle<IValue> new_value( m_value_factory->create( type.get()));
            Sint32 result = neuraylib::set_value( new_value.get(), field_name, value);
            if( result != 0)
                return result == -3 ? -3 : -5;
            base::Handle<IExpression> new_expression(
                m_expression_factory->create_constant( new_value.get()));
            result = mi->set_argument( parameter_index, new_expression.get());
            mi_neuray_assert( result == 0);
            return result;
        }

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<IFunction_call> fc( m_edit->get_interface<IFunction_call>());
        if( fc->is_default())
            return -4;
        base::Handle<const IExpression_list> arguments( fc->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_index));
        if( !argument)
            return -2;
        if( argument->get_kind() == IExpression::EK_CONSTANT) {
            // reuse existing constant expression
            base::Handle<IExpression> new_argument( m_expression_factory->clone( argument.get()));
            base::Handle<IExpression_constant> new_argument_constant(
                new_argument->get_interface<IExpression_constant>());
            base::Handle<IValue> new_value( new_argument_constant->get_value());
            Sint32 result = neuraylib::set_value( new_value.get(), field_name, value);
            if( result != 0)
                return result == -3 ? -3 : -5;
            result = fc->set_argument( parameter_index, new_argument.get());
            mi_neuray_assert( result == 0);
            return result;
        } else {
            // create new constant expression
            base::Handle<const IType> type( argument->get_type());
            base::Handle<IValue> new_value( m_value_factory->create( type.get()));
            Sint32 result = neuraylib::set_value( new_value.get(), field_name, value);
            if( result != 0)
                return result == -3 ? -3 : -5;
            base::Handle<IExpression> new_expression(
                m_expression_factory->create_constant( new_value.get()));
            result = fc->set_argument( parameter_index, new_expression.get());
            mi_neuray_assert( result == 0);
            return result;
        }

    } else
        return -1;
}

template <class T>
Sint32 Argument_editor::set_value(
    const char* parameter_name, const char* field_name, const T& value)
{
    promote_to_edit_if_needed();

#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<IMaterial_instance> mi( m_edit->get_interface<IMaterial_instance>());
        if( mi->is_default())
            return -4;
        base::Handle<const IExpression_list> arguments( mi->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_name));
        if( !argument)
            return -2;
        if( argument->get_kind() == IExpression::EK_CONSTANT) {
            // reuse existing constant expression
            base::Handle<IExpression> new_argument( m_expression_factory->clone( argument.get()));
            base::Handle<IExpression_constant> new_argument_constant(
                new_argument->get_interface<IExpression_constant>());
            base::Handle<IValue> new_value( new_argument_constant->get_value());
            Sint32 result = neuraylib::set_value( new_value.get(), field_name, value);
            if( result != 0)
                return result == -3 ? -3 : -5;
            result = mi->set_argument( parameter_name, new_argument.get());
            mi_neuray_assert( result == 0);
            return result;
        } else {
            // create new constant expression
            base::Handle<const IType> type( argument->get_type());
            base::Handle<IValue> new_value( m_value_factory->create( type.get()));
            Sint32 result = neuraylib::set_value( new_value.get(), field_name, value);
            if( result != 0)
                return result == -3 ? -3 : -5;
            base::Handle<IExpression> new_expression(
                m_expression_factory->create_constant( new_value.get()));
            result = mi->set_argument( parameter_name, new_expression.get());
            mi_neuray_assert( result == 0);
            return result;
        }

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<IFunction_call> fc( m_edit->get_interface<IFunction_call>());
        if( fc->is_default())
            return -4;
        base::Handle<const IExpression_list> arguments( fc->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_name));
        if( !argument)
            return -2;
        if( argument->get_kind() == IExpression::EK_CONSTANT) {
            // reuse existing constant expression
            base::Handle<IExpression> new_argument( m_expression_factory->clone( argument.get()));
            base::Handle<IExpression_constant> new_argument_constant(
                new_argument->get_interface<IExpression_constant>());
            base::Handle<IValue> new_value( new_argument_constant->get_value());
            Sint32 result = neuraylib::set_value( new_value.get(), field_name, value);
            if( result != 0)
                return result == -3 ? -3 : -5;
            result = fc->set_argument( parameter_name, new_argument.get());
            mi_neuray_assert( result == 0);
            return result;
        } else {
            // create new constant expression
            base::Handle<const IType> type( argument->get_type());
            base::Handle<IValue> new_value( m_value_factory->create( type.get()));
            Sint32 result = neuraylib::set_value( new_value.get(), field_name, value);
            if( result != 0)
                return result == -3 ? -3 : -5;
            base::Handle<IExpression> new_expression(
                m_expression_factory->create_constant( new_value.get()));
            result = fc->set_argument( parameter_name, new_expression.get());
            mi_neuray_assert( result == 0);
            return result;
        }

    } else
        return -1;
}

inline Sint32 Argument_editor::get_array_length( Uint32 parameter_index, Size& size) const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<const IMaterial_instance> mi( m_access->get_interface<IMaterial_instance>());
        base::Handle<const IExpression_list> arguments( mi->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_index));
        if( !argument)
            return -2;
        base::Handle<const IExpression_constant> argument_constant(
            argument->get_interface<IExpression_constant>());
        if( !argument_constant)
            return -4;
        base::Handle<const IValue_array> value( argument_constant->get_value<IValue_array>());
        if( !value)
            return -5;
        size = value->get_size();
        return 0;

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<const IFunction_call> fc( m_access->get_interface<IFunction_call>());
        base::Handle<const IExpression_list> arguments( fc->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_index));
        if( !argument)
            return -2;
        base::Handle<const IExpression_constant> argument_constant(
            argument->get_interface<IExpression_constant>());
        if( !argument_constant)
            return -4;
        base::Handle<const IValue_array> value( argument_constant->get_value<IValue_array>());
        if( !value)
            return -5;
        size = value->get_size();
        return 0;

    } else
        return -1;
}

inline Sint32 Argument_editor::get_array_length( const char* parameter_name, Size& size) const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<const IMaterial_instance> mi( m_access->get_interface<IMaterial_instance>());
        base::Handle<const IExpression_list> arguments( mi->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_name));
        if( !argument)
            return -2;
        base::Handle<const IExpression_constant> argument_constant(
            argument->get_interface<IExpression_constant>());
        if( !argument_constant)
            return -4;
        base::Handle<const IValue_array> value( argument_constant->get_value<IValue_array>());
        if( !value)
            return -5;
        size = value->get_size();
        return 0;

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<const IFunction_call> fc( m_access->get_interface<IFunction_call>());
        base::Handle<const IExpression_list> arguments( fc->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_name));
        if( !argument)
            return -2;
        base::Handle<const IExpression_constant> argument_constant(
            argument->get_interface<IExpression_constant>());
        if( !argument_constant)
            return -4;
        base::Handle<const IValue_array> value( argument_constant->get_value<IValue_array>());
        if( !value)
            return -5;
        size = value->get_size();
        return 0;

    } else
        return -1;
}

inline Sint32 Argument_editor::set_array_size( Uint32 parameter_index, Size size)
{
    promote_to_edit_if_needed();

#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<IMaterial_instance> mi( m_edit->get_interface<IMaterial_instance>());
        if( mi->is_default())
            return -4;
        base::Handle<const IExpression_list> arguments( mi->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_index));
        if( !argument)
            return -2;
        if( argument->get_kind() == IExpression::EK_CONSTANT) {
            // reuse existing constant expression
            base::Handle<IExpression> new_argument( m_expression_factory->clone( argument.get()));
            base::Handle<IExpression_constant> new_argument_constant(
                new_argument->get_interface<IExpression_constant>());
            base::Handle<IValue_array> new_value(
                new_argument_constant->get_value<IValue_array>());
            if( !new_value)
                return -5;
            Sint32 result = new_value->set_size( size);
            if( result != 0)
                return -5;
            result = mi->set_argument( parameter_index, new_argument.get());
            mi_neuray_assert( result == 0);
            return result;
        } else {
            // create new constant expression
            base::Handle<const IType> type( argument->get_type());
            base::Handle<IValue_array> new_value(
                m_value_factory->create<IValue_array>( type.get()));
            if( !new_value)
                return -5;
            Sint32 result = new_value->set_size( size);
            if( result != 0)
                return -5;
            base::Handle<IExpression> new_expression(
                m_expression_factory->create_constant( new_value.get()));
            result = mi->set_argument( parameter_index, new_expression.get());
            mi_neuray_assert( result == 0);
            return result;
        }

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<IFunction_call> fc( m_edit->get_interface<IFunction_call>());
        if( fc->is_default())
            return -4;
        base::Handle<const IExpression_list> arguments( fc->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_index));
        if( !argument)
            return -2;
        if( argument->get_kind() == IExpression::EK_CONSTANT) {
            // reuse existing constant expression
            base::Handle<IExpression> new_argument( m_expression_factory->clone( argument.get()));
            base::Handle<IExpression_constant> new_argument_constant(
                new_argument->get_interface<IExpression_constant>());
            base::Handle<IValue_array> new_value(
                new_argument_constant->get_value<IValue_array>());
            if( !new_value)
                return -5;
            Sint32 result = new_value->set_size( size);
            if( result != 0)
                return -5;
            result = fc->set_argument( parameter_index, new_argument.get());
            mi_neuray_assert( result == 0);
            return result;
        } else {
            // create new constant expression
            base::Handle<const IType> type( argument->get_type());
            base::Handle<IValue_array> new_value(
                m_value_factory->create<IValue_array>( type.get()));
            if( !new_value)
                return -5;
            Sint32 result = new_value->set_size( size);
            if( result != 0)
                return -5;
            base::Handle<IExpression> new_expression(
                m_expression_factory->create_constant( new_value.get()));
            result = fc->set_argument( parameter_index, new_expression.get());
            mi_neuray_assert( result == 0);
            return result;
        }

    } else
        return -1;
}

inline Sint32 Argument_editor::set_array_size( const char* parameter_name, Size size)
{
    promote_to_edit_if_needed();

#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<IMaterial_instance> mi( m_edit->get_interface<IMaterial_instance>());
        if( mi->is_default())
            return -4;
        base::Handle<const IExpression_list> arguments( mi->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_name));
        if( !argument)
            return -2;
        if( argument->get_kind() == IExpression::EK_CONSTANT) {
            // reuse existing constant expression
            base::Handle<IExpression> new_argument( m_expression_factory->clone( argument.get()));
            base::Handle<IExpression_constant> new_argument_constant(
                new_argument->get_interface<IExpression_constant>());
            base::Handle<IValue_array> new_value(
                new_argument_constant->get_value<IValue_array>());
            if( !new_value)
                return -5;
            Sint32 result = new_value->set_size( size);
            if( result != 0)
                return -5;
            result = mi->set_argument( parameter_name, new_argument.get());
            mi_neuray_assert( result == 0);
            return result;
        } else {
            // create new constant expression
            base::Handle<const IType> type( argument->get_type());
            base::Handle<IValue_array> new_value(
                m_value_factory->create<IValue_array>( type.get()));
            if( !new_value)
                return -5;
            Sint32 result = new_value->set_size( size);
            if( result != 0)
                return -5;
            base::Handle<IExpression> new_expression(
                m_expression_factory->create_constant( new_value.get()));
            result = mi->set_argument( parameter_name, new_expression.get());
            mi_neuray_assert( result == 0);
            return result;
        }

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<IFunction_call> fc( m_edit->get_interface<IFunction_call>());
        if( fc->is_default())
            return -4;
        base::Handle<const IExpression_list> arguments( fc->get_arguments());
        base::Handle<const IExpression> argument( arguments->get_expression( parameter_name));
        if( !argument)
            return -2;
        if( argument->get_kind() == IExpression::EK_CONSTANT) {
            // reuse existing constant expression
            base::Handle<IExpression> new_argument( m_expression_factory->clone( argument.get()));
            base::Handle<IExpression_constant> new_argument_constant(
                new_argument->get_interface<IExpression_constant>());
            base::Handle<IValue_array> new_value(
                new_argument_constant->get_value<IValue_array>());
            if( !new_value)
                return -5;
            Sint32 result = new_value->set_size( size);
            if( result != 0)
                return -5;
            result = fc->set_argument( parameter_name, new_argument.get());
            mi_neuray_assert( result == 0);
            return result;
        } else {
            // create new constant expression
            base::Handle<const IType> type( argument->get_type());
            base::Handle<IValue_array> new_value(
                m_value_factory->create<IValue_array>( type.get()));
            if( !new_value)
                return -5;
            Sint32 result = new_value->set_size( size);
            if( result != 0)
                return -5;
            base::Handle<IExpression> new_expression(
                m_expression_factory->create_constant( new_value.get()));
            result = fc->set_argument( parameter_name, new_expression.get());
            mi_neuray_assert( result == 0);
            return result;
        }

    } else
        return -1;
}

inline const char* Argument_editor::get_call( Size parameter_index) const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<const IMaterial_instance> mi( m_access->get_interface<IMaterial_instance>());
        base::Handle<const IExpression_list> arguments( mi->get_arguments());
        base::Handle<const IExpression_call> argument(
            arguments->get_expression<IExpression_call>( parameter_index));
        if( !argument)
            return 0;
        return argument->get_call();

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<const IFunction_call> fc( m_access->get_interface<IFunction_call>());
        base::Handle<const IExpression_list> arguments( fc->get_arguments());
        base::Handle<const IExpression_call> argument(
            arguments->get_expression<IExpression_call>( parameter_index));
        if( !argument)
            return 0;
        return argument->get_call();

    } else
        return 0;
}

inline const char* Argument_editor::get_call( const char* parameter_name) const
{
#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<const IMaterial_instance> mi( m_access->get_interface<IMaterial_instance>());
        base::Handle<const IExpression_list> arguments( mi->get_arguments());
        base::Handle<const IExpression_call> argument(
            arguments->get_expression<IExpression_call>( parameter_name));
        if( !argument)
            return 0;
        return argument->get_call();

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<const IFunction_call> fc( m_access->get_interface<IFunction_call>());
        base::Handle<const IExpression_list> arguments( fc->get_arguments());
        base::Handle<const IExpression_call> argument(
            arguments->get_expression<IExpression_call>( parameter_name));
        if( !argument)
            return 0;
        return argument->get_call();

    } else
        return 0;
}

inline Sint32 Argument_editor::set_call( Size parameter_index, const char* call_name)
{
    promote_to_edit_if_needed();

#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<IExpression_call> new_argument( m_expression_factory->create_call( call_name));
        if( !new_argument)
            return -6;
        base::Handle<IMaterial_instance> mi( m_edit->get_interface<IMaterial_instance>());
        return mi->set_argument( parameter_index, new_argument.get());

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<IExpression_call> new_argument( m_expression_factory->create_call( call_name));
        if( !new_argument)
            return -6;
        base::Handle<IFunction_call> fc( m_edit->get_interface<IFunction_call>());
        return fc->set_argument( parameter_index, new_argument.get());

    } else
        return -1;
}

inline Sint32 Argument_editor::set_call( const char* parameter_name, const char* call_name)
{
    promote_to_edit_if_needed();

#ifdef MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_MATERIAL_INSTANCE) {

        base::Handle<IExpression_call> new_argument( m_expression_factory->create_call( call_name));
        if( !new_argument)
            return -6;
        base::Handle<IMaterial_instance> mi( m_edit->get_interface<IMaterial_instance>());
        return mi->set_argument( parameter_name, new_argument.get());

    } else
#endif // MI_NEURAYLIB_DEPRECATED_13_0
    if( m_type == ELEMENT_TYPE_FUNCTION_CALL) {

        base::Handle<IExpression_call> new_argument( m_expression_factory->create_call( call_name));
        if( !new_argument)
            return -6;
        base::Handle<IFunction_call> fc( m_edit->get_interface<IFunction_call>());
        return fc->set_argument( parameter_name, new_argument.get());

    } else
        return -1;
}

inline ITransaction* Argument_editor::get_transaction() const
{
    m_transaction->retain();
    return m_transaction.get();
}

inline IMdl_factory* Argument_editor::get_mdl_factory() const
{
    m_mdl_factory->retain();
    return m_mdl_factory.get();
}

inline IValue_factory* Argument_editor::get_value_factory() const
{
    m_value_factory->retain();
    return m_value_factory.get();
}

inline IExpression_factory* Argument_editor::get_expression_factory() const
{
    m_expression_factory->retain();
    return m_expression_factory.get();
}

inline const IScene_element* Argument_editor::get_scene_element() const
{
    m_access->retain();
    return m_access.get();
}

inline IScene_element* Argument_editor::get_scene_element() //-V659 PVS
{
    promote_to_edit_if_needed();

    m_edit->retain();
    return m_edit.get();
}

inline Element_type Argument_editor::get_element_type() const
{
    return m_type;
}

inline const std::string& Argument_editor::get_name() const
{
    return m_name;
}

inline void Argument_editor::promote_to_edit_if_needed()
{
    if( m_edit)
        return;
    m_edit = m_transaction->edit<IScene_element>( m_name.c_str());
    mi_neuray_assert( m_edit);
    m_old_access = m_access;
    m_access = m_edit;
}

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_ARGUMENT_EDITOR_H

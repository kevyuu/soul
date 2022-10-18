/***************************************************************************************************
 * Copyright 2022 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief      The MDL execution context and the IMessage class.

#ifndef MI_NEURAYLIB_IMDL_EXECUTION_CONTEXT_H
#define MI_NEURAYLIB_IMDL_EXECUTION_CONTEXT_H

#include <mi/base/interface_declare.h>
#include <mi/base/enums.h>
#include <mi/base/handle.h>

namespace mi {

namespace neuraylib {


/** \addtogroup mi_neuray_mdl_misc
@{
*/

/// Message interface.
class IMessage: public
    base::Interface_declare<0x51965a01,0xcd3f,0x41fc,0xb1,0x8b,0x8,0x1c,0x7b,0x4b,0xba,0xb2>
{
public:

    /// The possible kinds of messages.
    /// A message can be uniquely identified by the message code and kind,
    /// except for uncategorized messages.
    enum Kind {

        /// MDL Core compiler message.
        MSG_COMILER_CORE,
        /// MDL Core compiler backend message.
        MSG_COMILER_BACKEND,
        /// MDL Core DAG generator message.
        MSG_COMPILER_DAG,
        /// MDL Core archive tool message.
        MSG_COMPILER_ARCHIVE_TOOL,
        /// MDL import/exporter message.
        MSG_IMP_EXP,
        /// MDL integration message.
        MSG_INTEGRATION,
        /// Uncategorized messages do not have a code.
        MSG_UNCATEGORIZED,
        //  Undocumented, for alignment only.
        MSG_FORCE_32_BIT = 0xffffffffU
    };

    /// Returns the kind of message.
    virtual Kind get_kind() const = 0;

    /// Returns the severity of the message.
    virtual base::Message_severity get_severity() const = 0;

    /// Returns the message string.
    virtual const char* get_string() const = 0;

    /// Returns a unique identifier for the message.
    virtual Sint32 get_code() const = 0;

    /// Returns the number of notes associated with the message
    ///
    /// Notes can be used to describe an error message further or add additional details.
    virtual Size get_notes_count() const = 0;

    /// Returns the note at index or \c NULL, if no such index exists.
    virtual const IMessage* get_note( Size index) const = 0;
};

/// The execution context can be used to query status information like error
/// and warning messages concerning the operation it was passed into.
///
/// The context supports the following options:
///
/// Options for module loading
/// - \c std::string "warning": Silence compiler warnings or promote them to errors.
///   Format: options = (option ',')* option. option = 'err' | number '=' ('on' | 'off' | 'err').
///   A single 'err' promotes all compiler warnings to errors.
///   Otherwise, warning number is either enabled ('on'), disabled ('off'), or promoted
///   to an error ('err').
/// - #mi::Sint32 "optimization_level": Sets the optimization level. Possible values: 0 (all
///   optimizations are disabled), 1 (only intra procedural optimizations are enabled), 2 (intra
///   and inter procedural optimizations are enabled). Default: 2.
/// - \c std::string "internal_space": Set the internal space of the backend. Possible values:
///   \c "coordinate_world", \c "coordinate_object". Default: \c "coordinate_world".
/// - \c bool "experimental": If \c true, enables undocumented experimental MDL features. Default:
///   \c false.
///
/// Options for MDL export
/// - \c bool "bundle_resources": If \c true, referenced resources are exported into the same
///   directory as the module, even if they can be found via the module search path. Default:
///   \c false.
///
/// Options for material compilation
/// - \c bool "fold_meters_per_scene_unit": If \c true, occurrences of the functions
///   state::meters_per_scene_unit() and state::scene_units_per_meter() will be folded
///   using the \c meters_per_scene_unit option. Default: \c true
/// - #mi::Float32 "meters_per_scene_unit": The conversion ratio between meters and scene units for
///   this material. Only used if folding is enabled. Default: 1.0f.
/// - #mi::Float32 "wavelength_min": The smallest supported wavelength. Default: 380.0f.
/// - #mi::Float32 "wavelength_max": The largest supported wavelength. Default: 780.0f.
/// - #mi::Float32 "fold_ternary_on_df": Fold all ternary operators of *df types, even in class
///   compilation mode. Default: \c false.
/// - \c bool "ignore_noinline": If \c true, anno::noinline() annotations are ignored during
///   material compilation. Default: \c false.
///
/// Options for code generation
/// - \c bool "fold_meters_per_scene_unit": If \c true, occurrences of the functions
///   state::meters_per_scene_unit() and state::scene_units_per_meter() will be folded
///   using the \c meters_per_scene_unit option. Default: \c true
/// - #mi::Float32 "meters_per_scene_unit": The conversion ratio between meters and scene units for
///   this material. Only used if folding is enabled. Default: 1.0f.
/// - #mi::Float32 "wavelength_min": The smallest supported wavelength. Default: 380.0f.
/// - #mi::Float32 "wavelength_max": The largest supported wavelength. Default: 780.0f.
/// - \c bool "include_geometry_normal": If \c true, the \c "geometry.normal" field will be applied
///   to the MDL state prior to evaluation of the given DF. Default: \c true.
class IMdl_execution_context: public
    base::Interface_declare<0x28eb1f99,0x138f,0x4fa2,0xb5,0x39,0x17,0xb4,0xae,0xfb,0x1b,0xca>
{
public:

    /// \name Messages
    //@{

    /// Returns the number of messages.
    virtual Size get_messages_count() const = 0;

    /// Returns the number of error messages.
    virtual Size get_error_messages_count() const = 0;

    /// Returns the message at index or \c NULL, if no such index exists.
    virtual const IMessage* get_message( Size index) const = 0;

    /// Returns the error message at index or \c NULL, if no such index exists.
    virtual const IMessage* get_error_message( Size index) const = 0;

    /// Clears all messages.
    virtual void clear_messages() = 0;

    /// Adds a message.
    virtual void add_message(
        IMessage::Kind kind,
        base::Message_severity severity,
        Sint32 code,
        const char* message) = 0;

    //@}
    /// \name Options
    //@{

    /// Returns the number of supported options.
    virtual Size get_option_count() const = 0;

    /// Returns the option name at index.
    virtual const char* get_option_name( Size index) const = 0;

    /// Returns the option type name at index.
    virtual const char* get_option_type( const char* name) const = 0;

    /// Returns a string option.
    ///
    /// \param name          The name of the option.
    /// \param[out] value    The value of the option.
    /// \return
    ///                      -  0: Success.
    ///                      - -1: Invalid option name.
    ///                      - -2: The option type does not match the value type.
    virtual Sint32 get_option( const char* name, const char*& value) const = 0;

    /// Returns an int option.
    ///
    /// \param name          The name of the option.
    /// \param[out] value    The value of the option.
    /// \return
    ///                      -  0: Success.
    ///                      - -1: Invalid option name.
    ///                      - -2: The option type does not match the value type.
    virtual Sint32 get_option( const char* name, Sint32& value) const = 0;

    /// Returns a float option.
    ///
    /// \param name          The name of the option.
    /// \param[out] value    The value of the option.
    /// \return
    ///                      -  0: Success.
    ///                      - -1: Invalid option name.
    ///                      - -2: The option type does not match the value type.
    virtual Sint32 get_option( const char* name, Float32& value) const = 0;

    /// Returns a bool option.
    ///
    /// \param name          The name of the option.
    /// \param[out] value    The value of the option.
    /// \return
    ///                      -  0: Success.
    ///                      - -1: Invalid option name.
    ///                      - -2: The option type does not match the value type.
    virtual Sint32 get_option( const char* name, bool& value) const = 0;

    /// Returns an interface option.
    ///
    /// \param name          The name of the option.
    /// \param[out] value    The value of the option.
    /// \return
    ///                      -  0: Success.
    ///                      - -1: Invalid option name.
    ///                      - -2: The option type does not match the value type.
    virtual Sint32 get_option( const char* name, const base::IInterface** value) const = 0;

    /// Returns an interface option.
    ///
    /// \param name          The name of the option.
    /// \param return_code
    ///                      -  0: Success.
    ///                      - -1: Invalid option name.
    ///                      - -2: The option type does not match the value type.
    /// \return              The interface value or \c NULL.
    template<typename T>
    const T* get_option( const char* name, Sint32& return_code)
    {
        const base::IInterface* pointer = 0;
        return_code = get_option( name, &pointer);
        if( return_code != 0 || !pointer)
            return 0;

        base::Handle<const base::IInterface> handle( pointer);
        const T* pointer_T = pointer->get_interface<T>();
        if( !pointer_T) {
            return_code = -2;
            return 0;
        }

        return pointer_T;
    }

    /// Sets a string option.
    ///
    /// \param name     The name of the option.
    /// \param value    The value of the option.
    /// \return
    ///                 -  0: Success.
    ///                 - -1: Invalid option name.
    ///                 - -2: The option type does not match the value type.
    virtual Sint32 set_option( const char* name, const char* value) = 0;

    /// Sets an int option.
    ///
    /// \param name     The name of the option.
    /// \param value    The value of the option.
    /// \return
    ///                 -  0: Success.
    ///                 - -1: Invalid option name.
    ///                 - -2: The option type does not match the value type.
    virtual Sint32 set_option( const char* name, Sint32 value) = 0;

    /// Sets a float option.
    ///
    /// \param name     The name of the option.
    /// \param value    The value of the option.
    /// \return
    ///                 -  0: Success.
    ///                 - -1: Invalid option name.
    ///                 - -2: The option type does not match the value type.
    virtual Sint32 set_option( const char* name, Float32 value) = 0;

    /// Sets a bool option.
    ///
    /// \param name     The name of the option.
    /// \param value    The value of the option.
    /// \return
    ///                 -  0: Success.
    ///                 - -1: Invalid option name.
    ///                 - -2: The option type does not match the value type.
    ///                 - -3: The value is invalid in the context of the option.
    virtual Sint32 set_option( const char* name, bool value) = 0;

    /// Sets an interface option.
    ///
    /// \param name     The name of the option.
    /// \param value    The value of the option.
    /// \return
    ///                 -  0: Success.
    ///                 - -1: Invalid option name.
    ///                 - -2: The option type does not match the value type.
    ///                 - -3: The value is invalid in the context of the option.
    virtual Sint32 set_option( const char* name, const base::IInterface* value) = 0;

    //@}
};


/**@}*/ // end group mi_neuray_mdl_misc

} // namespace neuraylib
} // namespace mi

#endif // MI_NEURAYLIB_IMDL_EXECUTION_CONTEXT_H

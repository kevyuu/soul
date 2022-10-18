/***************************************************************************************************
 * Copyright 2022 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief API component for MDL related settings.

#ifndef MI_NEURAYLIB_IMDL_CONFIGURATION_H
#define MI_NEURAYLIB_IMDL_CONFIGURATION_H

#include <mi/base/interface_declare.h>

namespace mi {

class IString;

namespace base { class ILogger; }

namespace neuraylib {

class IMdl_entity_resolver;

/** \addtogroup mi_neuray_configuration
@{
*/

/// This interface can be used to query and change the MDL configuration.
class IMdl_configuration : public
    mi::base::Interface_declare<0x2657ec0b,0x8a40,0x46c5,0xa8,0x3f,0x2b,0xb5,0x72,0xa0,0x8b,0x9c>
{
public:
    /// \name Logging
    //@{

    /// Sets the logger.
    ///
    /// \if IRAY_API Sets the receiving logger, see also
    /// #mi::neuraylib::ILogging_configuration::set_receiving_logger().\else
    /// Installs a custom logger, and deinstalls the previously installed logger.
    /// By default, an internal logger is installed that prints all messages of severity
    /// #mi::base::details::MESSAGE_SEVERITY_INFO or higher to stderr.\endif
    ///
    /// \param logger   The new logger that receives all log messages. Passing \c NULL is allowed
    ///                 to reinstall the default logger.
    virtual void set_logger( base::ILogger* logger) = 0;

    /// Returns the used logger.
    ///
    /// \return   \if IRAY_API Returns the forwarding logger. See
    ///           also #mi::neuraylib::ILogging_configuration::get_forwarding_logger(). \else The
    ///           currently used logger ( either explicitly installed via #set_logger(), or
    ///           the default logger). Never returns \c NULL. \endif
    virtual base::ILogger* get_logger() = 0;

    /// \name MDL paths
    //@{

    /// Adds a path to the list of paths to search for MDL modules.
    ///
    /// This search path is also used for resources referenced in MDL modules. By default, the list
    /// of MDL paths is empty.
    ///
    /// \param path                The path to be added.
    /// \return
    ///                            -  0: Success.
    ///                            - -1: Invalid parameters ( \c NULL pointer).
    ///                            - -2: Invalid path.
    virtual Sint32 add_mdl_path( const char* path) = 0;

    /// Removes a path from the list of paths to search for MDL modules.
    ///
    /// This search path is also used for resources referenced in MDL modules. By default, the list
    /// of MDL paths is empty.
    ///
    /// \param path                The path to be removed.
    /// \return
    ///                            -  0: Success.
    ///                            - -1: Invalid parameters ( \c NULL pointer).
    ///                            - -2: There is no such path in the path list.
    virtual Sint32 remove_mdl_path( const char* path) = 0;

    /// Clears the list of paths to search for MDL modules.
    ///
    /// This search path is also used for resources referenced in MDL modules. By default, the list
    /// of MDL paths is empty.
    virtual void clear_mdl_paths() = 0;

    /// Returns the number of paths to search for MDL modules.
    ///
    /// This search path is also used for resources referenced in MDL modules. By default, the list
    /// of MDL paths is empty.
    ///
    /// \return                    The number of currently configured paths.
    virtual Size get_mdl_paths_length() const = 0;

    /// Returns the \p index -th path to search for MDL modules.
    ///
    /// This search path is also used for resources referenced in MDL modules. By default, the list
    /// of MDL paths is empty.
    ///
    /// \return                    The \p index -th path, or \c NULL if \p index is out of bounds.
    virtual const IString* get_mdl_path( Size index) const = 0;

    /// Returns the number of MDL system paths.
    virtual Size get_mdl_system_paths_length() const = 0;

    /// Returns the \p index -th path in the MDL system paths.
    ///
    /// The default MDL system path is
    /// - \c %%PROGRAMDATA%\\NVIDIA \c Corporation\\mdl (on Windows),
    /// - \c /opt/nvidia/mdl (on Linux), and
    /// - \c /Library/Application \c Support/NVIDIA \c Corporation/mdl (on Mac OS).
    ///
    /// The MDL system paths can be changed via the environment variable \c MDL_SYSTEM_PATH. The
    /// environment variable can contain multiple paths which are separated by semicolons (on
    /// Windows) or colons (on Linux and Mac OS), respectively.
    ///
    /// \return The \p index -th path, or \c NULL if \p index is out of bounds.
    virtual const char* get_mdl_system_path( Size index) const = 0;

    /// Adds the MDL system paths to the MDL search path.
    inline void add_mdl_system_paths()
    {
        for( mi::Size i = 0, n = get_mdl_system_paths_length(); i < n; ++i)
            add_mdl_path( get_mdl_system_path( i));
    }

    /// Returns the number of MDL user paths.
    virtual Size get_mdl_user_paths_length() const = 0;

    /// Returns the \p index -th path in the MDL user paths.
    ///
    /// The default MDL user path is
    /// - \c %%DOCUMENTS%\\mdl (on Windows),
    /// - \c $HOME/Documents/mdl (on Linux), and
    /// - \c $HOME/Documents/mdl (on Mac OS),
    /// where \c \%DOCUMENTS% refers to the standard folder identified by \c FOLDERID_Documents from
    /// the Windows API (usually \c %%USERPROFILE%\\Documents).
    ///
    /// The MDL user paths can be changed via the environment variable \c MDL_USER_PATH. The
    /// environment variable can contain multiple paths which are separated by semicolons (on
    /// Windows) or colons (on Linux and Mac OS), respectively.
    ///
    /// \return The \p index -th path, or \c NULL if \p index is out of bounds.
    virtual const char* get_mdl_user_path( Size index) const = 0;

    /// Adds the MDL user paths to the MDL search path.
    inline void add_mdl_user_paths()
    {
        for( mi::Size i = 0, n = get_mdl_user_paths_length(); i < n; ++i)
            add_mdl_path( get_mdl_user_path( i));
    }

    //@}
    /// \name Resource paths
    //@{

    /// Adds a path to the list of paths to search for resources, i.e., textures, light profiles,
    /// and BSDF measurements.
    ///
    /// Note that for MDL resources referenced in .\c mdl files the MDL search paths are considered,
    /// not the resource search path. By default, the list of resource paths is empty.
    ///
    /// \param path                The path to be added.
    /// \return
    ///                            -  0: Success.
    ///                            - -1: Invalid parameters ( \c NULL pointer).
    ///                            - -2: Invalid path.
    virtual Sint32 add_resource_path( const char* path) = 0;

    /// Removes a path from the list of paths to search for resources, i.e., textures, light
    /// profiles, and BSDF measurements.
    ///
    /// Note that for MDL resources referenced in .\c mdl files the MDL search paths are considered,
    /// not the resource search path. By default, the list of resource paths is empty.
    ///
    /// \param path                The path to be removed.
    /// \return
    ///                            -  0: Success.
    ///                            - -1: Invalid parameters ( \c NULL pointer).
    ///                            - -2: There is no such path in the path list.
    virtual Sint32 remove_resource_path( const char* path) = 0;

    /// Clears the list of paths to search for resources, i.e., textures, light profiles,
    /// and BSDF measurements.
    ///
    /// Note that for MDL resources referenced in .\c mdl files the MDL search paths are considered,
    /// not the resource search path. By default, the list of resource paths is empty.
    virtual void clear_resource_paths() = 0;

    /// Returns the number of paths to search for resources, i.e., textures, light profiles,
    /// and BSDF measurements.
    ///
    /// Note that for MDL resources referenced in .\c mdl files the MDL search paths are considered,
    /// not the resource search path. By default, the list of resource paths is empty.
    ///
    /// \return                    The number of currently configured paths.
    virtual Size get_resource_paths_length() const = 0;

    /// Returns the \p index -th path to search for resources, i.e., textures, light profiles,
    /// and BSDF measurements.
    ///
    /// Note that for MDL resources referenced in .\c mdl files the MDL search paths are considered,
    /// not the resource search path. By default, the list of resource paths is empty.
    ///
    /// \return                    The \p index -th path, or \c NULL if \p index is out of bounds.
    virtual const IString* get_resource_path( Size index) const = 0;

    //@}
    /// \name Miscellaneous settings
    //@{

    /// Defines whether a cast operator is automatically inserted for compatible argument types.
    ///
    /// If set to \c true, an appropriate cast operator is automatically inserted if arguments for
    /// instances of #mi::neuraylib::IFunction_call have a different but compatible type. If set to
    /// \c false, such an assignment fails and it is necessary to insert the cast operator
    /// explicitly. Default: \c true.
    ///
    /// \see #mi::neuraylib::IExpression_factory::create_cast().
    ///
    /// \param value    \c True to enable the feature, \c false otherwise.
    /// \return
    ///                -  0: Success.
    ///                - -1: The method cannot be called at this point of time.
    virtual Sint32 set_implicit_cast_enabled( bool value) = 0;

    /// Indicates whether the SDK is supposed to automatically insert the cast operator for
    /// compatible types.
    ///
    /// \see #set_implicit_cast_enabled()
    virtual bool get_implicit_cast_enabled() const = 0;

    /// Defines whether an attempt is made to expose names of let expressions.
    ///
    /// If set to \c true, the MDL compiler attempts to represent let expressions as temporaries,
    /// and makes the name of let expressions available as names of such temporaries. In order to
    /// do so, certain optimizations are disabled, in particular, constant folding. These names are
    /// only available on material and functions definitions, not on compiled materials, which are
    /// always highly optimized. Default: \c true.
    ///
    /// \note Since some optimizations are essential for inner workings of the MDL compiler, there
    //        is no guarantee that the name of a particular let expression is exposed.
    ///
    /// \see #mi::neuraylib::IFunction_definition::get_temporary_name()
    virtual Sint32 set_expose_names_of_let_expressions( bool value) = 0;

    /// Indicates whether an attempt is made to expose names of let expressions.
    ///
    /// \see #set_expose_names_of_let_expressions()
    virtual bool get_expose_names_of_let_expressions() const = 0;

    /// Configures the behavior of \c df::simple_glossy_bsdf() in MDL modules
    /// of versions smaller than 1.3.
    ///
    /// \note \if IRAY_API This setting can only be configured before \NeurayProductName has been
    ///       started. \else This function has no effect in the MDL SDK and always returns -1.
    ///       \endif
    ///
    /// \param value    \c True to enable the feature, \c false otherwise.
    /// \return
    ///                -  0: Success.
    ///                - -1: The method cannot be called at this point of time.
    virtual Sint32 set_simple_glossy_bsdf_legacy_enabled( bool value) = 0;

    /// Returns \c true if the legacy behavior for bsdfs of type \c df::simple_glossy_bsdf() used
    /// in MDL modules with versions smaller that 1.3 is enabled, \c false otherwise.
    virtual bool get_simple_glossy_bsdf_legacy_enabled() const = 0;

    //@}
    /// \name Entity resolver
    //@{

    /// Returns an instance of the built-in entity resolver.
    ///
    /// \note The returned instance contains a copy of the currently configured search paths,
    ///       subsequent changes to the search paths are not reflected in this instance.
    virtual IMdl_entity_resolver* get_entity_resolver() const = 0;

    /// Installs an external entity resolver.
    ///
    /// \param resolver   The external entity resolver to be used instead of the built-in entity
    ///                   resolver. Pass \c NULL to uninstall a previously installed external
    ///                   entity resolver.
    ///
    /// \note MDL archive creation is not supported with an external entity resolver ( see
    ///       #mi::neuraylib::IMdl_archive_api::create_archive()).
    virtual void set_entity_resolver( IMdl_entity_resolver* resolver) = 0;

    //@}
    /// \name Miscellaneous settings
    //@{

    /// Defines whether materials are treated as functions.
    ///
    /// \see \ref mi_mdl_materials_are_functions, #get_materials_are_functions().
    virtual Sint32 set_materials_are_functions( bool value) = 0;

    /// Indicates whether materials are treated as functions.
    ///
    /// \see \ref mi_mdl_materials_are_functions, #set_materials_are_functions().
    virtual bool get_materials_are_functions() const = 0;

    /// Defines whether encoded names are enabled.
    ///
    /// See \ref mi_mdl_encoded_names for details.
    ///
    /// This feature is enabled by default. Support for the disabled feature will be deprecated and
    /// removed in a future release.
    ///
    /// This can only be configured before \neurayProductName has been started.
    ///
    /// \note This feature does not yet support module names containing parentheses or commas.
    ///
    /// \if IRAY_API \note All hosts in a cluster need to agree on this setting. \endif
    ///
    /// \if IRAY_API \note This setting needs to be the identical during export and import of
    ///                    \c .mib files. \endif
    ///
    /// \if IRAY_API \note Support for Iray Bridge requires that this feature is enabled. \endif
    ///
    /// \see #get_encoded_names_enabled().
    virtual Sint32 set_encoded_names_enabled( bool value) = 0;

    /// Indicates whether encoded names are enabled.
    ///
    /// \see #set_encoded_names_enabled().
    virtual bool get_encoded_names_enabled() const = 0;

    //@}
};

/**@}*/ // end group mi_neuray_configuration

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IMDL_CONFIGURATION_H

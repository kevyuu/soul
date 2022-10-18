/***************************************************************************************************
 * Copyright 2022 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief      Interface to handle waiting for threads and notifying waiting threads.

#ifndef MI_NEURAYLIB_IMDL_LOADING_WAIT_HANDLE_H
#define MI_NEURAYLIB_IMDL_LOADING_WAIT_HANDLE_H

#include <mi/base/interface_declare.h>

namespace mi {

namespace neuraylib {


/** \addtogroup mi_neuray_mdl_misc
@{
*/

/// Interface of a loading wait handle.
///
/// When loading in parallel, the element is loaded only in the context of the first call to
/// load function. Every further thread that is trying to load this element either directly or
/// indirectly using imports will block until the initial loading process has ended.
/// This interface allows to specify how the waiting and the awakening of threads is realized.
/// The default implementation that comes with the SDK uses conditional variable of the standard
/// template library. It is sufficient for most applications. Only in case the application wants 
/// to use a custom thread scheduling or light weight threads that are not compatible with std, 
/// this interface, along with the corresponding factory, has to be implemented.
class IMdl_loading_wait_handle : public
    base::Interface_declare<0xc942596c,0x80fd,0x46d1,0x9a,0x1d,0x57,0x4c,0x27,0xf9,0x20,0x24>
{
public:
    /// Called when the element is currently loaded by another threads.
    /// Blocks until the loading thread calls \c notify. Calling \c wait after \c notify has been 
    /// called already is valid. \c wait will not block in this case and return silently. 
    /// The number of threads that can wait at the same time is not limited. 
    /// The order in which threads awake after \c notify has been called is not specified.
    virtual void wait() = 0;

    /// Called by the loading thread after loading is done to wake the waiting threads. Other
    /// threads are not supposed to call this method. It is allowed to call notify multiple times  
    /// if other threads are waiting or not. However, it is not allowed to call notify multiple
    /// times with different \c result_codes.
    /// 
    ///
    /// \param result_code      The result code that is passed to the waiting threads.
    ///                         Values >=  are indicate that the loaded element is available.
    ///                         Negative values are treated as failure for dependent element.
    virtual void notify(Sint32 result_code) = 0;

    /// Gets the result code that was passed to \c notify. This allows waiting threads to
    /// query the result of the loading process after returning from \c wait.
    /// 
    /// \return       The result code.
    virtual Sint32 get_result_code() const = 0;
};


/// Interface of a loading wait handle factory.
///
/// This class creates wait handles.
class IMdl_loading_wait_handle_factory : public
    base::Interface_declare<0x32ee19,0x2020,0x4cca,0xa7,0xd7,0xde,0xa1,0x7a,0xc6,0x95,0x11>
{
public:
    /// Creates a loading wait handle.
    ///
    /// \return         The created handle.
    virtual IMdl_loading_wait_handle* create_wait_handle() const = 0;
};

/**@}*/ // end group mi_neuray_mdl_misc

} // namespace neuraylib
} // namespace mi

#endif // MI_NEURAYLIB_IMDL_LOADING_WAIT_HANDLE_H

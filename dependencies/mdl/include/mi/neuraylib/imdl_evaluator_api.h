/***************************************************************************************************
 * Copyright 2022 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief      API component that gives access to the MDL evaluator.

#ifndef MI_NEURAYLIB_IMDL_EVALUATOR_H
#define MI_NEURAYLIB_IMDL_EVALUATOR_H

#include <mi/base/interface_declare.h>
#include <mi/neuraylib/version.h>

namespace mi {
namespace neuraylib {

class IExpression;
class IFunction_call;
class IMaterial_instance;
class ITransaction;
class IValue_bool;
class IValue_factory;

/** \addtogroup mi_neuray_mdl_misc
@{
*/

/// Provides access to various functions for the evaluation of MDL expressions.
class IMdl_evaluator_api : public
    mi::base::Interface_declare<0x1dc8e8c2,0xa19e,0x4dc9,0xa3,0x0f,0xeb,0xb4,0x0a,0xf1,0x08,0x58>
{
public:
    virtual const IValue_bool* MI_NEURAYLIB_DEPRECATED_METHOD_13_0(is_material_parameter_enabled)(
        ITransaction* trans,
        IValue_factory* fact,
        const IMaterial_instance* inst,
        Size index,
        Sint32* error) const = 0;

    /// Evaluates if a function call parameter is enabled, i.e., the \c enable_if condition
    /// evaluates to \c true).
    ///
    /// \param[in]  trans  the transaction
    /// \param[in]  fact   the expression factory to create the result value
    /// \param[in]  call   the function call
    /// \param[in]  index  the index of the material instance parameter
    /// \param[out] error  An optional pointer to an #mi::Sint32 to which an error code will be
    ///                    written. The error codes have the following meaning:
    ///                    -  0: Success.
    ///                    - -1: An input parameter is NULL.
    ///                    - -2: The parameter index is out of bounds.
    ///                    - -3: A malformed expression (contains temporaries).
    ///                    - -4: An unsupported expression occurred.
    ///                    - -5: The evaluation was aborted, too complex to evaluate.
    ///
    /// \return NULL if the condition was to complex to evaluate, else \c true or \c false.
    virtual const IValue_bool* is_function_parameter_enabled(
        ITransaction* trans,
        IValue_factory* fact,
        const IFunction_call* call,
        Size index,
        Sint32* error) const = 0;
};

/**@}*/ // end group mi_neuray_mdl_misc

} // namespace neuraylib
} // namespace mi

#endif // MI_NEURAYLIB_IMDL_EVALUATOR_H

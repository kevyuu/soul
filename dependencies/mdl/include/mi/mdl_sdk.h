/***************************************************************************************************
 * Copyright 2022 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file mi/mdl_sdk.h
/// \brief \NeurayApiName
///
/// See \ref mi_neuray.

#ifndef MI_MDL_SDK_H
#define MI_MDL_SDK_H

#include <mi/base.h>
#include <mi/math.h>

#include <mi/neuraylib/annotation_wrapper.h>
#include <mi/neuraylib/argument_editor.h>
#include <mi/neuraylib/assert.h>
#include <mi/neuraylib/bsdf_isotropic_data.h>
#include <mi/neuraylib/definition_wrapper.h>
#include <mi/neuraylib/factory.h>
#include <mi/neuraylib/iallocator.h>
#include <mi/neuraylib/iarray.h>
#include <mi/neuraylib/iattribute_container.h>
#include <mi/neuraylib/iattribute_set.h>
#include <mi/neuraylib/ibbox.h>
#include <mi/neuraylib/ibsdf_isotropic_data.h>
#include <mi/neuraylib/ibsdf_measurement.h>
#include <mi/neuraylib/ibuffer.h>
#include <mi/neuraylib/icanvas.h>
#include <mi/neuraylib/icolor.h>
#include <mi/neuraylib/icompiled_material.h>
#include <mi/neuraylib/icompound.h>
#include <mi/neuraylib/idata.h>
#include <mi/neuraylib/idatabase.h>
#include <mi/neuraylib/idebug_configuration.h>
#include <mi/neuraylib/idynamic_array.h>
#include <mi/neuraylib/ienum.h>
#include <mi/neuraylib/ienum_decl.h>
#include <mi/neuraylib/iexpression.h>
#include <mi/neuraylib/ifactory.h>
#include <mi/neuraylib/ifunction_call.h>
#include <mi/neuraylib/ifunction_definition.h>
#include <mi/neuraylib/iimage.h>
#include <mi/neuraylib/iimage_api.h>
#include <mi/neuraylib/iimage_plugin.h>
#include <mi/neuraylib/iimpexp_base.h>
#include <mi/neuraylib/ilightprofile.h>
#include <mi/neuraylib/imap.h>
#include <mi/neuraylib/imaterial_definition.h>
#include <mi/neuraylib/imaterial_instance.h>
#include <mi/neuraylib/imatrix.h>
#include <mi/neuraylib/imdl_archive_api.h>
#include <mi/neuraylib/imdl_backend.h>
#include <mi/neuraylib/imdl_backend_api.h>
#include <mi/neuraylib/imdl_compatibility_api.h>
#include <mi/neuraylib/imdl_compiler.h>
#include <mi/neuraylib/imdl_configuration.h>
#include <mi/neuraylib/imdl_discovery_api.h>
#include <mi/neuraylib/imdl_distiller_api.h>
#include <mi/neuraylib/imdl_entity_resolver.h>
#include <mi/neuraylib/imdl_evaluator_api.h>
#include <mi/neuraylib/imdl_execution_context.h>
#include <mi/neuraylib/imdl_factory.h>
#include <mi/neuraylib/imdl_i18n_configuration.h>
#include <mi/neuraylib/imdl_impexp_api.h>
#include <mi/neuraylib/imdl_loading_wait_handle.h>
#include <mi/neuraylib/imdl_module_builder.h>
#include <mi/neuraylib/imdl_module_transformer.h>
#include <mi/neuraylib/imdle_api.h>
#include <mi/neuraylib/imodule.h>
#include <mi/neuraylib/ineuray.h>
#include <mi/neuraylib/inumber.h>
#include <mi/neuraylib/iplugin_api.h>
#include <mi/neuraylib/iplugin_configuration.h>
#include <mi/neuraylib/ipointer.h>
#include <mi/neuraylib/ireader.h>
#include <mi/neuraylib/ireader_writer_base.h>
#include <mi/neuraylib/iref.h>
#include <mi/neuraylib/iscene_element.h>
#include <mi/neuraylib/iscope.h>
#include <mi/neuraylib/ispectrum.h>
#include <mi/neuraylib/istream_position.h>
#include <mi/neuraylib/istring.h>
#include <mi/neuraylib/istructure.h>
#include <mi/neuraylib/istructure_decl.h>
#include <mi/neuraylib/itexture.h>
#include <mi/neuraylib/itile.h>
#include <mi/neuraylib/itransaction.h>
#include <mi/neuraylib/itype.h>
#include <mi/neuraylib/iuuid.h>
#include <mi/neuraylib/ivalue.h>
#include <mi/neuraylib/ivector.h>
#include <mi/neuraylib/iversion.h>
#include <mi/neuraylib/iwriter.h>
#include <mi/neuraylib/matrix_typedefs.h>
#include <mi/neuraylib/set_get.h>
#include <mi/neuraylib/target_code_types.h>
#include <mi/neuraylib/type_traits.h>
#include <mi/neuraylib/typedefs.h>
#include <mi/neuraylib/vector_typedefs.h>
#include <mi/neuraylib/version.h>

namespace mi {

/// Namespace for the \neurayApiName.
/// \ingroup mi_neuray
namespace neuraylib {

/// \defgroup mi_neuray \NeurayApiName
/// \brief \NeurayApiName
///
/// \par Include File:
/// <tt> \#include <mi/mdl_sdk.h></tt>

} // namespace neuraylib

} // namespace mi

#endif // MI_MDL_SDK_H

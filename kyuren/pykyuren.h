#define PY_SSIZE_T_CLEAN
#include <Python.h>

PyObject* on_register_func(PyObject*, PyObject* args, PyObject* keywds);

PyObject* init_func(PyObject* /*self*/, PyObject* args);

PyObject* update_mesh_func(PyObject*, PyObject* args);

PyObject* update_light_func(PyObject*, PyObject* args);

PyObject* create_mesh_entity_func(PyObject*, PyObject* args);
PyObject* clear_mesh_entities_func(PyObject*, PyObject* args);

PyObject* set_perspective_camera_func(PyObject*, PyObject* args);

PyObject* sync_depsgraph_func(PyObject*, PyObject* args);

PyObject* draw_func(PyObject*, PyObject* value);
PyObject* exit_func(PyObject* /*self*/, PyObject* value);
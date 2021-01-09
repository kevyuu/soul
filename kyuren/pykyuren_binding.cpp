#include "pykyuren.h"

// Our Module's Function Definition struct
// We require this `NULL` to signal the end of our method
// definition
static PyMethodDef methods[] = {
    {"on_register", (PyCFunction)(void(*)(void))on_register_func, METH_VARARGS | METH_KEYWORDS, ""},
    {"init", init_func, METH_VARARGS, ""},
    {"update_mesh", update_mesh_func, METH_VARARGS, ""},
    {"update_light", update_light_func, METH_VARARGS, ""},
    {"set_perspective_camera", set_perspective_camera_func, METH_VARARGS, ""},
    {"create_mesh_entity", create_mesh_entity_func, METH_VARARGS, ""},
    {"clear_mesh_entities", clear_mesh_entities_func, METH_VARARGS, ""},
    {"sync_depsgraph", sync_depsgraph_func, METH_VARARGS, ""},
    {"draw", draw_func, METH_VARARGS, ""},
    {"exit", exit_func, METH_O, ""},
    { NULL, NULL, 0, NULL }
};

// Our Module Definition struct
static struct PyModuleDef pykyuren = {
    PyModuleDef_HEAD_INIT,
    "pykyuren",
    "KyuRender",
    -1,
    methods
};

// Initializes our module using our above struct
PyMODINIT_FUNC PyInit_pykyuren(void)
{
    return PyModule_Create(&pykyuren);
}
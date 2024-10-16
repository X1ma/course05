#ifndef GDEXAMPLE_REGISTER_TYPES_H
#define GDEXAMPLE_REGISTER_TYPES_H

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void init_module_a(ModuleInitializationLevel p_level);
void uninit_module_a(ModuleInitializationLevel p_level);

void init_module_b(ModuleInitializationLevel p_level);
void uninit_module_b(ModuleInitializationLevel p_level);

#endif // GDEXAMPLE_REGISTER_TYPES_H

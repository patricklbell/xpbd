import bpy
bl_info = {
    "name" : "xpbd",
    "author" : "patricklbell",
    "description" : "",
    "blender" : (2, 80, 0),
    "version" : (0, 0, 1),
    "location" : "",
    "warning" : "",
    "category" : "Generic"
}

from . import auto_load

auto_load.init()

def register():
    auto_load.register()

def unregister():
    auto_load.unregister()

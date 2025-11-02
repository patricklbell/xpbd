import bpy
import sys
from . import ids
from .world import get_world, load_body, world
from .ids import id_mapper

class XPBDPanel(bpy.types.Panel):
    bl_label = "XPBD"
    bl_idname = "OBJECT_PT_xpbd"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "physics"

    def draw(self, context):
        layout = self.layout
        obj = context.object

        if obj is not None:
            layout.operator("object.toggle_xpbd_body", text="Body")

class ToggleXPBDRBodyOperator(bpy.types.Operator):
    bl_idname = "object.toggle_xpbd_body"
    bl_label = "Toggle XPBD Body"
    bl_description = "Toggle XPBD Body for the active object"

    def execute(self, context):
        obj = context.object
        if obj is None:
            self.report({'WARNING'}, "No active object selected")
            return {'CANCELLED'}
        
        if obj.xpbd_body_id == -1:
            body = world.add_body()
            body.set_mass(obj.xpbd_inv_body_mass)
            obj.xpbd_body_id = id_mapper.map_new(body)
        else:
            id_mapper.unmap(obj.xpbd_body_id)
            obj.xpbd_body_id = -1
        
        self.report({'INFO'}, "Body toggled for object")
        return {'FINISHED'}

class XPBDBodyPanel(bpy.types.Panel):
    bl_label = "XPBD Body"
    bl_idname = "OBJECT_PT_xpbd_body"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "physics"
    bl_parent_id = XPBDPanel.bl_idname

    @classmethod
    def poll(cls, context):
        return context.object is not None and context.object.xpbd_body_id != -1

    def draw(self, context):
        layout = self.layout
        obj = context.object

        layout.prop(obj, 'xpbd_inv_body_mass')

def xpbd_body_inv_mass_update(self, context):
    if self.xpbd_body_id !=  -1:
        body = id_mapper.get(self.xpbd_body_id)
        if body is None:
            self.xpbd_body_id =  -1
            return

        body.set_inv_mass(self.xpbd_inv_body_mass)

def register():
    bpy.types.Object.xpbd_body_id = bpy.props.IntProperty(default=-1)

    bpy.types.Object.xpbd_inv_body_mass = bpy.props.FloatProperty(
        name="Inverse Mass",
        description="XPBD Inverse Body mass",
        min = 0.0,
        default=1.0,
        update=xpbd_body_inv_mass_update,
    )
def unregister():
    del bpy.types.Object.xpbd_inv_body_mass
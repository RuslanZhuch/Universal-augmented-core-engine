import bpy
import sys
import os
import asyncio

root_dir = os.path.dirname(bpy.data.filepath)
libs_dir = root_dir + "\lib"
print(libs_dir)
sys.path.append(libs_dir)

import uacedb

class property_rt_blend_desc(bpy.types.PropertyGroup):
    def on_rt_blend_desc_property_update(self, context):
        if self.updates_enabled:
            print("Property updated:", self, context)
            blend_desc_to_write = uacedb.Desc.BlendDesc(name=self.name)
            
            blend_desc_to_write.blend_enable = self.blend_enable
            blend_desc_to_write.logic_op_enable = self.logic_op_enable
            blend_desc_to_write.src_blend = self.src_blend
            blend_desc_to_write.dest_blend = self.dest_blend
            blend_desc_to_write.blend_op = self.blend_op
            blend_desc_to_write.src_blend_alpha = self.src_blend_alpha
            blend_desc_to_write.dest_blend_alpha = self.dest_blend_alpha
            blend_desc_to_write.blend_op_alpha = self.blend_op_alpha
            blend_desc_to_write.logic_op = self.logic_op
            blend_desc_to_write.render_target_write_mask = self.render_target_write_mask
            global root_dir
            bd_path = root_dir + "/database/bsidedb.sqlite"
            
            asyncio.run(uacedb.Desc(uacedb.Desc.TYPE_RT_BLEND_DESC, bd_path).write(blend_desc_to_write))
            
    name: bpy.props.StringProperty(name="RT blend desc", update=on_rt_blend_desc_property_update)
    blend_enable: bpy.props.BoolProperty(name="Blend enable", update=on_rt_blend_desc_property_update)
    logic_op_enable: bpy.props.IntProperty(name="Logic op enable", update=on_rt_blend_desc_property_update)
    src_blend: bpy.props.IntProperty(name="Source blend", update=on_rt_blend_desc_property_update)
    dest_blend: bpy.props.IntProperty(name="Destination blend", update=on_rt_blend_desc_property_update)
    blend_op: bpy.props.IntProperty(name="Blend operation", update=on_rt_blend_desc_property_update)
    src_blend_alpha: bpy.props.IntProperty(name="Source blend alpha", update=on_rt_blend_desc_property_update)
    dest_blend_alpha: bpy.props.IntProperty(name="Destination blend alpha", update=on_rt_blend_desc_property_update)
    blend_op_alpha: bpy.props.IntProperty(name="Blend operation alpha", update=on_rt_blend_desc_property_update)
    logic_op: bpy.props.IntProperty(name="Logic operation", update=on_rt_blend_desc_property_update)
    render_target_write_mask: bpy.props.IntProperty(name="Render target write mask", update=on_rt_blend_desc_property_update)
    
    updates_enabled: bpy.props.BoolProperty(name="Update enabled", default=False)

bpy.utils.register_class(property_rt_blend_desc)
bpy.types.Scene.render_target_blend_descs = bpy.props.CollectionProperty(type=property_rt_blend_desc)


class property_rt_blend_desc_index(bpy.types.PropertyGroup):
    bpy.types.Scene.render_target_blend_desc_index = bpy.props.IntProperty(\
    name = "rt blend desc index",\
    description = "---",\
    default = 0,\
    min = 0\
    )

class UACE_UL_rt_blend_descs(bpy.types.UIList):
    def draw_item(self, context, layout, data, item, icon, active_data, active_propname, index):
        layout.label(text=str(item.name))
        layout.label(text=str(item.blend_enable))
        layout.label(text=str(item.logic_op_enable))
        layout.label(text=str(item.src_blend))
        layout.label(text=str(item.dest_blend))
        layout.label(text=str(item.blend_op))


class UACE_PT_rt_blend_descs(bpy.types.Panel):
    bl_label = "Render target blend descs"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "scene"

    def draw(self, context):
        layout = self.layout

        row = layout.row()
        row.label(text="Desc name")
        row.label(text="blend en.")
        row.label(text="src blend")
        row.label(text="dest blend")
        row.label(text="blend op")
        
        layout.template_list("UACE_UL_rt_blend_descs", "j", \
        context.scene, "render_target_blend_descs", context.scene, \
        "render_target_blend_desc_index", type='DEFAULT')

        #layout.operator("scene.add_blend_desc")
        #layout.operator("scene.remove_blend_desc")
        num_of_els = len(context.scene.render_target_blend_descs)
        if num_of_els > 0:
            context.scene.render_target_blend_desc_index = min(context.scene.render_target_blend_desc_index, num_of_els - 1)
            item = context.scene.render_target_blend_descs[context.scene.render_target_blend_desc_index]
            layout.prop(item, "name")
            layout.prop(item, "blend_enable")
            layout.prop(item, "logic_op_enable")
            layout.prop(item, "src_blend")
            layout.prop(item, "dest_blend")
            layout.prop(item, "blend_op")
            layout.prop(item, "src_blend_alpha")
            layout.prop(item, "dest_blend_alpha")
            layout.prop(item, "blend_op_alpha")
            layout.prop(item, "logic_op")
            layout.prop(item, "render_target_write_mask")

class property_multilight_scene_layers(bpy.types.PropertyGroup):
    def on_blend_desc_property_update(self, context):
        if self.updates_enabled:
            print("Property updated:", self, context)
            blend_desc_to_write = uacedb.Desc.BlendDesc(name=self.name)
            
            blend_desc_to_write.alpha_to_coverage = self.alpha_to_coverage
            blend_desc_to_write.independe_blend_enable = self.independe_blend_enable
            blend_desc_to_write.rt0_blend_desc_name = self.rt0_blend_desc_name
            blend_desc_to_write.rt1_blend_desc_name = self.rt1_blend_desc_name
            blend_desc_to_write.rt2_blend_desc_name = self.rt2_blend_desc_name
            blend_desc_to_write.rt3_blend_desc_name = self.rt3_blend_desc_name
            blend_desc_to_write.rt4_blend_desc_name = self.rt4_blend_desc_name
            blend_desc_to_write.rt5_blend_desc_name = self.rt5_blend_desc_name
            blend_desc_to_write.rt6_blend_desc_name = self.rt6_blend_desc_name
            blend_desc_to_write.rt7_blend_desc_name = self.rt7_blend_desc_name
            global root_dir
            bd_path = root_dir + "/database/bsidedb.sqlite"
            
            asyncio.run(uacedb.Desc(uacedb.Desc.TYPE_BLEND_DESC, bd_path).write(blend_desc_to_write))
            
    def on_blend_desc_set_rt_desc_names(self, context):
        print("enum update triggered", self)
        
    
        global root_dir
        bd_path = root_dir + "/database/bsidedb.sqlite"
        descs = asyncio.run(uacedb.Desc(uacedb.Desc.TYPE_RT_BLEND_DESC, bd_path).read_all())
        
        emp = " "
        out_data = [(emp, emp, emp)]
        
        for desc in descs:
            desc_name = desc.name
            out_data.append(tuple([desc_name, desc_name, desc_name]))
            
        return out_data
            
    name: bpy.props.StringProperty(name="Test Prop", default="Layer", update=on_blend_desc_property_update)
    alpha_to_coverage: bpy.props.BoolProperty(name="Use alpha to coverage", update=on_blend_desc_property_update)
    independe_blend_enable: bpy.props.BoolProperty(name="Independent blend enabled", update=on_blend_desc_property_update)
    rt0_blend_desc_name: bpy.props.EnumProperty(name="RT0 blend desc name", default=0, items=on_blend_desc_set_rt_desc_names, update=on_blend_desc_property_update)
    rt1_blend_desc_name: bpy.props.StringProperty(name="RT1 blend desc name", update=on_blend_desc_property_update)
    rt2_blend_desc_name: bpy.props.StringProperty(name="RT2 blend desc name", update=on_blend_desc_property_update)
    rt3_blend_desc_name: bpy.props.StringProperty(name="RT3 blend desc name", update=on_blend_desc_property_update)
    rt4_blend_desc_name: bpy.props.StringProperty(name="RT4 blend desc name", update=on_blend_desc_property_update)
    rt5_blend_desc_name: bpy.props.StringProperty(name="RT5 blend desc name", update=on_blend_desc_property_update)
    rt6_blend_desc_name: bpy.props.StringProperty(name="RT6 blend desc name", update=on_blend_desc_property_update)
    rt7_blend_desc_name: bpy.props.StringProperty(name="RT7 blend desc name", update=on_blend_desc_property_update)
    
    updates_enabled: bpy.props.BoolProperty(name="Update enabled", default=False)

bpy.utils.register_class(property_multilight_scene_layers)
bpy.types.Scene.multilight_scene_layers = bpy.props.CollectionProperty(type=property_multilight_scene_layers)

class multilight_properties(bpy.types.PropertyGroup):
    print("Props called")
    bpy.types.Scene.multilight_scene_layers_index = bpy.props.IntProperty(\
    name = "Layer Scene Index",\
    description = "---",\
    default = 0,\
    min = 0\
    )

class multilight_scene_layers_UL(bpy.types.UIList):
    def draw_item(self, context, layout, data, item, icon, active_data, active_propname, index):
        layout.label(text=str(item.name))
        layout.label(text=str(item.alpha_to_coverage))
        layout.label(text=str(item.independe_blend_enable))
        layout.label(text=str(item.rt0_blend_desc_name))

class multilight_scene_layers_UIListPanel(bpy.types.Panel):
    bl_label = "Multilight Scene Layers"
    bl_idname = "OBJECT_PT_ui_list_example"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "scene"

    def draw(self, context):
        layout = self.layout

        row = layout.row()
        row.label(text="Desc name")
        row.label(text="alpha to ceverage")
        row.label(text="indep. blend en.")
        row.label(text="rt0 blend desc name")
        
        layout.template_list("multilight_scene_layers_UL", "j", \
        context.scene, "multilight_scene_layers", context.scene, \
        "multilight_scene_layers_index", type='DEFAULT')

        layout.prop(context.scene, "multilight_scene_layers_index")
        layout.operator("scene.add_blend_desc")
        layout.operator("scene.remove_blend_desc")
        num_of_els = len(context.scene.multilight_scene_layers)
        if num_of_els > 0:
            context.scene.multilight_scene_layers_index = min(context.scene.multilight_scene_layers_index, num_of_els - 1)
            item = context.scene.multilight_scene_layers[context.scene.multilight_scene_layers_index]
            layout.prop(item, "name")
            layout.prop(item, "alpha_to_coverage")
            layout.prop(item, "independe_blend_enable")
            layout.prop(item, "rt0_blend_desc_name")
            layout.prop(item, "rt1_blend_desc_name")
            layout.prop(item, "rt2_blend_desc_name")
            layout.prop(item, "rt3_blend_desc_name")
            layout.prop(item, "rt4_blend_desc_name")
            layout.prop(item, "rt5_blend_desc_name")
            layout.prop(item, "rt6_blend_desc_name")
            layout.prop(item, "rt7_blend_desc_name")

class AddBlendDesc(bpy.types.Operator):
    """Tooltip"""
    bl_idname = "scene.add_blend_desc"
    bl_label = "Add new blend desc"
    
    def execute(self, context):
        my_item = bpy.context.scene.multilight_scene_layers.add()
        my_item.name = "New desc"
        my_item.alpha_to_coverage = False
        my_item.independe_blend_enable = False
        return {'FINISHED'}
    
class RemoveBlendDesc(bpy.types.Operator):
    """Tooltip"""
    bl_idname = "scene.remove_blend_desc"
    bl_label = "Remove selected blend desc"
    
    def execute(self, context):
        selected_index = bpy.context.scene.multilight_scene_layers_index
        #print("selected_index:", selected_index)
        #item_to_remove = bpy.types.Scene.multilight_scene_layers[selected_index]
        #remove_index = bpy.context.scene.multilight_scene_layers.find(item_to_remove)
        #print("prop_layer:", bpy.types.Scene.multilight_scene_layers)
        bpy.context.scene.multilight_scene_layers.remove(selected_index)
        return {'FINISHED'}

def init_blend_desc_props():
    
    if hasattr(bpy.context.scene, 'multilight_scene_layers'):
        bpy.context.scene.multilight_scene_layers.clear()
        bpy.context.scene.multilight_scene_layers_index = 0
    
        global root_dir
        bd_path = root_dir + "/database/bsidedb.sqlite"
        blend_descs = asyncio.run(uacedb.Desc(uacedb.Desc.TYPE_BLEND_DESC, bd_path).read_all())
        #print(blend_descs)
        
        for desc in blend_descs:
            new_desc = bpy.context.scene.multilight_scene_layers.add()
            new_desc.name = desc.name
            new_desc.alpha_to_coverage = desc.alpha_to_coverage
            new_desc.independe_blend_enable = desc.independe_blend_enable
            new_desc.rt0_blend_desc_name = desc.rt0_blend_desc_name if desc.rt0_blend_desc_name != "" else " "
            new_desc.rt1_blend_desc_name = desc.rt1_blend_desc_name
            new_desc.rt2_blend_desc_name = desc.rt2_blend_desc_name
            new_desc.rt3_blend_desc_name = desc.rt3_blend_desc_name
            new_desc.rt4_blend_desc_name = desc.rt4_blend_desc_name
            new_desc.rt5_blend_desc_name = desc.rt5_blend_desc_name
            new_desc.rt6_blend_desc_name = desc.rt6_blend_desc_name
            new_desc.rt7_blend_desc_name = desc.rt7_blend_desc_name
            new_desc.updates_enabled = True
    
def init_rt_blend_descs():
    bpy.context.scene.render_target_blend_descs.clear()
    bpy.context.scene.render_target_blend_desc_index = 0
    global root_dir
    bd_path = root_dir + "/database/bsidedb.sqlite"
    
    rt_blend_descs = asyncio.run(uacedb.Desc(uacedb.Desc.TYPE_RT_BLEND_DESC, bd_path).read_all())
    
    for desc in rt_blend_descs:
        new_desc = bpy.context.scene.render_target_blend_descs.add()
        new_desc.name = desc.name
        new_desc.blend_enable = desc.blend_enable
        new_desc.logic_op_enable = desc.logic_op_enable
        new_desc.src_blend = desc.src_blend
        new_desc.dest_blend = desc.dest_blend
        new_desc.blend_op = desc.blend_op
        new_desc.src_blend_alpha = desc.src_blend_alpha
        new_desc.dest_blend_alpha = desc.dest_blend_alpha
        new_desc.blend_op_alpha = desc.blend_op_alpha
        new_desc.logic_op = desc.logic_op
        new_desc.render_target_write_mask = desc.render_target_write_mask
        new_desc.updates_enabled = True

def register():
    #pass
    bpy.utils.register_class(UACE_UL_rt_blend_descs)
    bpy.utils.register_class(UACE_PT_rt_blend_descs)
    
    bpy.utils.register_class(RemoveBlendDesc)
    bpy.utils.register_class(AddBlendDesc)
    bpy.utils.register_class(multilight_scene_layers_UL)
    bpy.utils.register_class(multilight_scene_layers_UIListPanel)
    
    init_rt_blend_descs()
    init_blend_desc_props()

def unregister():
    #pass
    bpy.utils.unregister_class(UACE_UL_rt_blend_descs)
    bpy.utils.unregister_class(UACE_PT_rt_blend_descs)
    
    bpy.utils.unregister_class(RemoveBlendDesc)
    bpy.utils.unregister_class(AddBlendDesc)
    bpy.utils.unregister_class(multilight_scene_layers_UL)
    bpy.utils.unregister_class(multilight_scene_layers_UIListPanel)

if __name__ == "__main__":
    register()
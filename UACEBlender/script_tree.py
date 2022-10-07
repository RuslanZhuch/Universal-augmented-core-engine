
import bpy
import copy
import itertools
from bpy.types import NodeTree, NodeInternal, NodeSocketStandard, Operator
from bpy_extras.io_utils import ImportHelper
from bpy.props import StringProperty, BoolProperty
# Implementation of custom nodes from Python

import os

import sys

import json
import sqlite3

root_dir = os.path.dirname(bpy.data.filepath)
libs_dir = root_dir + "\lib"
print(libs_dir)
sys.path.append(libs_dir)

# Derived from the NodeTree base type, similar to Menu, Operator, Panel, etc.
class UACEScriptTree(NodeTree):
    bl_label = "UACE Scripts"
    bl_icon = 'NODETREE'

class UACEScriptSocketDataBlock(NodeSocketStandard):
    bl_label = "Data block"

    def draw(self, context, layout, node, text):
        layout.label(text=text)

    def draw_color(self, context, node):
        return (0.18, 0.702, 0.702, 1.0)
    
class UACEScriptSocketInputDataBlock(NodeSocketStandard):
    bl_label = "Data block"
    def draw(self, context, layout, node, text):
        display_text = "Data: "
        if self.is_linked:
            if len(self.links) > 0:
                link = self.links[0]
                src_node = link.from_node
                display_text += src_node.getDataAlias()
            
        layout.label(text=display_text)

    def draw_color(self, context, node):
        return (0.18, 0.702, 0.702, 1.0)
    
class UACEScriptSocketOutput(NodeSocketStandard):
    bl_label = "Output data"

    def draw(self, context, layout, node, text):
        layout.label(text=text)

    def draw_color(self, context, node):
        return (0.2, 0.2, 1.0, 1.0)

class UACEShaderNameSocket(NodeSocketStandard):
    bl_idname = 'UACEShaderNameSocket'
    bl_label = "Shader name socket"
    
    my_enum_prop = None
    
    def call_update(self, context):
        print("call_update", self)
        self.node.on_socket_update(context, self.my_enum_prop)
    
    def draw(self, context, layout, node, text):
        if self.is_output or self.is_linked:
            layout.label(text=text)
        else:
            layout.prop(self, "my_enum_prop", text=text)

    def draw_color(self, context, node):
        return (0.2, 0.2, 1.0, 1.0)

class UACEShaderSocket(NodeSocketStandard):
    bl_idname = 'UACEShaderSocket'
    bl_label = "Shader socket"
    
    def draw(self, context, layout, node, text):
        layout.label(text=text)

    def draw_color(self, context, node):
        return (1.0, 0.8, 0.8, 1.0)

class UACEScriptNodeConfig:
    @classmethod
    def poll(cls, ntree):
        return ntree.bl_idname == 'UACEScriptTree'

class UACECppNode(NodeInternal):
    bl_label = "C++ node"
    
    script_filename_prop: bpy.props.StringProperty(name="File", default="None")
    
    inputs_state = []
    outputs_state = []
    
    @classmethod
    def poll(cls, ntree):
        print(ntree.bl_idname)
        return ntree.bl_idname == 'UACEScriptTree'
    
    def init(self, context):
        self.inputs.new('UACEScriptSocketInputDataBlock', "Input")
        self.outputs.new('UACEScriptSocketDataBlock', "Output")
        
        self.inputs_state = [input.is_linked for input in self.inputs]
        self.outputs_state = [output.is_linked for output in self.outputs]

    def copy(self, node):
        pass
    
    def _update_sockets(self, cur_sockets, sockets_state, socket_type):

        new_sockets_state = [socket.is_linked for socket in cur_sockets]
        
        if sockets_state != new_sockets_state:
            num_of_new_sockets = len(new_sockets_state) - len(sockets_state)
            sockets_state = [*sockets_state, *[False for _ in range(num_of_new_sockets)]]

            sockets_to_remove = []
            for id, is_linked in enumerate(new_sockets_state):
                if not is_linked:
                    sockets_to_remove.append(id)
            
            num_of_removed_sockets = 0
            for socket_id in reversed(sockets_to_remove):
                if socket_id == len(sockets_state) - 1:
                    continue
                num_of_new_sockets += 1
                cur_sockets.remove(cur_sockets[socket_id])
            
            if new_sockets_state[-1] is True:
                cur_sockets.new(socket_type, "Data")
                new_sockets_state.append(False)
            
            sockets_state = copy.deepcopy(new_sockets_state)
    
    def update(self):
        
        self._update_sockets(self.inputs, self.inputs_state, 'UACEScriptSocketInputDataBlock')
        self._update_sockets(self.outputs, self.outputs_state, 'UACEScriptSocketDataBlock')
        return
        
        new_inputs_state = [input.is_linked for input in self.inputs]
        
        if self.inputs_state != new_inputs_state:
            print("new_state", new_inputs_state)
            print("old_state", self.inputs_state)
            num_of_new_inputs = len(new_inputs_state) - len(self.inputs_state)
            print("Len diff:", num_of_new_inputs)
            self.inputs_state = [*self.inputs_state, *[False for _ in range(num_of_new_inputs)]]

            print("inputs_state:", self.inputs_state)
            inputs_to_remove = []
            for id, is_linked in enumerate(new_inputs_state):
                if not is_linked:
                    inputs_to_remove.append(id)
            
            num_of_removed_inputs = 0
            for input_id in reversed(inputs_to_remove):
                if input_id == len(self.inputs_state) - 1:
                    continue
                num_of_removed_inputs += 1
                print("removed id", input_id)
                self.inputs.remove(self.inputs[input_id])
            
            if new_inputs_state[-1] is True:
                self.inputs.new('UACEScriptSocketInputDataBlock', "Input")
                new_inputs_state.append(False)
            
            self.inputs_state = copy.deepcopy(new_inputs_state)
            
    def update_sockets(self, shader_name):
        print("sn:", shader_name)
        if not shader_name:
            return
        
        global root_dir
        bd_path = root_dir + "/database/bsidedb.sqlite"
        conn = sqlite3.connect(bd_path)
        cursor = conn.cursor()
        
        ret = cursor.execute("SELECT * FROM ShaderData WHERE ShaderName=?", (shader_name, ))
        shaderData = ret.fetchone()
        
        print(shaderData)
        print(shaderData[0])
        print(shaderData[1])
        print(shaderData[2])
        
        if shaderData[2] == None:
            return
        
        jData = json.loads(str.encode(shaderData[2]))
        num_of_inputs = jData["NumOfInputs"]
        print(jData["NumOfInputs"])
        for inp_id in range(0, num_of_inputs):
            inpStr = "Input" + str(inp_id)
            sinput = jData[inpStr]
            self.inputs.new('NodeSocketString', sinput["Name"] + " (" + sinput["Type"] + ")")

    def on_socket_update(self, context, shader_name):
        while len(self.inputs) > 1:
            imp = self.inputs[len(self.inputs) - 1]
            self.inputs.remove(imp)
        
        self.update_sockets(shader_name)

    # Free function to clean up on removal.
    def free(self):
        pass

    def draw_buttons(self, context, layout):
        layout.label(text="Node settings")
        layout.prop(self, "script_filename_prop")

    def draw_buttons_ext(self, context, layout):
        pass


class UACEScriptInputDataNode(NodeInternal):
    bl_label = "Input data"
    
    #data_block_name: bpy.props.StringProperty(name="data block", default="None")
    data_alias: bpy.props.StringProperty(name="DataAlias", default="DataAlias")

    defaultItems = (
    ('DEFAULT_DATA_BLOCK', "Default", "Temporary default data"),
    )
    data_block_enum: bpy.props.EnumProperty(
    name="data",
    description="Data block source to read from",
    items = defaultItems,
    #items = upd,
    #update=sck_upd
    )
    
    def getDataAlias(self):
        return self.data_alias
    
    @classmethod
    def poll(cls, ntree):
        print(ntree.bl_idname)
        return ntree.bl_idname == 'UACEScriptTree'
    
    def init(self, context):
        self.outputs.new('UACEScriptSocketDataBlock', "Output")

    def copy(self, node):
        pass

    # Free function to clean up on removal.
    def free(self):
        pass

    def draw_buttons(self, context, layout):
        layout.prop(self, "data_alias")
        layout.prop(self, "data_block_enum")

    def draw_buttons_ext(self, context, layout):
        pass


class UACEScriptOutputDataNode(NodeInternal):
    bl_label = "Output data"
    
    @classmethod
    def poll(cls, ntree):
        print(ntree.bl_idname)
        return ntree.bl_idname == 'UACEScriptTree'
    
    def init(self, context):
        self.inputs.new('UACEScriptSocketDataBlock', "Output")

    def copy(self, node):
        pass

    # Free function to clean up on removal.
    def free(self):
        pass

    def draw_buttons(self, context, layout):
        pass

    def draw_buttons_ext(self, context, layout):
        pass

#class UACEConfigNode(Node, UACERenderConfigNode):
#    bl_idname = 'UACEConfigNode'
#    bl_label = "Config Node"
#    def init(self, context):
#        self.inputs.new('UACEShaderSocket', "Vertex shader")
#        self.inputs.new('UACEShaderSocket', "Geometry shader")
#        self.inputs.new('UACEShaderSocket', "Pixel shader")
#        self.inputs.new('UACEShaderSocket', "Compute shader")
#
#        #self.outputs.new('UACETextureResourceSocket', "out")
#
#    def copy(self, node):
#        pass
#
#    def free(self):
#        pass
#
#    def draw_buttons(self, context, layout):
#        pass
#
#    def draw_buttons_ext(self, context, layout):
#        pass
#
#    #def draw_label(self):
#    #    return "Texture resource"
#    
#    #def insert_link(self, link):
#    #    print("Link!")

### Node Categories ###
# Node categories are a python system for automatically
# extending the Add menu, toolbar panels and search operator.
# For more examples see release/scripts/startup/nodeitems_builtins.py

import nodeitems_utils
from nodeitems_utils import NodeCategory, NodeItem

# our own base class with an appropriate poll function,
# so the categories only show up in our own tree type


class ScriptsConfigNodeCategory(NodeCategory):
    @classmethod
    def poll(cls, context):
        return context.space_data.tree_type == 'UACEScriptTree'

node_categories = [
    # identifier, label, items list
    ScriptsConfigNodeCategory('UACE_SCRIPTS_CONFIG_CATEGORY', "ScriptsConfig", items=[
        # our basic node
        NodeItem("UACECppNode"),
        NodeItem("UACEScriptInputDataNode"),
        NodeItem("UACEScriptOutputDataNode"),
    ]),
]

#classes = (
#    UACERenderConfigNodes,
#    UACETextureResourceSocket,
#    UACETextureDescSocket,
#    UACEShaderNameSocket,
#    UACEShaderSocket,
#    UACEShaderNode,
#    UACEConfigNode,
#)

def sck_upd(_self, context):
    print("sck_upd", _self)
    _self.call_update(context)

my_items = []
def upd(_self, context):
    global my_items
    try:
        param = bpy.types.Scene.UACE_param_shaders
    
        #param_data = json.loads(param)
        
        #if param_data["NeedReload"] == False:
        #    return my_items
        
        if _self.node.name in param:
            return my_items
        
    except:
        return my_items
    
    global root_dir
    bd_path = root_dir + "/database/bsidedb.sqlite"
    conn = sqlite3.connect(bd_path)
    cursor = conn.cursor()
    
    ret = cursor.execute("SELECT ShaderName FROM ShaderData")
    
    my_items = tuple()
    
    names = ret.fetchall()
    
    conn.close()
    for shader_name_tup in names:
        shader_name = shader_name_tup[0]
        print(shader_name)
        my_items = my_items + tuple([tuple([shader_name, shader_name, shader_name])])

    print("Items:",my_items)
    #bpy.types.Scene.UACE_param_shaders = json.dumps({"NeedReload": False})
    bpy.types.Scene.UACE_param_shaders.append(_self.node.name)
    _self.call_update(context)
    print('Updated', _self)
    return my_items

class StopCallback(bpy.types.Operator):
    """Tooltip"""
    bl_idname = "scene.stop_callback"
    bl_label = "Stop Callback"
    
    @classmethod
    def poll(cls, context):
        return True

    def execute(self, context):
        from bpy.utils import unregister_class
        unregister_class(UACEScriptTree)
        unregister_class(UACEScriptSocketDataBlock)
        unregister_class(UACEScriptSocketInputDataBlock)
        unregister_class(UACEScriptOutputDataNode)
        unregister_class(UACECppNode)
        unregister_class(UACEScriptInputDataNode)
        
        nodeitems_utils.unregister_node_categories('UACE_SCRIPTS_CONFIG_CATEGORY')
        return {'FINISHED'}


class StartCallback(bpy.types.Operator):
    """Tooltip"""
    bl_idname = "scene.start_callback"
    bl_label = "Start Callback"
    
    global tracking_objects_list

    @classmethod
    def poll(cls, context):
        return True

    def execute(self, context):
        from bpy.utils import register_class
        register_class(UACEScriptTree)
        register_class(UACEScriptSocketDataBlock)
        register_class(UACEScriptSocketInputDataBlock)
        register_class(UACEScriptOutputDataNode)
        register_class(UACECppNode)
        register_class(UACEScriptInputDataNode)
        
        nodeitems_utils.register_node_categories('UACE_SCRIPTS_CONFIG_CATEGORY', node_categories)
            
        return {'FINISHED'}

class SceneTestPanel(bpy.types.Panel):
    """
    Creates a Panel in the Scene properties window
    """
    bl_label = "Scene Test Panel"
    bl_idname = "OBJECT_PT_scene_test"
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "scene"

    def draw(self, context):
        layout = self.layout

        scene = context.scene

        row = layout.row()

        row = layout.row()
        row.operator("scene.start_callback")

        row = layout.row()
        row.operator("scene.stop_callback")

def register():
    bpy.utils.register_class(SceneTestPanel)
    bpy.utils.register_class(StartCallback)
    bpy.utils.register_class(StopCallback)
#    from bpy.utils import register_class
#    for cls in classes:
#        register_class(cls)
#
#    nodeitems_utils.register_node_categories('UACE_RENDER_CONFIG_CATEGORY', node_categories)
#
#    
#    UACEShaderNameSocket.my_enum_prop = bpy.props.EnumProperty(
#        name="Direction",
#        description="Just an example",
#        items = upd,
#        update=sck_upd
#    )

def unregister():  
    bpy.utils.unregister_class(SceneTestPanel)
    bpy.utils.unregister_class(StartCallback)
    bpy.utils.unregister_class(StopCallback)
#    try:
#        nodeitems_utils.unregister_node_categories('UACE_RENDER_CONFIG_CATEGORY')
#        from bpy.utils import unregister_class
#        for cls in reversed(classes):
#            unregister_class(cls)
#    except:
#        pass


if __name__ == "__main__":
    register()

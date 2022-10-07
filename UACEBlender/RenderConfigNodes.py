
import bpy
from bpy.types import NodeTree, Node, NodeSocket, Operator
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
class UACERenderConfigNodes(NodeTree):
    # Description string
    '''A custom node tree type that will show up in the editor type list'''
    # Optional identifier string. If not explicitly defined, the python class name is used.
    bl_idname = 'UACERenderConfigTreeType'
    # Label for nice name display
    bl_label = "UACE Render config nodes"
    # Icon identifier
    bl_icon = 'NODETREE'

class UACETextureResourceSocket(NodeSocket):
    # Description string
    '''Custom node socket type'''
    # Optional identifier string. If not explicitly defined, the python class name is used.
    bl_idname = 'UACETextureResourceSocket'
    # Label for nice name display
    bl_label = "Texture resource socket"

    # Optional function for drawing the socket input value
    def draw(self, context, layout, node, text):
        layout.label(text=text)

    # Socket color
    def draw_color(self, context, node):
        return (0.4, 0.4, 1.0, 1.0)
    
class UACETextureDescSocket(NodeSocket):
    # Description string
    '''Custom node socket type'''
    # Optional identifier string. If not explicitly defined, the python class name is used.
    bl_idname = 'UACETextureDescSocket'
    # Label for nice name display
    bl_label = "Texture desc socket"

    # Optional function for drawing the socket input value
    def draw(self, context, layout, node, text):
        layout.label(text=text)

    # Socket color
    def draw_color(self, context, node):
        return (0.2, 0.2, 1.0, 1.0)

class UACEShaderNameSocket(NodeSocket):
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

class UACEShaderSocket(NodeSocket):
    bl_idname = 'UACEShaderSocket'
    bl_label = "Shader socket"
    
    def draw(self, context, layout, node, text):
        layout.label(text=text)

    def draw_color(self, context, node):
        return (1.0, 0.8, 0.8, 1.0)

# Mix-in class for all custom nodes in this tree type.
# Defines a poll function to enable instantiation.
class UACERenderConfigNode:
    @classmethod
    def poll(cls, ntree):
        return ntree.bl_idname == 'UACERenderConfigTreeType'

class UACEShaderNode(Node, UACERenderConfigNode):
    bl_idname = 'UACEShaderNode'
    bl_label = "Shader Node"
    
    def init(self, context):
        self.inputs.new('UACEShaderNameSocket', "Filename")
        self.outputs.new('UACEShaderSocket', "shader")

    def copy(self, node):
        pass

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
        print(self, "Update triggered")
        while len(self.inputs) > 1:
            imp = self.inputs[len(self.inputs) - 1]
            self.inputs.remove(imp)
        
        self.update_sockets(shader_name)

    # Free function to clean up on removal.
    def free(self):
        pass

    def draw_buttons(self, context, layout):
        pass

    def draw_buttons_ext(self, context, layout):
        pass

class UACEConfigNode(Node, UACERenderConfigNode):
    bl_idname = 'UACEConfigNode'
    bl_label = "Config Node"
    def init(self, context):
        self.inputs.new('UACEShaderSocket', "Vertex shader")
        self.inputs.new('UACEShaderSocket', "Geometry shader")
        self.inputs.new('UACEShaderSocket', "Pixel shader")
        self.inputs.new('UACEShaderSocket', "Compute shader")

        #self.outputs.new('UACETextureResourceSocket', "out")

    def copy(self, node):
        pass

    def free(self):
        pass

    def draw_buttons(self, context, layout):
        pass

    def draw_buttons_ext(self, context, layout):
        pass

    #def draw_label(self):
    #    return "Texture resource"
    
    #def insert_link(self, link):
    #    print("Link!")

### Node Categories ###
# Node categories are a python system for automatically
# extending the Add menu, toolbar panels and search operator.
# For more examples see release/scripts/startup/nodeitems_builtins.py

import nodeitems_utils
from nodeitems_utils import NodeCategory, NodeItem

# our own base class with an appropriate poll function,
# so the categories only show up in our own tree type


class RenderConfigNodeCategory(NodeCategory):
    @classmethod
    def poll(cls, context):
        return context.space_data.tree_type == 'UACERenderConfigTreeType'


# all categories in a list
node_categories = [
    # identifier, label, items list
    RenderConfigNodeCategory('UACE_RENDER_CONFIG_CATEGORY', "RenderConfig", items=[
        # our basic node
        NodeItem("UACEShaderNode"),
        NodeItem("UACEConfigNode"),
    ]),
]

classes = (
    UACERenderConfigNodes,
    UACETextureResourceSocket,
    UACETextureDescSocket,
    UACEShaderNameSocket,
    UACEShaderSocket,
    UACEShaderNode,
    UACEConfigNode,
)

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

def register():
    from bpy.utils import register_class
    for cls in classes:
        register_class(cls)

    nodeitems_utils.register_node_categories('UACE_RENDER_CONFIG_CATEGORY', node_categories)

    
    UACEShaderNameSocket.my_enum_prop = bpy.props.EnumProperty(
        name="Direction",
        description="Just an example",
        items = upd,
        update=sck_upd
    )


def unregister():  
    try:
        nodeitems_utils.unregister_node_categories('UACE_RENDER_CONFIG_CATEGORY')
        from bpy.utils import unregister_class
        for cls in reversed(classes):
            unregister_class(cls)
    except:
        pass


if __name__ == "__main__":
    unregister()
    register()

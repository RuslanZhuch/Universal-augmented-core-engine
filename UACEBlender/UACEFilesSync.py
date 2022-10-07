import bpy
import json

import sqlite3
#import asyncio
import os
import sys
root_dir = os.path.dirname(bpy.data.filepath)
libs_dir = root_dir + "\lib"
print(libs_dir)
sys.path.append(libs_dir)

import bser

class SimpleOperator(bpy.types.Operator):
    """Tooltip"""
    bl_idname = "object.simple_operator"
    bl_label = "Simple Object Operator"

    _timer = None
    
    _last_shader_devid = None

    @classmethod
    def poll(cls, context):
        return context.active_object is not None

    def cancel(self, context):
        wm = context.window_manager
        wm.event_timer_remove(self._timer)

    def modal(self, context, event):
        if event.type in 'ESC':
            self.cancel(context)
            print('worker closed')
            return {'CANCELLED'}
        
        if event.type == 'TIMER':
            print("Timer")
            
            global root_dir
            bd_path = root_dir + "/database/bsidedb.sqlite"
            conn = sqlite3.connect(bd_path)
            cursor = conn.cursor()
            
            ret = cursor.execute("SELECT Last_shader_deviation_ID FROM DeviationData")
            [lsd] = ret.fetchone()
            
            conn.close()
            
            print(lsd)
            if (self._last_shader_devid == None) or \
            self._last_shader_devid < lsd:
                print("Shaders need to sync with: ", lsd)
                self._last_shader_devid = lsd
                bpy.types.Scene.UACE_param_shaders = []#json.dumps({"NeedReload": True})
            
        return {'PASS_THROUGH'}

    def execute(self, context):
        wm = context.window_manager
        self._timer = wm.event_timer_add(1, window=context.window)
        wm.modal_handler_add(self)
        return {'RUNNING_MODAL'}


def register():
    bpy.types.Scene.UACE_param_shaders = bpy.props.StringProperty()
    bpy.utils.register_class(SimpleOperator)


def unregister():
    bpy.utils.unregister_class(SimpleOperator)


if __name__ == "__main__":
    register()

    # test call
    bpy.ops.object.simple_operator()

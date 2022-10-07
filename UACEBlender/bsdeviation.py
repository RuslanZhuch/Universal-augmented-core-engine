import bpy
import copy
import json

import socket
import sys
import os

import threading

import sqlite3
import time
#import asyncio

root_dir = os.path.dirname(bpy.data.filepath)
libs_dir = root_dir + "\lib"
print(libs_dir)
sys.path.append(libs_dir)

import bser
import devfactory
import bsdb

import importlib
importlib.reload(devfactory)
importlib.reload(bsdb)
importlib.reload(bser)

#import uacebd

sock = None

use_tcp = True

deviation_id = 0
tracking_objects_list = []
g_object_in_edit_mode = None

qpool = devfactory.DevQueuePool(num_of_queues=2, wlimit=10, time_limit=1)

database_path = root_dir + "/database/bsidedb.sqlite"

class RenameData:
    def __init__(self, newName, prevName, bSwap=False):
        self.newName = newName
        self.prevName = prevName
        self.bSwap = bSwap

#def getObjectsDiff(curr_names, prev_names):
#    print("renamed:", renamed_object_names)
#    lens_diff = len(curr_names) - len(prev_names)
#    removed_names = []
#    temp_rename0 = []
#    for prev_name in prev_names:
#        if curr_names.count(prev_name) == 0:
#            if renamed_object_names.count(prev_name) == 0:
#                removed_names.append(prev_name)
#            else:
#                temp_rename0.append(prev_name)
#            
#    for tr in temp_rename0:
#        prev_names.remove(tr)
#
#    added_names = []
#    temp_rename1 = []
#    for curr_name in curr_names:
#        if prev_names.count(curr_name) == 0:
#            if renamed_object_names.count(curr_name) == 0:
#                added_names.append(curr_name)
#            else:
#                temp_rename1.append(curr_name)
#                
#    #for tr in temp_rename0:
#    #    curr_names.append(tr)
#                
#        
#    return added_names, removed_names

def TCP_send_data(sock, data):
    global use_tcp
    if use_tcp is True:
        data_len = len(data)
        header_bytes = b'\xFD\xFC\xFB\xFA'
        header_len = bser.pack_int(data_len)
        send_bytes = bytearray(header_bytes)
        send_bytes.extend(header_len)
        if isinstance(data, (bytes, bytearray)):
            #print(data)
            send_bytes.extend(data)
            sock.send(send_bytes)
            #sock.send(len(data).to_bytes(4, byteorder='big'))
        else:
            data = str.encode(data)
            send_bytes.extend(data)
            sock.send(send_bytes)
            print("Data sended: ", data)
        #print("Data sended: ", data)
    else:
        print("Data to send: ", data)

def JSON_encode_mat4x4(m):
    data = {"Mat4x4" : {\
    "a00" : m[0][0], "a01" : m[0][1],"a02" : m[0][2],"a03" : m[0][3], \
    "a10" : m[1][0], "a11" : m[1][1],"a12" : m[1][2],"a13" : m[1][3], \
    "a20" : m[2][0], "a21" : m[2][1],"a22" : m[2][2],"a23" : m[2][3], \
    "a30" : m[3][0], "a31" : m[3][1],"a32" : m[3][2],"a33" : m[3][3]}}
    return data

def create_obj_dev(obj):
    dev_header = devfactory.DevHeader(obj_name=obj.name, obj_type=obj.type, dev_type=devfactory.DevTypes.CREATE)
    print("Deviation detected: name={}, obj_type={}, dev_type={}, weight={}".format(\
            dev_header.obj_name, dev_header.obj_type, dev_header.dev_type, dev_header.weight))
            
    while qpool.append(dev_header) is False:
        print("Trying to append")
        time.sleep(0.5)
        

def delete_obj_dev(obj):
    dev_header = devfactory.DevHeader(obj_name=obj.name, obj_type=obj.type, dev_type=devfactory.DevTypes.DELETE)
    print("Deviation detected: name={}, obj_type={}, dev_type={}, weight={}".format(\
            dev_header.obj_name, dev_header.obj_type, dev_header.dev_type, dev_header.weight))
            
    while qpool.append(dev_header) is False:
        print("Trying to append")
        time.sleep(0.5)

def update_mesh_geom(obj):
    obj.data.calc_loop_triangles()
    dev_header = devfactory.DevHeader(obj_name=obj.name, obj_type=obj.type, dev_type=devfactory.DevTypes.PARAMETERS)     
    while qpool.append(dev_header) is False:
        print("Trying to append")
        time.sleep(0.5)
    

def cb_scene_update(context):
    global tracking_objects_list
    global g_object_in_edit_mode
        
    depsgraph = bpy.context.evaluated_depsgraph_get()
    for update in depsgraph.updates:
        if type(update.id.original) is not bpy.types.Object:
            #print("type {} is not an Object", type(update.id.original))
            continue
        
        updated_element_name = update.id.original.name
        
        if g_object_in_edit_mode is update.id.original:
            if update.id.original.mode != 'EDIT':
                g_object_in_edit_mode = None
                update_mesh_geom(update.id.original)
        
        if update.id.original.mode == 'EDIT':
            print("Switchet to the edit mode", updated_element_name)
            g_object_in_edit_mode = update.id.original
        
        if updated_element_name not in tracking_objects_list:
            print("Need create new object", updated_element_name, tracking_objects_list)
            tracking_objects_list.append(updated_element_name)
            if update.id.original.type == 'MESH':
                update.id.original.data.calc_loop_triangles()
            create_obj_dev(update.id.original)
        
        dev_type = devfactory.DevTypes.NONE
        if update.is_updated_geometry and update.id.original.mode == 'OBJECT':
            dev_type = devfactory.DevTypes.PARAMETERS
        if update.is_updated_transform:
            dev_type = devfactory.DevTypes.TRANSFORMATION

        dev_header = devfactory.DevHeader(obj_name=updated_element_name, obj_type=update.id.original.type, dev_type=dev_type)
        #print("Deviation detected: name={}, obj_type={}, dev_type={}, weight={}".format(\
        #    dev_header.obj_name, dev_header.obj_type, dev_header.dev_type, dev_header.weight))

        while qpool.append(dev_header) is False:
            print("Trying to append")
            time.sleep(0.5)

        

        #if selected_objects_names.count(updated_element_name):
        #    prevName = tracket_objects_pairs.get(update.id.original.as_pointer())
        #    if prevName is not None:
        #        bSame = prevName == updated_element_name
        #        if not bSame:
        #            print("need rename:", prevName, updated_element_name, update.id.original.as_pointer())
        #            renamed_object_names.append(prevName)
        #            renamed_object_names.append(updated_element_name)
        #            tracked_objects_list.append(updated_element_name)
        #            tracket_objects_pairs.update({update.id.original.as_pointer(): updated_element_name})
        #            swapObjId = bpy.data.objects.find(prevName)
        #            if swapObjId >= 0:
        #                swapObj = bpy.data.objects[swapObjId]
        #                print("need rename:", updated_element_name, prevName, swapObj.as_pointer())
        #                tracket_objects_pairs.update({swapObj.as_pointer(): prevName})
        #                objects_rename_to_send.append(RenameData(updated_element_name, prevName, bSwap=True))
        #            else:
        #                tracked_objects_list.remove(prevName)
        #                objects_rename_to_send.append(RenameData(updated_element_name, prevName))
        #            
        #            print("tracked objects after rename", tracket_objects_pairs)
        #                    
        #    elif tracked_objects_list.count(updated_element_name) == 0:
        #        print("Need create object", updated_element_name)
        #        tracked_objects_list.append(updated_element_name)
        #        objects_creation_to_send.append(updated_element_name)
        #        tracket_objects_pairs.update({update.id.original.as_pointer(): updated_element_name})
        #        
        #    print("Object to update", updated_element_name)
        #    if update.is_updated_geometry:
        #        if update.id.original.type == 'MESH':
        #         #and update.id.original.is_instancer is False:
        #            if objects_mesh_to_send.count(updated_element_name) == 0:
        #                objects_mesh_to_send.append(updated_element_name)
        #        if update.id.original.type == 'CAMERA':
        #            if cameras_names_to_update.count(updated_element_name) == 0:
        #                print("Camera has been added to update queue", updated_element_name)
        #                cameras_names_to_update.append(updated_element_name)
        #    if update.is_updated_transform:
        #        if objects_transform_to_send.count(updated_element_name) == 0:
        #            objects_transform_to_send.append(updated_element_name)
        
    #apply_changes(selected_objects_names)
            
            
def gather_create_camera(obj_name):
    try:
        return ""
    except:
        print("Failed to gather camera creation data of object:", obj_name)
        return None

def prepare_mesh_data(mesh):
    lt_id = 0
    out_buffer = []
    indices = []
    processed_vertices_id = set()
    out_buffer.append(int(0))
    num_of_vertices = 0
    sorted_loops = mesh.loops.values()
    sorted_loops.sort(key=lambda loop: (loop.vertex_index))
    for loop in sorted_loops:
        loop_id = loop.index
        vid = loop.vertex_index
        if vid in processed_vertices_id:
            continue
        num_of_vertices += 1
        processed_vertices_id.add(vid)
        #print("Vertex id:", vid)
        vertex = mesh.vertices[vid]
        vx = -vertex.co[0]
        vy = vertex.co[2]
        vz = -vertex.co[1]
        #print("Vertex:", vertex.co)
        out_buffer.append(vx)
        out_buffer.append(vy)
        out_buffer.append(vz)
        color = [float(0), float(0), float(0), float(1)]
        if len(mesh.color_attributes) > 0:
            vc = mesh.color_attributes[0].data[loop_id].color
            color[0] = vc[0]
            color[1] = vc[1]
            color[2] = vc[2]
        #print("Color:", color[0], color[1], color[2], color[3])
        out_buffer.append(color[0])
        out_buffer.append(color[1])
        out_buffer.append(color[2])
        out_buffer.append(color[3])
        pad = [float(0), float(1)]
        out_buffer.append(pad[0])
        out_buffer.append(pad[1])
    out_buffer[0] = num_of_vertices
    
    num_of_indices_id = len(out_buffer)
    out_buffer.append(int(0))
    
    num_of_indices = 0
    for polygon in mesh.loop_triangles:
        for loop_id in reversed(polygon.loops):
            num_of_indices += 1
            vid = mesh.loops[loop_id].vertex_index
            out_buffer.append(vid)
            #print("Index id: ", vid)
    out_buffer[num_of_indices_id] = num_of_indices
    
    print("num of vertices", out_buffer[0])
    print("num of indices", out_buffer[num_of_indices_id])

    return bser.ser(out_buffer)

def gather_create_static_mesh(obj_name):
    try:
        print("Creating static mesh")
        curr_object = bpy.data.objects[obj_name]
        mesh = curr_object.data
        return prepare_mesh_data(mesh)
    except Exception as e:
        print("Failed to gather static mesh creation data of object:", obj_name, e)
        return None
                     
def gather_delete(obj_name):
    try:
        return ""
    except:
        print("Failed to gather deletion data of object:", obj_name)
        return None
    
def gather_mesh_geometry(obj_name):
    try:
        curr_object = bpy.data.objects[obj_name]
        mesh = curr_object.data
        return prepare_mesh_data(mesh)
            
        '''polys = mesh.polygons
        pid = 0
        for poly in polys:
            print("Polygon:", pid)
            pid +=1
            for lid in poly.loop_indices:
                vid = mesh.loops[lid].vertex_index
                vertex = mesh.vertices[vid]
                print("Vertex:", vertex.co)
        '''        
        return ""
    except Exception as e:
        print("Failed to gather transform data of object:", obj_name, e)
        return None    
    
def gather_transform(obj_name):
    try:
        curr_object = bpy.data.objects[obj_name]
        return bser.ser([curr_object.matrix_world])
    except:
        print("Failed to gather transform data of object:", obj_name)
        return None
            
def generate_cam_data(camera_name):
    obj = bpy.data.objects[camera_name]
    loc = obj.location
    rot = obj.rotation_axis_angle
    cam_object = obj.data
    cam_type = 0 if cam_object.type == 'PERSP' else 1
    resolution_x = bpy.data.scenes["Scene"].render.resolution_x
    resolution_y = bpy.data.scenes["Scene"].render.resolution_y
    aspect_ratio = resolution_x / resolution_y

    blob_data = bser.ser([\
        float(loc[0]), float(loc[1]), float(loc[2]), \
        float(rot[0]), float(rot[1]), float(rot[2]), float(rot[3]), \
        chr(cam_type), \
        float(cam_object.ortho_scale), \
        float(cam_object.clip_start), float(cam_object.clip_end), \
        float(aspect_ratio), \
        float(cam_object.angle_x)])

    return blob_data
            
def gather_camera_data(camera_name):
    try:
        return generate_cam_data(camera_name)
    except:
        print("Failed to gather data of object", camera_name)
        return None

def deviation_processor(stop_event):
    global sock
    
    gatherer = devfactory.DeviationDataGatherer()
    gatherer.setup_resolve(resolve_id=devfactory.ResolveIds.CREATE_CAMERA, cb=gather_create_camera)
    gatherer.setup_resolve(resolve_id=devfactory.ResolveIds.CREATE_STATIC_MESH, cb=gather_create_static_mesh)
    gatherer.setup_resolve(resolve_id=devfactory.ResolveIds.DELETE, cb=gather_delete)
    gatherer.setup_resolve(resolve_id=devfactory.ResolveIds.CAMERA_UPDATE, cb=gather_camera_data)
    gatherer.setup_resolve(resolve_id=devfactory.ResolveIds.TRANSFORMATION, cb=gather_transform)
    gatherer.setup_resolve(resolve_id=devfactory.ResolveIds.GEOMETRY, cb=gather_mesh_geometry)

    while not stop_event.wait(0.2):
        print("Try to read new queue")
        read_dev_id = None
        while read_dev_id is None:
            if stop_event.wait(1):
                print("No queue")
                break

            read_dev_id = qpool.init_queue_read()
            
        
        print("Found queue with dev_id: ", read_dev_id)
            
        while not stop_event.wait(0.1):
            dev_header = qpool.read()
            if dev_header is None:
                print("dev_header is empty. Drop queue")
                break

            print("Ready to process deviation with dev_id={}, name={}, obj_type={}, dev_type={}".format(\
                read_dev_id, dev_header.obj_name, dev_header.obj_type, dev_header.dev_type))

            resolve_id = devfactory.resolve_header_id(dev_header=dev_header)
            print("Resolve id is:", resolve_id)

            dev_data = gatherer.get_dev_data(obj_name=dev_header.obj_name, resolve_id=resolve_id)
            if dev_data is None:
                print("Resolved deviation data is None. Skip.")
                continue

            if resolve_id == devfactory.ResolveIds.CAMERA_UPDATE:
                header_data = devfactory.gen_camera_update_pkg(object_name=dev_header.obj_name, deviation_id=read_dev_id)
                
            elif resolve_id == devfactory.ResolveIds.TRANSFORMATION:
                header_data = devfactory.gen_transform_pkg(object_name=dev_header.obj_name, \
                    object_type=dev_header.obj_type, deviation_id=read_dev_id)
                    
            elif resolve_id == devfactory.ResolveIds.GEOMETRY:
                header_data = devfactory.gen_geometry_pkg(object_name=dev_header.obj_name, \
                    deviation_id=read_dev_id)
                    
            elif (resolve_id == devfactory.ResolveIds.CREATE_CAMERA) or (resolve_id == devfactory.ResolveIds.CREATE_STATIC_MESH):
                header_data = devfactory.gen_obj_create_pkg(object_name=dev_header.obj_name, \
                    object_type=dev_header.obj_type, deviation_id=read_dev_id)
                    
            elif resolve_id == devfactory.ResolveIds.DELETE:
                header_data = devfactory.gen_obj_delete_pkg(object_name=dev_header.obj_name, \
                    object_type=dev_header.obj_type, deviation_id=read_dev_id)
                    
            else:
                print("Cannot generate header_data")
                continue

            #print("Sending header data is:", header_data)
            #print("Sending dev data is", dev_data)

            TCP_send_data(sock, json.dumps(header_data))
            TCP_send_data(sock, dev_data)

            bsdb.log_last_dev_id(database_path, read_dev_id)

    stop_event.clear()
    print("Closing deviation_processor thread")


stop_event = threading.Event()

class StopCallback(bpy.types.Operator):
    """Tooltip"""
    bl_idname = "scene.stop_callback"
    bl_label = "Stop Callback"
    
    @classmethod
    def poll(cls, context):
        return cb_scene_update in bpy.app.handlers.depsgraph_update_post

    def execute(self, context):
        global sock
        global use_tcp
        print('Removing callback ...')
        if use_tcp is True:
            sock.close()
        bpy.app.handlers.depsgraph_update_post.remove(cb_scene_update)
        stop_event.set()
        bpy.types.World.deviation_timer_need_stop = "y"
        print('DONE')
        return {'FINISHED'}


class StartCallback(bpy.types.Operator):
    """Tooltip"""
    bl_idname = "scene.start_callback"
    bl_label = "Start Callback"
    
    global tracking_objects_list

    @classmethod
    def poll(cls, context):
        return cb_scene_update not in bpy.app.handlers.depsgraph_update_post

    def execute(self, context):
        global sock
        global use_tcp
        global deviation_id
        global tracking_objects_list
        print('Adding callback ...')
        #prev_selected_names = []
        deviation_id = bsdb.load_last_dev_id(database_path) + 1
        qpool.init_queue_write(deviation_id)
        #tracked_objects_list = bpy.data.objects.keys()
        tracking_objects_list = bpy.data.objects.keys()
        print(tracking_objects_list)
        
        #for obj in bpy.data.objects:
        #    tracket_objects_pairs.update({obj.as_pointer(): obj.name})
        
        if use_tcp is True:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_address = ('localhost', 50007)
            print('connecting to {} port {}'.format(*server_address))
            sock.connect(server_address)
        
            jd = json.dumps({\
            "ClientType": "CBlender",\
            "ClientName": "NameBlender0",\
            "LastDeviationId": deviation_id,\
            })
            TCP_send_data(sock, jd)
        
        bpy.app.handlers.depsgraph_update_post.append(cb_scene_update)
        print('DONE')
        bpy.types.World.deviation_timer_need_stop = bpy.props.StringProperty()
        bpy.types.World.deviation_timer_need_stop = "n"
        bpy.ops.wm.modal_deviation_update_timer()
        dev_processor = threading.Thread(target=deviation_processor, args=(stop_event,))
        dev_processor.start()
        return {'FINISHED'}


class ModalDeviationUpdateTimer(bpy.types.Operator):
    """Operator which runs its self from a timer"""
    bl_idname = "wm.modal_deviation_update_timer"
    bl_label = "Modal Timer Operator"

    _timer = None

    def modal(self, context, event):
        global deviation_id
        if bpy.types.World.deviation_timer_need_stop == "y":
            self.cancel(context)
            return {'CANCELLED'}

        if event.type == 'TIMER':
            # change theme color, silly!
            #print("qpool update")
            if qpool.update():
                deviation_id = deviation_id+1
                while qpool.init_queue_write(deviation_id=deviation_id) is False:
                    print("Trying to init write")
                    time.sleep(0.5)

        return {'PASS_THROUGH'}

    def execute(self, context):
        wm = context.window_manager
        self._timer = wm.event_timer_add(1.0, window=context.window)
        wm.modal_handler_add(self)
        return {'RUNNING_MODAL'}

    def cancel(self, context):
        wm = context.window_manager
        wm.event_timer_remove(self._timer)
        print("Timer has been cancelled")

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

class delete_override(bpy.types.Operator):
    """delete objects and their derivatives"""
    bl_idname = "object.delete"
    bl_label = "Object Delete Operator"

    global tracking_objects_list
    
    @classmethod
    def poll(cls, context):
        return context.active_object is not None

    def execute(self, context):
        for obj in context.selected_objects:

            # replace with your function:
            #my_function(obj)
            print("Object deleted", obj.name)

            tracking_objects_list.remove(obj.name)
            delete_obj_dev(obj)
            bpy.data.objects.remove(obj)
        return {'FINISHED'}

def register():
    bpy.utils.register_class(SceneTestPanel)
    bpy.utils.register_class(StartCallback)
    bpy.utils.register_class(StopCallback)
    bpy.utils.register_class(ModalDeviationUpdateTimer)
    bpy.utils.register_class(delete_override)
    print('registered')

def unregister():
    bpy.utils.unregister_class(SceneTestPanel)
    bpy.utils.unregister_class(StartCallback)
    bpy.utils.unregister_class(StopCallback)
    bpy.utils.unregister_class(ModalDeviationUpdateTimer)
    bpy.utils.unregister_class(delete_override)


if __name__ == '__main__':
    register()
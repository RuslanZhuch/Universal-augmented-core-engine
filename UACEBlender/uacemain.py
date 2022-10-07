import bpy
import copy
import json

import socket
import sys
import os

import sqlite3
#import asyncio

root_dir = os.path.dirname(bpy.data.filepath)
libs_dir = root_dir + "\lib"
print(libs_dir)
sys.path.append(libs_dir)

import bser
#import uacebd

sock = None

prev_selected_names = []
    

objects_mesh_to_send = []
objects_transform_to_send = []
objects_creation_to_send = []
objects_rename_to_send = []
cameras_names_to_update = []

tracked_objects_list = []

tracket_objects_pairs = dict()
renamed_object_names = []

use_tcp = True

deviation_id = 0

class RenameData:
    def __init__(self, newName, prevName, bSwap=False):
        self.newName = newName
        self.prevName = prevName
        self.bSwap = bSwap

def getObjectsDiff(curr_names, prev_names):
    print("renamed:", renamed_object_names)
    lens_diff = len(curr_names) - len(prev_names)
    removed_names = []
    temp_rename0 = []
    for prev_name in prev_names:
        if curr_names.count(prev_name) == 0:
            if renamed_object_names.count(prev_name) == 0:
                removed_names.append(prev_name)
            else:
                temp_rename0.append(prev_name)
            
    for tr in temp_rename0:
        prev_names.remove(tr)

    added_names = []
    temp_rename1 = []
    for curr_name in curr_names:
        if prev_names.count(curr_name) == 0:
            if renamed_object_names.count(curr_name) == 0:
                added_names.append(curr_name)
            else:
                temp_rename1.append(curr_name)
                
    #for tr in temp_rename0:
    #    curr_names.append(tr)
                
        
    return added_names, removed_names

def TCP_send_data(sock, data):
    global use_tcp
    if use_tcp is True:
        data_len = len(data)
        header_bytes = b'\xFD\xFC\xFB\xFA'
        header_len = bser.pack_int(data_len)
        send_bytes = bytearray(header_bytes)
        send_bytes.extend(header_len)
        if isinstance(data, (bytes, bytearray)):
            print(data)
            send_bytes.extend(data)
            sock.send(send_bytes)
            #sock.send(len(data).to_bytes(4, byteorder='big'))
        else:
            data = str.encode(data)
            send_bytes.extend(data)
            sock.send(send_bytes)
        print("Data sended: ", data)
    else:
        print("Data to send: ", data)

def JSON_encode_mat4x4(m):
    data = {"Mat4x4" : {\
    "a00" : m[0][0], "a01" : m[0][1],"a02" : m[0][2],"a03" : m[0][3], \
    "a10" : m[1][0], "a11" : m[1][1],"a12" : m[1][2],"a13" : m[1][3], \
    "a20" : m[2][0], "a21" : m[2][1],"a22" : m[2][2],"a23" : m[2][3], \
    "a30" : m[3][0], "a31" : m[3][1],"a32" : m[3][2],"a33" : m[3][3]}}
    return data

def load_last_deviation_id():
    global root_dir
    bd_path = root_dir + "/database/bsidedb.sqlite"
    print(bd_path)
    conn = sqlite3.connect(bd_path)
    cursor = conn.cursor()
    cursor.execute("SELECT Last_deviation_ID FROM DeviationData")
    [dev_id] = cursor.fetchone()
    conn.close()
    print("loaded dev id:", dev_id)
    
    return dev_id

def store_deviation_id(dev_id):
    global root_dir
    bd_path = root_dir + "/database/bsidedb.sqlite"
    conn = sqlite3.connect(bd_path)
    cursor = conn.cursor()
    
    print("stored dev id:", dev_id)
    cursor.execute("UPDATE DeviationData SET Last_deviation_ID=? WHERE Last_deviation_ID=?", (dev_id,dev_id-1))
    conn.commit()
    conn.close()

def apply_changes(selected_objects_names):
    global prev_selected_names
    global prev_selected_names
    global sock
    global deviation_id
    global renamed_object_names
    
    
    added_names, removed_names = getObjectsDiff(selected_objects_names, prev_selected_names)
    renamed_object_names = []
    
    for renameData in objects_rename_to_send:
        devType = "SwapNames" if renameData.bSwap else "Rename"
        jd = json.dumps({\
        "PkgType": "Deviation",\
        "DeviationType": devType,\
        "ObjectName": renameData.prevName, \
        "DeviationId": deviation_id,\
        "ObjectType": "Mesh",\
        })    
        TCP_send_data(sock, jd)
        TCP_send_data(sock, renameData.newName)    
        
    objects_rename_to_send.clear()
    
    for removed_name in removed_names:
        #print("removing: ", removed_name)
        if bpy.data.objects.find(removed_name) == -1:
            
            print("Need removeObject", removed_name)
            
            objects = list(tracket_objects_pairs.values())
            keys = list(tracket_objects_pairs.keys())
            pos = objects.index(removed_name)
            k = keys[pos]
            tracket_objects_pairs.pop(k)
            print("pairs after removing", tracket_objects_pairs)
            
            tracked_objects_list.remove(removed_name)
            jd = json.dumps({\
            "PkgType": "Deviation",\
            "DeviationType": "Deletion",\
            "ObjectName": removed_name, \
            "DeviationId": deviation_id,\
            "ObjectType": "Mesh",\
            })
            TCP_send_data(sock, jd)
            TCP_send_data(sock, removed_name)
            if objects_transform_to_send.count(removed_name):
                objects_transform_to_send.remove(removed_name)
            if objects_mesh_to_send.count(removed_name):
                objects_mesh_to_send.remove(removed_name)
            if cameras_names_to_update.count(removed_name):
                cameras_names_to_update.remove(removed_name)
            deviation_id = deviation_id + 1
            continue
        curr_object = bpy.data.objects[removed_name]
        if curr_object.mode == 'OBJECT':
            need_send_creation = objects_creation_to_send.count(removed_name) != 0
            if need_send_creation:
                is_instance = curr_object.is_instancer
                inst_obj_name = ""
                if is_instance:
                    inst_obj_name = curr_object.instance_collection.name
                jd = json.dumps({\
                "PkgType": "Deviation",\
                "DeviationType": "Creation",\
                "ObjectName": removed_name, \
                "DeviationId": deviation_id,\
                "ObjectType": "Mesh"\
                })
                TCP_send_data(sock, jd)
                TCP_send_data(sock, inst_obj_name)
                objects_creation_to_send.remove(removed_name)
                deviation_id = deviation_id + 1
                
            need_update_camera = cameras_names_to_update.count(removed_name) != 0
            if need_update_camera is True:
                loc = curr_object.location
                rot = curr_object.rotation_axis_angle
                cam_object = curr_object.data
                cam_type = 0 if cam_object.type == 'PERSP' else 1
                resolution_x = bpy.data.scenes["Scene"].render.resolution_x
                resolution_y = bpy.data.scenes["Scene"].render.resolution_y
                aspect_ratio = resolution_x / resolution_y
                print(curr_object)
                print(cam_object)
                print(loc[0])
                print(loc[1])
                print(loc[2])
                print(rot[0])
                print(rot[1])
                print(rot[2])
                print(rot[3])
                blob_data = bser.ser([\
                    float(loc[0]), float(loc[1]), float(loc[2]), \
                    float(rot[0]), float(rot[1]), float(rot[2]), float(rot[3]), \
                    chr(cam_type), \
                    float(cam_object.ortho_scale), \
                    float(cam_object.clip_start), float(cam_object.clip_end), \
                    float(aspect_ratio), \
                    float(cam_object.angle_x)])
                    
                jd = json.dumps({\
                    "PkgType": "Deviation",\
                    "ObjectName" : removed_name,\
                    "DeviationType" : "Camera",\
                    "DeviationId" : deviation_id,
                    "ObjectType" : "Entity"\
                    })
                TCP_send_data(sock, jd)
                TCP_send_data(sock, blob_data)
                print("Camera data sended", removed_name)
                cameras_names_to_update.remove(removed_name)
                deviation_id = deviation_id + 1
                
            need_send_transform = objects_transform_to_send.count(removed_name) != 0
            if need_send_transform is True:
                
                jd = json.dumps({\
                "PkgType": "Deviation",\
                "DeviationType": "Transform",\
                "ObjectName": removed_name, \
                "DeviationId": deviation_id,\
                "ObjectType": "Mesh",\
                })
                TCP_send_data(sock, jd)
                dataToSend = bser.ser([curr_object.matrix_world])
                print(curr_object.matrix_world)
                TCP_send_data(sock, dataToSend)
                print("Transform data sended", removed_name)
                objects_transform_to_send.remove(removed_name)
                deviation_id = deviation_id + 1
                
            need_send_mesh = objects_mesh_to_send.count(removed_name) != 0
            if need_send_mesh is True:
                
                jd = json.dumps({\
                "PkgType": "Deviation",\
                "DeviationType": "Geometry",\
                "ObjectName": removed_name, \
                "DeviationId": deviation_id,\
                "ObjectType": "Mesh",\
                })
                
                TCP_send_data(sock, jd)
                mesh = curr_object.data
                
                vertices = mesh.vertices
                num_of_vertices = len(vertices)
                ver_coors = [v.co for v in vertices]
                vertices_ser = bser.ser(ver_coors)
                
                mesh.calc_loop_triangles()
                indices_ser = bytearray()
                tri_loops = mesh.loop_triangles
                num_of_tri_loops = len(tri_loops)
                for tri_loop in tri_loops:
                    tl_verts_indices = tri_loop.vertices
                    tri_indices_ser = bser.ser(tl_verts_indices)
                    indices_ser.extend(tri_indices_ser)
                
                geometry_ser = bser.ser([num_of_vertices, num_of_tri_loops])
                geometry_ser.extend(vertices_ser)
                geometry_ser.extend(indices_ser)
                
                TCP_send_data(sock, geometry_ser)
                
                print("Mesh data sended", removed_name)
                objects_mesh_to_send.remove(removed_name)
                deviation_id = deviation_id + 1
                
            store_deviation_id(deviation_id)
     
    prev_selected_names = copy.deepcopy(selected_objects_names)

def cb_scene_update(context):
    global tracked_objects_list
    global renamed_object_names
    selected_objects = bpy.context.selected_objects
    selected_objects_names = []
    
    for sel_obj in selected_objects:
        selected_objects_names.append(sel_obj.name)
        
    depsgraph = bpy.context.evaluated_depsgraph_get()
    for update in depsgraph.updates:
        updated_element_name = update.id.original.name
        print('updated', updated_element_name)
        if selected_objects_names.count(updated_element_name):
            prevName = tracket_objects_pairs.get(update.id.original.as_pointer())
            if prevName is not None:
                bSame = prevName == updated_element_name
                if not bSame:
                    print("need rename:", prevName, updated_element_name, update.id.original.as_pointer())
                    renamed_object_names.append(prevName)
                    renamed_object_names.append(updated_element_name)
                    tracked_objects_list.append(updated_element_name)
                    tracket_objects_pairs.update({update.id.original.as_pointer(): updated_element_name})
                    swapObjId = bpy.data.objects.find(prevName)
                    if swapObjId >= 0:
                        swapObj = bpy.data.objects[swapObjId]
                        print("need rename:", updated_element_name, prevName, swapObj.as_pointer())
                        tracket_objects_pairs.update({swapObj.as_pointer(): prevName})
                        objects_rename_to_send.append(RenameData(updated_element_name, prevName, bSwap=True))
                    else:
                        tracked_objects_list.remove(prevName)
                        objects_rename_to_send.append(RenameData(updated_element_name, prevName))
                    
                    print("tracked objects after rename", tracket_objects_pairs)
                            
            elif tracked_objects_list.count(updated_element_name) == 0:
                print("Need create object", updated_element_name)
                tracked_objects_list.append(updated_element_name)
                objects_creation_to_send.append(updated_element_name)
                tracket_objects_pairs.update({update.id.original.as_pointer(): updated_element_name})
                
            print("Object to update", updated_element_name)
            if update.is_updated_geometry:
                if update.id.original.type == 'MESH':
                 #and update.id.original.is_instancer is False:
                    if objects_mesh_to_send.count(updated_element_name) == 0:
                        objects_mesh_to_send.append(updated_element_name)
                if update.id.original.type == 'CAMERA':
                    if cameras_names_to_update.count(updated_element_name) == 0:
                        print("Camera has been added to update queue", updated_element_name)
                        cameras_names_to_update.append(updated_element_name)
            if update.is_updated_transform:
                if objects_transform_to_send.count(updated_element_name) == 0:
                    objects_transform_to_send.append(updated_element_name)
        
    apply_changes(selected_objects_names)
            

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
        print('DONE')
        return {'FINISHED'}


class StartCallback(bpy.types.Operator):
    """Tooltip"""
    bl_idname = "scene.start_callback"
    bl_label = "Start Callback"
    
    @classmethod
    def poll(cls, context):
        return cb_scene_update not in bpy.app.handlers.depsgraph_update_post

    def execute(self, context):
        global sock
        global use_tcp
        global deviation_id
        global tracked_objects_list
        print('Adding callback ...')
        prev_selected_names = []
        deviation_id = load_last_deviation_id()
        tracked_objects_list = bpy.data.objects.keys()
        
        for obj in bpy.data.objects:
            tracket_objects_pairs.update({obj.as_pointer(): obj.name})
        
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
    print('registered')

def unregister():
    bpy.utils.unregister_class(SceneTestPanel)
    bpy.utils.unregister_class(StartCallback)
    bpy.utils.unregister_class(StopCallback)


if __name__ == '__main__':
    register()
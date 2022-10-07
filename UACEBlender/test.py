import unittest
import json
from lib import bser
from lib import uacedb
from lib import devfactory
from lib import bsdb
from lib import scriptdb
from lib import cppgen

import sqlite3

import asyncio

from shutil import copyfile
from time import sleep

class TestDataCoder(unittest.TestCase):

    def test_float_coding(self):
        fval = 123.0
        serf = bser.pack_float(fval)
        f_deser = bser.unpack_float(serf)
        self.assertEqual(fval, f_deser)

    def test_int_coding(self):
        ival = 10000000
        seri = bser.pack_int(ival)
        i_deser = bser.unpack_int(seri)
        self.assertEqual(ival, i_deser)

    def test_mat4x4_coding(self):
        ser_none = bser.ser_mat4x4(None)
        self.assertTrue(ser_none is None)

        mat = [[0.0, 1.0, 2.0, 3.0], [10.0, 11.0, 12.0, 13.0], [20.0, 21.0, 22.0, 23.0], [30.0, 31.0, 32.0, 33.0]]
        ser_data = bser.ser_mat4x4(mat)
        self.assertFalse(ser_data is None)

        mat_des = bser.deser_mat4x4(ser_data)
        self.assertEqual(mat_des, mat)

    def test_vec3_coding(self):
        vec3 = [1.0, 2.0, 3.0]
        ser_data = bser.ser_vec3(vec3)
        self.assertIsNotNone(ser_data)

        vec_des = bser.deser_vec3(ser_data)
        self.assertEqual(vec_des, vec3)

    def test_stream_coding(self):
        mat = [[0.0, 1.0, 2.0, 3.0], [10.0, 11.0, 12.0, 13.0], [20.0, 21.0, 22.0, 23.0], [30.0, 31.0, 32.0, 33.0]]
        f0 = 34.0
        vec3 = [100.0, 101.0, 102.0]
        i0 = 10000000
        ser_blob = bser.ser([f0, mat, vec3, i0])

        f0_deser, mat_deser, v3_deser, i0_deser = bser.deser(ser_blob, [bser.TYPE_FLOAT, bser.TYPE_MAT4X4, bser.TYPE_VEC3, bser.TYPE_INT])

        self.assertEqual(f0_deser, f0)
        self.assertEqual(mat_deser, mat)
        self.assertEqual(v3_deser, vec3)
        self.assertEqual(i0_deser, i0)

class TestDatabase(unittest.TestCase):


    def __init__(self, *args, **kwargs):
        super(TestDatabase, self).__init__(*args, **kwargs)
        self.database_path = "database/test_Database.sqlite"
        self.database_path_bs = "database/test_bsidedb.sqlite"

        copyfile("database\Database.sqlite", self.database_path)
        copyfile("database/bsidedb.sqlite", self.database_path_bs)

    def test_clear_db(self):
        asyncio.run(uacedb.clear_db(self.database_path))

        conn = sqlite3.connect(self.database_path)
        c = conn.cursor()
        c.execute("SELECT* FROM ElementsToChange")
        self.assertEqual(len(c.fetchall()), 0)
        c.execute("SELECT* FROM ChangeDestinations")
        self.assertEqual(len(c.fetchall()), 0)

    def test_log_deviation_data(self):

        asyncio.run(uacedb.clear_db(self.database_path))

        jd = {\
        "PkgType": "Deviation",\
        "DeviationType": "Transform",\
        "ObjectName": "TestObjName0", \
        "DeviationId": 0,\
        "ObjectType": "Mesh",\
        }

        mat = [[0.0, 1.0, 2.0, 3.0], [10.0, 11.0, 12.0, 13.0], [20.0, 21.0, 22.0, 23.0], [30.0, 31.0, 32.0, 33.0]]
        blob_data = bser.ser([mat])
        
        asyncio.run(uacedb.log_new_deviation(self.database_path, jd, blob_data))

        [o_jd], [o_blob_data] = asyncio.run(uacedb.get_deviation_data(self.database_path, deviation_id=0))

        self.assertEqual(o_jd, jd)
        self.assertEqual(o_blob_data, blob_data)

        jd["DeviationId"] = 1
        jd["ObjectName"] = "TestObjName1"
        asyncio.run(uacedb.log_new_deviation(self.database_path, jd, blob_data))
        [o_jd], [o_blob_data] = asyncio.run(uacedb.get_deviation_data(self.database_path, deviation_id=1))
        self.assertEqual(o_jd, jd)
        self.assertEqual(o_blob_data, blob_data)

        asyncio.run(uacedb.remove_deviaion(self.database_path, deviation_id=0))
        o_jd, o_blob_data = asyncio.run(uacedb.get_deviation_data(self.database_path, deviation_id=0))
        self.assertEqual(o_jd, [])
        self.assertEqual(o_blob_data, [])

        [o_jd], [o_blob_data] = asyncio.run(uacedb.get_deviation_data(self.database_path, deviation_id=1))
        self.assertEqual(o_jd, jd)
        self.assertEqual(o_blob_data, blob_data)

        jd["DeviationId"] = 2
        asyncio.run(uacedb.log_new_deviation(self.database_path, jd, blob_data))
        o_jd, o_blob_data = asyncio.run(uacedb.get_deviation_data(self.database_path, deviation_id=1))
        self.assertEqual(o_jd, [])
        self.assertEqual(o_blob_data, [])
        
        jd["DeviationId"] = 3
        jd["ObjectName"] = "TestObjName2"
        asyncio.run(uacedb.log_new_deviation(self.database_path, jd, blob_data))
        o_jd, o_blob_data = asyncio.run(uacedb.get_deviation_data(self.database_path, deviation_id=1))
        self.assertEqual(o_jd, [])
        self.assertEqual(o_blob_data, [])
        jd["DeviationId"] = 4
        jd["ObjectName"] = "TestObjName3"
        asyncio.run(uacedb.log_new_deviation(self.database_path, jd, blob_data))
        o_jd, o_blob_data = asyncio.run(uacedb.get_deviation_data(self.database_path, deviation_id=1))
        self.assertEqual(o_jd, [])
        self.assertEqual(o_blob_data, [])

        nearest_dev_id_of3 = asyncio.run(uacedb.get_nearest_dev_id(self.database_path, curr_deviation_id=3))
        self.assertEqual(nearest_dev_id_of3, 4)

        nearest_dev_id_of5 = asyncio.run(uacedb.get_nearest_dev_id(self.database_path, curr_deviation_id=5))
        self.assertEqual(nearest_dev_id_of5, None)

    def test_blend_desc(self):
        asyncio.run(uacedb.clear_db(self.database_path_bs))

        blend_desc_db = uacedb.Desc(uacedb.Desc.TYPE_BLEND_DESC, self.database_path_bs)

        blend_desc_none = asyncio.run(blend_desc_db.read(desc_name="bdesc0"))
        self.assertIsNone(blend_desc_none)

        blend_desc_to_write = uacedb.Desc.BlendDesc(name="bdesc0")
        blend_desc_to_write.alpha_to_coverage = True
        blend_desc_to_write.independe_blend_enable = False
        blend_desc_to_write.rt0_blend_desc_name = "rt0bd"
        blend_desc_to_write.rt1_blend_desc_name = "rt1bd"
        blend_desc_to_write.rt2_blend_desc_name = "rt2bd"
        blend_desc_to_write.rt3_blend_desc_name = "rt3bd"
        blend_desc_to_write.rt4_blend_desc_name = "rt4bd"
        blend_desc_to_write.rt5_blend_desc_name = "rt5bd"
        blend_desc_to_write.rt6_blend_desc_name = "rt6bd"
        blend_desc_to_write.rt7_blend_desc_name = "rt7bd"

        write_result = asyncio.run(blend_desc_db.write(blend_desc_to_write))
        self.assertTrue(write_result)

        blend_desc = asyncio.run(blend_desc_db.read(desc_name="bdesc0"))
        self.assertIsNotNone(blend_desc)
        self.assertEqual(blend_desc_to_write, blend_desc)

        blend_desc_to_write.alpha_to_coverage = True
        write_result = asyncio.run(blend_desc_db.write(blend_desc_to_write))
        self.assertTrue(write_result)
        blend_desc = asyncio.run(blend_desc_db.read(desc_name="bdesc0"))
        self.assertIsNotNone(blend_desc)
        self.assertEqual(blend_desc_to_write, blend_desc)

        blend_desc_to_write.name = "bdesc1"
        write_result = asyncio.run(blend_desc_db.write(blend_desc_to_write))
        self.assertTrue(write_result)
        blend_desc = asyncio.run(blend_desc_db.read(desc_name="bdesc1"))
        self.assertIsNotNone(blend_desc)
        self.assertEqual(blend_desc_to_write, blend_desc)

        descs = asyncio.run(blend_desc_db.read_all())
        self.assertIsNotNone(descs)
        self.assertEqual(len(descs), 2)
        self.assertEqual(descs[1], blend_desc_to_write)
        blend_desc_to_write.name = "bdesc0"
        self.assertEqual(descs[0], blend_desc_to_write)

    def test_rt_blend_desc(self):
        asyncio.run(uacedb.clear_db(self.database_path_bs))

        rt_blend_desc_db = uacedb.Desc(uacedb.Desc.TYPE_RT_BLEND_DESC, self.database_path_bs)

        rt_blend_desc_none = asyncio.run(rt_blend_desc_db.read(desc_name="rtblend0"))
        self.assertIsNone(rt_blend_desc_none)

        rt_blend_desc_to_write = uacedb.Desc.RTBlendDesc(name="rtblend0")
        rt_blend_desc_to_write.blend_enable = True
        rt_blend_desc_to_write.logic_op_enable = False
        rt_blend_desc_to_write.src_blend = 1
        rt_blend_desc_to_write.dest_blend = 2
        rt_blend_desc_to_write.blend_op = 3
        rt_blend_desc_to_write.src_blend_alpha = 4
        rt_blend_desc_to_write.dest_blend_alpha = 5
        rt_blend_desc_to_write.blend_op_alpha = 6
        rt_blend_desc_to_write.logic_op = 7
        rt_blend_desc_to_write.render_target_write_mask = 8
        
        write_result = asyncio.run(rt_blend_desc_db.write(rt_blend_desc_to_write))
        self.assertTrue(write_result)

        rt_blend_desc = asyncio.run(rt_blend_desc_db.read(desc_name="rtblend0"))
        self.assertIsNotNone(rt_blend_desc)
        self.assertEqual(rt_blend_desc_to_write, rt_blend_desc)

        rt_blend_desc_to_write.logic_op_enable = True
        write_result = asyncio.run(rt_blend_desc_db.write(rt_blend_desc_to_write))
        self.assertTrue(write_result)
        rt_blend_desc = asyncio.run(rt_blend_desc_db.read(desc_name="rtblend0"))
        self.assertIsNotNone(rt_blend_desc)
        self.assertEqual(rt_blend_desc_to_write, rt_blend_desc)

        rt_blend_desc_to_write.name = "rtblend1"
        write_result = asyncio.run(rt_blend_desc_db.write(rt_blend_desc_to_write))
        self.assertTrue(write_result)
        rt_blend_desc = asyncio.run(rt_blend_desc_db.read(desc_name="rtblend1"))
        self.assertIsNotNone(rt_blend_desc)
        self.assertEqual(rt_blend_desc_to_write, rt_blend_desc)

        descs = asyncio.run(rt_blend_desc_db.read_all())
        self.assertIsNotNone(descs)
        self.assertEqual(len(descs), 2)
        self.assertEqual(descs[1], rt_blend_desc_to_write)
        rt_blend_desc_to_write.name = "rtblend0"
        self.assertEqual(descs[0], rt_blend_desc_to_write)


class TestDeviation(unittest.TestCase):
    def test_deviation_header(self):
        obj1_dev1 = devfactory.DevHeader(obj_name="Obj1", obj_type=0, dev_type=devfactory.DevTypes.TRANSFORMATION)
        obj1_dev1_1 = devfactory.DevHeader(obj_name="Obj1", obj_type=0, dev_type=devfactory.DevTypes.TRANSFORMATION)
        obj1_dev2 = devfactory.DevHeader(obj_name="Obj1", obj_type=0, dev_type=devfactory.DevTypes.PARAMETERS)

        self.assertEqual(obj1_dev1, obj1_dev1_1)
        self.assertNotEqual(obj1_dev1, obj1_dev2)

    def test_deviations_list(self):
        dev_list = devfactory.DeviationList()
        
        self.assertEqual(dev_list.len(), 0)
        obj1_dev1 = devfactory.DevHeader(obj_name="Obj1", obj_type=0, dev_type=devfactory.DevTypes.TRANSFORMATION)
        dev_list.append(obj1_dev1)

        self.assertEqual(dev_list.len(), 1)
        pulled_obj1_dev1 = dev_list.pull()
        self.assertEqual(obj1_dev1, pulled_obj1_dev1)
        self.assertEqual(dev_list.len(), 0)

        obj2_dev1 = devfactory.DevHeader(obj_name="Obj2", obj_type=0, dev_type=devfactory.DevTypes.TRANSFORMATION)
        dev_list.append(obj2_dev1)
        self.assertEqual(dev_list.len(), 1)
        dev_list.append(obj1_dev1)
        self.assertEqual(dev_list.len(), 2)

        pulled_obj2_dev1 = dev_list.pull()
        self.assertEqual(obj2_dev1, pulled_obj2_dev1)
        pulled_obj1_dev1 = dev_list.pull()
        self.assertEqual(obj1_dev1, pulled_obj1_dev1)
        self.assertEqual(dev_list.len(), 0)

    def test_deviations_list_overlaps(self):
        dev_list = devfactory.DeviationList()
        obj1_dev1 = devfactory.DevHeader(obj_name="Obj1", obj_type=0, dev_type=devfactory.DevTypes.TRANSFORMATION)
        obj2_dev1 = devfactory.DevHeader(obj_name="Obj2", obj_type=0, dev_type=devfactory.DevTypes.TRANSFORMATION)

        dev_list.append(obj1_dev1)
        dev_list.append(obj1_dev1)
        obj1_dev1_2 = devfactory.DevHeader(obj_name="Obj1", obj_type=0, dev_type=devfactory.DevTypes.TRANSFORMATION)
        dev_list.append(obj1_dev1_2)
        self.assertEqual(dev_list.len(), 1)
        obj1_dev2 = devfactory.DevHeader(obj_name="Obj1", obj_type=0, dev_type=devfactory.DevTypes.PARAMETERS)
        dev_list.append(obj1_dev2)
        self.assertEqual(dev_list.len(), 2)

        obj1_dev3 = devfactory.DevHeader(obj_name="Obj1", obj_type=0, dev_type=devfactory.DevTypes.DELETE)
        dev_list.append(obj1_dev3)
        self.assertEqual(dev_list.len(), 1)
        
        pulled_obj1_dev3 = dev_list.pull()
        self.assertEqual(obj1_dev3, pulled_obj1_dev3)

    def test_deviation_list_weights(self):
        dev_list = devfactory.DeviationList(wlimit=4)
        obj1_dev1 = devfactory.DevHeader(obj_name="Obj1", obj_type=0, dev_type=devfactory.DevTypes.TRANSFORMATION)
        obj2_dev1 = devfactory.DevHeader(obj_name="Obj2", obj_type=0, dev_type=devfactory.DevTypes.TRANSFORMATION, weight=1)
        obj3_dev1 = devfactory.DevHeader(obj_name="Obj3", obj_type=0, dev_type=devfactory.DevTypes.TRANSFORMATION, weight=2)
        obj4_dev1 = devfactory.DevHeader(obj_name="Obj4", obj_type=0, dev_type=devfactory.DevTypes.TRANSFORMATION, weight=2)

        self.assertEqual(dev_list.wleft(), 4)
        dev_list.append(obj1_dev1)
        self.assertEqual(dev_list.wleft(), 3)
        dev_list.append(obj2_dev1)
        self.assertEqual(dev_list.wleft(), 2)
        dev_list.append(obj3_dev1)
        self.assertEqual(dev_list.wleft(), 0)
        self.assertEqual(dev_list.len(), 3)

        dev_list.append(obj4_dev1)
        self.assertEqual(dev_list.wleft(), 0)
        self.assertEqual(dev_list.len(), 3)

        pulled_obj1_dev1 = dev_list.pull()
        self.assertEqual(obj1_dev1, pulled_obj1_dev1)
        self.assertEqual(dev_list.wleft(), 1)
        self.assertEqual(dev_list.len(), 2)

        dev_list.append(obj4_dev1)
        self.assertEqual(dev_list.wleft(), -1)
        self.assertEqual(dev_list.len(), 3)

        pulled_obj2_dev1 = dev_list.pull()
        self.assertEqual(obj2_dev1, pulled_obj2_dev1)

        pulled_obj3_dev1 = dev_list.pull()
        self.assertEqual(obj3_dev1, pulled_obj3_dev1)

        pulled_obj4_dev1 = dev_list.pull()
        self.assertEqual(obj4_dev1, pulled_obj4_dev1)
        
        self.assertEqual(dev_list.wleft(), 4)
        self.assertEqual(dev_list.len(), 0)

        dev_list.append(obj1_dev1)
        self.assertEqual(dev_list.wleft(), 3)
        self.assertEqual(dev_list.len(), 1)

        dev_list.append(obj1_dev1)
        self.assertEqual(dev_list.wleft(), 3)
        self.assertEqual(dev_list.len(), 1)
        
    def test_deviation_header_resolve(self):
        obj1_dev1 = devfactory.DevHeader(obj_name="Obj1", obj_type='CAMERA', dev_type=devfactory.DevTypes.CREATE)
        obj1_dev2 = devfactory.DevHeader(obj_name="Obj1", obj_type='CAMERA', dev_type=devfactory.DevTypes.TRANSFORMATION)
        obj1_dev3 = devfactory.DevHeader(obj_name="Obj1", obj_type='CAMERA', dev_type=devfactory.DevTypes.PARAMETERS)
        obj1_dev4 = devfactory.DevHeader(obj_name="Obj1", obj_type='CAMERA', dev_type=devfactory.DevTypes.DELETE)
        obj1_dev5 = devfactory.DevHeader(obj_name="Obj1", obj_type='CAMERA', dev_type=devfactory.DevTypes.NONE)

        resolve_id_1 = devfactory.resolve_header_id(obj1_dev1)
        self.assertEqual(resolve_id_1, devfactory.ResolveIds.CREATE_CAMERA)

        resolve_id_2 = devfactory.resolve_header_id(obj1_dev2)
        self.assertEqual(resolve_id_2, devfactory.ResolveIds.CAMERA_UPDATE)
        
        resolve_id_3 = devfactory.resolve_header_id(obj1_dev3)
        self.assertEqual(resolve_id_3, devfactory.ResolveIds.CAMERA_UPDATE)
        
        resolve_id_4 = devfactory.resolve_header_id(obj1_dev4)
        self.assertEqual(resolve_id_4, devfactory.ResolveIds.DELETE)
        
        resolve_id_5 = devfactory.resolve_header_id(obj1_dev5)
        self.assertEqual(resolve_id_5, devfactory.ResolveIds.DISCARD)
        
        obj2_dev1 = devfactory.DevHeader(obj_name="Obj1", obj_type='MESH', dev_type=devfactory.DevTypes.CREATE)
        resolve_id_6 = devfactory.resolve_header_id(obj2_dev1)
        self.assertEqual(resolve_id_6, devfactory.ResolveIds.CREATE_STATIC_MESH)
        
        obj2_dev2 = devfactory.DevHeader(obj_name="Obj1", obj_type='MESH', dev_type=devfactory.DevTypes.TRANSFORMATION)
        resolve_id_7 = devfactory.resolve_header_id(obj2_dev2)
        self.assertEqual(resolve_id_7, devfactory.ResolveIds.TRANSFORMATION)

        obj2_dev3 = devfactory.DevHeader(obj_name="Obj1", obj_type='MESH', dev_type=devfactory.DevTypes.PARAMETERS)
        resolve_id_8 = devfactory.resolve_header_id(obj2_dev3)
        self.assertEqual(resolve_id_8, devfactory.ResolveIds.GEOMETRY)

    def test_deviation_data_gatherer(self):
        gatherer = devfactory.DeviationDataGatherer()
        g_meta_1 = gatherer.get_dev_data(obj_name="Obj1", resolve_id=devfactory.ResolveIds.CREATE_CAMERA)
        self.assertIsNone(g_meta_1)

        self.assertTrue(gatherer.setup_resolve(resolve_id=devfactory.ResolveIds.CREATE_CAMERA, cb=lambda obj_name: obj_name))
        g_meta_2 = gatherer.get_dev_data(obj_name="Obj1", resolve_id=devfactory.ResolveIds.CREATE_CAMERA)
        self.assertEqual(g_meta_2, "Obj1")

        g_meta_3 = gatherer.get_dev_data(obj_name="Obj1", resolve_id=devfactory.ResolveIds.DELETE)
        self.assertIsNone(g_meta_3)

        self.assertTrue(gatherer.setup_resolve(resolve_id=devfactory.ResolveIds.DELETE, cb=lambda obj_name: obj_name))
        g_meta_4 = gatherer.get_dev_data(obj_name="Obj1", resolve_id=devfactory.ResolveIds.DELETE)
        self.assertEqual(g_meta_4, "Obj1")
        
        self.assertTrue(gatherer.setup_resolve(resolve_id=devfactory.ResolveIds.TRANSFORMATION, cb=lambda obj_name: obj_name))
        g_meta_5 = gatherer.get_dev_data(obj_name="Obj1", resolve_id=devfactory.ResolveIds.TRANSFORMATION)
        self.assertEqual(g_meta_5, "Obj1")
        
        self.assertTrue(gatherer.setup_resolve(resolve_id=devfactory.ResolveIds.GEOMETRY, cb=lambda obj_name: obj_name))
        g_meta_6 = gatherer.get_dev_data(obj_name="Obj1", resolve_id=devfactory.ResolveIds.GEOMETRY)
        self.assertEqual(g_meta_6, "Obj1")

class TestBlenderPackage(unittest.TestCase):

    def test_header_pkg_generator(self):
        object_create_pkg = devfactory.gen_obj_create_pkg(object_name="Obj1", object_type="CAMERA", deviation_id=1)
        expected_object_create_pkg = {\
            "PkgType": "Deviation",\
            "DeviationType": "Creation",\
            "ObjectName": "Obj1", \
            "DeviationId": 1,\
            "ObjectType": "CAMERA"\
            }
        self.assertEqual(object_create_pkg, expected_object_create_pkg)

        object_delete_pkg = devfactory.gen_obj_delete_pkg(object_name="Obj1", object_type="CAMERA", deviation_id=1)
        expected_object_delete_pkg = {\
            "PkgType": "Deviation",\
            "DeviationType": "Deletion",\
            "ObjectName": "Obj1", \
            "DeviationId": 1,\
            "ObjectType": "CAMERA"\
            }
        self.assertEqual(object_delete_pkg, expected_object_delete_pkg)

        transform_pkg = devfactory.gen_transform_pkg(object_name="Obj1", object_type="MESH", deviation_id=1)
        expected_transform_pkg = {\
            "PkgType": "Deviation",\
            "DeviationType": "Transform",\
            "ObjectName": "Obj1", \
            "DeviationId": 1,\
            "ObjectType": "MESH",\
            }
        self.assertEqual(transform_pkg, expected_transform_pkg)

        geomerty_pkg = devfactory.gen_geometry_pkg(object_name="Obj1", deviation_id=1)
        expected_geometry_pkg = {\
            "PkgType": "Deviation",\
            "DeviationType": "Geometry",\
            "ObjectName": "Obj1", \
            "DeviationId": 1,\
            "ObjectType": "MESH",\
            }
        self.assertEqual(geomerty_pkg, expected_geometry_pkg)

        camera_update_pkg = devfactory.gen_camera_update_pkg(object_name="Obj1", deviation_id=1)
        expected_camera_update_pkg = {\
            "PkgType": "Deviation",\
            "DeviationType": "Camera",\
            "ObjectName": "Obj1", \
            "DeviationId": 1,\
            "ObjectType": "CAMERA"\
            }
        self.assertEqual(camera_update_pkg, expected_camera_update_pkg)


class TestBlenderDatabase(unittest.TestCase):


    def __init__(self, *args, **kwargs):
        super(TestBlenderDatabase, self).__init__(*args, **kwargs)
        self.database_path = "database/test_bsidedb.sqlite"
        copyfile("database/bsidedb.sqlite", self.database_path)

    def test_log_dev_headers_in_db(self):
        bsdb.clear_db(self.database_path)

        obj1_dev1 = devfactory.DevHeader(obj_name="Obj1", obj_type=1, dev_type=devfactory.DevTypes.TRANSFORMATION)
        bsdb.log_dev_header(self.database_path, obj1_dev1, deviation_id=1)
        loaded_dev_headers_1 = bsdb.load_dev_headers(self.database_path, deviation_id=1)
        loaded_obj1_dev1 = loaded_dev_headers_1[0]
        self.assertEqual(obj1_dev1, loaded_obj1_dev1)

        obj1_dev2 = devfactory.DevHeader(obj_name="Obj1", obj_type=1, dev_type=devfactory.DevTypes.PARAMETERS)
        obj1_dev3 = devfactory.DevHeader(obj_name="Obj1", obj_type=1, dev_type=devfactory.DevTypes.DELETE)

        bsdb.log_dev_header(self.database_path, obj1_dev1, deviation_id=2)
        bsdb.log_dev_header(self.database_path, obj1_dev2, deviation_id=2)
        bsdb.log_dev_header(self.database_path, obj1_dev3, deviation_id=2)

        loaded_dev_headers_2 = bsdb.load_dev_headers(self.database_path, deviation_id=2)
        self.assertEqual(obj1_dev1, loaded_dev_headers_2[0])
        self.assertEqual(obj1_dev2, loaded_dev_headers_2[1])
        self.assertEqual(obj1_dev3, loaded_dev_headers_2[2])
        
        loaded_dev_headers_1 = bsdb.load_dev_headers(self.database_path, deviation_id=1)
        self.assertEqual(loaded_dev_headers_1[0], loaded_obj1_dev1)

        loaded_dev_headers_3 = bsdb.load_dev_headers(self.database_path, deviation_id=3)
        self.assertEqual(loaded_dev_headers_3, [])

    def test_clear_dev_headers_in_db(self):
        bsdb.clear_db(self.database_path)

        obj1_dev1 = devfactory.DevHeader(obj_name="Obj1", obj_type=1, dev_type=devfactory.DevTypes.TRANSFORMATION)
        obj1_dev2 = devfactory.DevHeader(obj_name="Obj1", obj_type=1, dev_type=devfactory.DevTypes.PARAMETERS)
        obj1_dev3 = devfactory.DevHeader(obj_name="Obj1", obj_type=1, dev_type=devfactory.DevTypes.DELETE)

        bsdb.log_dev_header(self.database_path, obj1_dev1, deviation_id=1)
        bsdb.log_dev_header(self.database_path, obj1_dev2, deviation_id=1)
        bsdb.log_dev_header(self.database_path, obj1_dev3, deviation_id=2)

        loaded_dev_headers_1 = bsdb.load_dev_headers(self.database_path, deviation_id=1)
        self.assertEqual(obj1_dev1, loaded_dev_headers_1[0])
        self.assertEqual(obj1_dev2, loaded_dev_headers_1[1])

        loaded_dev_headers_2 = bsdb.load_dev_headers(self.database_path, deviation_id=2)
        self.assertEqual(obj1_dev3, loaded_dev_headers_2[0])

        bsdb.clear_dev_headers(self.database_path, deviation_id=1)
        loaded_dev_headers_1 = bsdb.load_dev_headers(self.database_path, deviation_id=1)
        self.assertEqual(loaded_dev_headers_1, [])
        
        loaded_dev_headers_2 = bsdb.load_dev_headers(self.database_path, deviation_id=2)
        self.assertEqual(obj1_dev3, loaded_dev_headers_2[0])

    def test_log_completed_dev_id_in_db(self):
        bsdb.clear_db(self.database_path)
        loaded_last_dev_id_0 = bsdb.load_last_dev_id(self.database_path)
        self.assertIsNone(loaded_last_dev_id_0)
        bsdb.log_last_dev_id(self.database_path, deviation_id=1)
        loaded_last_dev_id_1 = bsdb.load_last_dev_id(self.database_path)
        self.assertEqual(loaded_last_dev_id_1, 1)


class TestBlenderDeviationsManager(unittest.TestCase):


    def __init__(self, *args, **kwargs):
        super(TestBlenderDeviationsManager, self).__init__(*args, **kwargs)
        self.database_path = "database/test_bsidedb.sqlite"
        copyfile("database/bsidedb.sqlite", self.database_path)

    def test_timeout(self):
        tout = devfactory.Timeout(max_time_sec=0.05)
        tout.reset()
        sleep(0.01)
        self.assertFalse(tout.get_expired())
        tout.reset()
        sleep(0.06)
        self.assertTrue(tout.get_expired())
        tout.reset()
        sleep(0.01)
        self.assertFalse(tout.get_expired())

    def test_queue_dist_manager(self):
        manager = devfactory.DevQueueChecker(timeout_lim=0.05)
        manager.reset()
        self.assertFalse(manager.get_queue_complete(is_quque_full=False))
        self.assertTrue(manager.get_queue_complete(is_quque_full=True))
        manager.reset()
        self.assertFalse(manager.get_queue_complete(is_quque_full=False))
        sleep(0.06)
        self.assertTrue(manager.get_queue_complete(is_quque_full=False))
        manager.reset()
        sleep(0.06)
        self.assertTrue(manager.get_queue_complete(is_quque_full=True))

    def test_queue_pool(self):
        qpool = devfactory.DevQueuePool(num_of_queues=3, wlimit=2, time_limit=0.5)

        self.assertFalse(qpool.update())

        self.assertTrue(qpool.init_queue_write(deviation_id=1))
        
        self.assertIsNone(qpool.init_queue_read())

        self.assertFalse(qpool.update())
        
        #Write 1st queue
        p1_obj1_dev1 = devfactory.DevHeader(obj_name="Obj1", obj_type=0, dev_type=devfactory.DevTypes.CREATE)
        self.assertTrue(qpool.append(p1_obj1_dev1))
        self.assertFalse(qpool.get_write_queue_full())
        
        self.assertFalse(qpool.update())
        
        p1_obj1_dev2 = devfactory.DevHeader(obj_name="Obj1", obj_type=0, dev_type=devfactory.DevTypes.TRANSFORMATION)
        self.assertTrue(qpool.append(p1_obj1_dev2))
        self.assertTrue(qpool.get_write_queue_full())
        
        p1_obj2_dev1 = devfactory.DevHeader(obj_name="Obj2", obj_type=0, dev_type=devfactory.DevTypes.PARAMETERS)
        self.assertFalse(qpool.append(p1_obj2_dev1))
        self.assertTrue(qpool.get_write_queue_full())
        
        self.assertTrue(qpool.update())

        #Write 2nd queue
        self.assertTrue(qpool.init_queue_write(deviation_id=2))

        self.assertFalse(qpool.get_write_queue_full())
        self.assertTrue(qpool.append(p1_obj2_dev1))
        self.assertFalse(qpool.get_write_queue_full())

        self.assertFalse(qpool.update())
        
        p1_obj2_dev2 = devfactory.DevHeader(obj_name="Obj2", obj_type=0, dev_type=devfactory.DevTypes.TRANSFORMATION)
        self.assertTrue(qpool.append(p1_obj2_dev2))
        self.assertTrue(qpool.get_write_queue_full())

        #Read 1st queue
        self.assertEqual(qpool.init_queue_read(), 1)

        loaded_p1_obj1_dev1 = qpool.read()
        self.assertEqual(p1_obj1_dev1, loaded_p1_obj1_dev1)

        loaded_p1_obj1_dev2 = qpool.read()
        self.assertEqual(p1_obj1_dev2, loaded_p1_obj1_dev2)

        loaded_p1_obj2_dev1 = qpool.read()
        self.assertIsNone(loaded_p1_obj2_dev1)
        
        #2nd queue is not ready
        self.assertIsNone(qpool.init_queue_read())

        self.assertTrue(qpool.update())

        #Init reading 2nd queue but do not read. Block writing 
        self.assertEqual(qpool.init_queue_read(), 2)
        
        #Write 3nd queue
        self.assertTrue(qpool.init_queue_write(deviation_id=3))

        #Continue to write 3nd queue
        p1_obj3_dev1 = devfactory.DevHeader(obj_name="Obj3", obj_type=0, dev_type=devfactory.DevTypes.NONE)
        self.assertTrue(qpool.append(p1_obj3_dev1))

        p1_obj3_dev2 = devfactory.DevHeader(obj_name="Obj3", obj_type=0, dev_type=devfactory.DevTypes.TRANSFORMATION)
        self.assertTrue(qpool.append(p1_obj3_dev2))

        #Write 1st queue again
        self.assertTrue(qpool.init_queue_write(deviation_id=4))

        self.assertTrue(qpool.append(p1_obj1_dev1))
        self.assertTrue(qpool.append(p1_obj1_dev2))

        #Cannot write to the 2nd, it is streal in reading state
        self.assertFalse(qpool.init_queue_write(deviation_id=5))

        #Read the 2nd queue
        loaded_p1_obj2_dev1 = qpool.read()
        self.assertEqual(p1_obj2_dev1, loaded_p1_obj2_dev1)

        loaded_p1_obj2_dev2 = qpool.read()
        self.assertEqual(p1_obj2_dev2, loaded_p1_obj2_dev2)

        loaded_p1_obj3_dev1 = qpool.read()
        self.assertIsNone(loaded_p1_obj3_dev1)

        #Now we can write to the 2nd queue
        self.assertTrue(qpool.init_queue_write(deviation_id=5))

        self.assertFalse(qpool.update())
        sleep(0.6)
        #ignore timeout untill first write
        self.assertFalse(qpool.update())
        
        self.assertTrue(qpool.append(p1_obj2_dev1))
        self.assertFalse(qpool.update())
        
        sleep(0.6)
        self.assertTrue(qpool.update())

        #Waited too long, queue gone
        self.assertFalse(qpool.append(p1_obj2_dev2))


class TestCppGenerator(unittest.TestCase):


    def __init__(self, *args, **kwargs):
        super(TestCppGenerator, self).__init__(*args, **kwargs)
        self.database_path = "database/test_bscriptdb.sqlite"
        copyfile("database/bscriptdb.sqlite", self.database_path)

        self.maxDiff = None
        
        self.expected_gen_declarations = [\
            "#include \"scriptAssists/tableRow.h\"\n#include \"scriptAssists/scriptCore.h\"\n\nnamespace Tables\n{\n\tstruct Position\n\t{\n\t\tPosition(char* ptr, size_t numOfBytes) : \n\t\t\tx(ptr + 0, 65536 * sizeof(float) + 0),\n\t\t\ty(ptr + 0 + 65536 * sizeof(float), 65536 * sizeof(float) + 0 + 65536 * sizeof(float))\n\t\t{\t}\n\t\tTableRow<float> x;\n\t\tTableRow<float> y;\n\t\tint numOfElements{};\n\t};\n};", 
            "#include \"scriptAssists/tableRow.h\"\n#include \"scriptAssists/scriptCore.h\"\n\nnamespace Tables\n{\n\tstruct Health\n\t{\n\t\tHealth(char* ptr, size_t numOfBytes) : \n\t\t\thealth(ptr + 0, 1024 * sizeof(int32_t) + 0)\n\t\t{\t}\n\t\tTableRow<int32_t> health;\n\t\tint numOfElements{};\n\t};\n};",
            "#include \"scriptAssists/tableRow.h\"\n#include \"scriptAssists/scriptCore.h\"\n\nnamespace Tables\n{\n\tstruct Damage\n\t{\n\t\tDamage(char* ptr, size_t numOfBytes) : \n\t\t\tt1(ptr + 0, 512 * sizeof(int16_t) + 0),\n\t\t\tt2(ptr + 0 + 512 * sizeof(int16_t), 512 * sizeof(int8_t) + 0 + 512 * sizeof(int16_t)),\n\t\t\tt3(ptr + 0 + 512 * sizeof(int16_t) + 512 * sizeof(int8_t), 512 * sizeof(uint32_t) + 0 + 512 * sizeof(int16_t) + 512 * sizeof(int8_t)),\n\t\t\tt4(ptr + 0 + 512 * sizeof(int16_t) + 512 * sizeof(int8_t) + 512 * sizeof(uint32_t), 512 * sizeof(uint16_t) + 0 + 512 * sizeof(int16_t) + 512 * sizeof(int8_t) + 512 * sizeof(uint32_t))\n\t\t{\t}\n\t\tTableRow<int16_t> t1;\n\t\tTableRow<int8_t> t2;\n\t\tTableRow<uint32_t> t3;\n\t\tTableRow<uint16_t> t4;\n\t\tint numOfElements{};\n\t};\n};",
        ]

    def test_table_declaration_headers(self):
        headers = scriptdb.get_tables_header(self.database_path)
        self.assertIsNotNone(headers)
        self.assertEqual(len(headers), 3)

        for header in headers:
            self.assertEqual(len(header), 3)

        expected_data = [["Position", 2, 65536], ["Health", 1, 1024], ["Damage", 4, 512]]

        for id, table in enumerate(headers):
            self.assertEqual(table[0], expected_data[id][0])
            self.assertEqual(table[1], expected_data[id][1])
            self.assertEqual(table[2], expected_data[id][2])

    def test_table_declaration_col_desc(self):
        headers = scriptdb.get_tables_header(self.database_path)
        table_position = headers[0]
        
        expected_data = [["x", 1, "y", 1], ["health", 2], ["t1", 3, "t2", 4, "t3", 5, "t4", 6]]

        for table_id, table in enumerate(headers):
            col_desc = scriptdb.get_table_cols_desc(self.database_path, table[0])
            self.assertIsNotNone(col_desc)
            for col_id, col in enumerate(col_desc):
                self.assertEqual(col[1], expected_data[table_id][col_id * 2])
                self.assertEqual(col[2], expected_data[table_id][col_id * 2 + 1])

    def test_types_decoder(self):
        self.assertEqual(cppgen.decode_type(1), "float")
        self.assertEqual(cppgen.decode_type(2), "int32_t") 
        self.assertEqual(cppgen.decode_type(3), "int16_t") 
        self.assertEqual(cppgen.decode_type(4), "int8_t") 
        self.assertEqual(cppgen.decode_type(5), "uint32_t") 
        self.assertEqual(cppgen.decode_type(6), "uint16_t") 
        self.assertEqual(cppgen.decode_type(7), "uint8_t")

    def test_type_in_range(self):
        self.assertTrue(cppgen.type_is_valid(1))
        self.assertTrue(cppgen.type_is_valid(2))
        self.assertTrue(cppgen.type_is_valid(3))
        self.assertTrue(cppgen.type_is_valid(4))
        self.assertTrue(cppgen.type_is_valid(5))
        self.assertTrue(cppgen.type_is_valid(6))
        self.assertTrue(cppgen.type_is_valid(7))

        self.assertFalse(cppgen.type_is_valid(0))
        self.assertFalse(cppgen.type_is_valid(-1))
        self.assertFalse(cppgen.type_is_valid(8))
        self.assertFalse(cppgen.type_is_valid(9))

    def test_gen_declaration_for_table(self):
        headers = scriptdb.get_tables_header(self.database_path)


        for id, table in enumerate(headers):
            declaration = cppgen.gen_declaration(self.database_path, table)
            self.assertIsNotNone(declaration)
            self.assertEqual(declaration, self.expected_gen_declarations[id])

    def test_generate_declaration_file(self):
        path = "test_gen_files/"
        file_name = "demo_file"

        headers = scriptdb.get_tables_header(self.database_path)

        for id, table in enumerate(headers):
            self.assertTrue(cppgen.gen_declaration_file(path, self.database_path, table))

            f_read = open(path + table[0] + ".h", "r")
            self.assertFalse(f_read.closed)
            declaration = f_read.read()
            self.assertEqual(declaration, self.expected_gen_declarations[id])
            f_read.close()

if __name__ == "__main__":
    unittest.main()

import struct

TYPE_FLOAT = 0
TYPE_MAT4X4 = 1
TYPE_VEC3 = 2
TYPE_INT = 3

def pack_float(f):
    return bytearray(struct.pack("f", f))

def pack_int(i):
    return bytearray(struct.pack("i", i))

def ser_vec3(vec3):
    if vec3 == None:
        return None

    byte_array = bytearray()  

    byte_array.extend(pack_float(vec3[0]))
    byte_array.extend(pack_float(vec3[1]))
    byte_array.extend(pack_float(vec3[2]))

    return byte_array

def ser_mat4x4(mat):
    if mat == None:
        return None

    byte_array = pack_float(mat[0][0])
    byte_array.extend(pack_float(mat[0][1]))
    byte_array.extend(pack_float(mat[0][2]))
    byte_array.extend(pack_float(mat[0][3]))
    
    byte_array.extend(pack_float(mat[1][0]))
    byte_array.extend(pack_float(mat[1][1]))
    byte_array.extend(pack_float(mat[1][2]))
    byte_array.extend(pack_float(mat[1][3]))
    
    byte_array.extend(pack_float(mat[2][0]))
    byte_array.extend(pack_float(mat[2][1]))
    byte_array.extend(pack_float(mat[2][2]))
    byte_array.extend(pack_float(mat[2][3]))
    
    byte_array.extend(pack_float(mat[3][0]))
    byte_array.extend(pack_float(mat[3][1]))
    byte_array.extend(pack_float(mat[3][2]))
    byte_array.extend(pack_float(mat[3][3]))

    return byte_array

def unpack_float(ser):
    f = struct.unpack('f', ser)
    return float(f[0])

def unpack_int(ser):
    i = struct.unpack('i', ser)
    return int(i[0])

def _get_data_partion(ser, offset, len):
    bytes_blob = ser[offset:offset + len]
    return bytes_blob

def deser_vec3(ser_vec3):
    vec3 = [0.0, 0.0, 0.0]
    vec3[0] = unpack_float(_get_data_partion(ser_vec3, offset=0, len=4))
    vec3[1] = unpack_float(_get_data_partion(ser_vec3, offset=4, len=4))
    vec3[2] = unpack_float(_get_data_partion(ser_vec3, offset=8, len=4))

    return vec3

def deser_mat4x4(ser_mat):
    mat = [[0 for i in range(4)] for i in range(4)]
    mat[0][0] = unpack_float(_get_data_partion(ser_mat, 0, 4))
    mat[0][1] = unpack_float(_get_data_partion(ser_mat, 4, 4))
    mat[0][2] = unpack_float(_get_data_partion(ser_mat, 8, 4))
    mat[0][3] = unpack_float(_get_data_partion(ser_mat, 12, 4))
    mat[1][0] = unpack_float(_get_data_partion(ser_mat, 16, 4))
    mat[1][1] = unpack_float(_get_data_partion(ser_mat, 20, 4))
    mat[1][2] = unpack_float(_get_data_partion(ser_mat, 24, 4))
    mat[1][3] = unpack_float(_get_data_partion(ser_mat, 28, 4))
    mat[2][0] = unpack_float(_get_data_partion(ser_mat, 32, 4))
    mat[2][1] = unpack_float(_get_data_partion(ser_mat, 36, 4))
    mat[2][2] = unpack_float(_get_data_partion(ser_mat, 40, 4))
    mat[2][3] = unpack_float(_get_data_partion(ser_mat, 44, 4))
    mat[3][0] = unpack_float(_get_data_partion(ser_mat, 48, 4))
    mat[3][1] = unpack_float(_get_data_partion(ser_mat, 52, 4))
    mat[3][2] = unpack_float(_get_data_partion(ser_mat, 56, 4))
    mat[3][3] = unpack_float(_get_data_partion(ser_mat, 60, 4))

    return mat

def ser(objs):
    ser_stream = bytearray()
    for obj in objs:
        if isinstance(obj, float) is True:
            ser_stream.extend(pack_float(obj))
        elif isinstance(obj, int) is True:
            ser_stream.extend(pack_int(obj))
        elif isinstance(obj, str) is True:
            ser_stream.extend(struct.pack("c", obj[0].encode('ascii')))
        elif len(obj) == 3:
            ser_stream.extend(ser_vec3(obj))
        elif len(obj) == 4:
            ser_stream.extend(ser_mat4x4(obj))

    return ser_stream

def deser(ser_stream, types):
    outData = []
    point = 0
    for t in types:
        if t == TYPE_FLOAT:
            outData.append(unpack_float(_get_data_partion(ser_stream, point, 4)))
            point = point + 4
        elif t == TYPE_INT:
            outData.append(unpack_int(_get_data_partion(ser_stream, point, 4)))
            point = point + 4
        elif t == TYPE_VEC3:
            outData.append(deser_vec3(_get_data_partion(ser_stream, point, 12)))
            point = point + 12
        elif t == TYPE_MAT4X4:
            outData.append(deser_mat4x4(_get_data_partion(ser_stream, point, 64)))
            point = point + 64

    return outData
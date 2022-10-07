from lib import bser

func_data = [\
0,\
2,\
4,\
2,\
0,\
1,\
0,\
0,\
0,\
0,\
3,\
0,\
0,\
0,\
]

struct_data = [\
0,\
2,\
1,\
6,\
11,\
6,\
4,\
1,\
1,\
12,\
11,\
7,\
1,\
1,\
13,\
16,\
9,\
0,\
0,\
0,\
]

func_names = {'mathPlus': [0], 'mathNegate': [4], 'getObj1': [7], 'processObj': [9]}

version = 1
out_stream = bytearray()
out_stream.extend(bser.pack_int(version))

num_of_func_data_els = len(func_data)
out_stream.extend(bser.pack_int(num_of_func_data_els))
func_bytes = bytearray()
for el in func_data:
    func_bytes.extend(bser.pack_int(el))
out_stream.extend(func_bytes)

num_of_struct_data_els = len(struct_data)
out_stream.extend(bser.pack_int(num_of_struct_data_els))

struct_bytes = bytearray()
for el in struct_data:
    struct_bytes.extend(bser.pack_int(el))
out_stream.extend(struct_bytes)

num_of_func_names = len(func_names)
out_stream.extend(bser.pack_int(num_of_func_names))

for fn in func_names:
    name_bytes = bytearray(str.encode(fn))
    name_bytes.extend(bytearray(32 - len(name_bytes)))
    out_stream.extend(name_bytes)

    points = func_names[fn]
    num_of_points = len(points)
    out_stream.extend(bser.pack_int(num_of_points))
    for p in points:
        out_stream.extend(bser.pack_int(p))

scriptfile = open("../DirectX/UACE/UACE/UTests/testscript.lscr", "wb")
scriptfile.write(out_stream)
scriptfile.close()
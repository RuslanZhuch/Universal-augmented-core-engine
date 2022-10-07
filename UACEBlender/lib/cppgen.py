
try:
    from lib import scriptdb
except ImportError:
    import scriptdb

_decode_types = ["float", "int32_t", "int16_t", "int8_t", "uint32_t", "uint16_t", "uint8_t"]

def type_is_valid(type_id):
    return (type_id >= 1) and (type_id <= len(_decode_types))

def decode_type(type_id):
    return _decode_types[type_id - 1]

def gen_declaration(database_path, table):
    cols_desc = scriptdb.get_table_cols_desc(database_path, table[0])
    if len(cols_desc) == 0:
        return None

    col_data_str = ""
    col_init_str = ""

    col_init_str = ""
    col_offset_str = " + 0"

    for id, desc in enumerate(cols_desc):
        col_name = desc[1]
        col_type = desc[2]

        col_type_str = decode_type(col_type)
        
        col_new_offset_str = str(table[2]) + " * sizeof(" + col_type_str + ")"

        col_init_str += "\t\t\t" + col_name + "(ptr" + col_offset_str + ", " + col_new_offset_str + col_offset_str + ")"
        if id < len(cols_desc) - 1:
            col_init_str += ",\n"
        else:
            col_init_str += "\n\t\t{\t}\n"

        col_offset_str += " + " + col_new_offset_str

        col_str = "\t\tTableRow<" + decode_type(col_type) + "> " + col_name + ";\n"
        col_data_str += col_str

    if len(col_data_str) == 0:
        return None

    out_str = "#include \"scriptAssists/tableRow.h\"\n#include \"scriptAssists/scriptCore.h\"\n\nnamespace Tables\n{\n\tstruct " + table[0] + "\n\t{\n\t\t" + table[0] + "(char* ptr, size_t numOfBytes) : \n" + col_init_str + col_data_str + "\t\tint numOfElements{};\n\t};\n};"
    return out_str

def gen_declaration_file(path, database_path, table):
    f_write = open(path + table[0] + ".h", "w")

    declaration = gen_declaration(database_path, table)
    if declaration is None:
        return False

    f_write.write(declaration)
    f_write.close()

    return True
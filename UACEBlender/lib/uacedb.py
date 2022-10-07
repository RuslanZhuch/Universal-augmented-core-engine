import asyncio
import aiosqlite

_TABLE_PARAMETER = "{TABLE_PARAMETER}"
_DROP_TABLE_SQL = f"DELETE FROM {_TABLE_PARAMETER};"
_GET_TABLES_SQL = "SELECT name FROM sqlite_schema WHERE type='table';"


async def _delete_all_tables(con):
    tables = await _get_tables(con)
    await _delete_tables(con, tables)

async def _get_tables(con):
    cur = await con.cursor()
    await cur.execute(_GET_TABLES_SQL)
    tables = await cur.fetchall()
    await cur.close()
    return tables


async def _delete_tables(con, tables):
    cur = await con.cursor()
    for table, in tables:
        sql = _DROP_TABLE_SQL.replace(_TABLE_PARAMETER, table)
        await cur.execute(sql)
    await cur.close()

async def clear_db(database_path):
    conn = await aiosqlite.connect(database_path)
    await _delete_all_tables(conn)
    await conn.commit()
    await conn.close()

async def log_new_deviation(database_path, js_data, blob_data):

    conn = await aiosqlite.connect(database_path)

    cursor = await conn.cursor()

    pkgType = js_data["PkgType"]
    deviationType = js_data["DeviationType"]
    elementName = js_data["ObjectName"]
    objectType = js_data["ObjectType"]
    deviationId = js_data["DeviationId"]

    await cursor.execute("""SELECT Deviation_ID FROM ElementsToChange WHERE Element_name=? AND Deviation_type=?""", (elementName, deviationType))
    dev_ids = await cursor.fetchall()
    for dev_id in dev_ids:
        await cursor.execute("""DELETE FROM ElementsToChange WHERE Deviation_ID=?""", dev_id)

    await cursor.execute("""INSERT OR REPLACE INTO ElementsToChange VALUES (?, ?, ?, ?, ?, ?)""", \
        (pkgType, deviationType, elementName, objectType, deviationId, blob_data))
    '''await cursor.execute("""INSERT OR IGNORE INTO ChangeDestinations VALUES (?, ?)""", \
        (deviationId, targetId))'''

    await conn.commit()
    await conn.close()

async def get_nearest_dev_id(database_path, curr_deviation_id):
    conn = await aiosqlite.connect(database_path)
    cursor = await conn.cursor()

    await cursor.execute("""SELECT Deviation_ID FROM ElementsToChange WHERE Deviation_ID>?""", (curr_deviation_id,))

    ids = await cursor.fetchall()
    if len(ids) == 0:
        return None
    min_id = min(ids)
    return int(min_id[0])
    
async def get_deviation_data(database_path, deviation_id):
    conn = await aiosqlite.connect(database_path)
    cursor = await conn.cursor()

    await cursor.execute("""SELECT * FROM ElementsToChange WHERE Deviation_ID=?""", (deviation_id,))

    deviations = await cursor.fetchall()

    js_table_list = []
    blob_data_list = []
    for dev_data in deviations:
        pkgType = dev_data[0]
        devType = dev_data[1]
        objName = dev_data[2]
        objType = dev_data[3]
        devId = dev_data[4]
        js_table_list.append({"PkgType": pkgType, "DeviationType": devType, "ObjectName": objName, "ObjectType": objType, "DeviationId": devId})
        blob_data_list.append(dev_data[5])

    await conn.close()

    return js_table_list, blob_data_list

async def remove_deviaion(database_path, deviation_id):
    conn = await aiosqlite.connect(database_path)
    cursor = await conn.cursor()
    
    await cursor.execute("""DELETE FROM ElementsToChange WHERE Deviation_ID=?""", (deviation_id,))

    await conn.commit()
    await conn.close()


def _decode_blend_desc(desc_raw):
    desc = Desc.BlendDesc(name=desc_raw[0])  
    desc.alpha_to_coverage = bool(desc_raw[1])
    desc.independe_blend_enable = bool(desc_raw[2])
    desc.rt0_blend_desc_name = desc_raw[3]
    desc.rt1_blend_desc_name = desc_raw[4]
    desc.rt2_blend_desc_name = desc_raw[5]
    desc.rt3_blend_desc_name = desc_raw[6]
    desc.rt4_blend_desc_name = desc_raw[7]
    desc.rt5_blend_desc_name = desc_raw[8]
    desc.rt6_blend_desc_name = desc_raw[9]
    desc.rt7_blend_desc_name = desc_raw[10]
    return desc

async def _encode_blend_desc(cursor, desc_to_write):
    await cursor.execute("""INSERT OR REPLACE INTO BlendDesc VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)""", \
    (desc_to_write.name, \
    desc_to_write.alpha_to_coverage, \
    desc_to_write.independe_blend_enable, \
    desc_to_write.rt0_blend_desc_name, \
    desc_to_write.rt1_blend_desc_name, \
    desc_to_write.rt2_blend_desc_name, \
    desc_to_write.rt3_blend_desc_name, \
    desc_to_write.rt4_blend_desc_name, \
    desc_to_write.rt5_blend_desc_name, \
    desc_to_write.rt6_blend_desc_name, \
    desc_to_write.rt7_blend_desc_name, \
    ))

def _decode_rt_blend_desc(desc_raw):
    desc = Desc.RTBlendDesc(name=desc_raw[0])   
    desc.blend_enable = bool(desc_raw[1])
    desc.logic_op_enable = bool(desc_raw[2])
    desc.src_blend = desc_raw[3]
    desc.dest_blend = desc_raw[4]
    desc.blend_op = desc_raw[5]
    desc.src_blend_alpha = desc_raw[6]
    desc.dest_blend_alpha = desc_raw[7]
    desc.blend_op_alpha = desc_raw[8]
    desc.logic_op = desc_raw[9]
    desc.render_target_write_mask = desc_raw[10]
    return desc

async def _encode_rt_blend_desc(cursor, desc_to_write):
    await cursor.execute("""INSERT OR REPLACE INTO RTBlendDesc VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)""", \
    (desc_to_write.name, \
    desc_to_write.blend_enable, \
    desc_to_write.logic_op_enable, \
    desc_to_write.src_blend, \
    desc_to_write.dest_blend, \
    desc_to_write.blend_op, \
    desc_to_write.src_blend_alpha, \
    desc_to_write.dest_blend_alpha, \
    desc_to_write.blend_op_alpha, \
    desc_to_write.logic_op, \
    desc_to_write.render_target_write_mask \
    ))    

class Desc():
    TYPE_BLEND_DESC = 0
    TYPE_RT_BLEND_DESC = 1
    
    class BlendDesc:
        def __init__(self, name):
            self.name = name
            self.alpha_to_coverage = False
            self.independe_blend_enable = False
            self.rt0_blend_desc_name = ""
            self.rt1_blend_desc_name = ""
            self.rt2_blend_desc_name = ""
            self.rt3_blend_desc_name = ""
            self.rt4_blend_desc_name = ""
            self.rt5_blend_desc_name = ""
            self.rt6_blend_desc_name = ""
            self.rt7_blend_desc_name = ""
        
        def __eq__(self, other):
            return \
            self.name == other.name and \
            self.alpha_to_coverage == other.alpha_to_coverage and \
            self.independe_blend_enable == other.independe_blend_enable and \
            self.rt0_blend_desc_name == other.rt0_blend_desc_name and \
            self.rt1_blend_desc_name == other.rt1_blend_desc_name and \
            self.rt2_blend_desc_name == other.rt2_blend_desc_name and \
            self.rt3_blend_desc_name == other.rt3_blend_desc_name and \
            self.rt4_blend_desc_name == other.rt4_blend_desc_name and \
            self.rt5_blend_desc_name == other.rt5_blend_desc_name and \
            self.rt6_blend_desc_name == other.rt6_blend_desc_name and \
            self.rt7_blend_desc_name == other.rt7_blend_desc_name

    class RTBlendDesc:
        def __init__(self, name):
            self.name = name
            self.blend_enable = True
            self.logic_op_enable = False
            self.src_blend = 0
            self.dest_blend = 0
            self.blend_op = 0
            self.src_blend_alpha = 0
            self.dest_blend_alpha = 0
            self.blend_op_alpha = 0
            self.logic_op = 0
            self.render_target_write_mask = 0
        
        def __eq__(self, other):
            return \
            self.name == other.name and \
            self.blend_enable == other.blend_enable and \
            self.logic_op_enable == other.logic_op_enable and \
            self.src_blend == other.src_blend and \
            self.dest_blend == other.dest_blend and \
            self.blend_op == other.blend_op and \
            self.src_blend_alpha == other.src_blend_alpha and \
            self.dest_blend_alpha == other.dest_blend_alpha and \
            self.blend_op_alpha == other.blend_op_alpha and \
            self.logic_op == other.logic_op and \
            self.render_target_write_mask == other.render_target_write_mask

    def __init__(self, desc_type, databaase_path):
        self.database_path = databaase_path

        if desc_type == self.TYPE_BLEND_DESC:
            self.__decode = _decode_blend_desc
            self.__encode = _encode_blend_desc
            self.__table_name = "BlendDesc"
        elif desc_type == self.TYPE_RT_BLEND_DESC:
            self.__decode = _decode_rt_blend_desc
            self.__encode = _encode_rt_blend_desc
            self.__table_name = "RTBlendDesc"
        else:
            self.__decode = None
            self.__encode = None
            self.__table_name = None

    async def write(self, desc_to_write):
        conn = await aiosqlite.connect(self.database_path)
        cursor = await conn.cursor()

        #    asw = await cursor.execute("""SELECT Name FROM BlendDesc WHERE Name=?""", (blend_desc_to_write["name"],))
        #    name = await asw.fetchone()
        #    if name is not None:
        #        return False

        #print("Sended")

        await self.__encode(cursor, desc_to_write)

        await conn.commit()
        await conn.close()

        return True

    async def read(self, desc_name):
        conn = await aiosqlite.connect(self.database_path)
        cursor = await conn.cursor()

        query = 'SELECT * FROM {} WHERE Name=?'.format(self.__table_name)
        await cursor.execute(query, (desc_name,))

        desc_raw = await cursor.fetchone()
        if desc_raw is None:
            return None

        return self.__decode(desc_raw)

    async def read_all(self):
        conn = await aiosqlite.connect(self.database_path)
        cursor = await conn.cursor()

        query = 'SELECT * FROM {}'.format(self.__table_name)
        await cursor.execute(query)

        descs_raw = await cursor.fetchall()
        if descs_raw is None:
            return None

        descs = []

        for desc in descs_raw:
            desc = self.__decode(desc)
            descs.append(desc)

        return descs

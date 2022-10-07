
import sqlite3

try:
    from lib import devfactory
except ImportError:
    import devfactory


_TABLE_PARAMETER = "{TABLE_PARAMETER}"
_DROP_TABLE_SQL = f"DELETE FROM {_TABLE_PARAMETER};"
_GET_TABLES_SQL = "SELECT name FROM sqlite_schema WHERE type='table';"

def _delete_all_tables(con):
    tables = _get_tables(con)
    _delete_tables(con, tables)

def _get_tables(con):
    cur = con.cursor()
    cur.execute(_GET_TABLES_SQL)
    tables = cur.fetchall()
    cur.close()
    return tables

def _delete_tables(con, tables):
    cur = con.cursor()
    for table, in tables:
        sql = _DROP_TABLE_SQL.replace(_TABLE_PARAMETER, table)
        cur.execute(sql)
    cur.close()

def clear_db(database_path):
    conn = sqlite3.connect(database_path)
    _delete_all_tables(conn)
    conn.commit()
    conn.close()

def log_dev_header(database_path, dev_header, deviation_id):
    conn = sqlite3.connect(database_path)

    cursor = conn.cursor()
    cursor.execute("INSERT INTO DevHeaders VALUES (?, ?, ?, ?, ?)", \
        (deviation_id, dev_header.obj_name, dev_header.obj_type, dev_header.dev_type, dev_header.weight))
    conn.commit()
    conn.close()

def load_dev_headers(database_path, deviation_id):
    conn = sqlite3.connect(database_path)
    cursor = conn.cursor()

    cursor.execute("SELECT * FROM DevHeaders WHERE DeviationId=?", (deviation_id, ))
    dev_headers_raw = cursor.fetchall()
    conn.close()

    dev_headers = []
    for raw in dev_headers_raw:
        dev_headers.append(devfactory.DevHeader(raw[1], raw[2], raw[3], raw[4]))

    return dev_headers

def clear_dev_headers(database_path, deviation_id):
    conn = sqlite3.connect(database_path)
    cursor = conn.cursor()

    cursor.execute("DELETE FROM DevHeaders WHERE DeviationId=?", (deviation_id, ))
    conn.commit()
    conn.close()

def load_last_dev_id(database_path):
    conn = sqlite3.connect(database_path)
    cursor = conn.cursor()

    cursor.execute("SELECT Last_deviation_ID FROM DeviationData")
    dev_id_list = cursor.fetchone()
    conn.close()

    if dev_id_list is None:
        return None

    return dev_id_list[0]

def log_last_dev_id(database_path, deviation_id):
    conn = sqlite3.connect(database_path)
    cursor = conn.cursor()

    cursor.execute("DELETE FROM DeviationData")
    cursor.execute("INSERT INTO DeviationData VALUES (?, 0)", (deviation_id, ))

    conn.commit()
    conn.close()

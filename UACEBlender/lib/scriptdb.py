import sqlite3

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

def get_tables_header(database_path):
    conn = sqlite3.connect(database_path)
    cursor = conn.cursor()

    cursor.execute("SELECT * FROM table_headers")
    headers = cursor.fetchall()
    conn.close()

    return headers
    
def get_table_cols_desc(database_path, table_name):
    conn = sqlite3.connect(database_path)
    cursor = conn.cursor()

    cursor.execute("SELECT * FROM table_cols_desc WHERE TableName = ?", (table_name, ))
    cols = cursor.fetchall()
    conn.close()

    return cols
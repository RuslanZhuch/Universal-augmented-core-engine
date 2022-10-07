import asyncio
import aiosqlite

async def sqlite_func():

    conn = await aiosqlite.connect('Database.sqlite')

    cursor = await conn.cursor()

    try:

        await cursor.execute("""CREATE TABLE Humans_table ( 
            Name varchar(255), 
            Age int, 
            Status varchar(255) 
            )""")
            
    except:
        pass

    await cursor.execute("""INSERT INTO Humans_table VALUES (?, ?, ?)""", \
        ("Ruslan", 24, "Alive"))
    await cursor.execute("""INSERT INTO Humans_table VALUES (?, ?, ?)""", \
        ("Diana", 17, "Alive"))

    rows = await cursor.execute("SELECT * FROM Humans_table")

    async for row in rows:
        print(row)

    await conn.commit()
    await conn.close()
    
asyncio.run(sqlite_func())
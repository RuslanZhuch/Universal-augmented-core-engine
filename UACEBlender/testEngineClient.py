import asyncio
import aiosqlite
import socket
import json
from lib import bser
from lib import uacedb


class Chunk:
  def __init__(self, msg_data, other_data):
    self.msg_data = msg_data
    self.other_data = other_data

  def get_msg_data(self):
    return self.msg_data

  def get_other_data(self):
    return self.other_data

async def rec_tcp_pkg(reader, prev_data=bytearray()):
  
  total_rec_bytes = prev_data
  rec_size = len(total_rec_bytes)
  
  REC_HEADER_LEN = 8
  while rec_size < REC_HEADER_LEN:
    request = await reader.read(1024)
    if not request:
      return None
    rec_size = rec_size + len(request)
    total_rec_bytes.extend(request)

  header_bytes = total_rec_bytes[0:REC_HEADER_LEN]
  header_code = header_bytes[0:4]
  if header_code != b'\xFA\xFB\xFC\xFD':
    return None
  pkg_len = bser.unpack_int(header_bytes[4:REC_HEADER_LEN])
  if pkg_len <= 0:
    return None

  rec_pks_size = rec_size - REC_HEADER_LEN

  pkg_bytes = total_rec_bytes[8:rec_size]

  while rec_pks_size < pkg_len:
    request = await reader.read(1024)
    if not request:
      return None
    pkg_bytes.extend(request)
    rec_pks_size = len(pkg_bytes)

  pkg_data = pkg_bytes[0:pkg_len]
  other_data = pkg_bytes[pkg_len:rec_pks_size]

  return Chunk(pkg_data, other_data)

async def TCP_send_data(writer, data):
  data_len = len(data)
  header_bytes = b'\xFA\xFB\xFC\xFD'
  header_len = bser.pack_int(data_len)
  send_bytes = bytearray(header_bytes)
  send_bytes.extend(header_len)
  
  send_bytes.extend(data)
  
  writer.write(send_bytes)
  await writer.drain()

async def client_logic():

    deviation_id = 0

    reader, writer = await asyncio.open_connection('localhost', 50007)

    jd = json.dumps({\
    "ClientType": "CEngine",\
    "ClientName": "NameEngine0",\
    "LastDeviationId": deviation_id,\
    })

    await TCP_send_data(writer, str.encode(jd))

    chunk = Chunk(bytearray(), bytearray())

    while True:
        chunk = await rec_tcp_pkg(reader, chunk.get_other_data())
        if chunk is None:
            break

        msg = chunk.get_msg_data().decode('utf8')
        print(msg)
        jData = json.loads(msg)
        print(jData["PkgType"])
        objectName = jData["ObjName"]
        print(objectName)
        deviationType = jData["DeviationType"]
        print(deviationType)
        deviationId = jData["DeviationId"]
        print(deviationId)
        pbjectType = jData["ObjType"]
        print(pbjectType)

        chunk = await rec_tcp_pkg(reader, chunk.get_other_data())

        print("Blob data: ", chunk.get_msg_data())

        deviation_id = deviationId

        jd = json.dumps({\
        "PkgType": "DevId",\
        "LastDeviationId": deviation_id\
        })
        
        await TCP_send_data(writer, str.encode(jd))


    writer.close()
    await writer.wait_closed(); 


asyncio.run(client_logic())
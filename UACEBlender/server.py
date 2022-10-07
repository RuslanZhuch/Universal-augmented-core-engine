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
  if header_code != b'\xFD\xFC\xFB\xFA':
    return None
  pkg_len = bser.unpack_int(header_bytes[4:REC_HEADER_LEN])
  if pkg_len < 0:
    return None

  rec_pks_size = rec_size - REC_HEADER_LEN
  pkg_bytes = total_rec_bytes[8:rec_size]

  if pkg_len == 0:
    return Chunk(bytearray(), pkg_bytes)

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
  header_bytes = b'\xFD\xFC\xFB\xFA'
  header_len = bser.pack_int(data_len)
  send_bytes = bytearray(header_bytes)
  send_bytes.extend(header_len)
  
  send_bytes.extend(data)
  
  writer.write(send_bytes)
  await writer.drain()
  print("Data sended")
  #print("Data sended: ", data)

async def handle_blender_client(reader, writer, chunk):
  while True:
    chunk = await rec_tcp_pkg(reader, chunk.get_other_data())
    if chunk is None:
      break
    request = chunk.get_msg_data().decode('utf8')
    print(request)
    j_data = json.loads(request)
    print(j_data["PkgType"])
    object_name = j_data["ObjectName"]
    print(object_name)
    deviation_type = j_data["DeviationType"]
    print(deviation_type)
    deviation_id = j_data["DeviationId"]
    print(deviation_id)
    pbject_type = j_data["ObjectType"]
    print(pbject_type)

    chunk = await rec_tcp_pkg(reader, chunk.get_other_data())

    #matData = bser.deser(request, [bser.TYPE_MAT4X4])
    #print("Blob data: ", chunk.get_msg_data())

    await uacedb.log_new_deviation("database\Database.sqlite", j_data, chunk.get_msg_data())
    
  print("Connection closed")

async def handle_engine_client(reader, writer, chunk, deviation_id):
  
  while True:
    database_path = "database\Database.sqlite"
    deviation_id_to_send = await uacedb.get_nearest_dev_id(database_path, deviation_id)
    if deviation_id_to_send is None:
      deviation_id_to_send = deviation_id

    #print("Proxy deviaion is ", deviation_id_to_send)

    if deviation_id < deviation_id_to_send:
      print("Deviation found", deviation_id, deviation_id_to_send)
      jd_list, data_blob_list = await uacedb.get_deviation_data(database_path, deviation_id_to_send)
      for idx in range(len(jd_list)):
        jd = jd_list[idx]
        await TCP_send_data(writer, str.encode(json.dumps(jd)))

        data_blob = data_blob_list[idx]
        await TCP_send_data(writer, data_blob)
      print("dev data sent")

      #chunk = await rec_tcp_pkg(reader, chunk.get_other_data())
      #if chunk is None:
      #  break
      #msg = chunk.get_msg_data().decode('utf8')
      #print(msg)
      #j_data = json.loads(msg)
      #deviation_id = j_data["LastDeviationId"]
      deviation_id = deviation_id_to_send

    await asyncio.sleep(0.2)

  print("Connection closed")  

async def handle_client(reader, writer):
  prev_data = bytearray()
  chunk = await rec_tcp_pkg(reader, prev_data)
  if chunk is None:
    print("Failed to connect")
    return
  request = chunk.get_msg_data().decode('utf8')
  request = request.replace("\0", "")
  j_data = json.loads(request)
  client_type = j_data["ClientType"]
  client_name = j_data["ClientName"]
  last_dev_id = j_data["LastDeviationId"]
  print(f"New client connected, name: {client_name}, type: {client_type}, last dev id {last_dev_id}")

  if client_type == "CBlender":
    await handle_blender_client(reader, writer, chunk)
  elif client_type == "CEngine":
    await handle_engine_client(reader, writer, chunk, last_dev_id)

async def run_server():
  server = await asyncio.start_server(handle_client, 'localhost', 50007)
  async with server:
    await server.serve_forever()

asyncio.run(run_server())


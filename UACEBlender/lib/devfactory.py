import time
import queue

class Timeout():
    def __init__(self, max_time_sec):
        self.time_limit = max_time_sec
        self.start_time = 0

    def reset(self):
        self.start_time = self.__get_time()

    def get_expired(self):
        now = self.__get_time()
        return now - self.start_time > self.time_limit

    @staticmethod
    def __get_time():
        return time.time()


class DevQueueChecker():
    def __init__(self, timeout_lim):
        self.tout = Timeout(timeout_lim)

    def reset(self):
        self.tout.reset()

    def get_queue_complete(self, is_quque_full):
        if is_quque_full is True:
            return True

        return self.tout.get_expired()

class DevTypes:
    NONE = 0
    CREATE = 1
    TRANSFORMATION = 2
    PARAMETERS = 3
    DELETE = 4

class DevHeader:
    def __init__(self, obj_name, obj_type, dev_type, weight=1):
        self.obj_name = obj_name
        self.obj_type = obj_type
        self.dev_type = dev_type
        self.weight = weight

    def __eq__(self, o):
        return self.obj_name == o.obj_name and \
            self.obj_type == o.obj_type and \
            self.dev_type == o.dev_type and \
            self.weight == o.weight

class DevHeadersHolder:
    def __init__(self):
        self.header_obj_names = []
        self.header_obj_types = []
        self.header_dev_types = []
        self.header_weights = []
    
    def find_name_indices(self, name):
        return [i for i, x in enumerate(self.header_obj_names) if x == name]

    def find_same_idx_and_type(self, indices, obj_type, dev_type):
        for idx in indices:
            if self.header_obj_types[idx] == obj_type and \
                self.header_dev_types[idx] == dev_type:
                return True
        
        return False

    def append(self, dev_header):
        self.header_obj_names.append(dev_header.obj_name)
        self.header_obj_types.append(dev_header.obj_type)
        self.header_dev_types.append(dev_header.dev_type)
        self.header_weights.append(dev_header.weight)

    def pop(self, indices):
        indices.reverse()
        for idx in indices:
            self.header_obj_types.pop(idx)
        for idx in indices:
            self.header_dev_types.pop(idx)
        for idx in indices:
            self.header_obj_names.pop(idx)
        for idx in indices:
            self.header_weights.pop(idx)

    def pull_dev_header(self, id):
        if len(self.header_obj_names) == 0:
            return None
        return DevHeader(self.header_obj_names.pop(id), self.header_obj_types.pop(id), self.header_dev_types.pop(id), self.header_weights.pop(id))

    def len(self):
        return len(self.header_obj_names)

class DeviationList:
    
    def __init__(self, wlimit=0):
        self.headers = DevHeadersHolder()
        self.max_wlimit = wlimit
        self.wlimit_left = self.max_wlimit

    def append(self, dev_header):
        if (self.max_wlimit > 0) and (self.wlimit_left <= 0):
            return

        same_name_indices = self.headers.find_name_indices(dev_header.obj_name)
        if self.headers.find_same_idx_and_type(same_name_indices, dev_header.obj_type, dev_header.dev_type):
            return

        self.wlimit_left = self.wlimit_left - dev_header.weight

        if dev_header.dev_type == DevTypes.DELETE:
            self.headers.pop(same_name_indices)

        self.headers.append(dev_header)

    def pull(self):
        dev_header = self.headers.pull_dev_header(0)
        self.wlimit_left = self.wlimit_left + dev_header.weight
        return dev_header

    def len(self):
        return self.headers.len()

    def wleft(self):
        return self.wlimit_left

class DevQueuePool():
    def __init__(self, num_of_queues, wlimit, time_limit):
        self.write_candidants = queue.Queue()
        self.read_candidants = queue.Queue()
        self.read_candidants_dev_id = queue.Queue()

        self.write_deviation_id = None
        self.current_write_list = None
        self.b_write_started = False

        self.current_read_list = None

        self.dev_checker = DevQueueChecker(time_limit)

        for idx in range(num_of_queues):
            self.write_candidants.put_nowait(DeviationList(wlimit))

    def init_queue_write(self, deviation_id):
        self.complete_write_queue()

        if self.write_candidants.empty():
            return False

        self.current_write_list = self.write_candidants.get()

        self.write_deviation_id = deviation_id

        self.b_write_started = False
        self.dev_checker.reset()

        return True

    def complete_write_queue(self):
        if self.current_write_list is None:
            return

        self.read_candidants_dev_id.put_nowait(self.write_deviation_id)
        self.read_candidants.put_nowait(self.current_write_list)

        self.current_write_list = None

    def append(self, dev_header):
        if self.current_write_list is None:
            return False

        if self.current_write_list.wleft() <= 0:
            return False

        self.b_write_started = True

        self.current_write_list.append(dev_header)
        self.dev_checker.reset()

        return True

    def get_write_queue_full(self):
        if self.current_write_list is None:
            return False

        return self.current_write_list.wleft() <= 0
    
    def init_queue_read(self):
        if self.read_candidants.empty():
            return None

        self.current_read_list = self.read_candidants.get()
        return self.read_candidants_dev_id.get()
        
    def read(self):
        if self.current_read_list is None:
            return None
        if self.current_read_list.len() <= 0:
            return None

        dev_header = self.current_read_list.pull()
        if self.current_read_list.len() == 0:
            self.write_candidants.put_nowait(self.current_read_list)
            self.current_read_list = None

        return dev_header

    def update(self):
        if self.current_write_list is None:
            return False

        b_space_left = self.current_write_list.wleft() > 0
        if self.b_write_started:
            is_expired = self.dev_checker.get_queue_complete(not b_space_left)
            if is_expired is False:
                return False
        else:
            if b_space_left:
                return False

        self.complete_write_queue()
        return True

class ResolveIds:
    DISCARD = 0
    CREATE_CAMERA = 1
    CREATE_STATIC_MESH = 2
    CAMERA_UPDATE = 3
    DELETE = 4
    TRANSFORMATION = 5
    GEOMETRY = 6

class DeviationDataGatherer:
    def __init__(self):
        self.gather_list = [\
            lambda obj_name: None,\
            lambda obj_name: None,\
            lambda obj_name: None,\
            lambda obj_name: None,\
            lambda obj_name: None,\
            lambda obj_name: None,\
            lambda obj_name: None
            ]

    def setup_resolve(self, resolve_id, cb):
        if (resolve_id < 0) or (resolve_id >= len(self.gather_list)):
            return False

        self.gather_list[resolve_id] = cb
        return True

    def get_dev_data(self, obj_name, resolve_id):
        if (resolve_id < 0) or (resolve_id >= len(self.gather_list)):
            return None

        return self.gather_list[resolve_id](obj_name)

def resolve_header_id(dev_header):
    #For simplification purpose
    #Remove later
    #if dev_header.obj_type != 'CAMERA':
    #    return ResolveIds.DISCARD
    ##

    dev_type = dev_header.dev_type

    if dev_type == DevTypes.CREATE:
        if dev_header.obj_type == 'MESH':
            return ResolveIds.CREATE_STATIC_MESH
        elif dev_header.obj_type == 'CAMERA':
            return ResolveIds.CREATE_CAMERA
    elif dev_type == DevTypes.DELETE:
        return ResolveIds.DELETE
    elif dev_type == DevTypes.NONE:
        return ResolveIds.DISCARD

    obj_type = dev_header.obj_type
    if obj_type == 'CAMERA':
        return ResolveIds.CAMERA_UPDATE

    if dev_type == DevTypes.TRANSFORMATION:
        return ResolveIds.TRANSFORMATION

    if obj_type == 'MESH':
        if dev_type == DevTypes.PARAMETERS:
            return ResolveIds.GEOMETRY

    return ResolveIds.DISCARD

def gen_obj_create_pkg(object_name, object_type, deviation_id):
    return {\
        "PkgType": "Deviation",\
        "DeviationType": "Creation",\
        "ObjectName": object_name, \
        "DeviationId": deviation_id,\
        "ObjectType": object_type,\
        }

def gen_obj_delete_pkg(object_name, object_type, deviation_id):
    return {\
        "PkgType": "Deviation",\
        "DeviationType": "Deletion",\
        "ObjectName": object_name, \
        "DeviationId": deviation_id,\
        "ObjectType": object_type,\
        }

def gen_geometry_pkg(object_name, deviation_id):
    return {\
        "PkgType": "Deviation",\
        "DeviationType": "Geometry",\
        "ObjectName": object_name, \
        "DeviationId": deviation_id,\
        "ObjectType": "MESH",\
        }    

def gen_transform_pkg(object_name, object_type, deviation_id):
    return {\
        "PkgType": "Deviation",\
        "DeviationType": "Transform",\
        "ObjectName": object_name, \
        "DeviationId": deviation_id,\
        "ObjectType": object_type,\
        }

def gen_camera_update_pkg(object_name, deviation_id):
    return {\
        "PkgType": "Deviation",\
        "DeviationType": "Camera",\
        "ObjectName": object_name, \
        "DeviationId": deviation_id,\
        "ObjectType": "CAMERA",\
        }
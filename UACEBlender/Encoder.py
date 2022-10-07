import bpy
import json

#print(json.dumps({"val0" : 0, "val1" : 1}))
selected_objects = bpy.context.selected_objects
print(selected_objects)
sel_names = []
for obj in selected_objects:
    sel_names.append(obj.name)
if 'Cube2' in sel_names:
    print("exist")
    


def getObjectsDiff(curr_names, prev_names):
    lens_diff = len(curr_names) - len(prev_names)
#    if lens_diff == 0:
#        return [], []
    
    removed_names = []
    for prev_name in prev_names:
        if curr_names.count(prev_name) == 0:
            removed_names.append(prev_name)

    added_names = []
    for curr_name in curr_names:
        if prev_names.count(curr_name) == 0:
            added_names.append(curr_name)        
        
    return added_names, removed_names




prev_selected_names = ["Cube", "Cube.001", "Cobe.002"]

added_names, removed_names = getObjectsDiff(sel_names, prev_selected_names)

print("Removed: ", removed_names)
print("Added: ", added_names)
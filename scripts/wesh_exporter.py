bl_info = {
    "name": "Wesh Format Exporter",
    "description": "Writes a .wesh geometry file format to disk",
    "author": "ndoxx (ErwinEngine)",
    "version": (0, 1),
    "blender": (2, 83, 0),
    "location": "File > Export > Wesh",
    "warning": "",
    "wiki_url": "",
    "tracker_url": "",
    "support": 'COMMUNITY',
    "category": "Import-Export"
}

# Based on:
# https://www.gamedev.net/blogs/entry/2266917-blender-mesh-export-script/
# -> Made it 2.8x friendly

import bpy
import bmesh
import struct # https://docs.python.org/3/library/struct.html
from bpy import context

def triangulateObject(obj):
    me = obj.data
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.triangulate(bm, faces=bm.faces[:], quad_method='BEAUTY', ngon_method='BEAUTY')
    bm.to_mesh(me)
    bm.free()
    
class VertexInterleaved:
    position = []
    uv       = []
    normal   = []

def different(a, b):
    return bool(abs(a-b) > max(1e-09 * max(abs(a), abs(b)), 0.0))

def findVertex(thisVertex, vertBuff):
    for i, v in enumerate(vertBuff):
        if(different(v.position.x, thisVertex.position.x)):
            continue
        if(different(v.position.y, thisVertex.position.y)):
            continue
        if(different(v.position.z, thisVertex.position.z)):
            continue
        if(different(v.uv.x, thisVertex.uv.x)):
            continue
        if(different(v.uv.y, thisVertex.uv.y)):
            continue
        return (True, i)
    return (False, 0)

def writeObject(self, context):
    ob = context.object
    uv_layer = ob.data.uv_layers.active.data
    
    vertBuff = []
    faceBuff = []
    # rebuild vertex, uv and face indices excluding duplicates
    for poly in ob.data.polygons:
        for index in poly.loop_indices:
            thisVertex = VertexInterleaved()
            thisVertex.position = ob.data.vertices[ob.data.loops[index].vertex_index].co
            thisVertex.uv       = uv_layer[index].uv
            thisVertex.normal   = ob.data.vertices[ob.data.loops[index].vertex_index].normal
            
            # if vertex already in list add its index to faceBuff, otherwise stash a new vertex
            inList, idx = findVertex(thisVertex, vertBuff)
            if(inList):
                faceBuff.append(int(idx))
            else:
                faceBuff.append(len(vertBuff)) #index
                vertBuff.append(thisVertex)
                
    # write to file
    if(self.format == "OPT_A"):
        with open(self.filepath, 'w') as ofile:
            ofile.write("%d " % len(vertBuff)) # num unique vertex/uv pairs
            ofile.write("%d " % len(faceBuff)) # num indices
            for v in vertBuff:
                ofile.write("%f %f %f " % v.position[:])
                ofile.write("%f %f " % v.uv[:])
                ofile.write("%f %f %f " % v.normal[:])
            for p in faceBuff:
                ofile.write("%d " % p)
            ofile.close()
        return {'FINISHED'}
    else:
        
        with open(self.filepath, 'wb') as ofile:
            ofile.write(struct.pack('H', len(vertBuff))) 
            ofile.write(struct.pack('H', len(faceBuff)))
        
            for v in vertBuff:
                ofile.write(struct.pack('3f', v.position.x, v.position.y, v.position.z))
                ofile.write(struct.pack('2f', v.uv.x, v.uv.y))
                ofile.write(struct.pack('3f', v.normal.x, v.normal.y, v.normal.z))
            for p in faceBuff:
                ofile.write(struct.pack('H', p))
            ofile.close()
        return {'FINISHED'}    



class ObjectExport(bpy.types.Operator):
    """My object export script"""
    bl_idname = "object.export_wesh"
    bl_label = "Wesh Format Export"
    bl_options = {'REGISTER', 'UNDO'}
    filename_ext = ".wesh"
    
    total           : bpy.props.IntProperty(name="Steps", default=2, min=1, max=100)
    filter_glob     : bpy.props.StringProperty(default="*.wesh", options={'HIDDEN'}, maxlen=255)
    use_setting     : bpy.props.BoolProperty(name="Selected only", description="Export selected mesh items only", default=True)
    use_triangulate : bpy.props.BoolProperty(name="Triangulate", description="Triangulate object", default=True)
    format          : bpy.props.EnumProperty(name="Format", description="Choose between two items", items=(('OPT_A', "ASCII ", "Text file format"), ('OPT_B', "Binary", "Binary file format")), default='OPT_A')

    filepath : bpy.props.StringProperty(subtype='FILE_PATH')    
    
    def execute(self, context):
        if(context.active_object.mode == 'EDIT'):
            bpy.ops.object.mode_set(mode='OBJECT')
            
        if(self.use_triangulate):
            triangulateObject(context.active_object)
            
        writeObject(self, context);        
        return {'FINISHED'}

    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}



# Add trigger into a dynamic menu
def menu_func_export(self, context):
    self.layout.operator(ObjectExport.bl_idname, text="Wesh Export (.wesh)")
    

def register():
    bpy.utils.register_class(ObjectExport)
    #bpy.types.VIEW3D_MT_object.append(menu_func_export)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_class(ObjectExport)
    #bpy.types.VIEW3D_MT_object.remove(menu_func_export)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)


if __name__ == "__main__":
    register()
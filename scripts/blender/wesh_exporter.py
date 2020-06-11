bl_info = {
    "name": "Wesh Format Exporter",
    "description": "Writes a .wesh geometry file format to disk",
    "author": "ndoxx (ErwinEngine)",
    "version": (0, 3),
    "blender": (2, 83, 0),
    "location": "File > Export > Wesh",
    "warning": "",
    "wiki_url": "",
    "tracker_url": "",
    "support": 'COMMUNITY',
    "category": "Import-Export"
}

# Inspired by:
# https://www.gamedev.net/blogs/entry/2266917-blender-mesh-export-script/
# -> 2.8x friendly
# -> Binary header with version information for engine compatibility check
# -> Interleaved vertex data with tangent space
# -> Custom file naming scheme with ExportHelper mixin

import bpy
import bmesh
import mathutils
import struct # https://docs.python.org/3/library/struct.html
from bpy import context
from bpy_extras.io_utils import ExportHelper

def triangulateObject(obj):
    me = obj.data
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.triangulate(bm, faces=bm.faces[:], quad_method='BEAUTY', ngon_method='BEAUTY')
    bm.to_mesh(me)
    bm.free()


class VertexInterleaved:
    position = None
    normal   = None
    tangent  = None
    uv       = None


class Extent:
    def __init__(self):
        self.xmin = float("inf")
        self.xmax = float("-inf")
        self.ymin = float("inf")
        self.ymax = float("-inf")
        self.zmin = float("inf")
        self.zmax = float("-inf")

    def update(self, position):
        self.xmin = min(self.xmin, position.x)
        self.xmax = max(self.xmax, position.x)
        self.ymin = min(self.ymin, position.y)
        self.ymax = max(self.ymax, position.y)
        self.zmin = min(self.zmin, position.z)
        self.zmax = max(self.zmax, position.z)


class WeshHeader:
    def __init__(self, versionMajor, versionMinor, vertSize, vertCount, idxCount):
        self.magic = 0x48534557 # ASCII(WESH)
        self.versionMajor = versionMajor
        self.versionMinor = versionMinor
        self.vertSize = vertSize
        self.vertCount = vertCount
        self.idxCount = idxCount


def findVertex(thisVertex, vertBuff):
    def different(a, b):
        return bool(abs(a-b) > max(1e-09 * max(abs(a), abs(b)), 0.0))

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
        if(different(v.normal.x, thisVertex.normal.x)):
            continue
        if(different(v.normal.y, thisVertex.normal.y)):
            continue
        if(different(v.normal.z, thisVertex.normal.z)):
            continue
        return (True, i)
    return (False, 0)


def WBasis(vec):
    return mathutils.Vector((-vec.x, vec.z, vec.y))
        

def writeObject(exporter, context):
    me = context.active_object.data
    
    vertBuff = []
    faceBuff = []

    me.calc_tangents()
    # Must be declared after calc_tangents() which seems to have a
    # side effect on active UV layer
    uv_layer = me.uv_layers.active.data

    # Mesh spatial extent
    extent = Extent()

    # rebuild vertex, uv and face indices excluding duplicates
    # loop faces
    for face in me.polygons:
        # loop over face loop
        for meshloop in [me.loops[index] for index in face.loop_indices]:
            vert_index = meshloop.vertex_index
            thisVertex = VertexInterleaved()
            thisVertex.position = WBasis(me.vertices[vert_index].co)
            thisVertex.normal   = WBasis(meshloop.normal)
            thisVertex.tangent  = WBasis(meshloop.tangent)
            thisVertex.uv       = uv_layer[meshloop.index].uv

            # if vertex already in list add its index to faceBuff, otherwise stash a new vertex
            inList, idx = findVertex(thisVertex, vertBuff)
            if(inList):
                faceBuff.append(int(idx))
            else:
                faceBuff.append(len(vertBuff)) #index
                vertBuff.append(thisVertex)
                extent.update(thisVertex.position)

    # write to file
    if(exporter.format == "OPT_A"): # ASCII
        with open(exporter.filepath, 'w') as ofile:
            ofile.write("%d " % len(vertBuff)) # num unique vertex/uv pairs
            ofile.write("%d " % len(faceBuff)) # num indices
            ofile.write("\n")
            for v in vertBuff:
                ofile.write("%f %f %f " % v.position[:])
                ofile.write("%f %f %f " % v.normal[:])
                ofile.write("%f %f %f " % v.tangent[:])
                ofile.write("%f %f " % v.uv[:])
                ofile.write("\n")
            for p in faceBuff:
                ofile.write("%d " % p)
            ofile.close()
        return {'FINISHED'}
    else: # Binary
        # Create header
        global bl_info
        hh = WeshHeader(bl_info["version"][0], bl_info["version"][1], 11, len(vertBuff), len(faceBuff))

        with open(exporter.filepath, 'wb') as ofile:
            ofile.write(struct.pack('IHHIII', hh.magic, hh.versionMajor, hh.versionMinor, hh.vertSize, hh.vertCount, hh.idxCount))
            ofile.write(struct.pack('6f', extent.xmin, extent.xmax, extent.ymin, extent.ymax, extent.zmin, extent.zmax))
            for v in vertBuff:
                ofile.write(struct.pack('3f', v.position.x, v.position.y, v.position.z))
                ofile.write(struct.pack('3f', v.normal.x, v.normal.y, v.normal.z))
                ofile.write(struct.pack('3f', v.tangent.x, v.tangent.y, v.tangent.z))
                ofile.write(struct.pack('2f', v.uv.x, v.uv.y))
            for p in faceBuff:
                ofile.write(struct.pack('I', p))
            ofile.close()
        return {'FINISHED'}    


# Using ExportHelper mixin to override default file name
class ObjectExport(bpy.types.Operator, ExportHelper):
    """Wesh export script"""
    bl_idname = "object.export_wesh"
    bl_label = "Wesh Format Export"
    bl_options = {'REGISTER', 'UNDO'}
    filename_ext = ".wesh"
    
    total       : bpy.props.IntProperty(name="Steps", default=2, min=1, max=100)
    filter_glob : bpy.props.StringProperty(default="*.wesh", options={'HIDDEN'}, maxlen=255)
    use_setting : bpy.props.BoolProperty(name="Selected only", description="Export selected mesh items only", default=True)
    format      : bpy.props.EnumProperty(name="Format", description="Choose between two items", items=(('OPT_A', "ASCII ", "Text file format"), ('OPT_B', "Binary", "Binary file format")), default='OPT_B')
    filepath    : bpy.props.StringProperty(subtype='FILE_PATH')    

    def execute(self, context):
        if(context.active_object.mode == 'EDIT'):
            bpy.ops.object.mode_set(mode='OBJECT')
            
        triangulateObject(context.active_object)
            
        writeObject(self, context);        
        return {'FINISHED'}

    def invoke(self, context, event):
        # Generate file name with .wesh extension
        import os
        if not self.filepath:
            blend_filepath = context.blend_data.filepath
            if not blend_filepath:
                blend_filepath = "untitled"
            else:
                blend_filepath = os.path.splitext(blend_filepath)[0]

            self.filepath = blend_filepath + self.filename_ext

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
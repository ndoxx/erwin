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
    tangent  = []

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
        if(different(v.normal.x, thisVertex.normal.x)):
            continue
        if(different(v.normal.y, thisVertex.normal.y)):
            continue
        if(different(v.normal.z, thisVertex.normal.z)):
            continue
        return (True, i)
    return (False, 0)

def writeObject(context):
    triangulateObject(context.active_object)
    
    me = context.active_object.data
    uv_layer = me.uv_layers.active.data
    vertBuff = []
    faceBuff = []

    me.calc_tangents()

    # rebuild vertex, uv and face indices excluding duplicates
    # loop faces
    for face in me.polygons:
        # loop over face loop
        for meshloop in [me.loops[index] for index in face.loop_indices]:
            vert_index = meshloop.vertex_index
            thisVertex = VertexInterleaved()
            thisVertex.position = me.vertices[vert_index].co
            thisVertex.uv       = uv_layer[meshloop.index].uv
            thisVertex.normal   = meshloop.normal
            thisVertex.tangent  = meshloop.tangent

            # if vertex already in list add its index to faceBuff, otherwise stash a new vertex
            inList, idx = findVertex(thisVertex, vertBuff)
            if(inList):
                faceBuff.append(int(idx))
            else:
                faceBuff.append(len(vertBuff)) #index
                vertBuff.append(thisVertex)
                
    # write to file
    with open('/home/ndx/dev/Erwin_rel/Assets/work/models/cube.wesh', 'w') as ofile:
        ofile.write("%d " % len(vertBuff)) # num unique vertex/uv pairs
        ofile.write("%d " % len(faceBuff)) # num indices
        ofile.write("\n")
        for v in vertBuff:
            ofile.write("%f %f %f  " % v.position[:])
            ofile.write("%f %f  " % v.uv[:])
            ofile.write("%f %f %f  " % v.normal[:])
            ofile.write("%f %f %f  " % v.tangent[:])
            ofile.write("\n")
        for p in faceBuff:
            ofile.write("%d " % p)
        ofile.close()
    return {'FINISHED'}

if __name__ == "__main__":
    writeObject(context)
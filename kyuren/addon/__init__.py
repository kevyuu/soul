import bpy
import bgl
import sys
from . import pykyuren
import threading
from datetime import datetime
from datetime import timedelta
from . import utils
import math

KYUREN_NULL_MESH_ID = -1
KYUREN_NULL_ID = -1
NON_DEFORMING_MODIFIERS = {"COLLISION", "PARTICLE_INSTANCE", "PARTICLE_SYSTEM", "SMOKE"}

bl_info = {
    "name": "KyuRen",
    "blender": (2, 80, 0),
    "category": "Object",
}

start_time = datetime.now()
def millis():
   dt = datetime.now() - start_time
   ms = (dt.days * 24 * 60 * 60 + dt.seconds) * 1000 + dt.microseconds / 1000.0
   return ms

class KyuRenderEngine(bpy.types.RenderEngine):
    # These three members are used by blender to set up the
    # RenderEngine; define its internal name, visible name and capabilities.
    bl_idname = "Kyuren"
    bl_label = "Kyuren"
    bl_use_preview = True

    # Init is called whenever a new render engine instance is created. Multiple
    # instances may exist at the same time, for example for a viewport and final
    # render.
    def __init__(self):
        self.scene_data = None
        self.draw_data = None
        print(sys.version)
        self.session = None
        print(threading.get_ident())

    # When the render engine instance is destroy, this is called. Clean up any
    # render engine data here, for example stopping running render threads.
    def __del__(self):
        if self.session:
            pykyuren.exit(self.session)
            del self.session

    def _sync_entities(self, depsgraph):

        for update in depsgraph.updates:
             if type(update.id) == bpy.types.Object and update.is_updated_geometry:
                obj = update.id
                if obj.type == "MESH":
                    mesh = obj.to_mesh()
                    mesh.calc_loop_triangles()
                    mesh.calc_normals_split()
                    pykyuren.update_mesh(self.session, obj.as_pointer(), mesh.as_pointer())
                    obj.to_mesh_clear()
                elif obj.type == "LIGHT":
                    light_id = obj.original.kyuren_id
                    pykyuren.update_light(self.session, obj.as_pointer(), obj.original.data.as_pointer())

        start_sync = millis()
        pykyuren.sync_depsgraph(self.session, depsgraph.as_pointer())
        delta_sync = millis() - start_sync
        print(f"Delta sync : {delta_sync}")
                
    def _init_entities(self, depsgraph):
        for datablock in depsgraph.ids:
            if type(datablock) == bpy.types.Object:
                obj = datablock
                if obj.type == "MESH":
                    mesh = obj.to_mesh()
                    mesh.calc_loop_triangles()
                    mesh.calc_normals_split()
                    pykyuren.update_mesh(self.session, obj.as_pointer(), mesh.as_pointer())
                    obj.to_mesh_clear()
                elif obj.type == "LIGHT":
                    light_id = obj.original.kyuren_id
                    
                    pykyuren.update_light(self.session, obj.as_pointer(), obj.original.data.as_pointer())

        start_sync = millis()
        pykyuren.sync_depsgraph(self.session, depsgraph.as_pointer())
        delta_sync = millis() - start_sync
        print(f"Delta sync : {delta_sync}")

    # This is the method called by Blender for both final renders (F12) and
    # small preview for materials, world and lights.
    def render(self, depsgraph):
        scene = depsgraph.scene
        scale = scene.render.resolution_percentage / 100.0
        self.size_x = int(scene.render.resolution_x * scale)
        self.size_y = int(scene.render.resolution_y * scale)

        # Fill the render result with a flat color. The framebuffer is
        # defined as a list of pixels, each pixel itself being a list of
        # R,G,B,A values.
        if self.is_preview:
            color = [0.1, 0.2, 0.1, 1.0]
        else:
            color = [0.2, 0.1, 0.1, 1.0]

        pixel_count = self.size_x * self.size_y
        rect = [color] * pixel_count

        # Here we write the pixel values to the RenderResult
        result = self.begin_result(0, 0, self.size_x, self.size_y)
        layer = result.layers[0].passes["Combined"]
        layer.rect = rect
        self.end_result(result)

    # For viewport renders, this method gets called once at the start and
    # whenever the scene or 3D viewport changes. This method is where data
    # should be read from Blender in the same thread. Typically a render
    # thread will be started to do the work while keeping Blender responsive.
    def view_update(self, context, depsgraph):
       
        if not self.session:
            # First time initialization
            self.session = pykyuren.init()
            self._init_entities(depsgraph)
        else:
            self._sync_entities(depsgraph)

    def _sync_camera(self, context, scene):
        view_cam_type = context.region_data.view_perspective

        if view_cam_type == "ORTHO":
            raise NotImplementedError("")
        elif view_cam_type == "PERSP":
            zoom = 2.25
            width_raw, height_raw = utils.calc_filmsize_raw(scene, context)
            screen_window = utils.calc_screenwindow(zoom, 0, 0, scene, context)
            fov = 2 * math.atan(16 / context.space_data.lens)
            pykyuren.set_perspective_camera(self.session, context.region_data.view_matrix.copy(), context.region_data.window_matrix.copy())
        elif view_cam_type == "CAMERA":
            raise NotImplementedError("")
        else:
            raise NotImplementedError("Unknown context.region_data.view_perspective")
           
    # For viewport renders, this method is called whenever Blender redraws
    # the 3D viewport. The renderer is expected to quickly draw the render
    # with OpenGL, and not perform other expensive work.
    # Blender will draw overlays for selection and editing on top of the
    # rendered image automatically.
    def view_draw(self, context, depsgraph):
            
        region = context.region
        scene = depsgraph.scene_eval

        self._sync_camera(context, scene)

        # Get viewport dimensions
        dimensions = region.width, region.height

        start_draw = millis()
        pixels = pykyuren.draw(self.session, region.width, region.height)
        delta_draw = millis() - start_draw
        print(f"Delta draw : {delta_draw}")

        # Bind shader that converts from scene linear to display space,
        bgl.glEnable(bgl.GL_BLEND)
        bgl.glBlendFunc(bgl.GL_ONE, bgl.GL_ONE_MINUS_SRC_ALPHA)
        self.bind_display_space_shader(scene)
        self.draw_data = CustomDrawData(dimensions, pixels)
        self.draw_data.draw()

        self.unbind_display_space_shader()
        bgl.glDisable(bgl.GL_BLEND)

class CustomDrawData:
    def __init__(self, dimensions, pixelBuffer):
        # Generate dummy float image buffer
        self.dimensions = dimensions
        width, height = dimensions

        pixels = bgl.Buffer(bgl.GL_BYTE, width * height * 4, pixelBuffer)

        # Generate texture
        self.texture = bgl.Buffer(bgl.GL_INT, 1)
        bgl.glGenTextures(1, self.texture)
        bgl.glActiveTexture(bgl.GL_TEXTURE0)
        bgl.glBindTexture(bgl.GL_TEXTURE_2D, self.texture[0])
        bgl.glTexImage2D(bgl.GL_TEXTURE_2D, 0, bgl.GL_RGBA, width, height, 0, bgl.GL_RGBA, bgl.GL_UNSIGNED_BYTE, pixels)
        bgl.glTexParameteri(bgl.GL_TEXTURE_2D, bgl.GL_TEXTURE_MIN_FILTER, bgl.GL_LINEAR)
        bgl.glTexParameteri(bgl.GL_TEXTURE_2D, bgl.GL_TEXTURE_MAG_FILTER, bgl.GL_LINEAR)
        bgl.glBindTexture(bgl.GL_TEXTURE_2D, 0)

        # Bind shader that converts from scene linear to display space,
        # use the scene's color management settings.
        shader_program = bgl.Buffer(bgl.GL_INT, 1)
        bgl.glGetIntegerv(bgl.GL_CURRENT_PROGRAM, shader_program)

        # Generate vertex array
        self.vertex_array = bgl.Buffer(bgl.GL_INT, 1)
        bgl.glGenVertexArrays(1, self.vertex_array)
        bgl.glBindVertexArray(self.vertex_array[0])

        texturecoord_location = bgl.glGetAttribLocation(shader_program[0], "texCoord")
        position_location = bgl.glGetAttribLocation(shader_program[0], "pos")

        bgl.glEnableVertexAttribArray(texturecoord_location)
        bgl.glEnableVertexAttribArray(position_location)

        # Generate geometry buffers for drawing textured quad
        position = [0.0, 0.0, width, 0.0, width, height, 0.0, height]
        position = bgl.Buffer(bgl.GL_FLOAT, len(position), position)
        texcoord = [0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0]
        texcoord = bgl.Buffer(bgl.GL_FLOAT, len(texcoord), texcoord)

        self.vertex_buffer = bgl.Buffer(bgl.GL_INT, 2)

        bgl.glGenBuffers(2, self.vertex_buffer)
        bgl.glBindBuffer(bgl.GL_ARRAY_BUFFER, self.vertex_buffer[0])
        bgl.glBufferData(bgl.GL_ARRAY_BUFFER, 32, position, bgl.GL_STATIC_DRAW)
        bgl.glVertexAttribPointer(position_location, 2, bgl.GL_FLOAT, bgl.GL_FALSE, 0, None)

        bgl.glBindBuffer(bgl.GL_ARRAY_BUFFER, self.vertex_buffer[1])
        bgl.glBufferData(bgl.GL_ARRAY_BUFFER, 32, texcoord, bgl.GL_STATIC_DRAW)
        bgl.glVertexAttribPointer(texturecoord_location, 2, bgl.GL_FLOAT, bgl.GL_FALSE, 0, None)

        bgl.glBindBuffer(bgl.GL_ARRAY_BUFFER, 0)
        bgl.glBindVertexArray(0)

    def __del__(self):
        bgl.glDeleteBuffers(2, self.vertex_buffer)
        bgl.glDeleteVertexArrays(1, self.vertex_array)
        bgl.glBindTexture(bgl.GL_TEXTURE_2D, 0)
        bgl.glDeleteTextures(1, self.texture)

    def draw(self):
        bgl.glActiveTexture(bgl.GL_TEXTURE0)
        bgl.glBindTexture(bgl.GL_TEXTURE_2D, self.texture[0])
        bgl.glBindVertexArray(self.vertex_array[0])
        bgl.glDrawArrays(bgl.GL_TRIANGLE_FAN, 0, 4)
        bgl.glBindVertexArray(0)
        bgl.glBindTexture(bgl.GL_TEXTURE_2D, 0)

def register():
    pykyuren.on_register(
        objectRNA=bpy.types.Object.bl_rna.as_pointer(),
        meshRNA=bpy.types.Mesh.bl_rna.as_pointer(),
        depsgraphRNA=bpy.types.Depsgraph.bl_rna.as_pointer()
    )

    bpy.types.Object.kyuren_id = bpy.props.IntProperty(name="Kyuren ID", default=KYUREN_NULL_ID)

    # Register the RenderEngine
    bpy.utils.register_class(KyuRenderEngine)

def unregister():
    bpy.utils.unregister_class(KyuRenderEngine)

if __name__ == "__main__":
    register()
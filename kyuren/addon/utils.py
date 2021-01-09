import math

def calc_filmsize_raw(scene, context=None):
    if context:
        # Viewport render
        width = context.region.width
        height = context.region.height
    else:
        # Final render
        scale = scene.render.resolution_percentage / 100
        width = int(scene.render.resolution_x * scale)
        height = int(scene.render.resolution_y * scale)

    return width, height


def calc_filmsize(scene, context=None):
    render = scene.render
    border_min_x, border_max_x, border_min_y, border_max_y = calc_blender_border(scene, context)
    width_raw, height_raw = calc_filmsize_raw(scene, context)
    
    if context:
        # Viewport render        
        width = width_raw
        height = height_raw
        if context.region_data.view_perspective in ("ORTHO", "PERSP"):            
            width = int(width_raw * border_max_x) - int(width_raw * border_min_x)
            height = int(height_raw * border_max_y) - int(height_raw * border_min_y)
        else:
            # Camera viewport
            zoom = 0.25 * ((math.sqrt(2) + context.region_data.view_camera_zoom / 50) ** 2)
            aspectratio, aspect_x, aspect_y = calc_aspect(render.resolution_x * render.pixel_aspect_x,
                                                          render.resolution_y * render.pixel_aspect_y,
                                                          scene.camera.data.sensor_fit)

            if render.use_border:
                base = zoom
                if scene.camera.data.sensor_fit == "AUTO":
                    base *= max(width, height)
                elif scene.camera.data.sensor_fit == "HORIZONTAL":
                    base *= width
                elif scene.camera.data.sensor_fit == "VERTICAL":
                    base *= height

                width = int(base * aspect_x * border_max_x) - int(base * aspect_x * border_min_x)
                height = int(base * aspect_y * border_max_y) - int(base * aspect_y * border_min_y)

        pixel_size = int(scene.luxcore.viewport.pixel_size)
        width //= pixel_size
        height //= pixel_size
    else:
        # Final render
        width = int(width_raw * border_max_x) - int(width_raw * border_min_x)
        height = int(height_raw * border_max_y) - int(height_raw * border_min_y)

    # Make sure width and height are never zero
    # (can e.g. happen if you have a small border in camera viewport and zoom out a lot)
    width = max(2, width)
    height = max(2, height)

    return width, height


def calc_blender_border(scene, context=None):
    render = scene.render

    if context and context.region_data.view_perspective in ("ORTHO", "PERSP"):
        # Viewport camera
        border_max_x = context.space_data.render_border_max_x
        border_max_y = context.space_data.render_border_max_y
        border_min_x = context.space_data.render_border_min_x
        border_min_y = context.space_data.render_border_min_y
    else:
        # Final camera
        border_max_x = render.border_max_x
        border_max_y = render.border_max_y
        border_min_x = render.border_min_x
        border_min_y = render.border_min_y

    if context and context.region_data.view_perspective in ("ORTHO", "PERSP"):
        use_border = context.space_data.use_render_border
    else:
        use_border = render.use_border

    if use_border:
        blender_border = [border_min_x, border_max_x, border_min_y, border_max_y]
        # Round all values to avoid running into problems later
        # when a value is for example 0.699999988079071
        blender_border = [round(value, 6) for value in blender_border]
    else:
        blender_border = [0, 1, 0, 1]

    return blender_border


def calc_screenwindow(zoom, shift_x, shift_y, scene, context=None):
    # shift is in range -2..2
    # offset is in range -1..1
    render = scene.render

    width_raw, height_raw = calc_filmsize_raw(scene, context)
    border_min_x, border_max_x, border_min_y, border_max_y = calc_blender_border(scene, context)

    # Following: Black Magic
    scale = 1
    offset_x = 0
    offset_y = 0
    
    if context:
        # Viewport rendering
        if context.region_data.view_perspective == "CAMERA":
            # Camera view
            offset_x, offset_y = context.region_data.view_camera_offset
            
            if scene.camera and scene.camera.data.type == "ORTHO":                    
                scale = 0.5 * scene.camera.data.ortho_scale
                
            if render.use_border:
                offset_x = 0
                offset_y = 0
                zoom = 1
                aspectratio, xaspect, yaspect = calc_aspect(render.resolution_x * render.pixel_aspect_x,
                                                            render.resolution_y * render.pixel_aspect_y,
                                                            scene.camera.data.sensor_fit)
                    
                if scene.camera and scene.camera.data.type == "ORTHO":
                    # zoom = scale * world_scale
                    zoom = scale
                    
            else:
                # No border
                aspectratio, xaspect, yaspect = calc_aspect(width_raw, height_raw, scene.camera.data.sensor_fit)
                
        else:
            # Normal viewport
            aspectratio, xaspect, yaspect = calc_aspect(width_raw, height_raw)
    else:
        # Final rendering
        aspectratio, xaspect, yaspect = calc_aspect(render.resolution_x * render.pixel_aspect_x,
                                                    render.resolution_y * render.pixel_aspect_y,
                                                    scene.camera.data.sensor_fit)
        
        if scene.camera and scene.camera.data.type == "ORTHO":                    
            scale = 0.5 * scene.camera.data.ortho_scale                

    dx = scale * 2 * (shift_x + 2 * xaspect * offset_x)
    dy = scale * 2 * (shift_y + 2 * yaspect * offset_y)

    screenwindow = [
        -xaspect*zoom + dx,
         xaspect*zoom + dx,
        -yaspect*zoom + dy,
         yaspect*zoom + dy
    ]
    
    screenwindow = [
        screenwindow[0] * (1 - border_min_x) + screenwindow[1] * border_min_x,
        screenwindow[0] * (1 - border_max_x) + screenwindow[1] * border_max_x,
        screenwindow[2] * (1 - border_min_y) + screenwindow[3] * border_min_y,
        screenwindow[2] * (1 - border_max_y) + screenwindow[3] * border_max_y
    ]
    
    return screenwindow


def calc_aspect(width, height, fit="AUTO"):
    horizontal_fit = False
    if fit == "AUTO":
        horizontal_fit = (width > height)
    elif fit == "HORIZONTAL":
        horizontal_fit = True
    
    if horizontal_fit:
        aspect = height / width
        xaspect = 1
        yaspect = aspect
    else:
        aspect = width / height
        xaspect = aspect
        yaspect = 1
    
    return aspect, xaspect, yaspect

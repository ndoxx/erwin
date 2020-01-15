layout(std140, binding = 0) uniform pass_data
{
	mat4 u_m4_v;     // view
	mat4 u_m4_vp;    // view-projection
	vec4 u_v4_eye_w; // camera position, world space
	vec4 u_v4_camera_params; // x: camera near, y: camera far, z&w: padding
	vec4 u_v4_framebuffer_size; // x,y: framebuffer dimensions in pixels, z: aspect ratio, w: padding

	vec4 u_v4_light_position_w; // Directional light position, world space
	vec4 u_v4_light_color;
	vec4 u_v4_light_ambient_color;
    vec4 u_v4_proj_params; // For position reconstruction
	float u_f_light_ambient_strength;
};
layout(std140, binding = 1) uniform instance_data
{
	mat4 u_m4_m;    // model
	mat4 u_m4_mv;   // model-view
	mat4 u_m4_mvp;  // model-view-projection
};
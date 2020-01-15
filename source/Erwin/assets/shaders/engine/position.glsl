vec3 reconstruct_position(in float depth, in vec2 ray, in vec4 proj_params)
{
    float depth_ndc_offset = depth * 2.0f + proj_params.z;
    float depth_view = proj_params.w / depth_ndc_offset;
    return depth_view * vec3(ray, -1.0f);
}

vec3 reconstruct_position(in sampler2D depth_map, in vec2 uv, in vec2 ray, in vec4 proj_params)
{
    float depth = texture(depth_map, uv).r;
    float depth_ndc_offset = depth * 2.0f + proj_params.z;
    float depth_view = proj_params.w / depth_ndc_offset;
    return depth_view * vec3(ray, -1.0f);
}

// proj_params = vec2(P(2,2)-1.0f, P(2,3))
float depth_view_from_tex(in sampler2D depth_tex, in vec2 uv, in vec2 proj_params)
{
    // Get screen space depth at coords
    float depth_raw = texture(depth_tex, uv).r;
    // Convert to NDC depth (*2-1) and add P(2,2)
    float depth_ndc_offset = depth_raw * 2.0f + proj_params.x;
    // Return positive linear depth
    return proj_params.y / depth_ndc_offset;
}

vec3 to_world_vector(vec2 uv, int face_idx)
{
	//                                  +x           -x           +y            -y          +z              -z
	//                                right         left          top         bottom       back           front
	const vec3 center[6] = vec3[]( vec3(1,0,0),  vec3(-1,0,0), vec3(0,1,0), vec3(0,-1,0), vec3(0,0,1),  vec3(0,0,-1) );
	const vec3 u_w[6]    = vec3[]( vec3(0,0,-1), vec3(0,0,1),  vec3(1,0,0), vec3(1,0,0),  vec3(1,0,0),  vec3(-1,0,0) );
	const vec3 v_w[6]    = vec3[]( vec3(0,-1,0), vec3(0,-1,0), vec3(0,0,1), vec3(0,0,-1), vec3(0,-1,0), vec3(0,-1,0) );
	// Local base {u_w, v_w} specified as seen from outside the cube

	return normalize(center[face_idx] + uv.x * u_w[face_idx] + uv.y * v_w[face_idx]);
}

vec2 sample_spherical_map(vec3 v)
{
	// {1/2pi, 1/pi}
	const vec2 inv_atan = vec2(0.159154943f, 0.318309886f);
	
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv = uv * inv_atan + 0.5f;
    return uv;
}
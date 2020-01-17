const mat4 m4_stippling_threshold = mat4(vec4(1,13,4,16),vec4(9,5,12,8),vec4(3,15,2,14),vec4(11,7,10,6))/17.f;

// Selectively discard fragments when surface is too close to eye position
void stipple_near(vec4 fragcoord)
{
    float threshold = m4_stippling_threshold[int(fragcoord.x)%4][int(fragcoord.y)%4];
    if(1.2f*fragcoord.z-threshold < 0.f)
        discard;
}
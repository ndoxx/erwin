// FXAA constants
#define ONE_THIRD 0.33333333f
#define TWO_THIRD 0.66666666f
#define ONE_THIRD_MIN_05 -0.16666667f
#define TWO_THIRD_MIN_05 0.16666667f
const float FXAA_SPAN_MAX = 8.0f;
const float FXAA_REDUCE_MUL = 1.0f/8.0f;
const float FXAA_REDUCE_MIN = 1.0f/128.0f;
const float FXAA_EDGE_THRESHOLD = 1.0f/8.0f;      // 1/4 low-Q 1/8 high-Q
const float FXAA_EDGE_THRESHOLD_MIN = 1.0f/32.0f; // 1/12 upper limit 1/32 visible limit
const float LUMA_COEFF = 0.587f/0.299f;
const vec3  LUMA = vec3(0.299f, 0.587f, 0.114f);

// Luminance calculated on R and G channels only (faster)
float FxaaLuma(vec3 color)
{
    return color.g * LUMA_COEFF + color.r;
}

vec4 FXAA(sampler2D samp, vec2 texCoords, vec2 framebuffer_size)
{
    vec4 samp_center = texture(samp, texCoords);
    vec3 rgbM  = samp_center.rgb;
    float alpha = samp_center.a;

    // Get diagonal neighbors
    vec3 rgbNW = texture(samp, texCoords+(vec2(-1.0f,-1.0f)/framebuffer_size)).rgb;
    vec3 rgbNE = texture(samp, texCoords+(vec2(1.0f,-1.0f)/framebuffer_size)).rgb;
    vec3 rgbSW = texture(samp, texCoords+(vec2(-1.0f,1.0f)/framebuffer_size)).rgb;
    vec3 rgbSE = texture(samp, texCoords+(vec2(1.0f,1.0f)/framebuffer_size)).rgb;

    // Convert to luminance
    float lumaNW = FxaaLuma(rgbNW); //dot(rgbNW, LUMA);
    float lumaNE = FxaaLuma(rgbNE); //dot(rgbNE, LUMA);
    float lumaSW = FxaaLuma(rgbSW); //dot(rgbSW, LUMA);
    float lumaSE = FxaaLuma(rgbSE); //dot(rgbSE, LUMA);
    float lumaM  = FxaaLuma(rgbM);  //dot(rgbM,  LUMA);

    // Find brightest / darkest neighbor
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    // Local contrast check
    // Early exit if luminance range within neighborhood is small enough
    float range = lumaMax-lumaMin;
    if(range < max(FXAA_EDGE_THRESHOLD_MIN, lumaMax * FXAA_EDGE_THRESHOLD))
    {
        return vec4(rgbM, alpha);
    }

    // Edge detection, filtering
    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max(
        (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25f * FXAA_REDUCE_MUL),
        FXAA_REDUCE_MIN);

    float rcpDirMin = 1.0f/(min(abs(dir.x), abs(dir.y)) + dirReduce);

    dir = min(vec2( FXAA_SPAN_MAX,  FXAA_SPAN_MAX),
          max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
          dir * rcpDirMin)) / framebuffer_size;

    vec3 rgbA = 0.5f * (
        texture(samp, ONE_THIRD_MIN_05 * dir + texCoords.xy).xyz +
        texture(samp, TWO_THIRD_MIN_05 * dir + texCoords.xy).xyz);
    vec3 rgbB = (rgbA * 0.5f) + (0.25f * (
        texture(samp, -0.5f * dir + texCoords.xy).xyz +
        texture(samp,  0.5f * dir + texCoords.xy).xyz));
    float lumaB = dot(rgbB, LUMA);

    if((lumaB < lumaMin) || (lumaB > lumaMax))
    {
        return vec4(rgbA, alpha);
    }
    return vec4(rgbB, alpha);
}
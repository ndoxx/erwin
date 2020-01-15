// Relative luminance coefficients for sRGB primaries, values from Wikipedia
const vec3 W = vec3(0.2126f, 0.7152f, 0.0722f);

vec4 glow(in vec3 color, float brightness_threshold, float brightness_knee)
{
    float luminance = dot(color, W);
    float brightness_mask = smoothstep(brightness_threshold-brightness_knee, brightness_threshold, luminance);
    return vec4(brightness_mask*color, brightness_mask);
}
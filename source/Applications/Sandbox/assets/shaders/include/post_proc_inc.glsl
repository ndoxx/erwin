// Relative luminance coefficients for sRGB primaries, values from Wikipedia
const vec3 W = vec3(0.2126f, 0.7152f, 0.0722f);

vec3 vibrance_rgb(vec3 color_in, vec3 vibrance_bal, float vibrance)
{
    vec3 color = color_in;
    float luma = dot(W, color);

    float max_color = max(color.r, max(color.g, color.b)); // Find the strongest color
    float min_color = min(color.r, min(color.g, color.b)); // Find the weakest color

    float color_saturation = max_color - min_color; // The difference between the two is the saturation

    // Extrapolate between luma and original by 1 + (1-saturation) - current
    vec3 coeffVibrance = vec3(vibrance_bal * vibrance);
    color = mix(vec3(luma), color, 1.0 + (coeffVibrance * (1.0 - (sign(coeffVibrance) * color_saturation))));

    return color;
}

vec3 chromatic_aberration_rgb(sampler2D in_tex, vec2 texcoord, vec2 fb_size, float shift, float strength)
{
	vec3 color_in = texture(in_tex, texcoord).rgb;

    vec3 color;
    // Sample the color components
    color.r = texture(in_tex, texcoord + (shift / fb_size)).r;
    color.g = color_in.g;
    color.b = texture(in_tex, texcoord - (shift / fb_size)).b;

    // Adjust the strength of the effect
    return mix(color_in, color, strength);
}

vec3 exposure_tone_mapping_rgb(vec3 in_color, float exposure)
{
	return vec3(1.0f) - exp(-in_color * exposure);
}

vec3 reinhard_tone_mapping_rgb(vec3 in_color)
{
	return in_color / (in_color + vec3(1.0));
}

vec3 saturate_rgb(vec3 color_in, float adjustment)
{
    vec3 luminance = vec3(dot(color_in, W));
    return mix(luminance, color_in, adjustment);
}

vec3 contrast_rgb(vec3 in_color, float contrast)
{
    return ((in_color - 0.5f) * max(contrast, 0)) + 0.5f;
}

vec3 gamma_correct_rgb(vec3 color_in, vec3 gamma_factor)
{
    return pow(color_in, 1.f/gamma_factor);
}
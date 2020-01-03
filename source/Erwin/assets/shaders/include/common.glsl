// Declare a sampler uniform conventionally named SAMPLER_2D_[binding_index]
#define SAMPLER_2D_( BINDING ) layout(binding = BINDING) uniform sampler2D SAMPLER_2D_##BINDING
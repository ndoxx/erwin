#include "asset/atlas_loader.h"
#include "core/core.h"
#include "debug/logger.h"
#include "render/renderer.h"

namespace erwin
{

AssetMetaData TextureAtlasLoader::build_meta_data(const FilePath& file_path)
{
    W_ASSERT_FMT(file_path.exists(), "File does not exist: %s", file_path.c_str());
    W_ASSERT(file_path.check_extension(".cat"_h), "Invalid input file.");

    return {file_path, AssetMetaData::AssetType::TextureAtlasCAT};
}

cat::CATDescriptor TextureAtlasLoader::load_from_file(const AssetMetaData& meta_data)
{
    DLOG("asset", 1) << "Loading CAT file:" << std::endl;
    DLOGI << WCC('p') << meta_data.file_path << std::endl;

    cat::CATDescriptor descriptor;
    descriptor.filepath = meta_data.file_path;
    cat::read_cat(descriptor);

    // Make sure this is a texture atlas
    W_ASSERT(descriptor.remapping_type == cat::RemappingType::TextureAtlas,
             "Invalid remapping type for a texture atlas.");

    return descriptor;
}

TextureAtlas TextureAtlasLoader::upload(const cat::CATDescriptor& descriptor, hash_t /*resource_id*/)
{
    TextureAtlas atlas;

    atlas.width = descriptor.texture_width;
    atlas.height = descriptor.texture_height;
    float fwidth = float(atlas.width);
    float fheight = float(atlas.height);

    // Apply half-pixel correction if linear filtering is used
    /*glm::vec4 correction = (filter & MAG_LINEAR) ? glm::vec4(0.5f/fwidth, 0.5f/fheight, -0.5f/fwidth, -0.5f/fheight)
                                                 : glm::vec4(0.f);*/

    cat::traverse_texture_remapping(descriptor, [&](const cat::CATAtlasRemapElement& remap) {
        glm::vec4 uvs((remap.x) / fwidth, (remap.y) / fheight, (remap.x + remap.w) / fwidth,
                      (remap.y + remap.h) / fheight);
        atlas.remapping.insert(std::make_pair(H_(remap.name), uvs /*+correction*/));
    });

    // Create texture
    ImageFormat format;
    switch(descriptor.texture_compression)
    {
    case TextureCompression::None:
        format = ImageFormat::SRGB_ALPHA;
        break;
    case TextureCompression::DXT1:
        format = ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT1;
        break;
    case TextureCompression::DXT5:
        format = ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT5;
        break;
    }
    uint8_t filter = MAG_NEAREST | MIN_NEAREST;
    // uint8_t filter = MAG_NEAREST | MIN_LINEAR_MIPMAP_NEAREST;
    // uint8_t filter = MAG_LINEAR | MIN_NEAREST_MIPMAP_NEAREST;

    atlas.texture = Renderer::create_texture_2D(Texture2DDescriptor{descriptor.texture_width, descriptor.texture_height,
                                                                    0, descriptor.texture_blob, format, filter,
                                                                    TextureWrap::REPEAT, TF_MUST_FREE});

    DLOG("texture", 1) << "Found " << WCC('v') << atlas.remapping.size() << WCC(0) << " sub-textures in atlas."
                       << std::endl;
    DLOG("texture", 1) << "TextureHandle: " << WCC('v') << int(atlas.texture.index) << std::endl;

    return atlas;
}

void TextureAtlasLoader::destroy(TextureAtlas& resource) { Renderer::destroy(resource.texture); }

AssetMetaData FontAtlasLoader::build_meta_data(const FilePath& file_path)
{
    W_ASSERT_FMT(file_path.exists(), "File does not exist: %s", file_path.c_str());
    W_ASSERT(file_path.check_extension(".cat"_h), "Invalid input file.");

    return {file_path, AssetMetaData::AssetType::FontAtlasCAT};
}

cat::CATDescriptor FontAtlasLoader::load_from_file(const AssetMetaData& meta_data)
{
    DLOG("asset", 1) << "Loading CAT file:" << std::endl;
    DLOGI << WCC('p') << meta_data.file_path << std::endl;

    cat::CATDescriptor descriptor;
    descriptor.filepath = meta_data.file_path;
    cat::read_cat(descriptor);

    // Make sure this is a font atlas
    W_ASSERT(descriptor.remapping_type == cat::RemappingType::FontAtlas, "Invalid remapping type for a font atlas.");

    return descriptor;
}

FontAtlas FontAtlasLoader::upload(const cat::CATDescriptor& descriptor, hash_t /*resource_id*/)
{
    FontAtlas atlas;

    atlas.width = descriptor.texture_width;
    atlas.height = descriptor.texture_height;
    float fwidth = float(atlas.width);
    float fheight = float(atlas.height);

    // Get maximal index and resize remapping table accordingly
    /*uint64_t max_index = 0;
    cat::traverse_font_remapping(descriptor, [&](const cat::CATFontRemapElement& remap)
    {
        if(remap.index > max_index)
            max_index = remap.index;
    });
    remapping.resize(max_index);*/

    // Half-pixel correction
    glm::vec4 correction = glm::vec4(0.5f / fwidth, 0.5f / fheight, -0.5f / fwidth, -0.5f / fheight);

    cat::traverse_font_remapping(descriptor, [&](const cat::CATFontRemapElement& remap) {
        glm::vec4 uvs((remap.x) / fwidth, (remap.y) / fheight, (remap.x + remap.w) / fwidth,
                      (remap.y + remap.h) / fheight);
        atlas.remapping[remap.index] =
            FontAtlas::RemappingElement{uvs + correction,
                                        remap.w,
                                        remap.h,
                                        remap.bearing_x,
                                        int16_t(remap.bearing_y - std::max(0, remap.h - remap.bearing_y)),
                                        remap.advance};
    });

    // Create texture
    uint8_t filter = MAG_NEAREST | MIN_NEAREST;
    // uint8_t filter = MAG_NEAREST | MIN_LINEAR_MIPMAP_NEAREST;
    // uint8_t filter = MAG_LINEAR | MIN_NEAREST_MIPMAP_NEAREST;

    atlas.texture = Renderer::create_texture_2D(Texture2DDescriptor{descriptor.texture_width, descriptor.texture_height,
                                                                    0, descriptor.texture_blob, ImageFormat::RGBA8,
                                                                    // ImageFormat::R8,
                                                                    filter, TextureWrap::REPEAT, TF_MUST_FREE});

    DLOG("texture", 1) << "Found " << WCC('v') << atlas.remapping.size() << WCC(0) << " characters in atlas."
                       << std::endl;
    DLOG("texture", 1) << "TextureHandle: " << WCC('v') << int(atlas.texture.index) << std::endl;

    return atlas;
}

void FontAtlasLoader::destroy(FontAtlas& resource) { Renderer::destroy(resource.texture); }

} // namespace erwin
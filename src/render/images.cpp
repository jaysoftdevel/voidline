#include "render/renderer.hpp"
#include <fstream>
#include <stdexcept>


extern "C" {
unsigned char* WebPDecodeRGBA(const unsigned char*,size_t,int*,int*);
void WebPFree(void*);
struct PngImage {
    void* opaque; uint32_t version,width,height,format,flags,colormapEntries,warningOrError; char message[64];
};
int png_image_begin_read_from_file(PngImage*,const char*);
int png_image_finish_read(PngImage*,const void*,void*,int,void*);
void png_image_free(PngImage*);
}


namespace vl {
Renderer::Texture Renderer::readTexture(const std::filesystem::path& path) {
    gl.ActiveTexture(GL_TEXTURE0);
    if (path.extension()==".png") {
        PngImage info{}; info.version=1;
        if (!png_image_begin_read_from_file(&info,path.c_str())) {
            const std::string error=info.message;
            png_image_free(&info);
            throw std::runtime_error("PNG "+path.string()+": "+error);
        }
        info.format=3;
        std::vector<uint8_t> bytes(size_t(info.width)*info.height*4);
        if (!png_image_finish_read(&info,nullptr,bytes.data(),0,nullptr)) {
            const std::string error=info.message;
            png_image_free(&info);
            throw std::runtime_error("PNG "+path.string()+": "+error);
        }
        const auto result=texture(info.width,info.height,bytes.data());
        png_image_free(&info);
        return result;
    }
    std::ifstream in(path,std::ios::binary);
    const std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(in),{}};
    int w=0,h=0;
    auto* rgba=WebPDecodeRGBA(bytes.data(),bytes.size(),&w,&h);
    if (!rgba) throw std::runtime_error("Could not decode "+path.string());
    const auto result=texture(w,h,rgba);
    WebPFree(rgba);
    return result;
}
Renderer::Texture& Renderer::asset(std::string_view name) {
    const auto it=images.find(name);
    if (it!=images.end()) return it->second;
    const std::string key(name);
    auto path=assets/key;
    if (!path.has_extension()) path+=".webp";
    return images.emplace(key,readTexture(path)).first->second;
}
} // namespace vl

#include "render/renderer.hpp"
#include <span>
#include <stdexcept>


extern "C" {
unsigned char* WebPDecodeRGBA(const unsigned char*,size_t,int*,int*);
void WebPFree(void*);
struct PngImage {
    void* opaque; uint32_t version,width,height,format,flags,colormapEntries,warningOrError; char message[64];
};
int png_image_begin_read_from_memory(PngImage*,const void*,size_t);
int png_image_finish_read(PngImage*,const void*,void*,int,void*);
void png_image_free(PngImage*);
}


namespace vl {
Renderer::Texture Renderer::readTexture(std::string_view path) {
    gl.ActiveTexture(GL_TEXTURE0);
    const std::vector<uint8_t> encoded=assets.read(path);
    if (encoded.empty()) throw std::runtime_error("Empty packaged image: "+std::string(path));
    if (path.ends_with(".png")) {
        PngImage info{}; info.version=1;
        if (!png_image_begin_read_from_memory(&info,encoded.data(),encoded.size())) {
            const std::string error=info.message;
            png_image_free(&info);
            throw std::runtime_error("PNG "+std::string(path)+": "+error);
        }
        info.format=3;
        std::vector<uint8_t> bytes(size_t(info.width)*info.height*4);
        if (!png_image_finish_read(&info,nullptr,bytes.data(),0,nullptr)) {
            const std::string error=info.message;
            png_image_free(&info);
            throw std::runtime_error("PNG "+std::string(path)+": "+error);
        }
        const auto result=texture(info.width,info.height,bytes.data());
        png_image_free(&info);
        return result;
    }
    int w=0,h=0;
    auto* rgba=WebPDecodeRGBA(encoded.data(),encoded.size(),&w,&h);
    if (!rgba) throw std::runtime_error("Could not decode "+std::string(path));
    const auto result=texture(w,h,rgba);
    WebPFree(rgba);
    return result;
}
Renderer::Texture& Renderer::asset(std::string_view name) {
    const auto it=images.find(name);
    if (it!=images.end()) return it->second;
    const std::string key(name);
    std::string path=key;
    if (path.rfind('.',path.size())==std::string::npos) path+=".webp";
    return images.emplace(key,readTexture(path)).first->second;
}
} // namespace vl

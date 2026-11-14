#include "render/renderer.hpp"
#include <stdexcept>

namespace vl {
GLuint Renderer::program(const char* vertex,const char* fragment) {
    auto compile=[&](GLenum stage,const char* code) {
        GLuint shader=gl.CreateShader(stage);
        gl.ShaderSource(shader,1,&code,nullptr); gl.CompileShader(shader);
        int ok=0; gl.GetShaderiv(shader,GL_COMPILE_STATUS,&ok);
        if (!ok) {
            char log[4096]{}; gl.GetShaderInfoLog(shader,sizeof(log),nullptr,log);
            gl.DeleteShader(shader);
            throw std::runtime_error(std::string("Shader compilation failed: ")+log);
        }
        return shader;
    };
    const GLuint vs=compile(GL_VERTEX_SHADER,vertex), fs=compile(GL_FRAGMENT_SHADER,fragment);
    const GLuint result=gl.CreateProgram();
    gl.AttachShader(result,vs); gl.AttachShader(result,fs); gl.LinkProgram(result);
    gl.DeleteShader(vs); gl.DeleteShader(fs);
    int ok=0; gl.GetProgramiv(result,GL_LINK_STATUS,&ok);
    if (!ok) {
        char log[4096]{}; gl.GetProgramInfoLog(result,sizeof(log),nullptr,log);
        gl.DeleteProgram(result);
        throw std::runtime_error(std::string("Shader linking failed: ")+log);
    }
    return result;
}

Renderer::Texture Renderer::texture(int w,int h,const void* pixels,GLenum format) {
    Texture t{0,w,h};
    gl.GenTextures(1,&t.id); gl.BindTexture(GL_TEXTURE_2D,t.id);
    gl.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); gl.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    gl.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE); gl.TexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    gl.TexImage2D(GL_TEXTURE_2D,0,format,w,h,0,GL_RGBA,format==GL_RGBA8?GL_UNSIGNED_BYTE:GL_FLOAT,pixels);
    return t;
}

Renderer::Target Renderer::target(int w,int h,GLenum format) {
    Target t{texture(w,h,nullptr,format?format:hdrFormat),0};
    gl.GenFramebuffers(1,&t.framebuffer); gl.BindFramebuffer(GL_FRAMEBUFFER,t.framebuffer);
    gl.FramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,t.texture.id,0);
    if (gl.CheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE) {
        destroy(t);
        throw std::runtime_error("Graphics framebuffer is incomplete");
    }
    return t;
}

GLenum Renderer::chooseHdrFormat() {
    for (const auto format:{GL_R11F_G11F_B10F,GL_RGBA16F}) {
        try {
            auto trial=target(16,16,format);
            const bool valid=gl.GetError()==GL_NO_ERROR;
            destroy(trial);
            if (valid) return format;
        } catch (const std::runtime_error&) {
            // Unsupported packed formats may be reported as an incomplete target.
        }
        for (int i=0;i<8 && gl.GetError()!=GL_NO_ERROR;++i) {}
    }
    throw std::runtime_error("The graphics driver cannot create a floating-point color target.");
}

void Renderer::destroy(Target& t) {
    gl.DeleteFramebuffers(1,&t.framebuffer);
    gl.DeleteTextures(1,&t.texture.id);
}

void Renderer::bind(const Target& t,bool clear) {
    flush(); gl.BindFramebuffer(GL_FRAMEBUFFER,t.framebuffer);
    gl.Viewport(0,0,t.texture.w,t.texture.h);
    if (clear) { gl.ClearColor(0,0,0,1); gl.Clear(GL_COLOR_BUFFER_BIT); }
}

void Renderer::output() {
    flush(); gl.BindFramebuffer(GL_FRAMEBUFFER,0);
    gl.Viewport(viewX,viewY,viewW,viewH);
}

void Renderer::flush() {
    if (vertices.empty()) return;
    if (figureScale!=1.f) {
        for (auto& vertex:vertices) {
            const Vec local=vertex.p-figureOrigin;
            const float rim=smooth((local.x*figureLightSide+14.f)/32.f)*smooth((-local.y-4.f)/45.f)*.24f;
            // A side light picks up the suit contours and fades toward the floor.
            const float bounce=.16f*smooth((local.y+40.f)/38.f);
            vertex.tint.r+=vertex.tint.r*(figureRim.r*rim+bounce*.56f);
            vertex.tint.g+=vertex.tint.g*(figureRim.g*rim+bounce*.76f);
            vertex.tint.b+=vertex.tint.b*(figureRim.b*rim+bounce);
            vertex.p=figureOrigin+local*figureScale;
        }
    }
    gl.UseProgram(drawProgram);
    useTexture(drawProgram,"image",0,batchTexture);
    gl.BindVertexArray(vao); gl.BindBuffer(GL_ARRAY_BUFFER,buffer);
    gl.BufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STREAM_DRAW);
    gl.DrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(vertices.size()));
    vertices.clear();
}

void Renderer::useTexture(GLuint shader,const char* name,int slot,GLuint id) {
    gl.ActiveTexture(GL_TEXTURE0+slot); gl.BindTexture(GL_TEXTURE_2D,id); gl.Uniform1i(uniform(shader,name),slot);
}

void Renderer::screen(GLuint shader) {
    gl.UseProgram(shader); gl.BindVertexArray(vao); gl.DrawArrays(GL_TRIANGLES,0,3);
}
} // namespace vl

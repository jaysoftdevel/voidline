#pragma once
#include <cstddef>

namespace vl {
using GLenum=unsigned int; using GLuint=unsigned int; using GLint=int;
using GLsizei=int; using GLboolean=unsigned char; using GLsizeiptr=std::ptrdiff_t;
constexpr GLenum GL_COLOR_BUFFER_BIT=0x4000,GL_TRIANGLES=4,GL_FLOAT=0x1406,GL_UNSIGNED_BYTE=0x1401,
    GL_ARRAY_BUFFER=0x8892,GL_STREAM_DRAW=0x88E0,GL_VERTEX_SHADER=0x8B31,GL_FRAGMENT_SHADER=0x8B30,
    GL_COMPILE_STATUS=0x8B81,GL_LINK_STATUS=0x8B82,GL_TEXTURE_2D=0x0DE1,GL_TEXTURE0=0x84C0,
    GL_RGBA=0x1908,GL_RGBA8=0x8058,GL_RGBA16F=0x881A,GL_R11F_G11F_B10F=0x8C3A,GL_LINEAR=0x2601,GL_CLAMP_TO_EDGE=0x812F,
    GL_TEXTURE_MIN_FILTER=0x2801,GL_TEXTURE_MAG_FILTER=0x2800,GL_TEXTURE_WRAP_S=0x2802,GL_TEXTURE_WRAP_T=0x2803,
    GL_FRAMEBUFFER=0x8D40,GL_COLOR_ATTACHMENT0=0x8CE0,GL_FRAMEBUFFER_COMPLETE=0x8CD5,
    GL_BLEND=0x0BE2,GL_ONE=1,GL_SRC_ALPHA=0x0302,GL_ONE_MINUS_SRC_ALPHA=0x0303,GL_VERSION=0x1F02,
    GL_RENDERER=0x1F01,GL_NO_ERROR=0,GL_SCISSOR_TEST=0x0C11;
#define VL_GL_FUNCTIONS(X) \
    X(void,Viewport,(GLint,GLint,GLsizei,GLsizei)) \
    X(void,Scissor,(GLint,GLint,GLsizei,GLsizei)) \
    X(void,ClearColor,(float,float,float,float)) X(void,Clear,(GLenum)) \
    X(void,Enable,(GLenum)) X(void,Disable,(GLenum)) X(void,BlendFunc,(GLenum,GLenum)) \
    X(const unsigned char*,GetString,(GLenum)) X(GLenum,GetError,()) \
    X(void,GenVertexArrays,(GLsizei,GLuint*)) X(void,BindVertexArray,(GLuint)) X(void,DeleteVertexArrays,(GLsizei,const GLuint*)) \
    X(void,GenBuffers,(GLsizei,GLuint*)) X(void,BindBuffer,(GLenum,GLuint)) X(void,DeleteBuffers,(GLsizei,const GLuint*)) \
    X(void,BufferData,(GLenum,GLsizeiptr,const void*,GLenum)) \
    X(void,EnableVertexAttribArray,(GLuint)) X(void,VertexAttribPointer,(GLuint,GLint,GLenum,GLboolean,GLsizei,const void*)) \
    X(GLuint,CreateShader,(GLenum)) X(void,ShaderSource,(GLuint,GLsizei,const char* const*,const GLint*)) \
    X(void,CompileShader,(GLuint)) X(void,GetShaderiv,(GLuint,GLenum,GLint*)) X(void,GetShaderInfoLog,(GLuint,GLsizei,GLsizei*,char*)) \
    X(GLuint,CreateProgram,()) X(void,AttachShader,(GLuint,GLuint)) X(void,LinkProgram,(GLuint)) \
    X(void,GetProgramiv,(GLuint,GLenum,GLint*)) X(void,GetProgramInfoLog,(GLuint,GLsizei,GLsizei*,char*)) \
    X(void,DeleteShader,(GLuint)) X(void,DeleteProgram,(GLuint)) X(void,UseProgram,(GLuint)) \
    X(GLint,GetUniformLocation,(GLuint,const char*)) X(void,Uniform1i,(GLint,GLint)) X(void,Uniform1f,(GLint,float)) \
    X(void,Uniform2f,(GLint,float,float)) X(void,Uniform3f,(GLint,float,float,float)) X(void,Uniform4f,(GLint,float,float,float,float)) \
    X(void,Uniform4fv,(GLint,GLsizei,const float*)) \
    X(void,GenTextures,(GLsizei,GLuint*)) X(void,BindTexture,(GLenum,GLuint)) X(void,ActiveTexture,(GLenum)) \
    X(void,TexParameteri,(GLenum,GLenum,GLint)) X(void,TexImage2D,(GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const void*)) \
    X(void,DeleteTextures,(GLsizei,const GLuint*)) \
    X(void,GenFramebuffers,(GLsizei,GLuint*)) X(void,BindFramebuffer,(GLenum,GLuint)) \
    X(void,FramebufferTexture2D,(GLenum,GLenum,GLenum,GLuint,GLint)) X(GLenum,CheckFramebufferStatus,(GLenum)) \
    X(void,DeleteFramebuffers,(GLsizei,const GLuint*)) X(void,DrawArrays,(GLenum,GLint,GLsizei)) \
    X(void,ReadPixels,(GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,void*))
struct GL {
#define VL_DECLARE(result,name,args) result (*name) args=nullptr;
    VL_GL_FUNCTIONS(VL_DECLARE)
#undef VL_DECLARE
    void load();
};
}

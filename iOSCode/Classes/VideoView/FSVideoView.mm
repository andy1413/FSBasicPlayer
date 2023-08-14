//
//  FSVideoView.m
//  ffplayios
//
//  Created by andy on 2023/1/1.
//  Copyright © 2023年 andy. All rights reserved.
//

#import "FSVideoView.h"
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
#import "WZBPBundle.h"
#import <WZFFmpeg/libavcodec/avcodec.h>

# if __has_include(<WZFFPlay/FSPlay.hpp>)
#import <WZFFPlay/FSPlay.hpp>
#else
#import "FSPlay.hpp"
#endif

#define GLES2_MAX_PLANE 3

static const GLfloat vertices[12] = {
    -1.0f, -1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f,
    1.0f,  1.0f, 0.0f
};

static const GLfloat textureCoords[8] = {
    0.0f, 1.0f,
    1.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 0.0f
};

@interface FSVideoView () {
    CAEAGLLayer *_eaglLayer;
    EAGLContext *_context;
    
    GLuint _programId;
    
    GLuint _frameBuffer;
    GLuint _renderBuffer;
    GLint _renderWidth;
    GLint _renderHeight;
    
    GLuint _vertexShader;
    GLuint _fragmentShader;
    GLuint _planeTextures[GLES2_MAX_PLANE];//texture_id
    
    GLuint vsh_position;
    GLuint vsh_texcoord;
    GLuint vsh_matrix;
    
    GLuint sampler[GLES2_MAX_PLANE];
    
    FSPlay *_play;
}

@end

@implementation FSVideoView

- (instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        _play = new FSPlay();
        [self setupRender];
    }
    return self;
}

- (void)dealloc {
    if (_frameBuffer) {
        glDeleteFramebuffers(1, &_frameBuffer);
        _frameBuffer = 0;
    }
    
    if (_renderBuffer) {
        glDeleteRenderbuffers(1, &_renderBuffer);
        _renderBuffer = 0;
    }
    glFinish();
    
    _play->ffplay_stream_close();
}

- (void)layoutSubviews {
    [super layoutSubviews];
    [self setupRender];
}

+ (void)setLogCallback:(void (*)(void*, int, const char*, va_list))callback {
    fs_log_set_callback(callback);
}

- (int)setupRender {
    _eaglLayer = (CAEAGLLayer*) self.layer;
    _eaglLayer.opaque = YES;
    _eaglLayer.drawableProperties = @{kEAGLDrawablePropertyRetainedBacking: @(NO), kEAGLDrawablePropertyColorFormat: kEAGLColorFormatRGBA8};
    _context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    [EAGLContext setCurrentContext:_context];
    
    glGenFramebuffers(1, &_frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);
    glGenRenderbuffers(1, &_renderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, _renderBuffer);
    [_context renderbufferStorage:GL_RENDERBUFFER fromDrawable:_eaglLayer];
    
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &_renderWidth);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &_renderHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _renderBuffer);
    
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        NSLog(@"glCheckFramebufferStatus %x\n", status);
        return -1;
    }
    
    GLenum glError = glGetError();
    if (GL_NO_ERROR != glError) {
        NSLog(@"glGetError %x\n", glError);
        return -1;
    }
    return 0;
}

- (int)loadShaders:(VideoFrame *)frame {
    _programId = glCreateProgram();
    NSString *vshName = @"yuv420p_vsh";
    NSString *fshName = @"yuv420p_fsh";
    if (frame->format == AV_PIX_FMT_RGB24) {
        vshName = @"rgba_vsh";
        fshName = @"rgba_fsh";
    }
    _vertexShader = compileShader(vshName, GL_VERTEX_SHADER);
    _fragmentShader = compileShader(fshName, GL_FRAGMENT_SHADER);
    glAttachShader(_programId, _vertexShader);
    glAttachShader(_programId, _fragmentShader);
   
    glLinkProgram(_programId);
    GLint linkStatus;
    glGetProgramiv(_programId, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE) {
        GLchar messages[256];
        glGetProgramInfoLog(_programId, sizeof(messages), 0, messages);
        NSLog(@"%s", messages);
        return -1;
    }
    // 获取vertex shader中attribute变量的引用
    vsh_position = glGetAttribLocation(_programId, "vsh_position");
    vsh_texcoord = glGetAttribLocation(_programId, "vsh_texcoord");
    vsh_matrix = glGetUniformLocation(_programId, "vsh_matrix");
    // 获取fragment shader中uniform变量的引用
    if (frame->format == AV_PIX_FMT_YUV420P) {
        sampler[0] = glGetUniformLocation(_programId, "samplerY");
        sampler[1] = glGetUniformLocation(_programId, "samplerU");
        sampler[2] = glGetUniformLocation(_programId, "samplerV");
    } else if (frame->format == AV_PIX_FMT_RGB24) {
        sampler[0] = glGetUniformLocation(_programId, "sampler");
    }
    
    return 0;
}

- (void)play {
    dispatch_async(dispatch_get_main_queue(), ^{
        [self private_play];
    });
}

- (void)private_play {
    _play->ffplay_stream_close();
    _play->ffplay_main((__bridge void *)self, (char *)self.playUrl.UTF8String, setPlayerParams, render_frame);
    
    _play->statusChangeCallback = statusChange;
}

void render_frame(void *view,VideoFrame *frame) {
    @autoreleasepool {
        FSVideoView *glkView = (__bridge FSVideoView *)(view);
        [glkView render_frame:frame];
    }
}

void statusChange(void *view, enum FSPlayStatus playStatus) {
    FSVideoView *glkView = (__bridge FSVideoView *)(view);
    [glkView statusChange:playStatus];
}

- (void)statusChange:(enum FSPlayStatus)playStatus {
    switch (playStatus) {
        case FSPlayStatus_error:
        {
            self.statusChangeCallback(FSVideoStatus_error);
        }
            break;
        case FSPlayStatus_completed:
        {
            self.statusChangeCallback(FSVideoStatus_completed);
//            [self play];
        }
            break;
        default:
            break;
    }
}

- (void)stop {
    _play->ffplay_stream_close();
}

void setPlayerParams(void *view) {
    FSVideoView *videoView = (__bridge FSVideoView *)view;
    [videoView setPlayParams];
}

- (void)setPlayParams {
//    _play->set_format_opt("start_number", "0");
//    _play->set_format_opt("framerate", "5");
    
    switch (self.playMode) {
        case PlayMode_concat:
            [self setConcatParams];
            break;
        case PlayMode_dash:
            [self setDashParams];
            break;
        default:
            break;
    }
}

- (void)setDashParams {
    _play->start_time = 1000000 * _startSecond;
//    _play->opt_add_vfilter(nullptr, "vf", "setpts=0.25*PTS");
    _play->audio_disable = 1;
}

- (void)setConcatParams {
    _play->set_input_format("concat");
    _play->set_format_opt("safe", "0");
    _play->set_format_opt("protocol_whitelist", "file,http,https,crypto,data,tls,tcp");
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event{
    [super touchesBegan:touches withEvent:event];
//    _play->ffplay_pause();
}

#pragma mark - render_frame
- (void)render_frame:(VideoFrame *)frame {
    [EAGLContext setCurrentContext:_context];
    [self loadShaders:frame];

    glUseProgram(_programId);

    if (0 == _planeTextures[0])
        glGenTextures(frame->planar , _planeTextures);//创建texture对象

    int widths[frame->planar];
    int heights[frame->planar];
    if (frame->format == AV_PIX_FMT_YUV420P) {
        widths[0] = frame->width;
        widths[1] = frame->width / 2;
        widths[2] = frame->width / 2;
        heights[0] = frame->height;
        heights[1] = frame->height / 2;
        heights[2] = frame->height / 2;
    } else if (frame->format == AV_PIX_FMT_RGB24) {
        widths[0] = frame->width;
        heights[0] = frame->height;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (int i = 0; i < frame->planar; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);//设置当前活动的纹理单元
        glBindTexture(GL_TEXTURE_2D, _planeTextures[i]);//texture绑定
        if (frame->format == AV_PIX_FMT_YUV420P) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, widths[i], heights[i], 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->pixels[i]);
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, widths[i], heights[i], 0, GL_RGB, GL_UNSIGNED_BYTE, frame->pixels[i]);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glUniform1i(sampler[i], i);
    }

    glEnableVertexAttribArray(vsh_position);
    glEnableVertexAttribArray(vsh_texcoord);

    glVertexAttribPointer(vsh_position, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), vertices);
    glVertexAttribPointer(vsh_texcoord, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), textureCoords);

    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, _renderWidth, _renderHeight);
    glUniformMatrix4fv(vsh_matrix, 1, GL_FALSE, GLKMatrix4Identity.m);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindRenderbuffer(GL_RENDERBUFFER, _renderBuffer);
    //present render
    [_context presentRenderbuffer:GL_RENDERBUFFER];

    if ((_programId)) {
        if (_vertexShader){
            glDetachShader(_programId, _vertexShader);
            glDeleteShader(_vertexShader);
            _vertexShader = 0;
        }

        if (_fragmentShader){
            glDetachShader(_programId, _fragmentShader);
            glDeleteShader(_fragmentShader);
            _fragmentShader = 0;
        }

        if (_programId){
            glDeleteProgram(_programId);
            _programId  = 0;
        }

        for (int i = 0; i < GLES2_MAX_PLANE; ++i) {
            if (_planeTextures[i]) {
                glDeleteTextures(1, &_planeTextures[i]);
                _planeTextures[i] = 0;
            }
        }
    }
}

GLuint compileShader(NSString *shaderName, GLenum shaderType) {
    NSString *shaderPath = [WZBPBundle pathForName:shaderName ofType:@"glsl"];
    NSError *error;
    NSString *shaderString = [NSString stringWithContentsOfFile:shaderPath encoding:NSUTF8StringEncoding error:&error];
    if (!shaderString) {
        NSLog(@"shaderString %@", error.localizedDescription);
        return -1;
    }
    GLuint shader = glCreateShader(shaderType);

    const char* shaderStringUTF8 = [shaderString UTF8String];
    GLint shaderStringLength = (GLint)[shaderString length];

    glShaderSource(shader, 1, &shaderStringUTF8, &shaderStringLength);

    glCompileShader(shader);

    GLint compileSuccess;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileSuccess);
    if (compileSuccess == GL_FALSE) {
        GLchar messages[256];
        glGetShaderInfoLog(shader, sizeof(messages), 0, &messages[0]);
        NSString *messageString = [NSString stringWithUTF8String:messages];
        NSLog(@"GL_COMPILE_STATUS %@", messageString);
        return -1;
    }

    return shader;
}

@end

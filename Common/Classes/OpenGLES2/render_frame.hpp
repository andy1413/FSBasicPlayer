//
//  render_frame.h
//  ffplayios
//
//  Created by andy on 2023/1/1.
//  Copyright © 2023年 andy. All rights reserved.
//

#ifndef render_frame_h
#define render_frame_h

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/frame.h"
#ifdef __cplusplus
}
#endif

typedef struct VideoFrame VideoFrame;

struct VideoFrame {
    int width;
    int height;
    uint8_t *pixels[AV_NUM_DATA_POINTERS];
    enum AVPixelFormat format;
    int planar;
};

#endif /* render_frame_h */

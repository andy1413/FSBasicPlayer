//
//  WZMediaPlayerView.h
//  ffplayios
//
//  Created by andy on 2023/1/1.
//  Copyright © 2023年 andy. All rights reserved.
//

#import <GLKit/GLKit.h>

typedef enum : NSUInteger {
    PlayMode_concat,
    PlayMode_dash,
    PlayMode_none,
} PlayMode;

enum WZVideoStatus {
    WZVideoStatus_completed = 1,
    WZVideoStatus_error = 2,
    WZVideoStatus_loading = 3,
    WZVideoStatus_playing = 4,
    WZVideoStatus_stoped = 5,
};

@interface WZMediaPlayerView : GLKView

@property (nonatomic, assign) NSTimeInterval startSecond;
@property (nonatomic, strong) NSString *playUrl;
@property (nonatomic, assign) PlayMode playMode;
@property (nonatomic, assign) BOOL needRotate;
@property (nonatomic, copy) void(^renderChangeCallback)(NSInteger renderIndex);
@property (nonatomic, copy) void(^frameChangeCallback)(CGSize size);
@property (nonatomic, copy) void(^statusChangeCallback)(enum WZVideoStatus videoStatus);
@property (nonatomic, assign) BOOL mute;

- (void)play;
- (void)playWithDisableAudio;

- (void)togglePause;
- (void)stop;
- (BOOL)isStoped;
+ (void)setLogCallback:(void (*)(void*, int, const char*, va_list))callback;
- (NSTimeInterval)getCurrentTime;

@end

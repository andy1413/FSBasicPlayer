//
//  FSVideoView.h
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

enum FSVideoStatus {
    FSVideoStatus_completed = 1,
    FSVideoStatus_error = 2,
};

@interface FSVideoView : GLKView

@property (nonatomic, assign) NSTimeInterval startSecond;
@property (nonatomic, strong) NSString *playUrl;
@property (nonatomic, assign) PlayMode playMode;
@property (nonatomic, copy) void(^statusChangeCallback)(enum FSVideoStatus videoStatus);

- (void)play;
- (void)stop;
+ (void)setLogCallback:(void (*)(void*, int, const char*, va_list))callback;

@end

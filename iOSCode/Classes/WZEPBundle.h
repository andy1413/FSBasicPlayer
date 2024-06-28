//
//  WZEPBundle.h
//  AAInfographics
//
//  Created by a on 2022/11/29.
//
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface WZEPBundle : NSObject

+ (NSBundle *)currentBundle;

+ (NSString *)currentBundlePath;

+ (nullable NSString *)pathForName:(NSString *)name ofType:(nullable NSString *)type;

@end

NS_ASSUME_NONNULL_END

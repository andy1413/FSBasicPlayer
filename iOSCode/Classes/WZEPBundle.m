//
//  WZEPBundle.m
//  AAInfographics
//
//  Created by a on 2022/11/29.
//

#import "WZEPBundle.h"

@implementation WZEPBundle

+ (NSString *)currentResourceBundleName {
    return @"eventPlayer";
}

+ (NSBundle *)currentBundle
{
    NSString *currentBundleName = [self currentResourceBundleName];
    NSURL *currentBundleURL = [[NSBundle mainBundle] URLForResource:currentBundleName withExtension:@"bundle"];
    return [NSBundle bundleWithURL:currentBundleURL];
}

+ (NSString *)currentBundlePath
{
    return [[self currentBundle] bundlePath];
}

+ (NSString *)pathForName:(NSString *)name ofType:(nullable NSString *)type {
    NSString *fullName = type != nil ? [NSString stringWithFormat:@"%@.%@", name, type] : name;
    NSString *path = [WZEPBundle.currentBundle pathForResource:fullName ofType:nil];
    if ([[NSFileManager defaultManager] fileExistsAtPath:path]) {
        return path;
    } else {
        return [NSString stringWithFormat:@"%@/%@", [self currentBundlePath], fullName];
    }
}

@end

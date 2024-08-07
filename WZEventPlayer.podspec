
Pod::Spec.new do |s|

  s.name         = "WZEventPlayer"
  s.version      = "3.0.0"
  s.summary      = "WZEventPlayer"

  s.description  = <<-DESC
    WZEventPlayer
                   DESC

  s.homepage      = "https://github.com/wyzelabs-inc/wyze-module-eventplayer-ios"
  s.license       = { :type => 'MIT', :file => 'LICENSE' }
  s.author        = { "wangfangshuai" => "wangfangshuai@gmail.com" }
  s.source        = { :git => 'git@github.com:wyzelabs-inc/wyze-module-eventplayer-ios.git', :tag => "#{s.version}" }
  s.ios.deployment_target = "14.0"

  s.static_framework  = true
  s.swift_version     = '5.0'
  s.pod_target_xcconfig = {
        'ENABLE_BITCODE'       => 'YES',
        'OTHER_LDFLAGS'        => '$(inherited) -undefined dynamic_lookup -ObjC',
        'OTHER_CFLAGS'         => '-DLINUX',
        'VALID_ARCHS'          => 'x86_64 arm64',
        'SWIFT_OBJC_INTERFACE_HEADER_NAME' => '$(inherited)',
        'SWIFT_PRECOMPILE_BRIDGING_HEADER' => 'YES',
        'DEFINES_MODULE' => 'YES',
      }

#  s.user_target_xcconfig = { 'CLANG_ALLOW_NON_MODULAR_INCLUDES_IN_FRAMEWORK_MODULES' => 'YES' }

  s.source_files = 'Common/Classes/**/*', 'iOSCode/Classes/**/*'
  s.public_header_files = "Common/**/*.h", "iOSCode/**/*.h"
  
  s.resource_bundle = {
      "eventPlayer" =>  ["Common/Resource/**/*.*", "iOSCode/Resource/**/*.*"]
  }
  
  # s.vendored_frameworks  = 'SDL2.xcframework'
  # s.frameworks = "Foundation", "CoreHaptics", "GameController", "AVFoundation", "AudioToolbox", "MediaPlayer", "CoreMotion"

  # s.library   = "iconv"
   s.libraries = "iconv", "xml2", "z", "bz2"
   s.frameworks = "GLKit", "OpenGLES", "QuartzCore", "CoreGraphics", "OpenGLES", "UIKit"

  s.requires_arc = true

  # s.xcconfig = { "HEADER_SEARCH_PATHS" => ["$(PODS_ROOT)/SDL2/header/"]}

  s.pod_target_xcconfig = {
    'EXCLUDED_ARCHS[sdk=iphonesimulator*]' => 'arm64'
  }

  s.dependency "WZFFmpeg"
  s.dependency "SDL2"
  s.dependency "Masonry"

end

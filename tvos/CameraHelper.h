#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

@protocol CameraFrameDelegate <NSObject>
@required
- (void) PushCameraImageIOS:(long long)len buffer:(unsigned char*)data;
@end

#if TARGET_OS_IOS
@interface CameraHelper : NSObject<AVCaptureVideoDataOutputSampleBufferDelegate>

@property (nonatomic, strong) id<CameraFrameDelegate> delegate;

- (void) setCameraSize:(int)width h:(int)height;
- (void) startVideo;
- (void) stopVideo;

@end
#endif

#import "LocationHelper.h"

@interface LocationHelper()
@end

@implementation LocationHelper

-(id) init {
    NSLog(@"LocationHelper::init");
    locationManager = [[CLLocationManager alloc] init];
    [locationManager setDelegate:self];
    return self;
}

-(void) startLocationUpdates {
    NSLog(@"LocationHelper::startLocationUpdates");
    if ([locationManager respondsToSelector:@selector(requestWhenInUseAuthorization)]) {
        [locationManager requestWhenInUseAuthorization];
    }
#if TARGET_OS_IOS
    [locationManager startUpdatingLocation];
#endif
}

-(void) stopLocationUpdates {
    NSLog(@"LocationHelper::stopLocationUpdates");
    [locationManager stopUpdatingLocation];
}

- (void) locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray<CLLocation *> *)locations {
    for (id location in locations) {
        [self.delegate SetGpsDataIOS:location];
    }
}

@end

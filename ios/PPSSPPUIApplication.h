//
//  PPSSPPUIApplication.h
//  PPSSPP
//
//  Created by xieyi on 2017/9/4.
//
//

#ifndef PPSSPPUIApplication_h
#define PPSSPPUIApplication_h

#import <UIKit/UIKit.h>

@interface PPSSPPUIApplication : UIApplication
{
}

#if TARGET_OS_IOS
@property (nonatomic, strong) UISelectionFeedbackGenerator *feedbackGenerator;
#endif

@end


#endif /* PPSSPPUIApplication_h */

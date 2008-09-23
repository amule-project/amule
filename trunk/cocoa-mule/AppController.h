#import <Cocoa/Cocoa.h>

@interface AppController : NSObject {
    IBOutlet id m_main_tabview;
}
- (IBAction)show_Networks:(id)sender;
- (IBAction)show_Search:(id)sender;
- (IBAction)show_Sharing:(id)sender;
- (IBAction)show_Stats:(id)sender;
- (IBAction)show_Xfers:(id)sender;
- (IBAction)show_Preferences:(id)sender;
- (IBAction)show_About:(id)sender;
@end

#import <Cocoa/Cocoa.h>

@interface LoginDialogController : NSObject {
    IBOutlet id m_dlg;
	
	bool m_dlg_result;
}

- (bool)showDlg:(NSWindow *)window;

- (IBAction)closeCancel:(id)sender;
- (IBAction)closeOK:(id)sender;
@end

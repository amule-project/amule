#import "LoginDialogController.h"

@implementation LoginDialogController

- (bool)showDlg:(NSWindow *)window {
	if ( m_dlg == nil ) {
		[NSBundle loadNibNamed: @"LoginDialog" owner: self];
	}

	[NSApp beginSheet: m_dlg modalForWindow: window
		modalDelegate: nil didEndSelector: nil contextInfo: nil];
					
	[NSApp runModalForWindow: m_dlg];
	
	[NSApp endSheet: m_dlg];

    [m_dlg orderOut: self];
	
	return m_dlg_result;
}


- (IBAction)closeCancel:(id)sender {
	m_dlg_result = false;
    [NSApp stopModal];
}

- (IBAction)closeOK:(id)sender {
	m_dlg_result = true;
    [NSApp stopModal];
}


@end

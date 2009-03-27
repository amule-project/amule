#import "AddLinkDialogController.h"

@implementation AddLinkDialogController

- (bool)showDlg:(NSWindow *)window {

	[m_link setStringValue:@""];
	
	[NSApp beginSheet: m_dlg modalForWindow: window
		modalDelegate: nil didEndSelector: nil contextInfo: nil];
					
	[NSApp runModalForWindow: m_dlg];
	
	[NSApp endSheet: m_dlg];

    [m_dlg orderOut: self];
	
	return m_dlg_result;
}

- (IBAction)closeOK:(id)sender {
	[NSApp stopModal];
	 
	m_link_val = [m_link stringValue];
	m_dlg_result = true;
}

- (NSString *)link {
	return m_link_val;
}

@end

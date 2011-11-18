#import "AppController.h"

#import "LoginDialogController.h"
#import "DownloadsViewController.h"
#import "AddLinkDialogController.h"

#include <unistd.h>
#include <signal.h>

@implementation AppController

- (IBAction)show_Networks:(id)sender {
	[m_main_tabview selectTabViewItemAtIndex: 2];
	

//	ECLoginPacket *p = [ECLoginPacket loginPacket:@"123456" withVersion:@"0.1"];
//	NSOutputStream *stream = [NSOutputStream outputStreamToMemory];
//	uint8_t buffer[1024];
//	memset(buffer, 0, 1024);
//	NSOutputStream *stream = [NSOutputStream outputStreamToBuffer:buffer capacity:1024];
//	[stream open];
//	[p writeToSocket:stream];
//	id data = [stream propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
// 	int off = [data length];
//	NSLog(@"off=%d", off);
//	[m_connection sendPacket:p];
}

- (IBAction)show_Search:(id)sender {
	[m_main_tabview selectTabViewItemAtIndex: 1];
}

- (IBAction)show_Sharing:(id)sender {
	[m_main_tabview selectTabViewItemAtIndex: 3];
}

- (IBAction)show_Stats:(id)sender {
	[m_main_tabview selectTabViewItemAtIndex: 4];
}

- (IBAction)show_Xfers:(id)sender {
	[m_main_tabview selectTabViewItemAtIndex: 0];
}

-(IBAction)show_Preferences:(id)sender {
}

-(IBAction)show_About:(id)sender {
}

- (IBAction)addLink:(id)sender;{	
	bool dlgResult = [m_add_link_dlg showDlg:nil];
	if ( dlgResult ) {
		ECPacket *packet = [ECPacket packetWithOpcode:EC_OP_ADD_LINK];
		[packet.subtags addObject:[ECTagString tagFromString:[m_add_link_dlg link] withName:EC_TAG_ED2K_ID] ];
		[m_connection sendPacket:packet];
	}
}

- (bool)askCoreParams {
	LoginDialogController *dlg = [[LoginDialogController alloc] init];
	
	bool dlgResult = [dlg showDlg:nil];

	m_targetaddr = dlg.host;
	m_targetport = dlg.port;
	m_corepass = dlg.pass;
	
	return dlgResult;
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
	//
	// Save gui state in all views
	//
	if ([m_dload_controller respondsToSelector:@selector(saveGui)]) {
		[m_dload_controller performSelector:@selector(saveGui)];
	}
	
	//
	// Save main window
	//
	[[NSUserDefaults standardUserDefaults] setInteger:[m_main_tabview indexOfTabViewItem: [m_main_tabview selectedTabViewItem] ] forKey:@"activeTab"];
	
	//
	// If we have slave daemon process, terminate it
	//
	if ( m_daemon_pid ) {
		//
		// started in local mode - terminate daemon
		//
		kill(m_daemon_pid, SIGTERM);
	}

	NSLog(@"Exiting ...\n");
}

- (void)restoreMainWindow {
	int activeTab = [[NSUserDefaults standardUserDefaults] integerForKey:@"activeTab"];
	[m_main_tabview selectTabViewItemAtIndex: activeTab];

}

- (void)awakeFromNib {
	NSUserDefaults *args = [NSUserDefaults standardUserDefaults];
	NSString *mode = [args stringForKey:@"mode"];
	NSLog(@"amule controller started, mode = [%@]\n", mode);
	
	
	[[NSApplication sharedApplication] setDelegate:self];
	
	[self restoreMainWindow];
	
	m_targetaddr = 0;
	m_targetport = 0;
	m_corepass = nil;
	
	m_daemon_pid = 0;
	if ( (mode != nil) && ([mode compare:@"guitest"] == NSOrderedSame) ) {
		NSLog(@"Started in GUI test mode - will not connect to core");
		return;
	}	

	if ( (mode != nil) && ([mode compare:@"remote"] == NSOrderedSame) ) {
		m_targetaddr = @"127.0.0.1";
		m_targetaddr = @"localhost";
		m_targetport = 4712;
		if (![self askCoreParams] ) {
			// "Cancel" selected on login dialog
			[[NSApplication sharedApplication] terminate:self];
		}
		[[NSUserDefaults standardUserDefaults] setObject:m_targetaddr forKey:@"LastTargetHost"];
		[[NSUserDefaults standardUserDefaults] setInteger:m_targetport forKey:@"LastTargetPort"];
		[[NSUserDefaults standardUserDefaults] synchronize];
		NSLog(@"Remote mode selected, target=%@:%d\n", m_targetaddr, m_targetport);
	} else {
		NSLog(@"Local mode selected - starting daemon\n");
		m_targetaddr = @"127.0.0.1";
		m_targetport = 4712;
		if ( [self startDaemon] == -1 ) {
			NSRunCriticalAlertPanel(@"Daemon startup error", 
							@"Unable to start core daemon (amuled)",
							@"OK", nil,nil);
			[[NSApplication sharedApplication] terminate:self];
		}
	}
	
	m_connection = [ECRemoteConnection remoteConnection];
	[m_connection retain];
	m_data = [amuleData initWithConnection:m_connection];
	[m_data retain];

	//
	// bind datastructure to GUI controllers
	//
	[m_dload_controller linkAmuleData:m_data];
	[m_search_controller linkAmuleData:m_data];
	[m_shared_controller linkAmuleData:m_data];
	//
	// daemon (either local or remote) must be running by now
	//
	// try to connect 3 times, hopefully giving daemon enough
	// time to start listening for EC
	//
	for(int i = 0; i < 3; i++) {
		sleep(1);
		[m_connection connectToAddress:m_targetaddr withPort:m_targetport];
		[m_connection sendLogin:@"123456"];
		if ( !m_connection.error ) {
			break;
		}
	}
	if ( m_connection.error ) {
		NSRunCriticalAlertPanel(@"Connection error", 
                        @"Unable to start communication with daemon",
                        @"OK", nil,nil);
		exit(-1);
	}

}


- (int)startDaemon {
	int pid = fork();
	if ( pid == -1 ) {
		// fork failed
		return -1;
	} else if ( pid > 0 ) {
		sleep(2);
		NSLog(@"Parent running, calling waitpid for pid %d\n", pid);
		// parent
		int status;
		switch ( waitpid(pid, &status, WNOHANG) ) {
			case -1:
				NSLog(@"waitpid() call failed with code %d\n", errno);
				break;
			case 0:
				NSLog(@"Daemon running on pid %d status %d\n", pid, status);
				m_daemon_pid = pid;
				break;
			default:
				//NSLog(@"waitpid returned pid=%d status=%x\n", pid, status);
				if ( WIFEXITED(status) ) {
					int exit_code = WEXITSTATUS(status);
					NSLog(@"Daemon exec failed - child process exited with status %d", exit_code);
					return -1;
				} else if ( WIFSIGNALED(status) ) {
					int sig_num = WTERMSIG(status);
					NSLog(@"Child process terminated on signal %d", sig_num);
					return -1;
				} else if ( WIFSTOPPED(status) ) {
					NSLog(@"Child process stopped. Not supposed to happen.");
					return -1;
				} else {
					NSLog(@"Should not get here: child status unknown = %x", status);
				}
				break;
		}
		return 0;
	} else {
		// child
		NSLog(@"Child running, calling execlp\n");
		//execlp("/usr/bin/touch", "/usr/bin/touch", "xxxx", 0);
		execlp("/Users/lfroen/prog/amule/src/amuled", "/Users/lfroen/prog/amule/src/amuled", 0);
		NSLog(@"execlp() failed\n");
		exit(-1);
	}
	return 0;
}

@end

#import "AppController.h"

#import "EC.h"

@implementation AppController

- (IBAction)show_Networks:(id)sender {
	[m_main_tabview selectTabViewItemAtIndex: 2];
	
	m_connection = [ECRemoteConnection remoteConnection];
	[m_connection retain];
	
	[m_connection connectToAddress:@"127.0.0.1" withPort:4712];

	[m_connection sendLogin:@"123456"];

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

	ECPacket *p = [ECPacket packetWithOpcode:EC_OP_GET_DLOAD_QUEUE];
	[m_connection sendPacket:p];
}

-(IBAction)show_Preferences:(id)sender {
}

-(IBAction)show_About:(id)sender {
}

-(void)awakeFromNib {
}

@end

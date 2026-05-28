// Copyright (C) 2026 Taichi Murakami.
#import "AppDelegate.h"

@implementation AppDelegate

// アプリケーションが安全であることを証明します。
- (BOOL)applicationSupportsSecureRestorableState:(NSApplication *)app {
	return YES;
}

- (void)applicationDidBecomeActive:(NSNotification *)notification {
	NSApplication *app = [NSApplication sharedApplication];

	if ([[app windows] count] <= 0) {
		[app sendAction:@selector(openDocument:) to:nil from:self];
	}
}

@end

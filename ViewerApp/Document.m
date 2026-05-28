// Copyright (C) 2026 Taichi Murakami.
#import "Document.h"
#import "ImageView.h"

@interface Document () {
	NSImage *_image;
}

@property (weak) IBOutlet ImageView *imageView;
@end
@implementation Document

// ドキュメントの書式を取得します。
+ (NSArray<NSString *> *) readableTypes {
	return @[@"public.image"];
}

// ビューに画像を設定します。
- (void)awakeFromNib {
	[_imageView setImage:_image];
}

// 画像ファイルを読み込みます。
- (BOOL)readFromData:(NSData *)data ofType:(NSString *)typeName error:(NSError **)outError {
	_image = [[NSImage alloc] initWithData: data];
	return _image != nil;
}

// ドキュメントを読み込み専用で開きます。
- (BOOL) isInViewingMode {
	return YES;
}

// NIB ファイルの名前を取得します。
- (NSString *)windowNibName {
	return @"Document";
}

@end

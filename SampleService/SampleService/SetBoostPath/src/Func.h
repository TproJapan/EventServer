#pragma once

#include <iostream>
#include <winsock.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define BUFSIZE 1024
#define BoostPathTxtPath "..\\..\\..\\Boost_Path.txt"//exeファイルと同ディレクトリ
#define vcxprojPath "..\\..\\..\\SampleService.vcxproj"//exeファイルからの相対path

//ファイル名filenameの中身の該当箇所にincludePath,libraryPathを設定し、string型newFileContentを生成
int changeFile(char* fileName,
	char* includePath,
	char* libraryPath,
	std::string& newFileContent);

//文字列置換
//s:検索対象文字列、str1;検索文字列、str2;置換文字列
char* replace(char* s, const char* before, const char* after);

//ファイル名file_Inにcontentを上書き保存
int saveFile(char* file_In, const char* content);
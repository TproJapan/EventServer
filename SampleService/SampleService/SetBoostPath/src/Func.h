#pragma once

#include <iostream>
#include <winsock.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define BUFSIZE 1024
#define BoostPathTxtPath "..\\..\\..\\Boost_Path.txt"//exe�t�@�C���Ɠ��f�B���N�g��
#define vcxprojPath "..\\..\\..\\SampleService.vcxproj"//exe�t�@�C������̑���path

//�t�@�C����filename�̒��g�̊Y���ӏ���includePath,libraryPath��ݒ肵�Astring�^newFileContent�𐶐�
int Change_File(char* fileName,
	char* includePath,
	char* libraryPath,
	std::string& newFileContent);

//������u��
//s:�����Ώە�����Astr1;����������Astr2;�u��������
char* replace(char* s, const char* before, const char* after);

//�t�@�C����file_In��content���㏑���ۑ�
int Save_File(char* file_In, const char* content);
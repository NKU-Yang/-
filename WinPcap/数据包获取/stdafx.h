// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include <WinSock2.h>
#include <Windows.h>
#include<tchar.h>
//the macro HAVE_REMOTE must define before
#ifndef  HAVE_REMOTE
#define HAVE_REMOTE
#endif
#include <pcap.h>
#include <remote-ext.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "packet.lib")
#pragma comment(lib, "wpcap.lib")
using namespace std;


// TODO:  �ڴ˴����ó�����Ҫ������ͷ�ļ�

#pragma once

#include <windows.h>

enum mouse_btn { LEFT_BUTTON = 1, RIGHT_BUTTON, MIDDLE_BUTTON, MOUSE };

struct Mouse
{
	Mouse() :
		action(0), button(-1)  
	{
		point.x = 0;
		point.y = 0;
	}


	WORD action;	// ������ƶ���˫��
	WORD button;	// ������Ҽ����м�
	POINT point;	// ����
};




/******************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017
See the license in LICENSE
******************************************************/

/*!
\file
Заголвочный файл camera_update - пространства имен для управления навигацией
*/

/*! \defgroup render_camera_update camera_update - пространство имен для управления навигацией
 \ingroup render
@{*/

#ifndef __CAMERA_UPDATE
#define __CAMERA_UPDATE

#include <windows.h>
#include <input/sxinput.h>
#include <render/gdata.h>

//! пространство имен для управления навигацией
namespace CameraUpdate
{
	//! обработка вводы информации с клавиатуры
	void UpdateInputKeyBoard(DWORD timeDelta);

	//! обработка движения мыши вправо и влево
	void UpdateInputMouseRotate(DWORD timeDelta);

	//! обработка движения мыши вверх вниз
	void UpdateInputMouseUpDown(DWORD timeDelta);

	//! центрирвоание курсора мыши
	void CentererCursor();

	//! обновление навигации режим "редактор"
	void UpdateEditorial(DWORD timeDelta);
};

#endif

//!@} render_camera_update
#include "stdafx.h"

#include "gui_design.hpp"
#ifndef _MSC_VER
#include <QFontDatabase>
#include <QFont>
#endif

QFontDatabase FontVerdana()
{
   QFontDatabase resourcesFont;
   resourcesFont.addApplicationFont(":/fonts/font/OpenSans-Regular.ttf");
   
   return resourcesFont;
}

QFontDatabase FontVerdanaBold()
{
   QFontDatabase resourcesFont;
   resourcesFont.addApplicationFont(":/fonts/font/OpenSans-Bold.ttf");
   
   return resourcesFont;
}


QFont PaginationFont()
{
   return FontVerdana().font("Open Sans", "Normal",
#if defined(_WIN32)
        8
#elif WINDOWS_HIGH_DPI
        8
#elif __APPLE__
        12
#elif   __linux__
        10
#else
        12
#endif
          );
}

QFont TableHeaderFont()
{
   return FontVerdanaBold().font("Open Sans", "Bold",

#if defined (_WIN32)
      8
#elif defined(WINDOWS_HIGH_DPI)
      8
#elif __APPLE__
      12
#elif __linux__
      10
#else
      14
#endif
      );
}

QFont AccountBalanceFont()
{
   return FontVerdana().font("Open Sans", "Regular",
#if defined(_WIN32)
      8
#elif WINDOWS_HIGH_DPI
      8
#elif __APPLE__
      12
#elif __linux__
      10
#else 
      14
#endif
      );
}

QFont DescriptionDetailsFont()
{
   return FontVerdana().font("Open Sans", "Regular", 
#ifdef WINDOWS_HIGH_DPI
      8
#else
      14
#endif
      );
}

QFont PopupButtonRegularFont()
{
   return FontVerdana().font("Open Sans", "Regular",
#if defined (_WIN32)
      8
#elif WINDOWS_HIGH_DPI
      8
#elif __APPLE__
      12
#elif __linux__
      10
#else
      14
#endif
      );
}

QFont PopupButtonBigFont()
{
   return FontVerdana().font("Open Sans", "Regular",
#if defined (_WIN32)
      8
#elif WINDOWS_HIGH_DPI
      8
#elif __APPLE__
      12
#elif __linux__
      10
#else
      14
#endif
      );
}

QFont TabButtonFont()
{
   return FontVerdana().font("Open Sans", "Regular",
#if defined (_WIN32)
      8
#elif WINDOWS_HIGH_DPI
      8
#elif __APPLE__
      12
#elif __linux__
      10
#else
      14
#endif
      );
}

QFont MainFont()
{
   return FontVerdana().font("Open Sans", "Regular",
#if defined(_WIN32)
      14
#elif WINDOWS_HIGH_DPI
      8
#elif __linux__
      10
#elif __APPLE__
      12
#else
      14
#endif
      );
}

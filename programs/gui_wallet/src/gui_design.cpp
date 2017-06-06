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

int const FontSize()
{
   return
#if defined(_WIN32)
      8
#elif defined (WINDOWS_HIGH_DPI)
      8
#elif __APPLE__
      12
#elif __linux__
      10
#else
      12
#endif
   ;
}


QFont PaginationFont()
{
   return FontVerdana().font("Open Sans", "Normal", FontSize());
}

QFont TableHeaderFont()
{
   return FontVerdanaBold().font("Open Sans", "Bold", FontSize());
}

QFont AccountBalanceFont()
{
   return FontVerdana().font("Open Sans", "Regular", FontSize());
}

QFont DescriptionDetailsFont()
{
   return FontVerdana().font("Open Sans", "Regular", FontSize());
}

QFont PopupButtonRegularFont()
{
   return FontVerdana().font("Open Sans", "Regular", FontSize());
}

QFont PopupButtonBigFont()
{
   return FontVerdana().font("Open Sans", "Regular", FontSize());
}

QFont TabButtonFont()
{
   return FontVerdana().font("Open Sans", "Regular", FontSize());
}

QFont MainFont()
{
   return FontVerdana().font("Open Sans", "Regular",
#if defined(_WIN32)
      14
#elif defined (WINDOWS_HIGH_DPI)
      8
#elif __linux__
      10
#elif __APPLE__
      12
#else
      12
#endif
      );
}

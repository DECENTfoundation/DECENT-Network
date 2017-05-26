#include "gui_design.hpp"

#include <QFontDatabase>
#include <QFont>

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
   return FontVerdana().font("Open Sans", "Normal", 14);
}

QFont TableHeaderFont()
{
   return FontVerdanaBold().font("Open Sans", "Bold",
#ifdef WINDOWS_HIGH_DPI
      10
#else
      14
#endif
      );
}

QFont AccountBalanceFont()
{
   return FontVerdana().font("Open Sans", "Regular",
#ifdef WINDOWS_HIGH_DPI
      8
#else
      12
#endif
      );
}

QFont DescriptionDetailsFont()
{
   return FontVerdana().font("Open Sans", "Regular", 
#ifdef WINDOWS_HIGH_DPI
      8
#else
      13
#endif
      );
}

QFont PopupButtonRegularFont()
{
   return FontVerdana().font("Open Sans", "Regular",
#ifdef WINDOWS_HIGH_DPI
      8
#else
      13
#endif
      );
}

QFont PopupButtonBigFont()
{
   return FontVerdana().font("Open Sans", "Regular",
#ifdef WINDOWS_HIGH_DPI
      8
#else
      15
#endif
      );
}

QFont TabButtonFont()
{
   return FontVerdana().font("Open Sans", "Regular",
#ifdef WINDOWS_HIGH_DPI
      8
#else
      14
#endif
      );
}
QFont MainFont()
{
   return FontVerdana().font("Open Sans", "Regular",
#ifdef WINDOWS_HIGH_DPI
      8
#else
      12
#endif
      );
}

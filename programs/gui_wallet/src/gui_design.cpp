#include "gui_design.hpp"

#include <QFontDatabase>
#include <QFont>
#include <iostream>

QFontDatabase FontVerdana()
{
   QFontDatabase resourcesFont;
   resourcesFont.addApplicationFont(":/fonts/font/Verdana.ttf");
   
   return resourcesFont;
}

QFontDatabase FontVerdanaBold()
{
   QFontDatabase resourcesFont;
   resourcesFont.addApplicationFont(":/fonts/font/Verdana Bold.ttf");
   
   return resourcesFont;
}


QFont PaginationFont()
{
   return FontVerdana().font("Verdana", "Normal", 14);
}

QFont TableHeaderFont()
{
   return FontVerdanaBold().font("Verdana", "Bold",
#ifdef WINDOWS_HIGH_DPI
      10
#else
      14
#endif
      );
}

QFont AccountBalanceFont()
{
   return FontVerdana().font("Verdana", "Regular", 12);
}

QFont DescriptionDetailsFont()
{
   return FontVerdana().font("Verdana", "Regular", 13);
}

QFont PopupButtonRegularFont()
{
   return FontVerdana().font("Verdana", "Regular",
#ifdef WINDOWS_HIGH_DPI
      8
#else
      13
#endif
      );
}

QFont PopupButtonBigFont()
{
   return FontVerdana().font("Verdana", "Regular",
#ifdef WINDOWS_HIGH_DPI
      8
#else
      15
#endif
      );
}

QFont TabButtonFont()
{
   return FontVerdana().font("Verdana", "Regular",
#ifdef WINDOWS_HIGH_DPI
      8
#else
      14
#endif
      );
}

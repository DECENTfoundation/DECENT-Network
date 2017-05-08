#include "gui_design.hpp"

#include <QFont>

QFont PaginationFont()
{
   return QFont("Myriad Pro Regular",14, QFont::Weight::Normal);
}

QFont TableHeaderFont()
{
   return QFont("Open Sans Bold",
#ifdef WINDOWS_HIGH_DPI
      10,
#else
      14,
#endif
      QFont::Bold);
}

QFont AccountBalanceFont()
{
   return QFont("Myriad Pro Regular", 12, QFont::Bold);
}

QFont DescriptionDetailsFont()
{
   return QFont("Myriad Pro Regular", 13);
}

QFont PopupButtonRegularFont()
{
   return QFont("Myriad Pro Regular",
#ifdef WINDOWS_HIGH_DPI
      8,
#else
      13,
#endif
      QFont::Bold);
}

QFont PopupButtonBigFont()
{
   return QFont("Myriad Pro Regular",
#ifdef WINDOWS_HIGH_DPI
      8,
#else
      15,
#endif
      QFont::Bold);
}

QFont TabButtonFont()
{
   return QFont( "Myriad Pro Regular",
#ifdef WINDOWS_HIGH_DPI
      8,
#else
      14, 
#endif
      QFont::Bold);
}

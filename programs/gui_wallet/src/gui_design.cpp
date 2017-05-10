#include "gui_design.hpp"

#include <QFont>

QFont PaginationFont()
{
   return QFont("Verdana Regular",14, QFont::Weight::Normal);
}

QFont TableHeaderFont()
{
   return QFont("Verdana Bold",
#ifdef WINDOWS_HIGH_DPI
      10,
#else
      14,
#endif
      QFont::Bold);
}

QFont AccountBalanceFont()
{
   return QFont("Verdana Regular", 12, QFont::Bold);
}

QFont DescriptionDetailsFont()
{
   return QFont("Verdana Regular", 13);
}

QFont PopupButtonRegularFont()
{
   return QFont("Verdana Regular",
#ifdef WINDOWS_HIGH_DPI
      8,
#else
      13,
#endif
      QFont::Bold);
}

QFont PopupButtonBigFont()
{
   return QFont("Verdana Regular",
#ifdef WINDOWS_HIGH_DPI
      8,
#else
      15,
#endif
      QFont::Bold);
}

QFont TabButtonFont()
{
   return QFont( "Verdana Regular",
#ifdef WINDOWS_HIGH_DPI
      8,
#else
      14, 
#endif
      QFont::Bold);
}

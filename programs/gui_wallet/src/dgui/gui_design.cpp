#include "gui_design.hpp"

#include <QFont>

const char* const d_global_white_style =
"QDialog, QMainWindow, QMessageBox"
"{"
   "color:black;"
   "background-color:white;"
"}"
"QPushButton"
"{"
   "border: 0px ;"
   "background-color :rgb(27,176,104);"
   "color : white;"
   "min-width: 80px;"
   "min-height: 25px;"
   "width: 100px;"
   "height: 100px;"
   "max-width: 1000px;"
   "max-height: 1000px;"
"}"
"QPushButton:!enabled"
"{"
   "background-color :rgb(180,180,180);"
   "color : rgb(30, 30, 30);"
   "min-width: 80px;"
   "min-height: 25px;"
   "width: 100px;"
   "height: 100px;"
   "max-width: 1000px;"
   "max-height: 1000px;"
"}";

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

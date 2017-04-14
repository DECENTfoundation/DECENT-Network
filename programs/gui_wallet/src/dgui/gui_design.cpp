#include "gui_design.hpp"

#include <QFont>

QFont TableHeaderFont()
{
   return QFont("Open Sans Bold", 14, QFont::Bold);
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
   return QFont("Myriad Pro Regular", 13, QFont::Bold);
}

QFont PopupButtonBigFont()
{
   return QFont("Myriad Pro Regular", 15, QFont::Bold);
}

QFont TabButtonFont()
{
   return QFont( "Myriad Pro Regular", 14, QFont::Bold);
}


#include "gui_wallet_global.hpp"

#include <QMessageBox>
#include <iostream>

namespace gui_wallet
{
   
   

void makeWarningImediatly(const char* a_WaringTitle, const char* a_WaringText, const char* a_detailed, void* a_pParent )
{
   QMessageBox aMessageBox(QMessageBox::Warning,QObject::tr(a_WaringTitle),QObject::tr(a_WaringText),
                           QMessageBox::Ok,(QWidget*)a_pParent);
   aMessageBox.setDetailedText(QObject::tr(a_detailed));
   aMessageBox.exec();
}
   
std::string CalculateRemainingTime(QDateTime const& dt, QDateTime const& dtFuture)
{
   if (dtFuture <= dt)
      return "expired";
   else
   {
      QDate d = dt.date();
      QDate dFuture = dtFuture.date();

      int iMonthsDiff = 0, iMonthsStep = 12*12;
      while (true)
      {
         QDate d_temp = d.addMonths(iMonthsStep);
         if (d_temp > dFuture &&
             1 == iMonthsStep)
            break;
         else if (d_temp > dFuture)
            iMonthsStep /= 12;
         else
         {
            d = d_temp;
            iMonthsDiff += iMonthsStep;
         }
      }

      int iYearsDiff = iMonthsDiff / 12;
      iMonthsDiff %= 12;

      QDateTime dt2(d, dt.time());
      // 60*24 minutes (a day) = 12*5*12*2 minutes = 12*12*10
      // so the step here is almost a day
      int iMinutesDiff = 0, iMinutesStep = 12 * 12 * 12;
      while (true)
      {
         QDateTime dt2_temp = dt2.addSecs(60 * iMinutesStep);
         if (dt2_temp > dtFuture &&
             1 == iMinutesStep)
            break;
         else if (dt2_temp > dtFuture)
            iMinutesStep /= 12;
         else
         {
            dt2 = dt2_temp;
            iMinutesDiff += iMinutesStep;
         }
      }

      int iHoursDiff = iMinutesDiff / 60;
      iMinutesDiff %= 60;

      int iDaysDiff = iHoursDiff / 24;
      iHoursDiff %= 24;

      std::vector<std::string> arrParts;

      if (iYearsDiff)
         arrParts.push_back(std::to_string(iYearsDiff) + " y");
      if (iMonthsDiff)
         arrParts.push_back(std::to_string(iMonthsDiff) + " m");
      if (iDaysDiff)
         arrParts.push_back(std::to_string(iDaysDiff) + " d");
      if (iHoursDiff)
         arrParts.push_back(std::to_string(iHoursDiff) + " h");
      if (iMinutesDiff)
         arrParts.push_back(std::to_string(iMinutesDiff) + " min");

      std::string str_result;
      if (arrParts.empty())
         str_result = "expiring in a minute";
      else
      {
         str_result = arrParts.front();
         
         if (arrParts.size() > 1)
            str_result += " " + arrParts[1];
      }
      
      return str_result;
   }
}
   
   
}

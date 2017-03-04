/*
 *	File: gui_wallet_global.cpp
 *
 *	Created on: 30 Nov, 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements the global functions,
 *  that can be used by different classes
 *
 */

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

    
    
    std::string CalculateRemainingTime(QDateTime now_time , QDateTime time)
    {
        int now_year = now_time.date().year();
        int year =  time.date().year();
        
        int now_month = now_time.date().month();
        int month = time.date().month();
        
        
        int now_day = now_time.date().day();
        int day = time.date().day();
        
        int e_year = 0;
        int e_month = 0;
        int e_day = 0;
        int e_hour = 0;
        int e_min = 0;
        int e_sec = 0;
        std::cout<<"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"<<now_year<<"  "<<now_month<<"   "<<now_day<<"   "<<year<<"   "<<month<<"   "<<day<<std::endl;
        if(now_time < time)
        {
            if(now_time.date() != time.date())
            {
                if(year >= now_year)
                {
                    if(year == now_year)
                    {
                        e_year = 0;
                        if(month >= now_month)
                        {
                            if(month == now_month)
                            {
                                e_month = 0;
                                if(time.time() > now_time.time())
                                {
                                    e_day = time.date().day() - now_time.date().day();
                                    e_hour = time.time().hour() - now_time.time().hour();
                                }
                                else
                                {
                                    e_day = time.date().day() - now_time.date().day() - 1;
                                    if(e_day == 0)
                                    {
                                        if(time.time().minute() > now_time.time().minute())
                                        {
                                            e_hour = time.time().hour() + 24 - now_time.time().hour();
                                            e_min =time.time().minute() - now_time.time().minute();
                                        }
                                        else
                                        {
                                            e_hour = time.time().hour() + 24 - now_time.time().hour() - 1;
                                            e_min =time.time().minute() - now_time.time().minute() + 60;
                                        }
                                    }
                                    else
                                    {
                                        if(time.time().minute() > now_time.time().minute())
                                        {
                                            e_hour = time.time().hour() + 24 - now_time.time().hour();
                                        }
                                        else
                                        {
                                            e_hour = time.time().hour() + 24 - now_time.time().hour() - 1;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                if(day >= now_day)
                                {
                                    e_month = month - now_month;
                                    e_day = day - now_day;
                                }
                                else
                                {
                                    e_month = month - now_month - 1;
                                    if(e_month == 0)
                                    {
                                        if(time.time() > now_time.time())
                                        {
                                            e_day = now_time.date().daysInMonth() - now_time.date().day() + time.date().day();
                                            e_hour = time.time().hour() - now_time.time().hour();
                                        }
                                        else
                                        {
                                            e_day = now_time.date().daysInMonth() - now_time.date().day() + time.date().day() - 1;
                                            e_hour = time.time().hour() + 24 - now_time.time().hour();
                                        }
                                    }
                                    else
                                    {
                                        e_day = now_time.date().daysInMonth() - now_time.date().day() + time.date().day();
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        if(month >= now_month)
                        {
                            e_year = year - now_year;
                            e_month = month - now_month;
                        }
                        else
                        {
                            e_year = year - now_year - 1;
                            e_month = 12 - now_month + month;
                        }
                    }
                }
            }
            else
            {
                e_year = 0;
                e_month = 0;
                e_day = 0;
                
                int sec = now_time.time().secsTo(time.time());
                e_hour = sec / 3600;
                sec -= e_hour * 3600;
                
                e_min = sec / 60;
                e_sec = sec - e_min * 60;
            }
        }
        std::string e_str;
        
        if(e_year != 0)
        {
            e_str = std::to_string(e_year) + " year " + std::to_string(e_month) + " month";
        }
        else
        {
            if(e_month != 0)
            {
                e_str = std::to_string(e_month) + " month " + std::to_string(e_day) + " day";
            }
            else
            {
                if(e_day != 0)
                {
                    e_str = std::to_string(e_day) + " day " + std::to_string(e_hour) + " hour";
                }
                else
                {
                    if(e_hour != 0)
                    {
                        e_str = std::to_string(e_hour) + " hour " + std::to_string(e_min) + " minute";
                    }
                    else
                    {
                        if(e_min != 0)
                        {
                            e_str = std::to_string(e_min) + " minute";
                        }
                    }
                }
            }
        }
        
        
        return e_str;
    };
}

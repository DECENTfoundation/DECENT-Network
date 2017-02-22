/*
 *	File      : overview_tab.hpp
 *
 *	Created on: 21 Nov 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef OVERVIEW_TAB_HPP
#define OVERVIEW_TAB_HPP

#include <QWidget>
#include <QPushButton>
#include <QTextBrowser>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <vector>
#include <QTableWidget>
#include <map>
#include <QStringList>


namespace gui_wallet
{
    class Overview_tab : public QWidget
    {

        Q_OBJECT
    public:
        Overview_tab();
        virtual ~Overview_tab();
        void CreateTable();
    public slots:
        void func(int);
    public:
        QLineEdit search;
        QTextBrowser text;
        QTableWidget table_widget;
        std::vector<QString> accounts_names;
        std::map<std::string , std::string> info;
    protected:
    };
}

#endif // OVERVIEW_TAB_HPP

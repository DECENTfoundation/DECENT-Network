
#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QString>
#include <QHeaderView>
#include <QTextStream>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFrame>
#include <Qt>
#include <QColor>
#include <QMouseEvent>
#include <QTimer>

#include <iostream>
#include <vector>

#include "gui_wallet_tabcontentmanager.hpp"
#include "gui_wallet_global.hpp"


namespace gui_wallet
{
   class Mainwindow_gui_wallet;
   class TransactionsTab : public TabContentManager {
      
      Q_OBJECT
   public:
      TransactionsTab(QWidget* pParent);
      virtual void timeToUpdate(const std::string& result);
      virtual std::string getUpdateCommand();

      void set_user_filter(const std::string& user_name);
      
   public:
      QVBoxLayout       main_layout;
      QLabel            search_label;
      DecentTable       tablewidget;
      QLineEdit         user;
      
   private:
      std::string getAccountName(std::string accountId);
      
   public slots:
      
      void currentUserChanged(std::string user);
      
   private:
      QTimer   m_contentUpdateTimer;
      bool     m_doUpdate = true;
      
      std::map<std::string, std::string> _user_id_cache;
   };
}


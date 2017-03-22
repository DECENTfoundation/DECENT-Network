
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
      TransactionsTab(Mainwindow_gui_wallet* pMainWindow);
      
      virtual void content_activated() {}
      virtual void content_deactivated() {}

      void set_user_filter(const std::string& user_name);

      void RunTask(std::string const& str_command, std::string& str_result);
      
   public:
      QVBoxLayout       main_layout;
      QLabel            search_label;
      DecentTable       tablewidget;
      QLineEdit         user;
      
   private:
      std::string getAccountName(std::string accountId);
      
   public slots:
      void onTextChanged(const QString& text);
      void updateContents();
      void maybeUpdateContent();
      void currentUserChanged(std::string user);
      
   private:
      QTimer   m_contentUpdateTimer;
      bool     m_doUpdate = true;
      
      std::map<std::string, std::string> _user_id_cache;
      Mainwindow_gui_wallet* m_pMainWindow;
   };
}


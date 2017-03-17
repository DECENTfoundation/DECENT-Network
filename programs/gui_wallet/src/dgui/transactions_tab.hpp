
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



class HTableWidget : public QTableWidget
{
   Q_OBJECT
public:
   HTableWidget() : QTableWidget() {
      this->setMouseTracking(true);
   }
   
   
   virtual void mouseMoveEvent(QMouseEvent *event) {
      mouseMoveEventDid();
   }
   
   
public:
signals:
   void mouseMoveEventDid();
};




namespace gui_wallet
{
   class TransactionsTab : public TabContentManager {
      
      Q_OBJECT
   public:
      TransactionsTab();
      ~TransactionsTab();
      
      virtual void content_activated() {}
      virtual void content_deactivated() {}
      virtual void resizeEvent(QResizeEvent *a_event);
      
      void createNewRow();
      void ArrangeSize();
      void Connects();
      
      void SetInfo(std::string info_from_overview);
      
   public:
      QVBoxLayout       main_layout;
      QLabel            search_label;
      HTableWidget*     tablewidget;
      QTableWidgetItem* itm;
      QPushButton*      more;
      QLineEdit         user;
      int               green_row;
      
   private:
      std::string getAccountName(std::string accountId);
      
   public slots:
      
      void doRowColor();
      void onTextChanged(const QString& text);
      void updateContents();
      void maybeUpdateContent();
      void currentUserChanged(std::string user);
      
   private:
      QTimer   m_contentUpdateTimer;
      bool     m_doUpdate = true;
      
      std::map<std::string, std::string> _user_id_cache;
      
      const std::vector<std::string> _table_columns = { "Type", "From", "To", "Amount", "Fee", "Description" };
   };
}


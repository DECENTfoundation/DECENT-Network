//decent_wallet_ui_gui_purchasedtab
/*
 *	File: decent_wallet_ui_gui_purchasedtab.hpp
 *
 *	Created on: 11 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */

#ifndef DECENT_WALLET_UI_GUI_PURCHASEDTAB_HPP
#define DECENT_WALLET_UI_GUI_PURCHASEDTAB_HPP

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QLabel>
#include <QLineEdit>
#include <QTimer>
#include <QPushButton>

#include "qt_commonheader.hpp"
#include "gui_wallet_tabcontentmanager.hpp"
#include "gui_wallet_global.hpp"

#include <string>
#include <iostream>

class P_TableWidget : public QTableWidget
{
   Q_OBJECT
public:
   P_TableWidget(int row, int col) : QTableWidget(row,col) {
      this->setMouseTracking(true);
   }
   
   virtual void mouseMoveEvent(QMouseEvent * event) {
      emit mouseMoveEventDid(event->pos());
   }
public:
signals:
   void mouseMoveEventDid(QPoint position);
};




namespace gui_wallet {
   
   
   
   
   class PurchasedTab : public TabContentManager
   {
      
      Q_OBJECT;
            
   public:
      PurchasedTab();
      virtual ~PurchasedTab();
      
      void ShowDigitalContentsGUI(std::vector<SDigitalContent>& contents);
      
   public:
      virtual void content_activated() {
         m_contentUpdateTimer.start();
      }
      virtual void content_deactivated() {
         m_contentUpdateTimer.stop();
      }
      
   public:
   signals:
      void ShowDetailsOnDigContentSig(SDigitalContent dig_cont);
      
   protected:
      void PrepareTableWidgetHeaderGUI();
      virtual void resizeEvent ( QResizeEvent * a_event );
      void ArrangeSize();
      
   public slots:
      void hightlight_row(QPoint point);
      void onTextChanged(const QString& text);
      void updateContents();
      void maybeUpdateContent();
      void extractPackage();
      
      void show_content_popup();
      
      
   private:
      QTimer        m_contentUpdateTimer;
      bool          m_doUpdate = true;
      std::string   last_contents;
      
      std::vector<SDigitalContent>   _current_content;
      const std::vector<std::string> _column_names = {" ", "Title", "Rating", "Size", "Price", "Created", "Status", "Progress"};
      
      
      
   protected:
      QVBoxLayout     m_main_layout;
      P_TableWidget*  m_pTableWidget;
      QLineEdit       m_filterLineEditer;
      
   };
   
}


#include "decent_wallet_ui_gui_common.tos"

#endif // DECENT_WALLET_UI_GUI_PURCHASEDTAB_HPP

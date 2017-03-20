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




namespace gui_wallet {
   

class ContentDetailsBase;


class PurchasedTab : public TabContentManager
{
   
   Q_OBJECT;
         
public:
   PurchasedTab();
   
   void ShowDigitalContentsGUI(std::vector<SDigitalContent>& contents);
   
public:
   virtual void content_activated() {
      m_contentUpdateTimer.start();
   }
   virtual void content_deactivated() {
      m_contentUpdateTimer.stop();
   }
   
protected:
   void PrepareTableWidgetHeaderGUI();
   
   
public slots:
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
   
protected:
   QVBoxLayout             m_main_layout;
   DecentTable             m_pTableWidget;
   QLineEdit               m_filterLineEditer;
   ContentDetailsBase*     _details_dialog = nullptr;

};
   
   
   
}



#endif // DECENT_WALLET_UI_GUI_PURCHASEDTAB_HPP

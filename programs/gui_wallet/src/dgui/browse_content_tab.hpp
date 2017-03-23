/*
 *	File: BrowseContentTab.hpp
 *
 *	Created on: 11 Nov 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef BrowseContentTab_H
#define BrowseContentTab_H


#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <vector>
#include <string>
#include "qt_commonheader.hpp"
#include <QLineEdit>
#include <QHBoxLayout>
#include <QComboBox>
#include <QTimer>
#include <QLabel>
#include "gui_wallet_tabcontentmanager.hpp"
#include "gui_wallet_global.hpp"
#include "decent_wallet_ui_gui_contentdetailsgeneral.hpp"



namespace gui_wallet
{
   class Mainwindow_gui_wallet;
   
   
   class BrowseContentTab : public TabContentManager
   {
      Q_OBJECT;
   public:
      BrowseContentTab(Mainwindow_gui_wallet* parent);
      
      void ShowDigitalContentsGUI();
      
   public:
      
      virtual void contentActivated() { }
      virtual void contentDeactivated() {}
      virtual void timeToUpdate(const std::string& result);
      virtual std::string getUpdateCommand();
       
   public slots:
      
      void show_content_popup();
      void content_was_bought();
      
   protected:
      QVBoxLayout     m_main_layout;
      QHBoxLayout     m_search_layout;
      DecentTable     m_pTableWidget;
      QLineEdit       m_filterLineEdit;
      QComboBox       m_searchTypeCombo;
      
      std::vector<SDigitalContent>  _digital_contents;
      ContentDetailsGeneral*        _content_popup;
      Mainwindow_gui_wallet*        _parent;
   };
   
   
}

#endif // BrowseContentTab_H

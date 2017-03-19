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


namespace gui_wallet
{
   
   class BrowseContentTab : public TabContentManager
   {
      Q_OBJECT;
   public:
      BrowseContentTab();
      
      void ShowDigitalContentsGUI();
      
   public:
      
      virtual void content_activated() { m_doUpdate = true; }
      virtual void content_deactivated() {}
      
      
   public:
   signals:
      void ShowDetailsOnDigContentSig(SDigitalContent get_cont_str);
      
   public slots:
      
      void onTextChanged(const QString& text);
      void updateContents();
      void maybeUpdateContent();
      void show_content_popup();
      
   protected:
      QVBoxLayout     m_main_layout;
      QHBoxLayout     m_search_layout;
      DecentTable     m_pTableWidget;
      QLineEdit       m_filterLineEdit;
      QComboBox       m_searchTypeCombo;
      
      std::vector<SDigitalContent> _digital_contents;
      bool                         m_doUpdate = true;
      QTimer                       m_contentUpdateTimer;
   };
   
   
}

#endif // BrowseContentTab_H

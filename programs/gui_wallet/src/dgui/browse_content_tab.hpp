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
      virtual ~BrowseContentTab();
      
      void ShowDigitalContentsGUI(std::vector<SDigitalContent>& contents);
      
   public:
      
      virtual void content_activated() { m_doUpdate = true; }
      virtual void content_deactivated() {}
      virtual void resizeEvent(QResizeEvent * a_event);
      
      
   public:
   signals:
      void ShowDetailsOnDigContentSig(SDigitalContent get_cont_str);
      
   public slots:
      
      void onTextChanged(const QString& text);
      void hightlight_row(QPoint point);
      void updateContents();
      void maybeUpdateContent();
      
   protected:
      void DigContCallback(_NEEDED_ARGS2_);
      void PrepareTableWidgetHeaderGUI();
      void ArrangeSize();
      
      
   protected:
      QVBoxLayout     m_main_layout;
      QHBoxLayout     m_search_layout;
      DecentTable*    m_pTableWidget;
      QLineEdit       m_filterLineEdit;
      QComboBox       m_searchTypeCombo;
      
      std::vector<SDigitalContent> m_dContents;
      bool m_doUpdate = true;
      int green_row;
      QTimer  m_contentUpdateTimer;
   };
   
   
}

#endif // BrowseContentTab_H

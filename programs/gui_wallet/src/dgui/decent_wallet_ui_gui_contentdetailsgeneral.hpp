/*
 *	File: decent_wallet_ui_gui_contentdetailsbougth.hpp
 *
 *	Created on: 22 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef DECENT_WALLET_UI_GUI_CONTENTDETAILSGENERAL_HPP
#define DECENT_WALLET_UI_GUI_CONTENTDETAILSGENERAL_HPP

#include "decent_wallet_ui_gui_contentdetailsbase.hpp"
#include "decent_button.hpp"
#include <QLabel>

namespace gui_wallet {

//<<<<<<< HEAD
//   class ContentDetailsGeneral : public ContentDetailsBase
//   {
//      Q_OBJECT
//   public:
//      ContentDetailsGeneral(Mainwindow_gui_wallet* pMainWindow);
//
//      virtual void execCDD(const SDigitalContent& a_cnt_details);
//
//   public:
//   signals:
//      void ContentWasBought();
//
//      protected slots:
//      void LabelPushCallbackGUI();
//      
//      
//   protected:
//      DecentButton   m_label;
//   };
//=======

class ContentDetailsGeneral : public ContentDetailsBase
{
    Q_OBJECT
public:
    ContentDetailsGeneral(Mainwindow_gui_wallet* pMainWindow);

    virtual void execCDD(const SDigitalContent& a_cnt_details);

public:
signals:
    void ContentWasBought();

protected slots:
    void LabelPushCallbackGUI();
   

protected:
    DecentButton   m_label;
    DecentButton   m_close;
};
//>>>>>>> 601e33eff763d3315666f2b397ef7005a1b21d96

}



#endif // DECENT_WALLET_UI_GUI_CONTENTDETAILSGENERAL_HPP

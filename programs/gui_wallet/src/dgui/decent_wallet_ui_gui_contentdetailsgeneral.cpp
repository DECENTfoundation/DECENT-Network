#include "stdafx.h"

#ifndef _MSC_VER
#include <QGraphicsDropShadowEffect>
#endif
#include "decent_wallet_ui_gui_contentdetailsgeneral.hpp"
#include "gui_wallet_global.hpp"
#include "ui_wallet_functions.hpp"

#ifndef _MSC_VER
#include <QMouseEvent>
#include <QMessageBox>
#include <QFileDialog>
#endif

using namespace gui_wallet;
ContentDetailsGeneral::ContentDetailsGeneral() {
   QVBoxLayout* image_layout = new QVBoxLayout;
   m_label.setText("Get it");
   m_label.setFixedWidth(120);
   m_label.setFixedHeight(30);
   
   image_layout->addWidget(&m_label);
   image_layout->setContentsMargins(250, 5, 250, 10);
   
   m_free_for_child.addLayout(image_layout);
   
   connect(&m_label, SIGNAL(LabelClicked()), this, SLOT(LabelPushCallbackGUI()));
   setFixedSize(620,480);
}


void ContentDetailsGeneral::execCDD(const SDigitalContent& a_cnt_details) {
   execCDB(a_cnt_details);
}


void ContentDetailsGeneral::LabelPushCallbackGUI()
{
   
   QMessageBox::StandardButton reply;
   reply = QMessageBox::question(this, "Please confirm", "Do you really want to buy this content?",
                                 QMessageBox::Yes|QMessageBox::No);
   if (reply != QMessageBox::Yes) {
      return;
   }
   
   
   std::string downloadCommand = "download_content";
   downloadCommand += " " + GlobalEvents::instance().getCurrentUser();   //consumer
   downloadCommand += " \"" + m_pContentInfo->URI + "\"";                 //URI
   downloadCommand += " true";                                           //broadcast
   
   std::string a_result;
   
   SetNewTask(downloadCommand, this, NULL,
#if defined( _MSC_VER )
      []
#else
      +[]
#endif
      (void* owner, void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {
      if (a_err != 0) {
         ALERT("Failed to download content");
      }
   });
   emit ContentWasBought();
   
}





#include <QGraphicsDropShadowEffect>

#include "decent_wallet_ui_gui_contentdetailsgeneral.hpp"
#include "gui_wallet_global.hpp"
#include "ui_wallet_functions.hpp"

#include <QMouseEvent>
#include <QMessageBox>
#include <QFileDialog>

using namespace gui_wallet;
ContentDetailsGeneral::ContentDetailsGeneral() {
   QHBoxLayout* image_layout = new QHBoxLayout;
   m_label.setText("Get it");
   m_label.setFixedWidth(120);
   m_label.setFixedHeight(30);
   
   m_close.setText("Close");
   m_close.setFixedWidth(120);
   m_close.setFixedHeight(30);
   m_close.setStyleSheet("QLabel { background-color :rgb(255,255,255); color : rgb(0,0,0);border: 1px solid grey}");
   //m_close.setStyleSheet("border: 1px solid grey");
   
   image_layout->addWidget(&m_label);
   image_layout->addWidget(new QLabel());
   image_layout->addWidget(&m_close);
   image_layout->setContentsMargins(180, 5, 180, 10);
   
   m_free_for_child.addLayout(image_layout);
   
   connect(&m_label, SIGNAL(LabelClicked()), this, SLOT(LabelPushCallbackGUI()));
   connect(&m_close, SIGNAL(LabelClicked()), this, SLOT(close()));
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
   try {
      RunTask(downloadCommand, a_result);
      
      close();
      emit ContentWasBought();

   } catch (const std::exception& ex) {
      ALERT("Failed to download content");
   }
   

   
   
}





#include <QGraphicsDropShadowEffect>

#include "decent_wallet_ui_gui_contentdetailsgeneral.hpp"
#include "gui_wallet_global.hpp"
#include "ui_wallet_functions.hpp"

#include <QMouseEvent>
#include <QMessageBox>
#include <QFileDialog>

using namespace gui_wallet;

//<<<<<<< HEAD
//// Implemented in 'gui_wallet_centralwigdet.cpp'
//std::string FindImagePath(bool& a_bRet,const char* a_image_name);
//
//
//ContentDetailsGeneral::ContentDetailsGeneral()
//    :
//      m_label(this,NULL,&ContentDetailsGeneral::LabelPushCallbackGUI)
//{
//    QVBoxLayout* image_layout = new QVBoxLayout;
//    m_label.setText("GET IT");
//    m_label.setFixedWidth(120);
//    m_label.setFixedHeight(30);
//    
//    image_layout->addWidget(&m_label);
//    image_layout->setContentsMargins(250, 5, 250, 10);
//    
//    m_free_for_child.addLayout(image_layout);
//
//    
//    
//=======
//>>>>>>> b00b82b867e92361ad442a6ce8f2f8a7b452519d
//

ContentDetailsGeneral::ContentDetailsGeneral() {
   QVBoxLayout* image_layout = new QVBoxLayout;
   m_label.setText("GET IT");
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
   try {
      RunTask(downloadCommand, a_result);
      
      close();
      emit ContentWasBought();

   } catch (const std::exception& ex) {
      ALERT("Failed to download content");
   }
   

   
   
}




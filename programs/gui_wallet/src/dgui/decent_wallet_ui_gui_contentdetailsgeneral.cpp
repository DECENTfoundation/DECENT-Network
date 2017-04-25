#include "stdafx.h"

#ifndef _MSC_VER
#include <QGraphicsDropShadowEffect>
#endif
#include "decent_wallet_ui_gui_contentdetailsgeneral.hpp"
#include "gui_wallet_global.hpp"
#include "gui_wallet_mainwindow.hpp"
#include "gui_design.hpp"

#ifndef _MSC_VER
#include <QMouseEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QPushButton>
#endif

using namespace gui_wallet;
ContentDetailsGeneral::ContentDetailsGeneral(QWidget* pParent) : ContentDetailsBase(pParent){
   QHBoxLayout* image_layout = new QHBoxLayout;
   m_label.setText(tr("Get it!"));
   m_label.setFixedWidth(178);
   m_label.setFixedHeight(40);
   
   m_close.setText(tr("Close"));
   m_close.setFixedWidth(178);
   m_close.setFixedHeight(40);
   m_close.setStyleSheet(d_close);
   
   image_layout->addWidget(&m_label);
   image_layout->addWidget(new QLabel());
   image_layout->addWidget(&m_close);
   image_layout->setContentsMargins(115, 15, 115, 15);
   
   m_free_for_child.addLayout(image_layout);
   
   connect(&m_label, SIGNAL(LabelClicked()), this, SLOT(LabelPushCallbackGUI()));
   connect(&m_close, SIGNAL(LabelClicked()), this, SLOT(close()));
}


void ContentDetailsGeneral::execCDD(const SDigitalContent& a_cnt_details) {
   execCDB(a_cnt_details);
}


void ContentDetailsGeneral::LabelPushCallbackGUI()
{
   QMessageBox* reply = new QMessageBox();
   reply->setFixedSize(500, 400);
   reply->setContentsMargins(0, 30, 80, 30);
   reply->setWindowFlags(Qt::WindowTitleHint);
   reply->QDialog::setWindowTitle(tr("Decent-Blockchain Content Distributor"));
   reply->setText(tr("          Are you sure you want to buy this content?"));
   QPushButton* pButtonCencel = reply->addButton(tr("Cencel"), QMessageBox::YesRole);
   QPushButton* pButtonOk = reply->addButton(tr("Get it"), QMessageBox::NoRole);
   pButtonOk->setStyleSheet(d_pButtonOk);
   pButtonCencel->setStyleSheet(d_pbuttonCancel);
   pButtonOk->setFixedSize(100, 30);
   pButtonCencel->setFixedSize(100, 30);
   reply->exec();
   if (reply->clickedButton()==pButtonCencel) {
      return;
   }
   std::string downloadCommand = "download_content";
   downloadCommand += " " + Globals::instance().getCurrentUser();  // consumer
   downloadCommand += " \"" + m_pContentInfo->URI + "\"";               // URI
   downloadCommand += " \"\"";                                          // region_code
   downloadCommand += " true";                                          // broadcast
   
   std::string a_result;

   std::string str_error;
   try
   {
      RunTask(downloadCommand, a_result);
   }
   catch(std::exception const& ex)
   {
      str_error = ex.what();
   }
   if (false == str_error.empty())
      ALERT(tr("Failed to download content").toStdString() + str_error.c_str());


   emit ContentWasBought();
   
}




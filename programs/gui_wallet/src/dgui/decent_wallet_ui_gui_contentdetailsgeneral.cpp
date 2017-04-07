
#include <QGraphicsDropShadowEffect>

#include "decent_wallet_ui_gui_contentdetailsgeneral.hpp"
#include "gui_wallet_global.hpp"
#include "gui_wallet_mainwindow.hpp"

#include <QMouseEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QPushButton>

using namespace gui_wallet;
ContentDetailsGeneral::ContentDetailsGeneral(Mainwindow_gui_wallet* pMainWindow) : ContentDetailsBase(pMainWindow){
   QHBoxLayout* image_layout = new QHBoxLayout;
   m_label.setText("Get it!");
   m_label.setFixedWidth(178);
   m_label.setFixedHeight(40);
   
   m_close.setText("Close");
   m_close.setFixedWidth(178);
   m_close.setFixedHeight(40);
   m_close.setStyleSheet("QLabel { background-color :rgb(255,255,255); color : rgb(0,0,0);border: 1px solid grey}");
   
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
   reply->QDialog::setWindowTitle("Decent-Blockchain Content Distributor");
   reply->setText(tr("          Are you sure you want to buy this content?"));
   QPushButton* pButtonCencel = reply->addButton(tr("Cencel"), QMessageBox::YesRole);
   QPushButton* pButtonOk = reply->addButton(tr("Get it"), QMessageBox::NoRole);
   pButtonOk->setStyleSheet("background-color: rgb(27,176,104); color: rgb(255,255,255);border-top: 0px;border-left: 0px;border-right: 0px;border-bottom: 0px;");
   pButtonCencel->setStyleSheet("background-color: rgb(255,255,255); color: rgb(0,0,0);border: 1px solid grey;");
   pButtonOk->setFixedSize(100, 30);
   pButtonCencel->setFixedSize(100, 30);
   reply->exec();
   if (reply->clickedButton()==pButtonCencel) {
      return;
   }
   std::string downloadCommand = "download_content";
   downloadCommand += " " + GlobalEvents::instance().getCurrentUser();   //consumer
   downloadCommand += " \"" + m_pContentInfo->URI + "\"";                 //URI
   downloadCommand += " true";                                           //broadcast
   
   std::string a_result;

   std::string str_error;
   try
   {
      m_pMainWindow->RunTask(downloadCommand, a_result);
   }
   catch(std::exception const& ex)
   {
      str_error = ex.what();
   }
   if (false == str_error.empty())
      ALERT("Failed to download content" + str_error);

   emit ContentWasBought();
   
}




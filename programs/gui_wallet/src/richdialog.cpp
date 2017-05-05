/*
 *
 *	File: richdialog.cpp
 *
 *	Created on: 27 Jan 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#include "gui_wallet_global.hpp"
#include <QIntValidator>
#include <QMessageBox>
#include "richdialog.hpp"
#include "gui_design.hpp"
#include "gui_wallet_mainwindow.hpp"
#include "gui_design.hpp"
#include <QKeyEvent>

using namespace gui_wallet;

RichDialogBase::RichDialogBase(QString title) : m_ok_button(this), m_cancel_button(this)
{
    m_ok_button.setText(tr("Import"));
    m_cancel_button.setText(tr("Cancel"));
    m_ok_button.setFixedSize(140, 40);
    m_cancel_button.setFixedSize(140, 40);
    m_buttons_layout.setSpacing(20);
    m_buttons_layout.addWidget(&m_ok_button);
    m_buttons_layout.addWidget(&m_cancel_button);
    m_controls_layout.setContentsMargins(0, 0, 0, 0);
    m_cancel_button.setStyleSheet(d_cancel_button);
    m_main_layout.setContentsMargins(40, 40, 40, 40);
    m_main_layout.setAlignment(Qt::AlignCenter);
    m_main_layout.setSpacing(10);
    m_main_layout.addLayout(&m_controls_layout);
    m_main_layout.addLayout(&m_buttons_layout);
    setLayout(&m_main_layout);
    connect(&m_cancel_button,SIGNAL(clicked()),this,SLOT(close()));
    connect(&m_ok_button,SIGNAL(clicked()),this,SLOT(set_ok_and_closeSlot()));
    setWindowTitle(title);
    setFixedSize(380,240);
}


void RichDialogBase::set_ok_and_closeSlot()
{
    m_ret_value = RDB_OK;
    close();
}


RET_TYPE RichDialogBase::execRB(const QPoint* a_pMove)
{
    m_ret_value = RDB_CANCEL;
    if(a_pMove){QDialog::move(*a_pMove);} QDialog::exec();
    return m_ret_value;
}


void RichDialogBase::AddLayout(QLayout* a_pLayout)
{
    m_controls_layout.addLayout(a_pLayout);
}

void RichDialogBase::AddWidget(QWidget* a_pWidget)
{
    m_controls_layout.addWidget(a_pWidget);
}


/********************************************/
RichDialog::RichDialog(int a_num_of_text_boxes  , QString title)
    : m_nNumOfTextBoxes(a_num_of_text_boxes),m_pTextBoxes(NULL),RichDialogBase(title)
{
    if(a_num_of_text_boxes<=0) return;

    m_pTextBoxes = new QLineEdit[a_num_of_text_boxes];
    connect(&m_pTextBoxes[0], SIGNAL(returnPressed()), &m_ok_button, SIGNAL(LabelClicked()));
    connect(&m_pTextBoxes[1], SIGNAL(returnPressed()), &m_ok_button, SIGNAL(LabelClicked()));
    m_pTextBoxes[0].setPlaceholderText((tr("Account")));
    m_pTextBoxes[0].setAttribute(Qt::WA_MacShowFocusRect, 0);
    m_pTextBoxes[0].setFixedSize(300, 44);
    m_pTextBoxes[0].setStyleSheet(d_text_box);

    m_pTextBoxes[1].setPlaceholderText((tr("Key")));
    m_pTextBoxes[1].setAttribute(Qt::WA_MacShowFocusRect, 0);
    m_pTextBoxes[1].setFixedSize(300, 44);
    m_pTextBoxes[1].setStyleSheet(d_text_box);
    for(int i(0); i<a_num_of_text_boxes; ++i )
    {
        m_controls_layout.addWidget(&m_pTextBoxes[i]);
    }
#ifdef _MSC_VER
    int height = style()->pixelMetric(QStyle::PM_TitleBarHeight);
    setWindowIcon(height > 32 ? QIcon(":/icon/images/windows_decent_icon_32x32.png")
       : QIcon(":/icon/images/windows_decent_icon_16x16.png"));
#endif
}

RichDialog::~RichDialog() {
    if(m_nNumOfTextBoxes>0) {
       delete [] m_pTextBoxes;
    }
}


RET_TYPE RichDialog::execRD(const QPoint* a_pMove, std::vector<std::string>& a_cvResults)
{
    QString cqsResult;
    QByteArray cbaResult;
    int i,nVectInitSize(a_cvResults.size());
    int nSizeToSet(nVectInitSize<m_nNumOfTextBoxes ? nVectInitSize : m_nNumOfTextBoxes);

    for(i = 0; i<nSizeToSet; ++i){m_pTextBoxes[i].setText(tr(a_cvResults[i].c_str()));}

    RET_TYPE rtReturn = RichDialogBase::execRB(a_pMove);
    if(m_nNumOfTextBoxes>nVectInitSize){a_cvResults.resize(m_nNumOfTextBoxes);}

    for(i = 0; i<m_nNumOfTextBoxes; ++i)
    {
        cqsResult = m_pTextBoxes[i].text();
        cbaResult = cqsResult.toLatin1();
        a_cvResults[i] = cbaResult.data();
    }

    return rtReturn;
}



//Send Dialog *********************************************************
SendDialogBase::SendDialogBase(QString title) : m_ok_button(this), m_cancel_button(this)
{
   m_ok_button.setText(tr("Send"));
   m_cancel_button.setText(tr("Cancel"));
   m_ok_button.setFixedSize(140, 40);
   m_cancel_button.setFixedSize(140, 40);
   m_buttons_layout.setSpacing(20);
   m_buttons_layout.addWidget(&m_ok_button);
   m_buttons_layout.addWidget(&m_cancel_button);
   m_controls_layout.setContentsMargins(0, 0, 0, 0);
   m_cancel_button.setStyleSheet(d_cancel_button);
   m_main_layout.setContentsMargins(40, 10, 40, 10);
   m_main_layout.setAlignment(Qt::AlignCenter);
   m_main_layout.setSpacing(10);
   m_main_layout.addLayout(&m_controls_layout);
   m_main_layout.addLayout(&m_buttons_layout);
   setLayout(&m_main_layout);
   connect(&m_cancel_button,SIGNAL(clicked()),this,SLOT(close()));
   connect(&m_ok_button,SIGNAL(clicked()),this,SLOT(set_ok_and_closeSlot()));
   setWindowTitle(title);
   setFixedSize(380,240);
}


void SendDialogBase::set_ok_and_closeSlot()
{
   m_ret_value = RDB_OK;
   emit RDB_is_OK();
}


RET_TYPE SendDialogBase::execRB(const QPoint* a_pMove)
{
   m_ret_value = RDB_CANCEL;
   if(a_pMove){QDialog::move(*a_pMove);} QDialog::exec();
   return m_ret_value;
}


void SendDialogBase::AddLayout(QLayout* a_pLayout)
{
   m_controls_layout.addLayout(a_pLayout);
}

void SendDialogBase::AddWidget(QWidget* a_pWidget)
{
   m_controls_layout.addWidget(a_pWidget);
}


/********************************************/
SendDialog::SendDialog(int a_num_of_text_boxes  , QString title, QString userName)
: m_nNumOfTextBoxes(a_num_of_text_boxes),m_pTextBoxes(NULL),SendDialogBase(title) , m_userName(userName)
{
   _locale = ((QApplication*)QApplication::instance())->inputMethod()->locale();

   if(a_num_of_text_boxes<=0) return;
   
   connect(this, SIGNAL(RDB_is_OK()), this, SLOT(sendDCT()));
   
   m_pTextBoxes = new QLineEdit[a_num_of_text_boxes];
   connect(&m_pTextBoxes[0], SIGNAL(returnPressed()), &m_ok_button, SIGNAL(LabelClicked()));
   connect(&m_pTextBoxes[1], SIGNAL(returnPressed()), &m_ok_button, SIGNAL(LabelClicked()));
   connect(&m_pTextBoxes[2], SIGNAL(returnPressed()), &m_ok_button, SIGNAL(LabelClicked()));
   
   m_pTextBoxes[0].setPlaceholderText(QString(tr("Account")));
   m_pTextBoxes[0].setAttribute(Qt::WA_MacShowFocusRect, 0);
   m_pTextBoxes[0].setFixedSize(300, 44);
   m_pTextBoxes[0].setStyleSheet(d_text_box);

   QDoubleValidator* dblValidator = new QDoubleValidator(0.0001, 100000, 4, this);
   dblValidator->setLocale(_locale);
   m_pTextBoxes[1].setValidator(dblValidator);
   
   m_pTextBoxes[1].setPlaceholderText(QString(tr("Amount")));
   m_pTextBoxes[1].setAttribute(Qt::WA_MacShowFocusRect, 0);
   m_pTextBoxes[1].setFixedSize(300, 44);
   m_pTextBoxes[1].setStyleSheet(d_text_box);
   
   m_pTextBoxes[2].setPlaceholderText(QString(tr("Memo")));
   m_pTextBoxes[2].setAttribute(Qt::WA_MacShowFocusRect, 0);
   m_pTextBoxes[2].setFixedSize(300, 44);
   m_pTextBoxes[2].setStyleSheet(d_text_box);
   for(int i(0); i<a_num_of_text_boxes; ++i )
   {
      m_controls_layout.addWidget(&m_pTextBoxes[i]);
   }
#ifdef _MSC_VER
   int height = style()->pixelMetric(QStyle::PM_TitleBarHeight);
   setWindowIcon(height > 32 ? QIcon(":/icon/images/windows_decent_icon_32x32.png")
      : QIcon(":/icon/images/windows_decent_icon_16x16.png"));
#endif
}

SendDialog::~SendDialog() {
   if(m_nNumOfTextBoxes>0) {
      delete [] m_pTextBoxes;
   }
}

void SendDialog::sendDCT()
{
   std::string a_result;
   std::string message;

   bool isOK = false;
   QString amount = QString::number(_locale.toDouble(m_pTextBoxes[1].text(), &isOK));
   if (!isOK) {
      QMessageBox* msgBox = new QMessageBox();
      msgBox->setAttribute(Qt::WA_DeleteOnClose);
      msgBox->setWindowTitle(tr("Error"));
      msgBox->setText(tr("Invalid amount is specified"));
      msgBox->open();

      return;
   }

   QString curentName = Globals::instance().getCurrentUser().c_str();
   
   try {
      QString run_str = "transfer \""
      + curentName + "\" \""
      + m_pTextBoxes[0].text() + "\" \""
      + amount
      + "\" \"DCT\" \""
      + m_pTextBoxes[2].text()
      + "\" \"true\"";
      RunTask(run_str.toStdString(), a_result);
   } catch(const std::exception& ex){
      message = ex.what();
      setEnabled(true);
   }
   
   if (message.empty())
   {
      ShowMessageBox(tr("Success") , tr("Success"));
      close();
   }
   else
   {
      ShowMessageBox(tr("Error"), tr("Failed to send DCT"), message.c_str());
   }
}


RET_TYPE SendDialog::execRD(const QPoint* a_pMove, std::vector<std::string>& a_cvResults)
{
   QString cqsResult;
   QByteArray cbaResult;
   int i,nVectInitSize(a_cvResults.size());
   int nSizeToSet(nVectInitSize<m_nNumOfTextBoxes ? nVectInitSize : m_nNumOfTextBoxes);
   
   for(i = 0; i<nSizeToSet; ++i){m_pTextBoxes[i].setText(tr(a_cvResults[i].c_str()));}
   if(m_userName != "")
      m_pTextBoxes[0].setText(m_userName);
   RET_TYPE rtReturn = SendDialogBase::execRB(a_pMove);
   if(m_nNumOfTextBoxes>nVectInitSize){a_cvResults.resize(m_nNumOfTextBoxes);}
   
   for(i = 0; i<m_nNumOfTextBoxes; ++i)
   {
      cqsResult = m_pTextBoxes[i].text();
      cbaResult = cqsResult.toLatin1();
      a_cvResults[i] = cbaResult.data();
   }
   
   return rtReturn;
}

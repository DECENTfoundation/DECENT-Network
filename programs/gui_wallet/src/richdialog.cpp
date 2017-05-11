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
#include <QDateTime>

using namespace gui_wallet;



                  //Send Dialog
/*********************************************************/
 
TransferDialog::TransferDialog(QWidget* parent, QString const& userName/* = QString()*/) : m_toUserName(userName)
{
   QVBoxLayout* mainLayout       = new QVBoxLayout();
   QVBoxLayout* lineEditsLayout  = new QVBoxLayout();
   QHBoxLayout* buttonsLayout    = new QHBoxLayout();
   
   DecentButton* ok = new DecentButton(this);
   ok->setText(tr("Send"));
   ok->setFixedSize(140, 40);
   DecentButton* cancel = new DecentButton(this);
   cancel->setText(tr("Cancel"));
   cancel->setFixedSize(140, 40);
   
   QObject::connect(ok, &QPushButton::clicked, this, &TransferDialog::Transfer);
   QObject::connect(cancel, &QPushButton::clicked, this, &QDialog::close);
   
   QLineEdit* name = new QLineEdit(this);
   QLineEdit* amount = new QLineEdit(this);
   QLineEdit* memo = new QLineEdit(this);
   
   name->setPlaceholderText(tr("Account"));
   name->setAttribute(Qt::WA_MacShowFocusRect, 0);
   name->setFixedSize(300, 44);
   name->setText(m_toUserName);
   QObject::connect(name, &QLineEdit::textChanged, this, &TransferDialog::nameChanged);
   
   amount->setPlaceholderText(tr("Amount"));
   amount->setAttribute(Qt::WA_MacShowFocusRect, 0);
   amount->setFixedSize(300, 44);
   QDoubleValidator* dblValidator = new QDoubleValidator(0.0001, 100000, 4, this);
   dblValidator->setLocale(Globals::instance().locale());
   amount->setValidator(dblValidator);
   QObject::connect(amount, &QLineEdit::textChanged, this, &TransferDialog::amountChanged);
   
   memo->setPlaceholderText(tr("Memo"));
   memo->setAttribute(Qt::WA_MacShowFocusRect, 0);
   memo->setFixedSize(300, 44);
   QObject::connect(memo, &QLineEdit::textChanged, this, &TransferDialog::memoChanged);
   
   lineEditsLayout->addWidget(name);
   lineEditsLayout->addWidget(amount);
   lineEditsLayout->addWidget(memo);
   
   buttonsLayout->setSpacing(20);
   buttonsLayout->addWidget(ok);
   buttonsLayout->addWidget(cancel);
   
   
   mainLayout->setContentsMargins(40, 10, 40, 10);
   mainLayout->addLayout(lineEditsLayout);
   mainLayout->addLayout(buttonsLayout);
   
   setLayout(mainLayout);
   
   setFixedSize(380, 220);
}

void TransferDialog::nameChanged(const QString & name)
{
   m_toUserName = name;
}

void TransferDialog::amountChanged(const QString & amount)
{
   m_amount = amount.toDouble();
}

void TransferDialog::memoChanged(const QString & memo)
{
   m_memo = memo;
}

void TransferDialog::Transfer()
{
   std::string a_result;
   std::string message;
   
   if(m_fromUserName.isEmpty())
       m_fromUserName = Globals::instance().getCurrentUser().c_str();
   
   
   QString strAssetSymbol = Globals::instance().asset(0).m_str_symbol.c_str();

   try {
      QString run_str = "transfer "
      "\"" + m_fromUserName + "\" "
      "\"" + m_toUserName + "\" "
      "\"" + QString::number(m_amount) + "\""
      "\"" + strAssetSymbol + "\" "
      "\"" + m_memo + "\" "
      "\"true\"";
      RunTask(run_str.toStdString(), a_result);
   } catch(const std::exception& ex){
      message = ex.what();
   }
   
   if (message.empty())
   {
      close();
   }
   else
   {
      ShowMessageBox(tr("Error"), tr("Failed to transfer DCT"), message.c_str());
   }
}

/*********************************************************/


                  //ImportDialog
/*********************************************************/

ImportDialog::ImportDialog(QWidget* parent)
{
   QObject::connect(this, &ImportDialog::signal_keyImported, &Globals::instance(), &Globals::slot_displayWalletContent);
   
   QVBoxLayout* mainLayout       = new QVBoxLayout();
   QVBoxLayout* lineEditsLayout  = new QVBoxLayout();
   QHBoxLayout* buttonsLayout    = new QHBoxLayout();
   
   DecentButton* ok = new DecentButton(this);
   ok->setText(tr("Ok"));
   ok->setFixedSize(140, 40);
   DecentButton* cancel = new DecentButton(this);
   cancel->setText(tr("Cancel"));
   cancel->setFixedSize(140, 40);
   
   QObject::connect(ok, &QPushButton::clicked, this, &ImportDialog::Import);
   QObject::connect(cancel, &QPushButton::clicked, this, &QDialog::close);
   
   QLineEdit* name = new QLineEdit(this);
   QLineEdit* key  = new QLineEdit(this);
   
   name->setPlaceholderText(tr("Account"));
   name->setAttribute(Qt::WA_MacShowFocusRect, 0);
   name->setFixedSize(300, 44);
   QObject::connect(name, &QLineEdit::textChanged, this, &ImportDialog::nameChanged);
   
   key->setPlaceholderText(tr("Key"));
   key->setAttribute(Qt::WA_MacShowFocusRect, 0);
   key->setFixedSize(300, 44);
   QObject::connect(key, &QLineEdit::textChanged, this, &ImportDialog::keyChanged);
   

   
   lineEditsLayout->addWidget(name);
   lineEditsLayout->addWidget(key);
   
   buttonsLayout->setSpacing(20);
   buttonsLayout->addWidget(ok);
   buttonsLayout->addWidget(cancel);
   
   
   mainLayout->setContentsMargins(40, 10, 40, 10);
   mainLayout->addLayout(lineEditsLayout);
   mainLayout->addLayout(buttonsLayout);
   
   setLayout(mainLayout);
   
   setFixedSize(380, 220);
}

void ImportDialog::nameChanged(const QString & name)
{
   m_userName = name;
}

void ImportDialog::keyChanged(const QString & key)
{
   m_key = key;
}


void ImportDialog::Import()
{
   std::string message;
   std::string result;
   try {
      QString csTaskStr = "import_key "
      "\"" + m_userName + "\" "
      "\"" + m_key + "\" ";
      RunTask(csTaskStr.toStdString(), result);
   } catch (const std::exception& ex) {
      message = ex.what();
   }
   if (message.empty()) {
      emit signal_keyImported();
      close();
   }
   else
   {
      ShowMessageBox(tr("Error"), tr("Cannot import key."), message.c_str());
   }
}

/*********************************************************/


                  //ZebraDialog
/*********************************************************/




UserInfoDialog::UserInfoDialog(QWidget* parent,
                               const QString& registrar,
                               const QString& referrer,
                               const QString& lifetime_referrer,
                               const QString& network_fee_percentage,
                               const QString& lifetime_referrer_fee_percentage,
                               const QString& referrer_rewards_percentage,
                               const QString& name,
                               const QString& id
                               )
{
   QVBoxLayout* main_layout = new QVBoxLayout();
   main_layout->setSpacing(0);
   main_layout->setContentsMargins(0, 0, 0, 0);
   
   setStyleSheet("background-color:white;");
   
   QWidget*     widgetRegistrar = new QWidget(this);
   QVBoxLayout* layoutRegistrar = new QVBoxLayout();
   layoutRegistrar->setSpacing(0);
   layoutRegistrar->setContentsMargins(45,3,0,3);
   widgetRegistrar->setStyleSheet("background-color:rgb(244,244,244);");
   QLabel* lblTitle = new QLabel(tr("Registrar"), this);
   QLabel* lblInfo = new QLabel(registrar, this);
   layoutRegistrar->addWidget(lblTitle);
   layoutRegistrar->addWidget(lblInfo);
   widgetRegistrar->setLayout(layoutRegistrar);
   main_layout->addWidget(widgetRegistrar);
   
   
   QWidget*     widgetReferrer = new QWidget(this);
   QVBoxLayout* layoutReferrer = new QVBoxLayout();
   layoutReferrer->setSpacing(0);
   layoutReferrer->setContentsMargins(45,3,0,3);
   lblTitle = new QLabel(tr("Referrer"), this);
   lblInfo = new QLabel(referrer, this);
   layoutReferrer->addWidget(lblTitle);
   layoutReferrer->addWidget(lblInfo);
   widgetReferrer->setLayout(layoutReferrer);
   main_layout->addWidget(widgetReferrer);
   
   QWidget*     widgetLifetimeReferrer = new QWidget(this);
   QVBoxLayout* layoutLifetimeReferrer = new QVBoxLayout();
   layoutLifetimeReferrer->setSpacing(0);
   layoutLifetimeReferrer->setContentsMargins(45,3,0,3);
   widgetLifetimeReferrer->setStyleSheet("background-color:rgb(244,244,244);");
   lblTitle = new QLabel(tr("Lifetime Referrer"), this);
   lblInfo = new QLabel(lifetime_referrer, this);
   layoutLifetimeReferrer->addWidget(lblTitle);
   layoutLifetimeReferrer->addWidget(lblInfo);
   widgetLifetimeReferrer->setLayout(layoutLifetimeReferrer);
   main_layout->addWidget(widgetLifetimeReferrer);
   
   QWidget*     widgetNetworkFeePercentage = new QWidget(this);
   QVBoxLayout* layoutNetworkFeePercentage = new QVBoxLayout();
   layoutNetworkFeePercentage->setSpacing(0);
   layoutNetworkFeePercentage->setContentsMargins(45,3,0,3);
   lblTitle = new QLabel(tr("Network Fee"), this);
   lblInfo = new QLabel(network_fee_percentage, this);
   layoutNetworkFeePercentage->addWidget(lblTitle);
   layoutNetworkFeePercentage->addWidget(lblInfo);
   widgetNetworkFeePercentage->setLayout(layoutNetworkFeePercentage);
   main_layout->addWidget(widgetNetworkFeePercentage);
   
   QWidget*     widgetLifetimeReferrerFeePercentage = new QWidget(this);
   QVBoxLayout* lifetimeReferrerFeePercentage = new QVBoxLayout();
   lifetimeReferrerFeePercentage->setSpacing(0);
   lifetimeReferrerFeePercentage->setContentsMargins(45,3,0,3);
   widgetLifetimeReferrerFeePercentage->setStyleSheet("background-color:rgb(244,244,244);");
   lblTitle = new QLabel(tr("Lifetime Referrer Fee"), this);
   lblInfo = new QLabel(lifetime_referrer_fee_percentage, this);
   lifetimeReferrerFeePercentage->addWidget(lblTitle);
   lifetimeReferrerFeePercentage->addWidget(lblInfo);
   widgetLifetimeReferrerFeePercentage->setLayout(lifetimeReferrerFeePercentage);
   main_layout->addWidget(widgetLifetimeReferrerFeePercentage);
   
   QWidget*     widgetReferrerRewardsPercentage = new QWidget(this);
   QVBoxLayout* layoutReferrerRewardsPercentage = new QVBoxLayout();
   layoutReferrerRewardsPercentage->setSpacing(0);
   layoutReferrerRewardsPercentage->setContentsMargins(45,3,0,3);
   lblTitle = new QLabel(tr("Referrer Rewards Percentage"), this);
   lblInfo = new QLabel(referrer_rewards_percentage, this);
   layoutReferrerRewardsPercentage->addWidget(lblTitle);
   layoutReferrerRewardsPercentage->addWidget(lblInfo);
   widgetReferrerRewardsPercentage->setLayout(layoutReferrerRewardsPercentage);
   main_layout->addWidget(widgetReferrerRewardsPercentage);
   

   setWindowTitle(name + " (" + id + ")");
   setFixedSize(620,420);
   setLayout(main_layout);
}


/*********************************************************/

       //BuyDialog
/*********************************************************/

BuyDialog::BuyDialog(QWidget* parent, const SDigitalContent& a_cnt_details, bool bSilent)
{
   QVBoxLayout* main_layout = new QVBoxLayout();
   main_layout->setSpacing(0);
   main_layout->setContentsMargins(0, 0, 0, 0);
   
   setStyleSheet("background-color:white;");
   
   
                     //Author
               /***********************/
   QWidget*     WidgetRegistrar = new QWidget(this);
   QHBoxLayout* LayoutRegistrar = new QHBoxLayout();
   LayoutRegistrar->setSpacing(0);
   LayoutRegistrar->setContentsMargins(45,3,0,3);
   WidgetRegistrar->setStyleSheet("background-color:rgb(244,244,244);");
   QLabel* lblTitle = new QLabel("Author", this);
   QLabel* lblInfo = new QLabel(QString::fromStdString(a_cnt_details.author), this);
   LayoutRegistrar->addWidget(lblTitle);
   LayoutRegistrar->addWidget(lblInfo);
   WidgetRegistrar->setLayout(LayoutRegistrar);
   main_layout->addWidget(WidgetRegistrar);
               /***********************/
   
                     //Expiration
               /***********************/
   QWidget*     WidgetReferrer = new QWidget(this);
   QHBoxLayout* LayoutReferrer = new QHBoxLayout();
   LayoutReferrer->setSpacing(0);
   LayoutReferrer->setContentsMargins(45,3,0,3);
   lblTitle = new QLabel("Expiration", this);
   QDateTime time = QDateTime::fromString(QString::fromStdString(a_cnt_details.expiration), "yyyy-MM-ddTHH:mm:ss");
   std::string e_str = CalculateRemainingTime(QDateTime::currentDateTime(), time);
   lblInfo = new QLabel(QString::fromStdString(e_str), this);
   LayoutReferrer->addWidget(lblTitle);
   LayoutReferrer->addWidget(lblInfo);
   WidgetReferrer->setLayout(LayoutReferrer);
   main_layout->addWidget(WidgetReferrer);
               /***********************/
   
   
                           //Uploaded
                  /***********************/
   QWidget*     WidgetLifetimeReferrer = new QWidget(this);
   QHBoxLayout* LayoutLifetimeReferrer = new QHBoxLayout();
   LayoutLifetimeReferrer->setSpacing(0);
   LayoutLifetimeReferrer->setContentsMargins(45,3,0,3);
   WidgetLifetimeReferrer->setStyleSheet("background-color:rgb(244,244,244);");
   lblTitle = new QLabel("Uploaded", this);
   lblInfo = new QLabel(QString::fromStdString(a_cnt_details.created), this);
   LayoutLifetimeReferrer->addWidget(lblTitle);
   LayoutLifetimeReferrer->addWidget(lblInfo);
   WidgetLifetimeReferrer->setLayout(LayoutLifetimeReferrer);
   main_layout->addWidget(WidgetLifetimeReferrer);
                  /***********************/
   
   
                           //Amount
                  /***********************/
   QWidget*     WidgetNetworkFeePercentage = new QWidget(this);
   QHBoxLayout* LayoutNetworkFeePercentage = new QHBoxLayout();
   LayoutNetworkFeePercentage->setSpacing(0);
   LayoutNetworkFeePercentage->setContentsMargins(45,3,0,3);
   lblTitle = new QLabel("Amount", this);
   QString str_price = a_cnt_details.price.getString().c_str();
   lblInfo = new QLabel(str_price, this);
   LayoutNetworkFeePercentage->addWidget(lblTitle);
   LayoutNetworkFeePercentage->addWidget(lblInfo);
   WidgetNetworkFeePercentage->setLayout(LayoutNetworkFeePercentage);
   main_layout->addWidget(WidgetNetworkFeePercentage);
                  /***********************/
   
   
   
                              //Average Rating
                        /***********************/
   QWidget*     WidgetLifetimeReferrerFeePercentage = new QWidget(this);
   QHBoxLayout* LifetimeReferrerFeePercentage = new QHBoxLayout();
   LifetimeReferrerFeePercentage->setSpacing(0);
   LifetimeReferrerFeePercentage->setContentsMargins(45,3,0,3);
   WidgetLifetimeReferrerFeePercentage->setStyleSheet("background-color:rgb(244,244,244);");
   lblTitle = new QLabel("Average Rating", this);
   
   QPixmap green_star(green_star_image);
   QPixmap white_star(white_star_image);
   
   white_star = white_star.scaled(QSize(20,20));
   green_star = green_star.scaled(QSize(20,20));
   
   lblInfo = new QLabel(QString::number(a_cnt_details.AVG_rating), this);
   QLabel m_stars[5];
   for(int i = 0; i < a_cnt_details.AVG_rating; ++i) {
      m_stars[i].setPixmap(green_star);
   }
   
   for(int i = a_cnt_details.AVG_rating; i < 5; ++i) {
      m_stars[i].setPixmap(white_star);
   }

   LifetimeReferrerFeePercentage->addWidget(lblTitle);
   LifetimeReferrerFeePercentage->addWidget(lblInfo);
   WidgetLifetimeReferrerFeePercentage->setLayout(LifetimeReferrerFeePercentage);
   main_layout->addWidget(WidgetLifetimeReferrerFeePercentage);
                     /***********************/
   
   
   
                              //Size
                     /***********************/
//   QWidget*     WidgetReferrerRewardsPercentage = new QWidget(this);
//   QHBoxLayout* LayoutReferrerRewardsPercentage = new QHBoxLayout();
//   LayoutReferrerRewardsPercentage->setSpacing(0);
//   LayoutReferrerRewardsPercentage->setContentsMargins(45,3,0,3);
//   lblTitle = new QLabel("Size", this);
//   lblInfo = new QLabel(referrer_rewards_percentage, this);
//   LayoutReferrerRewardsPercentage->addWidget(lblTitle);
//   LayoutReferrerRewardsPercentage->addWidget(lblInfo);
//   WidgetReferrerRewardsPercentage->setLayout(LayoutReferrerRewardsPercentage);
//   main_layout->addWidget(WidgetReferrerRewardsPercentage);
//                  /***********************/
//   
//   
//                           //Size
//                  /***********************/
//   QWidget*     WidgetReferrerRewardsPercentage = new QWidget(this);
//   QHBoxLayout* LayoutReferrerRewardsPercentage = new QHBoxLayout();
//   LayoutReferrerRewardsPercentage->setSpacing(0);
//   LayoutReferrerRewardsPercentage->setContentsMargins(45,3,0,3);
//   WidgetLifetimeReferrerFeePercentage->setStyleSheet("background-color:rgb(244,244,244);");
//   lblTitle = new QLabel("Times Bought", this);
//   lblInfo = new QLabel(referrer_rewards_percentage, this);
//   LayoutReferrerRewardsPercentage->addWidget(lblTitle);
//   LayoutReferrerRewardsPercentage->addWidget(lblInfo);
//   WidgetReferrerRewardsPercentage->setLayout(LayoutReferrerRewardsPercentage);
//   main_layout->addWidget(WidgetReferrerRewardsPercentage);
                  /***********************/
   
   setLayout(main_layout);

}


/*********************************************************/





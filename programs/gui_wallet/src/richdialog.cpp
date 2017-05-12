#include "gui_wallet_global.hpp"
#include "richdialog.hpp"


#include "decent_button.hpp"
#include "decent_label.hpp"
#include "gui_wallet_mainwindow.hpp"
#include "gui_design.hpp"


#include <QIntValidator>
#include <QMessageBox>
#include <QKeyEvent>
#include <QDateTime>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLocale>
#include <QInputMethod>
#include <QApplication>
#include <vector>
#include <string>


namespace gui_wallet
{
//
// TransferDialog
//
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
//
// ImportKeyDialog
//
ImportKeyDialog::ImportKeyDialog(QWidget* parent)
{
   QObject::connect(this, &ImportKeyDialog::signal_keyImported, &Globals::instance(), &Globals::slot_displayWalletContent);
   
   QVBoxLayout* mainLayout       = new QVBoxLayout();
   QVBoxLayout* lineEditsLayout  = new QVBoxLayout();
   QHBoxLayout* buttonsLayout    = new QHBoxLayout();
   
   DecentButton* ok = new DecentButton(this);
   ok->setText(tr("Ok"));
   ok->setFixedSize(140, 40);
   DecentButton* cancel = new DecentButton(this);
   cancel->setText(tr("Cancel"));
   cancel->setFixedSize(140, 40);
   
   QObject::connect(ok, &QPushButton::clicked, this, &ImportKeyDialog::Import);
   QObject::connect(cancel, &QPushButton::clicked, this, &QDialog::close);
   
   QLineEdit* name = new QLineEdit(this);
   QLineEdit* key  = new QLineEdit(this);
   
   name->setPlaceholderText(tr("Account"));
   name->setAttribute(Qt::WA_MacShowFocusRect, 0);
   name->setFixedSize(300, 44);
   QObject::connect(name, &QLineEdit::textChanged, this, &ImportKeyDialog::nameChanged);
   
   key->setPlaceholderText(tr("Key"));
   key->setAttribute(Qt::WA_MacShowFocusRect, 0);
   key->setFixedSize(300, 44);
   QObject::connect(key, &QLineEdit::textChanged, this, &ImportKeyDialog::keyChanged);
   

   
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

void ImportKeyDialog::nameChanged(const QString & name)
{
   m_userName = name;
}

void ImportKeyDialog::keyChanged(const QString & key)
{
   m_key = key;
}

void ImportKeyDialog::Import()
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
//
// UserInfoDialog
//
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
   
   DecentLabel* registrarLabel = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::Highlighted);
   registrarLabel->setText(tr("Registrar\n") + registrar);
   main_layout->addWidget(registrarLabel);

   DecentLabel* referrerLabel = new DecentLabel(this, DecentLabel::RowLabel);
   referrerLabel->setText(tr("Referrer\n") + registrar);
   main_layout->addWidget(referrerLabel);
   
   DecentLabel* lifetimeReferrerLabel = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::Highlighted);
   lifetimeReferrerLabel->setText((tr("Lifetime Referrer\n") + lifetime_referrer));
   main_layout->addWidget(lifetimeReferrerLabel);
   
   DecentLabel* networkFeeLabel = new DecentLabel(this, DecentLabel::RowLabel);
   networkFeeLabel->setText((tr("Network Fee Percentage\n") + network_fee_percentage));
   main_layout->addWidget(networkFeeLabel);
   
   DecentLabel* lifetimeReferrerFeeLabel = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::Highlighted);
   lifetimeReferrerFeeLabel->setText((tr("Lifetime Referrer Fee Percentage\n") + lifetime_referrer_fee_percentage));
   main_layout->addWidget(lifetimeReferrerFeeLabel);
   
   DecentLabel* referrerRewardsPercentageLabel = new DecentLabel(this, DecentLabel::RowLabel);
   referrerRewardsPercentageLabel->setText((tr("Referrer Rewards Percentage\n") + referrer_rewards_percentage));
   main_layout->addWidget(referrerRewardsPercentageLabel);

   setWindowTitle(name + " (" + id + ")");
   setFixedSize(300, 300);
   setLayout(main_layout);
}
//
// BuyDialog
//
BuyDialog::BuyDialog(QWidget* parent, const SDigitalContent& a_cnt_details, bool bSilent)
{
   QVBoxLayout* main_layout = new QVBoxLayout();
   main_layout->setSpacing(0);
   main_layout->setContentsMargins(0, 0, 0, 0);
   
   setStyleSheet("background-color:white;");
   
   // Author
   //
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

   // Expiration
   //
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

   // Uploaded
   //
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

   // Amount
   //
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

   // Average Rating
   //
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

   // Size
   //
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

}  // end namespace gui_wallet


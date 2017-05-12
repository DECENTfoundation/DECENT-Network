#include "gui_wallet_global.hpp"
#include "richdialog.hpp"


#include "decent_button.hpp"
#include "decent_label.hpp"
#include "gui_wallet_mainwindow.hpp"
#include "gui_design.hpp"
#include "decent_text_edit.hpp"
#include <graphene/chain/content_object.hpp>


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
// RatingWidget
//
RatingWidget::RatingWidget(QWidget* pParent)
   : QWidget(pParent)
   , m_bAutomation(false)
{
   QHBoxLayout* pMainLayout = new QHBoxLayout();


   m_arr_p_rate.resize(size);
   for (int index = 0; index < size; ++index)
   {
      m_arr_p_rate[index] = new DecentButton(this, DecentButton::StarIcon);
      m_arr_p_rate[index]->setCheckable(true);

      pMainLayout->addWidget(m_arr_p_rate[index]);

      QObject::connect(m_arr_p_rate[index], &QPushButton::toggled,
                       this, &RatingWidget::slot_rating);
   }

   pMainLayout->setSpacing(0);
   pMainLayout->setContentsMargins(0, 0, 0, 0);

   setLayout(pMainLayout);
}

void RatingWidget::setRating(int rating)
{
   for (int index = 0; index < size; ++index)
   {
      if (index < rating)
         m_arr_p_rate[index]->setChecked(true);
      else
         m_arr_p_rate[index]->setChecked(false);
   }

   emit rated(rating);
}

void RatingWidget::slot_rating()
{
   if (m_bAutomation)
      return;

   m_bAutomation = true;
   for (int index = 0; index < size; ++index)
   {
      if (m_arr_p_rate[index] == sender())
         setRating(index + 1);
   }
   m_bAutomation = false;
}
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
BuyDialog::BuyDialog(QWidget* parent, const SDigitalContent& a_cnt_details)
{
   QGridLayout* main_layout = new QGridLayout();
   main_layout->setSpacing(0);
   main_layout->setContentsMargins(0, 0, 0, 0);

   int iRowIndex = 0;
   // Author
   //
   DecentLabel* labelAuthorTitle = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::Highlighted);
   DecentLabel* labelAuthorInfo = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::HighlightedRight);
   labelAuthorTitle->setText(tr("Author"));
   labelAuthorInfo->setText(QString::fromStdString(a_cnt_details.author));
   main_layout->addWidget(labelAuthorTitle, iRowIndex, 0);
   main_layout->addWidget(labelAuthorInfo, iRowIndex, 1);
   ++iRowIndex;

   // Expiration
   //
   DecentLabel* labelExpirationTitle = new DecentLabel(this, DecentLabel::RowLabel);
   DecentLabel* labelExpirationInfo = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::Right);
   labelExpirationTitle->setFixedSize(250, 50);
   labelExpirationInfo->setFixedSize(250, 50);
   QDateTime time = QDateTime::fromString(QString::fromStdString(a_cnt_details.expiration), "yyyy-MM-ddTHH:mm:ss");
   std::string e_str = CalculateRemainingTime(QDateTime::currentDateTime(), time);
   labelExpirationTitle->setText(tr("Expiration"));
   labelExpirationInfo->setText(QString::fromStdString(e_str));
   main_layout->addWidget(labelExpirationTitle, 1, 0, 1, 1);
   main_layout->addWidget(labelExpirationInfo, 1, 1, 1, 1);
   main_layout->itemAtPosition(1,1)->setAlignment(Qt::AlignRight);
   main_layout->itemAtPosition(1,0)->setAlignment(Qt::AlignLeft);

   // Uploaded
   //
   DecentLabel* labelUploadedTitle = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::Highlighted);
   DecentLabel* labelUploadedInfo = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::Highlighted);
   labelUploadedTitle->setFixedSize(250, 50);
   labelUploadedInfo->setFixedSize(250, 50);
   labelUploadedTitle->setText(tr("Uploaded"));
   labelUploadedInfo->setText(QString::fromStdString(a_cnt_details.created));
   main_layout->addWidget(labelUploadedTitle, 2, 0, 1, 1);
   main_layout->addWidget(labelUploadedInfo, 2, 1, 1, 1);
   main_layout->itemAtPosition(2,1)->setAlignment(Qt::AlignRight);
   main_layout->itemAtPosition(2,0)->setAlignment(Qt::AlignLeft);


   // Average Rating
   //
   DecentLabel* labelAverageRatingTitle = new DecentLabel(this, DecentLabel::RowLabel);
   RatingWidget* averageRatingInfo = new RatingWidget(this);
   labelAverageRatingTitle->setFixedSize(400, 50);
   averageRatingInfo->setFixedSize(100, 50);
   averageRatingInfo->setRating(a_cnt_details.AVG_rating);
   labelAverageRatingTitle->setText(tr("Average Rating"));
   main_layout->addWidget(labelAverageRatingTitle, 3, 0, 1, 1);
   main_layout->addWidget(averageRatingInfo, 3, 1, 1, 1);
   main_layout->itemAtPosition(3,1)->setAlignment(Qt::AlignRight);
   main_layout->itemAtPosition(3,0)->setAlignment(Qt::AlignLeft);
   
   // Amount
   //
   DecentLabel* labelAmountTitle = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::Highlighted);
   DecentLabel* labelAmountInfo  = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::Highlighted);
   labelAmountTitle->setFixedSize(250, 50);
   labelAmountInfo->setFixedSize(250, 50);
   QString str_price = a_cnt_details.price.getString().c_str();
   labelAmountTitle->setText(tr("Amount"));
   labelAmountInfo->setText(str_price);
   main_layout->addWidget(labelAmountTitle, 4, 0, 1, 1);
   main_layout->addWidget(labelAmountInfo, 4, 1, 1, 1);
   main_layout->itemAtPosition(4,1)->setAlignment(Qt::AlignRight);
   main_layout->itemAtPosition(4,0)->setAlignment(Qt::AlignLeft);

   // Size
   //
   DecentLabel* labelSizeTitle = new DecentLabel(this, DecentLabel::RowLabel);
   DecentLabel* labelSizeInfo = new DecentLabel(this, DecentLabel::RowLabel);
   labelSizeTitle->setFixedSize(250, 50);
   labelSizeInfo->setFixedSize(250, 50);
   labelSizeTitle->setText(tr("Size"));
   labelSizeInfo->setText(QString::number(a_cnt_details.size) + " MB");
   main_layout->addWidget(labelSizeTitle, 5, 0, 1, 1);
   main_layout->addWidget(labelSizeInfo, 5, 1, 1, 1);
   main_layout->itemAtPosition(5,1)->setAlignment(Qt::AlignRight);
   main_layout->itemAtPosition(5,0)->setAlignment(Qt::AlignLeft);
 
   
   // Times Bought
   //
   DecentLabel* labelTimesBoughtTitle = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::Highlighted);
   DecentLabel* labelTimesBoughtInfo = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::Highlighted);
   labelTimesBoughtTitle->setFixedSize(250, 50);
   labelTimesBoughtInfo->setFixedSize(250, 50);
   labelTimesBoughtTitle->setText(tr("Times Bought"));
   labelTimesBoughtInfo->setText(QString::number(a_cnt_details.times_bought));
   main_layout->addWidget(labelTimesBoughtTitle, 6, 0, 1, 1);
   main_layout->addWidget(labelTimesBoughtInfo, 6, 1, 1, 1);
   main_layout->itemAtPosition(6,1)->setAlignment(Qt::AlignRight);
   main_layout->itemAtPosition(6,0)->setAlignment(Qt::AlignLeft);
   
   DecentTextEdit* description = new DecentTextEdit(this, DecentTextEdit::Info);
   description->setText(tr("Description") + "\n\n");
   description->setReadOnly(true);
   description->setFont(DescriptionDetailsFont());

   
   std::string synopsis = a_cnt_details.synopsis;
   std::string title;
   std::string desc;
   
   graphene::chain::ContentObjectPropertyManager synopsis_parser(synopsis);
   title = synopsis_parser.get<graphene::chain::ContentObjectTitle>();
   desc = synopsis_parser.get<graphene::chain::ContentObjectDescription>();
   
   this->setWindowTitle(QString::fromStdString(title));
   description->setText(description->toPlainText() + QString::fromStdString(desc) + "\n");
   
   //main_layout->addWidget(description, 7, 0, -1, 1);
   
   setFixedSize(500, 500);
   setLayout(main_layout);

}

}  // end namespace gui_wallet


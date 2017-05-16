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
#include <QTextEdit>
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
void PlaceInsideLabel(QWidget* pParent, QWidget* pChild)
{
   pParent->show();
   QMargins margins = pParent->contentsMargins();

   QHBoxLayout* pMainLayout = new QHBoxLayout;
   pMainLayout->addWidget(pChild);

   pMainLayout->setSizeConstraint(QLayout::SetFixedSize);
   pMainLayout->setSpacing(0);
   pMainLayout->setContentsMargins(0, 0, 0, 0);
   pParent->setLayout(pMainLayout);
}
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
   pMainLayout->setSizeConstraint(QLayout::SetFixedSize);

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
   setLayout(main_layout);
}
//
// ContentInfoDialog
//
ContentInfoDialog::ContentInfoDialog(QWidget* parent, const SDigitalContent& a_cnt_details)
   : m_URI(a_cnt_details.URI)
   , m_amount(a_cnt_details.price.getString().c_str())
   , m_getItOrPay(GetIt)
{
   QGridLayout* main_layout = new QGridLayout();
   main_layout->setSpacing(0);
   main_layout->setContentsMargins(0, 0, 0, 15);

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
   QDateTime time = QDateTime::fromString(QString::fromStdString(a_cnt_details.expiration), "yyyy-MM-ddTHH:mm:ss");
   std::string e_str = CalculateRemainingTime(QDateTime::currentDateTime(), time);
   labelExpirationTitle->setText(tr("Expiration"));
   labelExpirationInfo->setText(QString::fromStdString(e_str));
   main_layout->addWidget(labelExpirationTitle, iRowIndex, 0);
   main_layout->addWidget(labelExpirationInfo, iRowIndex, 1);
   ++iRowIndex;

   // Uploaded
   //
   DecentLabel* labelUploadedTitle = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::Highlighted);
   DecentLabel* labelUploadedInfo = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::HighlightedRight);
   labelUploadedTitle->setText(tr("Uploaded"));
   labelUploadedInfo->setText(QString::fromStdString(a_cnt_details.created));
   main_layout->addWidget(labelUploadedTitle, iRowIndex, 0);
   main_layout->addWidget(labelUploadedInfo, iRowIndex, 1);
   ++iRowIndex;

   // Average Rating
   //
   DecentLabel* labelAverageRatingTitle = new DecentLabel(this, DecentLabel::RowLabel);
   DecentLabel* labelAverageRatingInfoWrapper = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::Right);
   RatingWidget* averageRatingInfo = new RatingWidget(this);
   PlaceInsideLabel(labelAverageRatingInfoWrapper, averageRatingInfo);
   averageRatingInfo->setRating(a_cnt_details.AVG_rating);
   averageRatingInfo->setEnabled(false);
   labelAverageRatingTitle->setText(tr("Average Rating"));
   main_layout->addWidget(labelAverageRatingTitle, iRowIndex, 0);
   main_layout->addWidget(labelAverageRatingInfoWrapper, iRowIndex, 1, Qt::AlignRight);
   ++iRowIndex;

   // Amount
   //
   DecentLabel* labelAmountTitle = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::Highlighted);
   DecentLabel* labelAmountInfo  = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::HighlightedRight);
   QString str_price = a_cnt_details.price.getString().c_str();
   labelAmountTitle->setText(tr("Amount"));
   labelAmountInfo->setText(str_price);
   main_layout->addWidget(labelAmountTitle, iRowIndex, 0);
   main_layout->addWidget(labelAmountInfo, iRowIndex, 1);
   ++iRowIndex;

   // Size
   //
   DecentLabel* labelSizeTitle = new DecentLabel(this, DecentLabel::RowLabel);
   DecentLabel* labelSizeInfo = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::Right);
   labelSizeTitle->setText(tr("Size"));
   labelSizeInfo->setText(QString::number(a_cnt_details.size) + " MB");
   main_layout->addWidget(labelSizeTitle, iRowIndex, 0);
   main_layout->addWidget(labelSizeInfo, iRowIndex, 1);
   ++iRowIndex;
   
   // Times Bought
   //
   DecentLabel* labelTimesBoughtTitle = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::Highlighted);
   DecentLabel* labelTimesBoughtInfo = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::HighlightedRight);
   labelTimesBoughtTitle->setText(tr("Times Bought"));
   labelTimesBoughtInfo->setText(QString::number(a_cnt_details.times_bought));
   main_layout->addWidget(labelTimesBoughtTitle, iRowIndex, 0);
   main_layout->addWidget(labelTimesBoughtInfo, iRowIndex, 1);
   ++iRowIndex;
   
   DecentTextEdit* description = new DecentTextEdit(this, DecentTextEdit::Info);
   description->setFixedSize(500, 200);
   description->setReadOnly(true);
   description->setFont(DescriptionDetailsFont());
   
   std::string synopsis = a_cnt_details.synopsis;
   std::string title;
   std::string desc;
   
   graphene::chain::ContentObjectPropertyManager synopsis_parser(synopsis);
   title = synopsis_parser.get<graphene::chain::ContentObjectTitle>();
   desc = synopsis_parser.get<graphene::chain::ContentObjectDescription>();
   
   setWindowTitle(QString::fromStdString(title));
   description->setText(QString::fromStdString(desc));
   
   main_layout->addWidget(description, iRowIndex, 0, 1, 2);
   ++iRowIndex;
   
   DecentButton* getItButton = new DecentButton(this, DecentButton::DialogAction);
   DecentButton* cancelButton = new DecentButton(this, DecentButton::DialogCancel);
   getItButton->setText(tr("Get it!"));
   cancelButton->setText(tr("Close"));
   
   connect(getItButton, &QPushButton::clicked, this, &ContentInfoDialog::ButtonWasClicked);
   connect(cancelButton, &QPushButton::clicked, this, &QDialog::close);
   
   main_layout->addWidget(getItButton, iRowIndex, 0, Qt::AlignRight);
   main_layout->addWidget(cancelButton, iRowIndex, 1, Qt::AlignLeft);
   
   setLayout(main_layout);
}
   
void ContentInfoDialog::ButtonWasClicked()
{
   QPushButton* button = (QPushButton *)sender();
   if(m_getItOrPay == GetIt)
   {
      m_getItOrPay = Pay;
      button->setText((tr("Pay") + " " + m_amount));
   }
   else
   {
      m_getItOrPay = GetIt;
      button->setText(tr("Get It!"));
      LabelPushCallbackGUI();
   }
}
   
void ContentInfoDialog::LabelPushCallbackGUI()
{
   std::string downloadCommand = "download_content";
   downloadCommand += " " + Globals::instance().getCurrentUser();       // consumer
   downloadCommand += " \"" + m_URI + "\"";                             // URI
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
      ShowMessageBox("", tr("Failed to download content"), QString::fromStdString(str_error));
   
   
   emit ContentWasBought();
   close();
}

//
// PurchasedDialog
//
ContentReviewDialog::ContentReviewDialog(QWidget* parent, const SDigitalContent& a_cnt_details)
: m_URI(a_cnt_details.URI)
{
   QGridLayout* main_layout = new QGridLayout();
   main_layout->setSpacing(0);
   main_layout->setContentsMargins(0, 0, 0, 15);
   
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
   
   // Created
   //
   DecentLabel* labelExpirationTitle = new DecentLabel(this, DecentLabel::RowLabel);
   DecentLabel* labelExpirationInfo = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::Right);
   QDateTime time = QDateTime::fromString(QString::fromStdString(a_cnt_details.created), "yyyy-MM-ddTHH:mm:ss");
   std::string e_str = CalculateRemainingTime(QDateTime::currentDateTime(), time);
   labelExpirationTitle->setText(tr("Created"));
   labelExpirationInfo->setText(QString::fromStdString(e_str));
   main_layout->addWidget(labelExpirationTitle, iRowIndex, 0);
   main_layout->addWidget(labelExpirationInfo, iRowIndex, 1);
   ++iRowIndex;
   
   // Amount
   //
   DecentLabel* labelAmountTitle = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::Highlighted);
   DecentLabel* labelAmountInfo  = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::HighlightedRight);
   QString str_price = a_cnt_details.price.getString().c_str();
   labelAmountTitle->setText(tr("Amount"));
   labelAmountInfo->setText(str_price);
   main_layout->addWidget(labelAmountTitle, iRowIndex, 0);
   main_layout->addWidget(labelAmountInfo, iRowIndex, 1);
   ++iRowIndex;
   
   // Average Rating
   //
   DecentLabel* labelAverageRatingTitle = new DecentLabel(this, DecentLabel::RowLabel);
   DecentLabel* labelAverageRatingInfoWrapper = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::Right);
   RatingWidget* averageRatingInfo = new RatingWidget(this);
   PlaceInsideLabel(labelAverageRatingInfoWrapper, averageRatingInfo);
   averageRatingInfo->setRating(a_cnt_details.AVG_rating);
   averageRatingInfo->setEnabled(false);
   labelAverageRatingTitle->setText(tr("Average Rating"));
   main_layout->addWidget(labelAverageRatingTitle, iRowIndex, 0);
   main_layout->addWidget(labelAverageRatingInfoWrapper, iRowIndex, 1, Qt::AlignRight);
   ++iRowIndex;
   
   // Size
   //
   DecentLabel* labelSizeTitle = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::Highlighted);
   DecentLabel* labelSizeInfo = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::HighlightedRight);
   labelSizeTitle->setText(tr("Size"));
   labelSizeInfo->setText(QString::number(a_cnt_details.size) + " MB");
   main_layout->addWidget(labelSizeTitle, iRowIndex, 0);
   main_layout->addWidget(labelSizeInfo, iRowIndex, 1);
   ++iRowIndex;
   
   // Times Bought
   //
   DecentLabel* labelTimesBoughtTitle = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::Highlighted);
   DecentLabel* labelTimesBoughtInfo = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::HighlightedRight);
   labelTimesBoughtTitle->setText(tr("Times Bought"));
   labelTimesBoughtInfo->setText(QString::number(a_cnt_details.times_bought));
   main_layout->addWidget(labelTimesBoughtTitle, iRowIndex, 0);
   main_layout->addWidget(labelTimesBoughtInfo, iRowIndex, 1);
   ++iRowIndex;
   
   DecentTextEdit* description = new DecentTextEdit(this, DecentTextEdit::Info);
   description->setFixedSize(500, 200);
   description->setReadOnly(true);
   description->setFont(DescriptionDetailsFont());
   
   std::string synopsis = a_cnt_details.synopsis;
   std::string title;
   std::string desc;
   
   graphene::chain::ContentObjectPropertyManager synopsis_parser(synopsis);
   title = synopsis_parser.get<graphene::chain::ContentObjectTitle>();
   desc = synopsis_parser.get<graphene::chain::ContentObjectDescription>();
   
   setWindowTitle(QString::fromStdString(title));
   description->setText(QString::fromStdString(desc));
   
   main_layout->addWidget(description, iRowIndex, 0, 1, 2);
   ++iRowIndex;
   
   

   setLayout(main_layout);
}
//
// PurchasedDialog
//
NextPreviousWidget::NextPreviousWidget() :
  m_next_button(new DecentButton(this, DecentButton::TableIcon, DecentButton::Transaction))
, m_previous_button(new DecentButton(this, DecentButton::TableIcon, DecentButton::Transfer))
{
   QVBoxLayout* main_layout = new QVBoxLayout();
   QObject::connect(m_next_button, &QPushButton::clicked, this, &NextPreviousWidget::next);
   QObject::connect(m_previous_button, &QPushButton::clicked, this, &NextPreviousWidget::previous);
   
   main_layout->addWidget(m_next_button);
   main_layout->addWidget(m_previous_button);
   setLayout(main_layout);
}
   
void NextPreviousWidget::reset()
{
   m_next_button->setEnabled(true);
   m_previous_button->setEnabled(true);
}
   
void NextPreviousWidget::first()
{
   m_next_button->setEnabled(true);
   m_previous_button->setEnabled(false);
}
   
void NextPreviousWidget::last()
{
   m_next_button->setEnabled(false);
   m_previous_button->setEnabled(true);
}
   

//
//CommentWidget
//
CommentWidget::CommentWidget(QWidget* parent,
                             const std::string& content_info,
                             const QString& strUser /*= QString()*/):
QWidget(parent),
m_content_uri(content_info),
m_user(strUser.toStdString())
{
   enum mode { list_feedback, leave_feedback, view_feedback};
   mode eMode;
   nlohmann::json feedback;

   if (strUser.isEmpty())
   {
      // will show all comments with pagination
      eMode = list_feedback;
   }
   else
   {
      try
      {
         feedback = Globals::instance().runTaskParse("search_feedback "
                                                     "\"" + m_user + "\" "
                                                     "\"" + m_content_uri + "\" "
                                                     "\"" + next_iterator()   + "\" " +
                                                     "2" ) ;
      }catch(...){}
      
      if (feedback.empty())
      {
         // will get feedback input
         eMode = leave_feedback;
      }
      else
      {
         // will show the feedback this user has left
         eMode = view_feedback;
      }
   }

   m_pLabelUserName = new DecentLabel(this);
   m_pRatingWidget = new RatingWidget(this);
   m_pComment = new QTextEdit(this);
   
   NextPreviousWidget* pNextPreviousWidget = nullptr;
   
   if (eMode == list_feedback)
   {
      pNextPreviousWidget = new NextPreviousWidget();
      
      m_pLabelUserName->setText(strUser);
      m_pRatingWidget->setRating(feedback[0]["rating"].get<int>());
      m_pRatingWidget->setEnabled(false);
   }
   
   DecentButton* pLeaveFeedbackButton = nullptr;
   
   if (eMode == leave_feedback)
   {
      pLeaveFeedbackButton = new DecentButton(this);
      
      m_pComment->setPlaceholderText("Comment here...");
      pLeaveFeedbackButton->setText(tr("Leave Feedback"));
      m_pLabelUserName->setText(strUser);
   }
   
   if (eMode == view_feedback )
   {
      m_pComment->setText( QString::fromStdString(feedback[0]["comment"].get<std::string>()) );
      
      m_pLabelUserName->setText(QString::fromStdString(m_user));
      m_pRatingWidget->setRating(feedback[0]["rating"].get<int>());
      m_pRatingWidget->setEnabled(false);
      
      m_pComment->setReadOnly(true);
   }
   
   // MainLayout
   QVBoxLayout* pMainLayout = new QVBoxLayout();
   QHBoxLayout* pRatingLayout = new QHBoxLayout();
   
   pRatingLayout->addWidget(m_pLabelUserName);
   pRatingLayout->addWidget(m_pRatingWidget);
   
   pMainLayout->addLayout(pRatingLayout);
   pMainLayout->addWidget(m_pComment);
   
   if (pLeaveFeedbackButton)
   {
      pMainLayout->addWidget(pLeaveFeedbackButton);
      QObject::connect(pLeaveFeedbackButton, &DecentButton::clicked,
                       this, &CommentWidget::submit);
      QObject::connect(m_pRatingWidget, &RatingWidget::rated,
                       this, &CommentWidget::getRating);
   }
   
   if (pNextPreviousWidget)
   {
      pMainLayout->addWidget(pNextPreviousWidget);
      QObject::connect(pNextPreviousWidget, &NextPreviousWidget::next,
                       this, &CommentWidget::nextButtonSlot);
      QObject::connect(pNextPreviousWidget, &NextPreviousWidget::previous,
                       this, &CommentWidget::previousButtonSlot);
      
      QObject::connect(this, &CommentWidget::signal_lastComment,
                       pNextPreviousWidget, &NextPreviousWidget::last);
      QObject::connect(this, &CommentWidget::signal_firstComment,
                       pNextPreviousWidget, &NextPreviousWidget::first);
   }
   
}

//Leave content feedback
void CommentWidget::submit()
{
   if( m_rating == 0 || m_pComment->toPlainText().isEmpty() )
   {
      return;
   }
   
   try
   {
      Globals::instance().runTaskParse("leave_rating_and_comment "
                                       "\"" + m_user + "\" "
                                       "\"" + m_content_uri + "\" "
                                       "\"" + std::to_string(m_rating) + "\" "
                                       "\"" + escape_string( m_pComment->toPlainText().toStdString() ) + "\" "
                                       "true" );
   }catch(...){}
}
   
// update contentInfo
void CommentWidget::update()
{
   nlohmann::json feedback;
   try
   {
      feedback = Globals::instance().runTaskParse("search_feedback "
                                                  "\"" /* empty user  */ "\" "
                                                  "\"" + m_content_uri + "\" "
                                                  "\"" + next_iterator()   + "\" " +
                                                  "2" ) ;
   }catch(...){}

   if ( feedback.size() > 1)
   {
      set_next_comment(feedback[1]["id"].get<std::string>());
   }
   else
   {
      set_next_comment(std::string());
   }
   
   m_pLabelUserName->setText(QString::fromStdString(feedback[0]["user"].get<std::string>()));
   m_pComment->setText(QString::fromStdString(feedback[0]["comment"].get<std::string>()));
   m_pRatingWidget->setRating(feedback[0]["rating"].get<int>());
}

void CommentWidget::getRating(const int& rated)
{
   m_rating = rated;
}

bool CommentWidget::is_first() const
{
   return m_iterators.empty();
}

bool CommentWidget::is_last() const
{
   return m_next_itr.empty();
}

void CommentWidget::set_next_comment(std::string const& last)
{
   m_next_itr = last;
}

std::string CommentWidget::next_iterator()
{
   if( !m_iterators.empty() )
   {
      return m_iterators.back();
   }
   
   return std::string();
}

void CommentWidget::controller()
{
   emit signal_SetNextPageDisabled(is_last());
   emit signal_SetPreviousPageDisabled(is_first());
}

bool CommentWidget::nextButtonSlot()
{
   //next();
   if( is_last() )
   {
      emit signal_lastComment();
      return false;
   }
   
   m_iterators.push_back(m_next_itr);
   update();
   controller();
   
   return true;
}

bool CommentWidget::previousButtonSlot()
{
   //previous();
   if( is_first() )
   {
      emit signal_firstComment();
      return false;
   }
   
   m_iterators.pop_back();
   update();
   controller();
   
   return true;
}

void CommentWidget::resetButtonSlot()
{
   //reset();
   m_iterators.clear();
   m_next_itr.clear();
   update();
   controller();
}

CommentWidget::~CommentWidget()
{
   
}

}  // end namespace gui_wallet

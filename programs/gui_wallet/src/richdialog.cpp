#include "gui_wallet_global.hpp"
#include "richdialog.hpp"


#include "decent_button.hpp"
#include "decent_label.hpp"
#include "decent_line_edit.hpp"
#include "decent_text_edit.hpp"
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
   if (m_bAutomation)
      return;

   m_bAutomation = true;
   for (int index = 0; index < size; ++index)
   {
      if (index < rating)
         m_arr_p_rate[index]->setChecked(true);
      else
         m_arr_p_rate[index]->setChecked(false);
   }
   m_bAutomation = false;

   m_rating = rating;
   emit rated(rating);
}

void RatingWidget::slot_rating()
{
   for (int index = 0; index < size; ++index)
   {
      if (m_arr_p_rate[index] == sender())
         setRating(index + 1);
   }
}
//
// TransferDialog
//
TransferDialog::TransferDialog(QWidget* parent, QString const& userName/* = QString()*/)
   : QDialog(parent)
   , m_toUserName(userName)
{
   QVBoxLayout* mainLayout       = new QVBoxLayout();
   QVBoxLayout* lineEditsLayout  = new QVBoxLayout();
   QHBoxLayout* buttonsLayout    = new QHBoxLayout();
   
   DecentButton* ok = new DecentButton(this, DecentButton::DialogAction);
   ok->setText(tr("Send"));

   DecentButton* cancel = new DecentButton(this, DecentButton::DialogCancel);
   cancel->setText(tr("Cancel"));
   
   QObject::connect(ok, &QPushButton::clicked, this, &TransferDialog::Transfer);
   QObject::connect(cancel, &QPushButton::clicked, this, &QDialog::close);
   
   DecentLineEdit* name = new DecentLineEdit(this, DecentLineEdit::DialogLineEdit);
   DecentLineEdit* amount = new DecentLineEdit(this, DecentLineEdit::DialogLineEdit);
   DecentLineEdit* memo = new DecentLineEdit(this, DecentLineEdit::DialogLineEdit);
   
   name->setPlaceholderText(tr("Account"));
   name->setAttribute(Qt::WA_MacShowFocusRect, 0);
   name->setText(m_toUserName);
   QObject::connect(name, &QLineEdit::textChanged, this, &TransferDialog::nameChanged);
   
   amount->setPlaceholderText(tr("Amount"));
   amount->setAttribute(Qt::WA_MacShowFocusRect, 0);

   QDoubleValidator* dblValidator = new QDoubleValidator(0.0001, 100000, 4, this);
   dblValidator->setLocale(Globals::instance().locale());
   amount->setValidator(dblValidator);
   QObject::connect(amount, &QLineEdit::textChanged, this, &TransferDialog::amountChanged);
   
   memo->setPlaceholderText(tr("Memo"));
   memo->setAttribute(Qt::WA_MacShowFocusRect, 0);
   QObject::connect(memo, &QLineEdit::textChanged, this, &TransferDialog::memoChanged);
   
   lineEditsLayout->addWidget(name);
   lineEditsLayout->addWidget(amount);
   lineEditsLayout->addWidget(memo);
   
//   buttonsLayout->setSpacing(20);
   buttonsLayout->addWidget(ok);
   buttonsLayout->addWidget(cancel);
   
   
   mainLayout->setContentsMargins(40, 10, 40, 10);
   mainLayout->addLayout(lineEditsLayout);
   mainLayout->addLayout(buttonsLayout);
   
   setLayout(mainLayout);
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
: QDialog(parent)
{
   QObject::connect(this, &QDialog::accepted,
                    &Globals::instance(), &Globals::signal_keyImported);
   
   QVBoxLayout* mainLayout       = new QVBoxLayout();
   QVBoxLayout* lineEditsLayout  = new QVBoxLayout();
   QHBoxLayout* buttonsLayout    = new QHBoxLayout();
   
   DecentButton* ok = new DecentButton(this, DecentButton::DialogAction);
   ok->setText(tr("Ok"));
//   ok->setFixedSize(140, 40);
   DecentButton* cancel = new DecentButton(this, DecentButton::DialogCancel);
   cancel->setText(tr("Cancel"));
//   cancel->setFixedSize(140, 40);
   
   QObject::connect(ok, &QPushButton::clicked,
                    this, &ImportKeyDialog::Import);
   QObject::connect(cancel, &QPushButton::clicked,
                    this, &QDialog::close);
   
   DecentLineEdit* name = new DecentLineEdit(this, DecentLineEdit::DialogLineEdit);
   DecentLineEdit* key  = new DecentLineEdit(this, DecentLineEdit::DialogLineEdit);
   
   name->setPlaceholderText(tr("Account"));
   name->setAttribute(Qt::WA_MacShowFocusRect, 0);
//   name->setFixedSize(300, 44);
   QObject::connect(name, &QLineEdit::textChanged,
                    this, &ImportKeyDialog::nameChanged);
   
   key->setPlaceholderText(tr("Key"));
   key->setAttribute(Qt::WA_MacShowFocusRect, 0);
//   key->setFixedSize(300, 44);
   QObject::connect(key, &QLineEdit::textChanged,
                    this, &ImportKeyDialog::keyChanged);

   lineEditsLayout->addWidget(name);
   lineEditsLayout->addWidget(key);
   
   buttonsLayout->setSpacing(20);
   buttonsLayout->addWidget(ok);
   buttonsLayout->addWidget(cancel);
   
   
   mainLayout->setContentsMargins(40, 10, 40, 10);
   mainLayout->addLayout(lineEditsLayout);
   mainLayout->addLayout(buttonsLayout);
   
   setLayout(mainLayout);
   
//   setFixedSize(380, 220);
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
   try
   {
      QString csTaskStr = "import_key "
      "\"" + m_userName + "\" "
      "\"" + m_key + "\" ";
      RunTask(csTaskStr.toStdString(), result);
   }
   catch (const std::exception& ex)
   {
      message = ex.what();
   }

   if (message.empty())
   {
      emit accepted();
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
   : QDialog(parent)
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
   : QDialog(parent)
   , m_URI(a_cnt_details.URI)
   , m_amount(a_cnt_details.price.getString().c_str())
   , m_getItOrPay(GetIt)
{
   QGridLayout* main_layout = new QGridLayout();
   main_layout->setSpacing(0);
   main_layout->setContentsMargins(0, 0, 0, 15);

   int iRowIndex = 0;
   // Author
   //
   DecentLabel* labelAuthorTitle = new DecentLabel(this, DecentLabel::RowLabel);
   DecentLabel* labelAuthorInfo = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::Right);
   labelAuthorTitle->setText(tr("Author"));
   labelAuthorInfo->setText(QString::fromStdString(a_cnt_details.author));
   main_layout->addWidget(labelAuthorTitle, iRowIndex, 0);
   main_layout->addWidget(labelAuthorInfo, iRowIndex, 1);
   ++iRowIndex;

   // Expiration
   //
   DecentLabel* labelExpirationTitle = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::Highlighted);
   DecentLabel* labelExpirationInfo = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::HighlightedRight);
   QDateTime time = QDateTime::fromString(QString::fromStdString(a_cnt_details.expiration), "yyyy-MM-ddTHH:mm:ss");
   std::string e_str = CalculateRemainingTime(QDateTime::currentDateTime(), time);
   labelExpirationTitle->setText(tr("Expiration"));
   labelExpirationInfo->setText(QString::fromStdString(e_str));
   main_layout->addWidget(labelExpirationTitle, iRowIndex, 0);
   main_layout->addWidget(labelExpirationInfo, iRowIndex, 1);
   ++iRowIndex;

   // Uploaded
   //
   DecentLabel* labelUploadedTitle = new DecentLabel(this, DecentLabel::RowLabel);
   DecentLabel* labelUploadedInfo = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::Right);
   labelUploadedTitle->setText(tr("Uploaded"));
   labelUploadedInfo->setText(QString::fromStdString(a_cnt_details.created));
   main_layout->addWidget(labelUploadedTitle, iRowIndex, 0);
   main_layout->addWidget(labelUploadedInfo, iRowIndex, 1);
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
   
   std::string synopsis = a_cnt_details.synopsis;
   std::string title;
   std::string desc;
   
   graphene::chain::ContentObjectPropertyManager synopsis_parser(synopsis);
   title = synopsis_parser.get<graphene::chain::ContentObjectTitle>();
   desc = synopsis_parser.get<graphene::chain::ContentObjectDescription>();

   CommentWidget* pCommentWidget = new CommentWidget(this,
                                                     a_cnt_details.AVG_rating,
                                                     a_cnt_details.author,
                                                     a_cnt_details.URI,
                                                     desc);

   main_layout->addWidget(pCommentWidget, iRowIndex, 0, 1, 2);
   pCommentWidget->update();
   ++iRowIndex;
   
   DecentButton* getItButton = new DecentButton(this, DecentButton::DialogAction);
   DecentButton* cancelButton = new DecentButton(this, DecentButton::DialogCancel);
   getItButton->setText(tr("Get it!"));
   cancelButton->setText(tr("Close"));
   
   QObject::connect(getItButton, &QPushButton::clicked,
                    this, &ContentInfoDialog::ButtonWasClicked);
   QObject::connect(cancelButton, &QPushButton::clicked,
                    this, &QDialog::close);
   
   main_layout->addWidget(getItButton, iRowIndex, 0, Qt::AlignRight);
   main_layout->addWidget(cancelButton, iRowIndex, 1, Qt::AlignLeft);
   
   setLayout(main_layout);

   setWindowTitle(QString::fromStdString(title));
}
   
void ContentInfoDialog::ButtonWasClicked()
{
   QPushButton* button = (QPushButton*) sender();
   if (m_amount == "Free" ||
       m_getItOrPay == Pay)
   {
      m_getItOrPay = GetIt;
      button->setText(tr("Get It!"));
      Buy();
   }
   else
   {
      m_getItOrPay = Pay;
      button->setText((tr("Pay") + " " + m_amount));
   }
}
   
void ContentInfoDialog::Buy()
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
: QDialog(parent)
, m_URI(a_cnt_details.URI)
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
   
   // Purchased
   //
   DecentLabel* labelExpirationTitle = new DecentLabel(this, DecentLabel::RowLabel);
   DecentLabel* labelExpirationInfo = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::Right);
   labelExpirationTitle->setText(tr("Purchased"));
   labelExpirationInfo->setText(a_cnt_details.purchased_time.c_str());
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

   std::string synopsis = a_cnt_details.synopsis;
   std::string title;
   std::string desc;
   
   graphene::chain::ContentObjectPropertyManager synopsis_parser(synopsis);
   title = synopsis_parser.get<graphene::chain::ContentObjectTitle>();
   desc = synopsis_parser.get<graphene::chain::ContentObjectDescription>();


   CommentWidget* pCommentWidget = new CommentWidget(this,
                                                     a_cnt_details.AVG_rating,
                                                     a_cnt_details.author,
                                                     a_cnt_details.URI,
                                                     desc,
                                                     Globals::instance().getCurrentUser());
   main_layout->addWidget(pCommentWidget, iRowIndex, 0, 1, 2);
   pCommentWidget->update();
   ++iRowIndex;

   setLayout(main_layout);

   setWindowTitle(QString::fromStdString(title));
}
//
//CommentWidget
//
CommentWidget::CommentWidget(QWidget* pParent,
                             uint32_t content_average_rating,
                             const std::string& content_author,
                             const std::string& content_uri,
                             const std::string& content_description,
                             const std::string& feedback_author/* = std::string()*/)
: QWidget(pParent)
, m_pComment(new DecentTextEdit(this, DecentTextEdit::Info))
, m_pLabel(new DecentLabel(this, DecentLabel::RowLabel))
, m_pRatingWidget(new RatingWidget(this))
, m_pPreviousButton(new DecentButton(this, DecentButton::DialogTextButton))
, m_pNextButton(new DecentButton(this, DecentButton::DialogTextButton))
, m_pLeaveFeedbackButton(new DecentButton(this, DecentButton::DialogTextButton))
, m_content_average_rating(content_average_rating)
, m_content_author(content_author)
, m_content_uri(content_uri)
, m_content_description(content_description)
, m_feedback_author(feedback_author)
{
   m_pLeaveFeedbackButton->setText(tr("submit"));

   QGridLayout* pMainLayout = new QGridLayout;

   int iRowIndex = 0;

   DecentLabel* labelRatingInfoWrapper = new DecentLabel(this, DecentLabel::RowLabel, DecentLabel::Right);
   PlaceInsideLabel(labelRatingInfoWrapper, m_pRatingWidget);

   pMainLayout->addWidget(m_pLabel, iRowIndex, 0);
   pMainLayout->addWidget(labelRatingInfoWrapper, iRowIndex, 1);
   ++iRowIndex;

   pMainLayout->addWidget(m_pComment, iRowIndex, 0, 1, 2);
   ++iRowIndex;

   QHBoxLayout* pButtonsLayout = new QHBoxLayout;
   pButtonsLayout->addWidget(m_pPreviousButton);
   pButtonsLayout->addWidget(m_pNextButton);
   pButtonsLayout->addWidget(m_pLeaveFeedbackButton);
   pButtonsLayout->setSizeConstraint(QLayout::SetFixedSize);
   pButtonsLayout->setSpacing(0);
   pButtonsLayout->setContentsMargins(0, 0, 0, 0);

   pMainLayout->addLayout(pButtonsLayout, iRowIndex, 0, 1, 2, Qt::AlignCenter);
   ++iRowIndex;


   QObject::connect(m_pNextButton, &QPushButton::clicked,
                    this, &CommentWidget::slot_Next);
   QObject::connect(m_pPreviousButton, &QPushButton::clicked,
                    this, &CommentWidget::slot_Previous);
   QObject::connect(m_pLeaveFeedbackButton, &DecentButton::clicked,
                    this, &CommentWidget::submit);

   //pMainLayout->setSizeConstraint(QLayout::SetFixedSize);
   pMainLayout->setSpacing(0);
   pMainLayout->setContentsMargins(0, 0, 0, 0);

   setLayout(pMainLayout);
}

//Leave content feedback
void CommentWidget::submit()
{
   if (m_pRatingWidget->m_rating == 0)
      return;
   
   try
   {
      Globals::instance().runTaskParse("leave_rating_and_comment "
                                       "\"" + Globals::instance().getCurrentUser() + "\" "
                                       "\"" + m_content_uri + "\" "
                                       "\"" + std::to_string(m_pRatingWidget->m_rating) + "\" "
                                       "\"" + escape_string(m_pComment->toPlainText().toStdString() ) + "\" "
                                       "true" );
   }catch(...){}

   slot_Previous();
   slot_Next();
}
   
// update contentInfo
void CommentWidget::update()
{
   enum mode {view_description, leave_feedback, view_my_feedback, view_others_feedback};
   mode eMode;

   QString const cstrDescription = tr("description");

   QString strFeedbackAuthor;
   QString strFeedbackComment;
   uint32_t feedback_rating;

   std::string const c_str_hack = "hack_to_leave_comment";

   nlohmann::json feedback;
   try
   {
      std::string str_next_iterator = next_iterator();

      if (str_next_iterator != c_str_hack)
      {
         feedback = Globals::instance().runTaskParse("search_feedback "
                                                     "\"" + m_feedback_author + "\" "
                                                     "\"" + m_content_uri + "\" "
                                                     "\"" + str_next_iterator   + "\" " +
                                                     "2") ;

         if (feedback.size() > 0)
         {
            strFeedbackAuthor = feedback[0]["author"].get<std::string>().c_str();
            strFeedbackComment = feedback[0]["comment"].get<std::string>().c_str();
            feedback_rating = feedback[0]["rating"].get<int>();
         }
      }
   }catch(...){}

   int test_count = 0;

   if (m_feedback_author.empty())
   {
      if (next_iterator().empty())
         eMode = view_description;
      else
      {
         eMode = view_others_feedback;
         test_count = 1;
      }
   }
   else
   {
      if (next_iterator() == c_str_hack)
         eMode = leave_feedback;
      else if (next_iterator().empty())
      {
         eMode = view_description;

         if (feedback.empty())
            test_count = -1;
      }
      else
      {
         eMode = view_my_feedback;
         test_count = 1;
      }
   }

   if (-1 == test_count)
      set_next_comment(c_str_hack);
   else if (feedback.size() > test_count)
      set_next_comment(feedback[test_count]["id"].get<std::string>());
   else
      set_next_comment(std::string());

   m_pNextButton->setDisabled(is_last());
   m_pPreviousButton->setDisabled(is_first());

   if (is_last() && is_first())
   {
      m_pNextButton->hide();
      m_pPreviousButton->hide();
   }
   else
   {
      m_pNextButton->show();
      m_pPreviousButton->show();
   }

   m_pNextButton->setText(tr("next"));
   m_pPreviousButton->setText(tr("previous"));

   if (eMode == view_description)
   {
      m_pLabel->setText(tr("Average Rating"));
      m_pComment->setText(m_content_description.c_str());
      m_pComment->setPlaceholderText(QString());
      m_pRatingWidget->setRating(m_content_average_rating);

      if (false == m_feedback_author.empty())
      {
         m_pNextButton->setText(tr("feedback"));
         m_pPreviousButton->hide();
      }

      m_pLeaveFeedbackButton->hide();
      m_pRatingWidget->setDisabled(true);
      m_pComment->setReadOnly(true);
   }
   else if (eMode == view_my_feedback ||
            eMode == view_others_feedback)
   {
      if (eMode == view_my_feedback)
      {
         m_pLabel->setText(tr("Your own feedback"));
         m_pNextButton->hide();
         m_pPreviousButton->setText(cstrDescription);
      }
      else
         m_pLabel->setText(strFeedbackAuthor);

      m_pComment->setText(strFeedbackComment);
      m_pComment->setPlaceholderText(QString());
      m_pRatingWidget->setRating(feedback_rating);

      m_pLeaveFeedbackButton->hide();
      m_pRatingWidget->setDisabled(true);
      m_pComment->setReadOnly(true);
   }
   else
   {
      m_pNextButton->hide();
      m_pPreviousButton->setText(cstrDescription);
      m_pLabel->setText(tr("Leave your feedback here"));
      m_pComment->setText(QString());
      m_pComment->setPlaceholderText(tr("Any thoughts?"));
      m_pRatingWidget->setRating(0);

      m_pLeaveFeedbackButton->show();
      m_pRatingWidget->setEnabled(true);
      m_pComment->setReadOnly(false);
      m_pNextButton->hide();
   }
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
   if (!m_iterators.empty())
   {
      return m_iterators.back();
   }
   
   return std::string();
}

bool CommentWidget::slot_Next()
{
   if(is_last())
      return false;

   m_iterators.push_back(m_next_itr);
   update();
   
   return true;
}

bool CommentWidget::slot_Previous()
{
   if (is_first())
      return false;

   m_iterators.pop_back();
   update();
   
   return true;
}
// PasswordDialog
//
PasswordDialog::PasswordDialog(QWidget* pParent, eType enType)
: QDialog(pParent, Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint)
, m_enType(enType)
, m_pError(new QLabel(this))
{
   QLabel* pLabel = new QLabel(this);
   pLabel->setText(tr("The password must be limited to 50 characters"));
   DecentButton* pButton = new DecentButton(this, DecentButton::DialogAction);

   m_pError->hide();

   if (enType == eSetPassword)
      pButton->setText(tr("Set Password"));
   else
      pButton->setText(tr("Unlock"));

   DecentLineEdit* pEditPassword = new DecentLineEdit(this, DecentLineEdit::DialogLineEdit);
   pEditPassword->setEchoMode(QLineEdit::Password);
   pEditPassword->setAttribute(Qt::WA_MacShowFocusRect, 0);
   pEditPassword->setPlaceholderText(QString(tr("Password")));
   pEditPassword->setMaxLength(50);

   if (enType == eSetPassword)
      setWindowTitle(tr("Set Password"));
   else
   {
      pLabel->hide();
      setWindowTitle(tr("Unlock your wallet"));
   }

   int iRowIndex = 0;
   QGridLayout* pMainLayout = new QGridLayout;
   pMainLayout->addWidget(pLabel, iRowIndex++, 0, Qt::AlignCenter | Qt::AlignVCenter);
   pMainLayout->addWidget(m_pError, iRowIndex++, 0, Qt::AlignCenter | Qt::AlignVCenter);
   pMainLayout->addWidget(pEditPassword, iRowIndex++, 0, Qt::AlignCenter | Qt::AlignVCenter);
   pMainLayout->addWidget(pButton, iRowIndex++, 0, Qt::AlignCenter | Qt::AlignVCenter);

#ifndef WINDOWS_HIGH_DPI
   pMainLayout->setSizeConstraint(QLayout::SetFixedSize);
#else
   setFixedWidth(600);
#endif

   pMainLayout->setSpacing(20);
   pMainLayout->setContentsMargins(20, 20, 20, 20);

   QObject::connect(pEditPassword, &QLineEdit::returnPressed,
                    this, &PasswordDialog::slot_action);
   QObject::connect(pButton, &QPushButton::clicked,
                    this, &PasswordDialog::slot_action);
   QObject::connect(pEditPassword, &QLineEdit::textChanged,
                    this, &PasswordDialog::slot_set_password);

   setLayout(pMainLayout);

   //QTimer::singleShot(0, &password_box, SLOT(setFocus()));
#ifdef _MSC_VER
   int height = style()->pixelMetric(QStyle::PM_TitleBarHeight);
   setWindowIcon(height > 32 ? QIcon(":/icon/images/windows_decent_icon_32x32.png")
                 : QIcon(":/icon/images/windows_decent_icon_16x16.png"));
#endif
}

void PasswordDialog::slot_set_password(QString const& strPassword)
{
   m_strPassword = strPassword;
}

void PasswordDialog::slot_action()
{
   if (m_strPassword.isEmpty())
      return;

   if (m_enType == eSetPassword)
   {
      try
      {
         Globals::instance().runTask("set_password \"" + m_strPassword.toStdString() + "\"");
      }
      catch(...)
      {
         m_pError->setText(tr("Cannot set this password"));
         m_pError->show();
         return;
      }
   }

   try
   {
      Globals::instance().runTask("unlock \"" + m_strPassword.toStdString() + "\"");
   }
   catch(...)
   {
      m_pError->setText(tr("Cannot unlock the wallet"));
      m_pError->show();
      return;
   }

   emit accepted();
   close();
}


}  // end namespace gui_wallet

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


ZebraDialog::ZebraDialog(QWidget* parent,
                         QString registrar,
                         QString referrer,
                         QString lifetime_referrer,
                         QString network_fee_percentage,
                         QString lifetime_referrer_fee_percentage,
                         QString referrer_rewards_percentage
                         )
{
//   QVBoxLayout* main_layout = new QVBoxLayout();
//   main_layout->setSpacing(0);
//   main_layout->setContentsMargins(0, 0, 0, 0);
//   
//   setStyleSheet("background-color:white;");
//   setLayout(main_layout);
}


/*********************************************************/


/* /////////////Comment Widget////////////////*/
CommentWidget::CommentWidget(QWidget* parent, const SDigitalContent* content_info)
:  QWidget(parent),
m_comment_count(1),
m_content_uri(content_info->URI)
{
   setVisible(false);
   update_run_task();
}

void CommentWidget::update_run_task()
{
   nlohmann::json comment;
   try
   {
      comment = Globals::instance().runTaskParse("search_feedback "
                                                 "\"" /*    empty user    */"\" "
                                                 "\"" + m_content_uri + "\" "
                                                 "\"" + next_iterator()   + "\" " +
                                                 std::to_string(m_comment_count + 1) );
   }catch(...){}
   
   QVBoxLayout* main_lay    = new QVBoxLayout();
   
   if(comment.empty())
   {
      QHBoxLayout* empty_comment_lay = new QHBoxLayout;
      QLabel*      empty_text        = new QLabel;
      
      empty_text->setText(tr("Not comment(s) on this content"));
      empty_comment_lay->addWidget(empty_text);
      empty_comment_lay->setAlignment(Qt::AlignCenter);
      main_lay->addLayout(empty_comment_lay);
      setLayout(main_lay);
      
      return;
   }
   
   if(comment.size() > m_comment_count){
      set_next_comment(comment[m_comment_count]["id"].get<std::string>());
   }else{
      set_next_comment(std::string());
   }
   
   QHBoxLayout* comment_lay = new QHBoxLayout();
   QHBoxLayout* buttons_lay = new QHBoxLayout();
   
   DecentButton* next     = new DecentButton(this);
   DecentButton* previous = new DecentButton(this);
   DecentButton* reset    = new DecentButton(this);
   
   next->setText(tr("Next"));
   previous->setText(tr("Previous"));
   reset->setText(tr("First"));
   
   auto result = comment[0];
   
   QLabel* result_user     = new QLabel;
   QLabel* result_comment  = new QLabel;
   QLabel* result_rating   = new QLabel;
   result_user->setText( QString::fromStdString(result["author"].get<std::string>()) );
   //result_comment->setText( QString::fromStdString(result["comment"].get<std::string>()) );
   result_rating->setText( QString::number( result["rating"].get<int>()) );
   
   comment_lay->addWidget(result_user);
   comment_lay->addWidget(result_comment);
   comment_lay->addWidget(result_rating);
   
   buttons_lay->addWidget(previous);
   buttons_lay->addWidget(next);
   buttons_lay->addWidget(reset);
   buttons_lay->setAlignment(Qt::AlignBottom);
   
   connect(this, &CommentWidget::signal_SetNextPageDisabled, next, &QPushButton::setDisabled);
   connect(this, &CommentWidget::signal_SetPreviousPageDisabled, previous, &QPushButton::setDisabled);
   
   connect(next, SIGNAL(clicked()), this, SLOT(nextButtonSlot()));
   connect(previous, SIGNAL(clicked()), this, SLOT(previousButtonSlot()));
   connect(reset, SIGNAL(clicked()), this, SLOT(resetButtonSlot()));
   
   controller();
   
   main_lay->addLayout(comment_lay);
   main_lay->addLayout(buttons_lay);
   main_lay->setContentsMargins(40, 0, 40, 5);
   //main_lay->setAlignment(Qt::AlignCenter);
   
   setLayout(main_lay);
}

bool CommentWidget::next()
{
   if( is_last() )
   {
      return false;
   }
   m_iterators.push_back(m_next_itr);
   update_run_task();
   return true;
}

bool CommentWidget::previous()
{
   if( is_last() )
   {
      return false;
   }
   
   m_iterators.pop_back();
   update_run_task();
   return true;
}

void CommentWidget::reset()
{
   m_iterators.clear();
   m_next_itr.clear();
   update_run_task();
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

void CommentWidget::nextButtonSlot()
{
   next();
   controller();
}

void CommentWidget::previousButtonSlot()
{
   previous();
   controller();
}

void CommentWidget::resetButtonSlot()
{
   reset();
   controller();
}

CommentWidget::~CommentWidget()
{
   
}


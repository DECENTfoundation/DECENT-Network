/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once

#ifndef _MSC_VER
#include <QDialog>
#include <QVector>
#endif

class QTextEdit;
class QCloseEvent;

namespace gui_wallet
{
struct SDigitalContent;
class DecentButton;
class DecentLabel;
class DecentTextEdit;
//
void PlaceInsideLabel(QWidget* pLabel, QWidget* pWidget);
//
// RatingWidget
//
class RatingWidget : public QWidget
{
   Q_OBJECT
public:
   RatingWidget(QWidget* pParent);
   
public slots:
   void setRating(int);
signals:
   void rated(int);
protected slots:
   void slot_rating();
protected:
   enum { size = 5 };
   
   bool m_bAutomation;
   QVector<QPushButton*> m_arr_p_rate;

public:
   uint32_t m_rating;
};
//
class StackLayerWidget: public QWidget
{
   Q_OBJECT
public:
   StackLayerWidget(QWidget* pParent);

signals:
   void accepted();
   void closed();
};
//
// TransferWidget
//
class TransferWidget : public StackLayerWidget
{
   Q_OBJECT
public:
   TransferWidget(QWidget* parent, QString const& userName = QString());
   
public slots:
   void nameChanged(const QString &);
   void amountChanged(const QString &);
   void memoChanged(const QString &);
   void Transfer();
private:
   QString  m_toUserName;
   double   m_amount;
   QString  m_memo;
   QString  m_fromUserName;
};
//
// ImportKeyWidget
//
class ImportKeyWidget : public StackLayerWidget
{
   Q_OBJECT
public:
   ImportKeyWidget(QWidget* parent);
   
public slots:
   void nameChanged(const QString &);
   void keyChanged(const QString &);
   void Import();

private:
   QString  m_userName;
   QString  m_key;
};
//
// UserInfoWidget
//
class UserInfoWidget : public StackLayerWidget
{
   Q_OBJECT
public:
   UserInfoWidget(QWidget* parent,
                  const bool&    is_publishing_manager,
                  const bool is_publishing_rights_received,
                  const QString& registrar,
                  const QString& name,
                  const QString& id
                  );
   
};
//
// ContentInfoWidget
//
class ContentInfoWidget : public StackLayerWidget
{
   Q_OBJECT
public:
   ContentInfoWidget(QWidget* parent, const SDigitalContent& a_cnt_details);
   
   void Buy();
public slots:
   void ButtonWasClicked();
   
private:
   enum GetItOrPay {GetIt, Pay};
   GetItOrPay m_getItOrPay;
   std::string m_URI;
   QString m_amount;
};
//
// ContentReviewWidget
//
class ContentReviewWidget : public StackLayerWidget
{
   Q_OBJECT
public:
   ContentReviewWidget(QWidget* parent, const SDigitalContent& a_cnt_details);
private:
   std::string m_URI;
};
//
// CommentWidget
//
class CommentWidget : public QWidget
{
   Q_OBJECT
public:
   CommentWidget(QWidget* pParent,
                 uint32_t content_average_rating,
                 const std::string& content_author,
                 const std::string& content_uri,
                 const std::string& content_description,
                 const std::string& feedback_author = std::string());

   void update();
   void submit();

   bool is_last() const;
   bool is_first() const;

   void set_next_comment(std::string const&);
   std::string next_iterator();

signals:
   void signal_lastComment();
   void signal_firstComment();

public slots:
   bool slot_Next();
   bool slot_Previous();

private:
   DecentTextEdit* m_pComment;
   DecentLabel* m_pLabel;
   RatingWidget* m_pRatingWidget;

   DecentButton* m_pPreviousButton;
   DecentButton* m_pNextButton;
   DecentButton* m_pLeaveFeedbackButton;

   uint32_t m_content_average_rating;
   std::string m_content_author;
   std::string m_content_uri;
   std::string m_content_description;
   std::string m_feedback_author;

   std::string                m_next_itr;
   std::vector<std::string>   m_iterators;
};
//
// PasswordWidget
//

class DecentLineEdit;

class PasswordWidget : public StackLayerWidget
{
   Q_OBJECT
public:
   enum eType { eSetPassword, eUnlock };
   PasswordWidget(QWidget* pParent, eType enType);

protected slots:
   void slot_action();
   void slot_textChanged(const QString& );

private:
   eType m_enType;
   QLabel* m_pError;

   DecentLineEdit* m_line1Edit;
   DecentLineEdit* m_line2Edit;
   DecentButton* m_pButton;
};

}


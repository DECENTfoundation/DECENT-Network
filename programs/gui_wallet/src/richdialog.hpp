#pragma once

#include <QDialog>
#include <QVector>

namespace gui_wallet
{
struct SDigitalContent;
class DecentButton;
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
   QVector<DecentButton*> m_arr_p_rate;
};
    
//
// TransferDialog
//
class TransferDialog : public QDialog
{
   Q_OBJECT
public:
   TransferDialog(QWidget* parent, QString const& userName = QString());
   
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
// ImportKeyDialog
//
class ImportKeyDialog : public QDialog
{
   Q_OBJECT
public:
   ImportKeyDialog(QWidget* parent);
   
   public slots:
   void nameChanged(const QString &);
   void keyChanged(const QString &);
   void Import();
public:
signals:
   void signal_keyImported();
private:
   QString  m_userName;
   QString  m_key;
};
   
//
// UserInfoDialog
//
class UserInfoDialog : public QDialog
{
   Q_OBJECT
public:
   UserInfoDialog(QWidget* parent,
               const QString& registrar,
               const QString& referrer,
               const QString& lifetime_referrer,
               const QString& network_fee_percentage,
               const QString& lifetime_referrer_fee_percentage,
               const QString& referrer_rewards_percentage,
               const QString& name,
               const QString& id
               );

};
   
//
// CommentWidget
//
class CommentWidget : public QWidget
{
   Q_OBJECT
public:
   CommentWidget(QWidget*,
                 const SDigitalContent*,
                 const QString& strUser = QString());
   ~CommentWidget();
   
protected:
   //
   // next, previous - list
   // one time from constructor - view
   //
   void update();
   void submit(uint8_t, QString const& strComment);   // leave feedback from user
   
   bool        is_last() const;
   bool        is_first() const;
   void        set_next_comment(std::string const&);
   void        controller();
   std::string next_iterator();
   
   
   
public:
   void get_content_feedback();  //ContentInfo
   void leave_content_feedback();//Pourchased
   
signals:
   void signal_SetNextPageDisabled(bool);
   void signal_SetPreviousPageDisabled(bool);
   void signal_textChanged(QString const&);
   
public slots:
   bool nextButtonSlot();
   bool previousButtonSlot();
   void resetButtonSlot();
   
private:
   std::string                m_last_result;
   std::string                m_next_itr;
   std::vector<std::string>   m_iterators;
   std::string                m_content_uri;
   std::string                m_user;
   QString                    m_strComment;
   uint32_t                   m_rating;
};

   
//
// ContentInfoDialog
//
class ContentInfoDialog : public QDialog
{
   Q_OBJECT
public:
   ContentInfoDialog(QWidget* parent, const SDigitalContent& a_cnt_details);
   
   void LabelPushCallbackGUI();
   CommentWidget* m_commentWidget;
public slots:
   void ButtonWasClicked();
public:
signals:
   void ContentWasBought();
   
private:
   enum GetItOrPay {GetIt, Pay};
   GetItOrPay m_getItOrPay;
   std::string m_URI;
   QString m_amount;
};

}


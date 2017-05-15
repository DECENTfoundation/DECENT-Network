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
// ContentInfoDialog
//
class ContentInfoDialog : public QDialog
{
   Q_OBJECT
public:
   ContentInfoDialog(QWidget* parent, const SDigitalContent& a_cnt_details);
   
   void LabelPushCallbackGUI();
   
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

//
// ContentReviewDialog
//
class ContentReviewDialog : public QDialog
{
   Q_OBJECT
public:
   ContentReviewDialog(QWidget* parent, const SDigitalContent& a_cnt_details);
private:
   std::string m_URI;
};
   
class NextPreviousWidget : public QWidget
{
   Q_OBJECT
public:
   NextPreviousWidget();

signals:
   void next();
   void previous();
public slots:
   void reset();
   void first();
   void last();
private:
   DecentButton* m_next_button;
   DecentButton* m_previous_button;
};
}


#pragma once

#include <QDialog>

namespace gui_wallet
{
struct SDigitalContent;
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
// BuyDialog
//
class BuyDialog : public QDialog
{
   Q_OBJECT
public:
   BuyDialog(QWidget* parent, const SDigitalContent& a_cnt_details, bool bSilent = false);
};

}


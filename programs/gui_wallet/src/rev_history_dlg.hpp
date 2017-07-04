#pragma once


class Rev_history_dlg : public QDialog
{
   Q_OBJECT
public:
   Rev_history_dlg(const QString& revHistory, QWidget* pParent);
   ~Rev_history_dlg();
private:
   const QString& m_revHistory;

signals:
   void signal_btn_ok(void);
   void signal_btn_cancel(void);
private slots:
   void slot_btn_ok(void);
   void slot_btn_cancel(void);
};
#pragma once

#include <QLabel>

namespace gui_wallet
{
   class DecentLabel : public QLabel
   {
      Q_OBJECT
   public:
      enum eType
      {
         Default,
         RowLabel,
         ConnectingSplash,
         DecentLogo,
         Account,
         Row1Spacer,
         Balance,
         TableSearch
      };

      enum eName
      {
         None,
         Highlighted,
         Right,
         HighlightedRight
      };
      
      DecentLabel(QWidget* pParent, eType enType = Default, eName = None);

   protected:
      virtual void changeEvent(QEvent* event) override;
   };

   class StatusLabel : public DecentLabel
   {
      Q_OBJECT
   public:
      StatusLabel(QWidget* pParent);

   signals:
      void signal_removeTimers();

   public slots:
      void showMessage(QString const& str_message, int timeout);
      void clearMessage();
   };
}

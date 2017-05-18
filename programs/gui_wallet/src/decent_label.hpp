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
         ConnectingSplash
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

   public slots:
      void showMessage(QString const& str_message, int timeout);
      void clearMessage();
   };
}

/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
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
         TableSearch,
         TableSearchFrame,
         SplashInfo
      };

      enum eName
      {
         None,
         Highlighted,
         Right,
         HighlightedRight
      };
      
      DecentLabel(QWidget* pParent, eType enType = Default, eName enName = None);

   protected:
      virtual void changeEvent(QEvent* event) override;
   };

   class StatusLabel : public DecentLabel
   {
      Q_OBJECT
   public:
      StatusLabel(QWidget* pParent,
                  DecentLabel::eType enType = DecentLabel::Default,
                  DecentLabel::eName enName = DecentLabel::None);

   signals:
      void signal_removeTimers();

   public slots:
      void showMessage(QString const& str_message, int timeout);
      void clearMessage();
   };
}

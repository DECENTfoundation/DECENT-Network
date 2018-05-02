/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once

#include <QPushButton>

namespace gui_wallet
{
class DecentButton : public QPushButton
{
   Q_OBJECT
public:
   enum eType
   {
      Default,
      Send,
      TableIcon,
      StarIcon,
      DialogAction,
      DialogCancel,
      DialogTextButton,
      SplashAction,
      TabChoice,
      PasswordView
   };

   enum eName
   {
      None,
      Transaction,
      Detail,
      Transfer,
      Export,
      Resubmit
   };
   DecentButton(QWidget* pParent, eType enType = Default, eName = None);

protected:
   virtual bool event(QEvent* event) override;
   virtual void changeEvent(QEvent* event) override;
};

}

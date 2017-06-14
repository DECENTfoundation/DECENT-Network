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
      TabChoice
   };

   enum eName
   {
      None,
      Transaction,
      Detail,
      Transfer,
      Export
   };
   DecentButton(QWidget* pParent, eType enType = Default, eName = None);

protected:
   virtual bool event(QEvent* event) override;
   virtual void changeEvent(QEvent* event) override;
};

}

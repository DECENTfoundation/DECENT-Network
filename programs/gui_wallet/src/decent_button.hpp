#pragma once

#include <QLabel>
#include <QPushButton>

namespace gui_wallet {

class DecentButton : public QPushButton
{
   Q_OBJECT
public:
   enum eType
   {
      Default,
      Send
   };
   DecentButton(QWidget* parent,
                QString enabledImage,
                QString disabledImage = QString());
   DecentButton(QWidget *parent, eType enType = Default);


protected:
   virtual bool event(QEvent* event) override;
   virtual void changeEvent(QEvent* event) override;
};

}

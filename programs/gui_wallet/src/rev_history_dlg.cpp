#ifndef STDAFX_H
#include "../stdafx.h"
#endif

#include "rev_history_dlg.hpp"
#include "decent_button.hpp"
#include "gui_wallet_global.hpp"

namespace gui_wallet
{

Rev_history_dlg::Rev_history_dlg(const QString& revHistory, QWidget* pParent) : QDialog(pParent)
   , m_revHistory(revHistory)
{
   
   QPlainTextEdit* pRevHistory = new QPlainTextEdit(this);
   pRevHistory->setReadOnly(true);
   pRevHistory->setLineWrapMode(QPlainTextEdit::WidgetWidth);
   pRevHistory->setPlainText(revHistory);
   pRevHistory->moveCursor(QTextCursor::MoveOperation::End);
   pRevHistory->resize(500, 600);

   DecentButton* pOKButton = new DecentButton(this, DecentButton::DialogAction);
   DecentButton* pCancelButton = new DecentButton(this, DecentButton::DialogCancel);

   pOKButton->setText(tr("Download and Install"));
   pOKButton->setFont(PopupButtonBigFont());

   pCancelButton->setText(tr("Cancel"));
   pCancelButton->setFont(PopupButtonBigFont());

   QHBoxLayout* pTextLayout = new QHBoxLayout();
   pTextLayout->addWidget(pRevHistory);

   QHBoxLayout* pButtonsLayout = new QHBoxLayout;
   //pButtonsLayout->setContentsMargins(20, 20, 20, 20);
   pButtonsLayout->addWidget(pOKButton);
   pButtonsLayout->addWidget(pCancelButton);

   QVBoxLayout* pMainLayout = new QVBoxLayout;
   pMainLayout->setContentsMargins(40, 10, 40, 10);
   
   pMainLayout->addLayout(pTextLayout);
   pMainLayout->addLayout(pButtonsLayout);
   setLayout(pMainLayout);
   
   bool connected = QObject::connect(pOKButton, SIGNAL(pressed(void)), this, SLOT(slot_btn_ok(void)));
   connected = QObject::connect(pCancelButton, SIGNAL(pressed(void)), this, SLOT(slot_btn_cancel(void)));
   (void)connected;

  // QObject::connect(pOKButton, &QPushButton::clicked, this, &QDialog::close);
  // QObject::connect(pCancelButton, &QPushButton::clicked, this, &QDialog::close);

   resize(500, 600);
#ifdef _MSC_VER
   int height = style()->pixelMetric(QStyle::PM_TitleBarHeight);
   setWindowIcon(height > 32 ? QIcon(":/icon/images/windows_decent_icon_32x32.png")
      : QIcon(":/icon/images/windows_decent_icon_16x16.png"));
#endif
}

Rev_history_dlg::~Rev_history_dlg()
{
}

void Rev_history_dlg::slot_btn_ok(void)
{
   accept();
}

void Rev_history_dlg::slot_btn_cancel(void)
{
   reject();
}

}

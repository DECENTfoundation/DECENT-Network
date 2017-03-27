/*
 *
 *	File: richdialog.cpp
 *
 *	Created on: 27 Jan 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */


#include "richdialog.hpp"

using namespace gui_wallet;

RichDialogBase::RichDialogBase()
    :
      m_ok_button(tr("OK")),
      m_cancel_button(tr("Cancel"))
{
    m_buttons_layout.addWidget(&m_ok_button);
    m_buttons_layout.addWidget(&m_cancel_button);
    m_main_layout.addLayout(&m_controls_layout);
    m_main_layout.addLayout(&m_buttons_layout);
    setLayout(&m_main_layout);
    connect(&m_cancel_button,SIGNAL(clicked()),this,SLOT(close()));
    connect(&m_ok_button,SIGNAL(clicked()),this,SLOT(set_ok_and_closeSlot()));
}


void RichDialogBase::set_ok_and_closeSlot()
{
    m_ret_value = RDB_OK;
    close();
}


RET_TYPE RichDialogBase::execRB(const QPoint* a_pMove)
{
    m_ret_value = RDB_CANCEL;
    if(a_pMove){QDialog::move(*a_pMove);} QDialog::exec();
    return m_ret_value;
}


void RichDialogBase::AddLayout(QLayout* a_pLayout)
{
    m_controls_layout.addLayout(a_pLayout);
}

void RichDialogBase::AddWidget(QWidget* a_pWidget)
{
    m_controls_layout.addWidget(a_pWidget);
}


/********************************************/
RichDialog::RichDialog(int a_num_of_text_boxes)
    : m_nNumOfTextBoxes(a_num_of_text_boxes),m_pTextBoxes(NULL)
{
    if(a_num_of_text_boxes<=0) return;

    m_pTextBoxes = new QLineEdit[a_num_of_text_boxes];
    if(!m_pTextBoxes) {throw "Low memory!";}

    for(int i(0); i<a_num_of_text_boxes; ++i )
    {
        m_controls_layout.addWidget(&m_pTextBoxes[i]);
    }
}

RichDialog::~RichDialog() {
    if(m_nNumOfTextBoxes>0) {
       delete [] m_pTextBoxes;
    }
}


RET_TYPE RichDialog::execRD(const QPoint* a_pMove, std::vector<std::string>& a_cvResults)
{
    QString cqsResult;
    QByteArray cbaResult;
    int i,nVectInitSize(a_cvResults.size());
    int nSizeToSet(nVectInitSize<m_nNumOfTextBoxes ? nVectInitSize : m_nNumOfTextBoxes);

    for(i = 0; i<nSizeToSet; ++i){m_pTextBoxes[i].setText(tr(a_cvResults[i].c_str()));}

    RET_TYPE rtReturn = RichDialogBase::execRB(a_pMove);
    if(m_nNumOfTextBoxes>nVectInitSize){a_cvResults.resize(m_nNumOfTextBoxes);}

    for(i = 0; i<m_nNumOfTextBoxes; ++i)
    {
        cqsResult = m_pTextBoxes[i].text();
        cbaResult = cqsResult.toLatin1();
        a_cvResults[i] = cbaResult.data();
    }

    return rtReturn;
}

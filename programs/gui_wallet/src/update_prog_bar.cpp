#include "stdafx.h"
#include "update_prog_bar.hpp"
#include "decent_button.hpp"

const int PROGBAR_SIZE_MY_STANDARD_H = 450;
const int PROGBAR_SIZE_MY_STANDARD_V = 100;

//const int TIMER_STEP = 200;// ms
//const uint64_t DISPLAY_AFTER = 500;// zobraz az po 2 sekundach

CProgBar::CProgBar(int maximum, const QString& text, bool modal, bool hasCancel, QWidget *parent) :
    QProgressDialog(text, hasCancel ? tr("Cancel") : QString(), 0, maximum, parent)
    , m_modal(modal)
    , m_abort(nullptr)
    //, m_msToAppear(DISPLAY_AFTER)
    //, m_timerId(0)
    //, m_msCounter(0ULL)
    //, m_delayedShow(false)
    , m_hasCancel(hasCancel)
{
    setRange(0, maximum);
    //resize(300, 40);
    setWindowTitle("DECENT update");
    //setModal(modal);
    bool connected = connect(this, SIGNAL(canceled(void)), SLOT(progCanceled_slot(void)));
    connected;
    Qt::WindowFlags flags = windowFlags();
    Qt::WindowFlags helpFlag = Qt::WindowContextHelpButtonHint;
    flags = flags & (~helpFlag);
    if(hasCancel == false)
        flags = flags & ~Qt::WindowCloseButtonHint;
    setWindowFlags(flags);

    //m_timerId = startTimer(TIMER_STEP);

    if (hasCancel)
    {
       gui_wallet::DecentButton* pCancelButton = new gui_wallet::DecentButton(this, gui_wallet::DecentButton::DialogCancel);
       setCancelButton(pCancelButton);
    }
}

void CProgBar::Init(int maximum, const QString& text, uint32_t* abort, QWidget* parent)
{
    //SetMyStandardSize();
    
    setModal(false);// musi byt, inak setValue vyvola vynimku
    setValue(0);
    setMaximum(maximum);
    setLabelText(text);
    if(parent)
        setParent(parent);

    if(abort)
        m_abort = abort;
    setModal(m_modal);

    //m_msCounter = DISPLAY_AFTER;
}
void CProgBar::EnableCancel(bool enable)
{
    m_hasCancel = enable;
    setCancelButtonText(enable ? "Cancel" : QString());
    Qt::WindowFlags flags = windowFlags();
    Qt::WindowFlags helpFlag = Qt::WindowContextHelpButtonHint;
    flags = flags & (~helpFlag);
    if(m_hasCancel == false)
        flags = flags & ~Qt::WindowCloseButtonHint;
    else
        flags |= Qt::WindowCloseButtonHint;

    setWindowFlags(flags);
}
void CProgBar::progCanceled_slot(void)
{
    if(m_abort)
        *m_abort = 1;
    hide();
}

void CProgBar::Show(void)
{
    QProgressDialog::show();
}
void CProgBar::Raise(void)
{
    QProgressDialog::raise();
}
void CProgBar::SetValue(int pos)
{
    if(isVisible() == false)    // setValues() vyvolava show() ! tym sa po cancelovani a naslednom volani setValue() "bliklo" pred delete (timerEvent())
        return;
    if(pos < maximum())
        setValue(pos);
}

void CProgBar::SetLabelText(const QString& title)
{
    setLabelText(title);
}
void CProgBar::Hide(void)
{
    QProgressDialog::setVisible(false);
    //hide();
}
void CProgBar::SetMaximum(int max)
{
    QProgressDialog::setMaximum(max);
}
bool CProgBar::WasCanceled(void) const
{
    return ((QProgressDialog*)this)->wasCanceled();
}

void CProgBar::SetMyStandardSize(void)
{
   /*
   QString t;
   foreach(QObject *widget, children())
   {
      QPushButton* b = qobject_cast<QPushButton*>(widget);
      if (b)
      {
         t = b->text();
         if (t == tr("Cancel"))
         {
            QRect r = b->geometry();
            r.adjust(-20, -5, 0, 0);
            //b->setGeometry(r);
            b->resize(200, 50);
            update();
         }
      }
   }*/
   //QProgressDialog::resize(PROGBAR_SIZE_MY_STANDARD_H, PROGBAR_SIZE_MY_STANDARD_V); toto napriklad nefungovalo pri exporte logu,
    // neviem preco. Skusal som sledovat, kde sa velkost zmeni , al enenasiel (asi pri volani setVisible(), ale to sa vola aj pri inych
    // progress baroch a tam to nezmeni velkost
   QProgressDialog::setFixedSize(QSize(PROGBAR_SIZE_MY_STANDARD_H, PROGBAR_SIZE_MY_STANDARD_V+15));//15 - asi velkost zahlavia
  
}

void CProgBar::SetAutoClose(bool autoclose)
{
    setAutoClose(autoclose);
}
/*
void CProgBar::setVisible(bool visible)
{
    if(visible) {
        if((uint64_t)m_msToAppear >= m_msCounter)
            m_delayedShow = true;
        else {
            QProgressDialog::setVisible(visible);
        }
    } else {
        QProgressDialog::setVisible(visible);
    }
}*/
/*
void CProgBar::timerEvent(QTimerEvent *event)
{
   
    if(m_timerId == event->timerId())   {
        m_msCounter += TIMER_STEP;
        if((uint64_t)m_msToAppear < m_msCounter) {
            if(m_delayedShow)
                setVisible(true);
        }
        return;
    }
    
    QProgressDialog::timerEvent(event);
}*/

void CProgBar::closeEvent(QCloseEvent* e)
{
    if(m_hasCancel == false)  {
        e->ignore();
    } else
        QProgressDialog::closeEvent(e);
}

#pragma once

#ifndef STDAFX_H
#include <QProgressDialog>
#endif

class CProgBar : protected QProgressDialog
{
    Q_OBJECT
public:
    explicit CProgBar(int maximum, const QString& text, bool modal, bool hasCancel, QWidget *parent);
    void Init(int maximum, const QString& title, uint32_t* abort, QWidget* parent);
    void EnableCancel(bool enable);

    void Show();
    void Raise();
    void Hide();
    void SetValue(int pos);
    void SetLabelText(const QString& title);
    void SetMaximum(int max);
    bool WasCanceled() const;
    void SetMyStandardSize();
    void SetAutoClose(bool autoclose);

    //virtual void setVisible(bool visible);

signals:
    void progCanceled();
public slots:
    void progCanceled_slot();

private:
    bool m_modal;
    uint32_t* m_abort;

    //int m_msToAppear;// Za kolko milisekund sa ma objavit progres dialog ?
    //uint64_t m_msCounter;
    //int m_timerId;
    //bool m_delayedShow;
    bool m_hasCancel;

protected:
    //void timerEvent(QTimerEvent *event);
    void closeEvent(QCloseEvent* e);
};

/*
 *	File: gui_wallet_application.hpp
 *
 *	Created on: 14 Dec 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file is header file for class application
 *  this class will implement functional part necessary for the application
 *
 */
#ifndef GUI_WALLET_APPLICATION_HPP
#define GUI_WALLET_APPLICATION_HPP

#include <QApplication>
//#include "connected_api_instance.hpp"
#include "qt_commonheader.hpp"
#include "unnamedsemaphorelite.hpp"
#include "ui_wallet_functions.hpp"



namespace gui_wallet
{

    class application : public QApplication
    {
        Q_OBJECT
    public:
        application(int argc, char** argv);
        virtual ~application();

    };

}

typedef void (*TypeInGuiFnc)(void*);

struct SInGuiThreadCallInfo
{
    void*           data;
    TypeInGuiFnc    fnc;
};

class InGuiThreatCaller : public QObject
{
    Q_OBJECT

public:
    class QWidget*                      m_pParent2;
    union
    {
    int                                 m_nRes;
    std::string                         m_csRes;
    };
    decent::tools::UnnamedSemaphoreLite  m_sema;
public:
    InGuiThreatCaller();
    ~InGuiThreatCaller();
    void EmitShowMessageBox(const QString& a_str,WarnYesOrNoFuncType a_fpYesOrNo,void* a_pDataForYesOrNo);
    void EmitCallFunc(SInGuiThreadCallInfo a_call_info);

public slots:
    void MakeShowMessageBoxSlot(const QString& a_str,WarnYesOrNoFuncType a_fpYesOrNo,void* a_pDataForYesOrNo);
    void MakeCallFuncSlot(SInGuiThreadCallInfo a_call_info);

private:
signals:
    void ShowMessageBoxSig(const QString& a_str,WarnYesOrNoFuncType a_fpYesOrNo,void* a_pDataForYesOrNo);
    void CallFuncSig(SInGuiThreadCallInfo a_call_info);
};


#endif // GUI_WALLET_APPLICATION_HPP

#pragma once

#include <QApplication>
#include "qt_commonheader.hpp"
#include "unnamedsemaphorelite.hpp"
#include "ui_wallet_functions.hpp"



namespace gui_wallet {

   
class application : public QApplication
{
   Q_OBJECT
public:
   application(int argc, char** argv);
   virtual ~application();
   
};


typedef void (*TypeInGuiFunction)(void*);

struct SInGuiThreadCallInfo
{
    void*             data;
    TypeInGuiFunction function;
};

class InGuiThreatCaller : public QObject
{
    Q_OBJECT

public:
    class QWidget*                      m_pParent2;
   
    union {
       int                                 m_nRes;
       std::string                         m_csRes;
    };
   
    UnnamedSemaphoreLite                m_sema;
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

}

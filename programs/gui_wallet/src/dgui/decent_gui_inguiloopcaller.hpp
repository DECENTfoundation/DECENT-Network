#pragma once

#include "ui_wallet_functions_base.hpp"
#include <QObject>

namespace gui_wallet {

class InGuiLoopCaller : public QObject
{
    Q_OBJECT
public:
    InGuiLoopCaller();
    ~InGuiLoopCaller();
   
public:
   static InGuiLoopCaller* instance();
   
   void CallFunctionInGuiLoop2(SetNewTask_last_args2,const std::string& a_result,
                                void* owner,TypeCallbackSetNewTaskGlb2 fpFnc);


private:
signals:
    void NewFunctionToCallSig2(void*,int64_t, std::string, std::string,void*,TypeCallbackSetNewTaskGlb2);

private slots:
    void NextFunctionToCallSlot2(void*,int64_t, std::string, std::string,void*,TypeCallbackSetNewTaskGlb2);
};

}

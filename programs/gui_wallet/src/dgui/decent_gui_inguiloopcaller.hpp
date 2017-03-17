/*
 *	File: decent_gui_inguiloopcaller.hpp
 *
 *	Created on: 04 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef DECENT_GUI_INGUILOOPCALLER_HPP
#define DECENT_GUI_INGUILOOPCALLER_HPP

#include "ui_wallet_functions_base.hpp"
#include <QObject>

namespace decent{ namespace gui{

class InGuiLoopCaller : public QObject
{
    Q_OBJECT
public:
    InGuiLoopCaller();
    ~InGuiLoopCaller();

    void CallFunctionInGuiLoop2(SetNewTask_last_args2,const std::string& a_result,
                                void* owner,TypeCallbackSetNewTaskGlb2 fpFnc);

    void CallFunctionInGuiLoop3(SetNewTask_last_args2,const fc::variant& a_result,
                                void* owner,TypeCallbackSetNewTaskGlb3 fpFnc);

    // void* a_clbData,int64_t a_err, const std::string& a_inp, const std::string& a_result
private:
signals:
    void NewFunctionToCallSig2(void*,int64_t, std::string, std::string,void*,TypeCallbackSetNewTaskGlb2);
    void NewFunctionToCallSig3(void*,int64_t, std::string, fc::variant,void*,TypeCallbackSetNewTaskGlb3);

private slots:
    void NextFunctionToCallSlot2(void*,int64_t, std::string, std::string,void*,TypeCallbackSetNewTaskGlb2);
    void NextFunctionToCallSlot3(void*,int64_t, std::string, fc::variant,void*,TypeCallbackSetNewTaskGlb3);
};

}}

#endif // DECENT_GUI_INGUILOOPCALLER_HPP

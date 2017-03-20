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


private:
signals:
    void NewFunctionToCallSig2(void*,int64_t, std::string, std::string,void*,TypeCallbackSetNewTaskGlb2);

private slots:
    void NextFunctionToCallSlot2(void*,int64_t, std::string, std::string,void*,TypeCallbackSetNewTaskGlb2);
};

}}

#endif // DECENT_GUI_INGUILOOPCALLER_HPP

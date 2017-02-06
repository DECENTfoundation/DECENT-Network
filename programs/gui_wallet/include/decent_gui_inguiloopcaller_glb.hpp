//decent_gui_inguiloopcaller_glb.hpp
/*
 *	File: decent_gui_inguiloopcaller_glb.hpp
 *
 *	Created on: 04 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef DECENT_GUI_INGUILOOPCALLER_GLB_HPP
#define DECENT_GUI_INGUILOOPCALLER_GLB_HPP

#include "ui_wallet_functions_base.hpp"

int CallFunctionInGuiLoop_base(SetNewTask_last_args,void* owner,...);

template <typename Type>
int CallFunctionInGuiLoop(SetNewTask_last_args,Type* a_memb,void (Type::*a_fpFunc)(SetNewTask_last_args)){
    return CallFunctionInGuiLoop_base(a_clbData,a_err,a_inp,a_result,a_memb,a_fpFunc);
}

int CallFunctionInGuiLoop(SetNewTask_last_args,void* owner,TypeCallbackSetNewTaskGlb fpFnc);

int WarnAndWaitFunc(void* a_pOwner,WarnYesOrNoFuncType a_fpYesOrNo,
                    void* a_pDataForYesOrNo,const char* a_form,...);

#endif // DECENT_GUI_INGUILOOPCALLER_GLB_HPP

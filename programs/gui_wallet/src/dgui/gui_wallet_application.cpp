
#include "gui_wallet_application.hpp"

#include <mutex>
#include <QMessageBox>
#include <QWidget>
#include <stdarg.h>
#include <thread>


using namespace gui_wallet;


/*InGuiThreatCaller* s_pWarner = NULL;

=======
>>>>>>> 23697d791aa0432922a72d609a202a5d2a2e8a1f

gui_wallet::application::application(int argc, char** argv)
    :
      QApplication(argc,argv)
{
    qRegisterMetaType<std::string>( "std::string" );
<<<<<<< HEAD
    //qRegisterMetaType<WarnYesOrNoFuncType>( "WarnYesOrNoFuncType" );
=======
>>>>>>> 23697d791aa0432922a72d609a202a5d2a2e8a1f
    qRegisterMetaType<int64_t>( "int64_t" );
    //qRegisterMetaType<TypeCallbackSetNewTaskGlb2>( "TypeCallbackSetNewTaskGlb2" );
    qRegisterMetaType<SDigitalContent>( "SDigitalContent" );

    setApplicationDisplayName("Decent");
<<<<<<< HEAD
    
    s_pWarner = new InGuiThreatCaller;
    
}


gui_wallet::application::~application() {
    delete s_pWarner;
}




InGuiThreatCaller::InGuiThreatCaller()
{
    connect( this, SIGNAL(ShowMessageBoxSig(const QString&,WarnYesOrNoFuncType,void*)),
             this, SLOT(MakeShowMessageBoxSlot(const QString&,WarnYesOrNoFuncType,void*)) );
    connect( this, SIGNAL(CallFuncSig(SInGuiThreadCallInfo)),
             this, SLOT(MakeCallFuncSlot(SInGuiThreadCallInfo)) );
}

InGuiThreatCaller* InGuiThreatCaller::instance() {
   return s_pWarner;
}

void InGuiThreatCaller::EmitShowMessageBox(const QString& a_str,WarnYesOrNoFuncType a_fpYesOrNo,void* a_pDataForYesOrNo)
{
    emit ShowMessageBoxSig(a_str, a_fpYesOrNo, a_pDataForYesOrNo);
=======
>>>>>>> 23697d791aa0432922a72d609a202a5d2a2e8a1f
}


<<<<<<< HEAD
void InGuiThreatCaller::MakeShowMessageBoxSlot(const QString& a_str,WarnYesOrNoFuncType a_fpYesOrNo,void* a_pDataForYesOrNo)
{
    (*a_fpYesOrNo)(m_pParent2,m_nRes,a_pDataForYesOrNo);
    m_sema.post();
}


void InGuiThreatCaller::MakeCallFuncSlot(SInGuiThreadCallInfo a_call_info)
{
    (*a_call_info.function)(a_call_info.data);
}*/

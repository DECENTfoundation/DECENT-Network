
#include "gui_wallet_application.hpp"

#include <mutex>
#include <QMessageBox>
#include <QWidget>
#include <stdarg.h>
#include <thread>


using namespace gui_wallet;



gui_wallet::application::application(int argc, char** argv)
    :
      QApplication(argc,argv)
{
    qRegisterMetaType<std::string>( "std::string" );
    qRegisterMetaType<int64_t>( "int64_t" );
    qRegisterMetaType<TypeCallbackSetNewTaskGlb2>( "TypeCallbackSetNewTaskGlb2" );
    qRegisterMetaType<SDigitalContent>( "SDigitalContent" );

    setApplicationDisplayName("Decent");
}



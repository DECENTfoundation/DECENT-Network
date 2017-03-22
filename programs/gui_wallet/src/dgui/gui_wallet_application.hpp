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
   virtual ~application() {}
   
};


}

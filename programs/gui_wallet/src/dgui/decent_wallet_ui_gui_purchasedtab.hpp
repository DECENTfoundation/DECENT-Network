//decent_wallet_ui_gui_purchasedtab
/*
 *	File: decent_wallet_ui_gui_purchasedtab.hpp
 *
 *	Created on: 11 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */

#ifndef DECENT_WALLET_UI_GUI_PURCHASEDTAB_HPP
#define DECENT_WALLET_UI_GUI_PURCHASEDTAB_HPP

#include <QWidget>

namespace decent{ namespace wallet{ namespace ui{ namespace gui{

class PurchasedTab : public QWidget
{
public:
    QString getFilterText()const;
};

}}}}


#include "decent_wallet_ui_gui_common.tos"

#endif // DECENT_WALLET_UI_GUI_PURCHASEDTAB_HPP

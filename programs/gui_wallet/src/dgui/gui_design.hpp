/*
 *	File: gui_design.hpp
 *
 *	Created on: 7 Apr 2017
 *	Created by: Me
 *
 *  This file implements ...
 *
 */

#ifndef gui_design_h
#define gui_design_h

#include <iostream>
#include <string>
#include <QFont>

//Brows_content_tab
const char* const d_lineEdit        = "border: 0; padding-left: 10px;";

//decent_button
const char* const decent_button_style   = "QLabel { background-color :rgb(27,176,104); color : white;}";

//ui_gui_contentdatailsbase
const char* const m_RateText_design     = "color:green;" "background-color:white;" "font-weight: bold";
const char* const green_star_image      = ":/icon/images/green_asterix.png";
const char* const white_star_image      = ":/icon/images/white_asterix.png";
const char* const bg_color_grey         = "background-color:rgb(244,244,244);";
const char* const bg_color_wgite        = "background-color:white;";
const char* const font_bold             = "font-weight: bold";
const char* const col_grey              = "color: rgb(244,244,244)";
const char* const border_0              = "border: 0px;";

//decent_wallet_ui_gui_contentdetailsgeneral
const char* const d_close               = "QLabel { background-color :rgb(255,255,255); color : rgb(0,0,0);border: 1px solid grey}";
const char* const d_pButtonOk           = "background-color: rgb(27,176,104); color: rgb(255,255,255);border-top: 0px;border-left: 0px;border-right: 0px;border-bottom: 0px;";
const char* const d_pbuttonCancel       = "background-color: rgb(255,255,255); color: rgb(0,0,0);border: 1px solid grey;";

//decent_wallet_ui_gui_newcheckbox
const std::string csCheckedPath     = ":/icon/images/green_asterix.png";
const std::string csUnCheckedPath   = ":/icon/images/white_asterix.png";

const std::string qsStyleSheet      =
    std::string("QCheckBox::indicator:checked { image: url(") + csCheckedPath + "); }" +
                "QCheckBox::indicator:unchecked { image: url(" + csUnCheckedPath + "); }" +
                "QCheckBox::indicator { width: 13px; height: 13px; }";

//gui_wallet_centralwidget
const char* const d_main_tabs           = "QTabBar::tab{"
                                                    "font:bold;"
                                                    " height: 40px; width: 181px;"
                                                    "color:rgb(0,0,0);background-color:white;"
                                                    "border-left: 0px;"
                                                    "border-top: 1px solid rgb(240,240,240);"
                                                    "border-bottom: 1px solid rgb(240,240,240);}"
                                                    "QTabBar::tab:selected{"
                                                    "color:rgb(27,176,104);"
                                                    "border-bottom:3px solid rgb(27,176,104);"
                                                    "border-top: 1px solid rgb(240,240,240);"
                                                    "border-left:0px;"
                                                    "border-right:0px;}";
const char* const d_amount_label        = "color: rgb(27,176,104);""background-color:white;";
const char* const d_asset               = "color:black;""background-color:white;";
const char* const icon_decent           = ":/icon/images/decent_logo.svg";
const char* const d_color               = "color: #f0f0f0";
const char* const icon_user             = ":/icon/images/user.png";
const char* const icon_balance          = ":/icon/images/balance.png";
const char* const icon_send             = ":/icon/images/send.png";
const char* const icon_inactive_send    = ":/icon/images/inactive_send.png";

//gui_wallet_connectdlg
const char* const d_pass                = "border: 1px solid rgb(143,143,143);padding-left:25px;";

//gui_wallet_mainwindow
const char* const d_style               = "QMainWindow{color:black;""background-color:white;}";


//users_tab
const char* const icon_search              = ":/icon/images/search.svg";
const char* const icon_transaction         = ":/icon/images/transaction.png";
const char* const icon_transaction_white   = ":/icon/images/transaction1.png";
const char* const icon_transfer            = ":/icon/images/transfer.png";
const char* const icon_transfer_white      = ":/icon/images/transfer1.png";
const char* const d_table                  = "* { background-color: rgb(255,255,255); color : rgb(27,176,104); }";

//purchased_tab
const char* const icon_popup               = ":/icon/images/pop_up.png";
const char* const icon_popup_white         = ":/icon/images/pop_up1.png";
const char* const icon_export              = ":/icon/images/export.png";
const char* const icon_export_white        = ":/icon/images/export1.png";

//richdialog
const char* const d_cancel_button          = "QLabel {border: 1px solid rgb(143,143,143); background-color :rgb(255,255,255); color: rgb(0,0,0);}";
const char* const d_text_box            = "border: 1px solid rgb(143,143,143);padding-left:25px;";

//upload
const char* const d_desc                = "border-top: 0px; border-left: 0px; border-right: 0px; padding-left: 10px; border-bottom: 0px;";
const char* const d_label               = "QLabel { background-color : white; border:1 solid lightGray; color: Gray}";
const char* const d_price               = "border:1px solid lightGray; padding-left: 8px; color: Gray";
const char* const d_cancel              = "QLabel { background-color :rgb(255, 255, 255); border:1px solid lightGray; color : Grey;}";
const char* const d_upload_popup        = "background-color : white";

#endif //gui_design_h

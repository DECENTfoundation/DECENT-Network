#pragma once

class QFont;

//Brows_content_tab
const char* const d_lineEdit        = "border: 0; padding-left: 10px;";

//decent_button
const char* const decent_button_style   = "border: 0px ; background-color :rgb(27,176,104); color : white;";
//ui_gui_contentdatailsbase
const char* const d_qdialog             = "background-color:white;";
const char* const m_RateText_design     = "color:green;" "background-color:white;" "font-weight: bold";
const char* const green_star_image      = ":/icon/images/green_asterix.png";
const char* const white_star_image      = ":/icon/images/white_asterix.png";
const char* const bg_color_grey         = "background-color:rgb(244,244,244);";
const char* const bg_color_wgite        = "background-color:white;";
const char* const font_bold             = "font-weight: bold";
const char* const col_grey              = "color: rgb(244,244,244)";
const char* const border_0              = "border: 0px;";

//decent_wallet_ui_gui_contentdetailsgeneral
const char* const d_pButtonOk           = "background-color: rgb(27,176,104); color: rgb(255,255,255);border-top: 0px;border-left: 0px;border-right: 0px;border-bottom: 0px;";
const char* const d_pbuttonCancel       = "background-color: rgb(255,255,255); color: rgb(0,0,0);border: 1px solid grey;";

//decent_wallet_ui_gui_newcheckbox
#define D_CHECKED_PATH  ":/icon/images/green_asterix.png"
#define D_UNCHECKED_PATH   ":/icon/images/white_asterix.png"

const char* const d_CheckedPath     = D_CHECKED_PATH;
const char* const d_UncheckedPath   = D_UNCHECKED_PATH;

const char* const d_StyleSheet      =
               "QCheckBox::indicator:checked { image: url(" D_CHECKED_PATH "); }"
               "QCheckBox::indicator:unchecked { image: url(" D_UNCHECKED_PATH "); }"
               "QCheckBox::indicator { width: 13px; height: 13px; }";

//gui_wallet_centralwidget
const char* const d_pagination_buttons = "QPushButton{border-top: 1px solid lightGrey ;border-left: 1px solid lightGrey ; "
                                                      "background-color :rgb(255, 255, 255); color : rgb(27,176,104);}"
                                          "QPushButton:!enabled{background-color :rgb(242, 242, 242); color : rgb(30, 30, 30);}";

const char* const d_main_tabs           = "QTabBar::tab{"
#ifdef WINDOWS_HIGH_DPI
                                          "font-size: 10pt;"
#endif
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
const char* const d_label               = "color:black;""background-color:white;";
const char* const icon_decent           = ":/icon/images/decent_logo.svg";
const char* const d_color               = "color: #f0f0f0";
const char* const icon_user             = ":/icon/images/user.png";
const char* const icon_balance          = ":/icon/images/balance.png";
const char* const c_line                = "color: #ffffff";
const char* const icon_send             = ":/icon/images/send.png";
const char* const icon_inactive_send    = ":/icon/images/inactive_send.png";

//gui_wallet_connectdlg
const char* const d_pass                = "border: 1px solid rgb(143,143,143);padding-left:25px;";

//gui_wallet_mainwindow
const char* const d_style               = "QMainWindow{color:black;background-color:white;};";

extern const char* const d_global_white_style;




//users_tab
const char* const icon_search              = ":/icon/images/search.svg";
const char* const icon_transaction         = ":/icon/images/transaction.png";
const char* const icon_transaction_white   = ":/icon/images/transaction1.png";
const char* const icon_transfer            = ":/icon/images/transfer.png";
const char* const icon_transfer_white      = ":/icon/images/transfer1.png";
const char* const icon_transfer_           = ":/icon/images/transactionSVG.svg";


//purchased_tab
const char* const icon_popup               = ":/icon/images/pop_up.png";
const char* const icon_popup_white         = ":/icon/images/pop_up1.png";
const char* const icon_export              = ":/icon/images/export.png";
const char* const icon_export_white        = ":/icon/images/export1.png";

//richdialog
const char* const d_cancel_button          = "border: 1px solid rgb(143,143,143); background-color :rgb(255,255,255); color: rgb(0,0,0);";
const char* const d_text_box               = "border: 1px solid rgb(143,143,143);padding-left:25px;";

//upload
const char* const d_desc                = "border: 1 solid lightGray; padding 5px;";
const char* const d_label_v1            = "QLabel { background-color : white; color: Gray}";
const char* const d_label_v2            = "border:1px solid lightGray; color: Gray";
const char* const d_upload_button_true  = "border: 0px ; background-color :rgb(27,176,104); color : white;";
const char* const d_upload_button_false = "border: 0px ; background-color :rgb(180,180,180); color : rgb(30, 30, 30); ";
const char* const d_cancel              = "QLabel { background-color :rgb(255, 255, 255); border:1px solid lightGray; color : Grey;}";
#ifdef WINDOWS_HIGH_DPI
const char* const d_upload_popup        = "background-color : white; height: 150px; width: 400px;";
#else
const char* const d_upload_popup        = "background-color : white;";
#endif
const char* const c_keyparts            = "color : black;";

//DecentButton
const char* const GreenDecentButtonEnabled       = "QPushButton{border: 0px ; background-color :rgb(27,176,104); color : white;}"
                                                   "QPushButton:!enabled{background-color :rgb(180,180,180); color : rgb(30, 30, 30);}";

const char* const DecentButtonNormal             = "QPushButton{border: 0px ; background-color :rgb(255, 255, 255); color : rgb(0, 0, 0);}";

const char* const GreenDecentButtonNormal        = "QPushButton{border: 0px ; background-color :rgb(27,176,104); color : white;}";

const char* const DecentButtonEnabled            = "QPushButton{border: 0px ; background-color :rgb(255, 255, 255); color : rgb(0, 0, 0);}"
                                                   "QPushButton:!enabled{background-color :rgb(180,180,180); color : rgb(30, 30, 30);}";

QFont TableHeaderFont();
QFont AccountBalanceFont();
QFont DescriptionDetailsFont();
QFont PopupButtonRegularFont();
QFont PopupButtonBigFont();
QFont TabButtonFont();
QFont PaginationFont();


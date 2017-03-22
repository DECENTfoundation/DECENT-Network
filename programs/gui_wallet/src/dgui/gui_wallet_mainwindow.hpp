/*
 *	File: gui_wallet_mainwindow.cpp
 *
 *	Created on: Nov 11, 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef MAINWINDOW_GUI_WALLET_H
#define MAINWINDOW_GUI_WALLET_H

#include <QMainWindow>
#include "gui_wallet_centralwidget.hpp"
#include <QAction>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "gui_wallet_connectdlg.hpp"
#include "text_display_dialog.hpp"
#include "richdialog.hpp"
#include <unnamedsemaphorelite.hpp>
#include <stdarg.h>
#include <string>
#include <map>
#include <decent/wallet_utility/wallet_utility.hpp>
#include "decent_wallet_ui_gui_contentdetailsgeneral.hpp"

namespace gui_wallet
{
   


class Mainwindow_gui_wallet : public QMainWindow
{
   
   Q_OBJECT
public:
   Mainwindow_gui_wallet();
   virtual ~Mainwindow_gui_wallet();   // virtual because may be this class will be
   // used by inheritanc
   void SetNewTaskQtMainWnd2(const std::string& a_inp_line, void* a_clbData);
   
   void GoToThisTab(int index, std::string info);
   void UpdateAccountBalances(const std::string& username);
   
protected:
   void CreateActions();
   void CreateMenues();
   
   
private:
   
   void UpdateLockedStatus();
   
   void CliCallbackFnc(void*arg,const std::string& task);
   
   void SetPassword(void* a_owner, void* a_str_ptr);
   
   
protected slots:
   void CurrentUserChangedSlot(const QString&);
   void CheckDownloads();
   void DisplayWalletContentGUI();
   void DisplayConnectionError(std::string errorMessage);
   void CurrentUserBalanceUpdate();
   
protected slots:
   
   void AboutSlot();
   void HelpSlot();
   void InfoSlot();
   void ViewAction();
   
   void ConnectSlot();
   void ImportKeySlot();
   void LockSlot();
   void UnlockSlot();
   
protected:
   class QVBoxLayout*   m_pCentralAllLayout;
   class QHBoxLayout*   m_pMenuLayout;
   CentralWigdet*       m_pCentralWidget;
   
   QMenuBar *          m_barLeft;
   QMenuBar *          m_barRight;
   QMenu*              m_pMenuFile;
   QMenu*              m_pMenuSetting;
   QMenu*              m_pMenuHelpL;
   QMenu*              m_pMenuContent;
   QMenu*              m_pMenuHelpR;
   QMenu*              m_pMenuView;
   QMenu*              m_pMenuStatus;
   
   QAction             m_ActionExit;
   QAction             m_ActionConnect;
   QAction             m_ActionAbout;
   QAction             m_ActionInfo;
   QAction             m_ActionHelp;
   QAction             m_ActionLock;
   QAction             m_ActionUnlock;
   QAction             m_ActionImportKey;
   TextDisplayDialog   m_info_dialog;
   
   
   QVBoxLayout                         m_main_layout;
   bool                                m_locked;
   RichDialog                          m_import_key_dlg;
   int                                 m_nConnected;
   SConnectionStruct                   m_wdata2;
   PasswordDialog                      m_SetPasswordDialog;
   PasswordDialog                      m_UnlockDialog;
   
   QTimer                              _downloadChecker;
   QTimer                              _balanceUpdater;
   std::set<std::string>               _activeDownloads;

};

   
   
   
}

#endif // MAINWINDOW_GUI_WALLET_H

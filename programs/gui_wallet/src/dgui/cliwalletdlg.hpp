/*
 *	File: cliwalletdlg.hpp
 *
 *	Created on: 28 Jan 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef CLIWALLETDLG_HPP
#define CLIWALLETDLG_HPP

#define NUM_OF_RING_STR_BUFFER_MIN_ONE  0xf
#define NUM_OF_RING_STR_BUFFER  0x10

#include <QDialog>
#include <QTextEdit>
#include <string>

#ifndef __THISCALL__
#ifdef __MSC_VER
#define __THISCALL__ __thiscall
#else  // #ifdef __MSC_VER
#define __THISCALL__
#endif  // #ifdef __MSC_VER
#endif  // #ifndef __THISCALL__

#ifndef CLI_NEW_LINE_FNC_defined
#define CLI_NEW_LINE_FNC_defined
typedef void (__THISCALL__ *CLI_NEW_LINE_FNC)(void*owner,void*arg,const std::string& a_task);
#endif

namespace gui_wallet
{

class CliTextEdit : public QTextEdit
{
public:
   // CliTextEdit(QWidget* pParent);
    CliTextEdit();
    virtual ~CliTextEdit();

    template <typename Type>
    void SetCallbackStuff2(Type* a_obj_ptr,void* a_callbackArg,void (Type::*a_fpFunction)(void*arg,const std::string& task))
    {
        SetCallbackStuff2_base(a_obj_ptr,a_callbackArg,a_fpFunction);
    }

    void SetCallbackStuff2(void* owner, void* callbackArg,CLI_NEW_LINE_FNC fpCallback);

protected:
    virtual void keyReleaseEvent ( QKeyEvent * event );
    virtual void keyPressEvent( QKeyEvent * a_event );

    void SetCallbackStuff2_base(void* owner, void* callbackArg,...);

protected:
    void*               m_pOwner;
    void*               m_pCallbackArg;
    CLI_NEW_LINE_FNC    m_fpTaskDone;
    int                 m_nIndex;
    int                 m_nIndexFollower;
    std::string         m_vStrings[NUM_OF_RING_STR_BUFFER_MIN_ONE+1];
};


/**************************************************************/

class CliWalletDlg : public QDialog
{
public:
    CliWalletDlg(QTextEdit* pTextEdit=NULL);
    virtual ~CliWalletDlg();

    void appentText(const std::string& a_text);

    QTextEdit* operator->();

protected:
    virtual void resizeEvent(QResizeEvent * event );

protected:
    //CliTextEdit         m_main_textbox;
    QTextEdit*            m_pMainTextBox;
    unsigned int          m_nCreatedInside : 1;
};

}

#endif // CLIWALLETDLG_HPP

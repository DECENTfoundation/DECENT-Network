/*
 *	File: browse_content_tab.hpp
 *
 *	Created on: 11 Nov 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef BROWSE_CONTENT_TAB_H
#define BROWSE_CONTENT_TAB_H

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <vector>
#include <string>

#ifndef __THISCALL__
#ifdef __MSC_VER
#define __THISCALL__ __thiscall
#else  // #ifdef __MSC_VER
#define __THISCALL__
#endif  // #ifdef __MSC_VER
#endif  // #ifndef __THISCALL__


namespace gui_wallet
{

    // DCF stands for Digital Contex Fields
    namespace DCF {enum DIG_CONT_FIELDS{IS_SELECTED,TIME,SYNOPSIS,RATING,LEFT,SIZE,PRICE};}
    // DCA stands for Digital Contex Actions
    namespace DCA {enum DIG_CONT_ACTNS{CALL_GET_CONTENT};}

    struct SDigitalContent{
        std::string author;
        struct{
            double amount;
            std::string asset_id;
        }price;
        std::string synopsis;
        std::string URI;
        double AVG_rating;
        //
        std::string created;
        std::string expiration;
        double  size;

        std::string  get_content_str;
    };

#define _NEEDED_ARGS_ void* a_clb_data,int a_act,const gui_wallet::SDigitalContent* a_pDigContent
    typedef void (__THISCALL__ *TypeDigContCallback)(void* owner, _NEEDED_ARGS_);


    template <typename QtType>
    class TableWidgetItemW : public QtType
    {
    public:
        template <typename ConstrArgType, typename ClbType>
        TableWidgetItemW(ConstrArgType a_cons_arg,
                         const SDigitalContent& clbData,ClbType* own,void*clbDt,void (ClbType::*a_fpFunction)(_NEEDED_ARGS_));
        template <typename ClbType>
        TableWidgetItemW(const SDigitalContent& clbData, ClbType* own,void*clbDt,void (ClbType::*a_fpFunction)(_NEEDED_ARGS_));
        virtual ~TableWidgetItemW();
    protected:
        void prepare_constructor(int,...);
        virtual void mouseDoubleClickEvent(QMouseEvent* event);
    protected:
        void*               m_pOwner;
        void*               m_pCallbackData;
        SDigitalContent     m_callback_data;
        TypeDigContCallback m_fpCallback;
    };


    class Browse_content_tab : public QWidget
    {
        Q_OBJECT
    public:
        Browse_content_tab();
        virtual ~Browse_content_tab();

        void SetDigitalContentsGUI(const std::vector<gui_wallet::SDigitalContent>& contents);

    public:
    signals:
        void ShowDetailsOnDigContentSig(std::string get_cont_str);

    protected:
        void PrepareTableWidgetHeaderGUI();
        void DigContCallback(_NEEDED_ARGS_);

    protected:
        QVBoxLayout     m_main_layout;
        //QTableWidget    m_TableWidget; // Should be investigated
        QTableWidget*    m_pTableWidget;
        //int              m_nNumberOfContentsPlus1;
    };
}

#endif // BROWSE_CONTENT_TAB_H

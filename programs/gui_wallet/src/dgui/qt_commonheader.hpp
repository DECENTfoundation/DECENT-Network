/*
 *	File: qt_commonheader.hpp
 *
 *	Created on: 30 Jan 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef __qt_commonheader_hpp__
#define __qt_commonheader_hpp__

#include <QEvent>
#include <QObject>
#include <stdio.h>
#include <QMouseEvent>
#include "ui_wallet_functions_base.hpp"

#ifndef _NEEDED_ARGS1_
#define _NEEDED_ARGS1_(__type__)    void* a_clb_data,int a_act, const __type__* a_pDigContent
#endif

#ifndef _NEEDED_ARGS2_
#define _NEEDED_ARGS2_ _NEEDED_ARGS1_(decent::wallet::ui::gui::SDigitalContent)
#endif

// ST stands for search type
namespace ST{
enum STtype{URI_start,author,content,bought};
static const char* s_vcpcSearchTypeStrs[] = {"URI_start","author","content","bought"};
}

// DCA stands for Digital Contex Actions
namespace DCA {enum DIG_CONT_ACTNS{CALL_GET_CONTENT};}


namespace decent{namespace wallet{namespace ui{namespace gui{

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

}}}}

typedef void (__THISCALL__ *TypeDigContCallback)(void* owner, _NEEDED_ARGS2_);

namespace decent{namespace wallet{namespace ui{namespace gui{

template <typename QtType,typename ClbkDataType>
class TableWidgetItemW_base : public QtType
{
public:
    template <typename ClbType,typename ...ConstrArgTypes>
    TableWidgetItemW_base(const ClbkDataType& clbCont,ClbType* own,void*clbData,
                          void (ClbType::*a_fpFunction)(_NEEDED_ARGS1_(ClbkDataType)),
                          ConstrArgTypes... a_cons_args);
    virtual ~TableWidgetItemW_base();
protected:
    void prepare_constructor(int,...);
protected:
    void*               m_pOwner;
    void*               m_pCallbackData;
    ClbkDataType        m_callback_content;
    TypeDigContCallback m_fpCallback;
};


template <typename QtType>
class TableWidgetItemW : public TableWidgetItemW_base<QtType,SDigitalContent>
{
public:
    template <typename ClbType,typename ...ConstrArgTypes>
    TableWidgetItemW(const SDigitalContent& clbCont,ClbType* owner,void*clbData,
                     void (ClbType::*fpFunction)(_NEEDED_ARGS2_),
                     ConstrArgTypes... cons_args);
    virtual ~TableWidgetItemW();
protected:
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
protected:
    // no members
};

}}}}


#include "decent_wallet_ui_gui_common.tos"


#endif // #ifdef __qt_commonheader_hpp__

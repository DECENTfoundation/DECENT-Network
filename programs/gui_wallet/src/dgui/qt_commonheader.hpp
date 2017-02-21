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

#ifndef _NEEDED_ARGS2_
#define _NEEDED_ARGS2_ void* a_clb_data,int a_act,const decent::wallet::ui::gui::SDigitalContent* a_pDigContent
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

template <typename QtType>
class TableWidgetItemW : public QtType
{
public:
    template <typename ConstrArgType, typename ClbType>
    TableWidgetItemW(ConstrArgType a_cons_arg,
                     const SDigitalContent& clbData,ClbType* own,void*clbDt,void (ClbType::*a_fpFunction)(_NEEDED_ARGS2_));
    template <typename ClbType>
    TableWidgetItemW(const SDigitalContent& clbData, ClbType* own,void*clbDt,void (ClbType::*a_fpFunction)(_NEEDED_ARGS2_));
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

}}}}


typedef void (__THISCALL__ *TypeDigContCallback)(void* owner, _NEEDED_ARGS2_);

#include "decent_wallet_ui_gui_common.tos"


#endif // #ifdef __qt_commonheader_hpp__

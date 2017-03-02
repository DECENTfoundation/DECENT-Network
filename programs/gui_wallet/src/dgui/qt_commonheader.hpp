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

#define MOUSE_EVENT_LAST_ARGS void*a_clbData,QMouseEvent*a_mouse_event
typedef void (__THISCALL__* TypeMouseEvntClbk)(void*owner,MOUSE_EVENT_LAST_ARGS);

// DCA stands for Digital Contex Actions
namespace DCA {enum DIG_CONT_ACTNS{CALL_GET_CONTENT};}
// DCT stands for Digital Contex Actions
namespace DCT {enum DIG_CONT_TYPES{GENERAL,BOUGHT};}


namespace decent{namespace wallet{namespace ui{namespace gui{

struct SDigitalContent{
    SDigitalContent():type(DCT::GENERAL){}
    DCT::DIG_CONT_TYPES type;
    std::string author;
    struct{
        std::string amount2;
        std::string asset_id;
    }price;
    std::string synopsis;
    std::string URI;
    std::string AVG_rating2;
    //
    std::string created;
    std::string expiration;
    std::string  size2;

    //std::string  get_content_str;
    std::string  times_bougth2;
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


template <typename QtType>
class WidgetWithCallback : public QtType
{
public:
    template <typename ClbType,typename ...ConstrArgTypes>
    WidgetWithCallback(ClbType* owner,void*clbData,
                       void (ClbType::*fpFunction)(MOUSE_EVENT_LAST_ARGS),
                       ConstrArgTypes... cons_args);
    virtual ~WidgetWithCallback();

protected:
    virtual void mousePressEvent(QMouseEvent* event);

protected:
    void*               m_pOwner;
    void*               m_pCallbackData;
    TypeMouseEvntClbk   m_fpCallback;
};

}}}}


#include "decent_wallet_ui_gui_common.tos"


#endif // #ifdef __qt_commonheader_hpp__

/*
 *	File: decent_wallet_ui_gui_contentdetailsgeneral.cpp
 *
 *	Created on: 21 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#include <QGraphicsDropShadowEffect>

#include "decent_wallet_ui_gui_contentdetailsgeneral.hpp"
#include "gui_wallet_global.hpp"
#include <QMouseEvent>

using namespace gui_wallet;

// Implemented in 'gui_wallet_centralwigdet.cpp'
std::string FindImagePath(bool& a_bRet,const char* a_image_name);


ContentDetailsGeneral::ContentDetailsGeneral()
    :
      m_label(this,NULL,&ContentDetailsGeneral::LabelPushCallbackGUI)
{
    QVBoxLayout* image_layout = new QVBoxLayout;
    m_label.setText("BUY");
    
    image_layout->addWidget(&m_label);
    image_layout->setContentsMargins(250, 5, 250, 10);
    
    m_free_for_child.addLayout(image_layout);

    
    


    setFixedSize(620,480);
}


ContentDetailsGeneral::~ContentDetailsGeneral()
{
    //
}


void ContentDetailsGeneral::execCDD(const SDigitalContent& a_cnt_details) {
    execCDB(a_cnt_details);
}

#include <QMessageBox>
#include <QFileDialog>

void ContentDetailsGeneral::LabelPushCallbackGUI(void*,QMouseEvent* a_mouse_event)
{

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Please confirm", "Do you really want to buy this content?",
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    
    std::string downloadCommand = "download_content";
    downloadCommand += " " + GlobalEvents::instance().getCurrentUser();   //consumer
    downloadCommand += " \"" + m_pContentInfo->URI + "\"";                 //URI
    downloadCommand += " true";                                           //broadcast
    
    
    SetNewTask(downloadCommand, this, NULL, +[](void* owner, void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {
        if (a_err != 0) {
            ALERT("Failed to download content");
            return;
        }
        
        ContentDetailsGeneral* me = (ContentDetailsGeneral*)owner;
        
        me->close();
    });
    
    emit ContentWasBought();
}




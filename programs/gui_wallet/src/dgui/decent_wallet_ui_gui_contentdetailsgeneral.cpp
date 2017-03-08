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
    QPixmap aPixMap(":/icon/images/buy.png");
    QVBoxLayout* image_layout = new QVBoxLayout;
    m_label.setScaledContents(true);
    m_label.setPixmap(aPixMap);
    
    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(this);
    effect->setBlurRadius(0);
    effect->setColor(QColor("#000000"));
    effect->setOffset(1,1);
    m_label.setGraphicsEffect(effect);
    
    image_layout->addWidget(new QLabel());
    image_layout->addWidget(&m_label);
    image_layout->addWidget(new QLabel());
    m_free_for_child.addWidget(new QLabel());
    m_free_for_child.addWidget(new QLabel());
    m_free_for_child.addLayout(image_layout);
    m_free_for_child.addWidget(new QLabel());
    m_free_for_child.addWidget(new QLabel());
    
    


    setFixedSize(620,480);
}


ContentDetailsGeneral::~ContentDetailsGeneral()
{
    //
}


void ContentDetailsGeneral::execCDD(
        const SDigitalContent& a_cnt_details)
{
    execCDB(a_cnt_details);
}

#include <QMessageBox>
#include <QFileDialog>

void ContentDetailsGeneral::LabelPushCallbackGUI(void*,QMouseEvent* a_mouse_event)
{

    QString saveDir = QFileDialog::getExistingDirectory(this, tr("Select download directory"), "~", QFileDialog::DontResolveSymlinks);
    if (saveDir.isEmpty()) {
        return;
    }

    
    std::string downloadCommand = "download_content";
    downloadCommand += " " + GlobalEvents::instance().getCurrentUser();   //consumer
    downloadCommand += " \"" + m_pContentInfo->URI + "\"";                 //URI
    downloadCommand += " \"" + saveDir.toStdString() + "\"";              //Save dir
    downloadCommand += " true";                                           //broadcast


    SetNewTask(downloadCommand, this, NULL, +[](void* owner, void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result) {
        if (a_err != 0) {
            ALERT("Failed to download content");
            return;
        }

    });
    
    
}




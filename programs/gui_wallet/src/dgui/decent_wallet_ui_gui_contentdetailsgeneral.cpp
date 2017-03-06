/*
 *	File: decent_wallet_ui_gui_contentdetailsgeneral.cpp
 *
 *	Created on: 21 Feb 2017
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */


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
    m_label.setScaledContents(true);
    m_label.setPixmap(aPixMap);
    m_free_for_child.addWidget(new QLabel());
    m_free_for_child.addWidget(&m_label);
    m_free_for_child.addWidget(new QLabel());

    setFixedSize(397,381);
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

/*
    QString aCont = tr("mouse clich on: [x=") + QString::number(a_mouse_event->pos().x()) + tr(";y=") +
            QString::number(a_mouse_event->pos().y()) + tr("];");

    QString aDetails = tr("Overload function ") + tr(__FUNCTION__) +
            tr("\nFrom file \"") + tr(__SOURCE_FILE__) +
            tr("\",line=") + QString::number(__LINE__,10);

    QMessageBox aMessageBox(QMessageBox::Warning,
                            QObject::tr("should be modified!"),aCont,
                            QMessageBox::Ok,this);
    //aMessageBox.setStyleSheet(QMessageBox::);
    aMessageBox.setDetailedText(aDetails);

    aMessageBox.setFixedSize(200,100);
    //aMessageBox.setStyleSheet("");
    aMessageBox.setStyleSheet("QLabel{min-width: 300px;}");

    aMessageBox.exec();
    */

}




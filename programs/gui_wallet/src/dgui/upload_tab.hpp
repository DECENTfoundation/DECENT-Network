/*
 *	File      : upload_tab.hpp
 *
 *	Created on: 21 Nov 2016
 *	Created by: Davit Kalantaryan (Email: davit.kalantaryan@desy.de)
 *
 *  This file implements ...
 *
 */
#ifndef UPLOAD_TAB_HPP
#define UPLOAD_TAB_HPP

#include <map>
#include <string>

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QComboBox>
#include <QTimer>

#include "gui_wallet_tabcontentmanager.hpp"



#define INFO_LIFETIME   "Lifetime"
#define INFO_SEEDERS    "Seeders"
#define INFO_KEYPARTS   "Key particles"
#define INFO_TAGS       "Tags"
#define INFO_PRICE      "Price"


namespace gui_wallet
{
                                            
    enum FieldsRows {
        LIFETIME = 0,
        KEYPARTS,
        PRICE,
        ASSETID,
        SEEDERS,
        CONTENTPATH,
        SELECTPATH,
        SAMPLESPATH,
        SELECTSAMPLES,
        NUM_FIELDS
    };


    class Upload_tab : public TabContentManager
    {    
    Q_OBJECT

        typedef std::map<std::string, std::string> AssetMap;

    public:
        Upload_tab();
        virtual ~Upload_tab();
    public slots:
        void browseContent();
        void browseSamples();
        void uploadContent();
        void onGrabPublishers();

    public:
        void uploadDone(void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result);
        void onPublishersDone(void* a_clbkArg, int64_t a_err, const std::string& a_task, const std::string& a_result);

        virtual void content_activated() {}
        virtual void content_deactivated() {}

    protected:
        virtual void resizeEvent ( QResizeEvent * event );
    private:
        QVBoxLayout     m_main_layout;
        QVBoxLayout     m_synopsis_layout;
        QVBoxLayout     m_info_layout;
        QTableWidget    m_info_widget;
        
        QLabel          m_title_label;
        QLineEdit       m_title_text;

        QLabel          m_description_label;
        QTextEdit       m_description_text;

        QLabel          m_infoLayoutHeader;
        QTimer          m_getPublishersTimer;

        QLineEdit*      m_contentPath;
        QLineEdit*      m_samplesPath;
    };

}

#endif // UPLOAD_TAB_HPP

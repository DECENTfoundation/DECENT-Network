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
#include <QDateEdit>
#include <QComboBox>

#include "decent_button.hpp"
#include "gui_wallet_tabcontentmanager.hpp"

#include <QDialog>
#include <vector>
#include "qt_commonheader.hpp"
#include "gui_wallet_global.hpp"
#include "decent_wallet_ui_gui_contentdetailsgeneral.hpp"



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
    
    
    class Upload_popup : public TabContentManager
    {
        Q_OBJECT
        
        typedef std::map<std::string, std::string> AssetMap;
        
    public:
        Upload_popup();
        virtual ~Upload_popup();
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
        QVBoxLayout     u_main_layout;

    protected:
        virtual void resizeEvent ( QResizeEvent * event );
    private:
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
        
        QDateEdit*      de;
        QComboBox*      seeders;
        QComboBox*      keyparts;
        QLineEdit*      price;
        QLineEdit*      sim;
        QLineEdit*      cont;
    };
    
}



namespace gui_wallet
{
    class Mainwindow_gui_wallet;
    
    
    class Upload_tab : public TabContentManager
    {
        Q_OBJECT;
    public:
        Upload_tab(){}
        Upload_tab(Mainwindow_gui_wallet* parent);
        void ShowDigitalContentsGUI();
        
    public:
        virtual void content_activated() { }
        virtual void content_deactivated() {}
        
    public slots:
        void onTextChanged(const QString& text);
        void updateContents();
        void maybeUpdateContent();
        void requestContentUpdate();
        void show_content_popup();
        void content_was_bought();
        void upload_popup();
        
    protected:
        QVBoxLayout     m_main_layout;
        QHBoxLayout     m_search_layout;
        DecentTable     m_pTableWidget;
        QLineEdit       m_filterLineEdit;
        QComboBox       m_searchTypeCombo;
        DecentButton*   upload_button;
        
        std::vector<SDigitalContent>  _digital_contents;
        ContentDetailsGeneral*        _content_popup;
        Mainwindow_gui_wallet*        _parent;
        bool                          m_doUpdate;
        QTimer                        m_contentUpdateTimer;
    };
    
    class upload_up : public QDialog
    {
        Q_OBJECT
    public:
        upload_up(QWidget *parent = 0);
        ~upload_up(){}
        friend class Upload_tab;
    };
    
    
}

#endif //UploadTab_H

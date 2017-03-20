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

#include <vector>
#include "qt_commonheader.hpp"
#include <QDialog>
#include "gui_wallet_tabcontentmanager.hpp"

#define INFO_LIFETIME   "Lifetime"
#define INFO_SEEDERS    "Seeders"
#define INFO_KEYPARTS   "Key particles"
#define INFO_TAGS       "Tags"
#define INFO_PRICE      "Price"

// =========================upload pop up
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


    class Upload_tab_pop_up : public TabContentManager
    {    
    Q_OBJECT

        typedef std::map<std::string, std::string> AssetMap;

    public:
        Upload_tab_pop_up();
        virtual ~Upload_tab_pop_up();
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

// Upload tab





namespace gui_wallet
{
    // ST stands for search type
    namespace S_T{
        enum S_Ttype{URI_start,author,content};
        static const char* s_vcpcSearchTypeStrs[] = {"URI_start","author","content"};
    }
    
    
    class UTableWidget : public QTableWidget
    {
        Q_OBJECT
    public:
        UTableWidget(int a , int b) : QTableWidget(a,b)
        {
            this->setMouseTracking(true);
        };
        
        virtual void mouseMoveEvent(QMouseEvent * event);
    public:
    signals:
        void mouseMoveEventDid();
    };
    
    
    
    class UButton : public QLabel
    {
        Q_OBJECT
    public:
        UButton() : QLabel() {this->setMouseTracking(true);}
        UButton(QString str) : QLabel(str){this->setMouseTracking(true);}
    public:
    signals:
        void mouseWasMoved();
    public:
        virtual void mouseMoveEvent(QMouseEvent * event)
        {
            emit mouseWasMoved();
            QLabel::mouseMoveEvent(event);
        }
    };
    
    
    
    
    
    class Upload_tab : public TabContentManager
    {
        friend class CentralWigdet;
        Q_OBJECT
    public:
        Upload_tab();
        virtual ~Upload_tab();
        
        void ShowDigitalContentsGUI(std::vector<SDigitalContent>& contents);
        void Connects();
        
    public:
        
        virtual void content_activated() { m_doUpdate = true; }
        virtual void content_deactivated() {}
        
        std::string e_str;
        
        
    public:
    signals:
        void ShowDetailsOnDigContentSig(SDigitalContent get_cont_str);
        
        public slots:
        void onTextChanged(const QString& text);
        void doRowColor();
        void updateContents();
        void maybeUpdateContent();
        void UploadPopUp();

    protected:
        void DigContCallback(_NEEDED_ARGS2_);
        void PrepareTableWidgetHeaderGUI();
        virtual void resizeEvent ( QResizeEvent * a_event );
        void ArrangeSize();
        
    private:
        bool FilterContent(const SDigitalContent& content);
        
    protected:
        QVBoxLayout     m_main_layout;
        QHBoxLayout     m_search_layout;
        
        UTableWidget*    m_pTableWidget;
        
        QLineEdit       m_filterLineEdit;
        QComboBox       m_searchTypeCombo;
        
        std::vector<SDigitalContent> m_dContents;
        bool m_doUpdate = true;
        int green_row;
        QTimer  m_contentUpdateTimer;
    };
    
    class upload_up : public QDialog
    {
        Q_OBJECT
    public:
        upload_up(QWidget *parent = 0);
        ~upload_up(){}
        friend class Upload_tab;
        
//    public slots:
//        void dialog();
    };
    
    
}

#endif // UPLOAD_TAB_HPP

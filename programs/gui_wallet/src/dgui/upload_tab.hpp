#pragma once

#include <map>
#include <string>

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
#include <QCheckBox>

#include "decent_button.hpp"
#include "gui_wallet_tabcontentmanager.hpp"

#include <QDialog>
#include <vector>




namespace gui_wallet
{
   class Publisher;

   class Upload_popup : public QDialog
   {
      Q_OBJECT
   public:
      Upload_popup(QWidget* pParent);
      ~Upload_popup();

   public slots:
      void browseContent();
      void browseSamples();
      void uploadContent();
      void onGrabPublishers();
      void uploadCanceled();
      void updateUploadButtonStatus();
      void seederOkSlot();

      void slot_ChooseSeeders();
   signals:
      void uploadFinished();

   private:
      std::vector<std::pair<Publisher, bool>> m_arrPublishers;


      QVBoxLayout*     u_main_layout;
      QLineEdit*       _titleText;
      QTextEdit*       _descriptionText;
      QDateEdit*       _lifeTime;
      QComboBox*       _keyparts;
      QLineEdit*       _price;
      QLineEdit*       _seedersPath;
      QLineEdit*       _contentPath;
      QLineEdit*       _samplesPath;
      DecentButton*    _upload_button;
      DecentButton*    _cancel_button;
      DecentButton*    _seeder_ok;
      DecentTable*     _seeder_table;

      QTimer           m_getPublishersTimer;
      QTimer*          _buttonStatusCheck;
      QDialog*         _seeders_dialog;
      QLocale          _locale;

      std::map<std::string, double> _publisherIdToPriceMap;
      std::vector<QCheckBox*>       _seeders_checkbox;
      std::vector<std::string>      _checkedSeeders;

   };

}


class QSignalMapper;

namespace gui_wallet
{
   class DecentTable;
   class SDigitalContent;

   class Upload_tab : public TabContentManager
   {
      Q_OBJECT
   public:
      Upload_tab(QWidget* pParent);
      ~Upload_tab();
   public:
      virtual void timeToUpdate(const std::string& result) override;
      virtual std::string getUpdateCommand() override;

      void ShowDigitalContentsGUI();

   public slots:
      void slot_SearchTermChanged(QString const& strSearchTerm);
      void slot_SortingChanged(int);
      void slot_UpdateContents();
      void slot_Bought();
      void slot_ShowContentPopup(int);
      void slot_UploadPopup();

   signals:
      void signal_setEnabledUpload(bool);

   protected:
      DecentTable* m_pTableWidget;
      QSignalMapper* m_pDetailsSignalMapper;
      QString m_strSearchTerm;
      std::vector<SDigitalContent> _digital_contents;
   };
}


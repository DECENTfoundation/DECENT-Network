/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once

#include "richdialog.hpp"

class QDateEdit;
class QStringList;

namespace gui_wallet
{
   class Publisher;
   class DecentButton;
   class DecentTextEdit;
   class DecentLineEdit;

   class Upload_popup : public StackLayerWidget
   {
      Q_OBJECT
   public:
      Upload_popup(QWidget* pParent, const std::string& id_modify = std::string());
      ~Upload_popup();

   public slots:
      void slot_UpdateStatus();
      void slot_ChooseSeeders();
      void slot_SeederChanged(int);
      void slot_TitleChanged(const QString&);
      void slot_ContentPathChanged(const QString&);
      void slot_SamplesPathChanged(const QString&);
      void slot_PriceChanged(const QString&);
      void slot_BrowseContent();
      void slot_BrowseSamples();
      void slot_UploadContent();
   signals:
      void signal_SetSeedersText(const QString&);
      void signal_ContentPathChange(const QString&);
      void signal_SamplesPathChange(const QString&);
      void signal_UploadButtonEnabled(bool);

   protected:
      QStringList getChosenPublishers() const;
      void getContents(std::string const& id,
                       std::string& hash,
                       std::string& str_expiration,
                       std::string& str_size,
                       std::string& str_quorum,
                       std::string& str_fee,
                       std::string& str_cd,
                       std::string& uri) const;
      void getContents(std::string const& id,
                       std::string& title,
                       std::string& description,
                       std::string& price,
                       std::string& str_expiration);

   private:
      QTimer* m_pStatusCheckTimer;
      DecentTextEdit* m_pDescriptionText;
      DecentLineEdit* m_pPriceEditor;
      QLabel* m_pTotalPriceLabel;
      QDateEdit* m_pLifeTime;
      double m_dPrice;
      std::string m_id_modify;
      QString m_strTitle;
      QString m_strContentPath;
      QString m_strSamplesPath;

      std::vector<std::pair<Publisher, bool>> m_arrPublishers;
   };
   
}

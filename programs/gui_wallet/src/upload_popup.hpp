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
      void slot_TitleChanged(QString const&);
      void slot_KeyParticlesChanged(QString const&);
      void slot_ContentPathChanged(QString const&);
      void slot_SamplesPathChanged(QString const&);
      void slot_PriceChanged(QString const&);
      void slot_BrowseContent();
      void slot_BrowseSamples();
      void slot_UploadContent();
   signals:
      void signal_SetSeedersText(QString const&);
      void signal_ContentPathChange(QString const&);
      void signal_SamplesPathChange(QString const&);
      void signal_UploadButtonEnabled(bool);
      void signal_UploadButtonSetText(QString const&);

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
      QDateEdit* m_pLifeTime;
      uint32_t m_iKeyParticles;
      double m_dPrice;
      std::string m_id_modify;
      QString m_strTitle;
      QString m_strContentPath;
      QString m_strSamplesPath;

      std::vector<std::pair<Publisher, bool>> m_arrPublishers;
   };
   
}

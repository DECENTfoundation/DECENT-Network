#pragma once

#include <QDialog>

class QDateEdit;
class QStringList;

namespace gui_wallet
{
   class Publisher;
   class DecentButton;
   class DecentTextEdit;

   class Upload_popup : public QDialog
   {
      Q_OBJECT
   public:
      Upload_popup(QWidget* pParent);
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

   private:
      QTimer* m_pStatusCheckTimer;
      DecentTextEdit* m_pDescriptionText;
      QDateEdit* m_pLifeTime;
      uint32_t m_iKeyParticles;
      double m_dPrice;
      QString m_strTitle;
      QString m_strContentPath;
      QString m_strSamplesPath;

      std::vector<std::pair<Publisher, bool>> m_arrPublishers;
   };
   
}

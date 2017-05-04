#pragma once

#include <QDialog>
#include <QStringList>
#include <QLocale>

#include "decent_button.hpp"

class QTextEdit;
class QDateEdit;

namespace gui_wallet
{
   class Publisher;

   class Upload_popup : public QDialog
   {
      Q_OBJECT
   public:
      Upload_popup(QWidget* pParent);
      ~Upload_popup();

   public:
      QStringList getChosenPublishers() const;
      public slots:
      void browseContent();
      void browseSamples();
      void uploadContent();


      void slot_UpdateStatus();
      void slot_ChooseSeeders();
      void slot_SeederChanged(int);
      void slot_TitleChanged(QString const&);
      void slot_KeyParticlesChanged(QString const&);
      void slot_PriceChanged(QString const&);
   signals:
      void signal_SetSeedersText(QString const&);

   private:
      QTimer* m_pStatusCheckTimer;
      QTextEdit* m_pDescriptionText;
      QDateEdit* m_pLifeTime;
      uint32_t m_iKeyParticles;
      double m_dPrice;
      QString m_strTitle;

      std::vector<std::pair<Publisher, bool>> m_arrPublishers;


      QLineEdit*       _contentPath;
      QLineEdit*       _samplesPath;
      DecentButton*    _upload_button;
      
      QLocale          _locale;
   };
   
}

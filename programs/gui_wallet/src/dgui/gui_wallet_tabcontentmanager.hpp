#pragma once

#include <QWidget>


namespace gui_wallet {
   

class TabContentManager : public QWidget {
   
public:
   virtual void contentActivated() {
      _lastResult = "";
   }
   
   virtual void contentDeactivated() {
      
   }
   virtual void timeToUpdate(const std::string& result) = 0;
   virtual std::string getUpdateCommand() = 0;
   
public:
   void tryToUpdate();
   
private:
   
   std::string _lastResult = "";
};

   
}


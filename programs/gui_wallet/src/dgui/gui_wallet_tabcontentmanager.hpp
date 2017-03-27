
#ifndef TAB_CONTENT_MANAGER_HPP
#define TAB_CONTENT_MANAGER_HPP


#include <QWidget>
#include "ui_wallet_functions.hpp"

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
   virtual void RunTask(std::string const& str_command, std::string& str_result) = 0;
   
public:
   void tryToUpdate() {
      try {
         std::string command = getUpdateCommand();
         if (command.empty()) {
            timeToUpdate("");
            return;
         }
         
         std::string result;
         try {
            RunTask(command, result);
         } catch (...) {
            result = "";
         }
         if (result != _lastResult) {
            _lastResult = result;
            timeToUpdate(result);
         }
      } catch (...) {
         
      }
   }
private:
   
   std::string _lastResult = "";
};

   
}


#endif // TAB_CONTENT_MANAGER_HPP

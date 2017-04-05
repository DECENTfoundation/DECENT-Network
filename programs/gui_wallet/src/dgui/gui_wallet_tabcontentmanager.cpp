
#include "gui_wallet_mainwindow.hpp"
#include <QWidget>


using namespace gui_wallet;


void TabContentManager::tryToUpdate() {
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


#include "gui_wallet_mainwindow.hpp"

using string = std::string;

namespace gui_wallet
{
TabContentManager::TabContentManager(QWidget* pParent/* = nullptr*/)
: QWidget(pParent)
, m_i_page_size(50)
, m_last_result()
, m_next_iterator()
{
}

void TabContentManager::contentActivated()
{
   m_last_result = "";
}
void TabContentManager::contentDeactivated()
{

}

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
      if (result != m_last_result) {
         m_last_result = result;
         timeToUpdate(result);
      }
   } catch (...) {
      
   }
}

bool TabContentManager::next()
{
   if (is_last())
      return false;

   m_iterators.push_back(m_next_iterator);
   tryToUpdate();
   return true;
}

void TabContentManager::reset()
{
   m_iterators.clear();
   m_next_iterator.clear();
   m_last_result.clear();
   tryToUpdate();
}

bool TabContentManager::previous()
{
   if (is_first())
      return false;

   m_iterators.pop_back();
   tryToUpdate();
   return true;
}

bool TabContentManager::is_first() const
{
   return m_iterators.empty();
}

bool TabContentManager::is_last() const
{
      if (m_next_iterator.empty())
         return true;
      return false;
}

void TabContentManager::set_next_page_iterator(string const& iterator)
{
   m_next_iterator = iterator;
}

std::string TabContentManager::next_iterator() const
{
   string str_iterator;
   if (false == m_iterators.empty())
      str_iterator = m_iterators.back();

   return str_iterator;
}

}  // namespace gui_wallet

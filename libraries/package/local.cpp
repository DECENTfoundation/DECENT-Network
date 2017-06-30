/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */


#include "local.hpp"
#include "detail.hpp"

#include <decent/package/package.hpp>



namespace decent { namespace package {

void LocalDownloadPackageTask::task() {
   PACKAGE_INFO_GENERATE_EVENT(package_download_start, ( ) );

   PACKAGE_TASK_EXIT_IF_REQUESTED;

   PACKAGE_INFO_CHANGE_TRANSFER_STATE(DOWNLOADING);

   PACKAGE_INFO_CHANGE_DATA_STATE(PARTIAL);

   PACKAGE_INFO_CHANGE_DATA_STATE(CHECKED);
   PACKAGE_INFO_CHANGE_TRANSFER_STATE(TS_IDLE);
   PACKAGE_INFO_GENERATE_EVENT(package_download_complete, ( ) );
}

}} //namespace




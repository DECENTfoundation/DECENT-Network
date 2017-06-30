/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once

#include "detail.hpp"

#include <decent/package/package.hpp>



namespace decent { namespace package {


class TransferEngineInterface;

class LocalDownloadPackageTask : public detail::PackageTask {
public:
   explicit LocalDownloadPackageTask(PackageInfo& package): detail::PackageTask(package){};

protected:
   virtual void task() override;

};

class LocalTransferEngine : public TransferEngineInterface {
public:
   virtual std::shared_ptr<detail::PackageTask> create_download_task(PackageInfo& package) override {
      return std::make_shared<LocalDownloadPackageTask>(package);
   };
   virtual std::shared_ptr<detail::PackageTask> create_start_seeding_task(PackageInfo& package) override 
   {
      std::shared_ptr<detail::PackageTask> result;
      return result;
   };
   virtual std::shared_ptr<detail::PackageTask> create_stop_seeding_task(PackageInfo& package) override 
   {
      std::shared_ptr<detail::PackageTask> result;
      return result;
   };
};

}} //namespace

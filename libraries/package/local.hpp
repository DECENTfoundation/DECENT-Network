/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#pragma once

#include "detail.hpp"

#include <decent/package/package.hpp>

#include <stdlib.h>

namespace decent { namespace package {


class TransferEngineInterface;

class LocalDownloadPackageTask : public detail::PackageTask {
public:
   explicit LocalDownloadPackageTask(PackageInfo& package): detail::PackageTask(package){};

protected:
   virtual void task() override;
private:
   virtual bool is_base_class() override {return false;};
};

class LocalTransferEngine : public TransferEngineInterface {
public:
   virtual std::shared_ptr<detail::PackageTask> create_download_task(PackageInfo& package) override {
      return std::make_shared<LocalDownloadPackageTask>(package);
   };
   virtual std::shared_ptr<detail::PackageTask> create_start_seeding_task(PackageInfo& package) override 
   {
      elog("this shall be never called!");
      std::abort();
      std::shared_ptr<detail::PackageTask> result;
      return result;
   };
   virtual std::shared_ptr<detail::PackageTask> create_stop_seeding_task(PackageInfo& package) override 
   {
      elog("this shall be never called!");
      std::abort();
      std::shared_ptr<detail::PackageTask> result;
      return result;
   };
};

}} //namespace

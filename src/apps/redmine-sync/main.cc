
//          Copyright Kay Stenschke 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <apps/redmine-sync/app/app.h>
#include "apps/redmine-sync/app/app_config.h"

/**
  * @param argc Amount of arguments received
  * @param argv Array of arguments received, argv[0] is name and path of executable
  */
int main(int argc,  char** argv) {
   // Ensure config file exists
   tictac_rms::AppConfig::GetInstance(argv);

   //tictac_track::ReportCrud report_crud = tictac_track::ReportCrud::GetInstance();
   //if (!report_crud.ReportExists()) {
   //  return -1;
   //}

  (new tictac_rms::App(argc, argv))->Process();

   return 0;
}

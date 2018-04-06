
//          Copyright Kay Stenschke 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef CLASS_TIMETRACKERCL_REPORT_FILE
#define CLASS_TIMETRACKERCL_REPORT_FILE

#include <string>

namespace tictac_rms {
class ReportFile {
 public:
  static const std::string kFilenameReport;

  static bool ReportExists();

  static std::string GetReportHtml();
  static std::string getReportFilePath();

 private:
  // Get input stream to report file
  static std::ifstream GetReportIfStream();
};
} // namespace tictac_lib

#endif

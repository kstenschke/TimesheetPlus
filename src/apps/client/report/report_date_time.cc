
//          Copyright Kay Stenschke 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <utility>
#include <vector>
#include <cstring>
#include "report_date_time.h"
#include "apps/client/app/app_config.h"
#include "helper/helper_date_time.h"
#include "helper/helper_string.h"
#include "helper/helper_numeric.h"

namespace tictac_track {

/**
   * Constructor
   */
  ReportDateTime::ReportDateTime() {
    AppConfig config = AppConfig::GetInstance();

    format_week_of_year_ = config.GetConfigValue("format_week_of_year");
    format_date_         = config.GetConfigValue("format_date");
    format_day_of_week_  = config.GetConfigValue("format_day_of_week");

    locale_key_ = config.GetConfigValue("locale");
  }

  std::string ReportDateTime::GetDateFormat() {
    return format_date_;
  }

  /**
   * Translate e.g. "Monday" to "Montag", using configured locale language
   */
  std::string ReportDateTime::TranslateWeekday(const char* weekday_name_en) {
    int weekday_index = helper::DateTime::GetWeekdayIndexByName(weekday_name_en);
    if (weekday_index > 7) return weekday_name_en;

    const char* locale = locale_key_.c_str();

    if (0 == std::strcmp("de", locale)) return weekDayLabelsDe[weekday_index];
    if (0 == std::strcmp("dk", locale)) return weekDayLabelsDk[weekday_index];
    if (0 == std::strcmp("es", locale)) return weekDayLabelsEs[weekday_index];
    if (0 == std::strcmp("fi", locale)) return weekDayLabelsFi[weekday_index];
    if (0 == std::strcmp("fr", locale)) return weekDayLabelsFr[weekday_index];
    if (0 == std::strcmp("hu", locale)) return weekDayLabelsHu[weekday_index];
    if (0 == std::strcmp("it", locale)) return weekDayLabelsIt[weekday_index];
    if (0 == std::strcmp("lt", locale)) return weekDayLabelsLt[weekday_index];
    if (0 == std::strcmp("lv", locale)) return weekDayLabelsLv[weekday_index];
    if (0 == std::strcmp("nl", locale)) return weekDayLabelsNl[weekday_index];
    if (0 == std::strcmp("no", locale)) return weekDayLabelsNo[weekday_index];
    if (0 == std::strcmp("pl", locale)) return weekDayLabelsPl[weekday_index];
    if (0 == std::strcmp("pt", locale)) return weekDayLabelsPt[weekday_index];
    if (0 == std::strcmp("ro", locale)) return weekDayLabelsRo[weekday_index];
    if (0 == std::strcmp("sv", locale)) return weekDayLabelsSv[weekday_index];
    if (0 == std::strcmp("tr", locale)) return weekDayLabelsTr[weekday_index];

    return weekDayLabelsEn[weekday_index];
  }

  std::string ReportDateTime::GetCurrentWeekOfYear(int offset_weeks) {
    std::string week = helper::DateTime::GetCurrentTimeFormatted(format_week_of_year_.c_str());

    if (0 != offset_weeks) {
      int week_number = helper::String::ToInt(week.c_str());
      week_number += offset_weeks;

      week = helper::Numeric::ToString(week_number);
      if (1 == week.size()) week = "0" + week;
    }

    return week;
  }

  std::string ReportDateTime::GetCurrentDate(int offset_days) {
    return helper::DateTime::GetCurrentTimeFormatted(format_date_.c_str(), offset_days);
  }

  std::string ReportDateTime::GetTimestampForMeta() {
    return helper::DateTime::GetCurrentTimeFormatted("%Y/%m/%W/%d");
  }

  /**
   * Get day of week as string, localized into configured language
   */
  std::string ReportDateTime::GetCurrentDayOfWeek() {
    std::string weekdayNameEnglish = helper::DateTime::GetCurrentTimeFormatted(format_day_of_week_.c_str());

    return TranslateWeekday(weekdayNameEnglish.c_str());
  }

  /**
   * Get localize name of day of week, from given meta data
   */
  std::string ReportDateTime::GetWeekdayByMeta(std::string meta) {
    std::vector<std::string> meta_parts = helper::String::Explode(meta, '/');
    int day_of_week  = helper::DateTime::GetWeekdayIndexByDate(
      helper::String::ToInt(meta_parts[1]),  // year
      helper::String::ToInt(meta_parts[2]),  // month
      helper::String::ToInt(meta_parts[4])); // day

    return TranslateWeekday(helper::DateTime::GetWeekdayEnByIndex(day_of_week).c_str());
  }

  std::string ReportDateTime::GetCurrentTime() {
    return helper::DateTime::GetCurrentTimeFormatted(helper::DateTime::FORMAT_TIME);
  }

  std::string ReportDateTime::GetDurationFormatted(std::string time_started, std::string time_stopped) {
    int minutes_start = helper::DateTime::GetSumMinutesFromTime(std::move(time_started), ":");
    int minutes_end   = helper::DateTime::GetSumMinutesFromTime(std::move(time_stopped), ":");

    int minutes_duration = minutes_end - minutes_start;
    if (minutes_duration < 0) {
      // Entry timespan is crossing midnight (1 midnight is supported supported)
      minutes_duration = helper::DateTime::kMinutesPerDay - minutes_start + minutes_end;
    }

    return helper::DateTime::GetHoursFormattedFromMinutes(minutes_duration);
  }

  /**
   * Check whether date1 is after date2
   */
  bool ReportDateTime::IsMetaDateAfter(std::string meta_date1, std::string meta_date2) {
    std::vector<std::string> meta_parts1 = helper::String::Explode(meta_date1, '/');
    std::vector<std::string> meta_parts2 = helper::String::Explode(meta_date2, '/');

    int year_date1 = helper::String::ToInt(meta_parts1[1]);
    int year_date2 = helper::String::ToInt(meta_parts2[1]);
    if (year_date1 > year_date2) return true;
    if (year_date1 < year_date2) return false;

    // Years are same
    int month_date1 = helper::String::ToInt(meta_parts1[2]);
    int month_date2 = helper::String::ToInt(meta_parts2[2]);
    if (month_date1 > month_date2) return true;
    if (month_date1 < month_date2) return false;

    // Months are same
    int day_date1 = helper::String::ToInt(meta_parts1[4]);
    int day_date2 = helper::String::ToInt(meta_parts2[4]);

    return day_date1 > day_date2;
  }
} // namespace tictac_track

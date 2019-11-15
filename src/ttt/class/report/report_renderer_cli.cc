/*
  Copyright (c) 2018-2019, Kay Stenschke
  All rights reserved.
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
   * Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
   * Neither the name of  nor the names of its contributors may be used to
     endorse or promote products derived from this software without specific
     prior written permission.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

#include <iostream>
#include <cstring>
#include <ttt/helper/helper_tui.h>
#include <fstream>
#include <ttt/helper/helper_file.h>

#include "report_renderer_cli.h"
#include "../../helper/helper_string.h"
#include "../app/app_config.h"
#include "../../helper/helper_numeric.h"
#include "report_parser.h"
#include "report_browser.h"
#include "../../helper/helper_date_time.h"
#include "../../helper/helper_html.h"
#include "../../helper/helper_system.h"
#include "../app/app_error.h"

namespace tictac_track {

/**
 * Constructor: init CLI ANSI color theme
 */
ReportRendererCli::ReportRendererCli() {
  InitAnsiTheme();
  offset_id_column_ = helper::String::ToInt(AppConfig::GetInstance().GetConfigValue("id_column"));

  AppConfig config = AppConfig::GetInstance();
  minutes_break_ = helper::String::ToInt(config.GetConfigValue("max_mergeable_minutes_gap"));
}

/**
 * Pretty-print timesheet HTML (table) to CLI, using given filters
 */
bool ReportRendererCli::PrintToCli(RenderScopes scope, int lookbehind_amount, int task_number, std::string comment) {
  render_scope_ = scope;

  ReportParser *parser = new ReportParser();
  std::string report_html = parser->GetHtml();
  bool containsRows = helper::String::GetSubStrCount(report_html.c_str(), "<tr") >= 2;

  if (!ExtractPartsFromReport(lookbehind_amount)) return false;

  max_index_digits_ = helper::Numeric::GetAmountDigits(amount_rows_);
  PrintHeader();

  if (containsRows && 0 < PrintRows(task_number, std::move(comment)))
    // Rows exist and have been printed
    return true;

  std::string message = !containsRows || rows_filter_.empty()
                        ? std::string(" No entries found.")
                        : std::string(" No entries with ")
                            .append("\"")
                            .append(rows_filter_)
                            .append("\" in ")
                            .append(GetActiveScopeName())
                            .append(" column found.");

  if (containsRows && RenderScopes::Scope_Day==render_scope_)
    message = message.append(GetMessageHintClosestDayEntryBefore(lookbehind_amount));

  tictac_track::AppError::PrintError(message.append("\n").c_str());

  return false;
}

bool ReportRendererCli::PrintBrowseDayTasks(int days_offset) {
  render_scope_ = RenderScopes::Scope_Day;

  ReportParser *parser = new ReportParser();
  if (!parser->LoadReportHtml()
      || -1==parser->GetLastIndex())
    return false;

  auto *browser = new ReportBrowser();

  AppConfig &config = AppConfig::GetInstance();
  std::ifstream file(config.GetReportFilePath());
  if (!file) return false;

  std::string html = helper::File::GetFileContents(file);
  if (html.empty()) return false;

  std::string date = report_date_time_instance_.GetDateFormatted(days_offset);
  std::vector<std::string> tasks = parser->GetTasksOfDay(date);
  int amount_tasks = tasks.size();

  int i = 1;
  for (auto const &task: tasks) {
    cells_.clear();

    if (!ExtractPartsFromReport(days_offset, html)) return false;

    max_index_digits_ = helper::Numeric::GetAmountDigits(amount_rows_);

    PrintHeader(false, false, false);

    int task_number = helper::String::ToInt(task);
    PrintRows(task_number, "", false, false, false, false);

    browser->BrowseTaskUrl(task_number);

    if (i < amount_tasks) {
      std::cout << "\n";
      std::cout << "[Enter]";
      helper::System::WaitForEnterKeyPress();
      std::cout << "\r       ";
    }

    ++i;
  }

  std::cout << "\n";

  return true;
}

std::string ReportRendererCli::GetMessageHintClosestDayEntryBefore(int lookbehind_amount) {
  ReportParser *parser = new ReportParser();
  if (!parser->LoadReportHtml()
      || -1==parser->GetLastIndex()
      )
    return std::string("");

  int available_lookbehind_offset = parser->GetExistingEntryOffsetBefore(lookbehind_amount);

  return std::string(" Closest entry is at: d ")
      .append(helper::Numeric::ToString(available_lookbehind_offset))
      .append(" (")
      .append(helper::DateTime::GetCurrentTimeFormatted("%Y/%m/%d", available_lookbehind_offset))
      .append(")");
}

std::string ReportRendererCli::GetActiveScopeName() {
  switch (render_scope_) {
    case RenderScopes::Scope_Day:return std::string("day");
    case RenderScopes::Scope_Week:return std::string("week");
    case RenderScopes::Scope_Month:return std::string("month");
    case RenderScopes::Scope_Invalid:
    default:return std::string("");
  }
}

void ReportRendererCli::PrintHeader(
    bool display_id,
    bool display_day_sum,
    bool display_saldo) {
  std::cout << "\n" << theme_style_header_;

  if (display_id
      && offset_id_column_==0)
    PrintHeaderCellForId(true);
  else
    std::cout << " ";

  int content_len;
  // Skip meta-column (start from index 1 instead of 0)
  for (int index_column = 1; index_column < amount_columns_; index_column++) {
    if (
        (display_day_sum
            || index_column!=Report::ColumnIndexes::Index_SumDay)
            && (display_saldo
                || index_column!=Report::ColumnIndexes::Index_Balance)
        ) {
      if (offset_id_column_==0
          || index_column > 1)
        std::cout << "| ";

      std::string column_title = column_titles_[index_column];
      std::cout << helper::Html::Decode(column_title) << " ";

      content_len = helper::String::GetAmountChars(helper::Html::Decode(column_title));
      if (content_len < column_content_max_len_[index_column]) {
        int column_len_diff = column_content_max_len_[index_column] - content_len;
        std::cout << std::string(static_cast<unsigned long>(column_len_diff), ' ');
      }
    }

    if (display_id
        && offset_id_column_==index_column)
      PrintHeaderCellForId(false);
  }

  std::cout << " " << helper::Tui::kAnsiFormatReset << "\n";
}

void ReportRendererCli::PrintHeaderCellForId(bool is_left_most) {
  if (!is_left_most) std::cout << "|";

  unsigned long amount_spaces = max_index_digits_ - 1;

  if (amount_spaces > 0) std::cout << std::string((amount_spaces), ' ');

  std::cout << " ID ";
}

/**
 * Output <tr>s using given filters, returns amount of rows printed
 */
int ReportRendererCli::PrintRows(
    int task_number,
    std::string comment,
    bool display_sum,
    bool display_id,
    bool dispay_day_sum,
    bool display_balance) {
  std::string task_number_str =
      task_number==-1
      ? ""
      : helper::Numeric::ToString(task_number);

  // Pre-render grid line between content of two rows
  std::string separation_row = RenderSeparationRow();

  bool has_comment = !comment.empty();
  int index_cell = 0;
  bool is_even = true;
  amount_cells_ = static_cast<int>(cells_.size());

  std::string comment_in_row;
  std::string task_number_in_row;
  std::string date_in_row;
  std::string duration_in_row;
  std::string previous_day;
  bool is_entry_running;

  bool display_viewed_sum =
      display_sum
          && (RenderScopes::Scope_All!=render_scope_ || -1!=task_number);

  int sum_task_minutes = 0;
  int amount_rows_printed = 0;

  bool is_around_break = false;
  for (int index_row = 0; index_row < amount_rows_ && index_cell < amount_cells_; index_row++) {
    comment_in_row = cells_[index_cell + ReportParser::ColumnIndexes::Index_Comment];
    date_in_row = cells_[index_cell + ReportParser::ColumnIndexes::Index_Date];
    duration_in_row = cells_[index_cell + ReportParser::ColumnIndexes::Index_Duration];
    task_number_in_row = cells_[index_cell + ReportParser::ColumnIndexes::Index_Week + 5];

    is_entry_running = 's'==cells_[index_cell + ReportParser::ColumnIndexes::Index_Meta][0];

    bool do_display =
        (task_number==-1
            || 0==std::strcmp(task_number_in_row.c_str(), task_number_str.c_str()))
            && (!has_comment
                || helper::String::Contains(comment_in_row, comment));

    if (do_display) {
      ++amount_rows_printed;

      // New day: preceded by separation row
      if (index_cell > 0
          && previous_day!=cells_[index_cell + 2])
        std::cout << separation_row << "\n";

      if (display_viewed_sum)
        sum_task_minutes = AddSumMinutes(
            index_cell,
            duration_in_row,
            is_entry_running,
            sum_task_minutes);

      if (display_id
          && offset_id_column_==0)
        PrintRowCellForId(true, index_row + id_first_row_rendered_);

      is_even = !is_even;
    }

    // Skip meta-column (start from index 1 instead of 0)
    for (int index_column = 0;
         index_column < amount_columns_ && index_cell < amount_cells_;
         index_column++) {
      if (index_column==2) previous_day = cells_[index_cell];

      if (do_display
          && (dispay_day_sum || index_column!=Report::ColumnIndexes::Index_SumDay)
          && (display_balance || index_column!=Report::ColumnIndexes::Index_Balance)
          ) {
        // Emphasize times around e.g. lunch-break (end-time before and start-time after)
        if (index_column==Index_End)
          is_around_break = IsEndTimeBeforeBreak(index_cell);

        bool is_emphasizable_column = index_column==Index_End || index_column==Index_Start;

        PrintColumn(
            index_cell,
            is_even,
            index_row,
            index_column,
            is_emphasizable_column && is_around_break,
            display_id);
      }

      // Skip meta cell
      ++index_cell;
    }

    if (do_display) {
      std::cout << " " << helper::Tui::kAnsiFormatReset << "\n";

      if (is_even) std::cout << helper::Tui::kAnsiFormatReset;
    }
  }

  if (display_viewed_sum
      && 0 < sum_task_minutes)
    PrintDurationSums(task_number, sum_task_minutes);

  std::cout << helper::Tui::kAnsiFormatReset;

  return amount_rows_printed;
}

/**
 * Check end-time at given cell: is there an unmergeable gap (lunch- or other break) between the start-time of the next entry?
 */
bool ReportRendererCli::IsEndTimeBeforeBreak(int index_cell) {
  // Last entry cannot end before a break
  if (amount_cells_ < index_cell + 11) return false;

  // Last entry of day is not considered to be ending before a break
  if (amount_cells_ > index_cell + 10
      && cells_[index_cell - 2]!=cells_[index_cell + 10])
    return false;

  int minutes_end_current = helper::DateTime::GetSumMinutesFromTime(cells_[index_cell]);
  int minutes_start_next = helper::DateTime::GetSumMinutesFromTime(cells_[index_cell + 11]);

  return minutes_start_next - minutes_end_current > minutes_break_ + 1;
}

int ReportRendererCli::AddSumMinutes(
    int index_cell,
    const std::string &duration_in_row,
    bool is_entry_running,
    int sum_task_minutes) const {
  if (is_entry_running) {
    int minutes_start = helper::DateTime::GetSumMinutesFromTime(cells_[index_cell + Index_Start]);
    int minutes_now = helper::DateTime::GetSumMinutesFromTime();
    sum_task_minutes += minutes_now - minutes_start;
  } else if (!duration_in_row.empty())
    sum_task_minutes += helper::DateTime::GetSumMinutesFromTime(duration_in_row);

  return sum_task_minutes;
}

void ReportRendererCli::PrintColumn(
    int index_cell,
    bool is_even,
    int index_row,
    int index_column,
    bool emphasize,
    bool display_id) {
  if (index_column > 1) {
    // Skip column 0 (meta)
    std::cout << (is_even
                  ? theme_style_default_
                  : theme_style_grid_)
              << "| " << helper::Tui::kAnsiFormatReset;
  } else if (index_column==1
      && offset_id_column_ > 0)
    std::cout << " ";

  if (index_column > 0) {
    std::string content = cells_[index_cell];

    if (index_column==Index_Comment) content = helper::Html::Decode(content);

    std::cout << (is_even ? theme_style_default_ : "")
              << (emphasize ? helper::Tui::kAnsiFormatInverted : "")
              << content
              << (emphasize ? helper::Tui::kAnsiFormatReset : "")
              << " ";
  }

  PrintRhsCellSpaces(index_cell, index_column);

  if (display_id
      && offset_id_column_==index_column
      && index_column > 0
      )
    PrintRowCellForId(false, index_row + id_first_row_rendered_);
}

void ReportRendererCli::PrintDurationSums(int task_number, int sum_task_minutes) {
  std::string sum_duration_formatted =
      helper::DateTime::GetHoursFormattedFromMinutes(sum_task_minutes);

  std::cout << "    ";

  for (int index_column = 0; index_column < Index_Duration; index_column++) {
    PrintRhsCellSpaces(-1, index_column);
    std::cout << " ";
  }

  std::cout << "  Σ";

  if (-1!=task_number) std::cout << " " << task_number;

  std::cout << ": " << sum_duration_formatted << "\n";
}

/**
 * Render separator row (printed between days)
 */
std::string ReportRendererCli::RenderSeparationRow() {
  std::string separation_row = theme_style_grid_ + std::string(4 + max_index_digits_, '-');
  for (int indexColumn = 1; indexColumn < amount_columns_; indexColumn++)
    separation_row
        .append(std::string(column_content_max_len_[indexColumn] + 3, '-'));

  return separation_row + helper::Tui::kAnsiFormatReset;
}

void ReportRendererCli::PrintRowCellForId(bool is_left_most, int index_row) {
  if (!is_left_most) std::cout << '|';

  std::cout << ' ';

  unsigned long amountDigitsInCurrentRowIndex = helper::Numeric::GetAmountDigits(index_row);
  unsigned long amountSpaces = max_index_digits_ - amountDigitsInCurrentRowIndex;

  if (amountSpaces > 0) std::cout << std::string(amountSpaces, ' ');

  std::cout << ' ' << index_row << ' ';

  if (is_left_most) std::cout << "| ";

  std::cout << theme_style_grid_;
}

/**
 * Fill cell w/ spaces to keep width of cells in column identical
 */
void ReportRendererCli::PrintRhsCellSpaces(int index_cell, int index_column) {
  int content_len = -1==index_cell
                    ? 0
                    : helper::String::GetAmountChars(helper::Html::Decode(cells_[index_cell]));

  if (content_len < column_content_max_len_[index_column]) {
    int column_len_diff = column_content_max_len_[index_column] - content_len;
    std::cout << std::string(static_cast<unsigned long>(column_len_diff), ' ');
  }
}

/**
 * Initialize color/formatting theme style codes
 */
void ReportRendererCli::InitAnsiTheme() {
  AppConfig config = AppConfig::GetInstance();
  int theme_id = helper::String::ToInt(config.GetConfigValue("cli_theme").c_str(), 0);

  switch (theme_id) {
    case THEME_MAC_TERMINAL_DRACULA:theme_style_header_ = "\033[0;100m";
      theme_style_default_ = "\033[17;36m";
      theme_style_grid_ = helper::Tui::kAnsiFormatReset;
      break;
    case THEME_LINUX_BASH_DRACULA:theme_style_header_ = "\033[1;40m";
      theme_style_default_ = "\033[17;34m";
      theme_style_grid_ = helper::Tui::kAnsiFormatReset;
      break;
    case THEME_LINUX_BASH_ALTERNATIVE:theme_style_header_ = "\033[1;44m";
      theme_style_default_ = "\033[17;36m";
      theme_style_grid_ = helper::Tui::kAnsiFormatReset;
      break;
    case THEME_LINUX_BASH_DEFAULT:theme_style_header_ = "\033[17;104m";
      theme_style_default_ = "\033[17;34m";
      theme_style_grid_ = helper::Tui::kAnsiFormatReset;
      break;
    case THEME_MAC_TERMINAL_VISOR:
    default:theme_style_header_ = "\033[1;44m";
      theme_style_default_ = "\033[17;100m";
      theme_style_grid_ = helper::Tui::kAnsiFormatReset;
  }
}

} // namespace tictac_trac

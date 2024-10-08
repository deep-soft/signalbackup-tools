/*
  Copyright (C) 2021-2024  Selwin van Dijk

  This file is part of signalbackup-tools.

  signalbackup-tools is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  signalbackup-tools is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with signalbackup-tools.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "signalbackup.ih"

//#include <chrono>

bool SignalBackup::reorderMmsSmsIds() const
{
  //auto t1 = std::chrono::high_resolution_clock::now();
  Logger::message_start(__FUNCTION__);

  // get all mms in the correct order
  SqliteDB::QueryResults res;
  if (!d_database.exec("SELECT _id FROM " + d_mms_table + " ORDER BY date_received ASC", &res)) // for sms table, use 'date'
    return false;

  d_database.exec("BEGIN TRANSACTION");

  // set all id's 'negatively ascending' (negative because of UNIQUE constraint)
  long long int negative_id_tmp = 0;
  long long int total = res.rows();

  bool adjustmention = d_database.containsTable("mention");
  bool adjustmsl_message = d_database.containsTable("msl_message");
  bool msl_has_is_mms = adjustmsl_message && d_database.tableContainsColumn("msl_message", "is_mms");
  bool adjustreaction = d_database.containsTable("reaction");
  bool reaction_has_is_mms = adjustreaction && d_database.tableContainsColumn("reaction", "is_mms");
  bool adjuststorysends = d_database.containsTable("story_sends");
  bool adjustcall = d_database.containsTable("call");
  bool adjustoriginal_message_id = d_database.tableContainsColumn(d_mms_table, "original_message_id");
  bool adjustlatest_revision_id = d_database.tableContainsColumn(d_mms_table, "latest_revision_id");

  // we purposefully do this in a dozen or so separate for-loops so SqliteDB can re-use its prepared statements!
  for (unsigned int i = 0; i < total; ++i)
  {
    std::any oldid = res.value(i, 0);
    ++negative_id_tmp;
    if (!d_database.exec("UPDATE " + d_mms_table + " SET _id = ? WHERE _id = ?", {-1 * negative_id_tmp, oldid})) [[unlikely]]
      return false;
  }
  negative_id_tmp = 0;
  Logger::message_continue(".");

  for (unsigned int i = 0; i < total; ++i)
  {
    std::any oldid = res.value(i, 0);
    ++negative_id_tmp;
    if (!d_database.exec("UPDATE  " + d_part_table + " SET  " + d_part_mid + " = ? WHERE " + d_part_mid + " = ?", {-1 * negative_id_tmp, oldid})) [[unlikely]]
      return false;
  }
  negative_id_tmp = 0;

  for (unsigned int i = 0; i < total; ++i)
  {
    std::any oldid = res.value(i, 0);
    ++negative_id_tmp;
    if (!d_database.exec("UPDATE group_receipts SET mms_id = ? WHERE mms_id = ?", {-1 * negative_id_tmp, oldid})) [[unlikely]]
      return false;
  }
  negative_id_tmp = 0;

  if (adjustmention)
    for (unsigned int i = 0; i < total; ++i)
    {
      std::any oldid = res.value(i, 0);
      ++negative_id_tmp;
      if (!d_database.exec("UPDATE mention SET message_id = ? WHERE message_id = ?", {-1 * negative_id_tmp, oldid})) [[unlikely]]
        return false;
    }
  negative_id_tmp = 0;

  if (adjustmsl_message)
    for (unsigned int i = 0; i < total; ++i)
    {
      std::any oldid = res.value(i, 0);
      ++negative_id_tmp;
      if (!d_database.exec("UPDATE msl_message SET message_id = ? WHERE message_id = ?"s + (msl_has_is_mms ? " AND is_mms IS 1" : ""), {-1 * negative_id_tmp, oldid})) [[unlikely]]
        return false;
    }
  negative_id_tmp = 0;

  if (adjustreaction)
    for (unsigned int i = 0; i < total; ++i)
    {
      std::any oldid = res.value(i, 0);
      ++negative_id_tmp;
      if (!d_database.exec("UPDATE reaction SET message_id = ? WHERE message_id = ?"s + (reaction_has_is_mms ? " AND is_mms IS 1" : ""), {-1 * negative_id_tmp, oldid})) [[unlikely]]
        return false;
    }
  negative_id_tmp = 0;

  if (adjuststorysends)
    for (unsigned int i = 0; i < total; ++i)
    {
      std::any oldid = res.value(i, 0);
      ++negative_id_tmp;
      if (!d_database.exec("UPDATE story_sends SET message_id = ? WHERE message_id = ?", {-1 * negative_id_tmp, oldid})) [[unlikely]]
        return false;
    }
  negative_id_tmp = 0;

  if (adjustcall)
    for (unsigned int i = 0; i < total; ++i)
    {
      std::any oldid = res.value(i, 0);
      ++negative_id_tmp;
      if (!d_database.exec("UPDATE call SET message_id = ? WHERE message_id = ?", {-1 * negative_id_tmp, oldid})) [[unlikely]]
        return false;
    }
  negative_id_tmp = 0;

  if (adjustoriginal_message_id)
    for (unsigned int i = 0; i < total; ++i)
    {
      std::any oldid = res.value(i, 0);
      ++negative_id_tmp;
      if (!d_database.exec("UPDATE " + d_mms_table + " SET original_message_id = ? WHERE original_message_id = ?", {-1 * negative_id_tmp, oldid})) [[unlikely]]
        return false;
    }
  negative_id_tmp = 0;

  Logger::message_continue(".");

  if (adjustlatest_revision_id)
    for (unsigned int i = 0; i < total; ++i)
    {
      std::any oldid = res.value(i, 0);
      ++negative_id_tmp;
      if (!d_database.exec("UPDATE " + d_mms_table + " SET latest_revision_id = ? WHERE latest_revision_id = ?", {-1 * negative_id_tmp, oldid})) [[unlikely]]
        return false;
    }

  /*
  for (unsigned int i = 0; i < total; ++i)
  {
    if (d_showprogress && i % 1000 == 0)
      Logger::message_overwrite(__FUNCTION__, " (", i, "/", total, ")");

    std::any oldid = res.value(i, 0);
    ++negative_id_tmp;
    if (!d_database.exec("UPDATE " + d_mms_table + " SET _id = ? WHERE _id = ?", {-1 * negative_id_tmp, oldid}) ||
        !d_database.exec("UPDATE  " + d_part_table + " SET  " + d_part_mid + " = ? WHERE " + d_part_mid + " = ?", {-1 * negative_id_tmp, oldid}) ||
        !d_database.exec("UPDATE group_receipts SET mms_id = ? WHERE mms_id = ?", {-1 * negative_id_tmp, oldid}))
      return false;
    if (adjustmention && !d_database.exec("UPDATE mention SET message_id = ? WHERE message_id = ?", {-1 * negative_id_tmp, oldid}))
      return false;
    if (adjustmsl_message &&
        !d_database.exec("UPDATE msl_message SET message_id = ? WHERE message_id = ?"s + (msl_has_is_mms ? " AND is_mms IS 1" : ""), {-1 * negative_id_tmp, oldid}))
      return false;
    if (adjustreaction && !d_database.exec("UPDATE reaction SET message_id = ? WHERE message_id = ?"s + (reaction_has_is_mms ? " AND is_mms IS 1" : ""), {-1 * negative_id_tmp, oldid}))
      return false;
    if (adjuststorysends && !d_database.exec("UPDATE story_sends SET message_id = ? WHERE message_id = ?", {-1 * negative_id_tmp, oldid}))
      return false;
    if (adjustcall && !d_database.exec("UPDATE call SET message_id = ? WHERE message_id = ?", {-1 * negative_id_tmp, oldid}))
      return false;
    if (adjustoriginal_message_id && !d_database.exec("UPDATE " + d_mms_table + " SET original_message_id = ? WHERE original_message_id = ?", {-1 * negative_id_tmp, oldid}))
      return false;
    if (adjustlatest_revision_id && !d_database.exec("UPDATE " + d_mms_table + " SET latest_revision_id = ? WHERE latest_revision_id = ?", {-1 * negative_id_tmp, oldid}))
      return false;
  }
  */

  d_database.exec("COMMIT");

  // now make all id's positive again
  d_database.exec("BEGIN TRANSACTION");

  Logger::message_continue(".");

  if (!d_database.exec("UPDATE " + d_mms_table + " SET _id = _id * -1 WHERE _id < 0")) [[unlikely]]
    return false;

  if (!d_database.exec("UPDATE " + d_part_table + " SET " + d_part_mid + " = " + d_part_mid + " * -1 WHERE " + d_part_mid + " < 0")) [[unlikely]]
    return false;

  if(!d_database.exec("UPDATE group_receipts SET mms_id = mms_id * -1 WHERE mms_id < 0")) [[unlikely]]
    return false;

  if (adjustmention && !d_database.exec("UPDATE mention SET message_id = message_id * -1 WHERE message_id < 0")) [[unlikely]]
    return false;
  if (adjustmsl_message && !d_database.exec("UPDATE msl_message SET message_id = message_id * -1 WHERE message_id < 0"s + (msl_has_is_mms ? " AND is_mms IS 1" : ""))) [[unlikely]]
    return false;
  if (adjustreaction && !d_database.exec("UPDATE reaction SET message_id = message_id * -1 WHERE message_id < 0"s + (reaction_has_is_mms ? " AND is_mms IS 1" : ""))) [[unlikely]]
    return false;
  if (adjuststorysends && !d_database.exec("UPDATE story_sends SET message_id = message_id * -1 WHERE message_id < 0")) [[unlikely]]
    return false;
  if (adjustcall && !d_database.exec("UPDATE call SET message_id = message_id * -1 WHERE message_id < 0")) [[unlikely]]
    return false;
  if (adjustoriginal_message_id && !d_database.exec("UPDATE " + d_mms_table + " SET original_message_id = original_message_id * -1 WHERE original_message_id < 0")) [[unlikely]]
    return false;
  if (adjustlatest_revision_id && !d_database.exec("UPDATE " + d_mms_table + " SET latest_revision_id = latest_revision_id * -1 WHERE latest_revision_id < 0")) [[unlikely]]
    return false;

  d_database.exec("COMMIT");

  // SAME FOR SMS
  if (d_database.containsTable("sms")) // removed in 168
  {
    if (!d_database.exec("SELECT _id FROM sms ORDER BY " + d_sms_date_received + " ASC", &res))
      return false;

    negative_id_tmp = 0;
    for (unsigned int i = 0; i < res.rows(); ++i)
    {
      long long int oldid = res.getValueAs<long long int>(i, 0);
    ++negative_id_tmp;
    if (!d_database.exec("UPDATE sms SET _id = ? WHERE _id = ?", {-1 * negative_id_tmp, oldid}))
      return false;
    if (d_database.containsTable("msl_message"))
      if (!d_database.exec("UPDATE msl_message SET message_id = ? WHERE message_id = ?"s + (d_database.tableContainsColumn("msl_message", "is_mms") ? " AND is_mms IS NOT 1" : ""), {-1 * negative_id_tmp, oldid}))
        return false;
    if (d_database.containsTable("reaction")) // dbv >= 121
      if (!d_database.exec("UPDATE reaction SET message_id = ? WHERE message_id = ?"s + (d_database.tableContainsColumn("reaction", "is_mms") ? " AND is_mms IS NOT 1" : ""), {-1 * negative_id_tmp, oldid}))
        return false;
    }

    if (!d_database.exec("UPDATE sms SET _id = _id * -1 WHERE _id < 0"))
      return false;
    if (d_database.containsTable("msl_message"))
      if (!d_database.exec("UPDATE msl_message SET message_id = message_id * -1 WHERE message_id < 0"s + (d_database.tableContainsColumn("msl_message", "is_mms") ? " AND is_mms IS NOT 1" : "")))
        return false;
    if (d_database.containsTable("reaction")) // dbv >= 121
      if (!d_database.exec("UPDATE reaction SET message_id = message_id * -1 WHERE message_id < 0"s + (d_database.tableContainsColumn("reaction", "is_mms") ? " AND is_mms IS NOT 1" : "")))
        return false;
  }

  Logger::message_end("ok");
  // auto t2 = std::chrono::high_resolution_clock::now();
  // auto ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
  // std::cout << " *** TIME: " << ms_int.count() << "ms\n";
  return true;
}

/*
  Copyright (C) 2023-2024  Selwin van Dijk

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

/*
  some known issues:

  - Overlapping text styles are not exported properly by telegram
  - Message delivery info might not be available in json
  - Message types other than 'message' (eg 'service') are currently skipped
  - underline style is not supported by signal
  - stickers turn into normal attachments?
  - 'forwarded from' is not an existing attribute in signal
  - message reaction are not exported by Telegram
  - poll-attachments skipped

 */

#include "signalbackup.ih"

#include "../jsondatabase/jsondatabase.h"

bool SignalBackup::importTelegramJson(std::string const &file, std::vector<long long int> const &chatselection,
                                      std::vector<std::pair<std::string, long long int>> contactmap,
                                      std::vector<std::string> const &inhibitmapping, bool prependforwarded,
                                      bool markdelivered, bool markread, std::string const &selfphone)
{
  Logger::message("Import from Telegram json export");

  if (bepaald::isDir(file))
  {
    Logger::error("Did not get regular file as input");
    return false;
  }

  // check and warn about selfid
  d_selfid = selfphone.empty() ? scanSelf() : d_database.getSingleResultAs<long long int>("SELECT _id FROM recipient WHERE " + d_recipient_e164 + " = ?", selfphone, -1);
  if (d_selfid == -1)
  {
    Logger::error("Failed to determine id of 'self'.",
                  (selfphone.empty() ? "Please pass `--setselfid \"[phone]\"' to set it manually" : ""));
    return false;
  }

  // set selfuuid
  d_selfuuid = bepaald::toLower(d_database.getSingleResultAs<std::string>("SELECT " + d_recipient_aci + " FROM recipient WHERE _id = ?", d_selfid, std::string()));

  JsonDatabase jsondb(file, d_verbose);
  if (!jsondb.ok())
    return false;

  // get base path of file (we need it to set the resolve relative paths
  // referenced in the JSON data
  std::string datapath;
  std::filesystem::path p(file);
  datapath = p.parent_path().string() + static_cast<char>(std::filesystem::path::preferred_separator);

  // build chatlist:
  std::string chatlist;
  for (auto idx : chatselection)
    chatlist += (chatlist.empty() ? "(" : ", ") + bepaald::toString(idx);
  chatlist += (chatlist.empty() ? "" : ")");

  // make sure all json-contacts are mapped to Signal contacts
  std::vector<std::pair<std::vector<std::string>, long long int>> finalcontactmap;
  for (uint i = 0; i < contactmap.size(); ++i)
    finalcontactmap.push_back({{contactmap[i].first}, contactmap[i].second});
  if (!tgMapContacts(jsondb, chatlist, &finalcontactmap, inhibitmapping))
    return false;

  SqliteDB::QueryResults chats;
  if (!jsondb.d_database.exec("SELECT idx, name, id, type FROM chats" + (chatlist.empty() ? "" : (" WHERE idx IN " + chatlist)), &chats))
    return false;

  // for each chat, get the messages and insert
  for (uint i = 0; i < chats.rows(); ++i)
  {
    Logger::message("Dealing with conversation ", i + 1, "/", chats.rows());

    if (chats.valueAsString(i, "type") == "private_group" /*|| chats.valueAsString(i, "type") == "some_other_group"*/)
      tgImportMessages(jsondb.d_database, finalcontactmap, datapath, chats.valueAsString(i, "id"), chats.valueAsInt(i, "idx"), prependforwarded, markdelivered, markread, true); // deal with group chat
    else if (chats.valueAsString(i, "type") == "personal_chat") // ????
      tgImportMessages(jsondb.d_database, finalcontactmap, datapath, chats.valueAsString(i, "id"), chats.valueAsInt(i, "idx"), prependforwarded, markdelivered, markread, false); // deal with 1-on-1 convo
    else
    {
      Logger::warning("Unsupported chat type `", chats.valueAsString(i, "type"), "'. Skipping...");
      continue;
    }
  }

  reorderMmsSmsIds();
  updateThreadsEntries();
  return checkDbIntegrity();
}

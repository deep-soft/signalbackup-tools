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

#include "signalbackup.ih"

#include <cerrno>

bool SignalBackup::HTMLwriteAttachment(std::string const &directory, std::string const &threaddir,
                                       long long int rowid, long long int uniqueid, //std::string const &ext,
                                       std::string const &attachment_filename,
                                       bool overwrite, bool append) const
{
  if (!bepaald::contains(d_attachments, std::pair{rowid, uniqueid}))
    return false;

  // directory + threaddir is guaranteed to exist at this point, check/create 'media'
  if (!bepaald::fileOrDirExists(directory + "/" + threaddir + "/media"))
  {
    if (!bepaald::createDir(directory + "/" + threaddir + "/media"))
    {
      Logger::error("Failed to create directory `", directory, "/", threaddir, "/media");
      return false;
    }
  }
  else if (!bepaald::isDir(directory + "/" + threaddir + "/media"))
  {
    Logger::error("Failed to create directory `", directory, "/", threaddir, "/media");
    return false;
  }

  // check actual attachmentfile file
  std::string attachment_filename_full = directory + "/" + threaddir + "/media/" +
    attachment_filename;
  // "/media/Attachment_" + bepaald::toString(rowid) + "_" + bepaald::toString(uniqueid) + "." + ext;

  if (bepaald::fileOrDirExists(attachment_filename_full))
  {
    if (append) // file already exists, but we were asked to just use the existing file, so we're done
      return true;
    if (!overwrite) // file already exists, but we were no asked to overwrite -> error!
    {

      std::cout << attachment_filename << std::endl;

      Logger::error("Attachment file exists. Not overwriting");
      return false;
    }
  }

  AttachmentFrame *a = d_attachments.at({rowid, uniqueid}).get();

  // write actual attachment:
  std::ofstream attachmentstream(attachment_filename_full, std::ios_base::binary);
  if (!attachmentstream.is_open())
  {
    Logger::error("Failed to open file for writing: '", attachment_filename_full, "'",
                  " (errno: ", std::strerror(errno), ")"); // note: errno is not required to be set by std
    // temporary !!
    {
      std::error_code error;
      std::filesystem::space_info const si = std::filesystem::space(directory, error);
      if (!error)
      {
        Logger::message("Space available: ", static_cast<std::intmax_t>(si.available),
                        "\nAttachment size: ", a->attachmentSize());
      }
    }
    return false;
  }
  else
  {
    if (!attachmentstream.write(reinterpret_cast<char *>(a->attachmentData()), a->attachmentSize()))
      return false;
    // write was succesfull. drop attachment data
    a->clearData();
  }
  return true;
}

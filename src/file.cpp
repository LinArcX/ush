#include "file.h"
#include <fstream>

uint32_t ush::File::m_inDirHistoryLastIndexVisited = 0U;

uint32_t ush::File::m_inCommandHistoryLastIndexVisited = 0U;

std::vector<std::string> ush::File::m_dirsHistory = {};

std::vector<std::string> ush::File::m_commandsHistory = {};
 

bool ush::File::readFile(const std::filesystem::path& path,
  std::vector<std::string>& vec)
{
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return false;
  }

  std::string line;
  while (std::getline(file, line)) {
    vec.push_back(std::move(line));
  }

  return !file.bad(); // true unless an I/O error occurred
}

ush::Error ush::File::readCommandHistory()
{
  std::filesystem::path path 
    = std::filesystem::path(std::getenv("HOME"))
                                       / ".config"
                                       / "ush"
                                       / "history"
                                       / "commands";
  if(true == readFile(path, m_commandsHistory)) {
    std::erase(m_commandsHistory, "");
    m_inCommandHistoryLastIndexVisited = m_commandsHistory.size();
    return Error::eSuccess;
  }
  return Error::eError;
}

ush::Error ush::File::readDirectoryHistory()
{
  std::filesystem::path path 
    = std::filesystem::path(std::getenv("HOME"))
                                       / ".config"
                                       / "ush"
                                       / "history"
                                       / "dirs";
  if (true == readFile(path, m_dirsHistory)) {
    std::erase(m_dirsHistory, "");
    m_inDirHistoryLastIndexVisited = m_dirsHistory.size() - 1;
    return Error::eSuccess;
  }
  return Error::eError;
}

bool ush::File::saveFile(std::filesystem::path path,
  std::string_view file,
  std::string_view text)
{
    // Create parent directories if needed.
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    if (ec) {
        return false;
    }

    std::filesystem::path fullPath = path / file;

    std::ofstream osfile(fullPath, std::ios::binary | std::ios::app);

    if (!osfile) {
        return false;
    }

    osfile.write(text.data(), static_cast<std::streamsize>(text.size()));
    osfile.put('\n');

    return osfile.good();
}

ush::Error ush::File::saveCommandHistory(std::string str)
{
  std::filesystem::path dir 
    = std::filesystem::path(std::getenv("HOME")) 
                                       / ".config" 
                                       / "ush"
                                       / "history";
  if (true == saveFile(dir, "commands", str)) {
    return Error::eSuccess;
  }
  return Error::eError;
}

ush::Error ush::File::saveDirectoryHistory(std::string str)
{
  std::filesystem::path dir = 
    std::filesystem::path(std::getenv("HOME")) 
                                     / ".config" 
                                     / "ush"
                                     / "history";
  if (true == saveFile(dir, "dirs", str)) {
    return Error::eSuccess;
  }
  return Error::eError;
}

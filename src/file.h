#ifndef USH_FILE_H
#define USH_FILE_H

#include "error.h"

#include <vector>
#include <filesystem>

namespace ush
{
  class File
  {
    public:
      static Error saveCommandHistory(std::string str);

      static Error saveDirectoryHistory(std::string str);

      static Error readCommandHistory();

      static Error readDirectoryHistory();

	    static uint32_t m_inDirHistoryLastIndexVisited;

	    static uint32_t m_inCommandHistoryLastIndexVisited;

      static std::vector<std::string> m_dirsHistory;

      static std::vector<std::string> m_commandsHistory;
    private:
      static bool saveFile(std::filesystem::path path,
          std::string_view file,
          std::string_view text);
 
      static bool readFile(const std::filesystem::path& path,
        std::vector<std::string>& vec);
 
  };
};
 
#endif // USH_FILE_H

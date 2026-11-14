// Included inside udm by the Options UI; not a Core settings dependency.
// File checks belong to the accepted-save worker, never draft collection.
inline std::vector<std::string> optionsSoundFiles(const Json& prefs){std::vector<std::string> paths;std::set<std::string> seen;for(const auto key:{"CompletionSoundFile","FailureSoundFile","QueueStartSoundFile","QueueStopSoundFile","PauseSoundFile"}){auto path=str(prefs,key);if(!path.empty()&&seen.insert(path).second)paths.push_back(std::move(path));}return paths;}
inline void validateOptionsSoundFiles(const std::vector<std::string>& paths){for(const auto& path:paths)if(!fs::is_regular_file(fs::path(wide(path))))throw std::runtime_error("Choose an existing sound file.");}

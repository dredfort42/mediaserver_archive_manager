#include "configMap.hpp"

ConfigMap::ConfigMap(int argc, char *argv[])
{
    _configFile = "./config.ini";
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg.find("--config=") != std::string::npos)
        {
            _configFile = arg.substr(arg.find('=') + 1);
            break;
        }
    }

    if (_readConfigMap(_configFile))
    {
        print(LogType::ERROR, "Failed to read configuration files");
        exit(1);
    }

    if (_map["debug"] == "true" || _map["debug"] == "1")
    {
        setDebug(true);
        print(LogType::DEBUGER, "Log level set to DEBUG");
    }

    print(LogType::DEBUGER, "Successfully read configuration files");
}

ConfigMap::ConfigMap(const std::string &configFile)
{
    if (_readConfigMap(configFile))
        exit(1);
}

ConfigMap::~ConfigMap()
{
}

std::string ConfigMap::getConfigFile()
{
    return _configFile;
}

int ConfigMap::_readConfigMap(const std::string &configFile)
{
    std::ifstream file(configFile);
    std::string line;
    std::string key;
    std::string value;
    std::string section;

    if (!file.is_open())
    {
        print(LogType::WARNING, "Failed to open file: " + configFile);
        return 1;
    }

    while (std::getline(file, line))
    {
        line = _removeComments(line);
        if (line.empty())
            continue;

        if (line[0] == '[' && line[line.size() - 1] == ']')
        {
            section = _trim(line.substr(1, line.size() - 2));
            continue;
        }

        if (line.find('=') == std::string::npos)
            continue;

        if (section.empty())
            key = _trim(line.substr(0, line.find('=')));
        else
            key = section + "." + _trim(line.substr(0, line.find('=')));
        value = _trim(line.substr(line.find('=') + 1));

        if (key.empty() || value.empty())
            continue;

        if (value.find('#') != std::string::npos)
            value = _trim(value.substr(0, value.find('#')));

        if (value.empty())
            continue;

        if (value.find(';') != std::string::npos)
            value = _trim(value.substr(0, value.find(';')));

        if (value.empty())
            continue;

        _map[key] = _removeQuotes(value);
    }

    file.close();

    print(LogType::DEBUGER, "Successfully read configuration from file: " + configFile);

    return 0;
}

std::string ConfigMap::getProperty(std::string key)
{
    return _map[_trim(key)];
}

void ConfigMap::setProperty(std::string key, std::string value)
{
    _map[_trim(key)] = _removeQuotes(_trim(value));
}

void ConfigMap::printConfiguration()
{
    for (auto it = _map.begin(); it != _map.end(); it++)
        print(it->first + " = " + it->second);
}

std::string ConfigMap::_trim(const std::string &s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && isspace(*it))
        it++;

    std::string::const_reverse_iterator rit = s.rbegin();
    while (rit.base() != it && isspace(*rit))
        rit++;

    return std::string(it, rit.base());
}

std::string ConfigMap::_removeQuotes(const std::string &s)
{
    std::string result = s;
    if (result.size() >= 2 && ((result.front() == '"' && result.back() == '"') || (result.front() == '\'' && result.back() == '\'')))
        result = result.substr(1, result.size() - 2);
    return result;
}

std::string ConfigMap::_removeComments(const std::string &line)
{
    std::string result = line;
    size_t pos = result.find('#');
    if (pos != std::string::npos)
        result = result.substr(0, pos);
    pos = result.find(';');
    if (pos != std::string::npos)
        result = result.substr(0, pos);
    return _trim(result);
}
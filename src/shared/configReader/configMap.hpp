#ifndef CONFIG_READER_HPP
#define CONFIG_READER_HPP

#include <string>
#include <map>
#include <fstream>
#include "logPrinter.hpp"

/*
 * @brief Class to read configuration files
 */
class ConfigMap
{
private:
    std::map<std::string, std::string> _map;
    std::string _configFile;

    ConfigMap(const ConfigMap &other);

    /*
     * @brief Read a configuration file
     * @param configFile Path to the configuration file
     * @return 0 if success, 1 if error
     */
    int _readConfigMap(const std::string &configFile);

    /*
     * @brief Trim from start and the end
     * @param s String to trim
     * @return Trimmed string
     */
    std::string _trim(const std::string &s);

    /*
     * @brief Remove quotes from a string
     * @param s String to remove quotes from
     * @return String without quotes
     */
    std::string _removeQuotes(const std::string &s);

    /*
     * @brief Remove comments from a line
     * @param line Line to remove comments from
     * @return Line without comments
     */
    std::string _removeComments(const std::string &line);

public:
    /*
     * @brief Constructor for reading path to configuration file from command line arguments in --config parameter (--config=/path/to/config.ini) if not found, reads default project configuration file located at ./config.ini
     * @param argc Number of arguments
     * @param argv Arguments
     */
    ConfigMap(int argc, char *argv[]);

    /*
     * @brief Constructor
     * @param configFile Path to the configuration file
     */
    ConfigMap(const std::string &configFile);

    ~ConfigMap();

    /*
     * @brief Get the configuration file path
     * @return Configuration file path
     */
    std::string getConfigFile();

    /*
     * @brief Get a property from the configuration map
     * @param key Key of the property
     */
    std::string getProperty(std::string key);

    /*
     * @brief Set a property to the configuration map
     * @param key Key of the property
     * @param value Value of the property
     */
    void setProperty(std::string key, std::string value);

    /*
     * @brief Print the configuration map
     */
    void printConfiguration();
};

#endif // CONFIG_READER_HPP
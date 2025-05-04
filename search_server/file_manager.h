#pragma once

#include <fstream>
#include <vector>

class File_manager
{
public:
	File_manager(const std::string filename_);

	void FillConfig(
		std::string* sqlhost,
		std::string* sqlport,
		std::string* dbname,
		std::string* username,
		std::string* password,
		std::string* url,
		std::string* crawler_depth,
		std::string* http_port);
	
		std::vector<std::string> SimpleRead();
private:
	const std::string filename;
	void ReadAndSetValues(std::string& line, std::string* config_sqlhost, std::string* config_sqlport, std::string* config_dbname, std::string* config_username, std::string* config_password, std::string* config_url, std::string* config_crawler_depth, std::string* config_http_port);
	void SetValue(std::string* destination, const std::string source, const std::string key, const std::string config_property);
	
	const std::string sql_host = "host";
	const std::string sql_port = "sqlport";
	const std::string dbname = "dbname";
	const std::string username = "username";
	const std::string password = "password";
	const std::string url = "url";
	const std::string crawler_depth = "crawler_depth";
	const std::string http_port = "http_port";
};

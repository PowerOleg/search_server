#include "file_manager.h"

File_manager::File_manager(const std::string filename_) : filename{filename_}
{}

void File_manager::FillConfig(
	std::string* config_sqlhost,
	std::string* config_sqlport,
	std::string* config_dbname,
	std::string* config_username,
	std::string* config_password,
	std::string* config_url,
	std::string* config_crawler_depth,
	std::string* config_http_port)
{
	std::string line;
	std::ifstream in(filename);
	if (!in.is_open())
	{
		printf("config.ini is not opened");
		return;
	}

	while (!(in >> line).eof())
	{
		ReadAndSetValues(line, config_sqlhost, config_sqlport, config_dbname, config_username, config_password, config_url, config_crawler_depth, config_http_port);
	}
	ReadAndSetValues(line, config_sqlhost, config_sqlport, config_dbname, config_username, config_password, config_url, config_crawler_depth, config_http_port);
	in.close();
}

std::vector<std::string> File_manager::SimpleRead()
{
	std::vector<std::string> result;

	std::string line;
	std::ifstream in(filename);
	if (!in.is_open())
	{
		printf("file is not opened");
		return result;
	}

	while (!(in >> line).eof())
	{
		result.push_back(line);
	}
	in >> line;
	result.push_back(line);


	in.close();
	return result;
}

void File_manager::ReadAndSetValues(std::string& line, std::string* config_sqlhost, std::string* config_sqlport, std::string* config_dbname, std::string* config_username, std::string* config_password, std::string* config_url, std::string* config_crawler_depth, std::string* config_http_port)
{
	size_t index = line.find("=");
	std::string key = line.substr(0, index);
	std::string value = line.substr(index + 1, line.size());
	SetValue(config_sqlhost, value, key, sql_host);
	SetValue(config_sqlport, value, key, sql_port);
	SetValue(config_dbname, value, key, dbname);
	SetValue(config_username, value, key, username);
	SetValue(config_password, value, key, password);
	SetValue(config_url, value, key, url);
	SetValue(config_crawler_depth, value, key, crawler_depth);
	SetValue(config_http_port, value, key, http_port);
}

void File_manager::SetValue(std::string* destination, const std::string source, const std::string key, const std::string config_property)
{
	if (key == config_property)
	{
		*destination = source;
	}
}


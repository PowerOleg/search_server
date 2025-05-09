#pragma once
#include <pqxx/pqxx>
#include <string>
#include <vector>
#include <map>
#include <iostream>

class Postgres_manager
{
public:
	Postgres_manager(const std::string host, const std::string port, const std::string dbname, const std::string username, const std::string password);
	
	bool Write(const std::string url, size_t postgres_count, const std::map<std::string, int>& counted_words, long& word_number);

	std::vector<std::string> SelectUrls(const std::string &word, const std::string &quantity);

private:
	bool Clean();
	bool CreateTable(std::string tablename, std::string param);
	bool InitTables();
	pqxx::connection connection;
};


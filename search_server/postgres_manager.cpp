#include "postgres_manager.h"

Postgres_manager::Postgres_manager(std::string host, std::string port, std::string dbname, std::string username, std::string password) : connection{ "host=" + host + " port=" + port + " dbname =" + dbname + " user = " + username + " password =" + password}
{
	std::cout << "Connection is successful" << std::endl;
}

std::vector<std::string> Postgres_manager::SelectUrls(const std::string &word, const std::string &quantity)
{
	std::vector<std::string> result;
	try
	{
		pqxx::work tx{ connection };
		auto values = tx.query<std::string>(
			"SELECT document FROM documents_words dw " 
			 "JOIN words w ON w.id = dw.word_id " 
			 "JOIN documents d ON d.id = dw.document_id where word = '" + word + 
			"' ORDER BY quantity DESC LIMIT "+ quantity + ";"
		);
		tx.commit();

		for (const std::tuple<std::string>& value : values)
		{
			result.push_back(std::get<0>(value));
		}
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
		result.push_back("500");
	}

	return result;
}

#include "postgres_manager.h"

Postgres_manager::Postgres_manager(std::string host, std::string port, std::string dbname, std::string username, std::string password) : connection{ "host=" + host + " port=" + port + " dbname =" + dbname + " user = " + username + " password =" + password}
{
	std::cout << "Connection is successful" << std::endl;
}

std::vector<std::string> Postgres_manager::Select10Urls(const std::string& word)
{
	std::vector<std::string> result;
	try
	{
		pqxx::work tx{ connection };
		auto values = tx.query<std::string>(
			"SELECT document from documents_words dw " 
			 "join words w on w.id = dw.word_id " 
			 "join documents d on d.id = dw.document_id where word = " + word + 
			" order by quantity DESC limit 10;"
		);
		tx.commit();

		for (std::tuple<std::string> value : values)
		{
			result.push_back(std::get<0>(value));
		}
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	return result;
}

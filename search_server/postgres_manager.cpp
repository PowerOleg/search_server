#include "postgres_manager.h"

Postgres_manager::Postgres_manager(std::string host, std::string port, std::string dbname, std::string username, std::string password) : connection{ "host=" + host + " port=" + port + " dbname =" + dbname + " user = " + username + " password =" + password}
{
	std::cout << "Connection is successful" << std::endl;
}

std::vector<std::string> Postgres_manager::Select10Urls(const std::string word)
{
	std::vector<std::string> result;

	try
	{
		pqxx::work tx2{ connection };
		std::string url = tx2.query_value<std::string>(
			"SELECT document_id FROM documents_words where word_id = 1;"
		);
		tx2.commit();
		std::cout << "select query" << std::endl;
		result.push_back(url);
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	return result;

	//std::string document_id = std::to_string(postgres_count);
	//std::string document_id = "";
//try
//{
//	pqxx::work tx2{ connection };
//	document_id = tx2.query_value<std::string>(
//		"SELECT d.id FROM documents d where d.document = " + url + ";"
//	);
//	tx2.commit();
//	std::cout << "select query" << std::endl;
//}
//catch (const std::exception& e)
//{
//	std::cout << e.what() << std::endl;
//}
//std::cout << "document_id: " << document_id << std::endl;
}

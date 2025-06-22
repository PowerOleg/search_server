#include "postgres_manager.h"

Postgres_manager::Postgres_manager(std::string host, std::string port, std::string dbname, std::string username, std::string password) : connection{ "host=" + host + " port=" + port + " dbname =" + dbname + " user = " + username + " password =" + password}
{
	std::cout << "Connection is successful" << std::endl;
}

std::map<std::string, std::pair<std::string, int>> Postgres_manager::FindWordsOccurrance(const std::vector<std::string> &words)
{
	std::map<std::string, std::pair<std::string, int>> document_word_quantity;
	std::cout << "searching words in database" << std::endl;
	for (size_t i = 0; i < words.size(); i++)
	{
		std::string word = words.at(i);
		try
		{
			pqxx::work tx{ connection };
			auto values = tx.query<std::string, std::string, int>(
				"SELECT document, word, quantity FROM documents_words dw "
				"JOIN words w ON w.id = dw.word_id "
				"JOIN documents d ON d.id = dw.document_id where word ILIKE '" + word + "';"
			);

			for (const std::tuple<std::string, std::string, int> &value : values)
			{
				std::string document = std::get<0>(value);
				int word_quantity = std::get<2>(value);//std::atoi(std::get<2>(value).c_str());
				std::pair<std::string, int> document_quantity{ std::get<1>(value), word_quantity };
				document_word_quantity.insert({ document , document_quantity });
				std::cout << "document: " << document << " word: " << document_quantity.first << " quantity: " << document_quantity.second << std::endl;
			}
		}
		catch (const std::exception& e)
		{
			std::cout << e.what() << std::endl;
			std::pair<std::string, int> pair{ "502", 502 };
			document_word_quantity.insert({ "502", pair });
		}
	}

	return document_word_quantity;
}

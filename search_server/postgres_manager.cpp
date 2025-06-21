#include "postgres_manager.h"

Postgres_manager::Postgres_manager(std::string host, std::string port, std::string dbname, std::string username, std::string password) : connection{ "host=" + host + " port=" + port + " dbname =" + dbname + " user = " + username + " password =" + password}
{
	std::cout << "Connection is successful" << std::endl;
}

std::map<std::string, std::pair<std::string, int>> Postgres_manager::FindWordsOccurrance(const std::vector<std::string> &words)
{
	std::map<std::string, std::pair<std::string, int>> document_word_occurrance;

	for (size_t i = 0; i < words.size(); i++)
	{
		std::string word = "";
		try
		{
			pqxx::work tx{ connection };
			auto values = tx.query<std::string, std::string, std::string>(
				"SELECT document, word, quantity FROM documents_words dw "
				"JOIN words w ON w.id = dw.word_id "
				"JOIN documents d ON d.id = dw.document_id where word ILIKE '" + word + "';"
				//+ "' ORDER BY quantity DESC LIMIT "+ quantity + ";"
			);

			for (const std::tuple<std::string, std::string, std::string> &value : values)
			{
				int word_quantity = std::atoi(std::get<2>(value).c_str());
				std::pair<std::string, int> pair{ std::get<1>(value), word_quantity };
				document_word_occurrance.insert({ std::get<0>(value) , pair });
			}
		}
		catch (const std::exception& e)
		{
			std::cout << e.what() << std::endl;
			result.put(result.begin(), "502");
		}
	}


	

	return result;
}

#include "postgres_manager.h"

Postgres_manager::Postgres_manager(std::string host, std::string port, std::string dbname, std::string username, std::string password) : connection{ "host=" + host + " port=" + port + " dbname =" + dbname + " user = " + username + " password =" + password}
{
	std::cout << "Connection is successful" << std::endl;
}

std::map<std::string, std::vector<std::pair<std::string, int>>> Postgres_manager::FindWordsOccurrance(const std::vector<std::string> &words)
{
	
	std::map<std::string, std::vector<std::pair<std::string, int>>> document_words_quantity;
	
	
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
				if (document_words_quantity.contains(document))
				{
					std::vector<std::pair<std::string, int>> &old_vector = document_words_quantity[document];
					int quantity = std::get<2>(value);
					std::pair<std::string, int> word_quantity{ std::get<1>(value), quantity };
					old_vector.push_back(word_quantity);
					std::cout << "OLD document: " << "document: " << document << "added word: " << word_quantity.first << " quantity: " << word_quantity.second << std::endl;
					//document_words_quantity.insert({ document, old_vector });
				}
				else
				{
					std::vector<std::pair<std::string, int>> new_vector;
					int word_quantity = std::get<2>(value);//std::atoi(std::get<2>(value).c_str());
					std::pair<std::string, int> document_quantity{ std::get<1>(value), word_quantity };
					new_vector.push_back(document_quantity);
					std::cout << "NEW document: " << "document: " << document << " word: " << document_quantity.first << " quantity: " << document_quantity.second << std::endl;
					document_words_quantity.insert({ document, new_vector });
				}

			}
		}
		catch (const std::exception& e)
		{
			std::cout << e.what() << std::endl;
			std::map<std::string, std::vector<std::pair<std::string, int>>> empty_map;
			document_words_quantity = std::move(empty_map);
		}
	}

	return document_words_quantity;
}

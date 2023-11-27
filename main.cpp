#include <iostream>
#include <pqxx/pqxx>
#include <string>
#include <vector>


class ClientManager {
public:
    // Метод для установки соединения с базой данных
    void connect(const std::string& connString) {
        try {
            connection = new pqxx::connection(connString);
            std::cout << "Успешное подключение к базе данных" << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Ошибка подключения к базе данных: " << e.what() << std::endl;
        }
    }

    // Метод для создания структуры БД (таблиц)
    void createDatabaseStructure() {
        pqxx::work txn(*connection);
        try {
            txn.exec("CREATE TABLE IF NOT EXISTS clients ("
                     "id SERIAL PRIMARY KEY,"
                     "first_name TEXT NOT NULL,"
                     "last_name TEXT NOT NULL,"
                     "email TEXT NOT NULL"
                     ")");

            txn.exec("CREATE TABLE IF NOT EXISTS phones ("
                     "id SERIAL PRIMARY KEY,"
                     "client_id INTEGER REFERENCES clients(id) ON DELETE CASCADE,"
                     "phone_number TEXT NOT NULL"
                     ")");

            txn.commit();
            std::cout << "Структура БД создана" << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Ошибка создания структуры БД: " << e.what() << std::endl;
                         txn.abort();
        }
    }

    // Метод для добавления нового клиента
    void addNewClient(const std::string& firstName, const std::string& lastName, const std::string& email) {
        pqxx::work txn(*connection);
        try {
            txn.exec_params("INSERT INTO clients (first_name, last_name, email) VALUES ($1, $2, $3)",
                            firstName, lastName, email);
            txn.commit();
            std::cout << "Новый клиент добавлен" << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Ошибка добавления нового клиента: " << e.what() << std::endl;
            txn.abort();
        }
    }

    // Метод для добавления телефона для существующего клиента
    void addPhoneForClient(int clientId, const std::string& phone) {
        pqxx::work txn(*connection);
        try {
            txn.exec_params("INSERT INTO phones (client_id, phone_number) VALUES ($1, $2)",
                            clientId, phone);
            txn.commit();
            std::cout << "Телефон добавлен для клиента" << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Ошибка добавления телефона для клиента: " << e.what() << std::endl;
            txn.abort();
        }
    }

    // Метод для изменения данных о клиенте
    void updateClientData(int clientId, const std::string& firstName, const std::string& lastName, const std::string& email) {
        pqxx::work txn(*connection);
        try {
            txn.exec_params("UPDATE clients SET first_name = $1, last_name = $2, email = $3 WHERE id = $4",
                            firstName, lastName, email, clientId);
            txn.commit();
            std::cout << "Данные о клиенте обновлены" << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Ошибка изменения данных о клиенте: " << e.what() << std::endl;
            txn.abort();
        }
    }

    // Метод для удаления телефона у существующего клиента
    void deletePhoneForClient(int clientId, const std::string& phone) {
        pqxx::work txn(*connection);
        try {
            pqxx::result res = txn.exec_params("SELECT id FROM phones WHERE client_id = $1 AND phone_number = $2", clientId, phone);
            if (res.empty()) {
                std::cerr << "Указанный телефон не найден у клиента" << std::endl;
                txn.abort();
                return;
            }

            int phoneId = res[0][0].as<int>();
            txn.exec_params("DELETE FROM phones WHERE id = $1", phoneId);
            txn.commit();
            std::cout << "Телефон удален у клиента" << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Ошибка удаления телефона у клиента: " << e.what() << std::endl;
            txn.abort();
        }
    }


    // Метод для удаления существующего клиента
    void deleteClient(int clientId) {
        pqxx::work txn(*connection);
        try {
            txn.exec_params("DELETE FROM phones WHERE client_id = $1", clientId); // Удаляем все телефоны этого клиента из таблицы телефонов
            txn.exec_params("DELETE FROM clients WHERE id = $1", clientId); // Затем удаляем самого клиента
            txn.commit();
            std::cout << "Клиент удален" << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Ошибка удаления клиента: " << e.what() << std::endl;
                                                                                 txn.abort();
        }
    }

    // Метод для поиска клиента по его данным
    int findClient(const std::string& searchQuery) {
        pqxx::work txn(*connection);
        try {
            pqxx::result res = txn.exec("SELECT clients.id, clients.first_name, clients.last_name, clients.email, phones.phone_number "
                                        "FROM clients LEFT JOIN phones ON clients.id = phones.client_id "
                                        "WHERE "
                                        "first_name ILIKE '%" + searchQuery + "%' OR "
                                                        "last_name ILIKE '%" + searchQuery + "%' OR "
                                                        "email ILIKE '%" + searchQuery + "%' OR "
                                                        "phone_number ILIKE '%" + searchQuery + "%'");

            if (res.empty()) {
                std::cout << "Клиент с указанными данными не найден" << std::endl;
                return -1;
            } else {
                for (const auto& row : res) {
                    std::cout << "ID: " << row[0].as<int>()
                              << ", Имя: " << row[1].as<std::string>()
                              << ", Фамилия: " << row[2].as<std::string>()
                              << ", Email: " << row[3].as<std::string>();
                    if (!row[4].is_null()) {
                        std::cout << ", Телефон: " << row[4].as<std::string>() << std::endl;
                    } else {
                        std::cout << ", Телефон: Не указан" << std::endl;
                    }
                }
                return res[0][0].as<int>();
            }
        } catch (const std::exception &e) {
            std::cerr << "Ошибка поиска клиента: " << e.what() << std::endl;
            txn.abort();
            return -1;
        }
    }


    // Метод для закрытия соединения с базой данных
    void disconnect() {
        connection->close();
        delete connection;
    }


private:
    pqxx::connection* connection;
};

int main() {
    ClientManager clientManager;
    clientManager.connect("dbname=netologydb user=user password=pass hostaddr=127.0.0.1 port=5432");
    clientManager.createDatabaseStructure();
    clientManager.addNewClient("Иван", "Иванов", "ivan@example.com");

    int clientId = clientManager.findClient("ivan@example.com");

    clientManager.addPhoneForClient(clientId, "+1234567890");
    // Изменение данных о клиенте
    clientManager.updateClientData(clientId, "Петр", "Петров", "petr@example.com");
    // Поиск клиента
    clientManager.findClient("Петров");
    // Удаление телефона у клиента
    clientManager.deletePhoneForClient(clientId, "+1234567890");
    // Удаление клиента
    clientManager.deleteClient(clientId);
    // Закрытие соединения с базой данных
    clientManager.disconnect();

    return 0;
}
